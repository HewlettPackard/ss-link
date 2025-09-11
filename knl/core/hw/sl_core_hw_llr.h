/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_LLR_H_
#define _SL_CORE_HW_LLR_H_

#include "sl_core_llr.h"

struct work_struct;
struct sl_core_link;
struct sl_core_llr;

void sl_core_hw_llr_link_init(struct sl_core_link *core_link);

void sl_core_hw_llr_setup_cmd(struct sl_core_llr *core_llr,
			      sl_core_llr_setup_callback_t callback, void *tag, u32 flags);
void sl_core_hw_llr_setup_work(struct work_struct *work);
void sl_core_hw_llr_setup_loop_time_intr_work(struct work_struct *work);
void sl_core_hw_llr_setup_unexp_loop_time_intr_work(struct work_struct *work);
void sl_core_hw_llr_setup_timeout_work(struct work_struct *work);
void sl_core_hw_llr_settingup_cancel_cmd(struct sl_core_llr *core_llr);

void sl_core_hw_llr_start_cmd(struct sl_core_llr *core_llr,
			      sl_core_llr_start_callback_t callback, void *tag, u32 flags);
void sl_core_hw_llr_start_work(struct work_struct *work);
void sl_core_hw_llr_start_init_complete_intr_work(struct work_struct *work);
void sl_core_hw_llr_start_timeout_work(struct work_struct *work);
void sl_core_hw_llr_starting_cancel_cmd(struct sl_core_llr *core_llr);

void sl_core_hw_llr_stop(struct sl_core_llr *core_llr);
void sl_core_hw_llr_setup_stop_cmd(struct sl_core_llr *core_llr);
void sl_core_hw_llr_running_stop_cmd(struct sl_core_llr *core_llr);

int  sl_core_hw_llr_stop_wait(struct sl_core_llr *core_llr);

void sl_core_hw_llr_config_timeouts(struct sl_core_llr *core_llr);

#endif /* _SL_CORE_HW_LLR_H_ */
