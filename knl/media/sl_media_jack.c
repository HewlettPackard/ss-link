// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_lgrp.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

#define SL_MEDIA_CMIS_POWER_UP_PAGE      0x0
#define SL_MEDIA_CMIS_POWER_UP_ADDR      0x1a
#define SL_MEDIA_CMIS_POWER_UP_DATA      0x0
#define SL_MEDIA_XCVR_POWER_UP_WAIT_TIME 10000

int sl_media_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num)
{
	return sl_media_data_jack_new(media_ldev, jack_num);
}

void sl_media_jack_del(u8 ldev_num, u8 jack_num)
{
	sl_media_data_jack_del(ldev_num, jack_num);
}

struct sl_media_jack *sl_media_jack_get(u8 ldev_num, u8 jack_num)
{
	return sl_media_data_jack_get(ldev_num, jack_num);
}

u8 sl_media_jack_state_get(struct sl_media_jack *media_jack)
{
	u8 state;

	spin_lock(&media_jack->data_lock);
	state = media_jack->state;
	spin_unlock(&media_jack->data_lock);

	return state;
}

bool sl_media_jack_is_cable_online(struct sl_media_jack *media_jack)
{
	u8 state;

	spin_lock(&media_jack->data_lock);
	state = media_jack->state;
	spin_unlock(&media_jack->data_lock);

	return state == SL_MEDIA_JACK_CABLE_ONLINE;
}

bool sl_media_jack_is_cable_format_invalid(struct sl_media_jack *media_jack)
{
	u8 format;

	spin_lock(&media_jack->data_lock);
	format = media_jack->is_cable_format_invalid;
	spin_unlock(&media_jack->data_lock);

	return format;
}

int sl_media_jack_cable_high_power_set(u8 ldev_num, u8 jack_num)
{
	int                   rtn;
	struct sl_media_jack *media_jack;

	media_jack = sl_media_jack_get(ldev_num, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "high power set");

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_CMIS_POWER_UP_PAGE,
	      SL_MEDIA_CMIS_POWER_UP_ADDR, SL_MEDIA_CMIS_POWER_UP_DATA);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "uc write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = true;

	media_jack->cable_power_up_wait_time_end = jiffies +
			msecs_to_jiffies(SL_MEDIA_XCVR_POWER_UP_WAIT_TIME);

	return 0;
}
