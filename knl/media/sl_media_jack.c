// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_jack_rosetta.h"
#include "data/sl_media_data_jack_emulator.h"
#include "data/sl_media_data_jack_cassini.h"
#include "data/sl_media_data_lgrp.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

enum sl_media_cable_shift {
	SL_MEDIA_JACK_CABLE_DOWNSHIFT,
	SL_MEDIA_JACK_CABLE_UPSHIFT,
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

void sl_media_jack_state_set(struct sl_media_jack *media_jack, u8 jack_state)
{
	spin_lock(&media_jack->data_lock);
	media_jack->state = jack_state;
	spin_unlock(&media_jack->data_lock);

	sl_media_data_jack_headshell_led_set(media_jack, jack_state);
}

u8 sl_media_jack_state_get(struct sl_media_jack *media_jack)
{
	u8 state;

	spin_lock(&media_jack->data_lock);
	state = media_jack->state;
	spin_unlock(&media_jack->data_lock);

	return state;
}

u8 sl_media_jack_cable_end_get(struct sl_media_jack *media_jack)
{
	u8 cable_end;

	spin_lock(&media_jack->data_lock);
	cable_end = media_jack->cable_end;
	spin_unlock(&media_jack->data_lock);

	return cable_end;
}

bool sl_media_jack_is_high_powered(struct sl_media_jack *media_jack)
{
	bool is_high_powered;

	spin_lock(&media_jack->data_lock);
	is_high_powered = media_jack->is_high_powered;
	spin_unlock(&media_jack->data_lock);

	return is_high_powered;
}

void sl_media_jack_cable_shift_state_set(struct sl_media_jack *media_jack, u8 state)
{
	spin_lock(&media_jack->data_lock);
	media_jack->cable_shift_state = state;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable shift state set = %u", media_jack->cable_shift_state);
}

u8 sl_media_jack_cable_shift_state_get(struct sl_media_jack *media_jack)
{
	u8 cable_shift_state;

	spin_lock(&media_jack->data_lock);
	cable_shift_state = media_jack->cable_shift_state;
	spin_unlock(&media_jack->data_lock);

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
	bool is_format_invalid;

	spin_lock(&media_jack->data_lock);
	is_format_invalid = media_jack->is_cable_format_invalid;
	spin_unlock(&media_jack->data_lock);

	return is_format_invalid;
}

u8 sl_media_jack_active_cable_200g_host_iface_get(struct sl_media_jack *media_jack)
{
	u8 host_interface_200_gaui;

	spin_lock(&media_jack->data_lock);
	host_interface_200_gaui = media_jack->host_interface_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return host_interface_200_gaui;
}

u8 sl_media_jack_active_cable_200g_appsel_no_get(struct sl_media_jack *media_jack)
{
	u8 appsel_no_200_gaui;

	spin_lock(&media_jack->data_lock);
	appsel_no_200_gaui = media_jack->appsel_no_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return appsel_no_200_gaui;
}

u8 sl_media_jack_active_cable_200g_lane_count_get(struct sl_media_jack *media_jack)
{
	u8 lane_count_200_gaui;

	spin_lock(&media_jack->data_lock);
	lane_count_200_gaui = media_jack->lane_count_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return lane_count_200_gaui;
}

u8 sl_media_jack_active_cable_400g_host_iface_get(struct sl_media_jack *media_jack)
{
	u8 host_interface_400_gaui;

	spin_lock(&media_jack->data_lock);
	host_interface_400_gaui = media_jack->host_interface_400_gaui;
	spin_unlock(&media_jack->data_lock);

	return host_interface_400_gaui;
}

u8 sl_media_jack_active_cable_400g_appsel_no_get(struct sl_media_jack *media_jack)
{
	u8 appsel_no_400_gaui;

	spin_lock(&media_jack->data_lock);
	appsel_no_400_gaui = media_jack->appsel_no_400_gaui;
	spin_unlock(&media_jack->data_lock);

	return appsel_no_400_gaui;
}

u8 sl_media_jack_active_cable_400g_lane_count_get(struct sl_media_jack *media_jack)
{
	u8 lane_count_400_gaui;

	spin_lock(&media_jack->data_lock);
	lane_count_400_gaui = media_jack->lane_count_400_gaui;
	spin_unlock(&media_jack->data_lock);

	return lane_count_400_gaui;
}

#define SL_MEDIA_XCVR_POWER_UP_WAIT_TIME 10000
int sl_media_jack_cable_high_power_set(u8 ldev_num, u8 jack_num)
{
	int                   rtn;
	struct sl_media_jack *media_jack;

	media_jack = sl_media_jack_get(ldev_num, jack_num);

	sl_media_log_dbg(media_jack, LOG_NAME, "high power set");

	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power set failed [%d]", rtn);
		return -EIO;
	}

	media_jack->cable_power_up_wait_time_end = jiffies +
			msecs_to_jiffies(SL_MEDIA_XCVR_POWER_UP_WAIT_TIME);

	return 0;
}

static int sl_media_jack_cable_shift_checks(struct sl_media_lgrp *media_lgrp, u8 shift_state)
{
	u8 i;

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
					 "shift check failed - fake cable (lgrp_num = %u)",
					 media_lgrp->media_jack->cable_info[i].lgrp_num);
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
	if (shift_state == SL_MEDIA_JACK_CABLE_DOWNSHIFT) {
		if ((media_lgrp->media_jack->host_interface_200_gaui != SL_MEDIA_SS1_HOST_INTERFACE_200GAUI_4_C2M) ||
			(media_lgrp->media_jack->lane_count_200_gaui != 0x44) ||
			(media_lgrp->media_jack->host_interface_400_gaui == 0)) {
			media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_INVALID_INFO;
			spin_unlock(&media_lgrp->media_jack->data_lock);
			sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
					 "downshift check failed - invalid host interface and/or lane count");
			return -EINVAL;
		}
	} else {
		if (((media_lgrp->media_jack->host_interface_400_gaui != SL_MEDIA_SS2_HOST_INTERFACE_400GAUI_4_S_C2M) &&
			(media_lgrp->media_jack->host_interface_400_gaui != SL_MEDIA_SS1_HOST_INTERFACE_400GAUI_4_L_C2M)) ||
			(media_lgrp->media_jack->lane_count_400_gaui != 0x44)) {
			media_lgrp->media_jack->cable_shift_state = SL_MEDIA_JACK_CABLE_SHIFT_STATE_FAILED_INVALID_INFO;
			spin_unlock(&media_lgrp->media_jack->data_lock);
			sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME,
					 "upshift check failed - invalid host interface and/or lane count");
			return -EINVAL;
		}
	}

	spin_unlock(&media_lgrp->media_jack->data_lock);

	return 0;
}

int sl_media_jack_cable_downshift(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_core_link  *core_link;
	struct sl_media_attr  media_attr;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);
	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

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

	rtn = sl_media_jack_cable_shift_checks(media_lgrp, SL_MEDIA_JACK_CABLE_DOWNSHIFT);
	if (rtn) {
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "cable shift checks failed [%d]", rtn);
		return 0;
	}

	if (!sl_core_link_policy_is_ignore_media_errors_set(core_link)) {
		sl_media_lgrp_media_attr_get(ldev_num, lgrp_num, &media_attr);
		if (media_attr.errors & SL_MEDIA_ERROR_CABLE_FW_INVALID) {
			sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "can't downshift - cable firmware not supported");
			return 0;
		}
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

int sl_media_jack_cable_upshift(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_core_link  *core_link;
	struct sl_media_attr  media_attr;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);
	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

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

	rtn = sl_media_jack_cable_shift_checks(media_lgrp, SL_MEDIA_JACK_CABLE_UPSHIFT);
	if (rtn) {
		sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "cable shift checks failed [%d]", rtn);
		return 0;
	}

	if (!sl_core_link_policy_is_ignore_media_errors_set(core_link)) {
		sl_media_lgrp_media_attr_get(ldev_num, lgrp_num, &media_attr);
		if (media_attr.errors & SL_MEDIA_ERROR_CABLE_FW_INVALID) {
			sl_media_log_err_trace(media_lgrp->media_jack, LOG_NAME, "can't upshift - cable firmware not supported");
			return 0;
		}
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

void sl_media_jack_fault_cause_set(struct sl_media_jack *media_jack, u32 fault_cause)
{
	spin_lock(&media_jack->data_lock);
	media_jack->fault_cause = fault_cause;
	media_jack->fault_time  = ktime_get_real_seconds();
	spin_unlock(&media_jack->data_lock);

	if (fault_cause == SL_MEDIA_FAULT_CAUSE_HIGH_TEMP_JACK_IO)
		sl_media_data_jack_headshell_led_set(media_jack, SL_MEDIA_JACK_CABLE_HIGH_TEMP);

	sl_media_log_dbg(media_jack, LOG_NAME, "fault cause set (cause = %u %s)", fault_cause,
		sl_media_fault_cause_str(fault_cause));
}

void sl_media_jack_fault_cause_get(struct sl_media_jack *media_jack, u32 *fault_cause,
	time64_t *fault_time)
{
	spin_lock(&media_jack->data_lock);
	*fault_cause = media_jack->fault_cause;
	*fault_time  = media_jack->fault_time;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable fault cause get (cause = %u %s)", *fault_cause,
		sl_media_fault_cause_str(*fault_cause));
}

const char *sl_media_fault_cause_str(u32 fault_cause)
{
	switch (fault_cause) {
	case SL_MEDIA_FAULT_CAUSE_EEPROM_FORMAT_INVALID:
		return "eeprom-format-invalid";
	case SL_MEDIA_FAULT_CAUSE_EEPROM_VENDOR_INVALID:
		return "eeprom-vendor-invalid";
	case SL_MEDIA_FAULT_CAUSE_EEPROM_JACK_IO:
		return "eeprom-jack-io";
	case SL_MEDIA_FAULT_CAUSE_ONLINE_STATUS_GET:
		return "online-status-get";
	case SL_MEDIA_FAULT_CAUSE_ONLINE_TIMEDOUT:
		return "online-timedout";
	case SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_IO:
		return "online-jack-io";
	case SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_GET:
		return "online-jack-get";
	case SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET:
		return "serdes-settings-get";
	case SL_MEDIA_FAULT_CAUSE_SCAN_STATUS_GET:
		return "scan-status-get";
	case SL_MEDIA_FAULT_CAUSE_SCAN_HDL_GET:
		return "scan-hdl-get";
	case SL_MEDIA_FAULT_CAUSE_SCAN_JACK_GET:
		return "scan-jack-get";
	case SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET:
		return "media-attr-set";
	case SL_MEDIA_FAULT_CAUSE_INTR_EVENT_JACK_IO:
		return "intr-event-jack-io";
	case SL_MEDIA_FAULT_CAUSE_POWER_SET:
		return "power-set";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO:
		return "shift-down-jack-io";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET:
		return "shift-down-jack-io-low-power-set";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET:
		return "shift-down-jack-io-high-power-set";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO:
		return "shift-up-jack-io";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET:
		return "shift-up-jack-io-low-power-set";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET:
		return "shift-up-jack-io-high-power-set";
	case SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO:
		return "shift-state-jack-io";
	case SL_MEDIA_FAULT_CAUSE_OFFLINE:
		return "offline";
	case SL_MEDIA_FAULT_CAUSE_HIGH_TEMP_JACK_IO:
		return "high-temp-jack-io";
	default:
		return "unknown";
	}
}

bool sl_media_jack_cable_is_high_temp(struct sl_media_jack *media_jack)
{
	return sl_media_data_jack_cable_is_high_temp(media_jack);
}

int sl_media_jack_cable_temp_get(u8 ldev_num, u8 lgrp_num, u8 *temp)
{
	return sl_media_data_jack_cable_temp_get((sl_media_lgrp_get(ldev_num, lgrp_num))->media_jack, temp);
}

void sl_media_jack_link_led_set(u8 ldev_num, u8 lgrp_num, u32 link_state)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "link led set");

	sl_media_data_jack_link_led_set(media_lgrp->media_jack, link_state);
}

void sl_media_jack_headshell_led_set(u8 ldev_num, u8 lgrp_num, u8 jack_state)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "headshell led set");

	sl_media_data_jack_headshell_led_set(media_lgrp->media_jack, jack_state);
}
