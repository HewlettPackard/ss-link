// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

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
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_mac.h"
#include "sl_test_common.h"

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
	[MAC_NEW_CMD]      = { .cmd = "mac_new",      .desc = "create mac object" },
	[MAC_DEL_CMD]      = { .cmd = "mac_del",      .desc = "delete mac object" },
	[MAC_TX_START_CMD] = { .cmd = "mac_tx_start", .desc = "start MAC tx"      },
	[MAC_RX_START_CMD] = { .cmd = "mac_rx_start", .desc = "start MAC rx"      },
	[MAC_TX_STOP_CMD]  = { .cmd = "mac_tx_stop",   .desc = "stop MAC tx"      },
	[MAC_RX_STOP_CMD]  = { .cmd = "mac_rx_stop",   .desc = "stop MAC rx"      },
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

	match_found = MAC_CMD_MATCH(MAC_TX_STOP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_tx_stop();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_tx_stop failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = MAC_CMD_MATCH(MAC_RX_STOP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_mac_rx_stop();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_mac_rx_stop failed [%d]", rtn);
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
	struct dentry *dentry;

	mac_dir = debugfs_create_dir("mac", top_dir);
	if (!mac_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"mac debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_mac_init(&mac, sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 0);

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
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "sl_mac_del failed [%d]", rtn);
		return;
	}

	rtn = sl_test_port_num_entry_put(lgrp_num, mac_num);
	if (rtn)
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"sl_test_port_num_sysfs_put failed [%d]", rtn);
}

int sl_test_mac_new(void)
{
	u8              mac_num;
	int             rtn;
	struct sl_lgrp *lgrp;
	struct kobject *port_num_kobj;

	lgrp = sl_test_lgrp_get();
	mac_num = sl_test_debugfs_mac_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"mac new (lgrp_num = %u, mac_num = %u)",
		lgrp->num, mac_num);

	rtn = sl_test_port_num_entry_init(lgrp->num, mac_num);
	switch (rtn) {
	case 0:
		port_num_kobj = sl_test_port_num_sysfs_get(lgrp->num, mac_num);
		break;
	case -EALREADY:
		sl_test_port_num_entry_get_unless_zero(lgrp->num, mac_num);
		port_num_kobj = sl_test_port_num_sysfs_get(lgrp->num, mac_num);
		break;
	default:
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"sl_test_port_num_entry_init failed [%d]", rtn);
		return rtn;
	}

	return IS_ERR(sl_mac_new(lgrp, mac_num, port_num_kobj));
}

int sl_test_mac_del(void)
{
	int            rtn;
	struct sl_mac *sl_mac;

	sl_mac = sl_test_mac_get();

	rtn = sl_mac_del(sl_mac);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"sl_mac_del failed [%d]", rtn);
		return rtn;
	}

	rtn = sl_test_port_num_entry_put(sl_mac->lgrp_num, sl_mac->num);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"sl_test_port_num_entry_put failed [%d]", rtn);
		return rtn;
	}

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
