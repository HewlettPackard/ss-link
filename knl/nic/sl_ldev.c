// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/sl_ldev.h>

#include "sl_log.h"
#include "sl_ldev.h"
#include "sl_media_ldev.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_LDEV_LOG_NAME

int sl_ldev_uc_ops_set(struct sl_ldev *ldev, struct sl_uc_ops *uc_ops,
		       struct sl_uc_accessor *uc_accessor)
{
	int rtn;

	rtn = sl_ldev_check(ldev);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "check fail");
		return rtn;
	}

	sl_log_dbg(ldev, LOG_BLOCK, LOG_NAME, "uc ops_set");

	rtn = sl_media_ldev_uc_ops_set(ldev->num, uc_ops, uc_accessor);
	if (rtn) {
		sl_log_err(ldev, LOG_BLOCK, LOG_NAME, "media_ldev_uc_ops_set failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
EXPORT_SYMBOL(sl_ldev_uc_ops_set);
