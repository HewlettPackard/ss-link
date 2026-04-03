/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022-2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_PCS_H_
#define _SL_CORE_HW_PCS_H_

struct sl_core_link;

void sl_core_hw_pcs_config(struct sl_core_link *link);
void sl_core_hw_pcs_config_swizzles(struct sl_core_link *core_link);

void sl_core_hw_pcs_tx_start(struct sl_core_link *link);
void sl_core_hw_pcs_rx_start(struct sl_core_link *link);
void sl_core_hw_pcs_stop(struct sl_core_link *link);
void sl_core_hw_pcs_toggle(struct sl_core_link *core_link);

bool sl_core_hw_pcs_is_pml_rec_success(struct sl_core_link *core_link);

bool sl_core_hw_pcs_is_ok(struct sl_core_link *link);

int  sl_core_hw_pcs_enable_auto_lane_degrade(struct sl_core_link *core_link);

void sl_core_hw_pcs_keep_all_lanes_active_set(struct sl_core_link *core_link, u64 value);

#endif /* _SL_CORE_HW_PCS_H_ */
