// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/completion.h>

#include "sl_asic.h"
#include "sl_sysfs.h"

#include "base/sl_ctl_log.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_priv.h"
#include "sl_ctl_link_counters.h"
#include "sl_core_str.h"
#include "sl_core_link.h"
#include "sl_core_link_an.h"
#include "sl_core_mac.h"
#include "sl_core_llr.h"
#include "sl_core_link_an.h"
#include "sl_test_common.h"

#define LOG_NAME SL_CTL_LINK_LOG_NAME

#define SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS 2000
#define SL_CTL_LINK_DEL_WAIT_TIMEOUT_MS  2000

static struct sl_ctl_link *ctl_links[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctl_links_lock);

int sl_ctl_link_new(u8 ldev_num, u8 lgrp_num, u8 link_num, struct kobject *sysfs_parent)
{
	int                 rtn;
	struct sl_ctl_lgrp *ctl_lgrp;
	struct sl_ctl_link *ctl_link;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "missing lgrp (lgrp_num = %u)", lgrp_num);
		return -EBADRQC;
	}

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (ctl_link) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "new exists (link = 0x%p)", ctl_link);
		return -EBADRQC;
	}

	ctl_link = kzalloc(sizeof(struct sl_ctl_link), GFP_KERNEL);
	if (!ctl_link)
		return -ENOMEM;

	ctl_link->magic    = SL_CTL_LINK_MAGIC;
	ctl_link->ver      = SL_CTL_LINK_VER;
	ctl_link->num      = link_num;
	ctl_link->ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	kref_init(&ctl_link->ref_cnt);
	init_completion(&ctl_link->del_complete);

	spin_lock_init(&ctl_link->config_lock);
	spin_lock_init(&ctl_link->data_lock);

	rtn = sl_ctl_link_counters_init(ctl_link);
	if (rtn) {
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"ctl_link_counters_init failed [%d]", rtn);
		goto out;
	}

	ctl_link->up_clock.start           = ktime_set(0, 0);
	ctl_link->up_clock.elapsed         = ktime_set(0, 0);
	ctl_link->up_clock.attempt_count   = 0;
	ctl_link->up_clock.attempt_start   = ktime_set(0, 0);
	ctl_link->up_clock.attempt_elapsed = ktime_set(0, 0);
	spin_lock_init(&ctl_link->up_clock.lock);

	spin_lock_init(&ctl_link->fec_data.lock);
	timer_setup(&ctl_link->fec_mon_timer, sl_ctl_link_fec_mon_timer, 0);
	spin_lock_init(&ctl_link->fec_mon_timer_lock);
	ctl_link->fec_data.curr_ptr = &ctl_link->fec_data.cntrs[0];
	ctl_link->fec_data.prev_ptr = &ctl_link->fec_data.cntrs[1];
	INIT_WORK(&ctl_link->fec_mon_timer_work, sl_ctl_link_fec_mon_timer_work);

	spin_lock_init(&ctl_link->fec_up_cache.lock);
	spin_lock_init(&ctl_link->fec_down_cache.lock);

	ctl_link->state = SL_LINK_STATE_DOWN;

	rtn = sl_core_link_new(ldev_num, lgrp_num, link_num);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_new failed [%d]", rtn);
		goto out;
	}

	if (sysfs_parent) {
		ctl_link->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_link_create(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME,
				"sysfs_link_create failed [%d]", rtn);
			goto out;
		}
	}

	spin_lock(&ctl_links_lock);
	ctl_links[ldev_num][lgrp_num][link_num] = ctl_link;
	spin_unlock(&ctl_links_lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "new (link = 0x%p)", ctl_link);

	return 0;

out:
	kfree(ctl_link);
	return rtn;
}

static int sl_ctl_link_down_cmd(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN_CMD);

	sl_ctl_link_up_clock_reset(ctl_link);

	sl_ctl_link_fec_mon_stop(ctl_link);
	cancel_work_sync(&ctl_link->fec_mon_timer_work);

	rtn = sl_core_link_an_lp_caps_stop(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
			"del core_link_an_lp_caps_stop failed [%d]", rtn);

	rtn = sl_core_link_down(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		sl_ctl_link_down_callback, ctl_link, SL_LINK_DOWN_CAUSE_COMMAND_MAP);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_down failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

static int sl_ctl_link_cancel_cmd(struct sl_ctl_link *ctl_link)
{
	int rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "cancel cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_CANCEL_CMD);

	rtn = sl_core_link_an_lp_caps_stop(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
			"del core_link_an_lp_caps_stop failed [%d]", rtn);

	rtn = sl_core_link_cancel(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		sl_ctl_link_down_callback, ctl_link);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_down failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

static void sl_ctl_link_down_wait(struct sl_ctl_link *ctl_link)
{
	unsigned long timeleft;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down wait");

	timeleft = wait_for_completion_timeout(&ctl_link->down_complete,
		msecs_to_jiffies(SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS));
	if (timeleft == 0)
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"del completion_timeout (down_complete = 0x%p)",
			&ctl_link->down_complete);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "del completion (timeleft = %lu)", timeleft);
}

static void sl_ctl_link_release(struct kref *kref)
{
	struct sl_ctl_link *ctl_link;
	u32                 link_state;
	u8                  ldev_num;
	u8                  lgrp_num;
	u8                  link_num;

	ctl_link = container_of(kref, struct sl_ctl_link, ref_cnt);
	ldev_num = ctl_link->ctl_lgrp->ctl_ldev->num;
	lgrp_num = ctl_link->ctl_lgrp->num;
	link_num = ctl_link->num;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "release (link = 0x%p)", ctl_link);

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->is_canceled = true;
		spin_unlock(&ctl_link->data_lock);
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_CANCEL_CMD);
		sl_ctl_link_cancel_cmd(ctl_link);
		sl_ctl_link_down_wait(ctl_link);
		break;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "release stopping");
		spin_unlock(&ctl_link->data_lock);
		sl_ctl_link_down_cmd(ctl_link);
		sl_ctl_link_down_wait(ctl_link);
		break;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "release already stopping");
		spin_unlock(&ctl_link->data_lock);
		sl_ctl_link_down_wait(ctl_link);
		break;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "release already down");
		spin_unlock(&ctl_link->data_lock);
		break;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "release invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		return;
	}

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_link_delete(ctl_link);

	sl_core_link_del(ldev_num, lgrp_num, link_num);

	sl_ctl_link_counters_del(ctl_link);

	spin_lock(&ctl_links_lock);
	ctl_links[ldev_num][lgrp_num][link_num] = NULL;
	spin_unlock(&ctl_links_lock);

	complete_all(&ctl_link->del_complete);

	kfree(ctl_link);
}

static int sl_ctl_link_put(struct sl_ctl_link *ctl_link)
{
	return kref_put(&ctl_link->ref_cnt, sl_ctl_link_release);
}

int sl_ctl_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctl_link *ctl_link;
	unsigned long timeleft;

	sl_ctl_log_dbg(NULL, LOG_NAME, "link del (ldev_num = %u, lgrp_num = %u, link_num = %u)",
		ldev_num, lgrp_num, link_num);

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err_trace(NULL, LOG_NAME,
			"link del link not found (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	/* Release occurs on the last caller. Block until complete. */
	if (!sl_ctl_link_put(ctl_link)) {
		timeleft = wait_for_completion_timeout(&ctl_link->del_complete,
			msecs_to_jiffies(SL_CTL_LINK_DEL_WAIT_TIMEOUT_MS));

		sl_ctl_log_dbg(ctl_link, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctl_log_err(ctl_link, LOG_NAME,
				"del completion_timeout (ctl_link = 0x%p)", ctl_link);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static bool sl_ctl_link_kref_get_unless_zero(struct sl_ctl_link *ctl_link)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctl_link->ref_cnt) != 0);

	if (!incremented)
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ctl_link = 0x%p)", ctl_link);

	return incremented;
}

struct sl_ctl_link *sl_ctl_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctl_link *ctl_link;

	spin_lock(&ctl_links_lock);
	ctl_link = ctl_links[ldev_num][lgrp_num][link_num];
	spin_unlock(&ctl_links_lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "get (link = 0x%p)", ctl_link);

	return ctl_link;
}

static int sl_ctl_link_config_set_cmd(struct sl_ctl_link *ctl_link, struct sl_link_config *link_config)
{
	struct sl_core_link_config core_link_config;
	int                        rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "config set cmd");

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  link_up_timeout    = %ums", link_config->link_up_timeout_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  link_up_tries_max  = %d",   link_config->link_up_tries_max);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_settle_wait = %dms", link_config->fec_up_settle_wait_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_check_wait  = %dms", link_config->fec_up_check_wait_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_ucw_limit   = %d",   link_config->fec_up_ucw_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_ccw_limit   = %d",   link_config->fec_up_ccw_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  pause_map          = 0x%X", link_config->pause_map);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  hpe_map            = 0x%X", link_config->hpe_map);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  options            = 0x%X", link_config->options);

	/* Check if the configuration is locked. If we are performing an admin operation, then we can ignore the
	 * lock bit and continue to write the config.
	 */
	spin_lock(&ctl_link->config_lock);
	if (!(link_config->options & SL_LINK_CONFIG_OPT_ADMIN) &&
		(ctl_link->config.options & SL_LINK_CONFIG_OPT_LOCK)) {
		sl_ctl_log_info(ctl_link, LOG_NAME, "link config locked (options: 0x%X)", ctl_link->config.options);
		spin_unlock(&ctl_link->config_lock);
		return 0;
	}
	spin_unlock(&ctl_link->config_lock);

	/* Admin bit is transient */
	link_config->options &= ~SL_LINK_CONFIG_OPT_ADMIN;

	if ((!link_config->fec_up_settle_wait_ms && link_config->fec_up_check_wait_ms) ||
		(link_config->fec_up_settle_wait_ms && !link_config->fec_up_check_wait_ms)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "fec_up invalid (settle_wait = %ums, check_wait = %ums)",
			link_config->fec_up_settle_wait_ms, link_config->fec_up_check_wait_ms);
		return -EINVAL;
	}

	core_link_config.magic           = SL_CORE_LINK_CONFIG_MAGIC;
	core_link_config.fault_callback  = sl_ctl_link_fault_callback;
	core_link_config.fault_intr_hdlr = sl_ctl_link_fault_intr_hdlr;

	/* timeouts */
	core_link_config.link_up_timeout_ms = link_config->link_up_timeout_ms;

	/* fec config */
	core_link_config.fec_up_settle_wait_ms = link_config->fec_up_settle_wait_ms;
	core_link_config.fec_up_check_wait_ms  = link_config->fec_up_check_wait_ms;
	core_link_config.fec_up_ucw_limit      = link_config->fec_up_ucw_limit;
	core_link_config.fec_up_ccw_limit      = link_config->fec_up_ccw_limit;

	/* maps */
	core_link_config.hpe_map   = link_config->hpe_map;
	core_link_config.pause_map = link_config->pause_map;

	/* flags */
	core_link_config.flags = link_config->options;

	rtn = sl_core_link_config_set(ctl_link->ctl_lgrp->ctl_ldev->num,
		ctl_link->ctl_lgrp->num, ctl_link->num, &core_link_config);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_config_set failed [%d]", rtn);
		return rtn;
	}

	spin_lock(&ctl_link->config_lock);
	ctl_link->config = *link_config;
	spin_unlock(&ctl_link->config_lock);

	if (ctl_link->config.link_up_tries_max == SL_LINK_INFINITE_UP_TRIES)
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "link up tries set to infinite");

	return 0;
}

int sl_ctl_link_config_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			   struct sl_link_config *link_config)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"config set NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "config set link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	link_state = sl_ctl_link_state_get(ctl_link);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "config set (link_state = %u %s)",
		link_state, sl_link_state_str(link_state));

	switch (link_state) {
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "config - set");
		rtn = sl_ctl_link_config_set_cmd(ctl_link, link_config);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_config_set_cmd failed [%d]", rtn);
			goto out;
		}

		rtn = 0;
		goto out;
	case SL_LINK_STATE_STARTING:
	case SL_LINK_STATE_INVALID:
	case SL_LINK_STATE_STOPPING:
	case SL_LINK_STATE_UP:
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "config - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		rtn = -EBADRQC;
		goto out;
	}

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "config - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

int sl_ctl_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			   struct sl_link_policy *link_policy)
{
	int                         rtn;
	u32                         link_state;
	struct sl_ctl_link         *ctl_link;
	struct sl_core_link_policy  core_link_policy;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"policy set NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "policy set link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	/* Check if the policy is locked. If we are performing an admin operation, then we can ignore the
	 * lock bit and continue to write the config.
	 */
	spin_lock(&ctl_link->config_lock);
	if (!(link_policy->options & SL_LINK_POLICY_OPT_ADMIN) &&
		(ctl_link->policy.options & SL_LINK_POLICY_OPT_LOCK)) {
		sl_ctl_log_info(ctl_link, LOG_NAME, "link policy locked (options: 0x%X)", ctl_link->policy.options);
		spin_unlock(&ctl_link->config_lock);
		rtn = 0;
		goto out;
	}
	spin_unlock(&ctl_link->config_lock);

	/* Admin bit is transient */
	link_policy->options &= ~SL_LINK_POLICY_OPT_ADMIN;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "policy set");
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ucw_down_limit = %d", link_policy->fec_mon_ucw_down_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ucw_warn_limit = %d", link_policy->fec_mon_ucw_warn_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ccw_down_limit = %d", link_policy->fec_mon_ccw_down_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ccw_warn_limit = %d", link_policy->fec_mon_ccw_warn_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_period         = %dms", link_policy->fec_mon_period_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  options                = 0x%X", link_policy->options);

	spin_lock(&ctl_link->config_lock);
	ctl_link->policy = *link_policy;
	spin_unlock(&ctl_link->config_lock);

	core_link_policy.options = link_policy->options;
	rtn = sl_core_link_policy_set(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
			&core_link_policy);
	if (rtn) {
		sl_core_log_err(ctl_link, LOG_NAME, "core link policy set failed [%d]", rtn);
		rtn = -EBADRQC;
		goto out;
	}

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(ctl_link, LOG_NAME, "link_policy_set not up (link_state = %u %s)", link_state,
		  sl_core_link_state_str(link_state));
		rtn = 0;
		goto out;
	}

	sl_ctl_link_fec_mon_start(ctl_link);

	rtn = 0;

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "policy set - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

int sl_ctl_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       struct sl_link_caps *caps, u32 timeout_ms, u32 flags)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"caps get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "caps get link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps get");

	rtn = sl_core_link_an_lp_caps_get(ldev_num, lgrp_num, link_num,
		sl_ctl_link_an_lp_caps_get_callback, ctl_link, caps, timeout_ms, flags);
	if (rtn)
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_an_lp_caps_get failed [%d]", rtn);

	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps get - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

int sl_ctl_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"caps stop NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "caps stop link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps stop");

	rtn = sl_core_link_an_lp_caps_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_an_lp_caps_stop failed [%d]", rtn);

	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps stop - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

static int sl_ctl_link_up_cmd(struct sl_ctl_link *ctl_link)
{
	int           rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_CMD);

	sl_ctl_link_up_clock_start(ctl_link);

	sl_ctl_link_is_canceled_set(ctl_link, false);

	init_completion(&ctl_link->down_complete);

	sl_ctl_link_up_clock_attempt_start(ctl_link);

	rtn = sl_core_link_up(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		sl_ctl_link_up_callback, ctl_link);
	if (rtn) {
		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_STOPPING);

		sl_ctl_log_err_trace(ctl_link, LOG_NAME, "core_link_up failed [%d]", rtn);

		sl_ctl_link_up_clock_reset(ctl_link);

		sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

		return rtn;
	}

	return 0;
}

int sl_ctl_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"link up NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "link up ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up");

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_DOWN:
		ctl_link->state = SL_LINK_STATE_STARTING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - starting");
		spin_unlock(&ctl_link->data_lock);
		rtn = sl_ctl_link_up_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_up_cmd failed [%d]", rtn);
			goto out;
		}

		rtn = 0;
		goto out;
	case SL_LINK_STATE_STARTING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - already starting");
		spin_unlock(&ctl_link->data_lock);
		rtn = -EINPROGRESS;
		goto out;
	case SL_LINK_STATE_UP:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - already up");
		spin_unlock(&ctl_link->data_lock);
		rtn = -EALREADY;
		goto out;
	case SL_LINK_STATE_STOPPING:
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		rtn = -EBADRQC;
		goto out;
	}

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

int sl_ctl_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_dbg(NULL, LOG_NAME,
			"link down NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "link down ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down");

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		ctl_link->is_canceled = true;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - canceling");
		spin_unlock(&ctl_link->data_lock);
		rtn = sl_ctl_link_cancel_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "cancel_cmd failed [%d]", rtn);
			goto out;
		}

		rtn = 0;
		goto out;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - stopping");
		spin_unlock(&ctl_link->data_lock);
		rtn = sl_ctl_link_down_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "down_cmd failed [%d]", rtn);
			goto out;
		}

		rtn = 0;
		goto out;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already down");
		spin_unlock(&ctl_link->data_lock);
		rtn = -EALREADY;
		goto out;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already stopping");
		spin_unlock(&ctl_link->data_lock);
		rtn = -EINPROGRESS;
		goto out;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		rtn = -EBADRQC;
		goto out;
	}

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

static int sl_ctl_link_reset_cmd(struct sl_ctl_link *ctl_link)
{
	int           rtn;
	u8            ldev_num;
	u8            lgrp_num;
	u8            link_num;
	unsigned long timeleft;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_RESET_CMD);

	sl_ctl_link_up_clock_reset(ctl_link);

	sl_ctl_link_fec_mon_stop(ctl_link);
	cancel_work_sync(&ctl_link->fec_mon_timer_work);

	ldev_num = ctl_link->ctl_lgrp->ctl_ldev->num;
	lgrp_num = ctl_link->ctl_lgrp->num;
	link_num = ctl_link->num;

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_llr_stop failed [%d]", rtn);

	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_mac_tx_stop failed [%d]", rtn);

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_mac_rx_stop failed [%d]", rtn);

	rtn = sl_core_link_down(ldev_num, lgrp_num, link_num, sl_ctl_link_down_callback,
		ctl_link, SL_LINK_DOWN_CAUSE_COMMAND_MAP);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_link_down failed [%d]", rtn);

	timeleft = wait_for_completion_timeout(&ctl_link->down_complete,
		msecs_to_jiffies(SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS));
	if (timeleft == 0)
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"completion_timeout (down_complete = 0x%p)", &ctl_link->down_complete);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "completion (timeleft = %lu)", timeleft);

	sl_core_link_reset(ldev_num, lgrp_num, link_num);

	return 0;
}

int sl_ctl_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"link reset NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "link reset ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset");

	spin_lock(&ctl_link->data_lock);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->is_canceled = true;
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_CANCEL_CMD);
		fallthrough;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - stopping");
		spin_unlock(&ctl_link->data_lock);
		rtn = sl_ctl_link_reset_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_reset_cmd failed [%d]", rtn);
			goto out;
		}

		rtn = 0;
		goto out;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - already down");
		spin_unlock(&ctl_link->data_lock);
		rtn = 0;
		goto out;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already stopping");
		spin_unlock(&ctl_link->data_lock);

		rtn = 0;
		goto out;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock(&ctl_link->data_lock);
		rtn = -EBADRQC;
		goto out;
	}

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - link removed (link = 0x%p)", ctl_link);

	return rtn;
}

void sl_ctl_link_up_clocks_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       s64 *attempt_time, s64 *total_time)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"clocks get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "clocks get link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return;
	}

	spin_lock(&ctl_link->up_clock.lock);

	if (!ktime_compare(ctl_link->up_clock.attempt_start, ktime_set(0, 0)))
		*attempt_time = ktime_to_ms(ctl_link->up_clock.attempt_elapsed);
	else
		*attempt_time = ktime_to_ms(ktime_sub(ktime_get(), ctl_link->up_clock.attempt_start));

	if (!ktime_compare(ctl_link->up_clock.start, ktime_set(0, 0)))
		*total_time = ktime_to_ms(ctl_link->up_clock.elapsed);
	else
		*total_time = ktime_to_ms(ktime_sub(ktime_get(), ctl_link->up_clock.start));

	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"clocks get (attempt_time = %lld, total_time = %lld)", *attempt_time, *total_time);

	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "clocks get - link removed (link = 0x%p)", ctl_link);
}

void sl_ctl_link_up_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *attempt_count)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"count get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "count get link ref unavailable (ctl_link = 0x%p)",
			ctl_link);
		return;
	}

	spin_lock(&ctl_link->up_clock.lock);
	*attempt_count = ctl_link->up_clock.attempt_count;
	spin_unlock(&ctl_link->up_clock.lock);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "count get (attempt_count = %u)", *attempt_count);

	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "count get - link removed (link = 0x%p)", ctl_link);
}

int sl_ctl_link_state_get_cmd(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state)
{
	int                 rtn;
	u32                 core_link_state;
	struct sl_ctl_link *ctl_link;

	*state = SL_LINK_STATE_INVALID;
	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_dbg(NULL, LOG_NAME,
			"state get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return 0;
	}

	if (!sl_ctl_link_kref_get_unless_zero(ctl_link)) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME,
			"state get kref unavailable (ctl_link = 0x%p)", ctl_link);
		return -EBADRQC;
	}

	core_link_state = SL_CORE_LINK_STATE_INVALID;
	rtn = sl_core_link_state_get(ldev_num, lgrp_num, link_num, &core_link_state);
	if (rtn) {
		sl_ctl_log_err_trace(NULL, LOG_NAME, "core state get invalid");
		goto out;
	}

	if (core_link_state == SL_CORE_LINK_STATE_AN)
		*state = SL_LINK_STATE_AN;
	else
		*state = sl_ctl_link_state_get(ctl_link);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"state get (state = %u %s)",
		*state, sl_link_state_str(*state));

out:
	if (sl_ctl_link_put(ctl_link))
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "state get - link removed (link = 0x%p)", ctl_link);

	return 0;
}

int sl_ctl_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"an lp caps state get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EINVAL;
	}

	*state = sl_core_link_an_lp_caps_state_get(ldev_num, lgrp_num, link_num);

	return 0;
}

void sl_ctl_link_an_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *fail_cause, time64_t *fail_time)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"an fail cause get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	sl_core_link_an_fail_cause_get(ldev_num, lgrp_num, link_num, fail_cause, fail_time);
}

u32 sl_ctl_link_an_retry_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_link_an_retry_count_get(ldev_num, lgrp_num, link_num);
}
