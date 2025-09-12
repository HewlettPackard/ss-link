// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/hsnxcvr-api.h>

#include <linux/sl_ldev.h>

#include "base/sl_media_log.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"

#define LOG_NAME SL_MEDIA_IO_LOG_NAME

int sl_media_io_write8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 data)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	i2c_data.addr    = 0;
	i2c_data.page    = page;
	i2c_data.bank    = 0;
	i2c_data.offset  = offset;
	i2c_data.data[0] = data;
	i2c_data.len     = sizeof(data);

	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media_io_write8 failed [%d]", rtn);
		return -EIO;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_write8 0x%x <- 0x%x", offset, data);

	return 0;
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	i2c_data.addr   = 0;
	i2c_data.page   = page;
	i2c_data.bank   = 0;
	i2c_data.offset = offset;
	i2c_data.len    = sizeof(*data);

	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media_io_read8 failed [%d]", rtn);
		return -EIO;
	}

	data = i2c_data.data;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read8 0x%x = 0x%x", offset, *data);

	return sizeof(*data);
}

void sl_media_io_led_set(struct sl_media_jack *media_jack, u8 led_pattern)
{
	int                  rtn;
	struct xcvr_led_data data;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_led_set (pattern = %u)", led_pattern);

	data.led_pattern = led_pattern;

	rtn = hsnxcvr_led_set(media_jack->hdl, &data);
	if (rtn)
		sl_media_log_warn(media_jack, LOG_NAME, "media io led set failed [%d]", rtn);
}
