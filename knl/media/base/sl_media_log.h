/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_LOG_H_
#define _SL_MEDIA_LOG_H_

#include "sl_log.h"

#define SL_MEDIA_LOG_BLOCK          "media"

#define SL_MEDIA_LDEV_LOG_NAME      "ldev"
#define SL_MEDIA_LGRP_LOG_NAME      "lgrp"
#define SL_MEDIA_DATA_LOG_NAME      "data"
#define SL_MEDIA_DATA_LDEV_LOG_NAME "d-ldev"
#define SL_MEDIA_DATA_LGRP_LOG_NAME "d-lgrp"
#define SL_MEDIA_JACK_LOG_NAME      "jack"
#define SL_MEDIA_DATA_JACK_LOG_NAME "d-jack"
#define SL_MEDIA_CABLE_LOG_NAME     "cable"
#define SL_MEDIA_EEPROM_LOG_NAME    "eeprom"
#define SL_MEDIA_TEST_LOG_NAME      "test"
#define SL_MEDIA_IO_LOG_NAME        "media-io"

#define sl_media_log_dbg(_ptr, _name, _text, ...) \
	sl_log_dbg((_ptr), SL_MEDIA_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_media_log_info(_ptr, _name, _text, ...) \
	sl_log_info((_ptr), SL_MEDIA_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_media_log_warn(_ptr, _name, _text, ...) \
	sl_log_warn((_ptr), SL_MEDIA_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_media_log_err(_ptr, _name, _text, ...) \
	sl_log_err((_ptr), SL_MEDIA_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)
#define sl_media_log_err_trace(_ptr, _name, _text, ...) \
	sl_log_err_trace((_ptr), SL_MEDIA_LOG_BLOCK, (_name), (_text), ##__VA_ARGS__)

#endif /* _SL_MEDIA_LOG_H_ */
