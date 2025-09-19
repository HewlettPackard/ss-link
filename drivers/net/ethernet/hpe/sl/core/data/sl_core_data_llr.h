/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_LLR_H_
#define _SL_CORE_DATA_LLR_H_

#include <linux/hpe/sl/sl_llr.h>

struct sl_core_llr;
struct sl_llr_config;

int                 sl_core_data_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num);
void                sl_core_data_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num);
struct sl_core_llr *sl_core_data_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int sl_core_data_llr_config_set(struct sl_core_llr *core_llr, struct sl_llr_config *llr_config);
int sl_core_data_llr_policy_set(struct sl_core_llr *core_llr, struct sl_llr_policy *llr_policy);
int sl_core_data_llr_policy_get(struct sl_core_llr *core_llr, struct sl_llr_policy *llr_policy);

int sl_core_data_llr_settings(struct sl_core_llr *core_llr);

void sl_core_data_llr_state_set(struct sl_core_llr *core_llr, u32 llr_state);
u32  sl_core_data_llr_state_get(struct sl_core_llr *core_llr);

void               sl_core_data_llr_data_set(struct sl_core_llr *core_llr,
					     struct sl_llr_data llr_data);
struct sl_llr_data sl_core_data_llr_data_get(struct sl_core_llr *core_llr);
void               sl_core_data_llr_data_clr(struct sl_core_llr *core_llr);
bool               sl_core_data_llr_data_is_valid(struct sl_core_llr *core_llr);

void sl_core_data_llr_info_map_clr(struct sl_core_llr *core_llr, u32 bit_num);
void sl_core_data_llr_info_map_set(struct sl_core_llr *core_llr, u32 bit_num);
u64  sl_core_data_llr_info_map_get(struct sl_core_llr *core_llr);

void sl_core_data_llr_last_fail_cause_set(struct sl_core_llr *core_llr, u32 llr_fail_cause);
void sl_core_data_llr_last_fail_cause_get(struct sl_core_llr *core_llr, u32 *llr_fail_cause, time64_t *llr_fail_time);

#endif /* _SL_CORE_DATA_LLR_H_ */
