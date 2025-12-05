// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/hsnxcvr-api.h>

#include <linux/hpe/sl/sl_ldev.h>

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
	if (rtn)
		return rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_write8 0x%x <- 0x%x", offset, data);

	return 0;
}

#define SL_MEDIA_IO_MAX_RETRY 5
int sl_media_io_read(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data, size_t len)
{
	int                  rtn;
	u8                   read_attempt;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read (page=0x%x offset=0x%x len=%zu)",
			 page, offset, len);

	i2c_data.addr   = 0;
	i2c_data.page   = page;
	i2c_data.bank   = 0;
	i2c_data.offset = offset;
	i2c_data.len    = len;

	read_attempt = 0;
	do {
		rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
		if (rtn == -EAGAIN) {
			sl_media_log_warn_trace(media_jack, LOG_NAME,
						"media_io_read retrying (read_attempt = %d)", read_attempt);
			continue;
		}

		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "media_io_read failed [%d]", rtn);
			return -EIO;
		}

	} while (rtn == -EAGAIN && ++read_attempt < SL_MEDIA_IO_MAX_RETRY);

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read (read_attempt=%u)", read_attempt);

	if (read_attempt >= SL_MEDIA_IO_MAX_RETRY) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "media_io_read exceeded max retries "
				       "(read_attempt = %u, SL_MEDIA_IO_MAX_RETRY = %u)",
				       read_attempt, SL_MEDIA_IO_MAX_RETRY);
		return -EIO;
	}

	memcpy(data, i2c_data.data, len);

	return 0;
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	return sl_media_io_read(media_jack, page, offset, data, 1);
}

void sl_media_io_led_set(struct sl_media_jack *media_jack, u8 led_pattern)
{
	struct xcvr_led_data led_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_led_set (pattern = %u)", led_pattern);

	led_data.led_pattern = led_pattern;

	hsnxcvr_led_set(media_jack->hdl, &led_data);
}
