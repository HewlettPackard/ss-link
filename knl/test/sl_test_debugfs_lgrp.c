// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"

static struct dentry *lgrp_dir;
static u8             lgrp_num;

int sl_test_lgrp_create(struct dentry *top_dir)
{
	lgrp_dir = debugfs_create_dir("lgrp", top_dir);
	if (!lgrp_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"lgrp debugfs_create_dir failed");
		return -ENOMEM;
	}

	lgrp_num = 0;
	debugfs_create_u8("num", 0644, lgrp_dir, &lgrp_num);

	return 0;
}

u8 sl_test_lgrp_num_get(void)
{
	return lgrp_num;
}
