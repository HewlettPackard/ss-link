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
#include "sl_core_mac.h"
#include "sl_core_llr.h"
#include "sl_core_link_an.h"
#include "sl_test_common.h"

#define LOG_NAME SL_CTL_LINK_LOG_NAME

#define SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS 2000

static struct sl_ctl_link *ctl_links[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static DEFINE_SPINLOCK(ctl_links_lock);

static void sl_ctl_link_is_deleting_set(struct sl_ctl_link *ctl_link)
{
	unsigned long flags;

	spin_lock_irqsave(&ctl_link->data_lock, flags);
	ctl_link->is_deleting = true;
	spin_unlock_irqrestore(&ctl_link->data_lock, flags);
}

static bool sl_ctl_link_is_deleting(struct sl_ctl_link *ctl_link)
{
	unsigned long flags;
	bool          is_deleting;

	spin_lock_irqsave(&ctl_link->data_lock, flags);
	is_deleting = ctl_link->is_deleting;
	spin_unlock_irqrestore(&ctl_link->data_lock, flags);

	return is_deleting;
}

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
		sl_ctl_log_err(ctl_link, LOG_NAME, "new exists (link = 0x%p, is_deleting = %s)",
			ctl_link, sl_ctl_link_is_deleting(ctl_link) ? "true" : "false");
		return -EBADRQC;
	}

	ctl_link = kzalloc(sizeof(struct sl_ctl_link), GFP_KERNEL);
	if (!ctl_link)
		return -ENOMEM;

	ctl_link->magic    = SL_CTL_LINK_MAGIC;
	ctl_link->ver      = SL_CTL_LINK_VER;
	ctl_link->num      = link_num;
	ctl_link->ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

	spin_lock_init(&ctl_link->config_lock);
	spin_lock_init(&ctl_link->data_lock);
	spin_lock_init(&ctl_link->up_count_lock);

	rtn = sl_ctl_link_counters_init(ctl_link);
	if (rtn) {
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"ctl_link_counters_init failed [%d]", rtn);
		goto out;
	}

	ctl_link->up_clock.start           = ktime_set(0, 0);
	ctl_link->up_clock.elapsed         = ktime_set(0, 0);
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

int sl_ctl_link_down_complete_callback(void *tag)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = tag;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down complete callback");

	cancel_work_sync(&ctl_link->ctl_lgrp->notif_work);
	sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);

	complete(&ctl_link->down_complete);
	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"link down complete callback (down_complete = 0x%p)", &ctl_link->down_complete);

	return 0;
}

void sl_ctl_link_del(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;
	unsigned long       irq_flags;
	unsigned long       timeleft;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_dbg(NULL, LOG_NAME,
			"del not found (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "del (link = 0x%p)", ctl_link);

	if (sl_ctl_link_is_deleting(ctl_link)) {
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "del in progress");
		return;
	}

	sl_ctl_link_is_deleting_set(ctl_link);

	spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->is_canceled = true;
		fallthrough;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "del - stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		goto down;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "del - already stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		goto delete;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "del - already down");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		goto delete;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "del - invalid state (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return;
	}

down:

	sl_ctl_link_fec_mon_stop(ctl_link);
	cancel_work_sync(&ctl_link->fec_mon_timer_work);

	rtn = sl_core_link_an_lp_caps_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME,
			"del core_link_an_lp_caps_stop failed [%d]", rtn);

	init_completion(&ctl_link->down_complete);

	rtn = sl_core_link_down(ldev_num, lgrp_num, link_num, sl_ctl_link_down_complete_callback,
		ctl_link, SL_LINK_DOWN_CAUSE_COMMAND);
	if (rtn)
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"del core_link_down failed [%d]", rtn);

	timeleft = wait_for_completion_timeout(&ctl_link->down_complete,
		msecs_to_jiffies(SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS));
	if (timeleft == 0)
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"completion_timeout (down_complete = 0x%p)", &ctl_link->down_complete);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "completion (timeleft = %lu)", timeleft);

delete:
	sl_core_link_del(ldev_num, lgrp_num, link_num);

	sl_ctl_link_counters_del(ctl_link);

	sl_sysfs_link_delete(ctl_link);

	spin_lock(&ctl_links_lock);
	ctl_links[ldev_num][lgrp_num][link_num] = NULL;
	spin_unlock(&ctl_links_lock);

	kfree(ctl_link);
}

struct sl_ctl_link *sl_ctl_link_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;

	spin_lock_irqsave(&ctl_links_lock, irq_flags);
	ctl_link = ctl_links[ldev_num][lgrp_num][link_num];
	spin_unlock_irqrestore(&ctl_links_lock, irq_flags);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "get (link = 0x%p)", ctl_link);

	return ctl_link;
}

static int sl_ctl_link_config_set_cmd(struct sl_ctl_link *ctl_link, struct sl_link_config *link_config)
{
	struct sl_core_link_config  core_link_config;
	u32                         tech_map;
	unsigned long               irq_flags;
	int                         rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "config set cmd");

	spin_lock(&ctl_link->ctl_lgrp->config_lock);
	tech_map = ctl_link->ctl_lgrp->config.tech_map;
	spin_unlock(&ctl_link->ctl_lgrp->config_lock);

	if ((!tech_map || hweight_long(tech_map) > 1) &&
		(link_config->fec_up_ucw_limit < 0 || link_config->fec_up_ccw_limit < 0)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "fec_limit_calc unavailable (tech_map = %u)", tech_map);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  link_up_timeout    = %ums", link_config->link_up_timeout_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  link_up_tries_max  = %u",   link_config->link_up_tries_max);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_settle_wait = %ums", link_config->fec_up_settle_wait_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_up_check_wait  = %ums", link_config->fec_up_check_wait_ms);
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

	if (link_config->fec_up_ucw_limit < 0)
		core_link_config.fec_up_ucw_limit = sl_ctl_link_fec_limit_calc(ctl_link,
			SL_CTL_LINK_FEC_UCW_MANT, SL_CTL_LINK_FEC_UCW_EXP);
	else
		core_link_config.fec_up_ucw_limit = link_config->fec_up_ucw_limit;

	if (link_config->fec_up_ccw_limit < 0)
		core_link_config.fec_up_ccw_limit = sl_ctl_link_fec_limit_calc(ctl_link,
			SL_CTL_LINK_FEC_CCW_MANT, SL_CTL_LINK_FEC_CCW_EXP);
	else
		core_link_config.fec_up_ccw_limit = link_config->fec_up_ccw_limit;

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

	spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
	ctl_link->config = *link_config;
	spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);

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

	link_state = sl_ctl_link_state_get(ctl_link);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "config set (link_state = %u %s)",
		link_state, sl_link_state_str(link_state));

	switch (link_state) {
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "config - set");
		rtn = sl_ctl_link_config_set_cmd(ctl_link, link_config);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_config_set_cmd failed [%d]", rtn);
			return rtn;
		}
		return 0;
	case SL_LINK_STATE_STARTING:
	case SL_LINK_STATE_INVALID:
	case SL_LINK_STATE_STOPPING:
	case SL_LINK_STATE_UP:
	default:
		sl_ctl_log_err(ctl_link, LOG_NAME, "config - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		return -EBADRQC;
	}
}

int sl_ctl_link_policy_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			   struct sl_link_policy *link_policy)
{
	int                         rtn;
	unsigned long               irq_flags;
	u32                         link_state;
	u32                         tech_map;
	struct sl_ctl_link         *ctl_link;
	struct sl_core_link_policy  core_link_policy;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"policy set NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	spin_lock(&ctl_link->ctl_lgrp->config_lock);
	tech_map = ctl_link->ctl_lgrp->config.tech_map;
	spin_unlock(&ctl_link->ctl_lgrp->config_lock);

	if ((!tech_map || hweight_long(tech_map) > 1) && (link_policy->fec_mon_ccw_down_limit < 0 ||
		link_policy->fec_mon_ccw_warn_limit < 0 ||
		link_policy->fec_mon_ucw_down_limit < 0 ||
		link_policy->fec_mon_ucw_warn_limit < 0)) {
		sl_ctl_log_err(ctl_link, LOG_NAME, "fec_limit_calc unavailable (tech_map = %u)", tech_map);
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
		return 0;
	}
	spin_unlock(&ctl_link->config_lock);

	/* Admin bit is transient */
	link_policy->options &= ~SL_LINK_POLICY_OPT_ADMIN;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "policy set");
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ucw_down_limit = %d", link_policy->fec_mon_ucw_down_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ucw_warn_limit = %d", link_policy->fec_mon_ucw_warn_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ccw_down_limit = %d", link_policy->fec_mon_ccw_down_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_ccw_warn_limit = %d", link_policy->fec_mon_ccw_warn_limit);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  fec_mon_period         = %ums", link_policy->fec_mon_period_ms);
	sl_ctl_log_dbg(ctl_link, LOG_NAME, "  options                = 0x%X", link_policy->options);

	spin_lock_irqsave(&ctl_link->fec_data.lock, irq_flags);
	if (link_policy->fec_mon_ccw_down_limit < 0) {
		ctl_link->fec_data.info.monitor.ccw_down_limit =
			sl_ctl_link_fec_limit_calc(ctl_link,
				SL_CTL_LINK_FEC_CCW_MANT, SL_CTL_LINK_FEC_CCW_EXP);
	} else {
		ctl_link->fec_data.info.monitor.ccw_down_limit = link_policy->fec_mon_ccw_down_limit;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"monitor policy (ccw_down_limit = %d)",
		ctl_link->fec_data.info.monitor.ccw_down_limit);

	if (link_policy->fec_mon_ccw_warn_limit < 0)
		ctl_link->fec_data.info.monitor.ccw_warn_limit = ctl_link->fec_data.info.monitor.ccw_down_limit >> 1;
	else
		ctl_link->fec_data.info.monitor.ccw_warn_limit = link_policy->fec_mon_ccw_warn_limit;

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"monitor policy (ccw_warn_limit = %d)",
		ctl_link->fec_data.info.monitor.ccw_warn_limit);

	if (ctl_link->fec_data.info.monitor.ccw_down_limit &&
		(ctl_link->fec_data.info.monitor.ccw_warn_limit > ctl_link->fec_data.info.monitor.ccw_down_limit))
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"CCW warning limit set greater than down limit (%d > %d)",
			ctl_link->fec_data.info.monitor.ccw_warn_limit, ctl_link->fec_data.info.monitor.ccw_down_limit);

	if (link_policy->fec_mon_ucw_down_limit < 0) {
		ctl_link->fec_data.info.monitor.ucw_down_limit =
			sl_ctl_link_fec_limit_calc(ctl_link,
				SL_CTL_LINK_FEC_UCW_MANT, SL_CTL_LINK_FEC_UCW_EXP);
	} else {
		ctl_link->fec_data.info.monitor.ucw_down_limit = link_policy->fec_mon_ucw_down_limit;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"monitor policy (ucw_down_limit = %d)",
		ctl_link->fec_data.info.monitor.ucw_down_limit);

	if (link_policy->fec_mon_ucw_warn_limit < 0)
		ctl_link->fec_data.info.monitor.ucw_warn_limit = ctl_link->fec_data.info.monitor.ucw_down_limit >> 1;
	else
		ctl_link->fec_data.info.monitor.ucw_warn_limit = link_policy->fec_mon_ucw_warn_limit;

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"monitor policy (ucw_warn_limit = %d)",
		ctl_link->fec_data.info.monitor.ucw_warn_limit);

	if (ctl_link->fec_data.info.monitor.ucw_down_limit &&
		(ctl_link->fec_data.info.monitor.ucw_warn_limit > ctl_link->fec_data.info.monitor.ucw_down_limit))
		sl_ctl_log_warn(ctl_link, LOG_NAME,
			"CCW warning limit set greater than down limit (%d > %d)",
			ctl_link->fec_data.info.monitor.ucw_warn_limit, ctl_link->fec_data.info.monitor.ucw_down_limit);

	spin_unlock_irqrestore(&ctl_link->fec_data.lock, irq_flags);

	spin_lock_irqsave(&ctl_link->config_lock, irq_flags);
	ctl_link->policy = *link_policy;
	spin_unlock_irqrestore(&ctl_link->config_lock, irq_flags);

	core_link_policy.options = link_policy->options;
	rtn = sl_core_link_policy_set(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
			&core_link_policy);
	if (rtn) {
		sl_core_log_err(ctl_link, LOG_NAME, "core link policy set failed [%d]", rtn);
		return -EBADRQC;
	}

	sl_core_link_state_get(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num,
		ctl_link->num, &link_state);

	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(ctl_link, LOG_NAME, "link_policy_set not up (link_state = %u %s)", link_state,
		  sl_core_link_state_str(link_state));
		return 0;
	}

	sl_ctl_link_fec_mon_start(ctl_link);

	return 0;
}

int sl_ctl_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       struct sl_link_caps *caps, u32 timeout_ms, u32 flags)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"caps get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps get");

	return sl_core_link_an_lp_caps_get(ldev_num, lgrp_num, link_num,
		sl_ctl_link_an_lp_caps_get_callback, ctl_link, caps, timeout_ms, flags);
}

int sl_ctl_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"caps stop NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "an lp caps stop");

	return sl_core_link_an_lp_caps_stop(ldev_num, lgrp_num, link_num);
}

static int sl_ctl_link_up_cmd(struct sl_ctl_link *ctl_link)
{
	int           rtn;
	unsigned long irq_flags;
	u32           link_state;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_UP_START);
	sl_ctl_link_up_count_zero(ctl_link);
	sl_ctl_link_up_clock_start(ctl_link);
	sl_ctl_link_up_attempt_clock_start(ctl_link);

	spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
	ctl_link->is_canceled = false;
	spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);

	rtn = sl_core_link_up(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		sl_ctl_link_up_callback, ctl_link);
	if (rtn) {

		if (!sl_ctl_link_state_stopping_set(ctl_link)) {
			link_state = sl_ctl_link_state_get(ctl_link);
			sl_ctl_log_err_trace(ctl_link, LOG_NAME,
				"link_state_stopping_set invalid state (link_state = %u %s)",
				link_state, sl_link_state_str(link_state));
		}

		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_up failed [%d]", rtn);

		sl_ctl_link_up_clock_clear(ctl_link);
		sl_ctl_link_up_attempt_clock_clear(ctl_link);

		return rtn;
	}

	return 0;
}

int sl_ctl_link_up(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"link up NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "up");

	spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_DOWN:
		ctl_link->state = SL_LINK_STATE_STARTING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - starting");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		rtn = sl_ctl_link_up_cmd(ctl_link);
		if (rtn) {
			sl_ctl_link_state_set(ctl_link, SL_LINK_STATE_DOWN);
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_up_cmd failed [%d]", rtn);
			return rtn;
		}
		return 0;
	case SL_LINK_STATE_STARTING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - already starting");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return 0;
	case SL_LINK_STATE_UP:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - already up");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return 0;
	case SL_LINK_STATE_STOPPING:
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "up - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return -EBADRQC;
	}
}

static int sl_ctl_link_down_cmd(struct sl_ctl_link *ctl_link)
{
	int                 rtn;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down cmd");
	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN_CLIENT);

	sl_ctl_link_fec_mon_stop(ctl_link);

	rtn = sl_core_link_down(ctl_link->ctl_lgrp->ctl_ldev->num, ctl_link->ctl_lgrp->num, ctl_link->num,
		sl_ctl_link_down_callback, ctl_link, SL_LINK_DOWN_CAUSE_COMMAND);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_link, LOG_NAME,
			"core_link_down failed [%d]", rtn);
		SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_DOWN_FAIL);
		return rtn;
	}

	return 0;
}

int sl_ctl_link_down(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_dbg(NULL, LOG_NAME,
			"link down NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "down");

	spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->is_canceled = true;
		fallthrough;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		rtn = sl_ctl_link_down_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_down_cmd failed [%d]", rtn);
			return rtn;
		}

		return 0;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already down");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return 0;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return 0;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return -EBADRQC;
	}
}

static int sl_ctl_link_reset_cmd(struct sl_ctl_link *ctl_link)
{
	int           rtn;
	u8            ldev_num;
	u8            lgrp_num;
	u8            link_num;
	unsigned long timeleft;

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset cmd");

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_RESET_START);

	sl_ctl_link_fec_mon_stop(ctl_link);
	cancel_work_sync(&ctl_link->fec_mon_timer_work);

	ldev_num = ctl_link->ctl_lgrp->ctl_ldev->num;
	lgrp_num = ctl_link->ctl_lgrp->num;
	link_num = ctl_link->num;

	rtn = sl_core_llr_stop(ldev_num, lgrp_num, link_num, SL_CORE_LLR_FLAG_STOP_CLEAR_SETUP);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_llr_stop failed [%d]", rtn);

	rtn = sl_core_mac_tx_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_mac_tx_stop failed [%d]", rtn);

	rtn = sl_core_mac_rx_stop(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_mac_rx_stop failed [%d]", rtn);

	init_completion(&ctl_link->down_complete);

	rtn = sl_core_link_down(ldev_num, lgrp_num, link_num, sl_ctl_link_down_complete_callback,
		ctl_link, SL_LINK_DOWN_CAUSE_COMMAND);
	if (rtn)
		sl_ctl_log_warn_trace(ctl_link, LOG_NAME, "core_link_down failed [%d]", rtn);

	timeleft = wait_for_completion_timeout(&ctl_link->down_complete,
		msecs_to_jiffies(SL_CTL_LINK_DOWN_WAIT_TIMEOUT_MS));
	if (timeleft == 0)
		sl_ctl_log_err(ctl_link, LOG_NAME,
			"completion_timeout (down_complete = 0x%p)", &ctl_link->down_complete);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "completion (timeleft = %lu)", timeleft);

	sl_core_link_reset(ldev_num, lgrp_num, link_num);

	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_RESET);

	sl_ctl_link_up_clock_clear(ctl_link);
	sl_ctl_link_up_attempt_clock_clear(ctl_link);

	return 0;
}

int sl_ctl_link_reset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                 rtn;
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;
	u32                 link_state;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"link reset NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset");

	spin_lock_irqsave(&ctl_link->data_lock, irq_flags);
	link_state = ctl_link->state;
	switch (link_state) {
	case SL_LINK_STATE_STARTING:
		ctl_link->is_canceled = true;
		fallthrough;
	case SL_LINK_STATE_UP:
		ctl_link->state = SL_LINK_STATE_STOPPING;
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		rtn = sl_ctl_link_reset_cmd(ctl_link);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_link, LOG_NAME, "link_reset_cmd failed [%d]", rtn);
			return rtn;
		}

		return 0;
	case SL_LINK_STATE_DOWN:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - already down");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return 0;
	case SL_LINK_STATE_STOPPING:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "down - already stopping");
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);

		return 0;
	case SL_LINK_STATE_INVALID:
	default:
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "reset - invalid (link_state = %u %s)",
			link_state, sl_link_state_str(link_state));
		spin_unlock_irqrestore(&ctl_link->data_lock, irq_flags);
		return -EBADRQC;
	}
}

void sl_ctl_link_up_clocks_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			       u32 *up_time, u32 *total_time)
{
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"clocks get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	spin_lock_irqsave(&ctl_link->up_clock.lock, irq_flags);
	if (!ktime_compare(ctl_link->up_clock.attempt_start, ktime_set(0, 0)))
		*up_time = ktime_to_ms(ctl_link->up_clock.attempt_elapsed); /* clock stopped */
	else
		*up_time = ktime_to_ms(ktime_sub(ktime_get(), ctl_link->up_clock.attempt_start));
	if (!ktime_compare(ctl_link->up_clock.start, ktime_set(0, 0)))
		*total_time = ktime_to_ms(ctl_link->up_clock.elapsed); /* clock stopped */
	else
		*total_time = ktime_to_ms(ktime_sub(ktime_get(), ctl_link->up_clock.start));
	spin_unlock_irqrestore(&ctl_link->up_clock.lock, irq_flags);
}

void sl_ctl_link_up_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *up_count)
{
	unsigned long       irq_flags;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"count get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return;
	}

	spin_lock_irqsave(&ctl_link->up_count_lock, irq_flags);
	*up_count = ctl_link->up_count;
	spin_unlock_irqrestore(&ctl_link->up_count_lock, irq_flags);
}

int sl_ctl_link_state_get_cmd(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *state)
{
	u32                 link_state;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_dbg(NULL, LOG_NAME,
			"state get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		*state = SL_LINK_STATE_INVALID;
		return 0;
	}

	link_state = sl_ctl_link_state_get(ctl_link);

	*state = link_state;

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"state get (state = %u %s, link_state = %u %s)",
		*state, sl_link_state_str(*state),
		link_state, sl_core_link_state_str(link_state));

	return 0;
}
