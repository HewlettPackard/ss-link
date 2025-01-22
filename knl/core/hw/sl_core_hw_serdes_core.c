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
#include "hw/sl_core_hw_serdes_lane.h"

#define LOG_NAME SL_CORE_SERDES_LOG_NAME

int sl_core_hw_serdes_core_sbus_reset(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u32 data32;
	u32 proc_state;
	u32 strapping;
	u16 stack_size;
	u32 misc_data;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "sbus reset");

	/* read SMB proc state */
	SL_CORE_HW_SBUS_RD(core_lgrp, 0xFD, 5, &proc_state);
	/* single step proc to halt it */
	SL_CORE_HW_SBUS_WR(core_lgrp, 0xFD, 5, ((proc_state & ~3) | 1));
	/* set target reg addr to RO */
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 0xFF, 0);
	/* soft reset */
	SL_CORE_HW_SBUS_RST(core_lgrp, core_lgrp->serdes.dt.dev_addr);
	/* check out of reset */
	SL_CORE_HW_SBUS_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, &data32);
	/* if not out of reset, then force it */
	if (((data32 & 3) == 0) || ((data32 & 6) == 2))
		SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, (data32 | 6));
	/* strapping */
	SL_CORE_HW_SBUS_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 33, &data32);
	if (data32 & (1 << 31))
		strapping = ((data32 >> 24) & 0x1F);
	else
		strapping = ((data32 >> 16) & 0x1F);
	/* stack size */
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 2, (0xD22E | (strapping << 27)));
	SL_CORE_HW_SBUS_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 4, &data32);
	stack_size = ((data32 >> 2) & 0x1FFF);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "stack size (sbus) = 0x%X", stack_size);
	/* read misc reg */
	SL_CORE_HW_SBUS_RD(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, &misc_data);

	/* POR */
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		48, 0x0, 1, 0x1); /* POR_H_RSTB_SBUS */
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		48, 0x1, 2, 0x1); /* POR_H_RSTB_GATE */
	usleep_range(1000, 2000);
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		48, 0x1, 1, 0x1); /* POR_H_RSTB_SBUS */

	/* restore stack size */
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 2, (0xD22E | (strapping << 27)));
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 3, ((stack_size << 18) | 0x8003));
	/* restore misc reg */
	SL_CORE_HW_SBUS_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr, 48, misc_data);
	/* restore SMB proc state */
	SL_CORE_HW_SBUS_WR(core_lgrp, 0xFD, 5, proc_state);

	/* re-gate POR */
	SL_CORE_HW_SBUS_FIELD_WR(core_lgrp, core_lgrp->serdes.dt.dev_addr,
		48, 0x3, 1, 0x3); /* POR_H_RSTB_GATE + POR_H_RSTB_SBUS */

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_uc_reset_set(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc reset set");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD23E, 0, 0, 0x0001); /* set micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD23E, 1, 0, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD217, 0, 0, 0x0001); /* disble CRC checking */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD22E, SL_HW_SERDES_FW_STACK_SIZE, 2, 0x7FFC); /* micro_core_stack_size */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD22E, 1, 15, 0x8000); /* stack enable */
	// FIXME: this is a RO bit
	// FIXME: SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD101, 0, 1, 0x0002);

	rtn = 0;
out:
	return rtn;
}

#define SL_HW_SERDES_WAIT_CRAM_INIT_TRIES 10
static int sl_core_hw_serdes_core_uc_reset_clr(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	int x;
	u16 data16;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc reset clr");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD23E, 1, 0, 0x0001); /* clear micro reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD200, 1, 0, 0x0001); /* enable micro subsystem clock */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD201, 1, 0, 0x0001); /* clear micro subsystem reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD227, 1, 0, 0x0001); /* enable access to code RAM */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD202, 2, 8, 0x0300); /* enable init code RAM */
	for (x = 0; x < SL_HW_SERDES_WAIT_CRAM_INIT_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD203, 15, 15, &data16); /* check init done */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "code RAM init D203 = 0x%X", data16);
		if (data16 != 0)
			break;
	}
	if (x >= SL_HW_SERDES_WAIT_CRAM_INIT_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "code RAM init timeout");
		rtn = -EIO;
		goto out;
	}
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD202, 0, 8, 0x0300); /* disable code RAM init */

	for (x = 0; x < core_lgrp->serdes.hw_info.num_micros; ++x) {
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x, 0xD240, 1, 0, 0x0001); /* enable micro clock */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, x, 0xD241, 1, 0, 0x0001); /* clear micro reset */
	}

	rtn = 0;
out:
	return rtn;
}

#define SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES 10
static int sl_core_hw_serdes_core_proc_reset(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u16 data16;
	u8  x;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "proc reset");

	rtn = sl_core_hw_serdes_core_uc_reset_set(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_reset_set failed [%d]", rtn);
		goto out;
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD19D, 1, 6, 0x0040); /* select common clock */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xFFDC, 0x1F, 0, 0x001F); /* broadcast port addr */

	for (x = 0; x < core_lgrp->serdes.hw_info.num_lanes; ++x)
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, x, 0, 0xD0B1, 0, 0, 0x0001); /* disable lane */

	usleep_range(10, 20);

	for (x = 0; x < core_lgrp->serdes.hw_info.num_plls; ++x)
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, x, 0xD184, 0, 13, 0x2000); /* PLL reset */

	usleep_range(10, 20);

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD101, 0, 0, 0x0001); /* set core reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0, 0, 0xD101, 1, 0, 0x0001); /* clear core reset */

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD182, 1, 1, 0x0002); /* clear pll0 reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 1, 0xD182, 1, 1, 0x0002); /* clear pll1 reset */

	rtn = sl_core_hw_serdes_core_uc_reset_set(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_reset_set failed [%d]", rtn);
		goto out;
	}

	rtn = sl_core_hw_serdes_core_uc_reset_clr(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "uc_reset_clr failed [%d]", rtn);
		goto out;
	}

	for (x = 0; x < SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES; ++x) {
		usleep_range(1000, 2000);
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD101, 14, 15, &data16); /* all micros init */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "micro init D101 = 0x%X", data16);
		if (data16 != 1) /* initialized */
			continue;
		SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD21A, 0, 0, &data16); /* micro status */
		sl_core_log_dbg(core_lgrp, LOG_NAME, "micro status D21A = 0x%X", data16);
		if ((data16 & 0x3) != 0x3) /* micro 0 and 1 active */
			continue;
		break;
	}
	if (x >= SL_HW_SERDES_WAIT_UC_ACTIVE_TRIES) {
		sl_core_log_err(core_lgrp, LOG_NAME, "uc active timeout");
		rtn = -EIO;
		goto out;
	}
	sl_core_log_dbg(core_lgrp, LOG_NAME, "uc active");

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_clock_init_defaults_set(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "defaults set");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11A, 0x3B,  0, 0x03FF);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11B,  0x0,  0, 0xFFFF);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C,  0x0,  0, 0x0003);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD111,  0x0,  7, 0x0080);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD111,  0x0,  5, 0x0020);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD111,  0x0,  6, 0x0040);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD327,  0x0,  1, 0x0002);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x1, 12, 0x3000);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x1, 11, 0x0800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD320,  0x0,  3, 0x0008);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD320,  0x0,  4, 0x0010);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0, 10, 0x0400);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x4,  4, 0x0070);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x7, 11, 0x7800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0,  6, 0x03C0);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD113,  0x0,  0, 0x000F);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD323,  0x2,  0, 0x0007);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x0,  4, 0x0010);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11D,  0xC,  2, 0x003C);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x0,  7, 0x0180);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD320,  0x3, 11, 0x1800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x0,  0, 0x000F);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0,  5, 0x0020);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0,  4, 0x0010);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0,  2, 0x000C);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x0,  0, 0x0003);

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_clock_init_config_set(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "config set");

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD327,  0x0,  2, 0x0004);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD110,  0x1,  4, 0x0010);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD110,  0x1,  3, 0x0008);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD320,  0x1,  3, 0x0008);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD320,  0x1,  4, 0x0104);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD110,  0x0,  3, 0x0008);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x6, 11, 0x7800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD322,  0x3,  6, 0x03C0);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD113,  0x3,  0, 0x000F);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD323,  0x2,  0, 0x0007);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD321,  0x1,  4, 0x0070);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD18D, 0xC1,  0, 0xFFFF);

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_clock_init(struct sl_core_lgrp *core_lgrp)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "clock init");

	/* refclk */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD112, 0,  5, 0x0060);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD114, 0, 10, 0x0400);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD114, 0,  9, 0x0200);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD114, 0,  8, 0x0100);

	/* set reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD184, 0, 13, 0x2000); /* active low */

	/* defaults set */
	rtn = sl_core_hw_serdes_core_clock_init_defaults_set(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "defaults_set failed [%d]", rtn);
		goto out;
	}

	/* config set */
	rtn = sl_core_hw_serdes_core_clock_init_config_set(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "config_set failed [%d]", rtn);
		goto out;
	}

	/* clear reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD184, 1, 13, 0x2000); /* active low */

	usleep_range(5000, 10000);

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_lgrp_reset(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u8  lane_num;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "reset");

	for (lane_num = 0; lane_num < core_lgrp->serdes.hw_info.num_lanes; ++lane_num) {
		/* TX */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1D1, 0, 0, 0x0001);
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1DE, 0, 0, 0x0001);
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1DE, 1, 0, 0x0001);
		/* RX */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1C1, 0, 0, 0x0001);
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1CE, 0, 0, 0x0001);
		usleep_range(10, 20);
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, lane_num, 0, 0xD1CE, 1, 0, 0x0001);
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

	if (core_lgrp->serdes.is_core_init)
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

	core_lgrp->serdes.is_core_init = true;

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_pll_set(struct sl_core_lgrp *core_lgrp, u32 clocking)
{
	int rtn;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll set");

	/* reset PLL MMD */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x0, 11, 0x0800);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x1, 11, 0x0800);

	/* assert low before programming fraction PLL div value */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C,  0x0,  3, 0x0008);
	/* MMD 8/9 mode */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119,  0x1, 12, 0x3000);

	if (clocking == SL_CORE_HW_SERDES_CLOCKING_82P5) {
		/* integer part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11A, 82, 0, 0x03FF);
		/* set lower 16 bits of fractional part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11B, 0, 0, 0xFFFF);
		/* set upper 2 bits of fractional part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C, 2, 0, 0x0003);
	} else {
		/* integer part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11A, 85, 0, 0x03FF);
		/* set lower 16 bits of fractional part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11B, 0, 0, 0xFFFF);
		/* set upper 2 bits of fractional part of fraction PLL div */
		SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C, 0, 0, 0x0003);
	}

	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C, 0x1, 3, 0x0008);
	usleep_range(10, 20);
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD11C, 0x0, 3, 0x0008);

	/* reset PLL MMD */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119, 0x0, 11, 0x0800); /* active low */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD119, 0x1, 11, 0x0800); /* active low */

	rtn = 0;
out:
	return rtn;
}

static int sl_core_hw_serdes_core_pll_check(struct sl_core_lgrp *core_lgrp)
{
	int rtn;
	u16 data16;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll check");

	SL_CORE_HW_PMI_RD(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD148, 0, 0, &data16);
	sl_core_log_dbg(core_lgrp, LOG_NAME, "D148 = 0x%X", data16);
	if (((data16 >> 9) & 0x001) != 1) {
		sl_core_log_err(core_lgrp, LOG_NAME, "pll failed to lock");
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
	int rtn;

	if (!SL_PLATFORM_IS_HARDWARE(core_lgrp->core_ldev))
		return 0;

	if (core_lgrp->serdes.is_pll_locked && (core_lgrp->serdes.clocking == clocking)) {
		sl_core_log_dbg(core_lgrp, LOG_NAME, "pll already locked and clocking already set");
		return 0;
	}

	sl_core_log_dbg(core_lgrp, LOG_NAME, "pll");

	/* set reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD184, 0, 13, 0x2000); /* active low */

	/* PLL set */
	rtn = sl_core_hw_serdes_core_pll_set(core_lgrp, clocking);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_pll_set failed [%d]", rtn);
		goto out;
	}

	/* clear reset */
	SL_CORE_HW_PMI_WR(core_lgrp, core_lgrp->serdes.dt.dev_id, 0xFF, 0, 0xD184, 1, 13, 0x2000); /* active low */

	usleep_range(5000, 10000);

	/* PLL check */
	rtn = sl_core_hw_serdes_core_pll_check(core_lgrp);
	if (rtn) {
		sl_core_log_err_trace(core_lgrp, LOG_NAME, "core_pll_check failed [%d]", rtn);
		goto out;
	}

	core_lgrp->serdes.clocking      = clocking;
	core_lgrp->serdes.is_pll_locked = true;

	rtn = 0;
out:
	return rtn;
}
