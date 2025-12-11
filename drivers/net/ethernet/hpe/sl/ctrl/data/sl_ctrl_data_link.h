/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_DATA_LINK_H_
#define _SL_CTRL_DATA_LINK_H_

#include "sl_ctrl_link.h"

int sl_ctrl_data_link_state_get(struct sl_ctrl_link *ctrl_link, u32 *link_state);

int sl_ctrl_data_link_fec_up_settle_wait_ms_get(struct sl_ctrl_link *ctrl_link, u32 *fec_up_settle_wait_ms);
int sl_ctrl_data_link_fec_up_check_wait_ms_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_check_wait_ms);
int sl_ctrl_data_link_fec_up_ucw_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_ucw_limit);
int sl_ctrl_data_link_fec_up_ccw_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_up_ccw_limit);

int sl_ctrl_data_link_policy_fec_mon_period_ms_get(struct sl_ctrl_link *ctrl_link, s32 *mon_period_ms);
int sl_ctrl_data_link_policy_fec_mon_ucw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ucw_warn_limit);
int sl_ctrl_data_link_policy_fec_mon_ucw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ucw_down_limit);
int sl_ctrl_data_link_policy_fec_mon_ccw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ccw_down_limit);
int sl_ctrl_data_link_policy_fec_mon_ccw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *mon_ccw_warn_limit);

int sl_ctrl_data_link_fec_mon_period_ms_get(struct sl_ctrl_link *ctrl_link, u32 *fec_mon_period_ms);
int sl_ctrl_data_link_fec_mon_ucw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_mon_ucw_down_limit);
int sl_ctrl_data_link_fec_mon_ucw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_mon_ucw_warn_limit);
int sl_ctrl_data_link_fec_mon_ccw_down_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_mon_ccw_down_limit);
int sl_ctrl_data_link_fec_mon_ccw_warn_limit_get(struct sl_ctrl_link *ctrl_link, s32 *fec_mon_ccw_warn_limit);

int sl_ctrl_data_link_fec_down_cache_ucw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ucw);
int sl_ctrl_data_link_fec_down_cache_ccw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ccw);
int sl_ctrl_data_link_fec_down_cache_gcw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *gcw);
int sl_ctrl_data_link_fec_down_cache_lane_cntr_get(struct sl_ctrl_link *ctrl_link, u8 lane_num, u64 *lane_cntr);
int sl_ctrl_data_link_fec_down_cache_tail_cntr_get(struct sl_ctrl_link *ctrl_link, u8 lane_num, u64 *tail_cntr);

int sl_ctrl_data_link_fec_up_cache_ucw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ucw);
int sl_ctrl_data_link_fec_up_cache_ccw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *ccw);
int sl_ctrl_data_link_fec_up_cache_gcw_cntr_get(struct sl_ctrl_link *ctrl_link, u64 *gcw);
int sl_ctrl_data_link_fec_up_cache_lane_cntr_get(struct sl_ctrl_link *ctrl_link, u8 lane_num, u64 *lane_cntr);
int sl_ctrl_data_link_fec_up_cache_tail_cntr_get(struct sl_ctrl_link *ctrl_link, u8 bin_num, u64 *tail_cntr);

#endif /* _SL_CTRL_DATA_LINK_H_ */
