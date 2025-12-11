// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"
#include "sl_ctrl_llr.h"
#include "data/sl_core_data_llr.h"
#include "sl_core_str.h"
#include "sl_ctrl_llr_counters.h"

#define LOG_NAME SL_CTRL_LLR_LOG_NAME

#define SL_CTRL_LLR_DEL_WAIT_TIMEOUT_MS 2000

static struct sl_ctrl_llr *ctrl_llrs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctrl_llrs_lock);

static void sl_ctrl_llr_setup_callback(void *tag, u32 core_llr_state, u64 core_imap, struct sl_llr_data core_llr_data)
{
	int                       rtn;
	struct sl_ctrl_llr       *ctrl_llr;
	union sl_lgrp_notif_info  info;

	ctrl_llr = tag;

	ctrl_llr->setup.state = core_llr_state;
	ctrl_llr->setup.imap  = core_imap;
	ctrl_llr->setup.data  = core_llr_data;

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "setup callback (state = %u %s)", core_llr_state,
			sl_core_llr_state_str(core_llr_state));

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_SETUP:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_SETUP);
		info.llr_data = ctrl_llr->setup.data;
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_SETUP, &info, ctrl_llr->setup.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "setup SETUP ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_SETUP_TIMEOUT);
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT, NULL, ctrl_llr->setup.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "setup TIMEOUT ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_CONFIGURED:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_CONFIGURED);
		memset(&ctrl_llr->setup.data, 0, sizeof(struct sl_llr_data));
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctrl_llr->setup.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "setup CONFIGURED ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	default:
		memset(&ctrl_llr->setup.data, 0, sizeof(struct sl_llr_data));
		sl_ctrl_log_err(ctrl_llr, LOG_NAME,
				"setup ERROR invalid (state = %u %s)",
				core_llr_state, sl_core_llr_state_str(core_llr_state));
		info.error = -ENOMSG;
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_ERROR, &info, ctrl_llr->setup.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "setup ERROR ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	}
}

static void sl_ctrl_llr_start_callback(void *tag, u32 core_llr_state, u64 core_imap)
{
	int                       rtn;
	struct sl_ctrl_llr       *ctrl_llr;
	union sl_lgrp_notif_info  info;

	ctrl_llr = tag;

	ctrl_llr->start.state = core_llr_state;
	ctrl_llr->start.imap  = core_imap;

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "start callback (state = %u %s)",
			core_llr_state, sl_core_llr_state_str(core_llr_state));

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_RUNNING:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_RUNNING);
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_RUNNING, NULL, ctrl_llr->start.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "start RUNNING ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_START_TIMEOUT);
		if (ctrl_llr->policy.options & SL_LLR_POLICY_OPT_CONTINUOUS_START_TRIES) {
			sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "start TIMEOUT retry");
			rtn = sl_core_llr_start(ctrl_llr->ctrl_lgrp->ctrl_ldev->num, ctrl_llr->ctrl_lgrp->num,
						ctrl_llr->num, sl_ctrl_llr_start_callback, ctrl_llr, 0);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
						       "start TIMEOUT core_llr_start retry failed [%d]", rtn);
		}
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_START_TIMEOUT, NULL, ctrl_llr->start.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "start TIMEOUT ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_SETUP:
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_SETUP);
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num,
						 SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctrl_llr->start.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME,
					       "start CONFIGURED ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	default:
		memset(&ctrl_llr->setup.data, 0, sizeof(struct sl_llr_data));
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "start ERROR invalid (state = %u %s)",
				core_llr_state, sl_core_llr_state_str(core_llr_state));
		info.error = -ENOMSG;
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_llr->ctrl_lgrp, ctrl_llr->num, SL_LGRP_NOTIF_LLR_ERROR,
						 &info, ctrl_llr->setup.imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME, "start ERROR ctrl_lgrp_notif_enqueue failed [%d[",
					       rtn);
		return;
	}
}

int sl_ctrl_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct kobject *sysfs_parent)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (ctrl_llr) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "exists (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	ctrl_llr = kzalloc(sizeof(*ctrl_llr), GFP_KERNEL);
	if (!ctrl_llr)
		return -ENOMEM;

	ctrl_llr->magic     = SL_CTRL_LLR_MAGIC;
	ctrl_llr->ver       = SL_CTRL_LLR_VER;
	ctrl_llr->num       = llr_num;
	ctrl_llr->ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&ctrl_llr->data_lock);
	kref_init(&ctrl_llr->ref_cnt);
	init_completion(&ctrl_llr->del_complete);

	rtn = sl_ctrl_llr_counters_init(ctrl_llr);
	if (rtn) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "ctrl_llr_counters_init failed [%d]", rtn);
		goto out;
	}

	rtn = sl_ctrl_llr_cause_counters_init(ctrl_llr);
	if (rtn) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "ctrl_llr_cause_counters_init failed [%d]", rtn);
		goto out;
	}

	rtn = sl_core_llr_new(ldev_num, lgrp_num, llr_num);
	if (rtn) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "core_llr_new failed [%d]", rtn);
		goto out;
	}

	if (sysfs_parent) {
		ctrl_llr->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_llr_create(ctrl_llr);
		if (rtn) {
			sl_ctrl_log_err(ctrl_llr, LOG_NAME, "sysfs_llr_create failed [%d]", rtn);
			goto out;
		}
	}

	spin_lock(&ctrl_llrs_lock);
	ctrl_llrs[ldev_num][lgrp_num][llr_num] = ctrl_llr;
	spin_unlock(&ctrl_llrs_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "new (ctrl_llr = 0x%p)", ctrl_llr);

	return 0;

out:
	kfree(ctrl_llr);
	return -ENOMEM;
}

static void sl_ctrl_llr_release(struct kref *kref)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u8                  ldev_num;
	u8                  lgrp_num;
	u8                  llr_num;

	ctrl_llr = container_of(kref, struct sl_ctrl_llr, ref_cnt);
	ldev_num = ctrl_llr->ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = ctrl_llr->ctrl_lgrp->num;
	llr_num = ctrl_llr->num;

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "release (ctrl_llr = 0x%p)", ctrl_llr);

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
	if (rtn)
		sl_ctrl_log_warn_trace(ctrl_llr, LOG_NAME, "release core_llr_stop failed [%d]", rtn);

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_llr_delete(ctrl_llr);

	sl_core_llr_del(ldev_num, lgrp_num, llr_num);

	sl_ctrl_llr_counters_del(ctrl_llr);
	sl_ctrl_llr_cause_counters_del(ctrl_llr);

	spin_lock(&ctrl_llrs_lock);
	ctrl_llrs[ldev_num][lgrp_num][llr_num] = NULL;
	spin_unlock(&ctrl_llrs_lock);

	complete_all(&ctrl_llr->del_complete);

	kfree(ctrl_llr);
}

static int sl_ctrl_llr_put(struct sl_ctrl_llr *ctrl_llr)
{
	return kref_put(&ctrl_llr->ref_cnt, sl_ctrl_llr_release);
}

int sl_ctrl_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctrl_llr *ctrl_llr;
	unsigned long       timeleft;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME, "del not found (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				      ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	/* Release occurs on the last caller. Block until complete. */
	if (!sl_ctrl_llr_put(ctrl_llr)) {
		timeleft = wait_for_completion_timeout(&ctrl_llr->del_complete,
						       msecs_to_jiffies(SL_CTRL_LLR_DEL_WAIT_TIMEOUT_MS));

		if (timeleft == 0) {
			sl_ctrl_log_err(ctrl_llr, LOG_NAME, "del completion_timeout (ctrl_llr = 0x%p)", ctrl_llr);
			return -ETIMEDOUT;
		}
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "del complete (ctrl_llr = 0x%p, timeleft = %lu)", ctrl_llr, timeleft);

	return 0;
}

static bool sl_ctrl_llr_kref_get_unless_zero(struct sl_ctrl_llr *ctrl_llr)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctrl_llr->ref_cnt) != 0);

	if (!incremented)
		sl_ctrl_log_warn(ctrl_llr, LOG_NAME, "kref_get_unless_zero ref unavailable (ctrl_llr = 0x%p)",
				 ctrl_llr);

	return incremented;
}

struct sl_ctrl_llr *sl_ctrl_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctrl_llr *ctrl_llr;

	spin_lock(&ctrl_llrs_lock);
	ctrl_llr = ctrl_llrs[ldev_num][lgrp_num][llr_num];
	spin_unlock(&ctrl_llrs_lock);

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "get (ctrl_llr = 0x%p)", ctrl_llr);

	return ctrl_llr;
}

int sl_ctrl_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "config set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "config set kref_get_unless_zero failed (ctrl_llr = 0x%p)",
				ctrl_llr);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "config set (setup timeout = %d, start timeout = %d)",
			llr_config->setup_timeout_ms, llr_config->start_timeout_ms);

	rtn = sl_core_llr_config_set(ldev_num, lgrp_num, llr_num, llr_config);
	if (rtn) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "core_llr_config_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&ctrl_llr->data_lock);
	ctrl_llr->config = *llr_config;
	spin_unlock(&ctrl_llr->data_lock);

out:
	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "config set - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

int sl_ctrl_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "policy set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME,
				"policy set kref_get_unless_zero failed (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "policy set");

	rtn = sl_core_llr_policy_set(ldev_num, lgrp_num, llr_num, llr_policy);
	if (rtn) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "core_llr_policy_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&ctrl_llr->data_lock);
	ctrl_llr->policy = *llr_policy;
	spin_unlock(&ctrl_llr->data_lock);

out:
	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "policy set - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

int sl_ctrl_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "setup NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "setup kref_get_unless_zero failed (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "setup");

	SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_SETUP_CMD);

	rtn = sl_core_llr_setup(ldev_num, lgrp_num, llr_num,
		sl_ctrl_llr_setup_callback, ctrl_llr, 0);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_llr, LOG_NAME, "core_llr_setup failed [%d]", rtn);
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_SETUP_FAIL);
	}

	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "setup - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

int sl_ctrl_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "start NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "start kref_get_unless_zero failed (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "start");

	SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_START_CMD);

	rtn = sl_core_llr_start(ldev_num, lgrp_num, llr_num,
				 sl_ctrl_llr_start_callback, ctrl_llr, 0);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_llr, LOG_NAME, "core_llr_start failed [%d]", rtn);
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_START_FAIL);
	}

	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "start - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

int sl_ctrl_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "stop NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "stop kref_get_unless_zero failed (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "stop");

	SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_STOP_CMD);

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_llr, LOG_NAME, "core_llr_stop failed [%d]", rtn);
		SL_CTRL_LLR_COUNTER_INC(ctrl_llr, LLR_STOP_FAIL);
	}

	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "stop - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

u32 sl_ctrl_llr_state_from_core_llr_state(u32 core_llr_state)
{
	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_NEW:
		return SL_LLR_STATE_OFF;
	case SL_CORE_LLR_STATE_CONFIGURED:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
	case SL_CORE_LLR_STATE_SETUP_STOPPING:
		return SL_LLR_STATE_CONFIGURED;
	case SL_CORE_LLR_STATE_SETTING_UP:
		return SL_LLR_STATE_SETUP_BUSY;
	case SL_CORE_LLR_STATE_SETUP:
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		return SL_LLR_STATE_SETUP;
	case SL_CORE_LLR_STATE_STARTING:
		return SL_LLR_STATE_START_BUSY;
	case SL_CORE_LLR_STATE_RUNNING:
		return SL_LLR_STATE_RUNNING;
	case SL_CORE_LLR_STATE_STOPPING:
		return SL_LLR_STATE_STOP_BUSY;
	case SL_CORE_LLR_STATE_SETUP_CANCELING:
	case SL_CORE_LLR_STATE_START_CANCELING:
		return SL_LLR_STATE_CANCELING;
	case SL_CORE_LLR_STATE_INVALID:
	default:
		return SL_LLR_STATE_INVALID;
	}
}

int sl_ctrl_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *state)
{
	int                rtn;
	struct sl_ctrl_llr *ctrl_llr;
	u32                core_llr_state;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "state get NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME, "state get kref_get_unless_zero failed (ctrl_llr = 0x%p)",
				ctrl_llr);
		return -EBADRQC;
	}

	rtn = sl_core_llr_state_get(ldev_num, lgrp_num, llr_num, &core_llr_state);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_llr, LOG_NAME, "core_llr_state_get failed [%d]", rtn);
		*state = SL_LLR_STATE_OFF;
		rtn = 0;
		goto out;
	}

	*state = sl_ctrl_llr_state_from_core_llr_state(core_llr_state);
	if (*state == SL_LLR_STATE_INVALID)
		sl_ctrl_log_err(ctrl_llr, LOG_NAME,
				"invalid (core_llr_state = %u %s)",
				core_llr_state, sl_core_llr_state_str(core_llr_state));

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "state (core = %u %s, ctrl = %u %s)",
			core_llr_state, sl_core_llr_state_str(core_llr_state),
			*state, sl_llr_state_str(*state));

out:
	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "state get - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}

int sl_ctrl_llr_info_map_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u64 *info_map)
{
	int                 rtn;
	struct sl_ctrl_llr *ctrl_llr;

	ctrl_llr = sl_ctrl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctrl_llr) {
		sl_ctrl_log_err(NULL, LOG_NAME, "info map get NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
				ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctrl_llr_kref_get_unless_zero(ctrl_llr)) {
		sl_ctrl_log_err(ctrl_llr, LOG_NAME,
				"info map get kref_get_unless_zero failed (ctrl_llr = 0x%p)", ctrl_llr);
		return -EBADRQC;
	}

	rtn = sl_core_data_llr_info_map_get(sl_core_llr_get(ldev_num, lgrp_num, llr_num), info_map);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_llr, LOG_NAME, "core_data_llr_info_map_get failed [%d]", rtn);
		goto out;
	}

	sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "info map get (info_map = 0x%llX)", *info_map);

out:
	if (sl_ctrl_llr_put(ctrl_llr))
		sl_ctrl_log_dbg(ctrl_llr, LOG_NAME, "info map get - llr removed (ctrl_llr = 0x%p)", ctrl_llr);

	return rtn;
}
