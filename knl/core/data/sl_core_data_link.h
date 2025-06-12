/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_LINK_H_
#define _SL_CORE_DATA_LINK_H_

struct sl_core_link;
struct sl_core_link_config;
struct time64_t;

int		     sl_core_data_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num);
void		     sl_core_data_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num);
struct sl_core_link *sl_core_data_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num);

void sl_core_data_link_config_set(struct sl_core_link *core_link,
				  struct sl_core_link_config *link_config);
int  sl_core_data_link_settings(struct sl_core_link *core_link);
void sl_core_data_link_timeouts(struct sl_core_link *core_link);

void sl_core_data_link_state_set(struct sl_core_link *core_link, u32 link_state);
u32  sl_core_data_link_state_get(struct sl_core_link *core_link);

u32  sl_core_data_link_speed_get(struct sl_core_link *core_link);
u16  sl_core_data_link_clocking_get(struct sl_core_link *core_link);

void sl_core_data_link_info_map_clr(struct sl_core_link *core_link, u32 bit_num);
void sl_core_data_link_info_map_set(struct sl_core_link *core_link, u32 bit_num);
u64  sl_core_data_link_info_map_get(struct sl_core_link *core_link);

void sl_core_data_link_last_up_fail_cause_map_set(struct sl_core_link *core_link, u64 up_fail_cause_map);
u64  sl_core_data_link_last_up_fail_cause_map_get(struct sl_core_link *core_link);
void sl_core_data_link_last_up_fail_info_get(struct sl_core_link *core_link, u64 *up_fail_cause_map,
	time64_t *up_fail_time);

void     sl_core_data_link_is_last_down_new_set(struct sl_core_link *core_link, bool is_last_down_new);
void     sl_core_data_link_last_down_cause_map_set(struct sl_core_link *core_link, u64 down_cause_map);
void     sl_core_data_link_last_down_cause_map_info_get(struct sl_core_link *core_link, u64 *down_cause_map,
							time64_t *down_time);
u64      sl_core_data_link_last_down_cause_map_get(struct sl_core_link *core_link);
time64_t sl_core_data_link_last_down_time_get(struct sl_core_link *core_link);

void sl_core_data_link_ccw_warn_limit_crossed_set(struct sl_core_link *core_link, bool is_limit_crossed);
void sl_core_data_link_ccw_warn_limit_crossed_get(struct sl_core_link *core_link, bool *is_limit_crossed,
						  time64_t *limit_crossed_time);
void sl_core_data_link_ucw_warn_limit_crossed_set(struct sl_core_link *core_link, bool is_limit_crossed);
void sl_core_data_link_ucw_warn_limit_crossed_get(struct sl_core_link *core_link, bool *is_limit_crossed,
						  time64_t *limit_crossed_time);

u32 sl_core_data_link_fec_mode_get(struct sl_core_link *core_link);
u32 sl_core_data_link_fec_type_get(struct sl_core_link *core_link);

u32 sl_core_data_link_an_lp_caps_state_get(struct sl_core_link *core_link);
void sl_core_data_link_an_lp_caps_state_set(struct sl_core_link *core_link, u32 lp_caps_state);

void sl_core_data_link_an_fail_cause_get(struct sl_core_link *core_link, u32 *fail_cause, time64_t *fail_time);
void sl_core_data_link_an_fail_cause_set(struct sl_core_link *core_link, u32 fail_cause);

u32 sl_core_data_link_an_retry_count_get(struct sl_core_link *core_link);

#endif /* _SL_CORE_DATA_LINK_H_ */
