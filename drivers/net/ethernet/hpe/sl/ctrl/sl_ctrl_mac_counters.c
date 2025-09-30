// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "sl_ctrl_mac.h"
#include "sl_ctrl_mac_counters.h"

#define SL_CTRL_MAC_COUNTER_INIT(_mac, _counter) \
	(_mac)->counters[_counter].name = #_counter

int sl_ctrl_mac_counters_init(struct sl_ctrl_mac *ctrl_mac)
{
	if (ctrl_mac->counters)
		return -EALREADY;

	ctrl_mac->counters = kzalloc(sizeof(*ctrl_mac->counters) * SL_CTRL_MAC_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_mac->counters)
		return -ENOMEM;

	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_START_CMD);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_STARTED);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_START_FAIL);

	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_STOP_CMD);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_STOPPED);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_TX_STOP_FAIL);

	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_START_CMD);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_STARTED);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_START_FAIL);

	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_STOP_CMD);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_STOPPED);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RX_STOP_FAIL);

	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RESET_CMD);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RESET);
	SL_CTRL_MAC_COUNTER_INIT(ctrl_mac, MAC_RESET_FAIL);

	return 0;
}

int sl_ctrl_mac_counter_get(struct sl_ctrl_mac *ctrl_mac, u32 counter)
{
	return atomic_read(&ctrl_mac->counters[counter].count);
}

void sl_ctrl_mac_counters_del(struct sl_ctrl_mac *ctrl_mac)
{
	kfree(ctrl_mac->counters);
}
