// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"

#include "base/sl_media_log.h"

#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_cassini.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

int sl_media_data_jack_scan(u8 ldev_num)
{
	struct sl_media_jack *media_jack;

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan");

	media_jack = sl_media_data_jack_get(ldev_num, 0);
	media_jack->cable_info[0].ldev_num = ldev_num;
	media_jack->cable_info[0].lgrp_num = 0; /* hardcoding 0 since only one lgrp*/

	return 0;
}


int sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr)
{
	return 0;
}

void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info)
{

}
