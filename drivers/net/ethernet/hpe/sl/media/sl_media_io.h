/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_IO_H_
#define _SL_MEDIA_IO_H_

#include "linux/types.h"

struct sl_media_jack;

int sl_media_io_write8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 data);
int sl_media_io_read8(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data);
int sl_media_io_read(struct sl_media_jack *media_jack, u8 page, u8 offset, u8 *data, size_t len);

void sl_media_io_led_set(struct sl_media_jack *media_jack, u8 led_pattern);

#endif /* _SL_MEDIA_IO_H_ */
