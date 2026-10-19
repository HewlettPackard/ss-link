// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/kobject.h>

#include <linux/hpe/sl/sl_link.h>
#include <linux/hpe/sl/sl_mac.h>
#include <linux/hpe/sl/sl_test.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
#include "sl_mac.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_mac.h"
#include "sl_test_common.h"
#include "sl_test_pdev.h"

#define LOG_BLOCK "mac"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *mac_dir;
static struct sl_mac  mac;

enum mac_cmd_index {
	MAC_NEW_CMD,
	MAC_DEL_CMD,
	MAC_TX_START_CMD,
	MAC_RX_START_CMD,
	MAC_TX_STOP_CMD,
	MAC_RX_STOP_CMD,
	NUM_CMDS,
};

static struct cmd_entry mac_cmd_list[]  = {
	[MAC_NEW_CMD]      = { .cmd = "mac_new",     .desc = "create mac object", .call = sl_test_mac_new      },
	[MAC_DEL_CMD]      = { .cmd = "mac_del",     .desc = "delete mac object", .call = sl_test_mac_del      },
	[MAC_TX_START_CMD] = { .cmd = "mac_tx_start", .desc = "start MAC tx",     .call = sl_test_mac_tx_start },
	[MAC_RX_START_CMD] = { .cmd = "mac_rx_start", .desc = "start MAC rx",     .call = sl_test_mac_rx_start },
	[MAC_TX_STOP_CMD]  = { .cmd = "mac_tx_stop",  .desc = "stop MAC tx",      .call = sl_test_mac_tx_stop  },
	[MAC_RX_STOP_CMD]  = { .cmd = "mac_rx_stop",  .desc = "stop MAC rx",      .call = sl_test_mac_rx_stop  },
};

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

	rtn = sl_test_debugfs_cmd_call(cmd_buf, mac_cmd_list, ARRAY_SIZE(mac_cmd_list), &call_rtn);
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

static const struct file_operations sl_test_mac_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_mac_cmd_write,
};

static void sl_test_mac_init(struct sl_mac *mac, u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	mac->magic    = SL_MAC_MAGIC;
	mac->ver      = SL_MAC_VER;
	mac->ldev_num = ldev_num;
	mac->lgrp_num = lgrp_num;
	mac->num      = link_num;
}

static struct sl_mac *sl_test_mac_get(void)
{
	mac.ldev_num = sl_test_debugfs_ldev_num_get();
	mac.lgrp_num = sl_test_debugfs_lgrp_num_get();

	return &mac;
}

int sl_test_debugfs_mac_create(struct dentry *top_dir)
{
	int            rtn;
	struct dentry *dentry;

	mac_dir = debugfs_create_dir("mac", top_dir);
	rtn = sl_test_debugfs_entry_check(mac_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "mac debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_mac_init(&mac, sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 0);

	SL_TEST_DEBUGFS_CREATE(u8, "num", mac_dir, &mac.num);

	dentry = debugfs_create_file("cmds", 0644, mac_dir, NULL, &sl_test_mac_cmds_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmds debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	dentry = debugfs_create_file("cmd", 0644, mac_dir, NULL, &sl_test_mac_cmd_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

static u8 sl_test_debugfs_mac_num_get(void)
{
	return mac.num;
}

void sl_test_mac_remove(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int           rtn;
	struct sl_mac sl_mac;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"mac remove (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
		ldev_num, lgrp_num, mac_num);

	sl_test_mac_init(&sl_mac, ldev_num, lgrp_num, mac_num);

	rtn = sl_mac_del(&sl_mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_mac_del failed [%d]", rtn);
		return;
	}

	rtn = sl_test_pdev_port_kobj_put(ldev_num, lgrp_num, mac_num);
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", rtn);
}

int sl_test_mac_new(void)
{
	u8              mac_num;
	int             rtn;
	int             put_rtn;
	struct sl_lgrp *lgrp;
	struct sl_mac  *new_mac;
	struct kobject *port_num_kobj;

	lgrp     = sl_test_lgrp_get();
	mac_num  = sl_test_debugfs_mac_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		   "mac new (ldev_num = %u, lgrp_num = %u, mac_num = %u)",
		   lgrp->ldev_num, lgrp->num, mac_num);

	port_num_kobj = sl_test_pdev_port_kobj_get(lgrp->ldev_num, lgrp->num, mac_num);
	if (!port_num_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_get failed");
		return -ENODEV;
	}

	new_mac = sl_mac_new(lgrp, mac_num, port_num_kobj);
	if (IS_ERR_OR_NULL(new_mac)) {
		rtn = new_mac ? PTR_ERR(new_mac) : -ENOMEM;
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_mac_new failed [%d]", rtn);
		put_rtn = sl_test_pdev_port_kobj_put(lgrp->ldev_num, lgrp->num, mac_num);
		if (put_rtn)
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", put_rtn);
		return rtn;
	}

	return 0;
}

int sl_test_mac_del(void)
{
	int            rtn;
	int            put_rtn;
	struct sl_mac *sl_mac;

	sl_mac = sl_test_mac_get();

	rtn = sl_mac_del(sl_mac);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_mac_del failed [%d]", rtn);
		return rtn;
	}

	put_rtn = sl_test_pdev_port_kobj_put(sl_mac->ldev_num, sl_mac->lgrp_num, sl_mac->num);
	if (put_rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", put_rtn);

	return rtn;
}

int sl_test_mac_tx_start(void)
{
	return sl_mac_tx_start(sl_test_mac_get());
}

int sl_test_mac_rx_start(void)
{
	return sl_mac_rx_start(sl_test_mac_get());
}

int sl_test_mac_tx_stop(void)
{
	return sl_mac_tx_stop(sl_test_mac_get());
}

int sl_test_mac_rx_stop(void)
{
	return sl_mac_rx_stop(sl_test_mac_get());
}
