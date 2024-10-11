/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LINK_FEC_H_
#define _SL_CORE_LINK_FEC_H_

#include <linux/sl_fec.h>

#define SL_CORE_LINK_FEC_NUM_LANES    SL_CTL_NUM_FEC_LANES
#define SL_CORE_LINK_FEC_NUM_CCW_BINS SL_CTL_NUM_CCW_BINS

struct sl_core_link_fec_cw_cntrs {
	u64 ccw;
	u64 ucw;
	u64 gcw;
};

struct sl_core_link_fec_lane_cntrs {
	u64 lanes[SL_CORE_LINK_FEC_NUM_LANES];
};

struct sl_core_link_fec_tail_cntrs {
	u64 ccw_bins[SL_CORE_LINK_FEC_NUM_CCW_BINS];
};

struct sl_core_link_fec_data_cntrs {
	struct sl_core_link_fec_cw_cntrs cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
};

int sl_core_link_fec_cw_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				  struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_core_link_fec_lane_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				    struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_core_link_fec_tail_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
				    struct sl_core_link_fec_tail_cntrs *tail_cntrs);

int sl_core_link_fec_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	struct sl_core_link_fec_cw_cntrs *cw_cntrs,
	struct sl_core_link_fec_lane_cntrs *lane_cntrs,
	struct sl_core_link_fec_tail_cntrs *tail_cntrs);

#endif /* _SL_CORE_LINK_FEC_H_ */
