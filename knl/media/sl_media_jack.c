// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

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

/*
 * FIXME: Eventually remove this struct array and get this info from cable DB
 */
static struct sl_media_downshift_info downshift_cable_db[] = {
		{
				.type = SL_MEDIA_TYPE_AOC,
				.vendor = SL_MEDIA_VENDOR_TE,
				.fw_major_ver = 0x04,
				.fw_minor_ver = 0x01,
		},
		{
				.type = SL_MEDIA_TYPE_AEC,
				.vendor = SL_MEDIA_VENDOR_TE,
				.fw_major_ver = 0x00,
				.fw_minor_ver = 0x04,
		},
		{
				.type = SL_MEDIA_TYPE_AEC,
				.vendor = SL_MEDIA_VENDOR_MOLEX,
				.fw_major_ver = 0x01,
				.fw_minor_ver = 0x00,
		},
		{
				.type = SL_MEDIA_TYPE_AOC,
				.vendor = SL_MEDIA_VENDOR_FINISAR,
				.fw_major_ver = 0x02,
				.fw_minor_ver = 0x06,
		},
};

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

void sl_media_jack_cable_shift_state_set(struct sl_media_jack *media_jack, u8 state)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	media_jack->cable_shift_state = state;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable shift state set = %u", media_jack->cable_shift_state);
}

u8 sl_media_jack_cable_shift_state_get(struct sl_media_jack *media_jack)
{
	u8            cable_shift_state;
	unsigned long irq_flags;

	spin_lock_irqsave(&media_jack->data_lock, irq_flags);
	cable_shift_state = media_jack->cable_shift_state;
	spin_unlock_irqrestore(&media_jack->data_lock, irq_flags);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable shift state get = %u", cable_shift_state);

	return cable_shift_state;
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
		sl_media_log_err(media_jack, LOG_NAME, "write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = true;

	media_jack->cable_power_up_wait_time_end = jiffies +
			msecs_to_jiffies(SL_MEDIA_XCVR_POWER_UP_WAIT_TIME);

	return 0;
}

#define SL_MEDIA_JACK_FW_MAJOR_VER_OFFSET 0
#define SL_MEDIA_JACK_FW_MINOR_VER_OFFSET 1
static bool sl_media_jack_cable_firmware_version_check(struct sl_media_lgrp *media_lgrp)
{
	int i;
	u32 type;
	u32 vendor;
	u8  fw_ver[SL_MEDIA_FIRMWARE_VERSION_SIZE];

	sl_media_lgrp_fw_ver_get(media_lgrp, fw_ver);
	type = sl_media_lgrp_type_get(media_lgrp);
	vendor = sl_media_lgrp_vendor_get(media_lgrp);

	for (i = 0; i < ARRAY_SIZE(downshift_cable_db); ++i) {
		if ((downshift_cable_db[i].type == type) &&
			(downshift_cable_db[i].vendor == vendor) &&
			(downshift_cable_db[i].fw_major_ver == fw_ver[SL_MEDIA_JACK_FW_MAJOR_VER_OFFSET]) &&
			(downshift_cable_db[i].fw_minor_ver == fw_ver[SL_MEDIA_JACK_FW_MINOR_VER_OFFSET])) {
			return true;
		}
	}

	return false;
}

static int sl_media_jack_cable_shift_checks(struct sl_media_lgrp *media_lgrp)
{
	u8  i;
	int rtn;

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable shift checks");

	spin_lock(&media_lgrp->media_jack->data_lock);
	if (media_lgrp->media_jack->state != SL_MEDIA_JACK_CABLE_ONLINE) {
		media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_NO_CABLE;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "shift check failed - no online cable");
		return -EFAULT;
	}

	for (i = 0; i < SL_MEDIA_MAX_LGRPS_PER_JACK; ++i) {
		if (media_lgrp->media_jack->cable_info[i].real_cable_status == CABLE_MEDIA_ATTR_STASHED) {
			media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_FAKE_CABLE;
			spin_unlock(&media_lgrp->media_jack->data_lock);
			sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
					 "shift check failed [%d] fake cable (lgrp_num = %u)",
					 rtn, media_lgrp->media_jack->cable_info[i].lgrp_num);
			return -EFAULT;
		}
	}
	if (!(media_lgrp->media_jack->cable_info[0].media_attr.speeds_map &
			(SL_MEDIA_SPEEDS_SUPPORT_CK_400G | SL_MEDIA_SPEEDS_SUPPORT_BS_200G))) {
		media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_NO_SUPPORT;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
				 "shift check failed - no shift support in cable");
		return -EINVAL;
	}
	if ((media_lgrp->media_jack->host_interface_200_gaui != SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M) ||
			(media_lgrp->media_jack->lane_count_200_gaui != 0x44) ||
			(media_lgrp->media_jack->host_interface_400_gaui == 0)) {
		media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_INAVLID_INFO;
		spin_unlock(&media_lgrp->media_jack->data_lock);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
				 "shift check failed - invalid host interface and/or lane count");
		return -EINVAL;
	}
	spin_unlock(&media_lgrp->media_jack->data_lock);

	return 0;
}

int sl_media_jack_cable_downshift(u8 ldev_num, u8 lgrp_num)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable downshift");

	if (!sl_media_lgrp_cable_type_is_active(media_lgrp->media_ldev->num, media_lgrp->num)) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "non-active cable - shift not required");
		return 0;
	}

	if (sl_media_data_jack_cable_hw_shift_state_get(media_lgrp->media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED) {
		sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "already downshifted");
		return 0;
	}

	rtn = sl_media_jack_cable_shift_checks(media_lgrp);
	if (rtn) {
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "cable shift checks failed [%d]", rtn);
		return 0;
	}

	if (!sl_media_jack_cable_firmware_version_check(media_lgrp)) {
		sl_media_log_err(media_lgrp->media_jack, LOG_NAME, "can't downshift - cable firmware not supported");
		return 0;
	}

	rtn = sl_media_data_jack_cable_downshift(media_lgrp->media_jack);
	if (rtn) {
		sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED);
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "data jack cable downshift failed [%d]", rtn);
		return rtn;
	}

	sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_DOWNSHIFTED);

	return 0;
}

int sl_media_jack_cable_upshift(u8 ldev_num, u8 lgrp_num)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable upshift");

	if (!sl_media_lgrp_cable_type_is_active(media_lgrp->media_ldev->num, media_lgrp->num)) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "non-active cable - shift not required");
		return 0;
	}

	if (sl_media_data_jack_cable_hw_shift_state_get(media_lgrp->media_jack) == SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED) {
		sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED);
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "already upshifted");
		return 0;
	}

	rtn = sl_media_jack_cable_shift_checks(media_lgrp);
	if (rtn) {
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "cable shift checks failed [%d]", rtn);
		return 0;
	}

	if (!sl_media_jack_cable_firmware_version_check(media_lgrp)) {
		sl_media_log_err(media_lgrp->media_jack, LOG_NAME, "can't upshift - cable firmware not supported");
		return 0;
	}

	rtn = sl_media_data_jack_cable_upshift(media_lgrp->media_jack);
	if (rtn) {
		sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED);
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "data jack cable upshift failed [%d]", rtn);
		return rtn;
	}

	sl_media_jack_cable_shift_state_set(media_lgrp->media_jack, SL_MEDIA_JACK_CABLE_SHIFT_STATE_UPSHIFTED);

	return 0;
}
