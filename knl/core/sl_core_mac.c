// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

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
	u32                 mac_state;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "tx start");

	mac_state = sl_core_data_mac_tx_state_get(core_mac);
	switch (mac_state) {
	case SL_CORE_MAC_STATE_ON:
		sl_core_log_dbg(core_mac, LOG_NAME, "tx start - already ON");
		return 0;
	case SL_CORE_MAC_STATE_OFF:
		sl_core_log_dbg(core_mac, LOG_NAME, "tx start - turning ON");
		rtn = sl_core_data_mac_settings(core_mac);
		if (rtn) {
			sl_core_log_err(core_mac, LOG_NAME, "tx start - mac_settings failed [%d]", rtn);
			return -EBADRQC;
		}
		sl_core_hw_mac_tx_config(core_mac);
		sl_core_hw_mac_tx_start(core_mac);
		return 0;
	default:
		sl_core_log_err(core_mac, LOG_NAME,
			"tx start - invalid (mac_state = %u %s)",
			mac_state, sl_core_mac_state_str(mac_state));
		return -EBADRQC;
	}
}

int sl_core_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	u32                 mac_state;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "tx stop");

	mac_state = sl_core_data_mac_tx_state_get(core_mac);
	switch (mac_state) {
	case SL_CORE_MAC_STATE_OFF:
		sl_core_log_dbg(core_mac, LOG_NAME, "tx stop - already OFF");
		return 0;
	case SL_CORE_MAC_STATE_ON:
		sl_core_log_dbg(core_mac, LOG_NAME, "tx stop - turning OFF");
		sl_core_hw_mac_tx_stop(core_mac);
		return 0;
	default:
		sl_core_log_err(core_mac, LOG_NAME,
			"tx stop - invalid (mac_state = %u %s)",
			mac_state, sl_core_mac_state_str(mac_state));
		return -EBADRQC;
	}
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
	u32                 mac_state;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "rx start");

	mac_state = sl_core_data_mac_rx_state_get(core_mac);
	switch (mac_state) {
	case SL_CORE_MAC_STATE_ON:
		sl_core_log_dbg(core_mac, LOG_NAME, "rx start - already ON");
		return 0;
	case SL_CORE_MAC_STATE_OFF:
		sl_core_log_dbg(core_mac, LOG_NAME, "rx start - turning ON");
		rtn = sl_core_data_mac_settings(core_mac);
		if (rtn) {
			sl_core_log_err(core_mac, LOG_NAME, "rx start - mac_settings failed [%d]", rtn);
			return -EBADRQC;
		}
		sl_core_hw_mac_rx_config(core_mac);
		sl_core_hw_mac_rx_start(core_mac);
		return 0;
	default:
		sl_core_log_err(core_mac, LOG_NAME,
			"rx start - invalid (mac_state = %u %s)",
			mac_state, sl_core_mac_state_str(mac_state));
		return -EBADRQC;
	}
}

int sl_core_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num)
{
	u32                 mac_state;
	struct sl_core_mac *core_mac;

	core_mac = sl_core_mac_get(ldev_num, lgrp_num, mac_num);

	sl_core_log_dbg(core_mac, LOG_NAME, "rx stop");

	mac_state = sl_core_data_mac_rx_state_get(core_mac);
	switch (mac_state) {
	case SL_CORE_MAC_STATE_OFF:
		sl_core_log_dbg(core_mac, LOG_NAME, "rx stop - already OFF");
		return 0;
	case SL_CORE_MAC_STATE_ON:
		sl_core_log_dbg(core_mac, LOG_NAME, "rx stop - turning OFF");
		sl_core_hw_mac_rx_stop(core_mac);
		return 0;
	default:
		sl_core_log_err(core_mac, LOG_NAME,
			"rx stop - invalid (mac_state = %u %s)",
			mac_state, sl_core_mac_state_str(mac_state));
		return -EBADRQC;
	}
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
