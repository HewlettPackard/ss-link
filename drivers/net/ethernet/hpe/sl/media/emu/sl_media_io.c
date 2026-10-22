// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include "sl_media_io.h"
#include "sl_media_jack.h"
#include <linux/errno.h>
#include <linux/string.h>

int sl_media_io_write8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 data)
{
	return 0;
}

int sl_media_io_read(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data, size_t len)
{
	if (len)
		memset(data, 0, len);
	return 0;
}

int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data)
{
	*data = 0;
	return 0;
}
