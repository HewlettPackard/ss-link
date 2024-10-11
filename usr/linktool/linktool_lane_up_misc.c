// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_cmds.h"
#include "linktool_lane_up_misc.h"

static unsigned int linktool_lane_up_misc_debug = 0;

void linktool_lane_up_misc_debug_set()
{
	linktool_lane_up_misc_debug = 1;
}

#define IS_READY_LOG "is_ready -"
int linktool_lane_up_is_ready(unsigned int dev, unsigned char lane, unsigned int *is_rx_ready, unsigned int *is_tx_ready)
{
	int            rtn;
	unsigned short rx_power_state;
	unsigned short tx_power_state;
	unsigned short rx_dp_state;
	unsigned short tx_dp_state;

	DEBUG(linktool_lane_up_misc_debug, "is_ready");

	// rd_bcm_serdes_ln_rx_s_pwrdn(sa__) (bcm_serdes_acc_rde_field_u8(sa__,0xd1a1,15,15,__ERR))
	LINKTOOL_PMI_FIELD_RD(IS_READY_LOG, dev, lane, 0, 0xD1A1, 15, 15, &rx_power_state);
	// rd_bcm_serdes_ln_tx_s_pwrdn(sa__) (bcm_serdes_acc_rde_field_u8(sa__,0xd1b1,15,15,__ERR))
	LINKTOOL_PMI_FIELD_RD(IS_READY_LOG, dev, lane, 0, 0xD1B1, 15, 15, &tx_power_state);
	DEBUG(linktool_lane_up_misc_debug, "%s lane = %u, rx_power_state = %u, tx_power_state = %u",
		IS_READY_LOG, lane, rx_power_state, tx_power_state);

	// blackhawk_sbus_rd_rx_lane_field(sa__, BH_RX_CTL_REG, 6, 1, (uint *)rx) = blackhawk_sbus_rd_field(sa__, reg + 8 * physical_rx_lane, start, mask, value)
	// FIXME: need better physical lane calc
	LINKTOOL_SBUS_FIELD_RD(IS_READY_LOG, dev, (LINKTOOL_RX_CTL_REG + (8 * lane)), 6, 0x1, &rx_dp_state);
	// blackhawk_sbus_rd_tx_lane_field(sa__, BH_TX_CTL_REG, 6, 1, (uint *)tx) = blackhawk_sbus_rd_field(sa__, reg + 8 * physical_tx_lane, start, mask, value)
	// FIXME: need better physical lane calc
	LINKTOOL_SBUS_FIELD_RD(IS_READY_LOG, dev, (LINKTOOL_TX_CTL_REG + (8 * lane)), 6, 0x1, &tx_dp_state);
	DEBUG(linktool_lane_up_misc_debug, "%s lane = %d, rx_dp_state = %u, tx_dp_state = %u",
		IS_READY_LOG, lane, rx_dp_state, tx_dp_state);

	// READY = power state is clear and reset state is clear
	*is_rx_ready = ((rx_power_state != 0) || (rx_dp_state != 0));
	*is_tx_ready = ((tx_power_state != 0) || (tx_dp_state != 0));

	DEBUG(linktool_lane_up_misc_debug, "%s lane = %d, is_rx_ready = %d, is_tx_ready = %d",
		IS_READY_LOG, lane, *is_rx_ready, *is_tx_ready);

	rtn = 0;
out:
	return rtn;
}
