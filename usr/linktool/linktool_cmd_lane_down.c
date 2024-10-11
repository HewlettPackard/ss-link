// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

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

#define LINKTOOL_TX_CTL_REG       64
#define LINKTOOL_RX_CTL_REG       67

static unsigned int linktool_cmd_lane_down_debug = 0;

void linktool_cmd_lane_down_debug_set()
{
	linktool_cmd_lane_down_debug = 1;
}

#define LANE_DOWN_LOG "lane_down -"
int linktool_cmd_lane_down(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	unsigned int dev = cmd_obj->dev_addr;
	unsigned int lane_map = cmd_obj->link_config.lane_map;
	unsigned int lane_num;

	DEBUG(linktool_cmd_lane_down_debug, "cmd_lane_down");

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(LANE_DOWN_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	for (lane_num = 0; lane_num < LINKTOOL_NUM_LANES; ++lane_num) {

		if (((lane_map >> lane_num) & 1) == 0)
			continue;

		/* TX */
		// wr_tx_ln_dp_s_rstb(0x0) => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1d1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(LANE_DOWN_LOG "wr tx ln dp s rstb", dev, lane_num, 0, 0xD1D1, 0, 0x0001, 0);
		// blackhawk_sbus_wr_tx_lane_field(sa__, BH_TX_CTL_REG, 6, 1, 0)
		// FIXME: need use physical lane as part of this calc
		LINKTOOL_SBUS_FIELD_WR(LANE_DOWN_LOG "tx ctl", dev, (LINKTOOL_TX_CTL_REG + (8 * lane_num)), 6, 0x1, 0x0);
		// wr_ln_tx_s_pwrdn(0x1) => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1a1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(LANE_DOWN_LOG "wr ln tx s pwrdn", dev, lane_num, 0, 0xD1A1, 1, 0x0001, 0);

		/* RX */
		// wr_rx_ln_dp_s_rstb(0x0) => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1c1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(LANE_DOWN_LOG "wr rx ln dp s rstb", dev, lane_num, 0, 0xD1C1, 0, 0x0001, 0);
		// blackhawk_sbus_wr_rx_lane_field(sa__, BH_RX_CTL_REG, 6, 1, 0)
		// FIXME: need use physical lane as part of this calc
		LINKTOOL_SBUS_FIELD_WR(LANE_DOWN_LOG "rx ctl", dev, (LINKTOOL_RX_CTL_REG + (8 * lane_num)), 6, 0x1, 0x0);
		// wr_ln_rx_s_pwrdn(0x1) => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd1b1,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(LANE_DOWN_LOG "wr ln rx s pwrdn", dev, lane_num, 0, 0xD1B1, 1, 0x0001, 0);
	}

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(LANE_DOWN_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}
