// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"

static struct dentry *ldev_dir;
static u8             ldev_num;

int sl_test_ldev_create(struct dentry *top_dir)
{
	ldev_dir = debugfs_create_dir("ldev", top_dir);
	if (!ldev_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"ldev debugfs_create_dir failed");
		return -ENOMEM;
	}

	ldev_num = 0;
	debugfs_create_u8("num", 0644, ldev_dir, &ldev_num);

	return 0;
}

u8 sl_test_ldev_num_get(void)
{
	return ldev_num;
}
