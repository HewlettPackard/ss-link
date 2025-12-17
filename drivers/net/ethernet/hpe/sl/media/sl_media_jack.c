// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_asic.h"
#include "sl_core_link.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "sl_media_io.h"
#include "sl_ctrl_ldev.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_ldev.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

#define SL_MEDIA_JACK_SIGNAL_CACHE_LIFETIME_MS 1200
#define SL_MEDIA_JACK_SIGNAL_READ_TIMEOUT_MS   (2 * SL_MEDIA_JACK_SIGNAL_CACHE_LIFETIME_MS)
#define SL_MEDIA_JACK_MAX_NUM_LANE_DATA_READS  4
#define SL_MEDIA_JACK_LANE_DATA_POLL_TIME_MS   (SL_MEDIA_JACK_SIGNAL_CACHE_LIFETIME_MS / \
						SL_MEDIA_JACK_MAX_NUM_LANE_DATA_READS)

#define SWAP_BYTE_NIBBLE(_x) ({                    \
	typeof(_x) __x = (_x);                     \
	((__x & 0x0F) << 4) | ((__x & 0xF0) >> 4); \
})

#define MEDIA_LANE_DP_STATE_GET(_lane_data, _lane_num) ({ \
	typeof(_lane_data) __lane_data = (_lane_data);    \
	typeof(_lane_num)  __lane_num  = (_lane_num);     \
	(__lane_data)->dp_states[__lane_num / 2] >>       \
		(4 * (__lane_num & 1)) & 0x0F;            \
})

#define SL_MEDIA_JACK_SIGNAL_INFO_ALLOWED(_dp_state) ({         \
	typeof(_dp_state) __dp_state = (_dp_state);             \
	((__dp_state) == SL_MEDIA_JACK_DP_STATE_ACTIVATED)   || \
	((__dp_state) == SL_MEDIA_JACK_DP_STATE_TX_TURN_ON)  || \
	((__dp_state) == SL_MEDIA_JACK_DP_STATE_TX_TURN_OFF) || \
	((__dp_state) == SL_MEDIA_JACK_DP_STATE_INITIALIZED);   \
})

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

	sl_media_data_jack_led_set(media_jack);
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

	return (state == SL_MEDIA_JACK_CABLE_ONLINE);
}

bool sl_media_jack_is_cable_format_invalid(struct sl_media_jack *media_jack)
{
	bool is_format_invalid;

	spin_lock(&media_jack->data_lock);
	is_format_invalid = media_jack->is_cable_format_invalid;
	spin_unlock(&media_jack->data_lock);

	return is_format_invalid;
}

u8 sl_media_jack_active_cable_200g_host_interface_get(struct sl_media_jack *media_jack)
{
	u8 host_interface_200_gaui;

	spin_lock(&media_jack->data_lock);
	host_interface_200_gaui = media_jack->host_interface_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return host_interface_200_gaui;
}

u8 sl_media_jack_active_cable_200g_appsel_num_get(struct sl_media_jack *media_jack)
{
	u8 appsel_num_200_gaui;

	spin_lock(&media_jack->data_lock);
	appsel_num_200_gaui = media_jack->appsel_num_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return appsel_num_200_gaui;
}

u8 sl_media_jack_active_cable_200g_lane_count_get(struct sl_media_jack *media_jack)
{
	u8 lane_count_200_gaui;

	spin_lock(&media_jack->data_lock);
	lane_count_200_gaui = media_jack->lane_count_200_gaui;
	spin_unlock(&media_jack->data_lock);

	return lane_count_200_gaui;
}

u8 sl_media_jack_active_cable_400g_host_interface_get(struct sl_media_jack *media_jack)
{
	u8 host_interface_400_gaui;

	spin_lock(&media_jack->data_lock);
	host_interface_400_gaui = media_jack->host_interface_400_gaui;
	spin_unlock(&media_jack->data_lock);

	return host_interface_400_gaui;
}

u8 sl_media_jack_active_cable_400g_appsel_num_get(struct sl_media_jack *media_jack)
{
	u8 appsel_num_400_gaui;

	spin_lock(&media_jack->data_lock);
	appsel_num_400_gaui = media_jack->appsel_num_400_gaui;
	spin_unlock(&media_jack->data_lock);

	return appsel_num_400_gaui;
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

	if (media_lgrp->media_jack->is_cable_not_supported) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable not supported - shift not required");
		return 0;
	}

	if (media_lgrp->media_jack->is_supported_ss200_cable) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "ss200 cable - shift not required");
		return 0;
	}

	if (!sl_media_lgrp_media_type_is_active(media_lgrp->media_ldev->num, media_lgrp->num)) {
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

	if (media_lgrp->media_jack->is_cable_not_supported) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "cable not supported - shift not required");
		return 0;
	}

	if (media_lgrp->media_jack->is_supported_ss200_cable) {
		sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "ss200 cable - shift not required");
		return 0;
	}

	if (!sl_media_lgrp_media_type_is_active(media_lgrp->media_ldev->num, media_lgrp->num)) {
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

	if (fault_cause == SL_MEDIA_FAULT_CAUSE_HIGH_TEMP)
		sl_media_data_jack_led_set(media_jack);

	sl_ctrl_media_cause_counter_inc(media_jack, fault_cause);

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
	case SL_MEDIA_FAULT_CAUSE_HIGH_TEMP:
		return "high-temp";
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

int sl_media_jack_cable_high_temp_threshold_get(u8 ldev_num, u8 lgrp_num, u8 *temp_threshold)
{
	return sl_media_data_jack_cable_high_temp_threshold_get((sl_media_lgrp_get(ldev_num, lgrp_num))->media_jack,
								temp_threshold);
}

int sl_media_jack_cable_low_power_set(struct sl_media_jack *media_jack)
{
	return sl_media_data_jack_cable_low_power_set(media_jack);
}

void sl_media_jack_led_set(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_lgrp_get(ldev_num, lgrp_num);

	sl_media_log_dbg(media_lgrp->media_jack, LOG_NAME, "led set");

	sl_media_data_jack_led_set(media_lgrp->media_jack);
}

static const char *sl_media_jack_lane_dp_state_str(u8 dp_state)
{
	switch (dp_state) {
	case SL_MEDIA_JACK_DP_STATE_DEACTIVATED:
		return "dp-deactivated";
	case SL_MEDIA_JACK_DP_STATE_INIT:
		return "dp-init";
	case SL_MEDIA_JACK_DP_STATE_DEINIT:
		return "dp-deinit";
	case SL_MEDIA_JACK_DP_STATE_ACTIVATED:
		return "dp-activated";
	case SL_MEDIA_JACK_DP_STATE_TX_TURN_ON:
		return "dp-tx-turn-on";
	case SL_MEDIA_JACK_DP_STATE_TX_TURN_OFF:
		return "dp-tx-turn-off";
	case SL_MEDIA_JACK_DP_STATE_INITIALIZED:
		return "dp-initialized";
	default:
		return "unknown";
	}
}

/* Swap the lanes in the lane data structure. See sl_core_lgrp_media_lane_data_swap
 * for more details.
 */
static void sl_media_jack_lane_data_swap(struct sl_media_jack_lane_data *lane_data)
{
	u8 dp_states;

	lane_data->signal.rx.los_map = SWAP_BYTE_NIBBLE(lane_data->signal.rx.los_map);
	lane_data->signal.tx.los_map = SWAP_BYTE_NIBBLE(lane_data->signal.tx.los_map);
	lane_data->signal.rx.lol_map = SWAP_BYTE_NIBBLE(lane_data->signal.rx.lol_map);
	lane_data->signal.tx.lol_map = SWAP_BYTE_NIBBLE(lane_data->signal.tx.lol_map);
	lane_data->dp_states_changed = SWAP_BYTE_NIBBLE(lane_data->dp_states_changed);

	/* Swap lanes 0 & 1 for 4 & 5 and visa versa */
	dp_states = lane_data->dp_states[2];
	lane_data->dp_states[2] = lane_data->dp_states[0];
	lane_data->dp_states[0] = dp_states;

	/* Swap lanes 2 & 3 for 6 & 7 and visa versa */
	dp_states = lane_data->dp_states[3];
	lane_data->dp_states[3] = lane_data->dp_states[1];
	lane_data->dp_states[1] = dp_states;
}

#define SL_MEDIA_JACK_DP_STATE_OFFSET    128
#define SL_MEDIA_JACK_TX_LOS_OFFSET      136
#define SL_MEDIA_JACK_RX_LOS_OFFSET      147
#define SL_MEDIA_JACK_MAX_COHERENT_BYTES 8
static int sl_media_jack_lane_data_read(u8 ldev_num, u8 lgrp_num, struct sl_media_jack_lane_data *lane_data)
{
	int                             rtn;
	u8                              read_bytes[SL_MEDIA_JACK_MAX_COHERENT_BYTES];
	bool                            swap;
	struct sl_media_jack_lane_data  tmp_lane_data;
	struct sl_media_jack           *media_jack;

	media_jack = sl_media_lgrp_get(ldev_num, lgrp_num)->media_jack;

	sl_media_log_dbg(media_jack, LOG_NAME, "media_io_read_lane_data (ldev_num = %u, lgrp_num = %u)",
			 ldev_num, lgrp_num);

	rtn = sl_media_io_read(media_jack, 0x11, SL_MEDIA_JACK_TX_LOS_OFFSET, (u8 *)&tmp_lane_data.signal.tx, 2);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media_io_read_data failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_media_io_read(media_jack, 0x11, SL_MEDIA_JACK_RX_LOS_OFFSET, (u8 *)&tmp_lane_data.signal.rx, 2);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media_io_read_data failed [%d]", rtn);
		return -EIO;
	}

	rtn = sl_media_io_read(media_jack, 0x11, SL_MEDIA_JACK_DP_STATE_OFFSET, read_bytes,
			       SL_MEDIA_JACK_MAX_COHERENT_BYTES);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "media_io_read_data failed [%d]", rtn);
		return -EIO;
	}

	memcpy(tmp_lane_data.dp_states, read_bytes, 4);
	tmp_lane_data.dp_states_changed = read_bytes[6];

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lane data read (dp_states_changed = 0x%X)", tmp_lane_data.dp_states_changed);

	swap = sl_core_lgrp_media_lane_data_swap(ldev_num, lgrp_num);
	if (swap)
		sl_media_jack_lane_data_swap(&tmp_lane_data);

	sl_media_log_dbg(media_jack, LOG_NAME, "lane data swap = %s", swap ? "yes" : "no");

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lane data read map (rx_lol = 0x%X, tx_lol = 0x%X, rx_los = 0x%X, tx_los = 0x%X)",
			 tmp_lane_data.signal.rx.lol_map, tmp_lane_data.signal.tx.lol_map,
			 tmp_lane_data.signal.rx.los_map, tmp_lane_data.signal.tx.los_map);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lanes dp_state (0 = %s, 1 = %s, 2 = %s, 3 = %s, 4 = %s, 5 = %s, 6 = %s, 7 = %s)",
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 0)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 1)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 2)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 3)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 4)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 5)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 6)),
			 sl_media_jack_lane_dp_state_str(MEDIA_LANE_DP_STATE_GET(&tmp_lane_data, 7)));

	spin_lock(&media_jack->data_lock);
	memcpy(lane_data, &tmp_lane_data, sizeof(*lane_data));
	spin_unlock(&media_jack->data_lock);

	return 0;
}

static bool sl_media_jack_is_lane_data_valid(struct sl_media_jack *media_jack, unsigned long serdes_lane_map,
					     struct sl_media_jack_lane_data *lane_data)
{
	u8 serdes_lane_num;
	u8 lane_dp_state;

	sl_media_log_dbg(media_jack, LOG_NAME, "lane data valid (serdes_lane_map = 0x%lx)", serdes_lane_map);

	if (lane_data->dp_states_changed & serdes_lane_map) {
		sl_media_log_err(media_jack, LOG_NAME,
				 "lane data valid DP state changed (dp_states_changed = 0x%X, serdes_lane_map = 0x%lx)",
				 lane_data->dp_states_changed, serdes_lane_map);
		return false;
	}

	for_each_set_bit(serdes_lane_num, &serdes_lane_map, SL_ASIC_MAX_LANES) {
		lane_dp_state = MEDIA_LANE_DP_STATE_GET(lane_data, serdes_lane_num);

		if (!SL_MEDIA_JACK_SIGNAL_INFO_ALLOWED(lane_dp_state)) {
			sl_media_log_err(media_jack, LOG_NAME,
					 "lane data valid DP state not allowed (lane_num = %u, dp_state = %u %s)",
					 serdes_lane_num, lane_dp_state,
					 sl_media_jack_lane_dp_state_str(lane_dp_state));
			return false;
		}
	}

	return true;
}

static void sl_media_jack_lane_data_cache_set(struct sl_media_jack *media_jack,
					      struct sl_media_jack_lane_data *lane_data)
{
	spin_lock(&media_jack->data_lock);
	media_jack->lane_data.cache.data   = *lane_data;
	media_jack->lane_data.cache.cached = true;
	media_jack->lane_data.cache.timestamp_s = ktime_get_real_seconds();
	spin_unlock(&media_jack->data_lock);
}

static bool sl_media_jack_lane_data_cache_get(struct sl_media_jack *media_jack,
					      struct sl_media_jack_lane_data *lane_data)
{
	bool     cached;

	spin_lock(&media_jack->data_lock);
	cached     = media_jack->lane_data.cache.cached;
	*lane_data = media_jack->lane_data.cache.data;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lane data cache get map (rx_los = 0x%X, tx_los = 0x%X, rx_lol = 0x%X, tx_lol = 0x%X)",
			 lane_data->signal.rx.los_map, lane_data->signal.tx.los_map,
			 lane_data->signal.rx.lol_map, lane_data->signal.tx.lol_map);

	return cached;
}

static bool sl_media_jack_signal_equal(struct sl_media_jack_signal *a, struct sl_media_jack_signal *b)
{
	return (a->rx.los_map == b->rx.los_map) &&
	       (a->tx.los_map == b->tx.los_map) &&
	       (a->rx.lol_map == b->rx.lol_map) &&
	       (a->tx.lol_map == b->tx.lol_map);
}

static void sl_media_jack_signal_or(struct sl_media_jack_signal *a, struct sl_media_jack_signal *b)
{
	a->rx.los_map |= b->rx.los_map;
	a->tx.los_map |= b->tx.los_map;
	a->rx.lol_map |= b->rx.lol_map;
	a->tx.lol_map |= b->tx.lol_map;
}

static int sl_media_jack_lane_data_get(u8 ldev_num, u8 lgrp_num, u8 serdes_lane_map,
				       struct sl_media_jack_lane_data *lane_data)
{
	int                             rtn;
	u8                              read_count;
	bool                            is_stable;
	struct sl_media_jack_lane_data  current_lane_data;
	struct sl_media_jack_signal     prev_signal;
	struct sl_media_jack           *media_jack;

	media_jack = sl_media_lgrp_get(ldev_num, lgrp_num)->media_jack;

	sl_media_log_dbg(media_jack, LOG_NAME, "lane data get (ldev_num = %u, lgrp_num = %u, serdes_lane_map = 0x%X)",
			 ldev_num, lgrp_num, serdes_lane_map);

	rtn = sl_media_jack_lane_data_read(ldev_num, lgrp_num, lane_data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read failed [%d]", rtn);
		return rtn;
	}

	msleep(SL_MEDIA_JACK_LANE_DATA_POLL_TIME_MS);

	read_count = 0;
	is_stable  = false;

	rtn = sl_media_jack_lane_data_read(ldev_num, lgrp_num, lane_data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read failed [%d]", rtn);
		return rtn;
	}

	if (!sl_media_jack_is_lane_data_valid(media_jack, serdes_lane_map, lane_data)) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "lane data not valid");
		return -EIO;
	}

	/* Short circuit, no new bits can be set */
	if (lane_data->signal.rx.los_map == 0xFF &&
	    lane_data->signal.tx.los_map == 0xFF &&
	    lane_data->signal.rx.lol_map == 0xFF &&
	    lane_data->signal.tx.lol_map == 0xFF) {
		read_count++;
		is_stable = true;
		goto out;
	}

	prev_signal = lane_data->signal;

	msleep(SL_MEDIA_JACK_LANE_DATA_POLL_TIME_MS);

	for (read_count = 1; read_count < SL_MEDIA_JACK_MAX_NUM_LANE_DATA_READS; ++read_count) {
		rtn = sl_media_jack_lane_data_read(ldev_num, lgrp_num, &current_lane_data);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read failed [%d]", rtn);
			return rtn;
		}

		if (!sl_media_jack_is_lane_data_valid(media_jack, serdes_lane_map, &current_lane_data)) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data not valid");
			return -EIO;
		}

		prev_signal = lane_data->signal;
		sl_media_jack_signal_or(&lane_data->signal, &current_lane_data.signal);

		is_stable = sl_media_jack_signal_equal(&lane_data->signal, &prev_signal);
		if (is_stable) {
			++read_count;
			break;
		}

		msleep(SL_MEDIA_JACK_LANE_DATA_POLL_TIME_MS);
	}

out:
	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lane data get (read_count = %u)", read_count);

	if (!is_stable) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "lane data unstable");
		return -EIO;
	}

	sl_media_jack_lane_data_cache_set(media_jack, lane_data);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "lane data get map (rx_los = 0x%X, tx_los = 0x%X, rx_lol = 0x%X, tx_lol = 0x%X)",
			 lane_data->signal.rx.los_map, lane_data->signal.tx.los_map,
			 lane_data->signal.rx.lol_map, lane_data->signal.tx.lol_map);

	return 0;
}

int sl_media_jack_signal_get(u8 ldev_num, u8 lgrp_num, u8 serdes_lane_map, struct sl_media_jack_signal *media_signal)
{
	int                             rtn;
	struct sl_media_jack_lane_data  lane_data;
	struct sl_media_jack           *media_jack;
	bool                            cached;
	unsigned long                   time_left;

	media_jack = sl_media_lgrp_get(ldev_num, lgrp_num)->media_jack;

	sl_media_log_dbg(media_jack, LOG_NAME, "signal get (ldev_num = %u, lgrp_num = %u, serdes_lane_map = 0x%X)",
			 ldev_num, lgrp_num, serdes_lane_map);

	if (!sl_media_lgrp_media_type_is_active(ldev_num, lgrp_num)) {
		sl_media_log_warn_trace(media_jack, LOG_NAME, "cable not active");
		return -EBADRQC;
	}

	if (!sl_media_lgrp_is_signal_status_supported(ldev_num, lgrp_num)) {
		sl_media_log_err(media_jack, LOG_NAME, "signal status not supported");
		return -EBADRQC;
	}

	spin_lock(&media_jack->data_lock);
	switch (media_jack->lane_data.read_state) {
	case SL_MEDIA_JACK_LANE_DATA_READ_STATE_IDLE:
		media_jack->lane_data.read_state = SL_MEDIA_JACK_LANE_DATA_READ_STATE_BUSY;
		sl_media_log_dbg(media_jack, LOG_NAME, "reading lane data - state set to BUSY");
		spin_unlock(&media_jack->data_lock);

		init_completion(&media_jack->lane_data.read_complete);

		rtn = sl_media_jack_lane_data_get(ldev_num, lgrp_num, serdes_lane_map, &lane_data);

		spin_lock(&media_jack->data_lock);
		media_jack->lane_data.read_state    = SL_MEDIA_JACK_LANE_DATA_READ_STATE_IDLE;
		media_jack->lane_data.last_read_rtn = rtn;
		spin_unlock(&media_jack->data_lock);

		complete(&media_jack->lane_data.read_complete);

		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read failed [%d]", rtn);
			return rtn;
		}

		goto out;

	case SL_MEDIA_JACK_LANE_DATA_READ_STATE_BUSY:
		sl_media_log_dbg(media_jack, LOG_NAME, "waiting for lane data read to complete");
		spin_unlock(&media_jack->data_lock);

		time_left = wait_for_completion_timeout(&media_jack->lane_data.read_complete,
							msecs_to_jiffies(SL_MEDIA_JACK_SIGNAL_READ_TIMEOUT_MS));

		if (time_left == 0) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read timed out");
			return -ETIMEDOUT;
		}

		spin_lock(&media_jack->data_lock);
		rtn = media_jack->lane_data.last_read_rtn;
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data read failed [%d]", rtn);
			spin_unlock(&media_jack->data_lock);
			return rtn;
		}

		cached    = media_jack->lane_data.cache.cached;
		lane_data = media_jack->lane_data.cache.data;
		spin_unlock(&media_jack->data_lock);

		if (!cached) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "lane data not cached");
			return -EIO;
		}

		goto out;

	default:
		sl_media_log_err(media_jack, LOG_NAME, "invalid lane data read state (%u)",
				 media_jack->lane_data.read_state);
		spin_unlock(&media_jack->data_lock);
		return -EIO;
	}

out:
	*media_signal = lane_data.signal;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "signal get (rx_los = 0x%X, rx_lol = 0x%X, tx_los = 0x%X, tx_lol = 0x%X)",
			 media_signal->rx.los_map, media_signal->rx.lol_map,
			 media_signal->tx.los_map, media_signal->tx.lol_map);

	return 0;
}

int sl_media_jack_signal_cache_get(u8 ldev_num, u8 lgrp_num, u8 serdes_lane_map,
				   struct sl_media_jack_signal *media_signal)
{
	struct sl_media_jack_lane_data  lane_data;
	struct sl_media_jack           *media_jack;

	media_jack = sl_media_lgrp_get(ldev_num, lgrp_num)->media_jack;

	sl_media_log_dbg(media_jack, LOG_NAME, "signal cache get (ldev_num =%u, lgrp_num = %u, serdes_lane_map = 0x%X)",
			 ldev_num, lgrp_num, serdes_lane_map);

	if (!sl_media_jack_lane_data_cache_get(media_jack, &lane_data)) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "lane data not cached");
		return -ENOENT;
	}

	*media_signal = lane_data.signal;

	return 0;
}

int sl_media_jack_signal_cache_time_s_get(u8 ldev_num, u8 lgrp_num, time64_t *cache_time)
{
	bool                  cached;
	struct sl_media_jack *media_jack;

	media_jack = sl_media_lgrp_get(ldev_num, lgrp_num)->media_jack;

	sl_media_log_dbg(media_jack, LOG_NAME, "signal cache time get (ldev_num =%u, lgrp_num = %u)",
			 ldev_num, lgrp_num);

	spin_lock(&media_jack->data_lock);
	cached = media_jack->lane_data.cache.cached;
	*cache_time = media_jack->lane_data.cache.timestamp_s;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "signal cache time get (cache_time = %lld, cached = %s)",
			 *cache_time, cached ? "yes" : "no");

	return cached ? 0 : -ENOENT;
}

int sl_media_jack_attr_error_map_str(u32 error_map, char *error_str, unsigned int error_str_size)
{
	int rtn;
	int str_pos;
	int which;

	if (!error_str)
		return -EINVAL;

	if (error_str_size < SL_MEDIA_JACK_ATTR_ERR_STR_SIZE)
		return -EINVAL;

	if (!error_map) {
		str_pos = snprintf(error_str, error_str_size, "none ");
		goto out;
	}

	str_pos = 0;

	for_each_set_bit(which, (unsigned long *)&error_map, sizeof(error_map) * BITS_PER_BYTE) {
		switch (BIT(which)) {
		case SL_MEDIA_ERROR_CABLE_NOT_SUPPORTED:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "cable-not-supported ");
			break;
		case SL_MEDIA_ERROR_CABLE_FORMAT_INVALID:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "cable-format-invalid ");
			break;
		case SL_MEDIA_ERROR_CABLE_FW_INVALID:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "cable-fw-invalid ");
			break;
		case SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "cable-headshell-fault ");
			break;
		case SL_MEDIA_ERROR_TEMP_THRESH_DEFAULT:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "temp-threshold-default ");
			break;
		case SL_MEDIA_ERROR_TRYABLE:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "tryable ");
			break;
		default:
			rtn = snprintf(error_str + str_pos, error_str_size - str_pos, "unknown ");
			break;
		}

		if (rtn < 0) {
			str_pos = snprintf(error_str, error_str_size, "error ");
			goto out;
		}
		if (str_pos + rtn >= error_str_size) {
			error_str[str_pos - 2] = '.';
			error_str[str_pos - 3] = '.';
			error_str[str_pos - 4] = '.';
			break;
		}
		str_pos += rtn;
	}

out:
	error_str[str_pos - 1] = '\0';

	return 0;
}

int sl_media_jack_attr_error_map_get(struct sl_media_jack *media_jack, u32 *error_map)
{
	spin_lock(&media_jack->data_lock);
	*error_map = media_jack->cable_info->media_attr.errors;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "attr error map get (error_map = %u)", *error_map);

	return 0;
}
