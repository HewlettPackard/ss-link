// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/sl_ldev.h>

#include "sl_media_ldev.h"
#include "sl_media_io.h"
#include "sl_media_jack.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_IO_LOG_NAME

int sl_media_io_write8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 data)
{
	struct sl_media_ldev *media_ldev;

	media_ldev = media_jack->media_ldev;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_write8 0x%x <- 0x%x", offset, data);

	return media_ldev->uc_ops->uc_write8(media_ldev->uc_accessor->uc, page, offset, data);
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	struct sl_media_ldev *media_ldev;
	int                   rtn;

	media_ldev = media_jack->media_ldev;

	rtn = media_ldev->uc_ops->uc_read8(media_ldev->uc_accessor->uc, (u32)offset, (u32)page, data);

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read8 0x%x = 0x%x", offset, *data);

	return rtn;
}
