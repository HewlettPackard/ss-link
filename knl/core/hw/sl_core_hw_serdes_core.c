// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_pmi.h"
#include "hw/sl_core_hw_sbus.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_serdes_fw.h"
#include "hw/sl_core_hw_serdes_lane.h"
#include "hw/sl_core_hw_serdes_core.h"
#include "hw/sl_core_hw_serdes_addrs.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

#define SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES 10
static int sl_core_hw_serdes_core_proc_reset(struct sl_core_lgrp *core_lgrp)
{
	int  rtn;
	u16  data16;
	u8   x;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "proc reset");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_D_COM_RMI_MICRO_CONTROL0], 0x0000, 0x0001); /* set micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_D_COM_RMI_MICRO_CONTROL0], 0x0001, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_B_COM_RMI_RAM_CR_CRCCONTROL0], 0x0000, 0x0001); /* disable CRC checking */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_C_COM_RAM_CONTROL2], (SL_HW_SERDES_FW_STACK_SIZE << 2), 0x7FFC); /* micro_core_stack_size */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_C_COM_RAM_CONTROL2], 0x8000, 0x8000); /* stack enable */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_COMMON_CLOCK], 0x0040, 0x0040); /* select common clock */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MDIO_MMDSEL_AER_COM_MDIO_BRCST_PORT_ADDR], 0x001F, 0x001F); /* broadcast port addr */

	for (x = 0; x < SL_SERDES_NUM_LANES; ++x)
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, x, 0,
			addrs[SERDES_RXTXCOM_CKRST_CTRL_RXTXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL], 0x0000, 0x0001); /* disable lane */

	usleep_range(10, 20);

	for (x = 0; x < SL_SERDES_NUM_PLLS; ++x)
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, x,
			addrs[SERDES_CORE_PLL_COM_TOP_USER_CONTROL], 0x0000, 0x2000); /* set PLL reset */

	usleep_range(10, 20);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0,
		addrs[SERDES_DIG_COM_RESET_CONTROL_PMD], 0x0000, 0x0001); /* set core reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0,
		addrs[SERDES_DIG_COM_RESET_CONTROL_PMD], 0x0001, 0x0001); /* clear core reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_CORE_PLL_COM_RESET_CONTROL_PLL_DP], 0x0002, 0x0002); /* clear pll0 reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 1,
		addrs[SERDES_CORE_PLL_COM_RESET_CONTROL_PLL_DP], 0x0002, 0x0002); /* clear pll1 reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_D_COM_RMI_MICRO_CONTROL0], 0x0000, 0x0001); /* set micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_D_COM_RMI_MICRO_CONTROL0], 0x0001, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_B_COM_RMI_RAM_CR_CRCCONTROL0], 0x0000, 0x0001); /* disable CRC checking */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_C_COM_RAM_CONTROL2], (SL_HW_SERDES_FW_STACK_SIZE << 2), 0x7FFC); /* micro_core_stack_size */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_C_COM_RAM_CONTROL2], 0x8000, 0x8000); /* stack enable */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_D_COM_RMI_MICRO_CONTROL0], 0x0001, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_CLOCK_CONTROL0], 0x0001, 0x0001); /* enable micro subsystem clock */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_A_COM_RESET_CONTROL0], 0x0001, 0x0001); /* clear micro subsystem reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_MICRO_C_COM_RAM_CONTROL0], 0x0001, 0x0001); /* enable micro access to code RAM */

	for (x = 0; x < SL_SERDES_NUM_MICROS; ++x) {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x,
			addrs[SERDES_MICRO_E_COM_MICRO_CORE_CLOCK_CONTROL0], 0x0001, 0x0001); /* enable micro clock */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x,
			addrs[SERDES_MICRO_E_COM_MICRO_CORE_RESET_CONTROL0], 0x0001, 0x0001); /* clear micro reset */
	}

	for (x = 0; x < SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_DIG_COM_RESET_CONTROL_PMD], &data16); /* all micros init */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "micro init D101 = 0x%X", data16);
		if ((data16 & 0x0002) == 0)
			continue;
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_MICRO_B_COM_RMI_MICRO_SDK_STATUS0], &data16); /* micro status */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "micro status D21A = 0x%X", data16);
		if ((data16 & 0x000F) != SL_SERDES_ACTIVE_MICROS)
			continue;
		break;
	}
	if (x >= SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "uc active failed");
		rtn = -EIO;
		goto out;
	}
	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc active");

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_clock_init(struct sl_core_lgrp *core_lgrp)
{
	int  rtn;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "clock init");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_2], 0x0000, 0x0060); /* pll_rtl_div_en */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_4], 0x0000, 0x0400); /* pll_i_en_ext_cml_refclk_in */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_4], 0x0000, 0x0200); /* pll_i_en_ext_cml_refclk_out */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_4], 0x0000, 0x0100); /* pll_i_en_ext_cmos_refclk_in */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_CORE_PLL_COM_TOP_USER_CONTROL], 0x0000, 0x2000); /* set PLL reset */

	/* defaults set */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_10], 0x003B, 0x03FF); /* integer portion */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_11], 0x0000, 0xFFFF); /* fractional portion low 16 bits */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0000, 0x0003); /* fractional portion top 2 bits */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_1], 0x0000, 0x0080); /* pll_refclk_freq2x_en */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_1], 0x0000, 0x0020); /* pll_refdiv2 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_1], 0x0000, 0x0040); /* pll_refdiv4 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_POWER_DOWN], 0x0000, 0x0002); /* pll_div4_2_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x1000, 0x3000); /* pll_pll_frac_mode */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0800, 0x0800); /* pll_resetb_mmd */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_0], 0x0000, 0x0008); /* pll_en_lb_res */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_0], 0x0000, 0x0010); /* pll_en_lb_ind */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x0400); /* pll_lb_sel_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x0040, 0x0070); /* pll_rz_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x3800, 0x7800); /* pll_cp_calib_adj */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x03C0); /* pll_cap_l_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_3], 0x0000, 0x000F); /* pll_cap_r_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_3], 0x0002, 0x0007); /* pll_ictrl_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0000, 0x0010); /* pll_fracn_enable */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_SELECT], 0x0030, 0x003C); /* pll_i_en_ovrrad */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x0000, 0x0180); /* pll_cp_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_0], 0x1800, 0x1800); /* pll_fp3_r_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x0000, 0x000F); /* pll_fp3_c_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x0020); /* pll_en_offset_fbck */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x0010); /* pll_en_offset_refclk */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x000C); /* pll_fracn_offset_ctrl */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x0000, 0x0003); /* pll_pfd_pulse_adj */

	/* config set */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_POWER_DOWN], 0x0000, 0x0004); /* power up PLL */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_CONTROL], 0x0010, 0x0010); /* power up high band */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_CONTROL], 0x0008, 0x0008); /* power up low band */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_0], 0x0008, 0x0008); /* pll_en_lb_res */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_0], 0x0010, 0x0010); /* pll_i_en_kvh */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_CONTROL], 0x0000, 0x0008); /* power down low band */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x3000, 0x7800); /* pll_cp_calib_adj */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_2], 0x00C0, 0x03C0); /* pll_cap_l_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_PLL_CONTROL_3], 0x0003, 0x000F); /* pll_cap_r_pll0 */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_3], 0x0002, 0x0007); /* pll_i_clkchnl */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_COM_B_PLL_CTRL_1], 0x0010, 0x0070); /* pll_rz_sel */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_FW_API_DATA0], 0x00C1, 0xFFFF); /* API data 0 */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_CORE_PLL_COM_TOP_USER_CONTROL], 0x2000, 0x2000); /* clear PLL reset */

	usleep_range(5000, 10000);

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_lgrp_reset(struct sl_core_lgrp *core_lgrp)
{
	int  rtn;
	u8   lane_num;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "reset");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	for (lane_num = 0; lane_num < SL_SERDES_NUM_LANES; ++lane_num) {
		/* TX */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_TXCOM_CKRST_CTRL_TXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
			0x0000, 0x0001); /* tx_ln_dp_s_rstb */
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_TXCOM_CKRST_CTRL_TXCOM_LN_S_RSTB_CONTROL], 0x0000, 0x0001); /* tx_ln_s_rstb */
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_TXCOM_CKRST_CTRL_TXCOM_LN_S_RSTB_CONTROL], 0x0001, 0x0001); /* tx_ln_s_rstb */
		/* RX */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_RXCOM_LANE_CLK_RESET_N_POWERDOWN_CONTROL],
			0x0000, 0x0001); /* rx_ln_dp_s_rstb */
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_RXCOM_CKRST_CTRL_RXCOM_LN_S_RSTB_CONTROL], 0x0000, 0x0001); /* rx_ln_s_rstb */
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0,
			addrs[SERDES_RXCOM_CKRST_CTRL_RXCOM_LN_S_RSTB_CONTROL], 0x0001, 0x0001); /* rx_ln_s_rstb */
	}

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_core_init(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "core init");

	rtn = sl_core_hw_serdes_core_proc_reset(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_proc_reset failed [%d]", rtn);
		goto out;
	}
	rtn = sl_core_hw_serdes_core_clock_init(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_clock_init failed [%d]", rtn);
		goto out;
	}
	rtn = sl_core_hw_serdes_core_lgrp_reset(core_lgrp);
	if (rtn != 0) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_lgrp_reset failed [%d]", rtn);
		goto out;
	}

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_pll_set(struct sl_core_lgrp *core_lgrp, u32 clocking)
{
	int  rtn;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll set");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0000, 0x0800); /* set PLL reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0800, 0x0800); /* clear PLL reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0000, 0x0008); /* update PLL ratio */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x1000, 0x3000); /* MMD 8/9 mode */

	if (clocking == SL_CORE_HW_SERDES_CLOCKING_82P5) {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_10], 0x0052, 0x03FF); /* integer portion */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_11], 0x0000, 0xFFFF); /* fractional portion low 16 bits */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0002, 0x0003); /* fractional portion top 2 bits */
	} else {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_10], 0x0055, 0x03FF); /* integer portion */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_11], 0x0000, 0xFFFF); /* fractional portion low 16 bits */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
			addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0000, 0x0003); /* fractional portion top 2 bits */
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0008, 0x0008); /* update PLL ratio */
	usleep_range(10, 20);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_AMS_PLL_COM_PLL_CONTROL_12], 0x0000, 0x0008); /* update PLL ratio */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0000, 0x0800); /* set PLL reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_RESET2], 0x0800, 0x0800); /* clear PLL reset */

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_pll_check(struct sl_core_lgrp *core_lgrp)
{
	int  rtn;
	u16  data16;
	u16 *addrs;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll check");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_PLL_CAL_COM_STS_0], &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "0x%X = 0x%X", addrs[SERDES_PLL_CAL_COM_STS_0], data16);
	if ((data16 & 0x0200) == 0) { /* PLL lock status */
		sl_core_log_err(core_lgrp, LOG_NAME, "pll lock failed");
		rtn = -EIO;
		goto out;
	}
	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll locked");

	rtn = 0;
out:
	return rtn;
}

int sl_core_hw_serdes_core_pll(struct sl_core_lgrp *core_lgrp, u32 clocking)
{
	int  rtn;
	u16 *addrs;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	if (core_lgrp->serdes.clocking == clocking) {
		sl_core_log_dbg(core_lgrp, LOG_NAME, "pll clocking already set");
		return 0;
	}

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll");

	addrs = core_lgrp->core_ldev->serdes.addrs;

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_CORE_PLL_COM_TOP_USER_CONTROL], 0x0000, 0x2000); /* set PLL reset */

	rtn = sl_core_hw_serdes_core_pll_set(core_lgrp, clocking);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_pll_set failed [%d]", rtn);
		goto out;
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0,
		addrs[SERDES_CORE_PLL_COM_TOP_USER_CONTROL], 0x2000, 0x2000); /* clear PLL reset */

	usleep_range(5000, 10000);

	rtn = sl_core_hw_serdes_core_pll_check(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_pll_check failed [%d]", rtn);
		goto out;
	}

	core_lgrp->serdes.clocking = clocking;

	rtn = 0;
out:
	return rtn;
}
