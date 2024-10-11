// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_llr.h"
#include "sl_test_debugfs_serdes.h"

// NOTE: will show up in /sys/kernel/debug/sl

static struct dentry *sl_test_top_dir;

#define CMD_LEN  32
#define CMD_DESC_LEN 128

struct cmd_entry {
	const char cmd[CMD_LEN];
	const char desc[CMD_DESC_LEN];
};

enum cmd_index {
	UP_CMD,
	DOWN_CMD,
	CANCEL_CMD,
	WRITE_LINK_CONFIG_CMD,
	WRITE_LINK_POLICY_CMD,
	LLR_START_CMD,
	LLR_STOP_CMD,
	SERDES_PARAMS_SET_CMD,
	SERDES_PARAMS_UNSET_CMD,
	NUM_CMDS,
};

static struct cmd_entry cmd_list[] = {
	[UP_CMD]                  = { .cmd = "up",                  .desc = "task link to go up"                 },
	[DOWN_CMD]                = { .cmd = "down",                .desc = "task link to go down"               },
	[CANCEL_CMD]              = { .cmd = "cancel",              .desc = "cancel command"                     },
	[WRITE_LINK_CONFIG_CMD]   = { .cmd = "write_link_config",   .desc = "write out the link config from dir" },
	[WRITE_LINK_POLICY_CMD]   = { .cmd = "write_link_policy",   .desc = "write out the link policy from dir" },
	[LLR_START_CMD]           = { .cmd = "llr_start",           .desc = "start LLR"                          },
	[LLR_STOP_CMD]            = { .cmd = "llr_stop",            .desc = "stop LLR"                           },
	[SERDES_PARAMS_SET_CMD]   = { .cmd = "serdes_params_set",   .desc = "set serdes params"                  },
	[SERDES_PARAMS_UNSET_CMD] = { .cmd = "serdes_params_unset", .desc = "return serdes params to original"   },
};

static bool cmd_match(u8 index, const char *str)
{
	if (index > NUM_CMDS)
		return false;

	return strncmp(str, cmd_list[index].cmd, strlen(cmd_list[index].cmd)) == 0;
}

static int sl_test_cmds_show(struct seq_file *s, void *unused)
{
	int x;

	seq_puts(s, "cmd list\n");
	seq_puts(s, "--------\n");
	for (x = 0; x < ARRAY_SIZE(cmd_list); ++x)
		seq_printf(s, "%d: %-25s %s\n", x, cmd_list[x].cmd, cmd_list[x].desc);

	return 0;
}

static int sl_test_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static ssize_t sl_test_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];
	bool    match_found;

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (size > sizeof(cmd_buf)) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf), pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	match_found = cmd_match(UP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_up();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_link_up failed [%d]", rtn);
			return rtn;
		}
		return size;
	};

	match_found = cmd_match(DOWN_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_down();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_link_down failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(WRITE_LINK_CONFIG_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_config_set();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_link_config_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(WRITE_LINK_POLICY_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_policy_set();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_link_policy_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(LLR_START_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_start();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_llr_start failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(LLR_STOP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_stop();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_llr_stop failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(SERDES_PARAMS_SET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_serdes_set();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_serdes_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = cmd_match(SERDES_PARAMS_UNSET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_serdes_unset();
		if (rtn < 0) {
			sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
				"sl_test_serdes_unset failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
}

static const struct file_operations sl_test_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_cmd_write,
};

int sl_test_debugfs_create(void)
{
	int            rtn;
	struct dentry *dentry;

	sl_test_top_dir = debugfs_create_dir("sl", NULL);
	if (!sl_test_top_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"sl debugfs_create_dir failed");
		return -ENOMEM;
	}

	rtn = sl_test_ldev_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_lgrp_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_link_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_serdes_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_llr_create(sl_test_top_dir);
	if (rtn)
		goto out;

	dentry = debugfs_create_file("cmds", 0644, sl_test_top_dir, NULL, &sl_test_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"cmds debugfs_create_file failed");
		rtn = -ENOMEM;
		goto out;
	}

	dentry = debugfs_create_file("cmd", 0644, sl_test_top_dir, NULL, &sl_test_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"cmd debugfs_create_file failed");
		rtn = -ENOMEM;
		goto out;
	}

	return 0;

out:
	debugfs_remove_recursive(sl_test_top_dir);
	return rtn;
}

void sl_test_debugfs_destroy(void)
{
	debugfs_remove_recursive(sl_test_top_dir);
}
