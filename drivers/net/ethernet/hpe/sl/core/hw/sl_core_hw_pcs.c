// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_media_lgrp.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_ldev.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pcs.h"

#define LOG_NAME SL_CORE_HW_PCS_LOG_NAME

void sl_core_hw_pcs_config(struct sl_core_link *core_link)
{
	u64 data64;
	u64 lanes;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "config (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS, &data64);
	data64 = SS2_PORT_PML_CFG_PCS_TIMESTAMP_SHIFT_UPDATE(data64, 0);

	if (sl_core_link_config_is_enable_ald_set(core_link))
		data64 = SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(data64, 1);
	else
		data64 = SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(data64, 0);

	data64 = SS2_PORT_PML_CFG_PCS_PCS_MODE_UPDATE(data64,
		core_link->pcs.settings.pcs_mode);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS, data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_GENERAL, &data64);
	data64 = SS2_PORT_PML_CFG_GENERAL_CLOCK_PERIOD_PS_UPDATE(data64,
		core_link->pcs.settings.clock_period);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_GENERAL, data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_RS_MODE_UPDATE(data64,
		core_link->pcs.settings.rs_mode);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_LOCK_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_RX_SM_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_SUBPORT_GEARBOX_CREDITS_UPDATE(data64,
		core_link->pcs.settings.tx_gearbox_credits);
	data64 = SS2_PORT_PML_CFG_TX_PCS_SUBPORT_ENABLE_CTL_OS_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_PCS, &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_CDC_READY_LEVEL_UPDATE(data64,
		core_link->pcs.settings.tx_cdc_ready_level);
	data64 = SS2_PORT_PML_CFG_TX_PCS_EN_PK_BW_LIMITER_UPDATE(data64,
		core_link->pcs.settings.tx_en_pk_bw_limiter);
	data64 = SS2_PORT_PML_CFG_TX_PCS_EN_PK_BW_LIMITER_UPDATE(data64,
		core_link->pcs.settings.tx_en_pk_bw_limiter);
	data64 = SS2_PORT_PML_CFG_TX_PCS_CDC_READY_LEVEL_UPDATE(data64,
		core_link->pcs.settings.tx_cdc_ready_level);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_PCS, data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_CW_GAP_544_UPDATE(data64,
		core_link->pcs.settings.cw_gap);

	if (sl_core_link_config_is_enable_ald_set(core_link))
		data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 0);
	else
		data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 1);

	data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_AMS_UPDATE(data64,
		core_link->pcs.settings.rx_restart_lock_on_bad_ams);
	lanes = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_GET(data64);
	lanes &= ~core_link->pcs.settings.rx_active_lanes;
	data64 = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(data64, lanes);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);

	/* need to set mac_tx_pcs_credits before pcs is enabled */
	sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_TX_MAC_SUBPORT_PCS_CREDITS_UPDATE(data64,
		core_link->pcs.settings.mac_tx_credits);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_MAC_SUBPORT(core_link->num), data64);

	sl_core_hw_pcs_config_swizzles(core_link);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);
}

/* Timeout for Health Markers. We'll surely receive health markers in 100 us, if any */
#define HM_RECV_TIMEOUT_US 100

/* Max number of Physical Lanes available on Tx/Rx */
#define MAX_PLS_AVAILABLE 0xF

void sl_core_hw_pcs_enable_auto_lane_degrade(struct sl_core_link *core_link)
{
	u64           data64;
	u32           port;
	unsigned long timeout;
	int           err;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "enable ald (port = %u)", port);

	err = 0;
	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS, &data64);
	if (SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_GET(data64)) {
		timeout = jiffies + usecs_to_jiffies(HM_RECV_TIMEOUT_US);
		do {
		    sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_LANE_DEGRADE, &data64);
		    if (time_after(jiffies, timeout)) {
		        sl_core_log_err(core_link, LOG_NAME, "%u: timeout for rx pls available", port);
		        err = -ETIMEDOUT;
		        goto out;
		    }
		} while (SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_RX_PLS_AVAILABLE_GET(data64) != MAX_PLS_AVAILABLE);

		timeout = jiffies + usecs_to_jiffies(HM_RECV_TIMEOUT_US);
		do {
		    sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_LANE_DEGRADE, &data64);
		    if (time_after(jiffies, timeout)) {
		        sl_core_log_err(core_link, LOG_NAME, "%u: timeout for lp pls available", port);
		        err = -ETIMEDOUT;
		        goto out;
		    }
		} while (SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_LP_PLS_AVAILABLE_GET(data64) != MAX_PLS_AVAILABLE);

		/*
		 * enable RX degrade before TX degrade
		 */
		sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
		data64 = SS2_PORT_PML_CFG_RX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(data64, 1);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);

		sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_PCS, &data64);
		data64 = SS2_PORT_PML_CFG_TX_PCS_ALLOW_AUTO_DEGRADE_UPDATE(data64, 1);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_PCS, data64);
		sl_core_flush64(core_link, SS2_PORT_PML_CFG_TX_PCS);

		sl_core_log_dbg(core_link, LOG_NAME, "auto lane degrade is enabled (port = %u)", port);

		spin_lock(&core_link->data_lock);
		core_link->degrade_state = SL_LINK_DEGRADE_STATE_ACTIVE;
		spin_unlock(&core_link->data_lock);
	}
out:
	if (err) {
		spin_lock(&core_link->data_lock);
		core_link->degrade_state = SL_LINK_DEGRADE_STATE_FAILED;
		spin_unlock(&core_link->data_lock);

		sl_core_log_err_trace(core_link, LOG_NAME, "enable auto lane degrade failed (port = %u)", port);
		sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS, &data64);
		data64 = SS2_PORT_PML_CFG_PCS_ENABLE_AUTO_LANE_DEGRADE_UPDATE(data64, 0);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS, data64);

		sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
		data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 1);
		sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);
		sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);
	}
}

void sl_core_hw_pcs_config_swizzles(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;
	u8  lane_num;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "config swizzles (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
	for (lane_num = 0; lane_num < SL_MAX_LANES; ++lane_num) {
		switch (core_link->core_lgrp->serdes.dt.lane_info[lane_num].rx_source) {
		case 3:
			data64 = SS2_PORT_PML_CFG_RX_PCS_LANE_3_SOURCE_UPDATE(data64, lane_num);
			break;
		case 2:
			data64 = SS2_PORT_PML_CFG_RX_PCS_LANE_2_SOURCE_UPDATE(data64, lane_num);
			break;
		case 1:
			data64 = SS2_PORT_PML_CFG_RX_PCS_LANE_1_SOURCE_UPDATE(data64, lane_num);
			break;
		case 0:
			data64 = SS2_PORT_PML_CFG_RX_PCS_LANE_0_SOURCE_UPDATE(data64, lane_num);
			break;
		}
	}
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_TX_PCS, &data64);
	data64 = SS2_PORT_PML_CFG_TX_PCS_LANE_3_SOURCE_UPDATE(data64,
		core_link->core_lgrp->serdes.dt.lane_info[3].tx_source);
	data64 = SS2_PORT_PML_CFG_TX_PCS_LANE_2_SOURCE_UPDATE(data64,
		core_link->core_lgrp->serdes.dt.lane_info[2].tx_source);
	data64 = SS2_PORT_PML_CFG_TX_PCS_LANE_1_SOURCE_UPDATE(data64,
		core_link->core_lgrp->serdes.dt.lane_info[1].tx_source);
	data64 = SS2_PORT_PML_CFG_TX_PCS_LANE_0_SOURCE_UPDATE(data64,
		core_link->core_lgrp->serdes.dt.lane_info[0].tx_source);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_TX_PCS, data64);
}

void sl_core_hw_pcs_tx_start(struct sl_core_link *core_link)
{
	u64 data64;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "tx start (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_PCS_ENABLE_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);
}

void sl_core_hw_pcs_rx_start(struct sl_core_link *core_link)
{
	u64 data64;
	u64 lanes;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"rx start (port = %u, active_lanes = 0x%X)",
		port, core_link->pcs.settings.rx_active_lanes);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_LOCK_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
	lanes = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_GET(data64);
	lanes |= core_link->pcs.settings.rx_active_lanes;
	data64 = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(data64, lanes);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);
}

void sl_core_hw_pcs_stop(struct sl_core_link *core_link)
{
	u64 data64;
	u64 lanes;
	u32 port;

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"stop (port = %u, active_lanes = 0x%X)",
		port, core_link->pcs.settings.rx_active_lanes);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS, &data64);
	lanes = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_GET(data64);
	lanes &= ~core_link->pcs.settings.rx_active_lanes;
	data64 = SS2_PORT_PML_CFG_RX_PCS_ACTIVE_LANES_UPDATE(data64, lanes);
	data64 = SS2_PORT_PML_CFG_RX_PCS_RESTART_LOCK_ON_BAD_CWS_UPDATE(data64, 1);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS, data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_RX_PCS);

	msleep(20);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_PCS_SUBPORT_PCS_ENABLE_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num), data64);

	sl_core_read64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), &data64);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_LOCK_UPDATE(data64, 0);
	data64 = SS2_PORT_PML_CFG_RX_PCS_SUBPORT_ENABLE_RX_SM_UPDATE(data64, 0);
	sl_core_write64(core_link, SS2_PORT_PML_CFG_RX_PCS_SUBPORT(core_link->num), data64);

	sl_core_flush64(core_link, SS2_PORT_PML_CFG_PCS_SUBPORT(core_link->num));
}

bool sl_core_hw_pcs_is_ok(struct sl_core_link *core_link)
{
	u64  data64;
	u32  port;
	bool pcs_is_ok;

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_CHECK);

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "is ok (port = %u)", port);

	sl_core_read64(core_link, SS2_PORT_PML_STS_RX_PCS_SUBPORT(core_link->num), &data64);

	pcs_is_ok = ((SS2_PORT_PML_STS_RX_PCS_SUBPORT_ALIGN_STATUS_GET(data64) != 0) &&
		(SS2_PORT_PML_STS_RX_PCS_SUBPORT_FAULT_GET(data64) == 0)             &&
		(SS2_PORT_PML_STS_RX_PCS_SUBPORT_LOCAL_FAULT_GET(data64) == 0)       &&
		(SS2_PORT_PML_STS_RX_PCS_SUBPORT_HI_SER_GET(data64) == 0));

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_PCS_CHECK);

	if (pcs_is_ok)
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_OK);
	else
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_PCS_OK);

	return pcs_is_ok;
}
