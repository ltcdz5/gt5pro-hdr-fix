// SPDX-License-Identifier: GPL-2.0-only
// sidecar55: 白名单 + 内容变化检测；只有新表才替换，避免反复覆盖
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>

#define GAMUT_SZ 39536
#define COL_OFF  204
#define NCOL     (4 * 1229)

struct cp_cfg_head { void *payload; u32 len; };
struct col { u32 c2_c1; u32 c0; };

static u8 *land_buf, *port_buf;
static int land_have, port_have;
static int enable = 1;
module_param(enable, int, 0600);

static unsigned long calls, land_hits, port_hits, others;
static struct proc_dir_entry *pe_land, *pe_port, *pe_stat;
static u32 lw[3], lfp;

static u32 fnv32(const u8 *p, u32 n)
{
	u32 h = 2166136261u; u32 i;

	for (i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
	return h;
}

static int idx_of(int r, int g, int b) { return g*289 + b*17 + r; }

static void rd_rgb(const u8 *p, int n, u32 *R, u32 *B, u32 *G)
{
	const struct col *c = (const struct col *)(p + COL_OFF + (n/4)*8 + (n%4)*1229*8);

	*R = c->c0;
	*B = c->c2_c1 & 0xFFF;
	*G = (c->c2_c1 >> 16) & 0xFFF;
}

static int pre(struct kprobe *p, struct pt_regs *regs)
{
	struct cp_cfg_head *hw = (struct cp_cfg_head *)regs->regs[1];
	u8 *d;
	u32 fp, wR, wB, wG;

	if (!hw || !hw->payload || hw->len != GAMUT_SZ)
		return 0;
	calls++;
	if (!enable)
		return 0;
	d = (u8 *)hw->payload;
	fp = fnv32(d, GAMUT_SZ);
	rd_rgb(d, idx_of(16,16,16), &wR, &wB, &wG);
	lw[0] = wR; lw[1] = wB; lw[2] = wG; lfp = fp;

	/* 白名单：横屏 HDR（蓝正常且指纹已知） */
	if (land_have && fp == 0xb9b4c7e9u) {
		memcpy(d, land_buf, GAMUT_SZ);
		land_hits++;
		return 0;
	}
	/* 白名单：竖屏 HDR（蓝明显偏低 B/R<0.97） */
	if (port_have && wR && (unsigned long long)wB * 100 < (unsigned long long)wR * 97) {
		memcpy(d, port_buf, GAMUT_SZ);
		port_hits++;
		return 0;
	}
	others++;
	return 0;
}

static ssize_t rd_tbl(struct file *f, char __user *ub, size_t n, loff_t *pos)
{
	u8 *b = pde_data(file_inode(f));
	size_t left;

	if (!b || *pos >= GAMUT_SZ) return 0;
	left = GAMUT_SZ - *pos;
	if (n > left) n = left;
	if (copy_to_user(ub, b + *pos, n)) return -EFAULT;
	*pos += n;
	return n;
}

static ssize_t wr_tbl(struct file *f, const char __user *ub, size_t n, loff_t *pos)
{
	struct inode *inode = file_inode(f);
	u8 *b = pde_data(inode);
	size_t left;

	if (!b || *pos >= GAMUT_SZ) return 0;
	left = GAMUT_SZ - *pos;
	if (n > left) n = left;
	if (copy_from_user(b + *pos, ub, n)) return -EFAULT;
	*pos += n;
	if (*pos >= GAMUT_SZ) {
		if (b == land_buf) land_have = 1;
		else port_have = 1;
	}
	return n;
}

static ssize_t rd_stat(struct file *f, char __user *ub, size_t n, loff_t *pos)
{
	char buf[160];
	int len;

	if (*pos) return 0;
	len = snprintf(buf, sizeof(buf),
		"calls=%lu land=%lu port=%lu other=%lu fp=%#010x white=%u/%u/%u en=%d\n",
		calls, land_hits, port_hits, others, lfp, lw[0], lw[1], lw[2], enable);
	if (n < len) len = n;
	if (copy_to_user(ub, buf, len)) return -EFAULT;
	*pos += len;
	return len;
}

static const struct proc_ops pops_tbl = { .proc_read = rd_tbl, .proc_write = wr_tbl };
static const struct proc_ops pops_stat = { .proc_read = rd_stat };

static struct kprobe kp = {
	.symbol_name = "reg_dmav2_setup_dspp_3d_gamutv43",
	.pre_handler = pre,
};

static int __init s55_init(void)
{
	int rc;

	land_buf = kzalloc(GAMUT_SZ, GFP_KERNEL);
	port_buf = kzalloc(GAMUT_SZ, GFP_KERNEL);
	if (!land_buf || !port_buf) return -ENOMEM;
	pe_land = proc_create_data("sdehdrlut_land", 0644, NULL, &pops_tbl, land_buf);
	pe_port = proc_create_data("sdehdrlut_port", 0644, NULL, &pops_tbl, port_buf);
	pe_stat = proc_create("sdehdrstat", 0444, NULL, &pops_stat);
	if (!pe_land || !pe_port || !pe_stat) return -ENOMEM;
	rc = register_kprobe(&kp);
	pr_info("sidecar55: rc=%d\n", rc);
	return rc;
}

static void __exit s55_exit(void)
{
	unregister_kprobe(&kp);
	remove_proc_entry("sdehdrlut_land", NULL);
	remove_proc_entry("sdehdrlut_port", NULL);
	remove_proc_entry("sdehdrstat", NULL);
	kfree(land_buf);
	kfree(port_buf);
	pr_info("sidecar55: calls=%lu land=%lu port=%lu other=%lu\n",
		calls, land_hits, port_hits, others);
}
module_init(s55_init);
module_exit(s55_exit);
MODULE_LICENSE("GPL");
