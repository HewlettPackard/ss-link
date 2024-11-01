// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_jack_rosetta.h"
#include "data/sl_media_data_jack_emulator.h"
#include "data/sl_media_data_jack_cassini.h"
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

void sl_media_jack_state_set(struct sl_media_jack *media_jack, u8 state)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->state = state;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);
}

u8 sl_media_jack_state_get(struct sl_media_jack *media_jack)
{
	u8            state;
	unsigned long irq_flags;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	state = media_jack->state;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	return state;
}

u8 sl_media_jack_downshift_state_get(struct sl_media_jack *media_jack)
{
	u8 downshift_state;

	spin_lock(&media_jack->data_lock);
	downshift_state = media_jack->downshift_state;
	spin_unlock(&media_jack->data_lock);

	return downshift_state;
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

int sl_media_jack_cable_downshift(u8 ldev_num, u8 lgrp_num)
{
	u8                    i;
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable downshift");

	if (sl_media_jack_downshift_state_get(media_lgrp->media_jack) == SL_MEDIA_JACK_DOWNSHIFT_STATE_SUCCESSFUL) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "already downshifted");
		return 0;
	}

	spin_lock(&media_lgrp->media_jack->data_lock);
	if (media_lgrp->media_jack->state != SL_MEDIA_JACK_CABLE_ONLINE) {
		media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_FAILED_NO_CABLE;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "downshift failed - no online cable");
		return 0;
	}
	for (i = 0; i < SL_MEDIA_MAX_LGRPS_PER_JACK; ++i) {
		if (media_lgrp->media_jack->cable_info[i].real_cable_status == CABLE_MEDIA_ATTR_STASHED) {
			media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_FAILED_FAKE_CABLE;
			spin_unlock(&media_lgrp->media_jack->data_lock);
			sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
					 "downshift failed [%d] fake cable (lgrp_num = %u)",
					 rtn, media_lgrp->media_jack->cable_info[i].lgrp_num);
			return 0;
		}
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	if (!sl_media_lgrp_cable_type_is_active(ldev_num, lgrp_num)) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "non-active cable - downshift not required");
		return 0;
	}

	spin_lock(&media_lgrp->media_jack->data_lock);
	if (!(media_lgrp->media_jack->cable_info[0].media_attr.speeds_map &
			(SL_MEDIA_SPEEDS_SUPPORT_CK_400G | SL_MEDIA_SPEEDS_SUPPORT_BS_200G))) {
		media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_FAILED_NO_SUPPORT;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
				 "downshift failed - no downshift support in cable");
		return 0;
	}
	if ((media_lgrp->media_jack->host_interface_200_gaui != SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M) ||
			(media_lgrp->media_jack->lane_count_200_gaui != 0x44)) {
		media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_FAILED_INAVLID_INFO;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
				 "downshift failed - invalid host interface and/or lane count");
		return 0;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	rtn = sl_media_data_jack_cable_downshift(media_lgrp->media_jack);
	if (rtn) {
		spin_lock(&media_lgrp->media_jack->data_lock);
		media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_FAILED;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "data jack cable downshift failed [%d]", rtn);
		return rtn;
	}

	spin_lock(&media_lgrp->media_jack->data_lock);
	media_lgrp->media_jack->downshift_state = SL_MEDIA_JACK_DOWNSHIFT_STATE_SUCCESSFUL;
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return 0;
}
