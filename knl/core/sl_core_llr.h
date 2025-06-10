/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LLR_H_
#define _SL_CORE_LLR_H_

#include <linux/spinlock.h>
#include <linux/completion.h>

#include <linux/sl_llr.h>

#include "sl_core_lgrp.h"
#include "base/sl_core_timer_llr.h"
#include "base/sl_core_work_llr.h"
#include "hw/sl_core_hw_intr_llr.h"

struct sl_llr_data;
struct sl_llr_config;

struct work_struct;

#define SL_CORE_LLR_MAX_LOOP_TIME_COUNT 10

#define SL_CORE_LLR_DATA_MAGIC 0x736c4C52

enum sl_core_llr_state {
	SL_CORE_LLR_STATE_NEW              = 1,    /* next: CONFIGURED                */
	SL_CORE_LLR_STATE_CONFIGURED,              /* next: SETTING_UP                */
	SL_CORE_LLR_STATE_SETTING_UP,              /* -- BUSY --                      */
	SL_CORE_LLR_STATE_SETUP_TIMEOUT,           /* next: CONFIGURED                */
	SL_CORE_LLR_STATE_SETUP_CANCELING,         /* next: CONFIGURED                */
	SL_CORE_LLR_STATE_SETUP_STOPPING,          /* next: CONFIGURED                */
	SL_CORE_LLR_STATE_SETUP,                   /* next: STARTING | SETUP_STOPPING */
	SL_CORE_LLR_STATE_STARTING,                /* -- BUSY --                      */
	SL_CORE_LLR_STATE_START_TIMEOUT,           /* next: SETUP                     */
	SL_CORE_LLR_STATE_START_CANCELING,         /* next: SETUP                     */
	SL_CORE_LLR_STATE_RUNNING,                 /* next: STOPPING                  */
	SL_CORE_LLR_STATE_STOPPING,                /* next: SETUP                     */
};

#define SL_LLR_FAIL_CAUSE_NONE                   0
#define SL_LLR_FAIL_CAUSE_SETUP_CONFIG           BIT(0) /* llr setup config invalid             */
#define SL_LLR_FAIL_CAUSE_SETUP_INTR_ENABLE      BIT(1) /* llr setup interrupt enable failed    */
#define SL_LLR_FAIL_CAUSE_SETUP_TIMEOUT          BIT(2) /* llr setup timeout                    */
#define SL_LLR_FAIL_CAUSE_START_INTR_ENABLE      BIT(3) /* llr start interrupt enable failed    */
#define SL_LLR_FAIL_CAUSE_START_TIMEOUT          BIT(4) /* llr start timeout                    */
#define SL_LLR_FAIL_CAUSE_COMMAND                BIT(5) /* running llr was commanded to stop    */
#define SL_LLR_FAIL_CAUSE_CANCELED               BIT(6) /* llr operation was canceled by client */

#define SL_LLR_FAIL_CAUSE_STR_SIZE 128

typedef void (*sl_core_llr_setup_callback_t)(void *tag, u32 llr_state,
					    u64 info_map, struct sl_llr_data llr_data);
typedef void (*sl_core_llr_start_callback_t)(void *tag, u32 llr_state,
					    u64 info_map);

#define SL_CORE_LLR_MAGIC 0x756c5C4E
struct sl_core_llr {
	u32                              magic;
	u8                               num;

	struct sl_core_lgrp             *core_lgrp;

	spinlock_t                                 data_lock;
	u32                                        last_fail_cause;
	time64_t                                   last_fail_time;
	bool                                       is_canceled;
	u32                                        state;
	u64                                        info_map;
	void                                      *tag;
	u64                                        loop_time[SL_CORE_LLR_MAX_LOOP_TIME_COUNT];
	bool                                       is_data_valid;
	struct sl_llr_data                         data;
	struct {
		sl_core_llr_setup_callback_t       setup;
		sl_core_llr_start_callback_t       start;
	} callbacks;
	struct {
		u32                                setup;
		u32                                start;
	} flags;
	struct sl_llr_policy                       policy;
	struct {
		u8      size;
		u8      lossless_when_off;
		u8      link_down_behavior;
		u8      filter_ctl_frames;
		u8      ctl_frame_smac;
		u16     ctl_frame_ethertype;
		u8      retry_threshold;
		u8      allow_re_init;
		u8      replay_ct_max;
		u16     replay_timer_max;
		u16     max_cap_data;
		u16     max_cap_seq;
		u8      bytes_per_ns;
	} settings;

	struct work_struct                         work[SL_CORE_WORK_LLR_COUNT];
	struct sl_core_timer_llr_info              timers[SL_CORE_TIMER_LLR_COUNT];
	struct sl_core_hw_intr_llr_info            intrs[SL_CORE_HW_INTR_LLR_FLGS_COUNT];
	struct completion                          stop_complete;
};

int                 sl_core_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num);
void                sl_core_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num);
struct sl_core_llr *sl_core_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int  sl_core_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num,
			    struct sl_llr_config *llr_config);
int  sl_core_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num,
			    struct sl_llr_policy *llr_policy);
int  sl_core_llr_policy_get(u8 ldev_num, u8 lgrp_num, u8 llr_num,
			    struct sl_llr_policy *llr_policy);

int  sl_core_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num,
		       sl_core_llr_setup_callback_t callback, void *tag, u32 flags);
int  sl_core_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num,
		       sl_core_llr_start_callback_t callback, void *tag, u32 flags);
int  sl_core_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int  sl_core_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *llr_state);

struct sl_llr_data sl_core_llr_data_get(u8 ldev_num, u8 lgrp_num, u8 llr_num);

bool sl_core_llr_setup_should_stop(struct sl_core_llr *core_llr);
bool sl_core_llr_start_should_stop(struct sl_core_llr *core_llr);
bool sl_core_llr_should_stop(struct sl_core_llr *core_llr);

void sl_core_llr_last_fail_cause_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 llr_fail_cause);
void sl_core_llr_last_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *llr_fail_cause,
	time64_t *llr_fail_time);

const char *sl_core_llr_fail_cause_str(u32 llr_fail_cause);

#endif /* _SL_CORE_LLR_H_ */
