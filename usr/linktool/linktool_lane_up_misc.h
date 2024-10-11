/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_LANE_UP_MISC_H_
#define _LINKTOOL_LANE_UP_MISC_H_

#define LINKTOOL_RX_CTL_REG       67
#define LINKTOOL_TX_CTL_REG       64

void linktool_lane_up_misc_debug_set();

int linktool_lane_up_is_ready(unsigned int dev, unsigned char lane,
			      unsigned int *is_rx_ready, unsigned int *is_tx_ready);

#endif /* _LINKTOOL_UP_MISC_H_ */
