// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/bitops.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "sl_core_str.h"

#define SL_CORE_INFO_MAP_DESC_SIZE 20
struct sl_core_info_map_str_item {
	char desc[SL_CORE_INFO_MAP_DESC_SIZE];
};
static struct sl_core_info_map_str_item sl_core_info_map_str_list[] = {
	[SL_CORE_INFO_MAP_MEDIA_CHECK]        = { .desc = "media-check"        },
	[SL_CORE_INFO_MAP_MEDIA_OK]           = { .desc = "media-ok"           },
	[SL_CORE_INFO_MAP_SERDES_OK]          = { .desc = "serdes-ok"          },
	[SL_CORE_INFO_MAP_PCS_LOCAL_FAULT]    = { .desc = "pcs-local-fault"    },
	[SL_CORE_INFO_MAP_PCS_REMOTE_FAULT]   = { .desc = "pcs-remote-fault"   },
	[SL_CORE_INFO_MAP_PCS_LINK_DOWN]      = { .desc = "pcs-link-down"      },
	[SL_CORE_INFO_MAP_PCS_OK]             = { .desc = "pcs-ok"             },
	[SL_CORE_INFO_MAP_FEC_CHECK]          = { .desc = "fec-check"          },
	[SL_CORE_INFO_MAP_FEC_OK]             = { .desc = "fec-ok"             },
	[SL_CORE_INFO_MAP_LINK_DEGRADED]      = { .desc = "link-degraded"      },
	[SL_CORE_INFO_MAP_LINK_DOWN]          = { .desc = "link-down"          },
	[SL_CORE_INFO_MAP_LINK_UP_CANCEL]     = { .desc = "link-up-cancel"     },
	[SL_CORE_INFO_MAP_LINK_UP_TIMEOUT]    = { .desc = "link-up-timeout"    },
	[SL_CORE_INFO_MAP_LINK_UP_FAIL]       = { .desc = "link-up-fail"       },
	[SL_CORE_INFO_MAP_LINK_UP]            = { .desc = "link-up"            },
	[SL_CORE_INFO_MAP_MAC_RX_CONFIG]      = { .desc = "mac-rx-config"      },
	[SL_CORE_INFO_MAP_MAC_TX_CONFIG]      = { .desc = "mac-tx-config"      },
	[SL_CORE_INFO_MAP_MAC_RX]             = { .desc = "mac-rx"             },
	[SL_CORE_INFO_MAP_MAC_TX]             = { .desc = "mac-tx"             },
	[SL_CORE_INFO_MAP_AN_BASE_PAGE]       = { .desc = "an-base-page"       },
	[SL_CORE_INFO_MAP_AN_NEXT_PAGE]       = { .desc = "an-next-page"       },
	[SL_CORE_INFO_MAP_AN_ERROR]           = { .desc = "an-error"           },
	[SL_CORE_INFO_MAP_AN_DONE]            = { .desc = "an-done"            },
	[SL_CORE_INFO_MAP_LLR_CONFIG]         = { .desc = "llr-config"         },
	[SL_CORE_INFO_MAP_LLR_SETTING_UP]     = { .desc = "llr-setting-up"     },
	[SL_CORE_INFO_MAP_LLR_SETUP]          = { .desc = "llr-setup"          },
	[SL_CORE_INFO_MAP_LLR_SETUP_TIMEOUT]  = { .desc = "llr-setup-timeout"  },
	[SL_CORE_INFO_MAP_LLR_STARTING]       = { .desc = "llr-starting"       },
	[SL_CORE_INFO_MAP_LLR_RUNNING]        = { .desc = "llr-running"        },
	[SL_CORE_INFO_MAP_LLR_START_TIMEOUT]  = { .desc = "llr-start-timeout"  },
	[SL_CORE_INFO_MAP_HIGH_SER]           = { .desc = "high-ser"           },
	[SL_CORE_INFO_MAP_LLR_MAX_STARVATION] = { .desc = "llr-max-starvation" },
	[SL_CORE_INFO_MAP_LLR_STARVED]        = { .desc = "llr-starved"        },
	[SL_CORE_INFO_MAP_LLR_REPLAY_MAX]     = { .desc = "llr-replay-max"     },
};

#define SL_CORE_INFO_MAP_STR_MIN 10
int sl_core_info_map_str(u64 info_map, char *info_map_str, unsigned int info_map_str_size)
{
	int          rtn;
	unsigned int x;
	unsigned int str_pos;

	if (!info_map_str)
		return -EINVAL;

	if (info_map_str_size < SL_CORE_INFO_MAP_STR_MIN)
		return -EINVAL;

	str_pos = 0;

	for (x = 0; x < SL_CORE_INFO_MAP_NUM_BITS; ++x) {
		if (!test_bit(x, (unsigned long *)&(info_map)))
			continue;
		rtn = snprintf(info_map_str + str_pos,
			info_map_str_size - str_pos, "%s ",
			sl_core_info_map_str_list[x].desc);
		if (rtn < 0) {
			str_pos = snprintf(info_map_str, info_map_str_size, "error ");
			break;
		}
		if (str_pos + rtn >= info_map_str_size) {
			info_map_str[str_pos - 2] = '.';
			info_map_str[str_pos - 3] = '.';
			info_map_str[str_pos - 4] = '.';
			break;
		}
		str_pos += rtn;
	}

	if (str_pos == 0)
		str_pos = snprintf(info_map_str, info_map_str_size, "none ");

	info_map_str[str_pos - 1] = '\0';

	return 0;
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
	case SL_CORE_LINK_STATE_GOING_UP:
		return "going-up";
	case SL_CORE_LINK_STATE_UP:
		return "up";
	case SL_CORE_LINK_STATE_AN:
		return "an";
	case SL_CORE_LINK_STATE_RECOVERING:
		return "recovering";
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
