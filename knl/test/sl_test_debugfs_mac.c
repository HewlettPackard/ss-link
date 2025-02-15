// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_link.h>
#include <linux/sl_mac.h>

#include "sl_lgrp.h"
#include "sl_mac.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_mac.h"
#include "sl_test_common.h"

#define LOG_BLOCK "mac"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *mac_dir;
struct sl_mac         mac;

enum mac_cmd_index {
	MAC_NEW_CMD,
	MAC_DEL_CMD,
	MAC_TX_START_CMD,
	MAC_RX_START_CMD,
	NUM_CMDS,
};

static struct cmd_entry mac_cmd_list[]  = {
	[MAC_NEW_CMD]      = { .cmd = "mac_new",      .desc = "create mac object"                  },
	[MAC_DEL_CMD]      = { .cmd = "mac_del",      .desc = "delete mac object"                  },
	[MAC_TX_START_CMD] = { .cmd = "mac_tx_start", .desc = "start MAC tx"                       },
	[MAC_RX_START_CMD] = { .cmd = "mac_rx_start", .desc = "start MAC rx"                       },
};

#define MAC_CMD_MATCH(_index, _str) (strncmp((_str), mac_cmd_list[_index].cmd, strlen(mac_cmd_list[_index].cmd)) == 0)

static int sl_test_mac_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, mac_cmd_list, ARRAY_SIZE(mac_cmd_list));
}

static int sl_test_mac_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_mac_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_mac_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_mac_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static ssize_t sl_test_mac_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
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

	match_found = MAC_CMD_MATCH(MAC_NEW_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_new();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_new failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = MAC_CMD_MATCH(MAC_DEL_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_del();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_del failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = MAC_CMD_MATCH(MAC_TX_START_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_tx_start();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_tx_start failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = MAC_CMD_MATCH(MAC_RX_START_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_rx_start();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_rx_start failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
}

static const struct file_operations sl_test_mac_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_mac_cmd_write,
};

static void sl_test_mac_init(void)
{
	mac.magic    = SL_MAC_MAGIC;
	mac.ver      = SL_MAC_VER;
	mac.ldev_num = sl_test_debugfs_ldev_num_get();
	mac.lgrp_num = sl_test_debugfs_lgrp_num_get();
	mac.num      = 0;
}

static struct sl_mac *sl_test_mac_get(void)
{
	mac.ldev_num = sl_test_debugfs_ldev_num_get();
	mac.lgrp_num = sl_test_debugfs_lgrp_num_get();

	return &mac;
}

int sl_test_debugfs_mac_create(struct dentry *top_dir)
{
	struct dentry *dentry;

	mac_dir = debugfs_create_dir("mac", top_dir);
	if (!mac_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"mac debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_mac_init();

	debugfs_create_u8("num", 0644, mac_dir, &mac.num);

	dentry = debugfs_create_file("cmds", 0644, mac_dir, NULL, &sl_test_mac_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, mac_dir, NULL, &sl_test_mac_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

static u8 sl_test_debugfs_mac_num_get(void)
{
	return mac.num;
}

int sl_test_mac_new(void)
{
	struct kobject *kobj;
	struct sl_lgrp *lgrp;

	lgrp = sl_test_lgrp_get();
	kobj = sl_test_lgrp_kobj_get(sl_test_debugfs_ldev_num_get(), lgrp->num);

	return IS_ERR(sl_mac_new(lgrp, sl_test_debugfs_mac_num_get(), kobj));
}

int sl_test_mac_del(void)
{
	sl_mac_del(sl_test_mac_get());

	return 0;
}

int sl_test_mac_tx_start(void)
{
	return sl_mac_tx_start(sl_test_mac_get());
}

int sl_test_mac_rx_start(void)
{
	return sl_mac_rx_start(sl_test_mac_get());
}
