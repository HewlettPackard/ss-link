/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LINK_FEC_H_
#define _SL_CTRL_LINK_FEC_H_

struct sl_ber;
struct sl_fec_info;
struct sl_fec_tail;
struct sl_core_link_fec_cw_cntrs;
struct sl_core_link_fec_lane_cntrs;
struct sl_core_link_fec_tail_cntrs;
struct sl_ctrl_link;

enum {
	SL_CTRL_LINK_FEC_MON_OFF,
	SL_CTRL_LINK_FEC_MON_ON
};

int sl_ctrl_link_fec_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      struct sl_fec_info *fec_info);
int sl_ctrl_link_fec_tail_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      struct sl_fec_tail *fec_tail);
int sl_ctrl_link_fec_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			      struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			      struct sl_core_link_fec_tail_cntrs *tail_cntrs);
int sl_ctrl_link_fec_mon_state_get(struct sl_ctrl_link *ctrl_link, u32 *state);
const char *sl_ctrl_link_fec_mon_state_str(u32 state);
int sl_ctrl_link_fec_ber_calc(struct sl_fec_info *fec_info,
			      struct sl_ber *ucw_ber, struct sl_ber *ccw_ber);

#endif /* _SL_CTRL_LINK_FEC_H_ */
