// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_link.h>
#include <linux/sl_llr.h>

#include "sl_link.h"
#include "sl_llr.h"
#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_llr.h"

static struct dentry *llr_dir;
static struct sl_llr  llr;

int sl_test_llr_create(struct dentry *top_dir)
{
	llr_dir = debugfs_create_dir("llr", top_dir);
	if (!llr_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"llr debugfs_create_dir failed");
		return -ENOMEM;
	}

	llr.magic = SL_LLR_MAGIC;
	llr.ver   = SL_LLR_VER;
	llr.num   = 0;
	debugfs_create_u8("num", 0644, llr_dir, &llr.num);

	return 0;
}

int sl_test_llr_start(void)
{
	llr.ldev_num = sl_test_ldev_num_get();
	llr.lgrp_num = sl_test_lgrp_num_get();

	return sl_llr_start(&llr);
}

int sl_test_llr_stop(void)
{
	llr.ldev_num = sl_test_ldev_num_get();
	llr.lgrp_num = sl_test_lgrp_num_get();

	return sl_llr_stop(&llr);
}
