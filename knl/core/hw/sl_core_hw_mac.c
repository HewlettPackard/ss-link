// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_kconfig.h"

#include "base/sl_core_log.h"
#include "sl_core_mac.h"
#include "data/sl_core_data_mac.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_mac.h"

#define LOG_NAME SL_CORE_HW_MAC_LOG_NAME

void sl_core_hw_mac_tx_config(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx - config (port = %d)", port);

	sl_core_data_mac_info_map_set(core_mac, SL_CORE_INFO_MAP_MAC_TX_CONFIG);

	/* need to set mac_if_credits for llr before mac is turned on */
	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_LLR_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_LLR_SUBPORT_MAC_IF_CREDITS_UPDATE(data64,
		core_mac->settings.llr_if_credits);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_LLR_SUBPORT(core_mac->num), data64);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_TX_MAC, &data64);
	data64 = SS2_PORT_PML_CFG_TX_MAC_IEEE_IFG_ADJUSTMENT_UPDATE(data64,
		core_mac->settings.tx_ifg_adj);
	data64 = SS2_PORT_PML_CFG_TX_MAC_IFG_MODE_UPDATE(data64,
		core_mac->settings.tx_ifg_mode);
	data64 = SS2_PORT_PML_CFG_TX_MAC_MAC_PAD_IDLE_THRESH_UPDATE(data64,
		core_mac->settings.tx_pad_idle_thresh);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_TX_MAC, data64);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), &data64);
#ifdef BUILDSYS_FRAMEWORK_ROSETTA
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_IDLE_DELAY_UPDATE(data64,
		core_mac->settings.tx_idle_delay);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_PRIORITY_THRESH_UPDATE(data64,
		core_mac->settings.tx_priority_thresh);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_THRESH_2_UPDATE(data64,
		core_mac->settings.tx_cdt_thresh_2);
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_THRESH_UPDATE(data64,
		core_mac->settings.tx_cdt_thresh);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_CDT_INIT_VAL_UPDATE(data64,
		core_mac->settings.tx_cdt_init_val);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_SHORT_PREAMBLE_UPDATE(data64,
		core_mac->settings.tx_short_preamble);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), data64);
}

void sl_core_hw_mac_tx_start(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx - start (port = %d)", port);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(data64, 1);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num));

	sl_core_data_mac_info_map_set(core_mac, SL_CORE_INFO_MAP_MAC_TX);
	sl_core_data_mac_tx_state_set(core_mac, SL_CORE_MAC_STATE_ON);
}

void sl_core_hw_mac_tx_stop(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "tx - stop (port = %d)", port);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(data64, 0);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_mac->num));

	sl_core_data_mac_info_map_clr(core_mac, SL_CORE_INFO_MAP_MAC_TX);
	sl_core_data_mac_tx_state_set(core_mac, SL_CORE_MAC_STATE_OFF);
}

void sl_core_hw_mac_rx_config(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "rx - config (port = %d)", port);

	sl_core_data_mac_info_map_set(core_mac, SL_CORE_INFO_MAP_MAC_RX_CONFIG);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_RX_MAC, &data64);
	data64 = SS2_PORT_PML_CFG_RX_MAC_FLIT_PACKING_CNT_UPDATE(data64,
		core_mac->settings.rx_flit_packing_cnt);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_RX_MAC, data64);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_MAC_SUBPORT_SHORT_PREAMBLE_UPDATE(data64,
		core_mac->settings.rx_short_preamble);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num));
}

void sl_core_hw_mac_rx_start(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "rx - start (port = %d)", port);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(data64, 1);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num));

	sl_core_data_mac_info_map_set(core_mac, SL_CORE_INFO_MAP_MAC_RX);
	sl_core_data_mac_rx_state_set(core_mac, SL_CORE_MAC_STATE_ON);
}

void sl_core_hw_mac_rx_stop(struct sl_core_mac *core_mac)
{
	u64 data64;
	u32 port;

	port = core_mac->core_lgrp->num;

	sl_core_log_dbg(core_mac, LOG_NAME, "rx - stop (port = %d)", port);

	sl_core_mac_read64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_MAC_SUBPORT_MAC_OPERATIONAL_UPDATE(data64, 0);
	sl_core_mac_write64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num), data64);

	sl_core_mac_flush64(core_mac, SS2_PORT_PML_CFG_RX_MAC_SUBPORT(core_mac->num));

	sl_core_data_mac_info_map_clr(core_mac, SL_CORE_INFO_MAP_MAC_RX);
	sl_core_data_mac_rx_state_set(core_mac, SL_CORE_MAC_STATE_OFF);
}
