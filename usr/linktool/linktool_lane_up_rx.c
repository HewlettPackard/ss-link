// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_sbus.h"
#include "linktool_pmi.h"
#include "linktool_uc_ram.h"
#include "linktool_cmds.h"
#include "linktool_lane_up_misc.h"
#include "linktool_lane_up_rx.h"

#define LINKTOOL_RX_MODE_REG      69

// FIXME: scrub out broadcom annotations

static unsigned int linktool_lane_up_rx_debug = 0;

void linktool_lane_up_rx_debug_set()
{
	linktool_lane_up_rx_debug = 1;
}

#define LANE_UP_RX_POWER_LOG "lane_up_rx_power -"
static int linktool_lane_up_rx_power(unsigned int dev, unsigned char lane)
{
	int           rtn;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx_power");

	/* reset */
        // EFUN(bcm_serdes_rx_dp_reset(sa__, 0)); => EFUN(wr_rx_ln_dp_s_rstb(0x1));
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "rx dp reset", dev, lane, 0, 0xD1C1, 1, 0x0001, 0);
	// EFUN(blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 6, 1, 0)); => blackhawk_sbus_wr_field(sa__, reg + 8 * physical_rx_lane, start, mask, value);
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_POWER_LOG "rx ctl", dev, (LINKTOOL_RX_CTL_REG + (8 * lane)), 1, 0x1, 0x0);

	/* clocks disable */
	// EFUN(wr_pmd_rx_clk_vld_frc_val(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "rx clk vld frc val",  dev, lane, 0, 0xD1A7, 0, 0x0010, 4);
	// EFUN(wr_pmd_rx_clk_vld_frc(0x0));     => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "rx clk vld frc",      dev, lane, 0, 0xD1A7, 0, 0x0008, 3);

	// EFUN(wr_pmd_tx_clk_vld_frc_val(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "tx clk vld frc val",  dev, lane, 0, 0xD1B7, 0, 0x0010, 4);
	// EFUN(wr_pmd_tx_clk_vld_frc(0x0));     => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "tx clk vld frc",      dev, lane, 0, 0xD1B7, 0, 0x0008, 3);

	// EFUN(wr_ln_rx_s_clkgate_frc_on(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "rx s clkgate frc on", dev, lane, 0, 0xD1A7, 0, 0x0001, 0);
	// EFUN(wr_ln_tx_s_clkgate_frc_on(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "tx s clkgate frc on", dev, lane, 0, 0xD1B7, 0, 0x0001, 0);

	/* power on */
	// EFUN(wr_ln_rx_s_pwrdn(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "rx s pwrdn",          dev, lane, 0, 0xD1A1, 0, 0x0001, 0);
	// EFUN(wr_ln_tx_s_pwrdn(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_POWER_LOG "tx s pwrdn",          dev, lane, 0, 0xD1B1, 0, 0x0001, 0);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_RX_MODE_SET_LOG "lane_up_rx_mode_set -"
static int linktool_lane_up_rx_mode_set(unsigned int dev, unsigned char lane, struct linktool_link_config link_config)
{
	int          rtn;
	unsigned int rx_mode;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx_mode_set");

	DEBUG(linktool_lane_up_rx_debug, "%s BMC reads rev id (sbus 0xFE)", LANE_UP_RX_MODE_SET_LOG);

	rx_mode = link_config.osr;
	if (link_config.encoding > 0) // anything other than NRZ
		rx_mode |= (0x1 << 6);
	switch (link_config.width) {
	case 40:
		rx_mode |= (0x0 << 7);
		break;
	case 80:
		rx_mode |= (0x1 << 7);
		break;
	case 160:
		rx_mode |= (0x2 << 7);
		break;
	default:
		ERROR("%s invalid (width = %d)", LANE_UP_RX_MODE_SET_LOG, link_config.width);
		rtn = -1;
		goto out;
	}
	DEBUG(linktool_lane_up_rx_debug, "%s rx_mode = 0x%X", LANE_UP_RX_MODE_SET_LOG, rx_mode);

	// EFUN(blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_MODE_REG, 16, 0x1ff, rx_mode));
	// FIXME: very basic lane calc for now
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_MODE_SET_LOG "rx lane mode", dev, (LINKTOOL_RX_MODE_REG + (8 * lane)), 16, 0x1FF, rx_mode);
	// EFUN(blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_MODE_REG, 31, 0x1, 1));
	// FIXME: very basic lane calc for now
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_MODE_SET_LOG "rx lane mode", dev, (LINKTOOL_RX_MODE_REG + (8 * lane)), 31, 0x1, 1);

	rtn = 0;
out:
	return rtn;
}

// FIXME: bare min for firmware mode with AN
#define FORCE_NRZ_MODE_SHL 15
#define SCRAMBLING_DIS_SHL  8
#define DFE_ON_SHL          2

#define LANE_UP_RX_CONFIG_LOG "lane_up_rx_config -"
static int linktool_lane_up_rx_config(unsigned int dev, unsigned char lane, struct linktool_uc_info uc_info, struct linktool_link_config link_config)
{
	int            rtn;
	unsigned short reset_state;
	unsigned int   addr;
	unsigned short fw_cfg;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx_config");

	// rd_rx_lane_dp_reset_state = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd1c9,13,13,__ERR)
	LINKTOOL_PMI_FIELD_RD(LANE_UP_RX_CONFIG_LOG, dev, lane, 0, 0xD1C9, 13, 13, &reset_state);
	DEBUG(linktool_lane_up_rx_debug, "%s reset_state = %d", LANE_UP_RX_CONFIG_LOG, reset_state);

	// EFUN(bcm_serdes_set_use_rx_osr_mode_pins_only(sa__, 1));
	addr = uc_info.lane_static_var_ram_base +
		(lane * uc_info.lane_static_var_ram_size) +
		0x9D + (uc_info.grp_ram_size * (lane >> 1));
	DEBUG(linktool_lane_up_rx_debug, "%s pin control addr = 0x%X", LANE_UP_RX_CONFIG_LOG, addr);
	rtn = linktool_uc_ram_wr8(dev, lane, addr, 0xBF); // bit 7 is enable
	if (rtn) {
		ERROR("%s uc_ram_wr failed [%d]", LANE_UP_RX_CONFIG_LOG, rtn);
		goto out;
	}

	// EFUN(blackhawk_sa_set_tx_rx_width_pam_osr(sa__, 0, config->rx_width, tx_encoding, config->rx_encoding, tx_osr, rx_osr, ip_type));
	rtn = linktool_lane_up_rx_mode_set(dev, lane, link_config);
	if (rtn) {
		ERROR("%s tx_mode_set failed [%d]", LANE_UP_RX_CONFIG_LOG, rtn);
		goto out;
	}

	// rd_rx_lane_dp_reset_state = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd1c9,13,13,__ERR)
	LINKTOOL_PMI_FIELD_RD(LANE_UP_RX_CONFIG_LOG, dev, lane, 0, 0xD1C9, 13, 13, &reset_state);
	DEBUG(linktool_lane_up_rx_debug, "%s reset_state = %d", LANE_UP_RX_CONFIG_LOG, reset_state);

	/* fw cfg */
	// FIXME: haredcode for AN
               fw_cfg = ((1 << FORCE_NRZ_MODE_SHL) | (1 << SCRAMBLING_DIS_SHL) | (1 << DFE_ON_SHL));
	DEBUG(linktool_lane_up_rx_debug, "%s fw_cfg = 0x%X", LANE_UP_RX_CONFIG_LOG, fw_cfg);
	// reg_wr_RX_CKRST_CTRL_LANE_UC_CONFIG(wr_val) osprey7_v2l4p1_acc_wr_reg(sa__, 0xd1ad,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "ra wraddr msw", dev, lane, 0, 0xD1AD, fw_cfg, 0xFFFF, 0);

	/* invert */
	DEBUG(linktool_lane_up_rx_debug, "%s invert is disabled", LANE_UP_RX_CONFIG_LOG);
	// EFUN(wr_bcm_serdes_rx_pmd_dp_invert(sa__, config->rx_invert)); = bcm_serdes_acc_mwr_reg_u8(sa__,0xd163,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "rx pmd dp invert", dev, lane, 0, 0xD163, 0, 0x0001, 0);

	/* link training */
	DEBUG(linktool_lane_up_rx_debug, "%s training is disabled", LANE_UP_RX_CONFIG_LOG);
	// EFUN(wr_bcm_serdes_linktrn_ieee_training_enable(sa__, 0)); = (bcm_serdes_acc_mwr_reg_u8(sa__,0x0096,0x0002,1,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "ieee training enable", dev, lane, 0, 0x0096, 0, 0x0002, 1);

	/* cdr */
	DEBUG(linktool_lane_up_rx_debug, "%s cdr (do nothing for now)", LANE_UP_RX_CONFIG_LOG);

	/* digital loopback */
	// EFUN(wr_prbs_chk_en_auto_mode(1)); = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd161,0x0080,7,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "en auto mode", dev, lane, 0, 0xD161, 1, 0x0080, 7);
	// EFUN(wr_dig_lpbk_en(0)); = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd162,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "dig lpbk en",  dev, lane, 0, 0xD162, 0, 0x0001, 0);

	/* compare data */
	// EFUN(blackhawk_sa_set_rx_cmp_data(sa__, PRBS_31, 0)); = EFUN(bcm_serdes_rx_prbs_en(sa__, prbs_enable));
	// EFUN(wr_prbs_chk_en(0x0)); = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd161,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "rx cmp data", dev, lane, 0, 0xD161, 0, 0x0001, 0);

	/* digital loopback */
	// FUN(wr_prbs_chk_en_auto_mode(1)); = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd161,0x0080,7,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "en auto mode", dev, lane, 0, 0xD161, 1, 0x0080, 7);
	// EFUN(wr_dig_lpbk_en(0)); = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd162,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_RX_CONFIG_LOG "dig lpbk en",  dev, lane, 0, 0xD162, 0, 0x0001, 0);

	// blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 17, 0x3, 0x1);
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_CONFIG_LOG "rx lane mode", dev, (LINKTOOL_RX_CTL_REG + (8 * lane)), 17, 0x3, 1);

	/* hoffset disable */
	DEBUG(linktool_lane_up_rx_debug, "%s hoffset (do nothing for now)", LANE_UP_RX_CONFIG_LOG);

	rtn = 0;
out:
	return rtn;
}

#define LINKTOOL_RX_RESET_COUNT 10
#define LANE_UP_RX_START_LOG  "lane_up_rx_start -"
static int linktool_lane_up_rx_start(unsigned int dev, unsigned char lane)
{
	int            rtn;
	unsigned int   x;
	unsigned short status;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx_start");

	// EFUN(bcm_serdes_tx_dp_reset(sa__, 0)); => EFUN(wr_tx_ln_dp_s_rstb(0x1));
	LINKTOOL_PMI_WR(LANE_UP_RX_START_LOG "tx dp reset", dev, lane, 0, 0xD1D1, 1, 0x0001, 0);
	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 6, 1, 1)); => blackhawk_sbus_wr_field(sa__, reg + 8 * physical_tx_lane, start, mask, value);
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_START_LOG "tx ctl", dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 6, 0x1, 0x1);
        // EFUN(bcm_serdes_rx_dp_reset(sa__, 0)); => EFUN(wr_rx_ln_dp_s_rstb(0x1));
	LINKTOOL_PMI_WR(LANE_UP_RX_START_LOG "rx dp reset", dev, lane, 0, 0xD1C1, 1, 0x0001, 0);
	// EFUN(blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 6, 1, 1)); => blackhawk_sbus_wr_field(sa__, reg + 8 * physical_rx_lane, start, mask, value);
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_RX_START_LOG "rx ctl", dev, (LINKTOOL_RX_CTL_REG + (8 * lane)), 6, 0x1, 0x1);

	/* check reset state */
	DEBUG(linktool_lane_up_rx_debug, "%s check reset state", LANE_UP_RX_START_LOG);
	for (x = 0; x < LINKTOOL_RX_RESET_COUNT; ++x) {
		// rd_bcm_serdes_tx_lane_dp_reset_state(sa__) (bcm_serdes_acc_rde_field_u8(sa__,0xd1d9,13,13,__ERR))
		LINKTOOL_PMI_FIELD_RD(LANE_UP_RX_START_LOG, dev, lane, 0, 0xD1D9, 13, 13, &status);
		if (status != 1)
			break;
		usleep(500);
	}
	if (x >= LINKTOOL_RX_RESET_COUNT) {
		ERROR("%s status = %d", LANE_UP_RX_START_LOG, status);
		rtn = -1;
		goto out;
	}

	rtn =  0;
out:
	return rtn;
}

#define LANE_UP_TX_CLOCK_ALIGN_LOG "lane_up_tx_clock_align -"
static int linktool_lane_up_tx_clock_align(unsigned int dev, unsigned char lane)
{
	int            rtn;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_tx_clock_align");

	DEBUG(linktool_lane_up_rx_debug, "%s BCM code calls to get the num_lanes (PMI 0xD10A)", LANE_UP_TX_CLOCK_ALIGN_LOG);

	// EFUN(wr_ams_tx_sel_txmaster(0));         /* Disable any master lanes */               => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0d3,0x0080,7,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx sel txmaster",  dev, lane, 0, 0xD0D3, 0, 0x0080, 7);
	// EFUN(wr_tx_pi_pd_bypass_vco(0));         /* for quick phase lock time */              => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a5,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi pd bypass vco",  dev, lane, 0, 0xD0A5, 0, 0x0800, 11);
	// EFUN(wr_tx_pi_pd_bypass_flt(0));         /* for quick phase lock time */              => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a5,0x0400,10,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi pd bypass flt",  dev, lane, 0, 0xD0A5, 0, 0x0400, 10);
	// EFUN(wr_tx_pi_hs_fifo_phserr_sel(0));    /* selects PD from afe as input */           => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a5,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi hs fifo phserr sel",  dev, lane, 0, 0xD0A5, 0, 0x0010, 4);
	// EFUN(wr_tx_pi_ext_pd_sel(0));            /* selects PD path based on tx_pi_repeater_mode_en */   => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a5,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi ext pd sel",  dev, lane, 0, 0xD0A5, 0, 0x0008, 3);
	// EFUN(wr_tx_pi_en(0));                    /* Enable TX_PI    */         => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a0,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi en",  dev, lane, 0, 0xD0A0, 0, 0x0001, 0);
	/* turn off frequency lock for tx_pi as all te tclk clocks are derived from VCO and same frequency */
	// EFUN(wr_tx_pi_jitter_filter_en(0));      => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a0,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi jitter filter en", dev, lane, 0, 0xD0A0, 0, 0x0002, 1);
	// EFUN(wr_tx_pi_ext_ctrl_en(0));           /* turn on PD path to TX_PI to lock the clocks */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a0,0x0004,2,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi ext ctrl en", dev, lane, 0, 0xD0A0, 0, 0x0004, 2);
	// EFUN(wr_tx_pi_ext_phase_bwsel_integ(0)); /* 0 to 7: higher value means faster lock time */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a0,0x7000,12,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi ext phase bwsel integ", dev, lane, 0, 0xD0A0, 0, 0x7000, 12);
	// EFUN(wr_tx_pi_pd_bypass_vco(0));         /* disable filter bypass for more accurate lock */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0a5,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx pi pd bypass vco", dev, lane, 0, 0xD0A5, 0, 0x0800, 11);

	rtn = 0;
out:
	return rtn;
}

#define SIGNAL_DETECT_CHECK_COUNT      60
#define LANE_UP_RX_SIGNAL_DETECT_LOG "lane_up_rx_signal_detect -"
static int linktool_lane_up_rx_signal_check(unsigned int dev, unsigned char lane)
{
	int            rtn;
	unsigned int   x;
	unsigned short status;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx_signal_detect");

	for (x = 0; x < SIGNAL_DETECT_CHECK_COUNT; ++x) {
		// reg_rd_SIGDET_SDSTATUS_0()  osprey7_v2l4p1_acc_rde_reg(sa__, 0xd0e8,__ERR)
		LINKTOOL_PMI_RD(LANE_UP_RX_SIGNAL_DETECT_LOG "signal detect", dev, lane, 0, 0xD0E8, &status);
		// NOTE: sd = bit 0, sd_change = bit 1 - if sd_change == 1 then sd = 0 else sd = bit 0
		if (((status & 0x0002) == 0) && ((status & 0x0001) == 1))
			break;
		usleep(500);
	}
	if (x >= SIGNAL_DETECT_CHECK_COUNT) {
		ERROR("%s no signal (lane = %d)", LANE_UP_RX_SIGNAL_DETECT_LOG, lane);
		rtn = -1;
		goto out;
	}

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_RX_LOG "lane_up_rx -"
int linktool_lane_up_rx(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	int           lane_map = cmd_obj->link_config.lane_map;
	int           lane_num;
	unsigned int  is_rx_ready;
	unsigned int  is_tx_ready;

	DEBUG(linktool_lane_up_rx_debug, "lane_up_rx");

	DEBUG(linktool_lane_up_rx_debug, "%s BCM code calls to get the rev id (sbus 0xFE)", LANE_UP_RX_LOG);
	DEBUG(linktool_lane_up_rx_debug, "%s BCM code calls to get the num_lanes (PMI 0xD10A)", LANE_UP_RX_LOG);

	for (lane_num = 0; lane_num < LINKTOOL_NUM_LANES; ++lane_num) {

		if (((lane_map >> lane_num) & 1) == 0)
			continue;

		DEBUG(linktool_lane_up_rx_debug, "%s power (lane_num = %d)", LANE_UP_RX_LOG, lane_num);
		rtn = linktool_lane_up_rx_power(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s rx_power failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_rx_debug, "%s BCM calls blackhawk_sa_get_rx_input_loopback (PMI 0xD1AE)", LANE_UP_RX_LOG);
		rtn = linktool_lane_up_is_ready(cmd_obj->dev_addr, lane_num, &is_rx_ready, &is_tx_ready);
		if (rtn) {
			ERROR("%s is_ready failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_rx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_osr (PMI 0xD1CB, 0xD1DB)", LANE_UP_RX_LOG);
		DEBUG(linktool_lane_up_rx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_width (sbus 0x45, 0x42)", LANE_UP_RX_LOG);
		DEBUG(linktool_lane_up_rx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_line_encoding (sbus 0x42, 0x45)", LANE_UP_RX_LOG);

		DEBUG(linktool_lane_up_rx_debug, "%s config (lane_num = %d)", LANE_UP_RX_LOG, lane_num);

		rtn = linktool_lane_up_rx_config(cmd_obj->dev_addr, lane_num, cmd_obj->uc_info, cmd_obj->link_config);
		if (rtn) {
			ERROR("%s rx_config failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_rx_debug, "%s start (lane_num = %d)", LANE_UP_RX_LOG, lane_num);
		rtn = linktool_lane_up_rx_start(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s rx_start failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_rx_debug, "%s signal check (lane_num = %d)", LANE_UP_RX_LOG, lane_num);
		rtn = linktool_lane_up_rx_signal_check(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s rx_signal_check failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}
	}

	for (lane_num = 0; lane_num < LINKTOOL_NUM_LANES; ++lane_num) {

		if (((lane_map >> lane_num) & 1) == 0)
			continue;

		DEBUG(linktool_lane_up_rx_debug, "%s clock align (lane_num = %d)", LANE_UP_RX_LOG, lane_num);
		rtn = linktool_lane_up_tx_clock_align(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s tx_clock_align failed [%d]", LANE_UP_RX_LOG, rtn);
			goto out;
		}
	}

        /* Disable any master lanes and move on to next TCA group */
	DEBUG(linktool_lane_up_rx_debug, "%s BCM calls to disables TX master here", LANE_UP_RX_LOG);
        // EFUN(wr_ams_tx_sel_txmaster(0));  => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0d3,0x0080,7,wr_val)
	// FIXME: blindly setting back to lane 0 for now
        //LINKTOOL_PMI_WR(LANE_UP_TX_CLOCK_ALIGN_LOG "tx sel txmaster", cmd_obj->dev_addr, 0, 0, 0xD0D3, 0, 0x0080, 7);

	DEBUG(linktool_lane_up_rx_debug, "%s BCM code calls tca_enable if txppm_offset_flag is set", LANE_UP_RX_LOG);

	rtn = 0;
out:
	return rtn;
}
