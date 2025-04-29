/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_LINK_H_
#define _SL_CORE_HW_LINK_H_

struct work_struct;

#include "sl_core_link.h"

void sl_core_hw_link_up_callback(struct sl_core_link *core_link,
	struct sl_core_link_up_info *core_link_up_info);
void sl_core_hw_link_up_cmd(struct sl_core_link *link,
	sl_core_link_up_callback_t callback, void *tag);
void sl_core_hw_link_up_start_work(struct work_struct *work);
void sl_core_hw_link_up_after_an_start(struct sl_core_link *core_link);
void sl_core_hw_link_up_work(struct work_struct *work);
void sl_core_hw_link_up_intr_work(struct work_struct *work);
void sl_core_hw_link_up_check_work(struct work_struct *work);
void sl_core_hw_link_up_fec_settle_work(struct work_struct *work);
void sl_core_hw_link_up_fec_check_work(struct work_struct *work);
void sl_core_hw_link_up_timeout_work(struct work_struct *work);
void sl_core_hw_link_up_cancel_work(struct work_struct *work);
void sl_core_hw_link_up_fail_work(struct work_struct *work);

void sl_core_hw_link_down_work(struct work_struct *work);

void sl_core_hw_link_high_ser_intr_work(struct work_struct *work);
void sl_core_hw_link_llr_max_starvation_intr_work(struct work_struct *work);
void sl_core_hw_link_llr_starved_intr_work(struct work_struct *work);
void sl_core_hw_link_fault_intr_work(struct work_struct *work);

#endif /* _SL_CORE_HW_LINK_H_ */
