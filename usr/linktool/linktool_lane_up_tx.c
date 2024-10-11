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
#include "linktool_lane_up_tx.h"

#define LINKTOOL_TX_MISC_REG      48
#define LINKTOOL_TX_MODE_REG      66

// FIXME: scrub out broadcom annotations

static unsigned int linktool_lane_up_tx_debug = 0;

void linktool_lane_up_tx_debug_set()
{
	linktool_lane_up_tx_debug = 1;
}

#define LANE_UP_TX_POWER_LOG "lane_up_tx_power -"
static int linktool_lane_up_tx_power(unsigned int dev, unsigned char lane)
{
	int           rtn;
	unsigned int  is_rx_ready;
	unsigned int  is_tx_ready;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_power");

	/* clocks disable */
	// EFUN(wr_pmd_rx_clk_vld_frc_val(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "rx clk vld frc val",  dev, lane, 0, 0xD1A7, 0, 0x0010, 4);
	// EFUN(wr_pmd_rx_clk_vld_frc(0x0));     => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "rx clk vld frc",      dev, lane, 0, 0xD1A7, 0, 0x0008, 3);

	// EFUN(wr_pmd_tx_clk_vld_frc_val(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "tx clk vld frc val",  dev, lane, 0, 0xD1B7, 0, 0x0010, 4);
	// EFUN(wr_pmd_tx_clk_vld_frc(0x0));     => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "tx clk vld frc",      dev, lane, 0, 0xD1B7, 0, 0x0008, 3);

	// EFUN(wr_ln_rx_s_clkgate_frc_on(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a7,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "rx s clkgate frc on", dev, lane, 0, 0xD1A7, 0, 0x0001, 0);
	// EFUN(wr_ln_tx_s_clkgate_frc_on(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b7,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "tx s clkgate frc on", dev, lane, 0, 0xD1B7, 0, 0x0001, 0);

	/* power on */
	// EFUN(wr_ln_rx_s_pwrdn(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "rx s pwrdn",          dev, lane, 0, 0xD1A1, 0, 0x0001, 0);
	// EFUN(wr_ln_tx_s_pwrdn(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "tx s pwrdn",          dev, lane, 0, 0xD1B1, 0, 0x0001, 0);

	rtn = linktool_lane_up_is_ready(dev, lane, &is_rx_ready, &is_tx_ready);
	if (rtn) {
		ERROR("%s is ready failed [%d]", LANE_UP_TX_POWER_LOG, rtn);
		goto out;
	}

	/* assert reset */
	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_power - toggle reset");
	// EFUN(bcm_serdes_tx_dp_reset(sa__, 0)); => EFUN(wr_tx_ln_dp_s_rstb(0x1));
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "tx dp reset", dev, lane, 0, 0xD1D1, 1, 0x0001, 0);
	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 6, 1, 0)); => blackhawk_sbus_wr_field(sa__, reg + 8 * physical_tx_lane, start, mask, value);
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_POWER_LOG "tx ctl", dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 6, 0x1, 0x0);
       	// EFUN(bcm_serdes_rx_dp_reset(sa__, 0)); => EFUN(wr_rx_ln_dp_s_rstb(0x1));
	LINKTOOL_PMI_WR(LANE_UP_TX_POWER_LOG "rx dp reset", dev, lane, 0, 0xD1C1, 1, 0x0001, 0);
	// EFUN(blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 6, 1, 0)); => blackhawk_sbus_wr_field(sa__, reg + 8 * physical_rx_lane, start, mask, value);
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_POWER_LOG "rx ctl", dev, (LINKTOOL_RX_CTL_REG + (8 * lane)), 6, 0x1, 0x0);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_TOGGLE_RESET_LOG "lane_up_tx_toggle_reset -"
static int linktool_lane_up_toggle_reset(unsigned int dev, unsigned char lane)
{
	int rtn;

	// EFUN(wr_tx_ln_dp_s_rstb(0x0));  => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1d1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ln dp s rstb", dev, lane, 0, 0xD1D1, 0, 0x0001, 0);
	usleep(1);
	// EFUN(wr_tx_ln_s_rstb(0x0)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1de,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ln s rstb", dev, lane, 0, 0xD1DE, 0, 0x0001, 0);
	usleep(1);
	// EFUN(wr_tx_ln_s_rstb(0x1)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1de,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ln s rstb", dev, lane, 0, 0xD1DE, 1, 0x0001, 0);

	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 5, 0x1, 0));
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ctl", dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 5, 0x1, 0x0);
	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 5, 0x1, 1));
	// FIXME: need use physical lane as part of this calc
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ctl", dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 5, 0x1, 0x1);
	// EFUN(wr_tx_ln_dp_s_rstb(0x1)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1d1,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_TOGGLE_RESET_LOG "tx ln dp s rstb", dev, lane, 0, 0xD1D1, 1, 0x0001, 0);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_MODE_SET_LOG "lane_up_tx_mode_set -"
static int linktool_lane_up_tx_mode_set(unsigned int dev, unsigned char lane, struct linktool_link_config link_config)
{
	int          rtn;
	unsigned int tx_mode;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_mode_set");

	tx_mode = link_config.osr;
	if (link_config.encoding > 0) // anything other than NRZ
		tx_mode |= (0x1 << 6);
	switch (link_config.width) {
	case 40:
		tx_mode |= (0x0 << 7);
		break;
	case 80:
		tx_mode |= (0x1 << 7);
		break;
	case 160:
		tx_mode |= (0x2 << 7);
		break;
	default:
		ERROR("%s invalid (width = %d)", LANE_UP_TX_MODE_SET_LOG, link_config.width);
		rtn = -1;
		goto out;
	}
	DEBUG(linktool_lane_up_tx_debug, "%s tx_mode = 0x%x", LANE_UP_TX_MODE_SET_LOG, tx_mode);

	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_MODE_REG, 16, 0x1ff, tx_mode));
	// FIXME: very basic lane calc for now
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_MODE_SET_LOG "tx lane mode", dev, (LINKTOOL_TX_MODE_REG + (8 * lane)), 16, 0x1FF, tx_mode);
	// EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_MODE_REG, 31, 0x1, 1));
	// FIXME: very basic lane calc for now
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_MODE_SET_LOG "tx lane mode", dev, (LINKTOOL_TX_MODE_REG + (8 * lane)), 31, 0x1, 1);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_EQ_LOG "lane_up_tx_eq -"
static int linktool_lane_up_tx_eq(unsigned int dev, unsigned char lane, struct linktool_link_config link_config)
{
	int rtn;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_eq");

	// EFUN(wr_txfir_nrz_tap_range_sel(((enable_taps == OSPREY7_V2L4P1_NRZ_LP_3TAP) || (enable_taps == OSPREY7_V2L4P1_NRZ_6TAP)) ? 1 : 0));
	// => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd133,0x0400,10,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "range sel", dev, lane, 0, 0xD133, 1, 0x0400, 10);

	// EFUN(wr_txfir_tap0_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd133,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "pre3",   dev, lane, 0, 0xD133, link_config.pre3,   0x1FF, 0);
	// EFUN(wr_txfir_tap1_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd134,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "pre2",   dev, lane, 0, 0xD134, link_config.pre2,   0x1FF, 0);
	// EFUN(wr_txfir_tap2_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd135,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "pre",    dev, lane, 0, 0xD135, link_config.pre1,   0x1FF, 0);
	// EFUN(wr_txfir_tap3_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd136,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "cursor", dev, lane, 0, 0xD136, link_config.cursor, 0x1FF, 0);
	// EFUN(wr_txfir_tap4_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd137,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "post",   dev, lane, 0, 0xD137, link_config.post1,  0x1FF, 0);
	// EFUN(wr_txfir_tap5_coeff ((uint16_t)val)); break; => osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd138,0x01ff,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "post2",  dev, lane, 0, 0xD138, link_config.post2,  0x1FF, 0);

	// EFUN(wr_txfir_tap_en(1)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd133,0x7000,12,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "enable", dev, lane, 0, 0xD133, 1, 0x7000, 12);

	// EFUN(wr_txfir_tap_load(1));  /* Load the tap coefficients into TXFIR. */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd133,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_EQ_LOG "load",   dev, lane, 0, 0xD133, 1, 0x0800, 11);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_CONFIG_LOG "lane_up_tx config -"
static int linktool_lane_up_tx_config(unsigned int dev, unsigned char lane, struct linktool_uc_info uc_info, struct linktool_link_config link_config)
{
	int            rtn;
	unsigned int   addr;
	unsigned short reset_state;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_config");

	// rd_tx_lane_dp_reset_state = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd1d9,13,13,__ERR)
	LINKTOOL_PMI_FIELD_RD(LANE_UP_TX_CONFIG_LOG, dev, lane, 0, 0xD1D9, 13, 13, &reset_state);
	DEBUG(linktool_lane_up_tx_debug, "%s reset_state = %d", LANE_UP_TX_CONFIG_LOG, reset_state);

	/* osr mode pins only */
	// EFUN(bcm_serdes_set_use_tx_osr_mode_pins_only(sa__, 1));
	addr = uc_info.lane_static_var_ram_base +
		(lane * uc_info.lane_static_var_ram_size) +
		0x9C + (uc_info.grp_ram_size * (lane >> 1));
	DEBUG(linktool_lane_up_tx_debug, "%s pin control addr = 0x%X", LANE_UP_TX_CONFIG_LOG, addr);
	rtn = linktool_uc_ram_wr8(dev, lane, addr, 0xBF); // bit 7 is enable
	if (rtn) {
		ERROR("%s uc_ram_wr failed [%d]", LANE_UP_TX_CONFIG_LOG, rtn);
		goto out;
	}

	// EFUN(blackhawk_sa_set_tx_rx_width_pam_osr(sa__, config->tx_width, 0, config->tx_encoding, rx_encoding, tx_osr, rx_osr, ip_type));
	rtn = linktool_lane_up_tx_mode_set(dev, lane, link_config);
	if (rtn) {
		ERROR("%s tx_mode_set failed [%d]", LANE_UP_TX_CONFIG_LOG, rtn);
		goto out;
	}

	/* precoder enable */
	// EFUN(wr_bcm_serdes_pam4_precoder_en(sa__, 1)
	// wr_bcm_serdes_pam4_precoder_en(sa__,wr_val) (bcm_serdes_acc_mwr_reg_u8(sa__,0xd175,0x0002,1,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_TX_CONFIG_LOG "pam4 precoder enable", dev, lane, 0, 0xD175, 0, 0x0002, 1);

	DEBUG(linktool_lane_up_tx_debug, "%s BCM logs an event here (PMI 0x57D, 0x57E)", LANE_UP_TX_CONFIG_LOG);

	// EFUN(osprey_sa_set_tx_eq(sa__, &config->tx_eq));
	rtn = linktool_lane_up_tx_eq(dev, lane, link_config);
	if (rtn) {
		ERROR("%s tx_equal failed [%d]", LANE_UP_TX_CONFIG_LOG, rtn);
		goto out;
	}

	// FIXME: inversions here, for now just disable
	DEBUG(linktool_lane_up_tx_debug, "%s invert is disabled", LANE_UP_TX_CONFIG_LOG);
	// EFUN(wr_bcm_serdes_tx_pmd_dp_invert(sa__, val)); = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd173,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_TX_CONFIG_LOG "inversion", dev, lane, 0, 0xD173, 0, 0x0001, 0);

	// FIXME: link training here, for now just disable
	DEBUG(linktool_lane_up_tx_debug, "%s training is disabled", LANE_UP_TX_CONFIG_LOG);
	// EFUN(wr_bcm_serdes_linktrn_ieee_training_enable(sa__, val)); = (bcm_serdes_acc_mwr_reg_u8(sa__,0x0096,0x0002,1,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_TX_CONFIG_LOG "training", dev, lane, 0, 0x0096, 0, 0x0002, 1);

	// FIXME: tca config here, for now just disable
	DEBUG(linktool_lane_up_tx_debug, "%s tca is disabled", LANE_UP_TX_CONFIG_LOG);
	// EFUN(wr_bcm_serdes_tx_data_vld_sync_en(sa__, 0)); = EFUN(wr_bcm_serdes_tx_data_vld_sync_en(sa__, val)); = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd173,0x0040,6,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_TX_CONFIG_LOG "tca", dev, lane, 0, 0xD173, 0, 0x0040, 6);
	// EFUN(blackhawk_sbus_wr_field(sa__, (BH_MISC_REG), 24+lane, 0x1, 0)); // #define BH_MISC_REG         48
	LINKTOOL_SBUS_FIELD_WR(LANE_UP_TX_CONFIG_LOG "misc reg", dev, (LINKTOOL_TX_MISC_REG + (8 * lane)), (24 + lane), 0x1, 0x0);

	// FIXME: txppm_offset here - only if it's enabled
	// EFUN(bcm_serdes_tx_pi_freq_override(sa__, 1, txppm_offset)); = osprey7_v2l4p1_tx_pi_freq_override
	DEBUG(linktool_lane_up_tx_debug, "%s txppm_offset (do nothing for now)", LANE_UP_TX_CONFIG_LOG);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_PRBS_DATA_SET_LOG "lane_up_prbs_data_set -"
static int linktool_lane_up_prbs_data_set(unsigned int dev, unsigned char lane)
{
	int rtn;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_prbs_data_set");

	// EFUN(wr_prbs_gen_mode_select((uint8_t)prbs_poly_mode));  /* PRBS Generator mode sel */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd171,0xf800,11,wr_val)
	// FIXME: 5 is a hardcoded value
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "prbs gen mode", dev, lane, 0, 0xD171, 5, 0xF800, 11);
	// EFUN(wr_prbs_gen_inv(prbs_inv)); /* PRBS Invert Enable/Disable */ => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd171,0x0010,4,wr_val)
	// FIXME: 0 is a hardcoded value
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "prbs gen inv",  dev, lane, 0, 0xD171, 0, 0x0010, 4);
	// EFUN(bcm_serdes_tx_prbs_en(sa__, 0x1)); => EFUN(wr_prbs_gen_en(val) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd171,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "tx prbs en",    dev, lane, 0, 0xD171, 1, 0x0001, 0);
	// EFUN(wr_patt_gen_en(0x0)) => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd170,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "patt gen en",   dev, lane, 0, 0xD170, 0, 0x0001, 0);
	// wr_bcm_serdes_rmt_lpbk_en(sa__, 0) (bcm_serdes_acc_mwr_reg_u8(sa__,0xd172,0x0001,0,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "rmt lpbk en",   dev, lane, 0, 0xD172, 0, 0x0001, 0);
	// EFUN(wr_bcm_serdes_prbs_chk_en(sa__, 0));  = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd161,0x0001,0,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "prbs chk en",   dev, lane, 0, 0xD161, 0, 0x0001, 0);
	// EFUN(wr_bcm_serdes_prbs_chk_en(sa__, 1));  = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd161,0x0001,0,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_PRBS_DATA_SET_LOG "prbs chk en",   dev, lane, 0, 0xD161, 1, 0x0001, 0);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_CORE_DATA_SET_LOG "lane_up_core_data_set -"
static int linktool_lane_up_core_data_set(unsigned int dev, unsigned char lane)
{
	int rtn;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_core_data_set");

	// EFUN(wr_bcm_serdes_rmt_lpbk_en(sa__, 0)) => bcm_serdes_acc_mwr_reg_u8(sa__,0xd172,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_CORE_DATA_SET_LOG "rmt lpbk en",  dev, lane, 0, 0xD172, 0, 0x0001, 0);
	// EFUN(bcm_serdes_tx_prbs_en(sa__, 0)) => EFUN(wr_prbs_gen_en(val) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd171,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_CORE_DATA_SET_LOG "tx prbs en",   dev, lane, 0, 0xD171, 0, 0x0001, 0);
	// EFUN(wr_patt_gen_en(0x0)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd170,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(LANE_UP_CORE_DATA_SET_LOG "patt gen en",  dev, lane, 0, 0xD170, 0, 0x0001, 0);
	// EFUN(wr_bcm_serdes_prbs_chk_en(sa__, 0));  = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd161,0x0001,0,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_CORE_DATA_SET_LOG "prbs chk en",  dev, lane, 0, 0xD161, 0, 0x0001, 0);
	// EFUN(wr_bcm_serdes_prbs_chk_en(sa__, 1));  = (bcm_serdes_acc_mwr_reg_u8(sa__,0xd161,0x0001,0,wr_val))
	LINKTOOL_PMI_WR(LANE_UP_CORE_DATA_SET_LOG "prbs chk en",  dev, lane, 0, 0xD161, 1, 0x0001, 0);

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_START_LOG "lane_up_tx_start -"
static int linktool_lane_up_tx_start(unsigned int dev, unsigned char lane)
{
	int rtn;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx_start");

	// wr_sdk_tx_disable(wr_val) osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd131,0x0001,0,wr_val) /* set to 0 to enable */
	LINKTOOL_PMI_WR(LANE_UP_TX_START_LOG "tx disable", dev, lane, 0, 0xD131, 0, 0x0001, 0);

	rtn = linktool_lane_up_prbs_data_set(dev, lane);
	if (rtn) {
		ERROR("%s prbs_data_set failed [%d]", LANE_UP_TX_START_LOG, rtn);
		goto out;
	}

	rtn = linktool_lane_up_core_data_set(dev, lane);
	if (rtn) {
		ERROR("%s core_data_set failed [%d]", LANE_UP_TX_START_LOG, rtn);
		goto out;
	}

	rtn = 0;
out:
	return rtn;
}

#define LANE_UP_TX_LOG "lane_up_tx -"
int linktool_lane_up_tx(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	int lane_map = cmd_obj->link_config.lane_map;
	int lane_num;

	DEBUG(linktool_lane_up_tx_debug, "lane_up_tx");

	DEBUG(linktool_lane_up_tx_debug, "%s BCM code calls to get the num_lanes (PMI 0xD10A)", LANE_UP_TX_LOG);

	for (lane_num = 0; lane_num < LINKTOOL_NUM_LANES; ++lane_num) {

		if (((lane_map >> lane_num) & 1) == 0)
			continue;

		DEBUG(linktool_lane_up_tx_debug, "%s power (lane_num = %d)", LANE_UP_TX_LOG, lane_num);
		rtn = linktool_lane_up_tx_power(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s tx_power failed [%d]", LANE_UP_TX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_tx_debug, "%s BCM calls blackhawk_sa_get_rx_input_loopback (PMI 0xD1AE)", LANE_UP_TX_LOG);

		rtn = linktool_lane_up_toggle_reset(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s toggle reset failed [%d]", LANE_UP_TX_LOG, rtn);
			goto out;
		}

		DEBUG(linktool_lane_up_tx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_osr (PMI 0xD1CB, 0xD1DB)", LANE_UP_TX_LOG);
		DEBUG(linktool_lane_up_tx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_width (sbus 0x45, 0x42)", LANE_UP_TX_LOG);
		DEBUG(linktool_lane_up_tx_debug, "%s BCM calls blackhawk_sa_get_tx_rx_line_encoding (sbus 0x42, 0x45)", LANE_UP_TX_LOG);

		DEBUG(linktool_lane_up_tx_debug, "%s config (lane_num = %d)", LANE_UP_TX_LOG, lane_num);
		rtn = linktool_lane_up_tx_config(cmd_obj->dev_addr, lane_num, cmd_obj->uc_info, cmd_obj->link_config);
		if (rtn) {
			ERROR("%s tx_config failed [%d]", LANE_UP_TX_LOG, rtn);
			goto out;
		}
	}

	for (lane_num = 0; lane_num < LINKTOOL_NUM_LANES; ++lane_num) {

		if (((lane_map >> lane_num) & 1) == 0)
			continue;

		DEBUG(linktool_lane_up_tx_debug, "%s start (lane_num = %d)", LANE_UP_TX_LOG, lane_num);
		rtn = linktool_lane_up_tx_start(cmd_obj->dev_addr, lane_num);
		if (rtn) {
			ERROR("%s tx_reset_start failed [%d]", LANE_UP_TX_LOG, rtn);
			goto out;
		}
	}

	rtn = 0;
out:
	return rtn;
}
