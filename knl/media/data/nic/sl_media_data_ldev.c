// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/errno.h>

#include "base/sl_media_log.h"
#include "sl_media_ldev.h"
#include "data/sl_media_data_ldev.h"

#define LOG_NAME  SL_LOG_LDEV_LOG_NAME

int sl_media_data_ldev_uc_ops_set(u8 ldev_num, struct sl_uc_ops *uc_ops,
				  struct sl_uc_accessor *uc_accessor)
{
	struct sl_media_ldev *media_ldev;

	media_ldev = sl_media_ldev_get(ldev_num);
	if (!media_ldev) {
		sl_media_log_err(NULL, LOG_NAME, "ldev not found (ldev_num = %u)", ldev_num);
		return -EINVAL;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME, "media_data_ldev_uc_ops_set");

	media_ldev->uc_ops = uc_ops;
	media_ldev->uc_accessor = uc_accessor;

	return 0;
}
