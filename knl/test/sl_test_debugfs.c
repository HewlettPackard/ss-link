// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_llr.h"
#include "sl_test_debugfs_mac.h"
#include "sl_test_debugfs_serdes.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME
// NOTE: will show up in /sys/kernel/debug/sl

static struct dentry *sl_test_top_dir;

int sl_test_debugfs_create(void)
{
	int            rtn;

	sl_test_top_dir = debugfs_create_dir("sl", NULL);
	if (!sl_test_top_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"sl debugfs_create_dir failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_ldev_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_debugfs_lgrp_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_debugfs_link_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_serdes_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_debugfs_llr_create(sl_test_top_dir);
	if (rtn)
		goto out;

	rtn = sl_test_debugfs_mac_create(sl_test_top_dir);
	if (rtn)
		goto out;

	return 0;

out:
	debugfs_remove_recursive(sl_test_top_dir);
	return rtn;
}

void sl_test_debugfs_destroy(void)
{
	debugfs_remove_recursive(sl_test_top_dir);
}
