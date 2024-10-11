/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_LANE_UP_RX_H_
#define _LINKTOOL_LANE_UP_RX_H_

struct linktool_cmd_obj;

void linktool_lane_up_rx_debug_set();

int  linktool_lane_up_rx(struct linktool_cmd_obj *cmd_obj);

#endif /* _LINKTOOL_LANE_UP_RX_H_ */
