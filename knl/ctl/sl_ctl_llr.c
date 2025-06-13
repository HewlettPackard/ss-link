// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_asic.h"
#include "sl_sysfs.h"

#include "base/sl_ctl_log.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_lgrp_notif.h"
#include "sl_ctl_llr.h"
#include "sl_core_llr.h"
#include "sl_core_str.h"

#define LOG_NAME SL_CTL_LLR_LOG_NAME
#define SL_CTL_LLR_DEL_WAIT_TIMEOUT_MS 2000

static struct sl_ctl_llr *ctl_llrs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctl_llrs_lock);

static void sl_ctl_llr_setup_callback(void *tag, u32 core_llr_state,
	u64 core_imap, struct sl_llr_data core_llr_data)
{
	int                       rtn;
	struct sl_ctl_llr        *ctl_llr;
	union sl_lgrp_notif_info  info;

	ctl_llr = tag;

	ctl_llr->setup.state = core_llr_state;
	ctl_llr->setup.imap  = core_imap;
	ctl_llr->setup.data  = core_llr_data;

	sl_ctl_log_dbg(ctl_llr, LOG_NAME,
		"setup callback (state = %u %s)",
		core_llr_state, sl_core_llr_state_str(core_llr_state));

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_SETUP:
		info.llr_data = ctl_llr->setup.data;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_SETUP, &info, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup SETUP ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT, NULL, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup TIMEOUT ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_CONFIGURED:
		memset(&(ctl_llr->setup.data), 0, sizeof(struct sl_llr_data));
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup CONFIGURED ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	default:
		memset(&(ctl_llr->setup.data), 0, sizeof(struct sl_llr_data));
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"setup ERROR invalid (state = %u %s)",
			core_llr_state, sl_core_llr_state_str(core_llr_state));
		info.error = -ENOMSG;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_ERROR, &info, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup ERROR ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	}
}

static void sl_ctl_llr_start_callback(void *tag, u32 core_llr_state, u64 core_imap)
{
	int                       rtn;
	struct sl_ctl_llr        *ctl_llr;
	union sl_lgrp_notif_info  info;

	ctl_llr = tag;

	ctl_llr->start.state = core_llr_state;
	ctl_llr->start.imap  = core_imap;

	sl_ctl_log_dbg(ctl_llr, LOG_NAME,
		"start callback (state = %u %s)",
		core_llr_state, sl_core_llr_state_str(core_llr_state));

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_RUNNING:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_RUNNING, NULL, ctl_llr->start.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"start RUNNING ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		if (ctl_llr->policy.options & SL_LLR_POLICY_OPT_CONTINUOUS_START_TRIES) {
			sl_ctl_log_dbg(ctl_llr, LOG_NAME, "start TIMEOUT retry");
			rtn = sl_core_llr_start(ctl_llr->ctl_lgrp->ctl_ldev->num, ctl_llr->ctl_lgrp->num,
						ctl_llr->num, sl_ctl_llr_start_callback, ctl_llr, 0);
			if (rtn)
				sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
					"start TIMEOUT core_llr_start retry failed [%d]", rtn);
		}
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_START_TIMEOUT, NULL, ctl_llr->start.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"start TIMEOUT ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	case SL_CORE_LLR_STATE_SETUP:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctl_llr->start.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"start CONFIGURED ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return;
	default:
		memset(&(ctl_llr->setup.data), 0, sizeof(struct sl_llr_data));
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"start ERROR invalid (state = %u %s)",
			core_llr_state, sl_core_llr_state_str(core_llr_state));
		info.error = -ENOMSG;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_ERROR, &info, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"start ERROR ctl_lgrp_notif_enqueue failed [%d[",
				rtn);
		return;
	}
}

int sl_ctl_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct kobject *sysfs_parent)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (ctl_llr) {
		sl_ctl_log_err(ctl_llr, LOG_NAME, "exists (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	ctl_llr = kzalloc(sizeof(struct sl_ctl_llr), GFP_KERNEL);
	if (!ctl_llr)
		return -ENOMEM;

	ctl_llr->magic    = SL_CTL_LLR_MAGIC;
	ctl_llr->ver      = SL_CTL_LLR_VER;
	ctl_llr->num      = llr_num;
	ctl_llr->ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&(ctl_llr->data_lock));
	kref_init(&ctl_llr->ref_cnt);
	init_completion(&ctl_llr->del_complete);

	rtn = sl_core_llr_new(ldev_num, lgrp_num, llr_num);
	if (rtn) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"core_llr_new failed [%d]", rtn);
		goto out;
	}

	if (sysfs_parent) {
		ctl_llr->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_llr_create(ctl_llr);
		if (rtn) {
			sl_ctl_log_err(ctl_llr, LOG_NAME,
				"sysfs_llr_create failed [%d]", rtn);
			goto out;
		}
	}

	spin_lock(&ctl_llrs_lock);
	ctl_llrs[ldev_num][lgrp_num][llr_num] = ctl_llr;
	spin_unlock(&ctl_llrs_lock);

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "new (ctl_llr = 0x%p)", ctl_llr);

	return 0;

out:
	kfree(ctl_llr);
	return -ENOMEM;
}

static void sl_ctl_llr_release(struct kref *kref)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;
	u8                 ldev_num;
	u8                 lgrp_num;
	u8                 llr_num;

	ctl_llr = container_of(kref, struct sl_ctl_llr, ref_cnt);
	ldev_num = ctl_llr->ctl_lgrp->ctl_ldev->num;
	lgrp_num = ctl_llr->ctl_lgrp->num;
	llr_num = ctl_llr->num;

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "release (ctl_llr = 0x%p)", ctl_llr);

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
			"release core_llr_stop failed [%d]", rtn);

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_llr_delete(ctl_llr);

	sl_core_llr_del(ldev_num, lgrp_num, llr_num);

	spin_lock(&ctl_llrs_lock);
	ctl_llrs[ldev_num][lgrp_num][llr_num] = NULL;
	spin_unlock(&ctl_llrs_lock);

	complete_all(&ctl_llr->del_complete);

	kfree(ctl_llr);
}

static int sl_ctl_llr_put(struct sl_ctl_llr *ctl_llr)
{
	return kref_put(&ctl_llr->ref_cnt, sl_ctl_llr_release);
}

int sl_ctl_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctl_llr *ctl_llr;
	unsigned long      timeleft;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err_trace(NULL, LOG_NAME,
			"del not found (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	/* Release occurs on the last caller. Block until complete. */
	if (!sl_ctl_llr_put(ctl_llr)) {
		timeleft = wait_for_completion_timeout(&ctl_llr->del_complete,
			msecs_to_jiffies(SL_CTL_LLR_DEL_WAIT_TIMEOUT_MS));

		if (timeleft == 0) {
			sl_ctl_log_err(ctl_llr, LOG_NAME,
				"del completion_timeout (ctl_llr = 0x%p)", ctl_llr);
			return -ETIMEDOUT;
		}
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del complete (ctl_llr = 0x%p, timeleft = %lu)", ctl_llr, timeleft);

	return 0;
}

static bool sl_ctl_llr_kref_get_unless_zero(struct sl_ctl_llr *ctl_llr)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctl_llr->ref_cnt) != 0);

	if (!incremented)
		sl_ctl_log_warn(ctl_llr, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ctl_llr = 0x%p)", ctl_llr);

	return incremented;
}

struct sl_ctl_llr *sl_ctl_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctl_llr *ctl_llr;

	spin_lock(&ctl_llrs_lock);
	ctl_llr = ctl_llrs[ldev_num][lgrp_num][llr_num];
	spin_unlock(&ctl_llrs_lock);

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "get (ctl_llr = 0x%p)", ctl_llr);

	return ctl_llr;
}

int sl_ctl_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"config set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"config set kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME,
		"config set (setup timeout = %d, start timeout = %d)",
		llr_config->setup_timeout_ms, llr_config->start_timeout_ms);

	rtn = sl_core_llr_config_set(ldev_num, lgrp_num, llr_num, llr_config);
	if (rtn) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"core_llr_config_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&ctl_llr->data_lock);
	ctl_llr->config = *llr_config;
	spin_unlock(&ctl_llr->data_lock);

out:
	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "config set - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}

int sl_ctl_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"policy set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"policy set kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "policy set");

	rtn = sl_core_llr_policy_set(ldev_num, lgrp_num, llr_num, llr_policy);
	if (rtn) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"core_llr_policy_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&ctl_llr->data_lock);
	ctl_llr->policy = *llr_policy;
	spin_unlock(&ctl_llr->data_lock);

out:
	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "policy set - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}

int sl_ctl_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"setup NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"setup kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "setup");

	rtn = sl_core_llr_setup(ldev_num, lgrp_num, llr_num,
		sl_ctl_llr_setup_callback, ctl_llr, 0);
	if (rtn)
		sl_ctl_log_err_trace(ctl_llr, LOG_NAME,
			"core_llr_setup failed [%d]", rtn);

	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "setup - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}

int sl_ctl_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"start NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"start kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "start");

	rtn = sl_core_llr_start(ldev_num, lgrp_num, llr_num,
				 sl_ctl_llr_start_callback, ctl_llr, 0);
	if (rtn)
		sl_ctl_log_err_trace(ctl_llr, LOG_NAME,
			"core_llr_start failed [%d]", rtn);

	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "start - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}

int sl_ctl_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"stop NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"stop kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "stop");

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
	if (rtn)
		sl_ctl_log_err_trace(ctl_llr, LOG_NAME,
			"core_llr_stop failed [%d]", rtn);

	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "stop - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}

int sl_ctl_llr_state_get(u8 ldev_num, u8 lgrp_num, u8 llr_num, u32 *state)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;
	u32                core_llr_state;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"state get NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	if (!sl_ctl_llr_kref_get_unless_zero(ctl_llr)) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"state get kref_get_unless_zero failed (ctl_llr = 0x%p)", ctl_llr);
		return -EBADRQC;
	}

	rtn = sl_core_llr_state_get(ldev_num, lgrp_num, llr_num, &core_llr_state);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_llr, LOG_NAME,
			"core_llr_state_get failed [%d]", rtn);
		*state = SL_LLR_STATE_OFF;
		rtn = 0;
		goto out;
	}

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_NEW:
		*state = SL_LLR_STATE_OFF;
		break;
	case SL_CORE_LLR_STATE_CONFIGURED:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
	case SL_CORE_LLR_STATE_SETUP_STOPPING:
		*state = SL_LLR_STATE_CONFIGURED;
		break;
	case SL_CORE_LLR_STATE_SETTING_UP:
		*state = SL_LLR_STATE_SETUP_BUSY;
		break;
	case SL_CORE_LLR_STATE_SETUP:
	case SL_CORE_LLR_STATE_START_TIMEOUT:
		*state = SL_LLR_STATE_SETUP;
		break;
	case SL_CORE_LLR_STATE_STARTING:
		*state = SL_LLR_STATE_START_BUSY;
		break;
	case SL_CORE_LLR_STATE_RUNNING:
		*state = SL_LLR_STATE_RUNNING;
		break;
	case SL_CORE_LLR_STATE_STOPPING:
		*state = SL_LLR_STATE_STOP_BUSY;
		break;
	case SL_CORE_LLR_STATE_SETUP_CANCELING:
	case SL_CORE_LLR_STATE_START_CANCELING:
		*state = SL_LLR_STATE_CANCELING;
		break;
	default:
		sl_ctl_log_err(ctl_llr, LOG_NAME, "invalid (core state = %u %s)",
			core_llr_state, sl_core_llr_state_str(core_llr_state));
		*state = 0;
		break;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "state (core = %u %s, ctl = %u %s)",
		core_llr_state, sl_core_llr_state_str(core_llr_state),
		*state, sl_llr_state_str(*state));

out:
	if (sl_ctl_llr_put(ctl_llr))
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "state get - llr removed (ctl_llr = 0x%p)", ctl_llr);

	return rtn;
}
