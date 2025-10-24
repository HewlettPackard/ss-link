// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include <linux/hpe/sl/sl_link.h>

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_link_priv.h"
#include "sl_ctrl_link_counters.h"
#include "sl_core_link.h"
#include "sl_core_link_an.h"
#include "sl_core_str.h"

#define LOG_NAME SL_CTRL_LINK_LOG_NAME

#define SL_LINK_DOWN_CAUSE_FATAL_MASK (                \
		SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH   | \
		SL_LINK_DOWN_CAUSE_CONFIG            | \
		SL_LINK_DOWN_CAUSE_DOWNSHIFT         | \
		SL_LINK_DOWN_CAUSE_INTR_ENABLE       | \
		SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE | \
		SL_LINK_DOWN_CAUSE_SS200_CABLE       | \
		SL_LINK_DOWN_CAUSE_UCW               | \
		SL_LINK_DOWN_CAUSE_HIGH_TEMP         | \
		SL_LINK_DOWN_CAUSE_NO_MEDIA          | \
		SL_LINK_DOWN_CAUSE_UPSHIFT)

void sl_ctrl_link_is_canceled_set(struct sl_ctrl_link *ctrl_link, bool canceled)
{
	spin_lock(&ctrl_link->data_lock);
	ctrl_link->is_canceled = canceled;
	spin_unlock(&ctrl_link->data_lock);
}

bool sl_ctrl_link_is_canceled(struct sl_ctrl_link *ctrl_link)
{
	bool is_canceled;

	spin_lock(&ctrl_link->data_lock);
	is_canceled = ctrl_link->is_canceled;
	spin_unlock(&ctrl_link->data_lock);

	return is_canceled;
}

static void sl_ctrl_link_last_up_fail_cause_map_clr(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->data_lock);
	ctrl_link->last_up_fail_cause_map = 0;
	ctrl_link->last_up_fail_time      = 0;
	spin_unlock(&ctrl_link->data_lock);
}

static void sl_ctrl_link_last_up_fail_cause_map_set(struct sl_ctrl_link *ctrl_link, u64 last_up_fail_cause_map)
{
	spin_lock(&ctrl_link->data_lock);
	ctrl_link->last_up_fail_cause_map |= last_up_fail_cause_map;
	ctrl_link->last_up_fail_time       = ktime_get_real_seconds();
	spin_unlock(&ctrl_link->data_lock);
}

static bool sl_ctrl_link_is_last_up_fail_cause_set(struct sl_ctrl_link *ctrl_link, u64 last_up_fail_cause)
{
	bool is_last_up_fail_cause_set;

	spin_lock(&ctrl_link->data_lock);
	is_last_up_fail_cause_set = ctrl_link->last_up_fail_cause_map & last_up_fail_cause;
	spin_unlock(&ctrl_link->data_lock);

	return is_last_up_fail_cause_set;
}

static int sl_ctrl_link_up_notif_send(struct sl_ctrl_lgrp *ctrl_lgrp, struct sl_ctrl_link *ctrl_link,
	u64 info_map, u32 speed, u32 fec_mode, u32 fec_type)
{
	union sl_lgrp_notif_info info;

	info.link_up.mode     = speed;
	info.link_up.fec_mode = fec_mode;
	info.link_up.fec_type = fec_type;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up notif send (speed = %u %s, fec mode = %u %s, fec type = %u)",
		speed, sl_lgrp_config_tech_str(speed), fec_mode, sl_lgrp_fec_mode_str(fec_mode), fec_type);

	return sl_ctrl_lgrp_notif_enqueue(ctrl_lgrp, ctrl_link->num, SL_LGRP_NOTIF_LINK_UP,
		&info, info_map);
}

static int sl_ctrl_link_up_fail_notif_send(struct sl_ctrl_lgrp *ctrl_lgrp, struct sl_ctrl_link *ctrl_link,
	u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;

	info.cause_map = cause_map;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up fail notif send (cause_map = 0x%llX)", cause_map);

	return sl_ctrl_lgrp_notif_enqueue(ctrl_lgrp, ctrl_link->num, SL_LGRP_NOTIF_LINK_UP_FAIL,
		&info, info_map);
}

static int sl_ctrl_async_link_down_notif_send(struct sl_ctrl_link *ctrl_link, u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;
	char                     cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	info.cause_map = cause_map;

	sl_link_down_cause_map_with_info_str(cause_map, cause_str, sizeof(cause_str));
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"async_link_down_notif_send (core_cause_map = 0x%llX %s)", cause_map, cause_str);

	return sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
		SL_LGRP_NOTIF_LINK_ASYNC_DOWN, &info, info_map);
}

static int sl_ctrl_link_down_notif_send(struct sl_ctrl_link *ctrl_link, u64 cause_map, u64 info_map)
{
	union sl_lgrp_notif_info info;
	char                     cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	info.cause_map = cause_map;

	sl_link_down_cause_map_with_info_str(cause_map, cause_str, sizeof(cause_str));
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"link_down_notif_send (core_cause_map = 0x%llX %s)", cause_map, cause_str);

	return sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
		SL_LGRP_NOTIF_LINK_DOWN, &info, info_map);
}

void sl_ctrl_link_up_clock_start(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->up_clock.lock);
	ctrl_link->up_clock.start         = ktime_get();
	ctrl_link->up_clock.elapsed       = ktime_set(0, 0);
	ctrl_link->up_clock.attempt_count = 0;
	spin_unlock(&ctrl_link->up_clock.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"clock start (time = %lld)", ctrl_link->up_clock.start);
}

static void sl_ctrl_link_up_clock_stop(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->up_clock.lock);
	ctrl_link->up_clock.elapsed = ktime_sub(ktime_get(), ctrl_link->up_clock.start);
	ctrl_link->up_clock.start   = ktime_set(0, 0);
	ctrl_link->up_clock.up      = ktime_get();
	spin_unlock(&ctrl_link->up_clock.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"clock stop (time = %lld)", ctrl_link->up_clock.elapsed);
}

void sl_ctrl_link_up_clock_reset(struct sl_ctrl_link *ctrl_link)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "clock reset");

	spin_lock(&ctrl_link->up_clock.lock);
	ctrl_link->up_clock.elapsed         = ktime_set(0, 0);
	ctrl_link->up_clock.start           = ktime_set(0, 0);
	ctrl_link->up_clock.attempt_count   = 0;
	ctrl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	ctrl_link->up_clock.attempt_start   = ktime_set(0, 0);
	ctrl_link->up_clock.up              = ktime_set(0, 0);
	spin_unlock(&ctrl_link->up_clock.lock);
}

void sl_ctrl_link_up_clock_attempt_start(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->up_clock.lock);
	ctrl_link->up_clock.attempt_start   = ktime_get();
	ctrl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	ctrl_link->up_clock.attempt_count++;
	spin_unlock(&ctrl_link->up_clock.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"clock attempt start (time = %lld)", ctrl_link->up_clock.attempt_start);
}

static void sl_ctrl_link_up_clock_attempt_stop(struct sl_ctrl_link *ctrl_link)
{
	spin_lock(&ctrl_link->up_clock.lock);
	ctrl_link->up_clock.attempt_elapsed = ktime_sub(ktime_get(), ctrl_link->up_clock.attempt_start);
	ctrl_link->up_clock.attempt_start   = ktime_set(0, 0);
	spin_unlock(&ctrl_link->up_clock.lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"clock attempt stop (time = %lld)", ctrl_link->up_clock.attempt_elapsed);
}

static void sl_ctrl_link_state_stopping_set(struct sl_ctrl_link *ctrl_link)
{
	u32           link_state;

	spin_lock(&ctrl_link->data_lock);
	link_state = ctrl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STOPPING:
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "stopping set - already stopping");
		spin_unlock(&ctrl_link->data_lock);
		return;
	case SL_LINK_STATE_STARTING:
	case SL_LINK_STATE_UP:
		ctrl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "stopping set - stopping");
		spin_unlock(&ctrl_link->data_lock);
		return;
	default:
		sl_ctrl_log_err(ctrl_link, LOG_NAME, "stopping set - invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctrl_link->data_lock);
		return;
	}
}

int sl_ctrl_link_up_callback(void *tag, struct sl_core_link_up_info *up_info)
{
	struct sl_ctrl_link         *ctrl_link;
	char                         core_imap_str[SL_LINK_INFO_STRLEN];
	int                          max_up_tries;
	u32                          up_count;
	int                          rtn;
	s64                          attempt_time;
	s64                          total_time;
	s64                          up_time;
	union sl_lgrp_notif_info     info;
	struct sl_core_link_up_info  core_link_up_info;

	ctrl_link = tag;
	core_link_up_info = *up_info;

	sl_core_info_map_str(core_link_up_info.info_map, core_imap_str, sizeof(core_imap_str));
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"up callback (core_state = %u %s, core_cause_map = 0x%llX, info_map = %s (0x%llx))",
		core_link_up_info.state, sl_core_link_state_str(core_link_up_info.state),
		core_link_up_info.cause_map, core_imap_str, core_link_up_info.info_map);

	sl_ctrl_link_last_up_fail_cause_map_clr(ctrl_link);
	sl_ctrl_link_last_up_fail_cause_map_set(ctrl_link, core_link_up_info.cause_map);

	sl_ctrl_link_up_clock_attempt_stop(ctrl_link);

	switch (core_link_up_info.state) {
	case SL_CORE_LINK_STATE_UP:
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_UP);
		sl_ctrl_link_up_clock_stop(ctrl_link);

		sl_ctrl_link_up_clocks_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
			ctrl_link->num, &attempt_time, &total_time, &up_time);
		sl_ctrl_link_up_count_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
			ctrl_link->num, &up_count);

		sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"up callback (count = %d, attempt_time = %lldms, total_time = %lldms)",
			up_count, attempt_time, total_time);

		sl_ctrl_link_fec_mon_start(ctrl_link);

		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_UP);

		rtn = sl_ctrl_link_up_notif_send(ctrl_link->ctrl_lgrp, ctrl_link, core_link_up_info.info_map,
			core_link_up_info.speed, core_link_up_info.fec_mode, core_link_up_info.fec_type);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"up callback ctrl_link_up_notif_send failed [%d]", rtn);
		return 0;

	case SL_CORE_LINK_STATE_DOWN:
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_UP_FAIL);

		spin_lock(&ctrl_link->config_lock);
		max_up_tries = ctrl_link->config.link_up_tries_max;
		spin_unlock(&ctrl_link->config_lock);

		/* canceled */
		if (sl_ctrl_link_is_canceled(ctrl_link)) {
			sl_ctrl_link_up_clock_reset(ctrl_link);

			sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up retry canceled");

			sl_ctrl_link_state_stopping_set(ctrl_link);

			flush_work(&ctrl_link->ctrl_lgrp->notif_work);
			sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);
			complete_all(&ctrl_link->down_complete);

			rtn = sl_ctrl_link_up_fail_notif_send(ctrl_link->ctrl_lgrp, ctrl_link,
				sl_ctrl_link_last_up_fail_cause_map_get(ctrl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"up callback work ctrl_link_up_fail_notif_send failed [%d]", rtn);

			return 0;
		}

		/* check up tries */
		sl_ctrl_link_up_count_get(ctrl_link->ctrl_lgrp->ctrl_ldev->num,
			ctrl_link->ctrl_lgrp->num, ctrl_link->num, &up_count);
		if ((up_count >= max_up_tries) && (max_up_tries != SL_LINK_INFINITE_UP_TRIES)) {
			sl_ctrl_link_up_clock_reset(ctrl_link);

			sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up callback work out of up tries");

			sl_ctrl_link_state_stopping_set(ctrl_link);

			sl_ctrl_link_last_up_fail_cause_map_set(ctrl_link, SL_LINK_DOWN_CAUSE_UP_TRIES_MAP);

			flush_work(&ctrl_link->ctrl_lgrp->notif_work);
			sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);

			complete_all(&ctrl_link->down_complete);

			rtn = sl_ctrl_link_up_fail_notif_send(ctrl_link->ctrl_lgrp, ctrl_link,
				sl_ctrl_link_last_up_fail_cause_map_get(ctrl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"up callback work ctrl_link_up_fail_notif_send failed [%d]", rtn);
			return 0;
		}

		/* check fatal down causes */
		if (sl_ctrl_link_is_last_up_fail_cause_set(ctrl_link, SL_LINK_DOWN_CAUSE_FATAL_MASK)) {
			sl_ctrl_link_up_clock_reset(ctrl_link);

			sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "up callback work fatal down cause");

			sl_ctrl_link_state_stopping_set(ctrl_link);

			flush_work(&ctrl_link->ctrl_lgrp->notif_work);

			sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);
			complete_all(&ctrl_link->down_complete);

			rtn = sl_ctrl_link_up_fail_notif_send(ctrl_link->ctrl_lgrp, ctrl_link,
				sl_ctrl_link_last_up_fail_cause_map_get(ctrl_link), core_link_up_info.info_map);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"up callback work ctrl_link_up_fail_notif_send failed [%d]", rtn);

			return 0;
		}

		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link up retry (ctrl_link = 0x%p)", ctrl_link);

		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_UP_RETRY);

		sl_ctrl_link_up_clock_attempt_start(ctrl_link);

		rtn = sl_core_link_up(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num,
			ctrl_link->num, sl_ctrl_link_up_callback, ctrl_link);
		if (rtn) {
			sl_ctrl_link_up_clock_reset(ctrl_link);

			sl_ctrl_link_state_stopping_set(ctrl_link);

			sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
				"up callback work core_link_up failed [%d]", rtn);

			flush_work(&ctrl_link->ctrl_lgrp->notif_work);
			sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);

			complete_all(&ctrl_link->down_complete);

			info.error = rtn;
			rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
				SL_LGRP_NOTIF_LINK_ERROR, &info, core_link_up_info.info_map);
			if (rtn)
				sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					"up callback work ctrl_lgrp_notif_enqueue failed [%d]", rtn);


			return info.error;
		}
		return 0;

	default:
		sl_ctrl_link_up_clock_reset(ctrl_link);
		sl_ctrl_link_state_stopping_set(ctrl_link);

		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"up callback work invalid (core_state = %u, core_imap = %s)",
			core_link_up_info.state, core_imap_str);

		flush_work(&ctrl_link->ctrl_lgrp->notif_work);
		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);

		complete_all(&ctrl_link->down_complete);

		info.error = -EBADRQC;
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &info, core_link_up_info.info_map);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"up callback work ctrl_lgrp_notif_enqueue failed [%d]", rtn);

		return info.error;
	}
}

int sl_ctrl_link_fault_start_callback(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctrl_link *ctrl_link;

	ctrl_link = sl_ctrl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctrl_link) {
		sl_ctrl_log_err(NULL, LOG_NAME,
			"NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "fault start callback");

	sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_STOPPING);

	sl_ctrl_link_fec_mon_stop(ctrl_link);

	return 0;
}

int sl_ctrl_link_fault_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_imap)
{
	struct sl_ctrl_link      *ctrl_link;
	char                      core_imap_str[SL_LINK_INFO_STRLEN];
	int                       rtn;
	union sl_lgrp_notif_info  info;

	ctrl_link = tag;

	SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_FAULT);

	sl_core_info_map_str(core_imap, core_imap_str, sizeof(core_imap_str));

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"fault callback (core_state = %u %s, core_cause_map = 0x%llX, core_imap = %s (0x%llx))",
		core_state, sl_core_link_state_str(core_state), core_cause_map, core_imap_str, core_imap);

	sl_ctrl_link_up_clock_reset(ctrl_link);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		flush_work(&ctrl_link->ctrl_lgrp->notif_work);
		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctrl_link->down_complete);

		rtn = sl_ctrl_async_link_down_notif_send(ctrl_link, core_cause_map, core_imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"fault callback async_link_down_notif_send failed [%d]", rtn);

		return 0;

	default:
		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"fault callback invalid (core_state = %u)", core_state);

		flush_work(&ctrl_link->ctrl_lgrp->notif_work);
		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);

		complete_all(&ctrl_link->down_complete);

		info.error = -EBADRQC;
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
			SL_LGRP_NOTIF_LINK_ERROR, &info, core_imap);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"fault callback ctrl_lgrp_notif_enqueue failed [%d]", rtn);

		return 0;
	}
}

int sl_ctrl_link_an_lp_caps_get_callback(void *tag, struct sl_link_caps *caps, u32 result)
{
	int                       rtn;
	struct sl_ctrl_link      *ctrl_link;
	union sl_lgrp_notif_info  info;

	ctrl_link          =  tag;
	info.lp_link_caps = *caps;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "an lp caps get callback");

	switch (result) {
	case SL_CORE_LINK_LP_CAPS_DATA:
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
			SL_LGRP_NOTIF_AN_DATA, &info, 0);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"lp caps data ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_TIMEOUT:
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
			SL_LGRP_NOTIF_AN_TIMEOUT, NULL, 0);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"lp caps timeout ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	case SL_CORE_LINK_LP_CAPS_ERROR:
	default:
		rtn = sl_ctrl_lgrp_notif_enqueue(ctrl_link->ctrl_lgrp, ctrl_link->num,
			SL_LGRP_NOTIF_AN_ERROR, NULL, 0);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"lp caps error ctrl_lgrp_notif_enqueue failed [%d[", rtn);
		break;
	}

	return 0;
}

int sl_ctrl_link_down_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_info_map)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	char                 cause_str[100];

	ctrl_link = tag;

	sl_link_down_cause_map_with_info_str(core_cause_map, cause_str, sizeof(cause_str));
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"down callback (core_state = %u %s, core_cause = 0x%llX %s, core_info_map = %llu)",
		core_state, sl_core_link_state_str(core_state), core_cause_map, cause_str, core_info_map);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_DOWN);

		if (core_cause_map & SL_LINK_DOWN_CAUSE_CANCELED)
			SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_UP_CANCELED);

		sl_ctrl_link_fec_mon_stop(ctrl_link);
		cancel_work_sync(&ctrl_link->fec_mon_timer_work);

		flush_work(&ctrl_link->ctrl_lgrp->notif_work);
		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctrl_link->down_complete);

		sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"down callback (down_complete = 0x%p)", &ctrl_link->down_complete);

		rtn = sl_ctrl_link_down_notif_send(ctrl_link, core_cause_map, core_info_map);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"down callback ctrl_link_down_notif_send failed [%d]", rtn);
		return 0;
	default:
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "down callback invalid (core_state = %u %s)",
			core_state, sl_core_link_state_str(core_state));
		return -EBADRQC;
	}
}

static int sl_ctrl_link_async_down_callback(void *tag, u32 core_state, u64 core_cause_map, u64 core_info_map)
{
	int                  rtn;
	struct sl_ctrl_link *ctrl_link;
	char                 cause_str[100];

	ctrl_link = tag;

	if (!sl_ctrl_link_kref_get_unless_zero(ctrl_link)) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"async_down kref_get_unless_zero failed (ctrl_link = 0x%p)", ctrl_link);
		return -EBADRQC;
	}

	sl_link_down_cause_map_with_info_str(core_cause_map, cause_str, sizeof(cause_str));

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"async down callback (core_state = %u %s, core_cause = 0x%llX %s, core_info_map = %llu)",
		core_state, sl_core_link_state_str(core_state), core_cause_map, cause_str, core_info_map);

	switch (core_state) {
	case SL_CORE_LINK_STATE_DOWN:
		SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_DOWN);

		if (core_cause_map & SL_LINK_DOWN_CAUSE_UCW)
			SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_DOWN_UCW_CAUSE);

		if (core_cause_map & SL_LINK_DOWN_CAUSE_CCW)
			SL_CTRL_LINK_COUNTER_INC(ctrl_link, LINK_DOWN_CCW_CAUSE);

		sl_ctrl_link_fec_mon_stop(ctrl_link);
		cancel_work_sync(&ctrl_link->fec_mon_timer_work);

		flush_work(&ctrl_link->ctrl_lgrp->notif_work);
		sl_ctrl_link_state_set(ctrl_link, SL_LINK_STATE_DOWN);
		complete_all(&ctrl_link->down_complete);

		sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
			"async down callback (down_complete = 0x%p)", &ctrl_link->down_complete);

		rtn = sl_ctrl_async_link_down_notif_send(ctrl_link, core_cause_map, core_info_map);
		if (rtn)
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
				"async down callback async_link_down_notif_send failed [%d]", rtn);

		sl_ctrl_link_put(ctrl_link);
		return 0;
	default:
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "async down callback invalid (core_state = %u %s)",
			core_state, sl_core_link_state_str(core_state));

		sl_ctrl_link_put(ctrl_link);
		return -EBADRQC;
	}
}

int sl_ctrl_link_async_down(struct sl_ctrl_link *ctrl_link, u64 down_cause_map)
{
	int                  rtn;
	u32                  link_state;
	char                 cause_str[SL_LINK_DOWN_CAUSE_STR_SIZE];

	if (!sl_ctrl_link_kref_get_unless_zero(ctrl_link)) {
		sl_ctrl_log_err(ctrl_link, LOG_NAME,
			"async_down kref_get_unless_zero failed (ctrl_link = 0x%p)", ctrl_link);
		return -EBADRQC;
	}

	sl_link_down_cause_map_with_info_str(down_cause_map, cause_str, sizeof(cause_str));

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "async_down (down_cause_map = 0x%llX %s)", down_cause_map, cause_str);

	sl_ctrl_link_up_clock_reset(ctrl_link);

	spin_lock(&ctrl_link->data_lock);
	link_state = ctrl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_UP:
		ctrl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "async_down - stopping");
		spin_unlock(&ctrl_link->data_lock);
		rtn = sl_core_link_down(ctrl_link->ctrl_lgrp->ctrl_ldev->num, ctrl_link->ctrl_lgrp->num, ctrl_link->num,
			sl_ctrl_link_async_down_callback, ctrl_link, down_cause_map);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_link, LOG_NAME,
				"core_link_down failed [%d]", rtn);
			sl_ctrl_link_put(ctrl_link);
			return rtn;
		}
		sl_ctrl_link_put(ctrl_link);
		return 0;
	case SL_LINK_STATE_DOWN:
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "async_down - already down");
		spin_unlock(&ctrl_link->data_lock);
		sl_ctrl_link_put(ctrl_link);
		return 0;
	case SL_LINK_STATE_STOPPING:
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "async_down - already stopping");
		spin_unlock(&ctrl_link->data_lock);
		sl_ctrl_link_put(ctrl_link);
		return 0;
	default:
		sl_ctrl_log_err(ctrl_link, LOG_NAME, "async_down - invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctrl_link->data_lock);
		sl_ctrl_link_put(ctrl_link);
		return -EBADRQC;
	}
}

void sl_ctrl_link_config_get(struct sl_ctrl_link *ctrl_link, struct sl_link_config *link_config)
{
	spin_lock(&ctrl_link->config_lock);
	*link_config = ctrl_link->config;
	spin_unlock(&ctrl_link->config_lock);
}

void sl_ctrl_link_policy_get(struct sl_ctrl_link *ctrl_link, struct sl_link_policy *link_policy)
{
	spin_lock(&ctrl_link->config_lock);
	*link_policy = ctrl_link->policy;
	spin_unlock(&ctrl_link->config_lock);
}

void sl_ctrl_link_state_set(struct sl_ctrl_link *ctrl_link, u32 link_state)
{
	spin_lock(&ctrl_link->data_lock);
	ctrl_link->state = link_state;
	spin_unlock(&ctrl_link->data_lock);
}

u32 sl_ctrl_link_state_get(struct sl_ctrl_link *ctrl_link)
{
	u32 state;

	spin_lock(&ctrl_link->data_lock);
	state = ctrl_link->state;
	spin_unlock(&ctrl_link->data_lock);

	return state;
}

u64 sl_ctrl_link_last_up_fail_cause_map_get(struct sl_ctrl_link *ctrl_link)
{
	u64 last_up_fail_cause_map;

	spin_lock(&ctrl_link->data_lock);
	last_up_fail_cause_map = ctrl_link->last_up_fail_cause_map;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"last_up_fail_cause_map_get (last_up_fail_cause_map = 0x%llX)", last_up_fail_cause_map);

	return last_up_fail_cause_map;
}

void sl_ctrl_link_last_up_fail_cause_info_get(struct sl_ctrl_link *ctrl_link, u64 *last_up_fail_cause_map,
	time64_t *last_up_fail_time)
{
	spin_lock(&ctrl_link->data_lock);
	*last_up_fail_cause_map = ctrl_link->last_up_fail_cause_map;
	*last_up_fail_time = ctrl_link->last_up_fail_time;
	spin_unlock(&ctrl_link->data_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"last up fail time show (cause_map = 0x%llX, time = %lld %ptTt %ptTd)",
		*last_up_fail_cause_map, *last_up_fail_time, last_up_fail_time, last_up_fail_time);
}
