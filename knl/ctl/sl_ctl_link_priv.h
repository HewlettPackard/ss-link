/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LINK_PRIV_H_
#define _SL_CTL_LINK_PRIV_H_

#include <linux/workqueue.h>

struct sl_ctl_link;
struct sl_link_config;
struct sl_link_policy;
struct sl_link_caps;

void sl_ctl_link_up_callback_work(struct work_struct *work);
int  sl_ctl_link_fault_callback(void *tag, u32 core_state, u32 core_cause, u64 core_imap);
int  sl_ctl_link_fault_intr_hdlr(u8 ldev_num, u8 lgrp_num, u8 link_num);
int  sl_ctl_link_an_lp_caps_get_callback(void *tag, struct sl_link_caps *caps, u32 result);
void sl_ctl_link_up_count_zero(struct sl_ctl_link *ctl_link);
void sl_ctl_link_up_clock_start(struct sl_ctl_link *ctl_link);
void sl_ctl_link_up_attempt_clock_start(struct sl_ctl_link *ctl_link);
int  sl_ctl_link_up_callback(void *tag, u32 state, u32 cause, u64 imap,
			     u32 speed, u32 fec_mode, u32 fec_type);
void sl_ctl_link_up_clock_clear(struct sl_ctl_link *ctl_link);
void sl_ctl_link_up_attempt_clock_clear(struct sl_ctl_link *ctl_link);
int  sl_ctl_link_down_callback(void *tag);
int  sl_ctl_link_async_down_callback(void *tag);
int  sl_ctl_link_async_down(struct sl_ctl_link *ctl_link);
void sl_ctl_link_config_get(struct sl_ctl_link *ctl_link, struct sl_link_config *link_config);
void sl_ctl_link_policy_get(struct sl_ctl_link *ctl_link, struct sl_link_policy *link_policy);

void sl_ctl_link_state_set(struct sl_ctl_link *ctl_link, u32 link_state);
u32  sl_ctl_link_state_get(struct sl_ctl_link *ctl_link);
int  sl_ctl_link_state_stopping_set(struct sl_ctl_link *ctl_link);

#endif /* _SL_CTL_LINK_PRIV_H_ */
