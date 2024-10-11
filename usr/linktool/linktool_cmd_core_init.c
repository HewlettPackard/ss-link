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

// FIXME: scrub out broadcom annotations

#define LINKTOOL_NUM_MICROS 2
#define LINKTOOL_NUM_PLLS   1

static unsigned int linktool_cmd_core_init_debug = 0;

void linktool_cmd_core_init_debug_set()
{
	linktool_cmd_core_init_debug = 1;
}

#define LINKTOOL_WAIT_ACTIVE_COUNT 100
static int linktool_cmd_wait_active(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	int            x;
	unsigned char  data8;
	unsigned short data16;

	/* uc wait active */
	for (x = 0; x < LINKTOOL_WAIT_ACTIVE_COUNT; ++x) {
		// rdc_uc_active() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd101,14,15,__ERR)
		LINKTOOL_PMI_FIELD_RD("wait active", dev, 0xFF, 0, 0xD101, 14, 15, &data8);
		if (data8 != 1)
			continue;
		// reg_rdc_MICRO_B_COM_RMI_MICRO_SDK_STATUS0 = osprey7_v2l4p1_acc_rde_reg(sa__, 0xd21a,__ERR)
		LINKTOOL_PMI_RD("wait active status", dev, 0xFF, 0, 0xD21A, &data16);
		if ((data16 & 0x3) != 0x3)
			continue;
		break;
	}
	if (x >= LINKTOOL_WAIT_ACTIVE_COUNT) {
		ERROR("wait active timeout");
		rtn = -1;
		goto out;
	}

	DEBUG(linktool_cmd_core_init_debug, "UC is active");
	rtn = 0;

out:
	return rtn;
}

#define LANE_RESET_LOG "lane_reset -"
static int linktool_cmd_lane_reset(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	int          x;
	unsigned int dev = cmd_obj->dev_addr;

	DEBUG(linktool_cmd_core_init_debug, "lane_reset");

	for (x = 0; x < LINKTOOL_NUM_LANES; ++x) {
		DEBUG(linktool_cmd_core_init_debug, "lane_reset (lane = %d)", x);
	/* TX */
		// wr_tx_ln_dp_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1d1, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "tx_dp", dev, x, 0, 0xD1D1, 0, 0x0001, 0);
		usleep(1);
		// wr_tx_ln_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1de, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "tx_s",  dev, x, 0, 0xD1DE, 0, 0x0001, 0);
		usleep(1);
		// wr_tx_ln_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1de, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "tx_s",  dev, x, 0, 0xD1DE, 1, 0x0001, 0);
	/* RX */
		// wr_rx_ln_dp_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1c1, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "rx_dp", dev, x, 0, 0xD1C1, 0, 0x0001, 0);
		usleep(1);
		// wr_rx_ln_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1ce, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "rx_s",  dev, x, 0, 0xD1CE, 0, 0x0001, 0);
		usleep(1);
		// wr_rx_ln_s_rstb(0x1)osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1ce, 0x0001, 0, wr_val)
		LINKTOOL_PMI_WR(LANE_RESET_LOG "rx_s",  dev, x, 0, 0xD1CE, 1, 0x0001, 0);
	}

	rtn = 0;

out:
	return rtn;
}

#define LINKTOOL_TEMP_REG   49
#define LINKTOOL_TEMP_TRIES 10
static int linktool_cmd_temperature_check(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	int          x;
	unsigned int dev = cmd_obj->dev_addr;
	unsigned int data32;

	DEBUG(linktool_cmd_core_init_debug, "temperature check");

	for (x = 0; x < LINKTOOL_TEMP_TRIES; ++x) {
		// blackhawk_sbus_rd_field(sa__, BH_TEMP_REG, 0, 0x7FF, &temperature)
		LINKTOOL_SBUS_FIELD_RD("temperature check - ", dev, LINKTOOL_TEMP_REG, 0, 0x7FF, &data32);
		if (data32 != 0) {
			DEBUG(linktool_cmd_core_init_debug, "temperature check (temp = 0x%X)", data32);
			cmd_obj->temperature = data32;
			rtn = 0;
			goto out;
		}
		usleep(100000);
	}

	ERROR("temperature check - not set");
	// FIXME: seems like this is ok?
	rtn = 0;

out:
	return rtn;
}

#define LINKTOOL_MISC_REG  48
#define SBUS_RESET_LOG     "sbus reset -"
static int linktool_cmd_sbus_reset(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	unsigned int   state;
	unsigned int   data32;
	unsigned int   strap;
	unsigned short stack_size;
	unsigned int   misc_reg_data;

	DEBUG(linktool_cmd_core_init_debug, "sbus reset");

	/* read SBM processor state */
        // blackhawk_sbus_rd(sa__, 5, &state);
	LINKTOOL_SBUS_RD(SBUS_RESET_LOG "proc state", 0xFD, 5, &state);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "RD (state = 0x%X)", state);

	/* single step SMB processor to halt it */
        // blackhawk_sbus_wr(sa__, 5, (state & ~3) | 1);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "WR (state = 0x%X)", ((state & ~3) | 1));
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "proc halt", 0xFD, 5, ((state & ~3) | 1));

	/* set target reg addr to RO */
	// blackhawk_sbus_wr(sa__, 0xff, 0x00);
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "read only", dev, 0xFF, 0);

	/* soft reset */
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "soft reset");
	// blackhawk_sbus(sa__, 0x00, 0x00, 0x00, 1, NULL); => (sa, reg, cmd, data, data_back, value)
	LINKTOOL_SBUS_RST(SBUS_RESET_LOG "reset", dev);
	/* check out of reset */
	// int reset = avago_sbus_rd(aapl, sbus_addr, 48);
	LINKTOOL_SBUS_RD(SBUS_RESET_LOG "reset check", dev, 48, &data32);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "(reset = 0x%x)", data32);
	// force out of reset if needed
	// if( (reset & 3) == 0 || (reset & 6) == 2 )
        //        avago_sbus_wr(aapl, sbus_addr, 48, reset | 6);
	if (((data32 & 3) == 0) || ((data32 & 6) == 2))
		LINKTOOL_SBUS_WR(SBUS_RESET_LOG "reset force", dev, 48, (data32 | 6));

	/* strapping */
	// blackhawk_sbus_rd(sa__, 33, &reg33)
	LINKTOOL_SBUS_RD(SBUS_RESET_LOG "strap get", dev, 33, &data32);
	if (data32 & (1 << 31))
		strap = ((data32 >> 24) & 0x1F);
	else
		strap = ((data32 >> 16) & 0x1F);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "(strap = %d)", strap);

	/* stack size */
	// blackhawk_sbus_wr(sa__, 0x2, stack_size_addr|(devid_strap << 27))
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "strap set", dev, 2, (0xD22E | (strap << 27)));
	// blackhawk_sbus_rd(sa__, 0x4, &stack_size32)
	LINKTOOL_SBUS_RD(SBUS_RESET_LOG "stack size get", dev, 4, &data32);
	// stack_size = (stack_size32>>2)&0x1fff
	stack_size = ((data32 >> 2) & 0x1FFF);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "(stack_size = 0x%X)", stack_size);

	/* read misc reg */
	// blackhawk_sbus_rd(sa__, BH_MISC_REG, &bh_misc_reg_value)
	LINKTOOL_SBUS_RD(SBUS_RESET_LOG "misc reg get", dev, LINKTOOL_MISC_REG, &misc_reg_data);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "soft reset (misc_reg_data = 0x%X)", misc_reg_data);

	/* set PMD reset */
	// blackhawk_sbus_wr_field(sa__, BH_MISC_REG, 1, 0x1, 0x0)
	LINKTOOL_SBUS_FIELD_WR(SBUS_RESET_LOG "PMD reset", dev, LINKTOOL_MISC_REG, 1, 0x1, 0x0);
	/* assert PMD POR reset */
	// blackhawk_sbus_wr_field(sa__, BH_MISC_REG, 2, 0x1, 0x1)
	LINKTOOL_SBUS_FIELD_WR(SBUS_RESET_LOG "PMD POR set", dev, LINKTOOL_MISC_REG, 2, 0x1, 0x1);
	/* release PMD POR reset */
	// blackhawk_sbus_wr_field(sa__, BH_MISC_REG, 1, 0x1, 0x1)
	LINKTOOL_SBUS_FIELD_WR(SBUS_RESET_LOG "PMD POR clear", dev, LINKTOOL_MISC_REG, 1, 0x1, 0x1);

	/* restore stack size */
	// blackhawk_sbus_wr(sa__, 0x2, stack_size_addr|(devid_strap << 27) )
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "stack size addr set", dev, 2, (0xD22E | (strap << 27)));
	// blackhawk_sbus_wr(sa__, 0x3, (stack_size<<18)|0x8003)
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "stack size set ", dev, 3, ((stack_size << 18) | 0x8003));

	/* restore misc reg */
	// blackhawk_sbus_wr(sa__, BH_MISC_REG, bh_misc_reg_value)
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "misc reg set", dev, LINKTOOL_MISC_REG, misc_reg_data);

	/* restore SMB processor state */
	// blackhawk_sbus_wr(sa__, 5, state);
	LINKTOOL_SBUS_WR(SBUS_RESET_LOG "restore state", 0xFD, 5, state);

	/* re-gate PMD POR RSTB after reset */
	// blackhawk_sbus_wr_field(sa__, BH_MISC_REG, 1, 0x3, 0x3);
	LINKTOOL_SBUS_FIELD_WR(SBUS_RESET_LOG "re-gate", dev, LINKTOOL_MISC_REG, 1, 0x3, 0x3);

	// FIXME: do we need this?
	// rdc_bcm_serdes_micro_core_stack_size(sa__) = bcm_serdes_acc_rde_field_u16(sa__,0xd22e,1,3,__ERR
	// rdc_micro_core_stack_size() = osprey7_v2l4p1_acc_rde_field_u16(sa__, 0xd22e,1,3,__ERR)
	LINKTOOL_PMI_FIELD_RD(SBUS_RESET_LOG "stack size read", dev, 0xFF, 0, 0xD22E, 1, 3, &stack_size);
	DEBUG(linktool_cmd_core_init_debug, SBUS_RESET_LOG "(stack_size 0x%X)", stack_size);

	rtn = 0;

out:
	return rtn;
}

#define UC_RESET_LOG "uc reset -"
static int linktool_cmd_uc_reset(struct linktool_cmd_obj *cmd_obj, int enable)
{
	int           rtn;
	unsigned int  dev = cmd_obj->dev_addr;
	int           x;
	unsigned char data8;

	DEBUG(linktool_cmd_core_init_debug, "uc reset (enable = %d)", enable);

	if (enable ) {
		/* reset registers to default */
		// wrc_micro_micro_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "reset clear",   dev, 0xFF, 0, 0xD23E, 0, 0x0001, 0);
		// wrc_micro_micro_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,1,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "reset set",     dev, 0xFF, 0, 0xD23E, 1, 0x0001, 0);

		/* stop CRC calc */
		// wrc_micro_cr_crc_calc_en(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "stop crc calc", dev, 0xFF, 0, 0xD217, 0, 0x0001, 0);

		/* set the stack size */
		// wrc_micro_core_stack_size(stack_size) = osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd22e,0x7ffc,2,wr_val)
		// FIXME: check to make sure we have the stack size at this point
		LINKTOOL_PMI_WR(UC_RESET_LOG "stack size",    dev, 0xFF, 0, 0xD22E, cmd_obj->stack_size, 0x7FFC, 2);
		// wrc_micro_core_stack_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd22e,0x8000,15,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "stack enable",  dev, 0xFF, 0, 0xD22E, 1, 0x8000, 15);

		/* set the micro inactive */
		// wrc_uc_active(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd101,0x0002,1,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "active clear",  dev, 0xFF, 0, 0xD101, 0, 0x0002, 1);
	} else {
		// wrc_micro_micro_s_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "set",                 dev, 0xFF, 0, 0xD23E, 1, 0x0001, 0);
		// wrc_micro_master_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd200,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "master clock enable", dev, 0xFF, 0, 0xD200, 1, 0x0001, 0);
		// wrc_micro_master_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "master clear reset",  dev, 0xFF, 0, 0xD201, 1, 0x0001, 0);
		// wrc_micro_cr_access_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "cRAM enable",         dev, 0xFF, 0, 0xD227, 1, 0x0001, 0);
		// wrc_micro_ra_init(2) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "init cRAM set",       dev, 0xFF, 0, 0xD202, 2, 0x0300, 8);
		// rdc_micro_ra_initdone = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd203,15,15,__ERR)
		for (x = 0; x < 10; ++x) {
			LINKTOOL_PMI_FIELD_RD(UC_RESET_LOG "init done check", dev, 0xFF, 0, 0xD203, 15, 15, &data8);
			if (data8 != 0)
				break;
		}
		if (x >= 10) {
			ERROR(UC_RESET_LOG "wait for init done timeout");
			rtn = -1;
			goto out;
		}
		// wrc_micro_ra_init(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
		LINKTOOL_PMI_WR(UC_RESET_LOG "init cRAM clear", dev, 0xFF, 0, 0xD202, 0, 0x0300, 8);
		for (x = 0; x < LINKTOOL_NUM_MICROS; ++x) {
			// wrc_micro_core_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd240,0x0001,0,wr_val)
			LINKTOOL_PMI_WR(UC_RESET_LOG "core clock enable", dev, 0xFF, x, 0xD240, 1, 0x0001, 0);
			// wrc_micro_core_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd241,0x0001,0,wr_val)
			LINKTOOL_PMI_WR(UC_RESET_LOG "enable uc core",    dev, 0xFF, x, 0xD241, 1, 0x0001, 0);
		}
	}

	rtn = 0;

out:
	return rtn;
}

#define CORE_RESET_LOG "core_reset -"
static int linktool_cmd_core_reset(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	unsigned int dev = cmd_obj->dev_addr;
	int          x;

	DEBUG(linktool_cmd_core_init_debug, "core_reset");

	rtn = linktool_cmd_uc_reset(cmd_obj, 1);
	if (rtn != 0) {
		ERROR(CORE_RESET_LOG "uc_reset failed [%d]", rtn);
		goto out;
	}

	/* select common clock */
	// wrc_micro_clk_s_comclk_sel(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd19d,0x0040,6,wr_val)
	LINKTOOL_PMI_WR(CORE_RESET_LOG "comclk select", dev, 0xFF, 0, 0xD19D, 1, 0x0040, 6);

	/* set the broadcast port addr */
	// wrc_mdio_brcst_port_addr(0x1f) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xffdc,0x001f,0,wr_val)
	LINKTOOL_PMI_WR(CORE_RESET_LOG "bcast addr", dev, 0xFF, 0, 0xFFDC, 0x1F, 0x001F, 0);

	/* reset dp lanes */
	for (x = 0; x < LINKTOOL_NUM_LANES; ++x) {
		// osprey7_v2l4p1_ln_dp_reset(sa__, 1) = wr_ln_dp_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd0b1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(CORE_RESET_LOG "ln dp reset", dev, x, 0, 0xD0B1, 0, 0x0001, 0);
	}

	usleep(1);

	/* reset plls */
	for (x = 0; x < LINKTOOL_NUM_PLLS; ++x) {
		// osprey7_v2l4p1_core_dp_reset(sa__, 1) = wrc_core_dp_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd184,0x2000,13,wr_val)
		LINKTOOL_PMI_WR(CORE_RESET_LOG "core dp reset", dev, 0, x, 0xD184, 0, 0x2000, 13);
	}

	usleep(1);

	/* assert core reset */
	// wrc_core_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd101,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(CORE_RESET_LOG "core clear", dev, 0, 0, 0xD101, 0, 0x0001, 0);

	/* deassert core reset */
	// wrc_core_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd101,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(CORE_RESET_LOG "core clear", dev, 0, 0, 0xD101, 1, 0x0001, 0);

	rtn = 0;

out:
	return rtn;
}

#define CORE_RSTB_LOG    "core_rstb -"
#define TX_CTL_REG       64
#define RX_CTL_REG       67
#define RX_BIT_SLIP_REG  70
static int linktool_cmd_core_rstb(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	unsigned int dev = cmd_obj->dev_addr;
	int          x;

	DEBUG(linktool_cmd_core_init_debug, "core_rstb");

	rtn = linktool_cmd_core_reset(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_RSTB_LOG "core_reset failed [%d]", rtn);
		goto out;
	}

	/* isolate ctrl pins */
	// wrc_pmd_core_dp_h_rstb_pkill(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd182,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(CORE_RSTB_LOG "core clear",    dev, 0xFF, 0, 0xD182, 1, 0x0002, 1);
	// wrc_pmd_core_dp_h_rstb_pkill(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd182,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(CORE_RSTB_LOG "core clear",    dev, 0xFF, 1, 0xD182, 1, 0x0002, 1);

	for (x = 0; x < LINKTOOL_NUM_LANES; ++x) {
		/* power down */
		DEBUG(linktool_cmd_core_init_debug, "core_rstb - (lane = %d)", x);
		// blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 4, 0x7, 0x2) = blackhawk_sbus_wr_field(sa__, reg + 8 * physical_rx_lane, start, mask, value);
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "1", dev, RX_CTL_REG + (8 * x), 4, 0x7, 0x2);
		// blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 4, 0x7, 0x2)
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "2", dev, TX_CTL_REG + (8 * x), 4, 0x7, 0x2);
		/* enable gate for resets */
		// blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 7, 0x1, 0x1)
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "3", dev, RX_CTL_REG + (8 * x), 7, 0x1, 0x1);
		// blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 7, 0x1, 0x1)
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "4", dev, TX_CTL_REG + (8 * x), 7, 0x1, 0x1);
		/* enables */
		// blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 14, 0x1, 0x1)
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "5", dev, TX_CTL_REG + (8 * x), 14, 0x1, 0x1);
		// blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_BIT_SLIP_REG, 16, 0x1, 0x1)
		LINKTOOL_SBUS_FIELD_WR(CORE_RSTB_LOG "6", dev, RX_BIT_SLIP_REG + (8 * x), 16, 0x1, 0x1);
		/* enable RX dp */
		// bcm_serdes_rx_dp_reset(sa__, 0) = wr_rx_ln_dp_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1c1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(CORE_RSTB_LOG "rx core clear", dev, x, 0, 0xD1C1, 1, 0x0001, 0);
		/* enable TX dp */
		// bcm_serdes_tx_dp_reset(sa__, 0) = wr_tx_ln_dp_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1d1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(CORE_RSTB_LOG "tx core clear", dev, x, 0, 0xD1D1, 1, 0x0001, 0);
	}

	rtn = 0;

out:
	return rtn;
}

#define CORE_LANE_MAP_LOG "core lane map -"
static int linktool_cmd_core_lane_map(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	unsigned int  dev = cmd_obj->dev_addr;
	unsigned char data8;

	DEBUG(linktool_cmd_core_init_debug, "core lane map");

	// FIXME: do we need these two checks if we are in control?

	/* check uc data path in reset */
	// rdc_core_dp_s_rstb() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd184,2,15,__ERR)
	LINKTOOL_PMI_FIELD_RD(CORE_LANE_MAP_LOG "uc data path reset check", dev, 0xFF, 0, 0xD184, 2, 15, &data8);
	if (data8 != 0) {
		DEBUG(linktool_cmd_core_init_debug, CORE_LANE_MAP_LOG "uc data path not in reset");
		goto out;
	}
	/* check micro in reset */
	// rdc_micro_core_rstb() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd241,15,15,__ERR)
	LINKTOOL_PMI_FIELD_RD(CORE_LANE_MAP_LOG "uc reset check", dev, 0xFF, 0, 0xD241, 15, 15, &data8);
	if (data8 != 0) {
		DEBUG(linktool_cmd_core_init_debug, CORE_LANE_MAP_LOG "uc not in reset");
		goto out;
	}

	/* set simple lane map */
	// wrc_tx_lane_addr_0(*(tx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd190,0x1f00,8,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 0", dev, 0xFF, 0, 0xD190, 0, 0x1F00, 8);
	// wrc_rx_lane_addr_0(*(rx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd190,0x001f,0,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 0", dev, 0xFF, 0, 0xD190, 0, 0x001F, 0);
	
	// wrc_tx_lane_addr_1(*(tx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd191,0x1f00,8,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 1", dev, 0xFF, 0, 0xD191, 1, 0x1F00, 8);
	// wrc_rx_lane_addr_1(*(rx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd191,0x001f,0,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 1", dev, 0xFF, 0, 0xD191, 1, 0x001F, 0);
	
	// wrc_tx_lane_addr_2(*(tx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd192,0x1f00,8,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 2", dev, 0xFF, 0, 0xD192, 2, 0x1F00, 8);
	// wrc_rx_lane_addr_2(*(rx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd192,0x001f,0,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 2", dev, 0xFF, 0, 0xD192, 2, 0x001F, 0);

	// wrc_tx_lane_addr_3(*(tx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd193,0x1f00,8,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 3", dev, 0xFF, 0, 0xD193, 3, 0x1F00, 8);
	// wrc_rx_lane_addr_3(*(rx_lane_map++)) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd193,0x001f,0,wr_val)
	LINKTOOL_PMI_WR(CORE_LANE_MAP_LOG "tx 3", dev, 0xFF, 0, 0xD193, 3, 0x001F, 0);
	
	rtn = 0;

out:
	return rtn;
}

#define PROC_RESET_LOG "proc reset -"
static int linktool_cmd_proc_reset(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	unsigned short heartbeat_count;

	DEBUG(linktool_cmd_core_init_debug, "proc reset");

	/* read the stack size */
	// rdc_bcm_serdes_micro_core_stack_size(sa__) = bcm_serdes_acc_rde_field_u16(sa__,0xd22e,1,3,__ERR)
	LINKTOOL_PMI_FIELD_RD(PROC_RESET_LOG "stack size get", dev, 0xFF, 0, 0xD22E, 1, 3, &(cmd_obj->stack_size));
	DEBUG(linktool_cmd_core_init_debug, "proc reset (stack size = 0x%X)", cmd_obj->stack_size);

	/* read the heartbeat count */
	// rdc_bcm_serdes_heartbeat_count_1us(sa__) (bcm_serdes_acc_rde_field_u16(sa__,0xd104,4,4,__ERR))
	LINKTOOL_PMI_FIELD_RD(PROC_RESET_LOG "heartbeat get", dev, 0xFF, 0, 0xD104, 4, 4, &(heartbeat_count));

	rtn = linktool_cmd_core_rstb(cmd_obj);
	if (rtn != 0) {
		ERROR(PROC_RESET_LOG "core_rstb failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_core_lane_map(cmd_obj);
	if (rtn != 0) {
		ERROR(PROC_RESET_LOG "lame_map failed [%d]", rtn);
		goto out;
	}

	DEBUG(linktool_cmd_core_init_debug, "proc reset - set");
	rtn = linktool_cmd_uc_reset(cmd_obj, 1);
	if (rtn != 0) {
		ERROR(PROC_RESET_LOG "uc_reset 1 failed [%d]", rtn);
		goto out;
	}

	DEBUG(linktool_cmd_core_init_debug, "proc reset - clear");
	rtn = linktool_cmd_uc_reset(cmd_obj, 0);
	if (rtn != 0) {
		ERROR(PROC_RESET_LOG "uc_reset 0 failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_wait_active(cmd_obj);
	if (rtn != 0) {
		ERROR(PROC_RESET_LOG "wait_active failed [%d]", rtn);
		goto out;
	}

	/* write the heartbeat count */
	// wrc_bcm_serdes_heartbeat_count_1us(sa__, heart_beat) bcm_serdes_acc_mwr_reg(sa__,0xd104,0x0fff,0,wr_val)
	LINKTOOL_PMI_WR(PROC_RESET_LOG "heartbeat set - WR", dev, 0xFF, 0, 0xD104, heartbeat_count, 0x0FFF, 0);
	
	rtn = 0;

out:
	return rtn;
}

#define CLOCK_DEFAULTS_LOG "clock init defaults set -"
static int linktool_cmd_clock_init_defaults_set(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;

	DEBUG(linktool_cmd_core_init_debug, "clock init defaults set");

	// wrc_ams_pll_fracn_ndiv_int(0x3B)   = osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd11a,0x03ff,0,wr_val)
        LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fracn_ndiv_int",    dev, 0xFF, 0, 0xD11A, 0x3B, 0x03FF,  0);
	// wrc_ams_pll_fracn_ndiv_frac_l(0x0) = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd11b,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fracn_ndev_frac_1", dev, 0xFF, 0, 0xD11B,    0, 0xFFFF,  0);
	// wrc_ams_pll_fracn_ndiv_frac_h(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11c,0x0003,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "franc_ndiv_frac_h", dev, 0xFF, 0, 0xD11C,    0, 0x0003,  0);
	// wrc_ams_pll_refclk_freq2x_en(0x0)  = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd111,0x0080,7,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "franc_ndiv_frac_h", dev, 0xFF, 0, 0xD111,    0, 0x0080,  7);
	// wrc_ams_pll_refdiv2(0x0)           = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd111,0x0020,5,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "refdiv2",           dev, 0xFF, 0, 0xD111,    0, 0x0020,  5);
	// wrc_ams_pll_refdiv4(0x0)           = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd111,0x0040,6,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "refdiv4",           dev, 0xFF, 0, 0xD111,    0, 0x0040,  6);
	// wrc_ams_pll_div4_2_sel(0x0)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd327,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "div4_2_sel",        dev, 0xFF, 0, 0xD327,    0, 0x0002,  1);
	// wrc_ams_pll_pll_frac_mode(0x1)     = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x3000,12,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "pll_frac_mode",     dev, 0xFF, 0, 0xD119,    1, 0x3000, 12);
	// wrc_ams_pll_resetb_mmd(0x1)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "resetb_mmd",        dev, 0xFF, 0, 0xD119,    1, 0x0800, 11);
	// wrc_ams_pll_en_lb_res(0x0)         = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd320,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "en_lb_res",         dev, 0xFF, 0, 0xD320,    0, 0x0008,  3);
	// wrc_ams_pll_en_lb_ind(0x0)         = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd320,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "en_lb_ind",         dev, 0xFF, 0, 0xD320,    0, 0x0010,  4);
	// wrc_ams_pll_lb_sel_pll0(0x0)       = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x0400,10,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "lb_sel_pll0",       dev, 0xFF, 0, 0xD322,    0, 0x0400, 10);
	// wrc_ams_pll_rz_sel(0x4)            = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x0070,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "rz_sel",            dev, 0xFF, 0, 0xD321,    4, 0x0070,  4);
	// wrc_ams_pll_cp_calib_adj(0x7)      = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x7800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "cp_calib_adj",      dev, 0xFF, 0, 0xD321,    7, 0x7800, 11);
	// wrc_ams_pll_cap_l_pll0(0x0)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x03c0,6,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "cap_l_pll0",        dev, 0xFF, 0, 0xD322,    0, 0x03C0,  6);
	// wrc_ams_pll_cap_r_pll0(0x0)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd113,0x000f,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "cap_r_pll0",        dev, 0xFF, 0, 0xD113,    0, 0x000F,  0);
	// wrc_ams_pll_ictrl_pll0(0x2)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd323,0x0007,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "ictrl_pll0",        dev, 0xFF, 0, 0xD323,    2, 0x0007,  0);
	// wrc_ams_pll_fracn_enable(0x0)      = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fracn_enable",      dev, 0xFF, 0, 0xD119,    0, 0x0010,  4);
	// wrc_ams_pll_icp_sel(0xC)           = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11d,0x003c,2,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "icp_sel",           dev, 0xFF, 0, 0xD11D,  0xC, 0x003C,  2);
	// wrc_ams_pll_cp_sel(0x0)            = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x0180,7,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "cp_sel",            dev, 0xFF, 0, 0xD321,    0, 0x0180,  7);
	// wrc_ams_pll_fp3_r_sel(0x3)         = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd320,0x1800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fp3_r_sel",         dev, 0xFF, 0, 0xD320,    3, 0x1800, 11);
	// wrc_ams_pll_fp3_c_sel(0x0)         = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x000f,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fp3_c_sel",         dev, 0xFF, 0, 0xD321,    0, 0x000F,  0);
	// wrc_ams_pll_en_offset_fbck(0x0)    = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x0020,5,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "en_offset_fbck",    dev, 0xFF, 0, 0xD322,    0, 0x0020,  5);
	// wrc_ams_pll_en_offset_refclk(0x0)  = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "en_offset_refclk",  dev, 0xFF, 0, 0xD322,    0, 0x0010,  4);
	// wrc_ams_pll_fracn_offset_ctrl(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x000c,2,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "fracn_offset_ctrl", dev, 0xFF, 0, 0xD322,    0, 0x000C,  2);
	// wrc_ams_pll_pfd_pulse_adj(0x0)     = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x0003,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_DEFAULTS_LOG "pfd_pulse_adj",     dev, 0xFF, 0, 0xD322,    0, 0x0003,  0);

	rtn = 0;

out:
	return rtn;
}

#define CLOCK_CONFIG_LOG       "clock init config set -"
#define PLL_FRACN_NDIV_INT     85
#define PLL_FRACN_NDIV_FRAC_L  0
#define PLL_FRACN_NDIV_FRAC_H  0
static int linktool_cmd_clock_init_config_set(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	unsigned short reset_check;

	// FIXME: all PLL settings are hardcoded based on spefic speed for now

	// wrc_ams_pll_pwrdn(0x0)        = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd327,0x0004,2,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pwrdn",             dev, 0xFF, 0, 0xD327, 0, 0x0004,  2);
	// wrc_ams_pll_pwrdn_hbvco(0x1)  = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd110,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pwrdn_hbvco",       dev, 0xFF, 0, 0xD110, 1, 0x0010,  4);
	// wrc_ams_pll_pwrdn_lbvco(0x1)  = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd110,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pwrdn_lbvco",       dev, 0xFF, 0, 0xD110, 1, 0x0008,  3);
	// wrc_ams_pll_en_lb_res(0x1)    = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd320,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "en_lb_res",         dev, 0xFF, 0, 0xD320, 1, 0x0008,  3);
	// wrc_ams_pll_en_lb_ind(0x1)    = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd320,0x0010,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "en_lb_ind",         dev, 0xFF, 0, 0xD320, 1, 0x0010,  4);
	// wrc_ams_pll_pwrdn_lbvco(0x0)  = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd110,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pwrdn_lbvco(",      dev, 0xFF, 0, 0xD110, 0, 0x0008,  3);
	// wrc_ams_pll_cp_calib_adj(0x6) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x7800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "cp_calib_adj",      dev, 0xFF, 0, 0xD321, 6, 0x7800, 11);
	// wrc_ams_pll_cap_l_pll0(cap_pll) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd322,0x03c0,6,wr_val
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "cap_l_pll0",        dev, 0xFF, 0, 0xD322, 3, 0x03C0,  6);
	// wrc_ams_pll_cap_r_pll0(cap_pll) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd113,0x000f,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "cap_r_pll0",        dev, 0xFF, 0, 0xD113, 3, 0x000F,  0);
	// wrc_ams_pll_ictrl_pll0(ictrl) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd323,0x0007,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "ictrl_pll0",        dev, 0xFF, 0, 0xD323, 2, 0x0007,  0);

	// wrc_ams_pll_rz_sel(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd321,0x0070,4,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "rz_sel",            dev, 0xFF, 0, 0xD321, 1, 0x0070,  4);
	/* reset PLL mmd */
	// wrc_ams_pll_resetb_mmd(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "resetb_mmd",        dev, 0xFF, 0, 0xD119, 0, 0x0800, 11);
	/* release reset */
	// wrc_ams_pll_resetb_mmd(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "resetb_mmd",        dev, 0xFF, 0, 0xD119, 1, 0x0800, 11);

	/* assert low before programming fracn PLL div value */
	// wrc_ams_pll_i_ndiv_valid(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11c,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "i_ndiv_valid",      dev, 0xFF, 0, 0xD11C, 0, 0x0008,  3);

	/* MMD 8/9 mode (pll_frac_mode = 1) => [60 <= Ndiv < 512] */
	// wrc_ams_pll_pll_frac_mode(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x3000,12,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pll_frac_mode",     dev, 0xFF, 0, 0xD119, 1, 0x3000, 12);
	/* interger part of fracn PLL div */
	// wrc_ams_pll_fracn_ndiv_int(pll_fracn_ndiv_int) = osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd11a,0x03ff,0,wr_val)
        LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "fracn_ndiv_int",    dev, 0xFF, 0, 0xD11A, PLL_FRACN_NDIV_INT, 0x03FF,  0);
	/* set lower 16 bits of fractional part of fracn PLL div */
	// wrc_ams_pll_fracn_ndiv_frac_l(_ndiv_frac_l(pll_fracn_div)) = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd11b,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "fracn_ndiv_frac_1", dev, 0xFF, 0, 0xD11B, PLL_FRACN_NDIV_FRAC_L, 0xFFFF,  0);
	/* set upper  2 bits of fractional part of fracn PLL div */
	// wrc_ams_pll_fracn_ndiv_frac_h((uint8_t)(_ndiv_frac_h(pll_fracn_div))) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11c,0x0003,0,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "pll_frac_mode",     dev, 0xFF, 0, 0xD11C, PLL_FRACN_NDIV_FRAC_H, 0x0003, 0);

	// wrc_ams_pll_i_ndiv_valid(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11c,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "i_ndiv_valid",      dev, 0xFF, 0, 0xD11C, 1, 0x0008,  3);
	usleep(1);
	// wrc_ams_pll_i_ndiv_valid(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd11c,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "i_ndiv_valid",      dev, 0xFF, 0, 0xD11C, 0, 0x0008,  3);

	/* reset PLL mmd */
	// wrc_ams_pll_resetb_mmd(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "resetb_mmd",        dev, 0xFF, 0, 0xD119, 0, 0x0800, 11);
	// wrc_ams_pll_resetb_mmd(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd119,0x0800,11,wr_val)
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "resetb_mmd",        dev, 0xFF, 0, 0xD119, 1, 0x0800, 11);

	// osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd188,13,13,__ERR)
	LINKTOOL_PMI_FIELD_RD(CLOCK_CONFIG_LOG "uc reset check", dev, 0xFF, 0, 0xD188, 13, 13, &(reset_check));
	DEBUG(linktool_cmd_core_init_debug, CLOCK_CONFIG_LOG "uc reset check = 0x%X", reset_check);

	// EFUN(reg_wrc_CORE_PLL_COM_PLL_UC_CORE_CONFIG(struct_val.word)); = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd18d,wr_val)
	// FIXME: this is a hardcoded calculated value
	LINKTOOL_PMI_WR(CLOCK_CONFIG_LOG "core config", dev, 0xFF, 0, 0xD18D, 0xC1, 0xFFFF,  0);

	rtn = 0;

out:
	return rtn;
}

#define CLOCK_INIT_LOG "clock init -"
static int linktool_cmd_clock_init(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	unsigned short data16;

	DEBUG(linktool_cmd_core_init_debug, "clock init");

	/* refclk */
	// wrc_bcm_serdes_ams_pll_rtl_div_en(sa__, core_config->refclk_div_en);
	LINKTOOL_PMI_WR(CLOCK_INIT_LOG "refclk div", dev, 0xFF, 0, 0xD112, 0, 0x0060,  5);
	// wrc_bcm_serdes_ams_pll_i_en_ext_cml_refclk_in(sa__, core_config->pll0_en_ext_cml_refclk_in) = bcm_serdes_acc_mwr_reg_u8(sa__,0xd114,0x0400,10,wr_val)
        LINKTOOL_PMI_WR(CLOCK_INIT_LOG "refclk in",  dev, 0xFF, 0, 0xD114, 0, 0x0400, 10);
	// wrc_bcm_serdes_ams_pll_i_en_ext_cml_refclk_out(sa__, core_config->pll0_en_ext_cml_refclk_out) = bcm_serdes_acc_mwr_reg_u8(sa__,0xd114,0x0200,9,wr_val)
        LINKTOOL_PMI_WR(CLOCK_INIT_LOG "refclk out", dev, 0xFF, 0, 0xD114, 0, 0x0200,  9);
	// wrc_bcm_serdes_ams_pll_i_en_ext_cmos_refclk_in(sa__, core_config->pll0_en_ext_cmos_refclk_in) = bcm_serdes_acc_mwr_reg_u8(sa__,0xd114,0x0100,8,wr_val)
        LINKTOOL_PMI_WR(CLOCK_INIT_LOG "refclk in",  dev, 0xFF, 0, 0xD114, 0, 0x0100,  8);

	// blackhawk_sa_mem_rd(sa__, BH_PMI, 0xd114, &reg_d114);
	LINKTOOL_PMI_RD(CLOCK_INIT_LOG "clocks", dev, 0xFF, 0, 0xD114, &data16);

	/* set reset */
	// osprey7_v2l4p1_core_dp_reset(sa__,1) = wrc_core_dp_s_rstb(0x0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd184,0x2000,13,wr_val)
	LINKTOOL_PMI_WR(CLOCK_INIT_LOG "core reset off", dev, 0xFF, 0, 0xD184, 0, 0x2000, 13);

	/* defaults */
	rtn = linktool_cmd_clock_init_defaults_set(cmd_obj);
	if (rtn != 0) {
		ERROR(CLOCK_INIT_LOG "clock_init_defaults_set failed [%d]", rtn);
		goto out;
	}

	/* configure */
	rtn = linktool_cmd_clock_init_config_set(cmd_obj);
	if (rtn != 0) {
		ERROR(CLOCK_INIT_LOG "clock_init_config_set failed [%d]", rtn);
		goto out;
	}

	/* clear reset */
	// osprey7_v2l4p1_core_dp_reset(sa__,0) = wrc_core_dp_s_rstb(0x1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd184,0x2000,13,wr_val)
        LINKTOOL_PMI_WR(CLOCK_INIT_LOG "core reset off", dev, 0xFF, 0, 0xD184, 1, 0x2000, 13);

	usleep(5000);

	/* check PLL */
	// EFUN(osprey7_v2l4p1_INTERNAL_pll_lock_status(sa__,&pll_lock,&pll_lock_chg)); = exc_PLL_CAL_COM_STS_0__pll_lock
	// reg_rdc_PLL_CAL_COM_STS_0() = osprey7_v2l4p1_acc_rde_reg(sa__, 0xd148,__ERR)
	LINKTOOL_PMI_FIELD_RD(CLOCK_INIT_LOG "wait active status", dev, 0xFF, 0, 0xD148, 0, 0, &data16);
	DEBUG(linktool_cmd_core_init_debug, "active (status = 0x%X)", data16);
	if (((data16 >> 9) & 0x001) != 1) {
		ERROR(CLOCK_INIT_LOG "pll not locked");
		rtn = -1;
		goto out;
	}
	DEBUG(linktool_cmd_core_init_debug, "pll locked");

	rtn = 0;

out:
	return rtn;
}

#define CORE_INIT_LOG "core init -"
int linktool_cmd_core_init(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;

	DEBUG(linktool_cmd_core_init_debug, "core init");

	// FIXME: use static clock values for now
	cmd_obj->clock_info.refclk = 312500000;
	cmd_obj->clock_info.comclk = 156250000;

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_lane_reset(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "lane_reset failed [%d]", rtn);
		goto out;
	}

        /* heartbeat set */
        // wrc_bcm_serdes_heartbeat_count_1us() = bcm_serdes_acc_mwr_reg(sa__, 0xd104, 0x0fff, 0, wr_val)
        LINKTOOL_PMI_WR(CORE_INIT_LOG "heartbeat set - WR", cmd_obj->dev_addr, 0xFF, 0, 0xD104, (((cmd_obj->clock_info.comclk/1000)*4)/1000), 0x0FFF, 0);

	rtn = linktool_cmd_temperature_check(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "temperature_check failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_sbus_reset(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "sbus_reset failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_proc_reset(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "proc_reset failed [%d]", rtn);
		goto out;
	}

	/* temperature set */
	// blackhawk_sbus_wr(sa__, BH_TEMP_REG, temperature | 0x8000);
	DEBUG(linktool_cmd_core_init_debug, CORE_INIT_LOG "set temperature");
	LINKTOOL_SBUS_WR(CORE_INIT_LOG "set temperature", cmd_obj->dev_addr, LINKTOOL_TEMP_REG, cmd_obj->temperature);

	rtn = linktool_cmd_clock_init(cmd_obj);
	if (rtn != 0) {
		ERROR(CORE_INIT_LOG "clock_init failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(CORE_INIT_LOG "iface_close_fn failed [%d]", rtn);

	return rtn;
}
