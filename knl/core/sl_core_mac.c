// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"

#include "base/sl_core_log.h"

#include "sl_core_lgrp.h"
#include "sl_core_mac.h"
#include "sl_core_str.h"
#include "data/sl_core_data_mac.h"
#include "hw/sl_core_hw_mac.h"

#define LOG_NAME SL_CORE_MAC_LOG_NAME

int sl_core_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	return sl_core_data_mac_new(ldev_num, lgrp_num, mac_num);
}

void sl_core_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	sl_core_data_mac_del(ldev_num, lgrp_num, mac_num);
}

struct sl_core_mac *sl_core_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	return sl_core_data_mac_get(ldev_num, lgrp_num, mac_num);
}

int sl_core_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                 rtn;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "tx start");

	rtn = sl_core_data_mac_tx_settings(core_mac);
	if (rtn) {
		sl_core_log_err(core_mac, LOG_NAME, "tx start - mac_tx_settings failed [%d]", rtn);
		return -EBADRQC;
	}
	sl_core_hw_mac_tx_config(core_mac);
	sl_core_hw_mac_tx_start(core_mac);

	return 0;
}

int sl_core_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);
	if (!core_mac)
		return 0;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx stop");

	sl_core_hw_mac_tx_stop(core_mac);

	return 0;
}

int sl_core_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *mac_state)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	*mac_state = sl_core_data_mac_tx_state_get(core_mac);

	sl_core_log_dbg(core_mac, LOG_NAME,
		"state get (tx_state = %u %s)", *mac_state, sl_core_mac_state_str(*mac_state));

	return 0;
}

int sl_core_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	int                 rtn;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "rx start");

	rtn = sl_core_data_mac_rx_settings(core_mac);
	if (rtn) {
		sl_core_log_err(core_mac, LOG_NAME, "rx start - mac_rx_settings failed [%d]", rtn);
		return -EBADRQC;
	}
	sl_core_hw_mac_rx_config(core_mac);
	sl_core_hw_mac_rx_start(core_mac);

	return 0;
}

int sl_core_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);
	if (!core_mac)
		return 0;

	sl_core_log_dbg(core_mac, LOG_NAME, "rx stop");

	sl_core_hw_mac_rx_stop(core_mac);

	return 0;
}

int sl_core_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *mac_state)
{
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	*mac_state = sl_core_data_mac_rx_state_get(core_mac);

	sl_core_log_dbg(core_mac, LOG_NAME,
		"state get (rx_state = %u %s)", *mac_state, sl_core_mac_state_str(*mac_state));

	return 0;
}

u64 sl_core_mac_info_map_get(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	struct sl_core_mac *core_mac;
	u64                 info_map;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	info_map = sl_core_data_mac_info_map_get(core_mac);

	sl_core_log_dbg(core_mac, LOG_NAME, "info map get (map = 0x%llX)", info_map);

	return info_map;
}
