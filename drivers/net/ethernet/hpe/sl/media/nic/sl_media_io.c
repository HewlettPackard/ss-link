// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/hpe/sl/sl_ldev.h>

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

int sl_media_io_read(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data, size_t len)
{
	int                   rtn_len;
	struct sl_media_ldev *media_ldev;

	media_ldev = media_jack->media_ldev;

	rtn_len = media_ldev->uc_ops->uc_read(media_ldev->uc_accessor->uc, (u32)offset, (u32)page, data, len);
	if (rtn_len < 0) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "media_io_read failed [%d]", rtn_len);
		return rtn_len;
	}

	if (rtn_len != len) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "media_io_read length mismatch (expected = %zu, returned = %d)", len, rtn_len);
		return -EIO;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "media_io_read (page = 0x%X, offset = 0x%X, rtn_len = %d)", page, offset, rtn_len);

	return 0;
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	return sl_media_io_read(media_jack, page, offset, data, 1);
}

void sl_media_io_led_set(struct sl_media_jack *media_jack, u8 led_pattern)
{
	struct sl_media_ldev *media_ldev;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_led_set (pattern = %u)", led_pattern);

	media_ldev = media_jack->media_ldev;

	media_ldev->uc_ops->uc_led_set(media_ldev->uc_accessor->uc, led_pattern);
}
