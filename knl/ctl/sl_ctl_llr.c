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

static struct sl_ctl_llr *ctl_llrs[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctl_llrs_lock);

static void sl_ctl_llr_is_deleting_set(struct sl_ctl_llr *ctl_llr)
{
	unsigned long flags;

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "is_deleting set");

	spin_lock_irqsave(&ctl_llr->data_lock, flags);
	ctl_llr->is_deleting = true;
	spin_unlock_irqrestore(&ctl_llr->data_lock, flags);
}

static bool sl_ctl_llr_is_deleting(struct sl_ctl_llr *ctl_llr)
{
	unsigned long flags;
	bool          is_deleting;

	spin_lock_irqsave(&ctl_llr->data_lock, flags);
	is_deleting = ctl_llr->is_deleting;
	spin_unlock_irqrestore(&ctl_llr->data_lock, flags);

	sl_ctl_log_dbg(ctl_llr, LOG_NAME,
		"is_deleting = %s", is_deleting ? "true" : "false");

	return is_deleting;
}

static int sl_ctl_llr_setup_callback(void *tag, u32 core_llr_state,
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
		return 0;
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_SETUP_TIMEOUT, NULL, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup TIMEOUT ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return 0;
	case SL_CORE_LLR_STATE_CONFIGURED:
		memset(&(ctl_llr->setup.data), 0, sizeof(struct sl_llr_data));
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctl_llr->setup.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"setup CONFIGURED ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return 0;
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
		return 0;
	}
}

static int sl_ctl_llr_start_callback(void *tag, u32 core_llr_state, u64 core_imap)
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
		return 0;
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
		return 0;
	case SL_CORE_LLR_STATE_SETUP:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_llr->ctl_lgrp, ctl_llr->num,
			SL_LGRP_NOTIF_LLR_CANCELED, NULL, ctl_llr->start.imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
				"start CONFIGURED ctl_lgrp_notif_enqueue failed [%d[", rtn);
		return 0;
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
		return 0;
	}
}

int sl_ctl_llr_new(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct kobject *sysfs_parent)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (ctl_llr) {
		sl_ctl_log_err(ctl_llr, LOG_NAME, "exists (llr = 0x%p, is_deleting = %s)",
			ctl_llr, sl_ctl_llr_is_deleting(ctl_llr) ? "true" : "false");
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

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "new (llr = 0x%p)", ctl_llr);

	return 0;

out:
	kfree(ctl_llr);
	return -ENOMEM;
}

void sl_ctl_llr_del(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int                rtn;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"not found (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del (llr = 0x%p)", ctl_llr);

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return;
	}

	sl_ctl_llr_is_deleting_set(ctl_llr);

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_llr, LOG_NAME,
			"core_llr_stop failed [%d[", rtn);

	sl_core_llr_del(ldev_num, lgrp_num, llr_num);

	sl_sysfs_llr_delete(ctl_llr);

	spin_lock(&ctl_llrs_lock);
	ctl_llrs[ldev_num][lgrp_num][llr_num] = NULL;
	spin_unlock(&ctl_llrs_lock);

	kfree(ctl_llr);
}

struct sl_ctl_llr *sl_ctl_llr_get(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	unsigned long      irq_flags;
	struct sl_ctl_llr *ctl_llr;

	spin_lock_irqsave(&ctl_llrs_lock, irq_flags);
	ctl_llr = ctl_llrs[ldev_num][lgrp_num][llr_num];
	spin_unlock_irqrestore(&ctl_llrs_lock, irq_flags);

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "get (llr = 0x%p)", ctl_llr);

	return ctl_llr;
}

int sl_ctl_llr_config_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_config *llr_config)
{
	int                rtn;
	unsigned long      irq_flags;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"config set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME,
		"config set (setup timeout = %d, start timeout = %d)",
		llr_config->setup_timeout_ms, llr_config->start_timeout_ms);

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return -EBADRQC;
	}

	rtn = sl_core_llr_config_set(ldev_num, lgrp_num, llr_num, llr_config);
	if (rtn) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"core_llr_config_set failed [%d]", rtn);
		return rtn;
	}

	spin_lock_irqsave(&ctl_llr->data_lock, irq_flags);
	ctl_llr->config = *llr_config;
	spin_unlock_irqrestore(&ctl_llr->data_lock, irq_flags);

	return 0;
}

int sl_ctl_llr_policy_set(u8 ldev_num, u8 lgrp_num, u8 llr_num, struct sl_llr_policy *llr_policy)
{
	int                rtn;
	unsigned long      irq_flags;
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"policy set NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "policy set");

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return -EBADRQC;
	}

	rtn = sl_core_llr_policy_set(ldev_num, lgrp_num, llr_num, llr_policy);
	if (rtn) {
		sl_ctl_log_err(ctl_llr, LOG_NAME,
			"core_llr_policy_set failed [%d]", rtn);
		return rtn;
	}

	spin_lock_irqsave(&ctl_llr->data_lock, irq_flags);
	ctl_llr->policy = *llr_policy;
	spin_unlock_irqrestore(&ctl_llr->data_lock, irq_flags);

	return 0;
}

int sl_ctl_llr_setup(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"setup NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "setup");

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return -EBADRQC;
	}

	return sl_core_llr_setup(ldev_num, lgrp_num, llr_num,
				 sl_ctl_llr_setup_callback, ctl_llr, 0);
}

int sl_ctl_llr_start(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"start NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "start");

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return -EBADRQC;
	}

	return sl_core_llr_start(ldev_num, lgrp_num, llr_num,
				 sl_ctl_llr_start_callback, ctl_llr, 0);
}

int sl_ctl_llr_stop(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	struct sl_ctl_llr *ctl_llr;

	ctl_llr = sl_ctl_llr_get(ldev_num, lgrp_num, llr_num);
	if (!ctl_llr) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"stop NULL llr (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
			ldev_num, lgrp_num, llr_num);
		return 0;
	}

	sl_ctl_log_dbg(ctl_llr, LOG_NAME, "stop");

	if (sl_ctl_llr_is_deleting(ctl_llr)) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME, "del in progress");
		return -EBADRQC;
	}

	return sl_core_llr_stop(ldev_num, lgrp_num, llr_num);
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
		return SL_LLR_STATE_OFF;
	}

	rtn = sl_core_llr_state_get(ldev_num, lgrp_num, llr_num, &core_llr_state);
	if (rtn) {
		sl_ctl_log_dbg(ctl_llr, LOG_NAME,
			"core_llr_state_get failed [%d]", rtn);
		return SL_LLR_STATE_OFF;
	}

	switch (core_llr_state) {
	case SL_CORE_LLR_STATE_NEW:
		*state = SL_LLR_STATE_OFF;
		break;
	case SL_CORE_LLR_STATE_CONFIGURED:
	case SL_CORE_LLR_STATE_SETUP_TIMEOUT:
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
	case SL_CORE_LLR_STATE_CANCELING:
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

	return 0;
}
