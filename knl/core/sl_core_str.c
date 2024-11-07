// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

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
	{ .desc = "hdshl-signal"       },
	{ .desc = "hdshl-lock"         },
	{ .desc = "hdshl-timeout"      },
	{ .desc = "hdshl-ok"           },
	{ .desc = "serdes-eidl0"       },
	{ .desc = "serdes-eidl1"       },
	{ .desc = "serdes-bad-params"  },
	{ .desc = "serdes-bad-eyes"    },
	{ .desc = "serdes-ok"          },
	{ .desc = "pcs-bitlock-ok"     },

	{ .desc = "pcs-align-ok"       },
	{ .desc = "pcs-lcl-fault"      },
	{ .desc = "pcs-rmt-fault"      },
	{ .desc = "pcs-hi-serdes"      },
	{ .desc = "pcs-link-down"      },
	{ .desc = "pcs-link-up"        },
	{ .desc = "pcs-timeout"        },
	{ .desc = "pcs-ok"             },
	{ .desc = "fec-check"          },
	{ .desc = "fec-ucw-high"       },

	{ .desc = "fec-ccw-high"       },
	{ .desc = "fec-ok"             },
	{ .desc = "link-up-timeout"    },
	{ .desc = "link-up"            },
	{ .desc = "recover-timeout"    },
	{ .desc = "mac-rx-config"      },
	{ .desc = "mac-tx-config"      },
	{ .desc = "mac-rx"             },
	{ .desc = "mac-tx"             },
	{ .desc = "an-base-pg"         },

	{ .desc = "an-next-pg"         },
	{ .desc = "an-error"           },
	{ .desc = "an-done"            },
	{ .desc = "llr-config"         },
	{ .desc = "llr-setting-up"     },
	{ .desc = "llr-setup"          },
	{ .desc = "llr-setup-timeout"  },
	{ .desc = "llr-starting"       },
	{ .desc = "llr-running"        },
	{ .desc = "llr-start_timeout"  },

	{ .desc = "high-serdes"        },
	{ .desc = "llr-max-starvation" },
	{ .desc = "llr-starved"        },
	{ .desc = "llr-replay-at-max"  },
};

#define SL_CORE_INFO_MAP_STR_MIN 10
void sl_core_info_map_str(u64 info_map, char *info_map_str,
	unsigned int info_map_str_size)
{
	int          rtn;
	unsigned int x;
	unsigned int str_pos;

	BUILD_BUG_ON(ARRAY_SIZE(sl_core_info_map_str_list) == (SL_CORE_INFO_MAP_NUM_BITS - 1));

	if (info_map_str_size < SL_CORE_INFO_MAP_STR_MIN)
		return;

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
			info_map_str[str_pos - 2] = 'X';
			break;
		}
		str_pos += rtn;
	}

	if (str_pos == 0)
		str_pos = snprintf(info_map_str, info_map_str_size, "none ");

	info_map_str[str_pos - 1] = '\0';
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
	case SL_CORE_LLR_STATE_INVALID:
		return "invalid";
	case SL_CORE_LLR_STATE_OFF:
		return "off";
	case SL_CORE_LLR_STATE_CONFIGURED:
		return "configured";
	case SL_CORE_LLR_STATE_SETTING_UP:
		return "setting-up";
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		return "setup-timeout";
	case SL_CORE_LLR_STATE_SETUP:
		return "setup";
	case SL_CORE_LLR_STATE_STARTING:
		return "starting";
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		return "start-timeout";
	case SL_CORE_LLR_STATE_RUNNING:
		return "running";
	case SL_CORE_LLR_STATE_CANCELING:
		return "canceling";
	case SL_CORE_LLR_STATE_STOPPING:
		return "stopping";
	default:
		return "unknown";
	}
}

const char *sl_core_llr_flag_str(unsigned int llr_flag)
{
	switch (llr_flag) {
	case SL_CORE_LLR_FLAG_SETUP_REUSE_TIMING:
		return "setup-reuse-timing";
	case SL_CORE_LLR_FLAG_STOP_CLEAR_SETUP:
		return "stop-clear-setup";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_state_str(u8 lane_state)
{
	switch (lane_state) {
	case SL_CORE_HW_SERDES_LANE_STATE_DOWN:
		return "down";
	case SL_CORE_HW_SERDES_LANE_STATE_BUSY:
		return "busy";
	case SL_CORE_HW_SERDES_LANE_STATE_UP:
		return "up";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_encoding_str(u8 encoding)
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

const char *sl_core_serdes_lane_clocking_str(u8 clocking)
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

const char *sl_core_serdes_lane_osr_str(u8 osr)
{
	switch (osr) {
	case SL_CORE_HW_SERDES_OSR_OSX1:
		return "OSX1";
	case SL_CORE_HW_SERDES_OSR_OSX2:
		return "OSX2";
	case SL_CORE_HW_SERDES_OSR_OSX42P5:
		return "OSX42P5";
	default:
		return "unknown";
	}
}

const char *sl_core_serdes_lane_width_str(u8 width)
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
