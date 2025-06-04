// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/sl_ldev.h>

#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_IO_LOG_NAME

int sl_media_io_write8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 data)
{
	int rtn;

	media_jack->i2c_data.addr    = 0;
	media_jack->i2c_data.page    = page;
	media_jack->i2c_data.bank    = 0;
	media_jack->i2c_data.offset  = offset;
	media_jack->i2c_data.data[0] = data;
	media_jack->i2c_data.len     = sizeof(data);

	rtn = hsnxcvr_i2c_write(media_jack->hdl, &media_jack->i2c_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media io write8 failed [%d]", rtn);
		return -EIO;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_write8 0x%x <- 0x%x", offset, data);

	return 0;
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	int rtn;

	media_jack->i2c_data.addr   = 0;
	media_jack->i2c_data.page   = page;
	media_jack->i2c_data.bank   = 0;
	media_jack->i2c_data.offset = offset;
	media_jack->i2c_data.len    = sizeof(*data);

	rtn = hsnxcvr_i2c_read(media_jack->hdl, &media_jack->i2c_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media io read8 failed [%d]", rtn);
		return -EIO;
	}

	data = media_jack->i2c_data.data;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read8 0x%x = 0x%x", offset, *data);

	return sizeof(*data);
}
