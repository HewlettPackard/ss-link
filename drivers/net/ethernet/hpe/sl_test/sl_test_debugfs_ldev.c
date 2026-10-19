// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2026 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/err.h>

#include "sl_asic.h"
#include "sl_ldev.h"
#include "log/sl_log.h"
#include "sl_test_common.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_llr.h"
#include "sl_test_debugfs_mac.h"

#define LOG_BLOCK "ldev"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *ldev_dir;
static struct sl_ldev ldev;

enum ldev_cmd_index {
	LDEV_NEW_CMD,
	LDEV_DEL_CMD,
	NUM_CMDS,
};

static struct cmd_entry ldev_cmd_list[]  = {
	[LDEV_NEW_CMD] = { .cmd = "ldev_new", .desc = "create ldev object", .call = sl_test_ldev_new },
	[LDEV_DEL_CMD] = { .cmd = "ldev_del", .desc = "delete ldev object", .call = sl_test_ldev_del },
};

static void sl_test_ldev_init(void)
{
	ldev.magic = SL_LDEV_MAGIC;
	ldev.ver   = SL_LDEV_VER;
	ldev.size  = sizeof(ldev);
	ldev.num   = 0;
}

static int sl_test_ldev_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, ldev_cmd_list, ARRAY_SIZE(ldev_cmd_list));
}

static int sl_test_ldev_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_ldev_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_ldev_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_ldev_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static ssize_t sl_test_ldev_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	int     call_rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "partial cmd_write");
		return 0;
	}

	if (!size || size >= sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf) - 1, pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	rtn = sl_test_debugfs_cmd_call(cmd_buf, ldev_cmd_list, ARRAY_SIZE(ldev_cmd_list), &call_rtn);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write failed (cmd_buf = %s) [%d]", cmd_buf, rtn);
		return rtn;
	}

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "cmd_write succeeded (cmd_buf = %s, call_rtn = %d)", cmd_buf, call_rtn);

	if (call_rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write call failed (cmd_buf = %s) [%d]", cmd_buf, call_rtn);
		return call_rtn;
	}

	return size;
}

static const struct file_operations sl_test_ldev_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_ldev_cmd_write,
};

struct sl_ldev *sl_test_ldev_get(void)
{
	return &ldev;
}

int sl_test_debugfs_ldev_create(struct dentry *top_dir)
{
	int rtn;
	struct dentry *dentry;

	ldev_dir = debugfs_create_dir("ldev", top_dir);
	rtn = sl_test_debugfs_entry_check(ldev_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "ldev debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_ldev_init();

	SL_TEST_DEBUGFS_CREATE(u8, "num", ldev_dir, &ldev.num);

	dentry = debugfs_create_file("cmds", 0644, ldev_dir, NULL, &sl_test_ldev_cmds_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmds debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	dentry = debugfs_create_file("cmd", 0644, ldev_dir, NULL, &sl_test_ldev_cmd_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

u8 sl_test_debugfs_ldev_num_get(void)
{
	return ldev.num;
}

int sl_test_ldev_new(void)
{
	struct sl_ctrl_ldev *ctrl_ldev;

	//TODO: Create ldev object
	/* We rely on the ldev created by the client. Don't create new */
	ctrl_ldev = sl_test_ctrl_ldev_get(sl_test_debugfs_ldev_num_get());
	if (!ctrl_ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "ldev_new ctrl_ldev_get failed");
		return -ENODEV;
	}

	return 0;
}

int sl_test_ldev_del(void)
{
	//TODO: Delete ldev object.
	return 0;
}

int sl_test_ldev_exit(void)
{
	u8 ldev_num;
	u8 lgrp_num;
	u8 link_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num) {
		for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
			for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
				sl_test_llr_remove(ldev_num, lgrp_num, link_num);
				sl_test_mac_remove(ldev_num, lgrp_num, link_num);
				sl_test_link_remove(ldev_num, lgrp_num, link_num);
			}
		}
	}

	return 0;
}
