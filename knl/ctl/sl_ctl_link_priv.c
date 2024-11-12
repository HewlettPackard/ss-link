// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include "uapi/sl_link.h"

#include "base/sl_ctl_log.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_lgrp_notif.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_priv.h"
#include "sl_ctl_link_counters.h"
#include "sl_core_link.h"
#include "sl_core_link_an.h"
#include "sl_core_str.h"

#define LOG_NAME SL_CTL_LINK_LOG_NAME

static int sl_ctl_link_up_notif_send(struct sl_ctl_lgrp *ctl_lgrp, struct sl_ctl_link *ctl_link,
	u64 info_map, u32 speed, u32 fec_mode, u32 fec_type)
{
	struct sl_lgrp_notif_info_link_up link_up_info;

	link_up_info.mode     = speed;
	link_up_info.fec_mode = fec_mode;
	link_up_info.fec_type = fec_type;

	return sl_ctl_lgrp_notif_enqueue(ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_LINK_UP,
		&link_up_info, sizeof(struct sl_lgrp_notif_info_link_up), info_map);
}

static int sl_ctl_link_up_fail_notif_send(struct sl_ctl_lgrp *ctl_lgrp, struct sl_ctl_link *ctl_link,
	u32 cause, struct sl_link_data *link_data, u64 info_map)
{
	struct sl_lgrp_notif_info_link_up_fail link_up_fail_info;

	link_up_fail_info.cause     = cause;
	link_up_fail_info.link_data = *link_data;

	return sl_ctl_lgrp_notif_enqueue(ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_LINK_UP_FAIL,
		&link_up_fail_info, sizeof(struct sl_lgrp_notif_info_link_up_fail), info_map);
}

static int sl_ctl_link_async_down_notif_send(struct sl_ctl_lgrp *ctl_lgrp, struct sl_ctl_link *ctl_link,
	u32 cause, struct sl_link_data *link_data, u64 info_map)
{
	struct sl_lgrp_notif_info_link_async_down link_async_down_info;

	link_async_down_info.cause     = cause;
	link_async_down_info.link_data = *link_data;

	return sl_ctl_lgrp_notif_enqueue(ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_LINK_ASYNC_DOWN,
		&link_async_down_info, sizeof(struct sl_lgrp_notif_info_link_async_down), info_map);
}

void sl_ctl_link_up_clock_start(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.start   = ktime_get();
	ctl_link->up_clock.elapsed = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

static void sl_ctl_link_up_clock_stop(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.elapsed = ktime_sub(ktime_get(), ctl_link->up_clock.start);
	ctl_link->up_clock.start   = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

void sl_ctl_link_up_clock_clear(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.elapsed = ktime_set(0, 0);
	ctl_link->up_clock.start   = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

void sl_ctl_link_up_attempt_clock_start(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.attempt_start   = ktime_get();
	ctl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

static void sl_ctl_link_up_attempt_clock_stop(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.attempt_elapsed = ktime_sub(ktime_get(), ctl_link->up_clock.attempt_start);
	ctl_link->up_clock.attempt_start   = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

void sl_ctl_link_up_attempt_clock_clear(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	ctl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	ctl_link->up_clock.attempt_start   = ktime_set(0, 0);
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

void sl_ctl_link_up_count_zero(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_count_lock, irq_flags);
	ctl_link->up_count = 0;
	spin_unlock_irqrestore(&ctl_link->up_count_lock, irq_flags);
}

static void sl_ctl_link_up_count_inc(struct sl_ctl_link *ctl_link)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&ctl_link->up_count_lock, irq_flags);
	ctl_link->up_count++;
	spin_unlock_irqrestore(&ctl_link->up_count_lock, irq_flags);
}

int sl_ctl_link_up_callback(void *tag, u32 core_state, u32 core_cause, u64 core_imap,
			    u32 core_speed, u32 core_fec_mode, u32 core_fec_type)
{
	struct sl_ctl_link *ctl_link;
	unsigned long irq_flags;

	ctl_link = tag;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up callback");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_ASYNC_UP_NOTIFIER);

	spin_lock_irqsave(&ctl_link->up_notif_lock, irq_flags);
	ctl_link->up_notif_state    = core_state;
	ctl_link->up_notif_cause    = core_cause;
	ctl_link->up_notif_imap     = core_imap;
	ctl_link->up_notif_speed    = core_speed;
	ctl_link->up_notif_fec_mode = core_fec_mode;
	ctl_link->up_notif_fec_type = core_fec_type;
	spin_unlock_irqrestore(&ctl_link->up_notif_lock, irq_flags);

	if (!queue_work(ctl_link->ctl_lgrp->ctl_ldev->workq, &ctl_link->up_notif_work))
		sl_ctl_log_warn(ctl_link, LOG_NAME, "up callback queue work failed");

	return 0;
}

void sl_ctl_link_up_callback_work(struct work_struct *work)
{
	struct sl_ctl_link *ctl_link = container_of(work, struct sl_ctl_link, up_notif_work);
	u32 core_state;
	u32 core_cause;
	u64 core_imap;
	u32 core_speed;
	u32 core_fec_mode;
	u32 core_fec_type;
	char core_imap_str[SL_LINK_INFO_STRLEN];
	struct sl_link_data link_data;
	int max_up_tries;
	unsigned long irq_flags;
	u32 up_count;
	int error;
	int rtn;
	u32 up_time;
	u32 total_time;
	bool is_canceled;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up callback work");

	mutex_lock(&ctl_link->async_mtx);

	spin_lock_irqsave(&ctl_link->up_notif_lock, irq_flags);
	core_state    = ctl_link->up_notif_state;
	core_cause    = ctl_link->up_notif_cause;
	core_imap     = ctl_link->up_notif_imap;
	core_speed    = ctl_link->up_notif_speed;
	core_fec_mode = ctl_link->up_notif_fec_mode;
	core_fec_type = ctl_link->up_notif_fec_type;
	spin_unlock_irqrestore(&ctl_link->up_notif_lock, irq_flags);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"up callback work (core_state = %s, core_cause = %s, core_imap = 0x%llx)",
		sl_core_link_state_str(core_state), sl_link_down_cause_str(core_cause), core_imap);

	sl_core_info_map_str(core_imap, core_imap_str, sizeof(core_imap_str));

	sl_ctl_link_up_attempt_clock_stop(ctl_link);

	switch (core_state) {
	case SL_CORE_LINK_STATE_UP:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP);
		sl_ctl_link_up_clock_stop(ctl_link);

		sl_ctl_link_up_clocks_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, &up_time, &total_time);
		sl_ctl_link_up_count_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, &up_count);
		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"up callback work (count = %d, up_time = %dms, total_time = %dms)",
			up_count, up_time, total_time);

		rtn = sl_ctl_link_up_notif_send(ctl_link->ctl_lgrp, ctl_link, core_imap,
			core_speed, core_fec_mode, core_fec_type);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"up callback work ctl_link_up_notif_send failed [%d]", rtn);

		sl_ctl_link_fec_mon_start(ctl_link);

		break;

	case SL_CORE_LINK_STATE_DOWN:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_FAIL);

		spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
		max_up_tries = ctl_link->config.link_up_tries_max;
		spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);

		sl_ctl_link_up_count_inc(ctl_link);
		sl_ctl_link_up_count_get(ctl_link->ctl_lgrp->ctl_ldev->num,
			ctl_link->ctl_lgrp->num, ctl_link->num, &up_count);

		/* check for fatal down causes */
		switch (core_cause) {
		case SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH:
		case SL_LINK_DOWN_CAUSE_AUTONEG_FAIL:
		case SL_LINK_DOWN_CAUSE_CONFIG:
		case SL_LINK_DOWN_CAUSE_UCW:
		case SL_LINK_DOWN_CAUSE_CCW:
		case SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE:
			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				core_cause, &link_data, core_imap);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);
			goto out;
		}

		/* check up tries */
		if ((up_count >= max_up_tries) && (max_up_tries != SL_LINK_INFINITE_UP_TRIES)) {
			sl_ctl_log_dbg(ctl_link, LOG_NAME, "up callback work out of up tries");
			sl_ctl_link_up_clock_clear(ctl_link);
			rtn = sl_core_link_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
				ctl_link->ctl_lgrp->num, ctl_link->num, &link_data);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work core_link_data_get failed [%d]", rtn);
			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				SL_LINK_DOWN_CAUSE_UP_TRIES, &link_data, core_imap);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);
			goto out;
		}

		/* canceled */
		spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
		is_canceled = ctl_link->is_canceled;
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		if (is_canceled) {
			sl_ctl_log_dbg(ctl_link, LOG_NAME, "up retry canceled");
			sl_ctl_link_up_clock_clear(ctl_link);
			rtn = sl_core_link_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
				ctl_link->ctl_lgrp->num, ctl_link->num, &link_data);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work core_link_data_get failed [%d]", rtn);
			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				SL_LINK_DOWN_CAUSE_CANCELED, &link_data, core_imap);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);
			goto out;
		}

		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_RETRY);
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "link up retry (ctl_link = 0x%p)", ctl_link);
		sl_ctl_link_up_attempt_clock_start(ctl_link);

		rtn = sl_core_link_up(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, sl_ctl_link_up_callback, ctl_link);
		if (rtn) {
			sl_ctl_log_err(ctl_link, LOG_NAME,
				"up callback work core_link_up failed [%d]", rtn);
			sl_ctl_link_up_clock_clear(ctl_link);
			sl_ctl_link_up_attempt_clock_clear(ctl_link);
			error = rtn;
			rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
				SL_LGRP_NOTIF_LINK_ERROR, &error, sizeof(error), core_imap);
			if (rtn)
				sl_ctl_log_warn(ctl_link, LOG_NAME,
					"up callback work ctl_lgrp_notif_enqueue failed [%d]", rtn);
			goto out;
		}
		break;

	default:
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"up callback work invalid (core_state = %u, core_imap = %s)", core_state, core_imap_str);
		sl_ctl_link_up_clock_clear(ctl_link);
		sl_ctl_link_up_attempt_clock_clear(ctl_link);
		error = -EBADRQC;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &error, sizeof(error), core_imap);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"up callback work ctl_lgrp_notif_enqueue failed [%d]", rtn);
		break;
	}

out:
	mutex_unlock(&ctl_link->async_mtx);
}

int sl_ctl_link_fault_intr_hdlr(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "link_fault_intr_hdlr");

	sl_ctl_link_fec_mon_stop(ctl_link);

	return 0;
}

int sl_ctl_link_fault_callback(void *tag, u32 core_state, u32 core_cause, u64 core_imap)
{
	struct sl_ctl_link *ctl_link;
	unsigned long irq_flags;

	ctl_link = tag;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "fault callback");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_ASYNC_FAULT_NOTIFIER);

	spin_lock_irqsave(&ctl_link->fault_notif_lock, irq_flags);
	ctl_link->fault_notif_state = core_state;
	ctl_link->fault_notif_cause = core_cause;
	ctl_link->fault_notif_imap  = core_imap;
	spin_unlock_irqrestore(&ctl_link->fault_notif_lock, irq_flags);

	if (!queue_work(ctl_link->ctl_lgrp->ctl_ldev->workq, &ctl_link->fault_notif_work))
		sl_ctl_log_warn(ctl_link, LOG_NAME, "fault callback queue work failed");

	return 0;
}

void sl_ctl_link_fault_callback_work(struct work_struct *work)
{
	struct sl_ctl_link *ctl_link = container_of(work, struct sl_ctl_link, fault_notif_work);
	unsigned long irq_flags;
	u32 core_state;
	u32 core_cause;
	u64 core_imap;
	char core_imap_str[SL_LINK_INFO_STRLEN];
	struct sl_link_data link_data;
	int error;
	int rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "fault callback work");

	sl_ctl_link_fec_mon_stop(ctl_link);

	cancel_work_sync(&ctl_link->up_notif_work);

	mutex_lock(&ctl_link->async_mtx);

	spin_lock_irqsave(&ctl_link->fault_notif_lock, irq_flags);
	core_state = ctl_link->fault_notif_state;
	core_cause = ctl_link->fault_notif_cause;
	core_imap  = ctl_link->fault_notif_imap;
	spin_unlock_irqrestore(&ctl_link->fault_notif_lock, irq_flags);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"fault callback work (core_state = %s, core_cause = %s, core_imap = 0x%llx)",
		sl_core_link_state_str(core_state), sl_link_down_cause_str(core_cause), core_imap);

	sl_core_info_map_str(core_imap, core_imap_str, sizeof(core_imap_str));

	sl_ctl_link_up_clock_clear(ctl_link);
	sl_ctl_link_up_attempt_clock_clear(ctl_link);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_ASYNC_DOWN);

		rtn = sl_core_link_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
			ctl_link->ctl_lgrp->num, ctl_link->num, &link_data);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"fault callback work core_link_data_get failed [%d]", rtn);
		rtn = sl_ctl_link_async_down_notif_send(ctl_link->ctl_lgrp, ctl_link,
			core_cause, &link_data, core_imap);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"fault callback work ctl_lgrp_notif_link_async_down_send failed [%d]", rtn);
		break;

	default:
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"fault callback work invalid (core_state = %u, core_imap = %s)", core_state, core_imap_str);
		error = -EBADRQC;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &error, sizeof(error), core_imap);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"fault callback work ctl_lgrp_notif_enqueue failed [%d]", rtn);
		break;
	}

	mutex_unlock(&ctl_link->async_mtx);
}

int sl_ctl_link_an_lp_caps_get_callback(void *tag, struct sl_link_caps *caps, u32 result)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;

	ctl_link = tag;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps get callback");

	switch (result) {
	case SL_CORE_LINK_LP_CAPS_DATA:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_AN_DATA,
			caps, sizeof(struct sl_link_caps), 0);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"lp caps data ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_TIMEOUT:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_AN_TIMEOUT, NULL, 0, 0);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"lp caps timeout ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_ERROR:
	default:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_AN_ERROR, NULL, 0, 0);
		if (rtn)
			sl_ctl_log_warn(ctl_link, LOG_NAME,
				"lp caps error ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	}

	sl_core_link_an_lp_caps_free(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, caps);

	return 0;
}

void sl_ctl_link_down_work(struct work_struct *work)
{
	int                  rtn;
	u64                  core_imap;
	unsigned long        irq_flags;
	struct sl_link_data  link_data;
	struct sl_ctl_link  *ctl_link;

	ctl_link = container_of(work, struct sl_ctl_link, down_work);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "link_down_work");

	rtn = sl_core_link_down(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_down failed [%d]", rtn);
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN_FAIL);
	}

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN);
	sl_ctl_link_up_clock_clear(ctl_link);
	sl_ctl_link_up_attempt_clock_clear(ctl_link);

	spin_lock_irqsave(&ctl_link->up_notif_lock, irq_flags);
	core_imap  = ctl_link->up_notif_imap;
	spin_unlock_irqrestore(&ctl_link->up_notif_lock, irq_flags);

	rtn = sl_core_link_data_get(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &link_data);
	if (rtn)
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"core_link_data_get failed [%d]", rtn);

	rtn = sl_ctl_link_async_down_notif_send(ctl_link->ctl_lgrp, ctl_link,
		ctl_link->down_cause, &link_data, core_imap);
	if (rtn)
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"ctl_link_async_down_notif_send failed [%d]", rtn);

}

void sl_ctl_link_config_get(struct sl_ctl_link *ctl_link, struct sl_link_config *link_config)
{
	spin_lock(&ctl_link->config_lock);
	*link_config = ctl_link->config;
	spin_unlock(&ctl_link->config_lock);
}

void sl_ctl_link_policy_get(struct sl_ctl_link *ctl_link, struct sl_link_policy *link_policy)
{
	spin_lock(&ctl_link->config_lock);
	*link_policy = ctl_link->policy;
	spin_unlock(&ctl_link->config_lock);
}
