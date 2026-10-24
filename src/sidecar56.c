// SPDX-License-Identifier: GPL-2.0-only
// sidecar56: 纯指纹白名单。只认两张已知 HDR 表，其余一律放行。
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>

#define GAMUT_SZ 39536

/* SDR 指纹（实测稳定，永远放行） */
#define SDR_FNV  0x8777be4fu
/* 横屏 HDR 指纹 */
#define LAND_FNV 0xb9b4c7e9u
/* 竖屏 HDR 指纹（可能随会话变化，同时用白点兜底） */
#define PORT_FNV1 0x1333d2b9u
#define PORT_FNV2 0x5f8c2761u

struct cp_cfg_head { void *payload; u32 len; };

struct tbl {
	u8 *buf;
	int have;
	unsigned long hits;
	struct proc_dir_entry *pe;
};

static struct tbl land, port;
static struct proc_dir_entry *stat_pe;
static int enable = 1;
module_param(enable, int, 0600);

static unsigned long calls, others;
static u32 last_fp;

static u32 fnv32(const u8 *p, u32 n)
{
	u32 h = 2166136261u;
	u32 i;

	for (i = 0; i < n; i++) {
		h ^= p[i];
		h *= 16777619u;
	}
	return h;
}

static int pre(struct kprobe *p, struct pt_regs *regs)
{
	struct cp_cfg_head *hw = (struct cp_cfg_head *)regs->regs[1];
	u8 *d;
	u32 fp;

	if (!hw || !hw->payload || hw->len != GAMUT_SZ)
		return 0;

	calls++;
	if (!enable)
		return 0;

	d = (u8 *)hw->payload;
	fp = fnv32(d, GAMUT_SZ);
	last_fp = fp;

	/* 1. SDR 指纹：最高优先级，永远放行 */
	if (fp == SDR_FNV) {
		others++;
		return 0;
	}

	/* 2. 横屏 HDR：指纹白名单 */
	if (land.have && fp == LAND_FNV) {
		memcpy(d, land.buf, GAMUT_SZ);
		land.hits++;
		return 0;
	}

	/* 3. 竖屏 HDR：已知指纹，或白点蓝明显偏低（此时 SDR 已被排除） */
	if (port.have) {
		if (fp == PORT_FNV1 || fp == PORT_FNV2) {
			memcpy(d, port.buf, GAMUT_SZ);
			port.hits++;
			return 0;
		}
		{
			const u8 *p = d + 204 + (16*289 + 16*17 + 16)/4*8
				    + ((16*289 + 16*17 + 16)%4)*1229*8;
			u32 c0 = *(const u32 *)(p + 4);
			u32 c1 = *(const u32 *)p & 0xFFF;

			if (c0 && (unsigned long long)c1 * 100 <
				  (unsigned long long)c0 * 97) {
				memcpy(d, port.buf, GAMUT_SZ);
				port.hits++;
				return 0;
			}
		}
	}

	others++;
	return 0;
}

static ssize_t rd_tbl(struct file *f, char __user *ub, size_t n, loff_t *pos)
{
	struct tbl *t = pde_data(file_inode(f));
	size_t left;

	if (!t || !t->have || *pos >= GAMUT_SZ)
		return 0;
	left = GAMUT_SZ - *pos;
	if (n > left)
		n = left;
	if (copy_to_user(ub, t->buf + *pos, n))
		return -EFAULT;
	*pos += n;
	return n;
}

static ssize_t wr_tbl(struct file *f, const char __user *ub, size_t n, loff_t *pos)
{
	struct tbl *t = pde_data(file_inode(f));
	size_t left;

	if (!t || *pos >= GAMUT_SZ)
		return 0;
	left = GAMUT_SZ - *pos;
	if (n > left)
		n = left;
	if (copy_from_user(t->buf + *pos, ub, n))
		return -EFAULT;
	*pos += n;
	if (*pos >= GAMUT_SZ)
		t->have = 1;
	return n;
}

static ssize_t rd_stat(struct file *f, char __user *ub, size_t n, loff_t *pos)
{
	char buf[128];
	int len;

	if (*pos)
		return 0;
	len = snprintf(buf, sizeof(buf),
		"calls=%lu land=%lu port=%lu other=%lu fp=%#010x en=%d\n",
		calls, land.hits, port.hits, others, last_fp, enable);
	if (n < len)
		len = n;
	if (copy_to_user(ub, buf, len))
		return -EFAULT;
	*pos += len;
	return len;
}

static const struct proc_ops pops_tbl = {
	.proc_read = rd_tbl,
	.proc_write = wr_tbl,
};
static const struct proc_ops pops_stat = {
	.proc_read = rd_stat,
};

static struct kprobe kp = {
	.symbol_name = "reg_dmav2_setup_dspp_3d_gamutv43",
	.pre_handler = pre,
};

static void free_all(void)
{
	if (land.pe) {
		remove_proc_entry("sdehdrlut_land", NULL);
		land.pe = NULL;
	}
	if (port.pe) {
		remove_proc_entry("sdehdrlut_port", NULL);
		port.pe = NULL;
	}
	if (stat_pe) {
		remove_proc_entry("sdehdrstat", NULL);
		stat_pe = NULL;
	}
	kfree(land.buf);
	kfree(port.buf);
	land.buf = port.buf = NULL;
}

static int __init s56_init(void)
{
	int rc;

	land.buf = kzalloc(GAMUT_SZ, GFP_KERNEL);
	port.buf = kzalloc(GAMUT_SZ, GFP_KERNEL);
	if (!land.buf || !port.buf) {
		free_all();
		return -ENOMEM;
	}

	land.pe = proc_create_data("sdehdrlut_land", 0644, NULL, &pops_tbl, &land);
	port.pe = proc_create_data("sdehdrlut_port", 0644, NULL, &pops_tbl, &port);
	stat_pe = proc_create("sdehdrstat", 0444, NULL, &pops_stat);
	if (!land.pe || !port.pe || !stat_pe) {
		free_all();
		return -ENOMEM;
	}

	rc = register_kprobe(&kp);
	if (rc) {
		free_all();
		return rc;
	}

	pr_info("sidecar56: loaded\n");
	return 0;
}

static void __exit s56_exit(void)
{
	unregister_kprobe(&kp);
	free_all();
	pr_info("sidecar56: unloaded calls=%lu land=%lu port=%lu other=%lu\n",
		calls, land.hits, port.hits, others);
}
module_init(s56_init);
module_exit(s56_exit);
MODULE_LICENSE("GPL");
