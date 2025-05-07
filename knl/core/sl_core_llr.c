// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

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
	struct sl_core_llr *core_llr;
	u32                 llr_state;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "config set");

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_NEW:
	case SL_CORE_LLR_STATE_CONFIGURED:
		rtn = sl_core_data_llr_config_set(core_llr, llr_config);
		if (rtn) {
			sl_core_log_err(core_llr, LOG_NAME,
				"config set - llr_config_set failed [%d]", rtn);
			spin_unlock(&core_llr->data_lock);
			return rtn;
		}
		core_llr->state = SL_CORE_LLR_STATE_CONFIGURED;
		spin_unlock(&core_llr->data_lock);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"config set - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
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
			"policy set - llr_policy_set failed [%d]", rtn);
		return rtn;
	}
	return 0;
}

int sl_core_llr_policy_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "policy get");

	return sl_core_data_llr_policy_get(core_llr, llr_policy);
}

int sl_core_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	sl_core_llr_setup_callback_t callback, void *tag, u32 flags)
{
	int                 rtn;
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "setup");

	reinit_completion(&core_llr->stop_complete);
	spin_lock(&core_llr->data_lock);

	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_SETUP:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup - already setup");
		spin_unlock(&core_llr->data_lock);
		return -EALREADY;
	case SL_CORE_LLR_STATE_CONFIGURED:
		sl_core_log_dbg(core_llr, LOG_NAME, "setup - in configured");
		core_llr->state = SL_CORE_LLR_STATE_SETTING_UP;
		spin_unlock(&core_llr->data_lock);
		rtn = sl_core_data_llr_settings(core_llr);
		if (rtn) {
			sl_core_log_err_trace(core_llr, LOG_NAME,
				"setup - llr_settings failed [%d]", rtn);
			return -EBADRQC;
		}
		sl_core_hw_llr_setup_cmd(core_llr, callback, tag, flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"setup - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return -EBADRQC;
	}
}

int sl_core_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num,
	sl_core_llr_start_callback_t callback, void *tag, u32 flags)
{
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "start");

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	switch (llr_state) {
	case SL_CORE_LLR_STATE_RUNNING:
		sl_core_log_dbg(core_llr, LOG_NAME, "start - already started");
		spin_unlock(&core_llr->data_lock);
		return -EALREADY;
	case SL_CORE_LLR_STATE_SETUP:
		sl_core_log_dbg(core_llr, LOG_NAME, "start - in setup");
		core_llr->state = SL_CORE_LLR_STATE_STARTING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_start_cmd(core_llr, callback, tag, flags);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"start - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
		return -EBADRQC;
	}
}

int sl_core_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	u32                 llr_state;
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);
	if (!core_llr)
		return 0;

	spin_lock(&core_llr->data_lock);
	llr_state = core_llr->state;
	sl_core_log_dbg(core_llr, LOG_NAME, "stop (state = %u)", core_llr->state);
	switch (llr_state) {
	case SL_CORE_LLR_STATE_NEW:
	case SL_CORE_LLR_STATE_CONFIGURED:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - already off");
		spin_unlock(&core_llr->data_lock);
		return 0;
	case SL_CORE_LLR_STATE_CANCELING:
	case SL_CORE_LLR_STATE_STOPPING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - waiting");
		spin_unlock(&core_llr->data_lock);
		return sl_core_hw_llr_stop_wait(core_llr);
	case SL_CORE_LLR_STATE_SETTING_UP:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - cancel setting up");
		core_llr->state = SL_CORE_LLR_STATE_CANCELING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_settingup_cancel_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_STARTING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - cancel starting");
		core_llr->state = SL_CORE_LLR_STATE_CANCELING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_starting_cancel_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_SETUP:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - in setup");
		core_llr->state = SL_CORE_LLR_STATE_STOPPING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_setup_stop_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_RUNNING:
		sl_core_log_dbg(core_llr, LOG_NAME, "stop - in running");
		core_llr->state = SL_CORE_LLR_STATE_STOPPING;
		spin_unlock(&core_llr->data_lock);
		sl_core_hw_llr_running_stop_cmd(core_llr);
		return 0;
	case SL_CORE_LLR_STATE_START_TIMEOUT:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		/* do nothing */
		spin_unlock(&core_llr->data_lock);
		return 0;
	default:
		sl_core_log_err(core_llr, LOG_NAME,
			"stop - invalid (llr_state = %u %s)",
			llr_state, sl_core_llr_state_str(llr_state));
		spin_unlock(&core_llr->data_lock);
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

struct sl_llr_data sl_core_llr_data_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_core_llr *core_llr;

	core_llr = sl_core_llr_get(ldev_num, lgrp_num, llr_num);

	sl_core_log_dbg(core_llr, LOG_NAME, "data get");

	return sl_core_data_llr_data_get(core_llr);
}

bool sl_core_llr_is_canceled(struct sl_core_llr *core_llr)
{
	bool is_canceled;

	spin_lock(&core_llr->data_lock);
	is_canceled  = core_llr->is_canceled;
	spin_unlock(&core_llr->data_lock);

	sl_core_log_dbg(core_llr, LOG_NAME,
		"is_canceled = %s", is_canceled ? "true" : "false");

	return is_canceled;
}

void sl_core_llr_is_canceled_set(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "is_canceled_set");

	spin_lock(&core_llr->data_lock);
	core_llr->is_canceled = true;
	spin_unlock(&core_llr->data_lock);
}

void sl_core_llr_is_canceled_clr(struct sl_core_llr *core_llr)
{
	sl_core_log_dbg(core_llr, LOG_NAME, "is_canceled_clr");

	spin_lock(&core_llr->data_lock);
	core_llr->is_canceled = false;
	spin_unlock(&core_llr->data_lock);
}

void sl_core_llr_last_fail_cause_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 llr_fail_cause)
{
	sl_core_data_llr_last_fail_cause_set(sl_core_llr_get(ldev_num, lgrp_num, llr_num), llr_fail_cause);
}

void sl_core_llr_last_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *llr_fail_cause,
	time64_t *llr_fail_time)
{
	sl_core_data_llr_last_fail_cause_get(sl_core_llr_get(ldev_num, lgrp_num, llr_num),
		llr_fail_cause, llr_fail_time);
}

const char *sl_core_llr_fail_cause_str(u32 llr_fail_cause)
{
	switch (llr_fail_cause) {
	case SL_LLR_FAIL_CAUSE_SETUP_CONFIG:
		return "setup-config";
	case SL_LLR_FAIL_CAUSE_SETUP_INTR_ENABLE:
		return "setup-intr-enable";
	case SL_LLR_FAIL_CAUSE_SETUP_TIMEOUT:
		return "setup-timeout";
	case SL_LLR_FAIL_CAUSE_START_INTR_ENABLE:
		return "start-intr-enable";
	case SL_LLR_FAIL_CAUSE_START_TIMEOUT:
		return "start-timeout";
	default:
		return "unknown";
	}
}
