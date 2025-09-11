// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_mac.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_mac.h"

#define LOG_NAME SL_CORE_HW_MAC_LOG_NAME

void sl_core_hw_mac_tx_config_subport(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx config subport (port = %d)", port);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_IDLE_DELAY_UPDATE(data64,
		core_mac->settings.tx_idle_delay);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_PRIORITY_THRESH_UPDATE(data64,
		core_mac->settings.tx_priority_thresh);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_THRESH_2_UPDATE(data64,
		core_mac->settings.tx_cdt_thresh_2);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_THRESH_UPDATE(data64,
		core_mac->settings.tx_cdt_thresh);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_INIT_VAL_UPDATE(data64,
		core_mac->settings.tx_cdt_init_val);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_SHORT_PREAMBLE_UPDATE(data64,
		core_mac->settings.tx_short_preamble);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num));
}
