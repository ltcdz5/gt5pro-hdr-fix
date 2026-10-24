// SPDX-License-Identifier: GPL-2.0-only
// sidecar42: HDR 指纹匹配时，用 /proc 提供的表替换 payload
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define GAMUT_SZ 39536
#define HDR_FNV  0xb9b4c7e9u

struct cp_cfg_head {
	void *payload;
	u32 len;
};

static u8 *buf;
static int have;
static unsigned long calls, hdr_seen, applied;
static struct proc_dir_entry *pe;

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

	if (!hw || !hw->payload || hw->len != GAMUT_SZ)
		return 0;
	calls++;
	if (fnv32((const u8 *)hw->payload, GAMUT_SZ) != HDR_FNV)
		return 0;
	hdr_seen++;
	if (!have)
		return 0;
	memcpy(hw->payload, buf, GAMUT_SZ);
	applied++;
	return 0;
}

static ssize_t rd(struct file *f, char __user *ub, size_t n, loff_t *pos)
{
	size_t left;

	if (!have || *pos >= GAMUT_SZ)
		return 0;
	left = GAMUT_SZ - *pos;
	if (n > left)
		n = left;
	if (copy_to_user(ub, buf + *pos, n))
		return -EFAULT;
	*pos += n;
	return n;
}

static ssize_t wr(struct file *f, const char __user *ub, size_t n, loff_t *pos)
{
	size_t left;

	if (*pos >= GAMUT_SZ)
		return 0;
	left = GAMUT_SZ - *pos;
	if (n > left)
		n = left;
	if (copy_from_user(buf + *pos, ub, n))
		return -EFAULT;
	*pos += n;
	if (*pos >= GAMUT_SZ)
		have = 1;
	return n;
}

static const struct proc_ops pops = {
	.proc_read = rd,
	.proc_write = wr,
};

static struct kprobe kp = {
	.symbol_name = "reg_dmav2_setup_dspp_3d_gamutv43",
	.pre_handler = pre,
};

static int __init s42_init(void)
{
	int rc;

	buf = kzalloc(GAMUT_SZ, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	pe = proc_create("sdehdrlut", 0644, NULL, &pops);
	if (!pe) {
		kfree(buf);
		return -ENOMEM;
	}
	rc = register_kprobe(&kp);
	pr_info("sidecar42: rc=%d\n", rc);
	return rc;
}

static void __exit s42_exit(void)
{
	unregister_kprobe(&kp);
	remove_proc_entry("sdehdrlut", NULL);
	kfree(buf);
	pr_info("sidecar42: calls=%lu hdr=%lu applied=%lu\n", calls, hdr_seen, applied);
}
module_init(s42_init);
module_exit(s42_exit);
MODULE_LICENSE("GPL");
