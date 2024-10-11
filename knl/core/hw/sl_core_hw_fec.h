/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_FEC_H_
#define _SL_CORE_HW_FEC_H_

struct sl_core_link;
struct sl_core_link_fec_cw_cntrs;
struct sl_core_link_fec_lane_cntrs;
struct sl_core_link_fec_tail_cntrs;

int sl_core_hw_fec_cw_cntrs_get(struct sl_core_link *core_link,
				 struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_core_hw_fec_lane_cntrs_get(struct sl_core_link *core_link,
				   struct sl_core_link_fec_lane_cntrs *lane_cntrs);
int sl_core_hw_fec_tail_cntrs_get(struct sl_core_link *core_link,
				   struct sl_core_link_fec_tail_cntrs *tail_cntrs);
int sl_core_hw_fec_data_get(struct sl_core_link *core_link,
			    struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			    struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			    struct sl_core_link_fec_tail_cntrs *tail_cntrs);

#endif /* _SL_CORE_HW_FEC_H_ */
