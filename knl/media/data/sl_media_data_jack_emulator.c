// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"

#include "base/sl_media_log.h"

#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_emulator.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

int sl_media_data_jack_scan(u8 ldev_num)
{
	struct sl_media_jack *media_jack;
	u8                    i;
	u8                    j;
	u8                    start;
	u8                    end;
	u8                    index;

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan");

	for (i = 0; i < SL_MEDIA_MAX_JACK_NUM; ++i) {
		media_jack = sl_media_data_jack_get(ldev_num, i);
		start = i * SL_MEDIA_LGRPS_PER_PORT;
		end = start + SL_MEDIA_LGRPS_PER_PORT;
		index = 0;
		for (j = start; j < end; ++j) {
			media_jack->cable_info[index].ldev_num = ldev_num;
			media_jack->cable_info[index].lgrp_num = j;
			index++;
		}
	}

	return 0;
}