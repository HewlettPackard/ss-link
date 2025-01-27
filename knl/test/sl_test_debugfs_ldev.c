// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include "linux/kernel.h"
#include <linux/debugfs.h>

#include "sl_ldev.h"
#include "log/sl_log.h"
#include "sl_test_common.h"
#include "sl_test_debugfs_ldev.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *ldev_dir;
static struct sl_ldev ldev;

enum ldev_cmd_index {
	LDEV_NEW_CMD,
	LDEV_DEL_CMD,
	NUM_CMDS,
};

static struct cmd_entry ldev_cmd_list[]  = {
	[LDEV_NEW_CMD] = { .cmd = "ldev_new", .desc = "create ldev object" },
	[LDEV_DEL_CMD] = { .cmd = "ldev_del", .desc = "delete ldev object" },
};

#define LDEV_CMD_MATCH(_index, _str) \
	(strncmp((_str), ldev_cmd_list[_index].cmd, strlen(ldev_cmd_list[_index].cmd)) == 0)

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
	ssize_t len;
	char    cmd_buf[CMD_LEN];
	bool    match_found;

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (size > sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf), pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	match_found = LDEV_CMD_MATCH(LDEV_NEW_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_ldev_new();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_ldev_new failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LDEV_CMD_MATCH(LDEV_DEL_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_ldev_del();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_ldev_del failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
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
	struct dentry *dentry;

	ldev_dir = debugfs_create_dir("ldev", top_dir);
	if (!ldev_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"ldev debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_ldev_init();

	debugfs_create_u8("num", 0644, ldev_dir, &ldev.num);

	dentry = debugfs_create_file("cmds", 0644, ldev_dir, NULL, &sl_test_ldev_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, ldev_dir, NULL, &sl_test_ldev_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

u8 sl_test_debugfs_ldev_num_get(void)
{
	return ldev.num;
}

int sl_test_ldev_new(void)
{
	struct sl_ctl_ldev *ctl_ldev;

	//TODO: Create ldev object
	/* We rely on the ldev created by the client. Don't create new */
	ctl_ldev = sl_test_ctl_ldev_get(sl_test_debugfs_ldev_num_get());
	if (!ctl_ldev) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "ldev_new ctl_ldev_get failed");
		return -ENODEV;
	}

	return 0;
}

int sl_test_ldev_del(void)
{
	//TODO: Delete ldev object.
	return 0;
}
