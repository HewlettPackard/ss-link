// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_kconfig.h"

#include "base/sl_core_log.h"

#include "sl_core_lgrp.h"
#include "sl_core_llr.h"
#include "sl_core_str.h"
#include "data/sl_core_data_link.h"
#include "data/sl_core_data_llr.h"
#include "hw/sl_core_hw_llr.h"

#define LOG_NAME SL_CORE_LLR_LOG_NAME

int sl_core_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	return sl_core_data_llr_new(ldev_num, lgrp_num, llr_num);
}

void sl_core_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	sl_core_data_llr_del(ldev_num, lgrp_num, llr_num);
}

struct sl_core_llr *sl_core_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	return sl_core_data_llr_get(ldev_num, lgrp_num, llr_num);
}

int sl_core_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config)
{
	int                 rtn;
	unsigned long       irq_flags;
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "config set");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_OFF:
	case SL_CORE_LLR_STATE_CONFIGURED:
		rtn = sl_core_data_llr_config_set(core_llr, llr_config);
		if (rtn) {
			sl_core_log_err(core_llr, LOG_NAME,
				"config set - core_data_llr_config_set failed [%d]", rtn);
			spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
			return rtn;
		}
		core_llr->state = SL_CORE_LLR_STATE_CONFIGURED;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"config set - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	int                 rtn;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "policy set");

	rtn = sl_core_data_llr_policy_set(core_llr, llr_policy);
	if (rtn) {
		sl_core_log_err(core_llr, LOG_NAME,
			"policy set - core_data_llr_policy_set failed [%d]", rtn);
		return rtn;
	}
	return 0;
}

int sl_core_llr_policy_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	return sl_core_data_llr_policy_get(core_llr, llr_policy);
}

int sl_core_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	sl_core_llr_setup_callback_t callback, void *tag, u32 flags)
{
	int                 rtn;
	unsigned long       irq_flags;
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_CONFIGURED:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup");
		core_llr->state = SL_CORE_LLR_STATE_SETTING_UP;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		rtn = sl_core_data_llr_settings(core_llr);
		if (rtn) {
			sl_core_log_err_trace(core_llr, LOG_NAME, "setup - llr_settings failed [%d]", rtn);
			return -EBADRQC;
		}
		sl_core_hw_llr_setup_cmd(core_llr, callback, tag, flags);
		return 0;
	case SL_CORE_LLR_STATE_SETTING_UP:
	case SL_CORE_LLR_STATE_SETUP:
	case SL_CORE_LLR_STATE_STARTING:
	case SL_CORE_LLR_STATE_CANCELING:
	case SL_CORE_LLR_STATE_STOPPING:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup - in progress");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return -EINPROGRESS;
	case SL_CORE_LLR_STATE_RUNNING:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup - already running");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"setup - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	sl_core_llr_start_callback_t callback, void *tag, u32 flags)
{
	unsigned long       irq_flags;
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "start");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_CONFIGURED:
		sl_core_log_dbg(core_llr, LOG_NAME, "start - configured, not setup");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return 0;
	case SL_CORE_LLR_STATE_SETUP:
		sl_core_log_dbg(core_llr, LOG_NAME, "start");
		core_llr->state = SL_CORE_LLR_STATE_STARTING;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		sl_core_hw_llr_start_cmd(core_llr, callback, tag, flags);
		return 0;
	case SL_CORE_LLR_STATE_RUNNING:
		sl_core_log_dbg(core_llr, LOG_NAME, "start - already running");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"start - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 flags)
{
	unsigned long       irq_flags;
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	llr_state = core_llr->state;
	sl_core_log_dbg(core_llr, LOG_NAME, "stop (state = %u)", core_llr->state);
	switch (llr_state) {
	case SL_CORE_LLR_STATE_OFF:
	case SL_CORE_LLR_STATE_CONFIGURED:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - already off");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return 0;
	case SL_CORE_LLR_STATE_CANCELING:
	case SL_CORE_LLR_STATE_STOPPING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - waiting");
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		sl_core_hw_llr_off_wait(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_SETTING_UP:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - cancel setup");
		core_llr->state = SL_CORE_LLR_STATE_CANCELING;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		sl_core_hw_llr_setup_cancel_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_STARTING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - cancel start");
		core_llr->state = SL_CORE_LLR_STATE_CANCELING;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		sl_core_hw_llr_start_cancel_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_SETUP:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
	case SL_CORE_LLR_STATE_START_TIMEOUT:
	case SL_CORE_LLR_STATE_RUNNING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop");
		core_llr->state = SL_CORE_LLR_STATE_STOPPING;
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		sl_core_hw_llr_stop_cmd(core_llr, flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"stop - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
		return -EBADRQC;
	}
}

int sl_core_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *llr_state)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	*llr_state = sl_core_data_llr_state_get(core_llr);

	sl_core_log_dbg(core_llr, LOG_NAME, "state get (llr_state = %u %s)",
		*llr_state, sl_core_llr_state_str(*llr_state));

	return 0;
}

int sl_core_llr_data_get(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	struct sl_llr_data *llr_data)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "data get");

	*llr_data = sl_core_data_llr_data_get(core_llr);

	return 0;
}

void sl_core_llr_data_free(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	struct sl_llr_data *llr_data)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "data free");

	kmem_cache_free(core_llr->data_cache, llr_data);
}

bool sl_core_llr_is_canceled(struct sl_core_llr *core_llr)
{
	bool          is_canceled;
	unsigned long irq_flags;

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	is_canceled  = core_llr->is_canceled;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);

	sl_core_log_dbg(core_llr, LOG_NAME, "is_canceled = %s", is_canceled ? "true" : "false");

	return is_canceled;
}

void sl_core_llr_is_canceled_set(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_llr, LOG_NAME, "is_canceled_set");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->is_canceled = true;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
}

void sl_core_llr_is_canceled_clr(struct sl_core_llr *core_llr)
{
	unsigned long irq_flags;

	sl_core_log_dbg(core_llr, LOG_NAME, "is_canceled_clr");

	spin_lock_irqsave(&core_llr->data_lock, irq_flags);
	core_llr->is_canceled = false;
	spin_unlock_irqrestore(&core_llr->data_lock, irq_flags);
}
