// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_iface.h"
#include "linktool_sbus.h"
#include "linktool_pmi.h"
#include "linktool_cmds.h"

#include "linktool_uc_ram.h"
#include "linktool_lane_up_tx.h"
#include "linktool_lane_up_rx.h"

// FIXME: scrub out broadcom annotations

static unsigned int linktool_cmd_lane_up_debug = 0;

void linktool_cmd_lane_up_debug_set()
{
	linktool_cmd_lane_up_debug = 1;
}

#define LANE_UP_CHECK_LOG         "lane_up_check -"
#define SIGNAL_DETECT_CHECK_COUNT 10
#define LOCK_CHECK_COUNT          20
#define TUNING_CHECK_COUNT        10
#define LINKTOOL_TX_CTL_REG       64
static int linktool_lane_up_check(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	int            lane;
	unsigned int   dev = cmd_obj->dev_addr;
	int            lane_map = cmd_obj->link_config.lane_map;
	unsigned short signal_detect;
	unsigned short lock;
	int            x;
	unsigned int   addr;
	unsigned char  tuning_state;

	DEBUG(linktool_cmd_lane_up_debug, "lane_up_check");

	/* lane map */
	for (lane = 0; lane < LINKTOOL_NUM_LANES; ++lane) {
		unsigned short dummy;
		if (((lane_map >> lane) & 1) == 0)
			continue;
		DEBUG(linktool_cmd_lane_up_debug, "%s map (lane = %d)", LANE_UP_CHECK_LOG, lane);
		LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "lane 0 map", dev, lane, 0, 0xD190, &dummy);
		LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "lane 1 map", dev, lane, 0, 0xD191, &dummy);
		LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "lane 2 map", dev, lane, 0, 0xD192, &dummy);
		LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "lane 3 map", dev, lane, 0, 0xD193, &dummy);
	}

	/* pin control disable */
	// blackhawk_set_pin_control(aapl,new_addr,BH_PIN_TX_DISABLE,0); = EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 14, 0x1, ~enable));
	// /* Clear TX_DISABLE_GATE */ = blackhawk_sbus_wr_field(sa__, reg + 8 * physical_tx_lane, start, mask, value);
	for (lane = 0; lane < LINKTOOL_NUM_LANES; ++lane) {
		if (((lane_map >> lane) & 1) == 0)
			continue;
		DEBUG(linktool_cmd_lane_up_debug, "%s pin control (lane = %d)", LANE_UP_CHECK_LOG, lane);
		LINKTOOL_SBUS_FIELD_WR(LANE_UP_CHECK_LOG, dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 14, 0x1, 0xFFFF);
	}


	for (lane = 0; lane < LINKTOOL_NUM_LANES; ++lane) {
		if (((lane_map >> lane) & 1) == 0)
			continue;

		/* check signal */
		DEBUG(linktool_cmd_lane_up_debug, "%s signal detect (lane = %d)", LANE_UP_CHECK_LOG, lane);
		for (x = 0; x < SIGNAL_DETECT_CHECK_COUNT; ++x) {
			// ESTM(rddata = reg_rd_bcm_serdes_SIGDET_SDSTATUS_0(sa__)); => bcm_serdes_acc_rde_reg(sa__,0xd0e8,__ERR)
			LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "signal detect", dev, lane, 0, 0xD0E8, &signal_detect);
                	DEBUG(linktool_cmd_lane_up_debug, "%s signal_detect = 0x%X", LANE_UP_CHECK_LOG, signal_detect);
                        // NOTE: sd = bit 0, sd_change = bit 1 - if sd_change == 1 then sd = 0 else sd = bit 0
                        if (((signal_detect & 0x0002) == 0) && ((signal_detect & 0x0001) == 1))
                                break;
			usleep(500);
		}
		if (x >= SIGNAL_DETECT_CHECK_COUNT) {
			ERROR("%s signal detect failed", LANE_UP_CHECK_LOG);
			rtn = -1;
			goto out;
		}

		/* check lock */
		DEBUG(linktool_cmd_lane_up_debug, "%s lock (lane = %d)", LANE_UP_CHECK_LOG, lane);
		for (x = 0; x < LOCK_CHECK_COUNT; ++x) {
			// rd_bcm_serdes_pmd_rx_lock(sa__) (bcm_serdes_acc_rde_field_u8(sa__,0xd16c,15,15,__ERR))
			LINKTOOL_PMI_RD(LANE_UP_CHECK_LOG "rx lock", dev, lane, 0, 0xD16C, &lock);
                	DEBUG(linktool_cmd_lane_up_debug, "%s lock = %u", LANE_UP_CHECK_LOG, lock);
			if (lock != 0)
				break;
			usleep(1000000);
		}
		if (x >= LOCK_CHECK_COUNT) {
			ERROR("%s lock failed", LANE_UP_CHECK_LOG);
			rtn = -1;
			goto out;
		}
	}

	/* check tuning */
	for (lane = 0; lane < LINKTOOL_NUM_LANES; ++lane) {
		if (((lane_map >> lane) & 1) == 0)
			continue;

		DEBUG(linktool_cmd_lane_up_debug, "%s tuning (lane = %d)", LANE_UP_CHECK_LOG, lane);

		// EPSTM(rddata = osprey7_v2l4p1_rdb_uc_ram(sa__, err_code_p, lane_addr_offset)); /* Use Micro register interface for reading RAM */
                addr = cmd_obj->uc_info.lane_var_ram_base + 0xE4 +
                        (lane * cmd_obj->uc_info.lane_var_ram_size) +
                        (cmd_obj->uc_info.grp_ram_size * (lane >> 1));
                DEBUG(linktool_cmd_lane_up_debug, "%s tuning status addr = 0x%X", LANE_UP_CHECK_LOG, addr);
		for (x = 0; x < TUNING_CHECK_COUNT; ++x) {
                	rtn = linktool_uc_ram_rd8(dev, lane, addr, &tuning_state);
			if (rtn) {
				ERROR("%s uc_ram_rd8 failed [%d]", LANE_UP_CHECK_LOG, rtn);
				goto out;
			}
                	DEBUG(linktool_cmd_lane_up_debug, "%s tuning status = 0x%X", LANE_UP_CHECK_LOG, tuning_state);
			// NOTE: runing state done = 3
			if (tuning_state == 3)
				break;
		}
		if (x >= TUNING_CHECK_COUNT) {
			ERROR("%s tuning failed", LANE_UP_CHECK_LOG);
			rtn = -1;
			goto out;
		}

		DEBUG(linktool_cmd_lane_up_debug, "%s BCM calls blackhawk_serdes_error_reset (PMI)", LANE_UP_CHECK_LOG);
		DEBUG(linktool_cmd_lane_up_debug, "%s BCM calls bcm_serdes_prbs_err_count_state (PMI)", LANE_UP_CHECK_LOG);
	}

	/* pin control return */
	// blackhawk_set_pin_control(aapl,new_addr,BH_PIN_TX_DISABLE,val); = EFUN(blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 14, 0x1, ~enable));
	// /* Clear TX_DISABLE_GATE */ = blackhawk_sbus_wr_field(sa__, reg + 8 * physical_tx_lane, start, mask, value);
	for (lane = 0; lane < LINKTOOL_NUM_LANES; ++lane) {
		if (((lane_map >> lane) & 1) == 0)
			continue;
		DEBUG(linktool_cmd_lane_up_debug, "%s pin control (lane = %d)", LANE_UP_CHECK_LOG, lane);
		LINKTOOL_SBUS_FIELD_WR(LANE_UP_CHECK_LOG, dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 14, 0x1, 0xFFFF);
	}


	rtn = 0;
out:
	return rtn;
}

#define FW_INFO_GET_LOG                         "fw_info_get -"
#define FW_INFO_SIGNATURE                       0x464E49
#define FW_INFO_LANE_STATIC_VAR_RAM_BASE_OFFSET 0x74
#define FW_INFO_LANE_STATIC_VAR_RAM_SIZE_OFFSET 0x78
#define FW_INFO_LANE_VAR_RAM_BASE_OFFSET        0x74
#define FW_INFO_LANE_VAR_RAM_SIZE_OFFSET        0x78
#define FW_INFO_GRP_RAM_SIZE_OFFSET             0x68
#define FW_INFO_OTHER_SIZE_OFFSET               0x0C
#define FW_INFO_LANE_MEM_PTR_OFFSET             0x1C
#define FW_INFO_LANE_MEM_SIZE_OFFSET            0x08
static int linktool_lane_up_fw_info_get(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	unsigned char rd_buff[128];
	unsigned int  signature;
	unsigned char version;

	DEBUG(linktool_cmd_lane_up_debug, "fw_info_get");

	// NOTE: sizes based on BCM code
	rtn = linktool_uc_ram_rd_blk(cmd_obj->dev_addr, 0x100, 128, rd_buff);
	if (rtn) {
		ERROR(FW_INFO_GET_LOG "ram_rd_blk failed [%d]", rtn);
		return rtn;
	}

	signature = (*(unsigned int *)rd_buff & 0xFFFFFF);
	version   = ((*(unsigned int *)rd_buff >> 24) & 0xFF);
	DEBUG(linktool_cmd_lane_up_debug, "%s sig = 0x%X, ver = 0x%X", FW_INFO_GET_LOG, signature, version);
	if (signature != FW_INFO_SIGNATURE) {
		ERROR(FW_INFO_GET_LOG "signature check failed (sig = 0x%X, ver = 0x%X)", signature, version);
		return -1;
	}

	cmd_obj->uc_info.lane_count               = rd_buff[FW_INFO_OTHER_SIZE_OFFSET];
	cmd_obj->uc_info.lane_static_var_ram_base = *(unsigned int *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_BASE_OFFSET];
	cmd_obj->uc_info.lane_static_var_ram_size = *(unsigned int *)&rd_buff[FW_INFO_LANE_STATIC_VAR_RAM_SIZE_OFFSET];
	cmd_obj->uc_info.lane_var_ram_base        = *(unsigned int *)&rd_buff[FW_INFO_LANE_MEM_PTR_OFFSET];
	cmd_obj->uc_info.lane_var_ram_size        = (*(unsigned int *)&rd_buff[FW_INFO_LANE_MEM_SIZE_OFFSET] >> 16) & 0xFFFF;
	cmd_obj->uc_info.grp_ram_size             = *(unsigned int *)&rd_buff[FW_INFO_GRP_RAM_SIZE_OFFSET] & 0xFFFF;

	DEBUG(linktool_cmd_lane_up_debug, "%s lane count               = %d", FW_INFO_GET_LOG, cmd_obj->uc_info.lane_count);
	DEBUG(linktool_cmd_lane_up_debug, "%s lane var ram base        = 0x%X", FW_INFO_GET_LOG, cmd_obj->uc_info.lane_var_ram_base);
	DEBUG(linktool_cmd_lane_up_debug, "%s lane var ram size        = 0x%X", FW_INFO_GET_LOG, cmd_obj->uc_info.lane_var_ram_size);
	DEBUG(linktool_cmd_lane_up_debug, "%s lane static var ram base = 0x%X", FW_INFO_GET_LOG, cmd_obj->uc_info.lane_static_var_ram_base);
	DEBUG(linktool_cmd_lane_up_debug, "%s lane static var ram size = 0x%X", FW_INFO_GET_LOG, cmd_obj->uc_info.lane_static_var_ram_size);
	DEBUG(linktool_cmd_lane_up_debug, "%s grp ram size             = 0x%X", FW_INFO_GET_LOG, cmd_obj->uc_info.grp_ram_size);

	return 0;
}

#define LANE_UP_LOG "lane_up -"
int linktool_cmd_lane_up(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;

	DEBUG(linktool_cmd_lane_up_debug, "cmd_lane_up");

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(LANE_UP_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

// NOTE: reading RAM lane is 0xFF
// load RAM info

	rtn = linktool_lane_up_fw_info_get(cmd_obj);
	if (rtn != 0) {
		ERROR(LANE_UP_LOG "fw_info_get failed [%d]", rtn);
		goto out;
	}

// -> blackhawk_serdes_multi_lane_tx_init -> blackhawk_sa_lane_tx_init
//    NOTE: tx pll = 0xF
//    -> loop lanes
//       -> bcm_serdes_lane_pwrdn(sa__, PWR_ON)
//       -> blackhawk_sa_get_tx_rx_ready(sa__, &tx_en, &rx_en)
//       -> blackhawk_sa_set_lane_dp_reset(sa__, TRUE, TRUE, 1, 1)
//       -> blackhawk_sa_get_rx_input_loopback
//       -> blackhawk_sa_toggle_tx_rx_lane_rstb(sa__, 1, 0)
//       -> blackhawk_sa_get_tx_rx_osr(sa__, &tx_osr, &rx_osr, ip_type)
//       -> blackhawk_sa_get_tx_rx_width(sa__, &tx_width, &rx_width, ip_type)
//       -> blackhawk_sa_get_tx_rx_line_encoding(sa__, &tx_encoding, &rx_encoding, ip_type)
//       -> bcm_serdes_set_use_tx_osr_mode_pins_only(sa__, 1)
//       -> blackhawk_sa_set_tx_rx_width_pam_osr(sa__, config->tx_width, 0, config->tx_encoding, rx_encoding, tx_osr, rx_osr, ip_type)
//       -> wr_bcm_serdes_pam4_precoder_en(sa__, 0)
//       -> osprey_sa_set_tx_eq(sa__, &config->tx_eq)
//       INVERT = 0
//       -> wr_bcm_serdes_tx_pmd_dp_invert(sa__, config->tx_invert)
//       TRAIN enable = 0
//       -> wr_bcm_serdes_linktrn_ieee_training_enable(sa__, 0)
//       TCA enable = 0
//       -> wr_bcm_serdes_tx_data_vld_sync_en(sa__, 0)
//       -> blackhawk_sbus_wr_field(sa__, (BH_MISC_REG), 24+lane, 0x1, 0)
//       TXPPM_OFFSET = 0 (nothing to do)
//    -> loop lanes
//       -> bcm_serdes_tx_disable(sa__, !config->tx_output_en)
//       -> blackhawk_sa_set_tx_data_sel(sa__, config->prbs_mode, BLACKHAWK_PRBS_PATT)
//       -> blackhawk_sa_set_tx_data_sel(sa__, PRBS_31, BLACKHAWK_CORE_DATA)

	rtn = linktool_lane_up_tx(cmd_obj);
	if (rtn) {
		ERROR("%s tx failed [%d]", LANE_UP_LOG, rtn);
		goto out;
	}

// -> blackhawk_serdes_multi_lane_rx_init -> blackhawk_sa_lane_rx_init
//    NOTE: rx pll = 0xF
//    -> loop lanes
//       -> blackhawk_sa_set_lane_dp_reset(sa__, TRUE, FALSE, 1, 1)
//       -> bcm_serdes_lane_pwrdn(sa__, PWR_ON)
//       -> blackhawk_sa_get_rx_input_loopback(sa__, &input_loopback, ip_type)
//       -> blackhawk_sa_get_tx_rx_ready(sa__, &tx_en, &rx_en)
//       -> blackhawk_sa_get_tx_rx_osr(sa__, &tx_osr, &rx_osr, ip_type)
//       -> blackhawk_sa_get_tx_rx_width(sa__, &tx_width, &rx_width, ip_type)
//       -> blackhawk_sa_get_tx_rx_line_encoding(sa__, &tx_encoding, &rx_encoding, ip_type)
//       -> bcm_serdes_set_use_rx_osr_mode_pins_only(sa__, 1)
//       -> blackhawk_sa_set_tx_rx_width_pam_osr(sa__, 0, config->rx_width, tx_encoding, config->rx_encoding, tx_osr, rx_osr, ip_type)
//       FIRMWARE MODE = 1
//       -> bcm_serdes_set_uc_lane_cfg(sa__, lane_struct_val)
//       INVERT = 0
//       -> wr_bcm_serdes_rx_pmd_dp_invert(sa__, config->rx_invert)
//       TRAIN = 0
//       -> wr_bcm_serdes_linktrn_ieee_training_enable(sa__, 0)
//       FORCE CDR MODE = 0 (nothing to do)
//       -> bcm_serdes_dig_lpbk(sa__, FALSE)
//       INIT MODE = BLACKHAWK_CORE_DATA_ELB = 3
//       -> blackhawk_sa_set_rx_cmp_data(sa__, PRBS_31, 0)
//       -> bcm_serdes_dig_lpbk(sa__, FALSE)
//       -> blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 17, 0x3, 0x1)
//       HOFFSET DISABLE = 0 (nothing to do)
//       -> blackhawk_sa_set_lane_dp_reset(sa__, TRUE, TRUE, 0, 0)
//          WAIT rd_bcm_serdes_tx_lane_dp_reset_state(sa__) == 0
//       LOOP CHECK SIGNAL OK
//       -> blackhawk_sa_get_signal_ok(sa__, &sig_ok)
//    -> loop lanes
//       TXPPM OFFSET = 0
//       -> bcm_serdes_tx_clock_align_enable(sa__, tca_enable, tca_lane_mask)
//       TCA ENABLE = 0 (nothing to do)

	rtn = linktool_lane_up_rx(cmd_obj);
	if (rtn) {
		ERROR("%s rx failed [%d]", LANE_UP_LOG, rtn);
		goto out;
	}

// -> loop lanes
//    -> blackhawk_set_pin_control
// -> blackhawk_serdes_pmd_rx_lock_wait_timeout(aapl, addr, config->rx_encoding==AVAGO_SERDES_NRZ ? 1500:6000)
//    -> loop lanes
// -> loop lanes
//    -> bcm_serdes_get_rx_tuning_status
// -> loop lanes
//    -> blackhawk_set_pin_control

	rtn = linktool_lane_up_check(cmd_obj);
	if (rtn) {
		ERROR("%s check failed [%d]", LANE_UP_LOG, rtn);
		goto out;
	}

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(LANE_UP_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}
