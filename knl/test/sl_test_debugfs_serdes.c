// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_test.h>

#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_serdes.h"
#include "sl_test_common.h"
#include "log/sl_log.h"

#define LOG_BLOCK SL_LOG_BLOCK
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
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes create");

	sl_test_serdes_dir = debugfs_create_dir("serdes", parent);
	if (!sl_test_serdes_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "serdes debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_serdes_init();

	debugfs_create_u16("pre1",     0644, sl_test_serdes_dir, &(settings.pre1));
	debugfs_create_u16("pre2",     0644, sl_test_serdes_dir, &(settings.pre2));
	debugfs_create_u16("pre3",     0644, sl_test_serdes_dir, &(settings.pre3));
	debugfs_create_u16("cursor",   0644, sl_test_serdes_dir, &(settings.cursor));
	debugfs_create_u16("post1",    0644, sl_test_serdes_dir, &(settings.post1));
	debugfs_create_u16("post2",    0644, sl_test_serdes_dir, &(settings.post2));
	debugfs_create_u16("media",    0644, sl_test_serdes_dir, &(settings.media));

	debugfs_create_u16("osr",      0644, sl_test_serdes_dir, &(settings.osr));
	debugfs_create_u16("encoding", 0644, sl_test_serdes_dir, &(settings.encoding));
	debugfs_create_u16("clocking", 0644, sl_test_serdes_dir, &(settings.clocking));
	debugfs_create_u16("width",    0644, sl_test_serdes_dir, &(settings.width));
	debugfs_create_u16("dfe",      0644, sl_test_serdes_dir, &(settings.dfe));
	debugfs_create_u16("scramble", 0644, sl_test_serdes_dir, &(settings.scramble));

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

	return sl_test_serdes_params_set(sl_test_ldev_num_get(), sl_test_lgrp_num_get(), sl_test_link_num_get(),
		settings.pre1, settings.pre2, settings.pre3, settings.cursor,
		settings.post1, settings.post2, settings.media,
		settings.osr, settings.encoding, settings.clocking, settings.width,
		settings.dfe, settings.scramble, settings.options);
}

int sl_test_serdes_unset(void)
{
	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "serdes unset");

	return sl_test_serdes_params_unset(sl_test_ldev_num_get(), sl_test_lgrp_num_get(), sl_test_link_num_get());
}
