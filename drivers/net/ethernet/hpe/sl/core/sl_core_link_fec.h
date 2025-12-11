/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LINK_FEC_H_
#define _SL_CORE_LINK_FEC_H_

#include <linux/hpe/sl/sl_fec.h>

struct sl_core_link;

#define SL_CORE_LINK_FEC_NUM_LANES    SL_CTRL_NUM_FEC_LANES
#define SL_CORE_LINK_FEC_NUM_CCW_BINS SL_CTRL_NUM_CCW_BINS

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

int sl_core_link_fec_ccw_get(struct sl_core_link *core_link, u64 *ccw);
int sl_core_link_fec_ucw_get(struct sl_core_link *core_link, u64 *ucw);
int sl_core_link_fec_gcw_get(struct sl_core_link *core_link, u64 *gcw);
int sl_core_link_fec_lane_cntr_get(struct sl_core_link *core_link, u8 lane_num, u64 *lane_cntr);
int sl_core_link_fec_tail_cntr_get(struct sl_core_link *core_link, u8 tail_cntr_num, u64 *tail_cntr);

int sl_core_link_fec_data_get(struct sl_core_link *core_link,
			      struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			      struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			      struct sl_core_link_fec_tail_cntrs *tail_cntrs);

#endif /* _SL_CORE_LINK_FEC_H_ */
