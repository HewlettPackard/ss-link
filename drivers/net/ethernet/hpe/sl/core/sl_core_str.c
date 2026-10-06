// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022-2026 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include "sl_common.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "sl_core_str.h"

static const char * const sl_core_info_map_str_list[] = {
	[SL_CORE_INFO_MAP_MEDIA_CHECK]             = "media-check",
	[SL_CORE_INFO_MAP_MEDIA_OK]                = "media-ok",
	[SL_CORE_INFO_MAP_MEDIA_TEMPERATURE_CHECK] = "media-temperature-check",
	[SL_CORE_INFO_MAP_MEDIA_TEMPERATURE_OK]    = "media-temperature-ok",

	[SL_CORE_INFO_MAP_SERDES_START]            = "serdes-start",
	[SL_CORE_INFO_MAP_SERDES_CHECK]            = "serdes-check",
	[SL_CORE_INFO_MAP_SERDES_OK]               = "serdes-ok",

	[SL_CORE_INFO_MAP_LOOPBACK_SERDES_ON]      = "loopback-serdes-on",
	[SL_CORE_INFO_MAP_LOOPBACK_HOST_ON]        = "loopback-host-on",
	[SL_CORE_INFO_MAP_LOOPBACK_MEDIA_ON]       = "loopback-media-on",

	[SL_CORE_INFO_MAP_PCS_LOCAL_FAULT]         = "pcs-local-fault",
	[SL_CORE_INFO_MAP_PCS_REMOTE_FAULT]        = "pcs-remote-fault",
	[SL_CORE_INFO_MAP_PCS_LINK_DOWN]           = "pcs-link-down",
	[SL_CORE_INFO_MAP_PCS_CHECK]               = "pcs-check",
	[SL_CORE_INFO_MAP_PCS_OK]                  = "pcs-ok",

	[SL_CORE_INFO_MAP_FEC_CHECK]               = "fec-check",
	[SL_CORE_INFO_MAP_FEC_OK]                  = "fec-ok",
	[SL_CORE_INFO_MAP_LINK_DEGRADED]           = "link-degraded",
	[SL_CORE_INFO_MAP_FEC_SETTLE_START]        = "fec-settle-start",
	[SL_CORE_INFO_MAP_FEC_SETTLE_DONE]         = "fec-settle-done",

	[SL_CORE_INFO_MAP_LINK_DOWN]               = "link-down",
	[SL_CORE_INFO_MAP_LINK_UP_CANCEL]          = "link-up-cancel",
	[SL_CORE_INFO_MAP_LINK_UP_TIMEOUT]         = "link-up-timeout",
	[SL_CORE_INFO_MAP_LINK_UP_FAIL]            = "link-up-fail",
	[SL_CORE_INFO_MAP_LINK_UP]                 = "link-up",

	[SL_CORE_INFO_MAP_MAC_RX_CONFIG]           = "mac-rx-config",
	[SL_CORE_INFO_MAP_MAC_TX_CONFIG]           = "mac-tx-config",
	[SL_CORE_INFO_MAP_MAC_RX]                  = "mac-rx",
	[SL_CORE_INFO_MAP_MAC_TX]                  = "mac-tx",

	[SL_CORE_INFO_MAP_AN_BASE_PAGE]            = "an-base-page",
	[SL_CORE_INFO_MAP_AN_NEXT_PAGE]            = "an-next-page",
	[SL_CORE_INFO_MAP_AN_ERROR]                = "an-error",
	[SL_CORE_INFO_MAP_AN_DONE]                 = "an-done",

	[SL_CORE_INFO_MAP_LLR_CONFIG]              = "llr-config",
	[SL_CORE_INFO_MAP_LLR_SETTING_UP]          = "llr-setting-up",
	[SL_CORE_INFO_MAP_LLR_SETUP]               = "llr-setup",
	[SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT]       = "llr-setup-timeout",
	[SL_CORE_INFO_MAP_LLR_STARTING]            = "llr-starting",
	[SL_CORE_INFO_MAP_LLR_RUNNING]             = "llr-running",
	[SL_CORE_INFO_MAP_LLR_START_TIMEOUT]       = "llr-start-timeout",
	[SL_CORE_INFO_MAP_LLR_MAX_STARVATION]      = "llr-max-starvation",
	[SL_CORE_INFO_MAP_LLR_STARVED]             = "llr-starved",
	[SL_CORE_INFO_MAP_LLR_REPLAY_MAX]          = "llr-replay-max",

	[SL_CORE_INFO_MAP_HIGH_SER]                = "high-ser",

	[SL_CORE_INFO_MAP_LINK_UP_ALD_CONFIG]      = "ald-config",
	[SL_CORE_INFO_MAP_LINK_UP_ALD_CONFIG_OK]   = "ald-config-ok",

	[SL_CORE_INFO_MAP_CABLE_SPEED_200G_START]  = "speed-200G-start",
	[SL_CORE_INFO_MAP_CABLE_SPEED_200G_OK]     = "speed-200G-ok",

	[SL_CORE_INFO_MAP_CABLE_SPEED_400G_START]  = "speed-400G-start",
	[SL_CORE_INFO_MAP_CABLE_SPEED_400G_OK]     = "speed-400G-ok",
};

static const struct sl_str_map sl_core_info_str_map = {
	.strings     = sl_core_info_map_str_list,
	.num_strings = ARRAY_SIZE(sl_core_info_map_str_list),
};

#define SL_CORE_INFO_MAP_STR_MIN 10
int sl_core_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size)
{
	return sl_str_map_create(info_map, &sl_core_info_str_map,
				 info_map_str, info_map_str_size);
}

const char *sl_core_link_state_str(enum sl_core_link_state link_state)
{
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
		return "unconfigured";
	case SL_CORE_LINK_STATE_CONFIGURING:
		return "configuring";
	case SL_CORE_LINK_STATE_CONFIGURED:
		return "configured";
	case SL_CORE_LINK_STATE_DOWN:
		return "down";
	case SL_CORE_LINK_STATE_AN:
		return "an";
	case SL_CORE_LINK_STATE_GOING_UP:
		return "going-up";
	case SL_CORE_LINK_STATE_UP:
		return "up";
	case SL_CORE_LINK_STATE_CANCELING:
		return "canceling";
	case SL_CORE_LINK_STATE_GOING_DOWN:
		return "going-down";
	case SL_CORE_LINK_STATE_TIMEOUT:
		return "timeout";
	default:
		return "unknown";
	}
}

const char *sl_core_mac_state_str(enum sl_core_mac_state mac_state)
{
	switch (mac_state) {
	case SL_CORE_MAC_STATE_INVALID:
		return "invalid";
	case SL_CORE_MAC_STATE_OFF:
		return "off";
	case SL_CORE_MAC_STATE_ON:
		return "on";
	default:
		return "unknown";
	}
}

const char *sl_core_llr_state_str(enum sl_core_llr_state llr_state)
{
	switch (llr_state) {
	case SL_CORE_LLR_STATE_INVALID:
		return "invalid";
	case SL_CORE_LLR_STATE_NEW:
		return "new";
	case SL_CORE_LLR_STATE_CONFIGURED:
		return "configured";
	case SL_CORE_LLR_STATE_SETTING_UP:
		return "setting-up";
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		return "setup-timeout";
	case SL_CORE_LLR_STATE_SETUP_CANCELING:
		return "setup-canceling";
	case SL_CORE_LLR_STATE_SETUP_STOPPING:
		return "setup-stopping";
	case SL_CORE_LLR_STATE_SETUP:
		return "setup";
	case SL_CORE_LLR_STATE_STARTING:
		return "starting";
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		return "start-timeout";
	case SL_CORE_LLR_STATE_START_CANCELING:
		return "start-canceling";
	case SL_CORE_LLR_STATE_RUNNING:
		return "running";
	case SL_CORE_LLR_STATE_STOPPING:
		return "stopping";
	default:
		return "unknown";
	}
}

const char *sl_core_llr_flag_str(unsigned int llr_flag)
{
	switch (llr_flag) {
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_state_str(u8 lane_state)
{
	switch (lane_state) {
	case SL_CORE_HW_SERDES_LANE_STATE_DOWN:
		return "down";
	case SL_CORE_HW_SERDES_LANE_STATE_SETUP:
		return "setup";
	case SL_CORE_HW_SERDES_LANE_STATE_CONFIG:
		return "config";
	case SL_CORE_HW_SERDES_LANE_STATE_START:
		return "start";
	case SL_CORE_HW_SERDES_LANE_STATE_CHECK:
		return "check";
	case SL_CORE_HW_SERDES_LANE_STATE_STOP:
		return "stop";
	case SL_CORE_HW_SERDES_LANE_STATE_UP:
		return "up";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_encoding_str(u16 encoding)
{
	switch (encoding) {
	case SL_CORE_HW_SERDES_ENCODING_NRZ:
		return "NRZ";
	case SL_CORE_HW_SERDES_ENCODING_PAM4_NR:
		return "PAM4_normal";
	case SL_CORE_HW_SERDES_ENCODING_PAM4_ER:
		return "PAM4_extended";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_clocking_str(u16 clocking)
{
	switch (clocking) {
	case SL_CORE_HW_SERDES_CLOCKING_82P5:
		return "82.5/165";
	case SL_CORE_HW_SERDES_CLOCKING_85:
		return "85/170";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_osr_str(u16 osr)
{
	switch (osr) {
	case SL_CORE_HW_SERDES_OSR_OSX1:
		return "OSX1";
	case SL_CORE_HW_SERDES_OSR_OSX2:
		return "OSX2";
	case SL_CORE_HW_SERDES_OSR_OSX4:
		return "OSX4";
	case SL_CORE_HW_SERDES_OSR_OSX42P5:
		return "OSX42P5";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_width_str(u16 width)
{
	switch (width) {
	case SL_CORE_HW_SERDES_WIDTH_40:
		return "40";
	case SL_CORE_HW_SERDES_WIDTH_80:
		return "80";
	case SL_CORE_HW_SERDES_WIDTH_160:
		return "160";
	default:
		return "unknown";
	}
}

const char *sl_core_lgrp_dt_jack_part_str(u32 jack_type)
{
	switch (jack_type) {
	case SL_DT_JACK_TYPE_LEGACY:
		return "legacy";
	case SL_DT_JACK_TYPE_DD:
		return "double-density";
	case SL_DT_JACK_TYPE_EXAMAX_LEFT:
		return "examax-left";
	case SL_DT_JACK_TYPE_EXAMAX_RIGHT:
		return "examax-right";
	default:
		return "unknown";
	}
}
