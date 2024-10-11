// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_uc_ram.h"
#include "data/sl_core_data_lgrp.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

int sl_core_hw_serdes_lane_up_tx_setup(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	int rtn;
	u64 data64;
	u32 port;
	u8  asic_lane_num;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx setup (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* clock gate */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1B7, 0, 4, 0x0010); /* tx_clk_vld_frc_val */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1B7, 0, 3, 0x0008); /* tx_clk_vld_frc */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1B7, 0, 0, 0x0001); /* tx_s_clkgate_frc_on */

	/* power on */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1B1, 0, 0, 0x0001); /* tx_s_pwrdn */

	usleep_range(10000, 20000); /* avoid power surge */

	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_H_PWRDN_UPDATE(data64, 0); /* active high */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_DISABLE_UPDATE(data64, 0); /* match sbus effect */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	/* disable TX */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD131, 1, 0, 0x0001); /* sdk_tx_disable */

	/* disable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1D1, 0, 0, 0x0001); /* TX_LN_DP_EN */

	/* assert DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_DP_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	/* toggle reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));
	usleep_range(10, 20);
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_H_RSTB_UPDATE(data64, 1); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_up_tx_config(struct sl_core_lgrp *core_lgrp,
	struct sl_core_link *core_link, u8 serdes_lane_num)
{
	int rtn;
	u64 data64;
	u32 port;
	u8  asic_lane_num;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx config (port = %u, serdes_lane_num = %u, asic_lane_num = %u, hpe_map = 0x%X)",
		port, serdes_lane_num, asic_lane_num,
		core_link->core_lgrp->link_caps[core_link->num].hpe_map);

	/* OSR */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_TX_OSR_MODE_UPDATE(data64,
			sl_core_hw_serdes_mode(core_lgrp, &(core_link->serdes.core_serdes_settings)));
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx config (pre3 = %d, pre2 = %d, pre1 = %d, cursor = %d, post1 = %d, post2 = %d)",
		core_link->serdes.media_serdes_settings.pre3, core_link->serdes.media_serdes_settings.pre2,
		core_link->serdes.media_serdes_settings.pre1, core_link->serdes.media_serdes_settings.cursor,
		core_link->serdes.media_serdes_settings.post1, core_link->serdes.media_serdes_settings.post2);

	/* precoder enable */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD175, 0, 1, 0x0002); /* PAM4_PRECODER_EN */

	/* pre3 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD133, core_link->serdes.media_serdes_settings.pre3, 0, 0x01FF); /* TXFIR_TAP0_COEFF */
	/* pre2 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD134, core_link->serdes.media_serdes_settings.pre2, 0, 0x01FF); /* TXFIR_TAP1_COEFF */
	/* pre1 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD135, core_link->serdes.media_serdes_settings.pre1, 0, 0x01FF); /* TXFIR_TAP2_COEFF */
	/* cursor */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD136, core_link->serdes.media_serdes_settings.cursor, 0, 0x01FF); /* TXFIR_TAP3_COEFF */
	/* post 1 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD137, core_link->serdes.media_serdes_settings.post1, 0, 0x01FF); /* TXFIR_TAP4_COEFF */
	/* post 2 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD138, core_link->serdes.media_serdes_settings.post2, 0, 0x01FF); /* TXFIR_TAP5_COEFF */
	/* tap enable */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD133, 1, 12, 0x7000); /* TXFIR_TAP_EN */
	/* load coefficients */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD133, 1, 11, 0x0800); /* TXFIR_TAP_LOAD */

	/* invert */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD173, core_lgrp->serdes.dt.lane_info[asic_lane_num].tx_invert,
		0, 0x0001); /* TX_PMD_DP_INVERT */

	/* link training */
	if (is_flag_set(core_link->core_lgrp->link_caps[core_link->num].hpe_map, SL_LINK_CONFIG_HPE_LINKTRAIN)) {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up tx config link train on (serdes = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
			0, 0x0096, 1, 1, 0x0002); /* LINKTRN_IEEE_TRAINING_ENABLE */
	} else {
		sl_core_log_dbg(core_lgrp, LOG_NAME,
			"lane up tx config link train off (serdes = %u)", serdes_lane_num);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
			0, 0x0096, 0, 1, 0x0002); /* LINKTRN_IEEE_TRAINING_ENABLE */
	}

	/* disable remote loopback */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD172, 0, 0, 0x0001); /* RMT_LPBK_EN */

#if 0
	// FIXME: use when PRBS mode is set
	/* set prbs mode */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD171, 5, 11, 0xF800); /* PRBS_GEN_MODE_SEL_MSB ?? */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD171, 0, 4, 0x0010); /* PRBS_GEN_INV */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD171, 1, 0, 0x0001); /* PRBS_GEN_EN */
#endif

	/* set core mode */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD171, 0, 0, 0x0001); /* PRBS_GEN_EN */

	/* patern generator disable */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD170, 0, 0, 0x0001); /* PATT_GEN_EN */

	/* PRBS check enable */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD161, 0, 0, 0x0001); /* PRBS_CHK_EN */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD161, 1, 0, 0x0001); /* PRBS_CHK_EN */

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_lane_up_tx_start(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	int rtn;
	u64 data64;
	u32 port;
	u8  asic_lane_num;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane up tx start (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* enable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1D1, 1, 0, 0x0001); /* TX_LN_DP_EN */

	/* release DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_DP_H_RSTB_UPDATE(data64, 1); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	/* enable TX */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD131, 0, 0, 0x0001); /* TX_DISABLE */

	rtn = 0;
out:
	return rtn;
}

void sl_core_hw_serdes_lane_down_tx_stop(struct sl_core_lgrp *core_lgrp, u8 serdes_lane_num)
{
	int rtn;
	u64 data64;
	u32 port;
	u8  asic_lane_num;

	port = core_lgrp->num;
	asic_lane_num = sl_core_hw_serdes_tx_asic_lane_num_get(core_lgrp, serdes_lane_num);

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"lane down tx stop (port = %u, serdes_lane_num = %u, asic_lane_num = %u)",
		port, serdes_lane_num, asic_lane_num);

	/* disable TX */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD131, 1, 0, 0x0001); /* TX_DISABLE */

	/* disable DP */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1D1, 0, 0, 0x0001); /* TX_LN_DP_EN */

	/* assert DP reset */
	sl_core_lgrp_read64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), &data64);
	data64 = SS2_PORT_PML_CFG_SERDES_TX_PMD_LN_TX_DP_H_RSTB_UPDATE(data64, 0); /* active low */
	sl_core_lgrp_write64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num), data64);
	sl_core_lgrp_flush64(core_lgrp, SS2_PORT_PML_CFG_SERDES_TX(asic_lane_num));

	/* power down */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, serdes_lane_num,
		0, 0xD1B1, 1, 0, 0x0001); /* tx_s_pwrdn */

	usleep_range(10000, 20000);  /* avoid power surge */

out:
	return;
}
