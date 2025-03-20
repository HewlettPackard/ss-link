// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_serdes_addrs.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

int sl_core_hw_serdes_lane_up_rx_setup(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	u64                  data64;
	u32                  port;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up rx setup (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* clock gate */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_CKRST_CTRL_RX_CLOCK_N_RESET_DEBUG_CONTROL],
		0x0000, 0x0010); /* rx_clk_vld_frc_val */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_CKRST_CTRL_RX_CLOCK_N_RESET_DEBUG_CONTROL],
		0x0000, 0x0008); /* rx_clk_vld_frc */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_CKRST_CTRL_RX_CLOCK_N_RESET_DEBUG_CONTROL],
		0x0000, 0x0001); /* rx_s_clkgate_frc_on */

	/* power on */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_CKRST_CTRL_RX_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
		0x0000, 0x0001); /* rx_s_pwrdn */

	usleep_range(10000, 20000); /* avoid power surge */

	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_H_PWRDN_UPDATE(data64, 0); /* active high */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	/* disable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
		0x0000, 0x0001); /* RX_LN_DP_EN */

	/* assert DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_DP_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	/* toggle reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));
	usleep_range(10, 20);
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_H_RSTB_UPDATE(data64, 1); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_up_rx_config(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	u64                  data64;
	u32                  port;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up rx config (port = %u, serdes_lane_num = %u, asic_lane_num = %u, hpe_map = 0x%X)",
		port, serdes_lane_num, asic_lane_num,
		core_link->core_lgrp->link_caps[core_link->num].hpe_map);

	/* OSR */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_RX_OSR_MODE_UPDATE(data64,
		sl_core_hw_serdes_mode(core_lgrp, &(core_link->serdes.core_serdes_settings)));
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	/* fw cfg */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_FW_API_DATA0],
		sl_core_hw_serdes_config(core_lgrp,
			&(core_link->serdes.core_serdes_settings), &(core_link->serdes.media_serdes_settings)),
		0xFFFF); /* LANE_CFG_FWAPI_DATA0 */

	/* invert */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_MISC_CONFIG],
		core_lgrp->serdes.dt.lane_info[asic_lane_num].rx_invert, 0x0001); /* RX_PMD_DP_INVERT */

	/* link training */
	if (is_flag_set(core_link->core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN)) {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up rx config link train on (logical = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_LINKTRN_IEEE_TX_LINKTRNIT_BASE_R_PMD_CONTROL],
			0x0002, 0x0002); /* linktrn_ieee_training_enable */
	} else {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up rx config link train off (logical = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_LINKTRN_IEEE_TX_LINKTRNIT_BASE_R_PMD_CONTROL],
			0x0000, 0x0002); /* linktrn_ieee_training_enable */
	}

	/* digital loopback */
	if (is_flag_set(core_link->core_lgrp->config.options, SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up rx config loopback on (logical = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_TLB_RX_DIG_LPBK_CONFIG],
			0x0001, 0x0001); /* DIG_LPBK_EN */
	} else {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up rx config loopback off (logical = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
			core_lgrp->core_ldev->serdes.addrs[SERDES_TLB_RX_DIG_LPBK_CONFIG],
			0x0000, 0x0001); /* DIG_LPBK_EN */
	}

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_up_rx_start(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	u64                  data64;
	u32                  port;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up rx start (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* enable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
		0x0001, 0x0001); /* RX_LN_DP_EN */

	/* release DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_DP_H_RSTB_UPDATE(data64, 1); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	rtn = 0;
out:
	return rtn;
}

void sl_core_hw_serdes_lane_down_rx_stop(struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int                  rtn;
	u64                  data64;
	u32                  port;
	u8                   asic_lane_num;
	struct sl_core_lgrp *core_lgrp;

	core_lgrp     = core_link->core_lgrp;
	port          = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_rx_asic_lane_num_get(core_link, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane down rx stop (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* disable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
		0x0000, 0x0001); /* RX_LN_DP_EN */

	/* assert DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_RX_PMD_LN_RX_DP_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_RX(asic_lane_num));

	/* power down */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_RX_CKRST_CTRL_RX_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
		0x0001, 0x0001); /* rx_s_pwrdn */

	usleep_range(10000, 20000); /* avoid power surge */

	/* clear the config */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num, 0,
		core_lgrp->core_ldev->serdes.addrs[SERDES_FW_API_DATA0],
		0x0000, 0xFFFF); /* LANE_CFG_FWAPI_DATA0 */

out:
	return;
}
