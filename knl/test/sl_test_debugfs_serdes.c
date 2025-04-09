// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_test.h>

#include "core/hw/sl_core_hw_serdes_lane.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_serdes.h"
#include "sl_test_common.h"
#include "log/sl_log.h"

#define LOG_BLOCK "serdes"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *sl_test_serdes_dir;

struct sl_test_serdes_settings {
	// media settings
	s16 pre1;
	s16 pre2;
	s16 pre3;
	s16 cursor;
	s16 post1;
	s16 post2;
	u32 media;
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

static void sl_test_serdes_lane_encoding_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s %s\n",
		sl_test_serdes_lane_encoding_str(SL_CORE_HW_SERDES_ENCODING_NRZ),
		sl_test_serdes_lane_encoding_str(SL_CORE_HW_SERDES_ENCODING_PAM4_NR),
		sl_test_serdes_lane_encoding_str(SL_CORE_HW_SERDES_ENCODING_PAM4_ER));
};

static void sl_test_serdes_lane_clocking_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s\n",
		sl_test_serdes_lane_clocking_str(SL_CORE_HW_SERDES_CLOCKING_82P5),
		sl_test_serdes_lane_clocking_str(SL_CORE_HW_SERDES_CLOCKING_85));
};

static void sl_test_serdes_lane_osr_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s %s %s\n",
		sl_test_serdes_lane_osr_str(SL_CORE_HW_SERDES_OSR_OSX1),
		sl_test_serdes_lane_osr_str(SL_CORE_HW_SERDES_OSR_OSX2),
		sl_test_serdes_lane_osr_str(SL_CORE_HW_SERDES_OSR_OSX4),
		sl_test_serdes_lane_osr_str(SL_CORE_HW_SERDES_OSR_OSX42P5));
};

static void sl_test_serdes_lane_width_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s %s\n",
		sl_test_serdes_lane_width_str(SL_CORE_HW_SERDES_WIDTH_40),
		sl_test_serdes_lane_width_str(SL_CORE_HW_SERDES_WIDTH_80),
		sl_test_serdes_lane_width_str(SL_CORE_HW_SERDES_WIDTH_160));
};

static void sl_test_media_opts(char *buf, size_t size)
{
	snprintf(buf, size, "%s %s %s %s %s %s %s %s %s %s %s %s\n",
		sl_test_media_type_str(SL_MEDIA_TYPE_HEADSHELL),
		sl_test_media_type_str(SL_MEDIA_TYPE_BACKPLANE),
		sl_test_media_type_str(SL_MEDIA_TYPE_ELECTRICAL),
		sl_test_media_type_str(SL_MEDIA_TYPE_OPTICAL),
		sl_test_media_type_str(SL_MEDIA_TYPE_PASSIVE),
		sl_test_media_type_str(SL_MEDIA_TYPE_ACTIVE),
		sl_test_media_type_str(SL_MEDIA_TYPE_ANALOG),
		sl_test_media_type_str(SL_MEDIA_TYPE_DIGITAL),
		sl_test_media_type_str(SL_MEDIA_TYPE_AOC),
		sl_test_media_type_str(SL_MEDIA_TYPE_PEC),
		sl_test_media_type_str(SL_MEDIA_TYPE_AEC),
		sl_test_media_type_str(SL_MEDIA_TYPE_BKP));
};

static struct str_conv_u16 lane_encoding = {
	.to_str = sl_test_serdes_lane_encoding_str,
	.to_u16 = sl_test_serdes_lane_encoding_from_str,
	.opts   = sl_test_serdes_lane_encoding_opts,
	.value  = &settings.encoding,
};

static struct str_conv_u16 lane_clocking = {
	.to_str = sl_test_serdes_lane_clocking_str,
	.to_u16 = sl_test_serdes_lane_clocking_from_str,
	.opts   = sl_test_serdes_lane_clocking_opts,
	.value  = &settings.clocking,
};

static struct str_conv_u16 lane_osr = {
	.to_str = sl_test_serdes_lane_osr_str,
	.to_u16 = sl_test_serdes_lane_osr_from_str,
	.opts   = sl_test_serdes_lane_osr_opts,
	.value  = &settings.osr,
};

static struct str_conv_u16 lane_width = {
	.to_str = sl_test_serdes_lane_width_str,
	.to_u16 = sl_test_serdes_lane_width_from_str,
	.opts   = sl_test_serdes_lane_width_opts,
	.value  = &settings.width,
};

static struct str_conv_u32 media = {
	.to_str = sl_test_media_type_str,
	.to_u32 = sl_test_media_type_from_str,
	.opts   = sl_test_media_opts,
	.value  = &settings.media,
};

static struct str_conv_u16 dfe = {
	.to_str = sl_test_state_str,
	.to_u16 = sl_test_state_from_str,
	.opts   = sl_test_state_opts,
	.value  = &settings.dfe,
};

static struct str_conv_u16 scramble = {
	.to_str = sl_test_state_str,
	.to_u16 = sl_test_state_from_str,
	.opts   = sl_test_state_opts,
	.value  = &settings.scramble,
};

enum serdes_cmd_index {
	SERDES_PARAMS_SET_CMD,
	SERDES_PARAMS_UNSET_CMD,
	NUM_CMDS,
};

static struct cmd_entry serdes_cmd_list[]  = {
	[SERDES_PARAMS_SET_CMD]   = { .cmd = "serdes_params_set",   .desc = "set serdes params"                },
	[SERDES_PARAMS_UNSET_CMD] = { .cmd = "serdes_params_unset", .desc = "return serdes params to original" },
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
			"cmd_write partial not allowed");
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
				"cmd_write test_serdes_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = SERDES_CMD_MATCH(SERDES_PARAMS_UNSET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_serdes_unset();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"cmd_write test_serdes_unset failed [%d]", rtn);
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

static void sl_test_serdes_init(void)
{
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
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"serdes debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_serdes_init();

	settings_dir = debugfs_create_dir("settings", sl_test_serdes_dir);
	if (!sl_test_serdes_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"serdes settings debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_debugfs_create_s16("pre1",     0644, settings_dir, &(settings.pre1));
	sl_test_debugfs_create_s16("pre2",     0644, settings_dir, &(settings.pre2));
	sl_test_debugfs_create_s16("pre3",     0644, settings_dir, &(settings.pre3));
	sl_test_debugfs_create_s16("cursor",   0644, settings_dir, &(settings.cursor));
	sl_test_debugfs_create_s16("post1",    0644, settings_dir, &(settings.post1));
	sl_test_debugfs_create_s16("post2",    0644, settings_dir, &(settings.post2));

	sl_test_debugfs_create_str_conv_u16("encoding", 0644, settings_dir, &lane_encoding);
	sl_test_debugfs_create_str_conv_u16("clocking", 0644, settings_dir, &lane_clocking);
	sl_test_debugfs_create_str_conv_u16("osr",      0644, settings_dir, &lane_osr);
	sl_test_debugfs_create_str_conv_u16("width",    0644, settings_dir, &lane_width);
	sl_test_debugfs_create_str_conv_u32("media",    0644, settings_dir, &media);
	sl_test_debugfs_create_str_conv_u16("dfe",      0644, settings_dir, &dfe);
	sl_test_debugfs_create_str_conv_u16("scramble", 0644, settings_dir, &scramble);

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

	return sl_test_serdes_params_set(sl_test_debugfs_ldev_num_get(),
		sl_test_debugfs_lgrp_num_get(), sl_test_debugfs_link_num_get(),
		settings.pre1, settings.pre2, settings.pre3, settings.cursor,
		settings.post1, settings.post2, settings.media,
		settings.osr, settings.encoding, settings.clocking, settings.width,
		settings.dfe, settings.scramble, settings.options);
}

int sl_test_serdes_unset(void)
{
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes unset");

	return sl_test_serdes_params_unset(sl_test_debugfs_ldev_num_get(),
		sl_test_debugfs_lgrp_num_get(), sl_test_debugfs_link_num_get());
}
