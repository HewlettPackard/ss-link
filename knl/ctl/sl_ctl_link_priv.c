// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include <linux/sl_link.h>

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

#define SL_LINK_DOWN_CAUSE_FATAL_MASK (                \
		SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH   | \
		SL_LINK_DOWN_CAUSE_CONFIG            | \
		SL_LINK_DOWN_CAUSE_DOWNSHIFT         | \
		SL_LINK_DOWN_CAUSE_INTR_ENABLE       | \
		SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE | \
		SL_LINK_DOWN_CAUSE_SS200_CABLE       | \
		SL_LINK_DOWN_CAUSE_UCW               | \
		SL_LINK_DOWN_CAUSE_UPSHIFT)

void sl_ctl_link_is_canceled_set(struct sl_ctl_link *ctl_link, bool canceled)
{
	spin_lock(&ctl_link->data_lock);
	ctl_link->is_canceled = canceled;
	spin_unlock(&ctl_link->data_lock);
}

bool sl_ctl_link_is_canceled(struct sl_ctl_link *ctl_link)
{
	bool is_canceled;

	spin_lock(&ctl_link->data_lock);
	is_canceled = ctl_link->is_canceled;
	spin_unlock(&ctl_link->data_lock);

	return is_canceled;
}

static void sl_ctl_link_last_up_fail_cause_map_clr(struct sl_ctl_link *ctl_link)
{
	spin_lock(&ctl_link->data_lock);
	ctl_link->last_up_fail_cause_map = 0;
	ctl_link->last_up_fail_time      = 0;
	spin_unlock(&ctl_link->data_lock);
}

static void sl_ctl_link_last_up_fail_cause_map_set(struct sl_ctl_link *ctl_link, u64 last_up_fail_cause_map)
{
	spin_lock(&ctl_link->data_lock);
	ctl_link->last_up_fail_cause_map |= last_up_fail_cause_map;
	ctl_link->last_up_fail_time       = ktime_get_real_seconds();
	spin_unlock(&ctl_link->data_lock);
}

static bool sl_ctl_link_is_last_up_fail_cause_set(struct sl_ctl_link *ctl_link, u64 last_up_fail_cause)
{
	bool is_last_up_fail_cause_set;

	spin_lock(&ctl_link->data_lock);
	is_last_up_fail_cause_set = ctl_link->last_up_fail_cause_map & last_up_fail_cause;
	spin_unlock(&ctl_link->data_lock);

	return is_last_up_fail_cause_set;
}

static int sl_ctl_link_up_notif_send(struct sl_ctl_lgrp *ctl_lgrp, struct sl_ctl_link *ctl_link,
	u64 info_map, u32 speed, u32 fec_mode, u32 fec_type)
{
	union sl_lgrp_notif_info info;

	info.link_up.mode     = speed;
	info.link_up.fec_mode = fec_mode;
	info.link_up.fec_type = fec_type;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up notif send (speed = %u %s, fec mode = %u %s, fec type = %u)",
		speed, sl_lgrp_config_tech_str(speed), fec_mode, sl_lgrp_fec_mode_str(fec_mode), fec_type);

	return sl_ctl_lgrp_notif_enqueue(ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_LINK_UP,
		&info, info_map);
}

static int sl_ctl_link_up_fail_notif_send(struct sl_ctl_lgrp *ctl_lgrp, struct sl_ctl_link *ctl_link,
	u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;

	info.cause_map = cause_map;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up fail notif send (cause_map = 0x%llX)", cause_map);

	return sl_ctl_lgrp_notif_enqueue(ctl_lgrp, ctl_link->num, SL_LGRP_NOTIF_LINK_UP_FAIL,
		&info, info_map);
}

static int sl_ctl_async_link_down_notif_send(struct sl_ctl_link *ctl_link, u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;
	char                     cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	info.cause_map = cause_map;

	sl_link_down_cause_map_with_info_str(cause_map, cause_str, sizeof(cause_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"async_link_down_notif_send (core_cause_map = 0x%llX %s)", cause_map, cause_str);

	return sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
		SL_LGRP_NOTIF_LINK_ASYNC_DOWN, &info, info_map);
}

static int sl_ctl_link_down_notif_send(struct sl_ctl_link *ctl_link, u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;
	char                     cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	info.cause_map = cause_map;

	sl_link_down_cause_map_with_info_str(cause_map, cause_str, sizeof(cause_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"link_down_notif_send (core_cause_map = 0x%llX %s)", cause_map, cause_str);

	return sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
		SL_LGRP_NOTIF_LINK_DOWN, &info, info_map);
}

void sl_ctl_link_up_clock_start(struct sl_ctl_link *ctl_link)
{
	spin_lock(&ctl_link->up_clock.lock);
	ctl_link->up_clock.start         = ktime_get();
	ctl_link->up_clock.elapsed       = ktime_set(0, 0);
	ctl_link->up_clock.attempt_count = 0;
	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"clock start (time = %lld)", ctl_link->up_clock.start);
}

static void sl_ctl_link_up_clock_stop(struct sl_ctl_link *ctl_link)
{
	spin_lock(&ctl_link->up_clock.lock);
	ctl_link->up_clock.elapsed = ktime_sub(ktime_get(), ctl_link->up_clock.start);
	ctl_link->up_clock.start   = ktime_set(0, 0);
	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"clock stop (time = %lld)", ctl_link->up_clock.elapsed);
}

void sl_ctl_link_up_clock_reset(struct sl_ctl_link *ctl_link)
{
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "clock reset");

	spin_lock(&ctl_link->up_clock.lock);
	ctl_link->up_clock.elapsed         = ktime_set(0, 0);
	ctl_link->up_clock.start           = ktime_set(0, 0);
	ctl_link->up_clock.attempt_count   = 0;
	ctl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	ctl_link->up_clock.attempt_start   = ktime_set(0, 0);
	spin_unlock(&ctl_link->up_clock.lock);
}

void sl_ctl_link_up_clock_attempt_start(struct sl_ctl_link *ctl_link)
{
	spin_lock(&ctl_link->up_clock.lock);
	ctl_link->up_clock.attempt_start   = ktime_get();
	ctl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	ctl_link->up_clock.attempt_count++;
	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"clock attempt start (time = %lld)", ctl_link->up_clock.attempt_start);
}

static void sl_ctl_link_up_clock_attempt_stop(struct sl_ctl_link *ctl_link)
{
	spin_lock(&ctl_link->up_clock.lock);
	ctl_link->up_clock.attempt_elapsed = ktime_sub(ktime_get(), ctl_link->up_clock.attempt_start);
	ctl_link->up_clock.attempt_start   = ktime_set(0, 0);
	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"clock attempt stop (time = %lld)", ctl_link->up_clock.attempt_elapsed);
}

static void sl_ctl_link_state_stopping_set(struct sl_ctl_link *ctl_link)
{
	u32           link_state;

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "stopping set - already stopping");
		spin_unlock(&ctl_link->data_lock);
		return;
	case SL_LINK_STATE_STARTING:
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "stopping set - stopping");
		spin_unlock(&ctl_link->data_lock);
		return;
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "stopping set - invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		return;
	}
}

int sl_ctl_link_up_callback(void *tag, struct sl_core_link_up_info *up_info)
{
	struct sl_ctl_link         *ctl_link;
	char                        core_imap_str[SL_LINK_INFO_STRLEN];
	int                         max_up_tries;
	u32                         up_count;
	int                         rtn;
	s64                         up_time;
	s64                         total_time;
	union sl_lgrp_notif_info    info;
	struct sl_core_link_up_info core_link_up_info;

	ctl_link = tag;
	core_link_up_info = *up_info;

	sl_core_info_map_str(core_link_up_info.info_map, core_imap_str, sizeof(core_imap_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"up callback (core_state = %u %s, core_cause_map = 0x%llX, info_map = %s (0x%llx))",
		core_link_up_info.state, sl_core_link_state_str(core_link_up_info.state),
		core_link_up_info.cause_map, core_imap_str, core_link_up_info.info_map);

	sl_ctl_link_last_up_fail_cause_map_clr(ctl_link);
	sl_ctl_link_last_up_fail_cause_map_set(ctl_link, core_link_up_info.cause_map);

	sl_ctl_link_up_clock_attempt_stop(ctl_link);

	switch (core_link_up_info.state) {
	case SL_CORE_LINK_STATE_UP:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP);
		sl_ctl_link_up_clock_stop(ctl_link);

		sl_ctl_link_up_clocks_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, &up_time, &total_time);
		sl_ctl_link_up_count_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, &up_count);

		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"up callback (count = %d, up_time = %lldms, total_time = %lldms)",
			up_count, up_time, total_time);

		sl_ctl_link_fec_mon_start(ctl_link);

		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_UP);

		rtn = sl_ctl_link_up_notif_send(ctl_link->ctl_lgrp, ctl_link, core_link_up_info.info_map,
			core_link_up_info.speed, core_link_up_info.fec_mode, core_link_up_info.fec_type);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"up callback ctl_link_up_notif_send failed [%d]", rtn);
		return 0;

	case SL_CORE_LINK_STATE_DOWN:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_FAIL);

		spin_lock(&ctl_link->config_lock);
		max_up_tries = ctl_link->config.link_up_tries_max;
		spin_unlock(&ctl_link->config_lock);

		/* canceled */
		if (sl_ctl_link_is_canceled(ctl_link)) {
			sl_ctl_link_up_clock_reset(ctl_link);

			sl_ctl_log_dbg(ctl_link, LOG_NAME, "up retry canceled");

			sl_ctl_link_state_stopping_set(ctl_link);

			flush_work(&ctl_link->ctl_lgrp->notif_work);
			sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
			complete_all(&ctl_link->down_complete);

			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				sl_ctl_link_last_up_fail_cause_map_get(ctl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);

			return 0;
		}

		/* check up tries */
		sl_ctl_link_up_count_get(ctl_link->ctl_lgrp->ctl_ldev->num,
			ctl_link->ctl_lgrp->num, ctl_link->num, &up_count);
		if ((up_count >= max_up_tries) && (max_up_tries != SL_LINK_INFINITE_UP_TRIES)) {
			sl_ctl_link_up_clock_reset(ctl_link);

			sl_ctl_log_dbg(ctl_link, LOG_NAME, "up callback work out of up tries");

			sl_ctl_link_state_stopping_set(ctl_link);

			sl_ctl_link_last_up_fail_cause_map_set(ctl_link, SL_LINK_DOWN_CAUSE_UP_TRIES_MAP);

			flush_work(&ctl_link->ctl_lgrp->notif_work);
			sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

			complete_all(&ctl_link->down_complete);

			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				sl_ctl_link_last_up_fail_cause_map_get(ctl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);
			return 0;
		}

		/* check fatal down causes */
		if (sl_ctl_link_is_last_up_fail_cause_set(ctl_link, SL_LINK_DOWN_CAUSE_FATAL_MASK)) {
			sl_ctl_link_up_clock_reset(ctl_link);

			sl_ctl_log_dbg(ctl_link, LOG_NAME, "up callback work fatal down cause");

			sl_ctl_link_state_stopping_set(ctl_link);

			flush_work(&ctl_link->ctl_lgrp->notif_work);

			sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
			complete_all(&ctl_link->down_complete);

			rtn = sl_ctl_link_up_fail_notif_send(ctl_link->ctl_lgrp, ctl_link,
				sl_ctl_link_last_up_fail_cause_map_get(ctl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
					"up callback work ctl_link_up_fail_notif_send failed [%d]", rtn);

			return 0;
		}

		sl_ctl_log_dbg(ctl_link, LOG_NAME, "link up retry (ctl_link = 0x%p)", ctl_link);

		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_RETRY);

		sl_ctl_link_up_clock_attempt_start(ctl_link);

		rtn = sl_core_link_up(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
			ctl_link->num, sl_ctl_link_up_callback, ctl_link);
		if (rtn) {
			sl_ctl_link_up_clock_reset(ctl_link);

			sl_ctl_link_state_stopping_set(ctl_link);

			sl_ctl_log_err_trace(ctl_link, LOG_NAME,
				"up callback work core_link_up failed [%d]", rtn);

			flush_work(&ctl_link->ctl_lgrp->notif_work);
			sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

			complete_all(&ctl_link->down_complete);

			info.error = rtn;
			rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
				SL_LGRP_NOTIF_LINK_ERROR, &info, core_link_up_info.info_map);
			if (rtn)
				sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
					"up callback work ctl_lgrp_notif_enqueue failed [%d]", rtn);


			return info.error;
		}
		return 0;

	default:
		sl_ctl_link_up_clock_reset(ctl_link);
		sl_ctl_link_state_stopping_set(ctl_link);

		sl_ctl_log_err(ctl_link, LOG_NAME,
			"up callback work invalid (core_state = %u, core_imap = %s)",
			core_link_up_info.state, core_imap_str);

		flush_work(&ctl_link->ctl_lgrp->notif_work);
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

		complete_all(&ctl_link->down_complete);

		info.error = -EBADRQC;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &info, core_link_up_info.info_map);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"up callback work ctl_lgrp_notif_enqueue failed [%d]", rtn);

		return info.error;
	}
}

int sl_ctl_link_fault_start_callback(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "fault start callback");

	sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_STOPPING);

	sl_ctl_link_fec_mon_stop(ctl_link);

	return 0;
}

int sl_ctl_link_fault_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_imap)
{
	struct sl_ctl_link       *ctl_link;
	char                      core_imap_str[SL_LINK_INFO_STRLEN];
	int                       rtn;
	union sl_lgrp_notif_info  info;

	ctl_link = tag;

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_FAULT);

	sl_core_info_map_str(core_imap, core_imap_str, sizeof(core_imap_str));

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"fault callback (core_state = %u %s, core_cause_map = 0x%llX, core_imap = %s (0x%llx))",
		core_state, sl_core_link_state_str(core_state), core_cause_map, core_imap_str, core_imap);

	sl_ctl_link_up_clock_reset(ctl_link);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		flush_work(&ctl_link->ctl_lgrp->notif_work);
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctl_link->down_complete);

		rtn = sl_ctl_async_link_down_notif_send(ctl_link, core_cause_map, core_imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"fault callback ctl_async_link_down_notif_send failed [%d]", rtn);

		return 0;

	default:
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"fault callback invalid (core_state = %u)", core_state);

		flush_work(&ctl_link->ctl_lgrp->notif_work);
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

		complete_all(&ctl_link->down_complete);

		info.error = -EBADRQC;
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &info, core_imap);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"fault callback ctl_lgrp_notif_enqueue failed [%d]", rtn);

		return 0;
	}
}

int sl_ctl_link_an_lp_caps_get_callback(void *tag, struct sl_link_caps *caps, u32 result)
{
	int                       rtn;
	struct sl_ctl_link       *ctl_link;
	union sl_lgrp_notif_info  info;

	ctl_link          =  tag;
	info.lp_link_caps = *caps;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps get callback");

	switch (result) {
	case SL_CORE_LINK_LP_CAPS_DATA:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_AN_DATA, &info, 0);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"lp caps data ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_TIMEOUT:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_AN_TIMEOUT, NULL, 0);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"lp caps timeout ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_ERROR:
	default:
		rtn = sl_ctl_lgrp_notif_enqueue(ctl_link->ctl_lgrp, ctl_link->num,
			SL_LGRP_NOTIF_AN_ERROR, NULL, 0);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"lp caps error ctl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	}

	return 0;
}

int sl_ctl_link_down_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_info_map)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	char                cause_str[100];

	ctl_link = tag;

	sl_link_down_cause_map_with_info_str(core_cause_map, cause_str, sizeof(cause_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"down callback (core_state = %u %s, core_cause = 0x%llX %s, core_info_map = %llu)",
		core_state, sl_core_link_state_str(core_state), core_cause_map, cause_str, core_info_map);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN);

		if (core_cause_map == SL_LINK_DOWN_CAUSE_CANCELED)
			SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_CANCELED);

		sl_ctl_link_fec_mon_stop(ctl_link);
		cancel_work_sync(&ctl_link->fec_mon_timer_work);

		flush_work(&ctl_link->ctl_lgrp->notif_work);
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctl_link->down_complete);

		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"down callback (down_complete = 0x%p)", &ctl_link->down_complete);

		rtn = sl_ctl_link_down_notif_send(ctl_link, core_cause_map, core_info_map);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"down callback ctl_link_down_notif_send failed [%d]", rtn);
		return 0;
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down callback invalid (core_state = %u %s)",
			core_state, sl_core_link_state_str(core_state));
		return -EBADRQC;
	}
}

static int sl_ctl_link_async_down_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_info_map)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	char                cause_str[100];

	ctl_link = tag;

	sl_link_down_cause_map_with_info_str(core_cause_map, cause_str, sizeof(cause_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"async down callback (core_state = %u %s, core_cause = 0x%llX %s, core_info_map = %llu)",
		core_state, sl_core_link_state_str(core_state), core_cause_map, cause_str, core_info_map);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN);

		sl_ctl_link_fec_mon_stop(ctl_link);
		cancel_work_sync(&ctl_link->fec_mon_timer_work);

		flush_work(&ctl_link->ctl_lgrp->notif_work);
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctl_link->down_complete);

		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"async down callback (down_complete = 0x%p)", &ctl_link->down_complete);

		rtn = sl_ctl_async_link_down_notif_send(ctl_link, core_cause_map, core_info_map);
		if (rtn)
			sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
				"async down callback ctl_async_link_down_notif_send failed [%d]", rtn);
		return 0;
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "async down callback invalid (core_state = %u %s)",
			core_state, sl_core_link_state_str(core_state));
		return -EBADRQC;
	}
}

int sl_ctl_link_async_down(struct sl_ctl_link *ctl_link, u64 down_cause_map)
{
	int                  rtn;
	u32                  link_state;
	char                 cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	sl_link_down_cause_map_with_info_str(down_cause_map, cause_str, sizeof(cause_str));
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "async_down (down_cause_map = 0x%llX %s)", down_cause_map, cause_str);

	sl_ctl_link_up_clock_reset(ctl_link);

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "async_down - stopping");
		spin_unlock(&ctl_link->data_lock);

		rtn = sl_core_link_down(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
			sl_ctl_link_async_down_callback, ctl_link, down_cause_map);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME,
				"core_link_down failed [%d]", rtn);
			return rtn;
		}

		return 0;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "async_down - already down");
		spin_unlock(&ctl_link->data_lock);
		return 0;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "async_down - already stopping");
		spin_unlock(&ctl_link->data_lock);
		return 0;
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "async_down - invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		return -EBADRQC;
	}
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

void sl_ctl_link_state_set(struct sl_ctl_link *ctl_link, u32 link_state)
{
	spin_lock(&ctl_link->data_lock);
	ctl_link->state = link_state;
	spin_unlock(&ctl_link->data_lock);
}

u32 sl_ctl_link_state_get(struct sl_ctl_link *ctl_link)
{
	u32 state;

	spin_lock(&ctl_link->data_lock);
	state = ctl_link->state;
	spin_unlock(&ctl_link->data_lock);

	return state;
}

u64 sl_ctl_link_last_up_fail_cause_map_get(struct sl_ctl_link *ctl_link)
{
	u64 last_up_fail_cause_map;

	spin_lock(&ctl_link->data_lock);
	last_up_fail_cause_map = ctl_link->last_up_fail_cause_map;
	spin_unlock(&ctl_link->data_lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"last_up_fail_cause_map_get (last_up_fail_cause_map = 0x%llX)", last_up_fail_cause_map);

	return last_up_fail_cause_map;
}

void sl_ctl_link_last_up_fail_cause_info_get(struct sl_ctl_link *ctl_link, u64 *last_up_fail_cause_map,
	time64_t *last_up_fail_time)
{
	spin_lock(&ctl_link->data_lock);
	*last_up_fail_cause_map = ctl_link->last_up_fail_cause_map;
	*last_up_fail_time = ctl_link->last_up_fail_time;
	spin_unlock(&ctl_link->data_lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"last up fail time show (cause_map = 0x%llX, time = %lld %ptTt %ptTd)",
		*last_up_fail_cause_map, *last_up_fail_time, last_up_fail_time, last_up_fail_time);
}
