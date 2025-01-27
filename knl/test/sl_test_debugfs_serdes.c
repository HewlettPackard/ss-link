// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_test.h>

#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_serdes.h"
#include "sl_test_common.h"
#include "log/sl_log.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *sl_test_serdes_dir;
static u8 serdes_num;

enum serdes_cmd_index {
	SERDES_PARAMS_SET_CMD,
	SERDES_PARAMS_UNSET_CMD,
	NUM_CMDS,
};

static struct cmd_entry serdes_cmd_list[]  = {
	[SERDES_PARAMS_SET_CMD]     = { .cmd = "serdes_params_set",   .desc = "set serdes params"                  },
	[SERDES_PARAMS_UNSET_CMD]   = { .cmd = "serdes_params_unset", .desc = "return serdes params to original"   },
};

#define SERDES_CMD_MATCH(_index, _str) \
	(strncmp((_str), serdes_cmd_list[_index].cmd, strlen(serdes_cmd_list[_index].cmd)) == 0)

static int sl_test_serdes_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, serdes_cmd_list, ARRAY_SIZE(serdes_cmd_list));
}

static int sl_test_serdes_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_serdes_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_serdes_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_serdes_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static ssize_t sl_test_serdes_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
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

	match_found = SERDES_CMD_MATCH(SERDES_PARAMS_SET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_serdes_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_serdes_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = SERDES_CMD_MATCH(SERDES_PARAMS_UNSET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_serdes_unset();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_serdes_unset failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
}

static const struct file_operations sl_test_serdes_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_serdes_cmd_write,
};

struct sl_test_serdes_settings {
	// media settings
	s16 pre1;
	s16 pre2;
	s16 pre3;
	s16 cursor;
	s16 post1;
	s16 post2;
	u16 media;
	// core settings
	u16 osr;
	u16 encoding;
	u16 clocking;
	u16 width;
	u16 dfe;
	u16 scramble;
	// options
	u32 options;
};

static struct sl_test_serdes_settings settings;

static void sl_test_serdes_init(void)
{
	serdes_num        = 0;
	settings.pre1     = 0;
	settings.pre2     = 0;
	settings.pre3     = 0;
	settings.cursor   = 100;
	settings.post1    = 0;
	settings.post2    = 0;
	settings.media    = 1;

	settings.osr      = 1;
	settings.encoding = 4;
	settings.clocking = 1;
	settings.width    = 1;
	settings.dfe      = 0;
	settings.scramble = 0;

	settings.options  = 0;
}

int sl_test_serdes_create(struct dentry *parent)
{
	struct dentry *dentry;
	struct dentry *settings_dir;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes create");

	sl_test_serdes_dir = debugfs_create_dir("serdes", parent);
	if (!sl_test_serdes_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "serdes debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_serdes_init();

	debugfs_create_u8("num",       0644, sl_test_serdes_dir, &serdes_num);

	settings_dir = debugfs_create_dir("settings", sl_test_serdes_dir);
	if (!sl_test_serdes_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "serdes debugfs_create_dir failed");
		return -ENOMEM;
	}

	debugfs_create_u16("pre1",     0644, settings_dir, &(settings.pre1));
	debugfs_create_u16("pre2",     0644, settings_dir, &(settings.pre2));
	debugfs_create_u16("pre3",     0644, settings_dir, &(settings.pre3));
	debugfs_create_u16("cursor",   0644, settings_dir, &(settings.cursor));
	debugfs_create_u16("post1",    0644, settings_dir, &(settings.post1));
	debugfs_create_u16("post2",    0644, settings_dir, &(settings.post2));
	debugfs_create_u16("media",    0644, settings_dir, &(settings.media));

	debugfs_create_u16("osr",      0644, settings_dir, &(settings.osr));
	debugfs_create_u16("encoding", 0644, settings_dir, &(settings.encoding));
	debugfs_create_u16("clocking", 0644, settings_dir, &(settings.clocking));
	debugfs_create_u16("width",    0644, settings_dir, &(settings.width));
	debugfs_create_u16("dfe",      0644, settings_dir, &(settings.dfe));
	debugfs_create_u16("scramble", 0644, settings_dir, &(settings.scramble));

	dentry = debugfs_create_file("cmds", 0644, sl_test_serdes_dir, NULL, &sl_test_serdes_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, sl_test_serdes_dir, NULL, &sl_test_serdes_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

u8 sl_test_debugfs_serdes_num_get(void)
{
	return serdes_num;
}

int sl_test_serdes_set(void)
{
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes set");

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"serdes set (pre1 = %d, pre2 = %d, pre3 = %d, cursor = %d, post1 = %d, post2 = %d, media = %u)",
		settings.pre1, settings.pre2, settings.pre3, settings.cursor,
		settings.post1, settings.post2, settings.media);
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"serdes set (osr = %u, encoding = %u, clocking = %u, width = %u, dfe = %u, scramble = %u)",
		settings.osr, settings.encoding, settings.clocking, settings.width,
		settings.dfe, settings.scramble);

	return sl_test_serdes_params_set(sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(),
		sl_test_debugfs_serdes_num_get(),
		settings.pre1, settings.pre2, settings.pre3, settings.cursor,
		settings.post1, settings.post2, settings.media,
		settings.osr, settings.encoding, settings.clocking, settings.width,
		settings.dfe, settings.scramble, settings.options);
}

int sl_test_serdes_unset(void)
{
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes unset");

	return sl_test_serdes_params_unset(sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(),
		sl_test_debugfs_serdes_num_get());
}
