// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "uapi/sl_media.h"
#include "uapi/sl_lgrp.h"
#include "base/sl_core_log.h"
#include "sl_core_lgrp.h"
#include "sl_core_llr.h"
#include "sl_core_str.h"
#include "hw/sl_core_hw_llr.h"
#include "hw/sl_core_hw_intr_llr.h"
#include "hw/sl_core_hw_intr_flgs_llr.h"
#include "data/sl_core_data_llr.h"

static struct sl_core_llr *core_llrs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(core_llrs_lock);

#define LOG_NAME SL_CORE_DATA_LLR_LOG_NAME

#define SL_CORE_TIMER_LLR_INIT(_llr, _timer_num, _work_num, _log)        \
	do {                                                             \
		(_llr)->timers[_timer_num].data.core_llr   = _llr;       \
		(_llr)->timers[_timer_num].data.timer_num  = _timer_num; \
		(_llr)->timers[_timer_num].data.work_num   = _work_num;  \
		strncpy((_llr)->timers[_timer_num].data.log,             \
			_log, SL_CORE_TIMER_LLR_LOG_SIZE);               \
	} while (0)
#define SL_CORE_INTR_LLR_INIT(_llr, _intr_num, _work_num, _log)           \
	do {                                                              \
		(_llr)->intrs[_intr_num].flgs =                           \
			sl_core_hw_intr_flgs_llr[_intr_num][(_llr)->num]; \
		(_llr)->intrs[_intr_num].data.core_llr = _llr;            \
		(_llr)->intrs[_intr_num].data.intr_num = _intr_num;       \
		(_llr)->intrs[_intr_num].data.work_num = _work_num;       \
		strncpy((_llr)->intrs[_intr_num].data.log,                \
			_log, SL_CORE_HW_INTR_LLR_LOG_SIZE);              \
	} while (0)

int sl_core_data_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_data_llr_get(ldev_num, lgrp_num, llr_num);
	if (core_llr) {
		sl_core_log_err(core_llr, LOG_NAME, "exists (llr = 0x%p)", core_llr);
		return -EBADRQC;
	}

	core_llr = kzalloc(sizeof(struct sl_core_llr), GFP_KERNEL);
	if (!core_llr)
		return -ENOMEM;

	core_llr->magic     = SL_CORE_LLR_MAGIC;
	core_llr->num       = llr_num;
	core_llr->core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&core_llr->data_lock);

	sl_core_hw_llr_stop(core_llr);
	sl_core_data_llr_state_set(core_llr, SL_CORE_LLR_STATE_OFF);

	core_llr->data_cache = KMEM_CACHE(sl_llr_data, 0);
	if (core_llr->data_cache == NULL)
		return -ENOMEM;

	timer_setup(&(core_llr->timers[SL_CORE_TIMER_LLR_SETUP].timer),
		sl_core_timer_llr_timeout, 0);
	SL_CORE_TIMER_LLR_INIT(core_llr, SL_CORE_TIMER_LLR_SETUP,
		SL_CORE_WORK_LLR_SETUP_TIMEOUT, "llr setup");
	timer_setup(&(core_llr->timers[SL_CORE_TIMER_LLR_START].timer),
		sl_core_timer_llr_timeout, 0);
	SL_CORE_TIMER_LLR_INIT(core_llr, SL_CORE_TIMER_LLR_START,
		SL_CORE_WORK_LLR_START_TIMEOUT, "llr start");

	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_SETUP]),
		sl_core_hw_llr_setup_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_REUSE_TIMING]),
		sl_core_hw_llr_setup_reuse_timing_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_TIMEOUT]),
		sl_core_hw_llr_setup_timeout_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR]),
		sl_core_hw_llr_setup_unexp_loop_time_intr_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR]),
		sl_core_hw_llr_setup_loop_time_intr_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_START]),
		sl_core_hw_llr_start_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_START_TIMEOUT]),
		sl_core_hw_llr_start_timeout_work);
	INIT_WORK(&(core_llr->work[SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR]),
		sl_core_hw_llr_start_init_complete_intr_work);

	SL_CORE_INTR_LLR_INIT(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME,
		SL_CORE_WORK_LLR_SETUP_UNEXP_LOOP_TIME_INTR, "llr setup unexp loop time");
	SL_CORE_INTR_LLR_INIT(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME,
		SL_CORE_WORK_LLR_SETUP_LOOP_TIME_INTR, "llr setup loop time");
	SL_CORE_INTR_LLR_INIT(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE,
		SL_CORE_WORK_LLR_START_INIT_COMPLETE_INTR, "llr start init complete");

	rtn = sl_core_hw_intr_llr_hdlr_register(core_llr);
	if (rtn != 0) {
		sl_core_log_err_trace(core_llr, LOG_NAME,
			"core_hw_intr_llr_hdlr_register failed [%d]", rtn);
		kfree(core_llr);
		return rtn;
	}

	sl_core_log_dbg(core_llr, LOG_NAME, "new (llr = 0x%p)", core_llr);

	spin_lock(&core_llrs_lock);
	core_llrs[ldev_num][lgrp_num][llr_num] = core_llr;
	spin_unlock(&core_llrs_lock);

	return 0;
}

void sl_core_data_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_core_llr *core_llr;
	int                 x;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);
	if (!core_llr) {
		sl_core_log_dbg(NULL, LOG_NAME, "not found (llr_num = %u)", llr_num);
		return;
	}

	sl_core_hw_llr_stop(core_llr);

	spin_lock(&core_llrs_lock);
	core_llrs[ldev_num][lgrp_num][llr_num] = NULL;
	spin_unlock(&core_llrs_lock);

	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_UNEXP_LOOP_TIME);
	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_SETUP_LOOP_TIME);
	sl_core_hw_intr_llr_flgs_disable(core_llr, SL_CORE_HW_INTR_LLR_START_INIT_COMPLETE);
	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_SETUP);
	sl_core_timer_llr_end(core_llr, SL_CORE_TIMER_LLR_START);

	for (x = 0; x < SL_CORE_WORK_LLR_COUNT; ++x)
		cancel_work_sync(&(core_llr->work[x]));

	sl_core_hw_intr_llr_hdlr_unregister(core_llr);

	kmem_cache_destroy(core_llr->data_cache);
	core_llr->data_cache = NULL;

	sl_core_log_dbg(core_llr, LOG_NAME, "del (llr = 0x%p)", core_llr);

	kfree(core_llr);
}

struct sl_core_llr *sl_core_data_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	unsigned long       irq_flags;
	struct sl_core_llr *core_llr;

	spin_lock_irqsave(&core_llrs_lock, irq_flags);
	core_llr = core_llrs[ldev_num][lgrp_num][llr_num];
	spin_unlock_irqrestore(&core_llrs_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME, "get (llr = 0x%p)", core_llr);

	return core_llr;
}

int sl_core_data_llr_config_set(struct sl_core_llr *core_llr, struct sl_llr_config *llr_config)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "config set");

	core_llr->settings.size              = 1;
	core_llr->settings.lossless_when_off = 1;

	switch (llr_config->link_dn_behavior) {
	default:
	case SL_LLR_LINK_DN_BEHAVIOR_DISCARD:
		core_llr->settings.link_down_behavior = SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_DISCARD;
		break;
	case SL_LLR_LINK_DN_BEHAVIOR_BLOCK:
		core_llr->settings.link_down_behavior = SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_BLOCK;
		break;
	case SL_LLR_LINK_DN_BEHAVIOR_BEST_EFFORT:
		core_llr->settings.link_down_behavior = SS2_PORT_PML_LINK_DN_BEHAVIOR_T_LD_BEST_EFFORT;
		break;
	}

	core_llr->settings.filter_ctl_frames   = 1;
	core_llr->settings.ctl_frame_smac      = 0;
	core_llr->settings.ctl_frame_ethertype = 0x88B6;
	core_llr->settings.retry_threshold     = 2;
	core_llr->settings.allow_re_init       = 0;
	core_llr->settings.replay_ct_max       = 0xFF;
	core_llr->settings.replay_timer_max    = 15500;

	if (llr_config->setup_timeout_ms == 0) {
		sl_core_log_warn(core_llr, LOG_NAME,
			"config set - setup timeout is 0, setting to default");
		core_llr->timers[SL_CORE_TIMER_LLR_SETUP].data.timeout_ms = 3000;
	} else {
		core_llr->timers[SL_CORE_TIMER_LLR_SETUP].data.timeout_ms = llr_config->setup_timeout_ms;
	}
	if (llr_config->start_timeout_ms == 0) {
		sl_core_log_warn(core_llr, LOG_NAME,
			"config set - start timeout is 0, setting to default");
		core_llr->timers[SL_CORE_TIMER_LLR_START].data.timeout_ms = 3000;
	} else {
		core_llr->timers[SL_CORE_TIMER_LLR_START].data.timeout_ms = llr_config->start_timeout_ms;
	}

	sl_core_log_dbg(core_llr, LOG_NAME,
		"config set settings (link_dn_behavior %u %s = %u)",
		llr_config->link_dn_behavior, sl_llr_link_dn_behavior_str(llr_config->link_dn_behavior),
		core_llr->settings.link_down_behavior);

	return 0;
}

int sl_core_data_llr_policy_set(struct sl_core_llr *core_llr, struct sl_llr_policy *llr_policy)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_llr, LOG_NAME, "policy set");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->policy = *llr_policy;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	return 0;
}

int sl_core_data_llr_policy_get(struct sl_core_llr *core_llr, struct sl_llr_policy *llr_policy)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_llr, LOG_NAME, "policy get");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	*llr_policy = core_llr->policy;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	return 0;
}

int sl_core_data_llr_settings(struct sl_core_llr *core_llr)
{
	struct sl_lgrp_config *lgrp_config;

	sl_core_log_dbg(core_llr, LOG_NAME, "settings");

	lgrp_config = &(core_llr->core_lgrp->config);

	if (hweight_long(lgrp_config->tech_map) != 1) {
		sl_core_log_err(core_llr, LOG_NAME,
			"settings - tech map invalid (map = 0x%08X)",
			lgrp_config->tech_map);
		return -EINVAL;
	}

	if (SL_LGRP_CONFIG_TECH_CK_400G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 50;
	if (SL_LGRP_CONFIG_TECH_CK_200G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 25;
	if (SL_LGRP_CONFIG_TECH_CK_100G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 13;
	if (SL_LGRP_CONFIG_TECH_BS_200G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 25;
	if (SL_LGRP_CONFIG_TECH_CD_100G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 13;
	if (SL_LGRP_CONFIG_TECH_CD_50G  & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 7;
	if (SL_LGRP_CONFIG_TECH_BJ_100G & lgrp_config->tech_map)
		core_llr->settings.bytes_per_ns = 13;

	sl_core_log_dbg(core_llr, LOG_NAME, "settings (bytes = %u)", core_llr->settings.bytes_per_ns);

	if (is_flag_set(lgrp_config->options, SL_LGRP_OPT_FABRIC)) {
		core_llr->settings.max_cap_data   = 0x800ULL;
		core_llr->settings.max_cap_seq    = 0x800ULL;
	} else {
		switch (lgrp_config->furcation) {
		case SL_MEDIA_FURCATION_X1:
			core_llr->settings.max_cap_data   = 0x800ULL;
			core_llr->settings.max_cap_seq    = 0x800ULL;
			break;
		case SL_MEDIA_FURCATION_X2:
			core_llr->settings.max_cap_data   = 0x400ULL;
			core_llr->settings.max_cap_seq    = 0x400ULL;
			break;
		case SL_MEDIA_FURCATION_X4:
			core_llr->settings.max_cap_data   = 0x200ULL;
			core_llr->settings.max_cap_seq    = 0x200ULL;
			break;
		}
	}

	return 0;
}

void sl_core_data_llr_state_set(struct sl_core_llr *core_llr, u32 llr_state)
{
	unsigned long irq_flags;

	if (sl_core_llr_is_canceled(core_llr)) {
		sl_core_log_dbg(core_llr, LOG_NAME,
			"llr_state_set canceled (core_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		return;
	}

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->state = llr_state;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"set state = %s", sl_core_llr_state_str(llr_state));
}

u32 sl_core_data_llr_state_get(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;
	u32           llr_state;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_state = core_llr->state;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"get state = %u %s", llr_state, sl_core_llr_state_str(llr_state));

	return llr_state;
}

void sl_core_data_llr_data_set(struct sl_core_llr *core_llr, struct sl_llr_data llr_data)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->is_data_valid = true;
	core_llr->data          = llr_data;
	core_llr->data.magic    = SL_CORE_LLR_DATA_MAGIC;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"data set (min = %lldns, max = %lldns, average = %lldns, calc = %lldns)",
		llr_data.loop.min, llr_data.loop.max,
		llr_data.loop.average, llr_data.loop.calculated);
}

struct sl_llr_data sl_core_data_llr_data_get(struct sl_core_llr *core_llr)
{
	unsigned long      irq_flags;
	struct sl_llr_data llr_data;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_data = core_llr->data;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"data get (min = %lluns, max = %lluns, average = %lluns, calc = %lluns)",
		llr_data.loop.min, llr_data.loop.max,
		llr_data.loop.average, llr_data.loop.calculated);

	return llr_data;
}

void sl_core_data_llr_data_clr(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_llr, LOG_NAME, "data clear");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->is_data_valid = false;
	memset(&(core_llr->data), 0, sizeof(core_llr->data));
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
}

bool sl_core_data_llr_data_is_valid(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;
	bool          is_valid;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	is_valid = core_llr->is_data_valid;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	return is_valid;
}

void sl_core_data_llr_info_map_clr(struct sl_core_llr *core_llr, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);

	if (bit_num == SL_CORE_INFO_MAP_NUM_BITS)
		bitmap_zero((unsigned long *)&(core_llr->info_map), SL_CORE_INFO_MAP_NUM_BITS);
	else
		clear_bit(bit_num, (unsigned long *)&(core_llr->info_map));

	sl_core_log_dbg(core_llr, LOG_NAME,
		"clr info map 0x%016llX", core_llr->info_map);

	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
}

void sl_core_data_llr_info_map_set(struct sl_core_llr *core_llr, u32 bit_num)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	set_bit(bit_num, (unsigned long *)&(core_llr->info_map));

	sl_core_log_dbg(core_llr, LOG_NAME,
		"set info map 0x%016llX", core_llr->info_map);

	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
}

u64 sl_core_data_llr_info_map_get(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;
	u64           info_map;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	info_map = core_llr->info_map;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"get info map 0x%016llX", info_map);

	return info_map;
}
