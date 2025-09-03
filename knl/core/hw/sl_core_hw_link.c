// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_ctl_link.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_link.h"
#include "hw/sl_core_hw_mac.h"
#include "hw/sl_core_hw_llr.h"
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_up.h"
#include "hw/sl_core_hw_serdes_link.h"
#include "hw/sl_core_hw_settings.h"
#include "hw/sl_core_hw_pcs.h"
#include "hw/sl_core_hw_fec.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_reset.h"
#include "sl_ctl_lgrp.h"

#define LOG_NAME SL_CORE_HW_LINK_LOG_NAME

static void sl_core_hw_link_off(struct sl_core_link *core_link)
{
	struct sl_core_mac *core_mac;
	struct sl_core_llr *core_llr;
	u64                 rx_state;
	u64                 tx_state;

	sl_core_log_dbg(core_link, LOG_NAME, "link off");

	core_mac = sl_core_mac_get(core_link->core_lgrp->core_ldev->num,
				   core_link->core_lgrp->num, core_link->num);
	core_llr = sl_core_llr_get(core_link->core_lgrp->core_ldev->num,
				   core_link->core_lgrp->num, core_link->num);

	if (core_mac) {
		rx_state = sl_core_hw_mac_rx_state_get(core_mac);
		tx_state = sl_core_hw_mac_tx_state_get(core_mac);
		/* stop TX mac before PCS so we don't lock up PCS */
		sl_core_hw_mac_tx_stop(core_mac);
	}

	if (core_llr)
		sl_core_llr_stop(core_link->core_lgrp->core_ldev->num,
				 core_link->core_lgrp->num, core_link->num);

	sl_core_hw_pcs_stop(core_link);

	if (core_mac)
		sl_core_hw_mac_rx_stop(core_mac);

	sl_core_hw_serdes_link_down(core_link);

	/* resetting the link resets the LLR which is needed to reset ordered sets */
	sl_core_hw_reset_link(core_link);

	if (core_mac) {
		if (tx_state == SL_CORE_MAC_STATE_ON)
			sl_core_hw_mac_tx_start(core_mac);
		if (rx_state == SL_CORE_MAC_STATE_ON)
			sl_core_hw_mac_rx_start(core_mac);
	}
}

static void sl_core_hw_link_high_ser_intr_work_priv(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "high SER intr work priv");

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_HIGH_SER);

	sl_core_log_warn_trace(core_link, LOG_NAME, "high symbol error ratio occurred");

	while (sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER) == -EALREADY) {
		if (sl_core_link_is_canceled_or_timed_out(core_link))
			return;
		if (sl_core_data_link_state_get(core_link) == SL_CORE_LINK_STATE_GOING_DOWN)
			return;
		usleep_range(10000, 12000);
		sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_HIGH_SER);
}

void sl_core_hw_link_up_callback(struct sl_core_link *core_link, struct sl_core_link_up_info *core_link_up_info)
{
	int rtn;

	sl_core_log_dbg(core_link, LOG_NAME, "up callback (callback = 0x%p, state = %d)",
		core_link->link.callbacks.up, core_link_up_info->state);

	rtn = core_link->link.callbacks.up(core_link->link.tags.up, core_link_up_info);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME, "up callback failed [%d]", rtn);
}

static void sl_core_hw_link_down_callback(struct sl_core_link *core_link)
{
	int rtn;
	u64 info_map;
	u64 down_cause_map;
	u32 state;

	sl_core_log_dbg(core_link, LOG_NAME, "down callback (callback = 0x%p)",
		core_link->link.callbacks.down);

	spin_lock(&core_link->link.data_lock);
	state          = core_link->link.state;
	down_cause_map = core_link->link.last_down_cause_map;
	info_map       = core_link->info_map;
	spin_unlock(&core_link->link.data_lock);

	rtn = core_link->link.callbacks.down(core_link->link.tags.down, state, down_cause_map, info_map);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME, "down callback failed [%d]", rtn);
}

void sl_core_hw_link_up_cmd(struct sl_core_link *core_link,
	sl_core_link_up_callback_t callback, void *tag)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);

	sl_core_data_link_is_last_down_new_set(core_link, true);
	sl_core_data_link_last_up_fail_cause_map_clr(core_link);

	sl_core_log_dbg(core_link, LOG_NAME,
		"up cmd (link = 0x%p, callback = 0x%p, flags = 0x%X)",
		core_link, callback, core_link->config.flags);

	core_link->link.tags.up      = tag;
	core_link->link.callbacks.up = callback;

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_MEDIA_CHECK);

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
	if (sl_media_lgrp_is_cable_not_supported(media_lgrp) &&
		!sl_core_link_policy_is_use_unsupported_cable_set(core_link)) {

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	if (media_lgrp->media_jack->is_supported_ss200_cable &&
		!sl_core_link_policy_is_use_supported_ss200_cable_set(core_link)) {

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_SS200_CABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	if (sl_media_lgrp_media_has_error(media_lgrp) &&
		!sl_core_link_policy_is_ignore_media_errors_set(core_link) &&
		!media_lgrp->media_jack->is_supported_ss200_cable) {

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_MEDIA_ERROR_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_MEDIA_CHECK);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_MEDIA_OK);

	sl_core_link_ccw_warn_limit_crossed_set(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num,
		core_link->num, false);
	sl_core_link_ucw_warn_limit_crossed_set(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num,
		core_link->num, false);

	if (is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
		queue_work(core_link->core_lgrp->core_ldev->workqueue,
			&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]));
	else
		queue_work(core_link->core_lgrp->core_ldev->workqueue,
			&(core_link->work[SL_CORE_WORK_LINK_UP_START]));
}

void sl_core_hw_link_up_start_work(struct work_struct *work)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_core_link  *core_link;
	u32                   link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_START]);

	sl_core_log_dbg(core_link, LOG_NAME, "up start work (link = 0x%p)", core_link);

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_GOING_UP) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up start work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	core_link->core_lgrp->link_caps[core_link->num].tech_map  = core_link->core_lgrp->config.tech_map;
	core_link->core_lgrp->link_caps[core_link->num].fec_map   = core_link->core_lgrp->config.fec_map;
	core_link->core_lgrp->link_caps[core_link->num].pause_map = core_link->config.pause_map;
	core_link->core_lgrp->link_caps[core_link->num].hpe_map   = core_link->config.hpe_map;

	rtn = sl_core_data_link_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up start data_link_settings failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_CONFIG_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_core_data_link_timeouts(core_link);

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP);

	sl_core_hw_link_off(core_link);

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up start link up disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up start link high SER disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up start link llr max starvation disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up start link llr starved disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up start link fault disable failed [%d]", rtn);

	/* clear faults here to see if faults happen during up */
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_FAULT);

	if (sl_media_lgrp_cable_type_is_active(core_link->core_lgrp->core_ldev->num,
		core_link->core_lgrp->num)) {
		media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num,
				core_link->core_lgrp->num);
		if (time_before(jiffies, media_lgrp->media_jack->cable_power_up_wait_time_end)) {
			core_link->timers[SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER].data.timeout_ms =
				jiffies_to_msecs(media_lgrp->media_jack->cable_power_up_wait_time_end - jiffies);
			sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);
			return;
		}
	}

	queue_work(core_link->core_lgrp->core_ldev->workqueue, &(core_link->work[SL_CORE_WORK_LINK_UP]));
}

void sl_core_hw_link_up_after_an_start(struct sl_core_link *core_link)
{
	int           rtn;
	u32           link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "up after an (link = 0x%p)", core_link);

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_AN:
		core_link->link.state = SL_CORE_LINK_STATE_GOING_UP;
		sl_core_log_dbg(core_link, LOG_NAME, "up after an start - going up");
		spin_unlock(&core_link->link.data_lock);
		break;
	case SL_CORE_LINK_STATE_CANCELING:
		sl_core_log_dbg(core_link, LOG_NAME, "up after an start - canceled");
		spin_unlock(&core_link->link.data_lock);
		return;
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "up after an start - timeout");
		spin_unlock(&core_link->link.data_lock);
		return;
	default:
		sl_core_log_err(core_link, LOG_NAME, "up after an start - invalid state (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return;
	};

	rtn = sl_core_data_link_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up after an data_link_settings failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_core_data_link_timeouts(core_link);

	sl_core_hw_link_off(core_link);

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up after an link up disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up after an link high SER disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up after an link llr max starvation disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up after an link llr starved disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up after an link fault disable failed [%d]", rtn);

	/* clear faults here to see if faults happen during up */
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_FAULT);

	queue_work(core_link->core_lgrp->core_ldev->workqueue, &(core_link->work[SL_CORE_WORK_LINK_UP]));
}

void sl_core_hw_link_up_work(struct work_struct *work)
{
	int                   rtn;
	struct sl_core_link  *core_link;
	u32                   link_state;
	struct sl_media_lgrp *media_lgrp;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP]);

	sl_core_log_dbg(core_link, LOG_NAME, "up work (link = 0x%p)", core_link);

	link_state = sl_core_data_link_state_get(core_link);
	if ((link_state != SL_CORE_LINK_STATE_GOING_UP) && (link_state != SL_CORE_LINK_STATE_AN)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_hw_serdes_link_down(core_link);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
		(core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
		rtn = sl_core_hw_intr_register(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
					       sl_core_hw_intr_hdlr, &(core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].data));
		if (rtn != 0) {
			sl_core_log_err(core_link, LOG_NAME,
				"up work - lane degrade register failed [%d]", rtn);

			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_REGISTER_MAP);
			rtn = sl_core_link_up_fail(core_link);
			if (rtn)
				sl_core_log_warn_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

			return;
		}
	}

	sl_core_hw_pcs_config(core_link);

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
	if ((media_lgrp->media_jack->is_supported_ss200_cable) &&
		((core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_CK_400G) ||
		(core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_CK_200G)  ||
		(core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_CK_100G))) {
		sl_core_log_err_trace(core_link, LOG_NAME, "link_up failed - speed not supported [%d]", rtn);
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_UNSUPPORTED_SPEED_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_warn_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);
		return;
	}

	if (core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_BS_200G) {
		rtn = sl_media_jack_cable_downshift(core_link->core_lgrp->core_ldev->num,
				core_link->core_lgrp->num, core_link->num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "downshift failed [%d]", rtn);

			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_DOWNSHIFT_MAP);
			rtn = sl_core_link_up_fail(core_link);
			if (rtn)
				sl_core_log_warn_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

			return;
		}
	} else {
		rtn = sl_media_jack_cable_upshift(core_link->core_lgrp->core_ldev->num,
				core_link->core_lgrp->num, core_link->num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "upshift failed [%d]", rtn);

			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_UPSHIFT_MAP);
			rtn = sl_core_link_up_fail(core_link);
			if (rtn)
				sl_core_log_warn_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

			return;
		}
	}

	rtn = sl_core_hw_serdes_link_up(core_link);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up work hw_serdes_link_up failed [%d]", rtn);

		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_UP);
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up work link up enable failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	if (!SL_PLATFORM_IS_HARDWARE(core_link->core_lgrp->core_ldev))
		sl_core_hw_pcs_tx_start(core_link);

	sl_core_hw_pcs_rx_start(core_link);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
		(core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
		sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);

		rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "up work link ald enable failed [%d]", rtn);

			spin_lock(&core_link->data_lock);
			core_link->degrade_state = SL_LINK_DEGRADE_STATE_FAILED;
			spin_unlock(&core_link->data_lock);

			sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
			rtn = sl_core_link_up_fail(core_link);
			if (rtn)
				sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

			return;
		}

		/*
		 * enable ALD after PCS start but before MAC start
		 */
		sl_core_hw_pcs_enable_auto_lane_degrade(core_link);
	} else {
		spin_lock(&core_link->data_lock);
		core_link->degrade_state = SL_LINK_DEGRADE_STATE_DISABLED;
		spin_unlock(&core_link->data_lock);
	}
}

void sl_core_hw_link_up_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "up intr work");

	link_state = sl_core_data_link_state_get(core_link);
	if ((link_state != SL_CORE_LINK_STATE_GOING_UP) && (link_state != SL_CORE_LINK_STATE_AN)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up intr work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
}

static bool sl_core_hw_link_media_check(struct sl_core_link *core_link)
{
	u32                   flags;
	struct sl_media_lgrp *media_lgrp;

	sl_core_log_dbg(core_link, LOG_NAME, "media check");

	flags = sl_core_data_lgrp_config_flags_get(core_link->core_lgrp);

	if (is_flag_set(flags, SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
		sl_core_log_dbg(core_link, LOG_NAME, "serdes loopback set (flags = %u)", flags);
		return true;
	}

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);

	return sl_media_jack_is_cable_online(media_lgrp->media_jack);
}

static void sl_core_hw_link_up_success(struct sl_core_link *core_link)
{
	int                                rtn;
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;
	struct sl_core_link_up_info        link_up_info;
	u32                                link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "up success");

	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	if (rtn == -EAGAIN) {
		sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
		sl_core_hw_link_high_ser_intr_work_priv(core_link);
	} else if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up success link high SER enable failed [%d]", rtn);
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);
		return;
	}
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up success link llr max starvation enable failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up success link llr starved enable failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up success link fault enable failed [%d]", rtn);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_ctl_link_fec_down_cache_clear(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
		core_link->core_lgrp->num, core_link->num));

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_core_log_warn_trace(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
		sl_ctl_link_fec_up_cache_clear(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num));
	} else {
		sl_ctl_link_fec_up_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);
	}

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_GOING_UP:
		sl_core_log_dbg(core_link, LOG_NAME, "up success - up");
		set_bit(SL_CORE_INFO_MAP_LINK_UP, (unsigned long *)&(core_link->info_map));

		core_link->link.state                  = SL_CORE_LINK_STATE_UP;
		core_link->link.last_up_fail_cause_map = SL_LINK_DOWN_CAUSE_NONE;
		core_link->link.last_up_fail_time      = 0;

		link_up_info.state                     = core_link->link.state;
		link_up_info.cause_map                 = core_link->link.last_up_fail_cause_map;
		link_up_info.info_map                  = core_link->info_map;
		link_up_info.speed                     = core_link->pcs.settings.speed;
		link_up_info.fec_mode                  = core_link->fec.settings.mode;
		link_up_info.fec_type                  = core_link->fec.settings.type;

		spin_unlock(&core_link->link.data_lock);
		sl_media_jack_led_set(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
		sl_core_hw_link_up_callback(core_link, &link_up_info);
		return;
	default:
		sl_core_log_err(core_link, LOG_NAME, "up success - invalid state (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return;
	};
}

void sl_core_hw_link_up_check_work(struct work_struct *work)
{
	int                  rtn;
	u32                  port;
	u64                  data64;
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_CHECK]);

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "up check work (port = %u)", port);

	link_state = sl_core_data_link_state_get(core_link);
	if ((link_state != SL_CORE_LINK_STATE_GOING_UP) && (link_state != SL_CORE_LINK_STATE_AN)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up check work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	/* check PCS */
	if (!sl_core_hw_pcs_is_ok(core_link)) {
		sl_core_log_warn_trace(core_link, LOG_NAME, "up check work pcs is not ok");

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_PCS_FAULT_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	/* check to see if there were faults during up */
	sl_core_hw_intr_flgs_check(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_FAULT);

	/* clear underrun */
	sl_core_read64(core_link, SS2_PORT_PML_ERR_INFO_PCS_TX_DP, &data64);
	data64 = SS2_PORT_PML_ERR_INFO_PCS_TX_DP_TX_CDC_UNDERRUN_UPDATE(data64,
		core_link->pcs.settings.underrun_clr);
	sl_core_write64(core_link, SS2_PORT_PML_ERR_INFO_PCS_TX_DP, data64);

	if (core_link->fec.settings.up_settle_wait_ms) {
		sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
		return;
	}

	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);

	sl_core_hw_link_up_success(core_link);
}

void sl_core_hw_link_up_fec_settle_work(struct work_struct *work)
{
	int                                  rtn;
	struct sl_core_link_fec_cw_cntrs     cw_cntrs;
	struct sl_core_link_fec_lane_cntrs   lane_cntrs;
	struct sl_core_link_fec_tail_cntrs   tail_cntrs;
	struct sl_core_link                 *core_link;
	struct sl_ctl_link                  *ctl_link;
	u32                                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]);
	ctl_link = sl_ctl_link_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, core_link->num);

	sl_core_log_dbg(core_link, LOG_NAME, "up fec settle work");

	link_state = sl_core_data_link_state_get(core_link);
	if ((link_state != SL_CORE_LINK_STATE_GOING_UP) && (link_state != SL_CORE_LINK_STATE_AN)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up fec settle work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn_trace(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_data_store(ctl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
}

void sl_core_hw_link_up_fec_check_work(struct work_struct *work)
{
	int                                  rtn;
	struct sl_core_link_fec_cw_cntrs     cw_cntrs;
	struct sl_core_link_fec_lane_cntrs   lane_cntrs;
	struct sl_core_link_fec_tail_cntrs   tail_cntrs;
	struct sl_fec_info                   fec_info;
	struct sl_core_link                 *core_link;
	struct sl_ctl_link                  *ctl_link;
	u32                                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_FEC_CHECK]);
	ctl_link = sl_ctl_link_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, core_link->num);

	sl_core_log_dbg(core_link, LOG_NAME, "up fec check work");

	link_state = sl_core_data_link_state_get(core_link);
	if ((link_state != SL_CORE_LINK_STATE_GOING_UP) && (link_state != SL_CORE_LINK_STATE_AN)) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up fec check work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_CHECK);

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up fec check work hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_data_store(ctl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_ctl_link_fec_data_calc(ctl_link);
	fec_info = sl_ctl_link_fec_data_info_get(ctl_link);

	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);

	if (SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(core_link->fec.settings.up_ucw_limit, &fec_info)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"UCW exceeded up limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);

		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_FEC_OK);
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_UCW_UP_CHECK_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	if (SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(core_link->fec.settings.up_ccw_limit, &fec_info)) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"CCW exceeded up limit (UCW = %llu, CCW = %llu)",
			fec_info.ucw, fec_info.ccw);

		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_CCW_UP_CHECK_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link_up_fail failed [%d]", rtn);

		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_FEC_CHECK);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_OK);

	sl_core_hw_link_up_success(core_link);
}

void sl_core_hw_link_up_timeout_work(struct work_struct *work)
{
	struct sl_core_link         *core_link;
	struct sl_core_link_up_info  link_up_info;
	u32                          link_state;
	int                          rtn;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_TIMEOUT]);

	sl_core_log_dbg(core_link, LOG_NAME, "up timeout work");

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		sl_core_log_dbg(core_link, LOG_NAME, "up timeout work - going down");
		core_link->link.state                   = SL_CORE_LINK_STATE_TIMEOUT;
		core_link->link.last_up_fail_cause_map |= SL_LINK_DOWN_CAUSE_TIMEOUT_MAP;
		spin_unlock(&core_link->link.data_lock);
		break;
	default:
		sl_core_log_err(core_link, LOG_NAME, "up timeout work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_UP_TIMEOUT);

	/* stop timers */
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	/* cancel work - except for timeout work */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FAIL]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CANCEL]));

	/* disable interrupts */
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
	   (core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
	   rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
					    sl_core_hw_intr_hdlr);
	   if (rtn != 0)
		   sl_core_log_warn(core_link, LOG_NAME,
				   "up timeout - lane degrade unregister failed [%d]", rtn);
	}

	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]));

	/* stop hardware */
	sl_core_hw_an_stop(core_link);
	sl_core_hw_link_off(core_link);

	if (!sl_core_hw_link_media_check(core_link))
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_NO_MEDIA);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	sl_core_hw_link_up_callback(core_link, sl_core_link_up_info_get(core_link, &link_up_info));

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
}

void sl_core_hw_link_up_cancel_work(struct work_struct *work)
{
	struct sl_core_link         *core_link;
	struct sl_core_link_up_info  link_up_info;
	u32                          link_state;
	int                          rtn;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_CANCEL]);

	link_state = sl_core_data_link_state_get(core_link);

	sl_core_log_dbg(core_link, LOG_NAME, "up cancel work (link_state = %u %s)", link_state,
		sl_core_link_state_str(link_state));

	if (link_state != SL_CORE_LINK_STATE_CANCELING) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up cancel work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_UP_CANCEL);

	/* stop timers */
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	/* cancel work */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_TIMEOUT]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FAIL]));

	/* disable interrupts */
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
	   (core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
	   rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
					    sl_core_hw_intr_hdlr);
	   if (rtn != 0)
		   sl_core_log_warn(core_link, LOG_NAME,
				   "up cancel - lane degrade unregister failed [%d]", rtn);
	}

	/* Cancel work queued from interrupt */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]));

	/* stop hardware */
	sl_core_hw_an_stop(core_link);
	sl_core_hw_link_off(core_link);

	if (!sl_core_hw_link_media_check(core_link))
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_NO_MEDIA);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	sl_core_hw_link_up_callback(core_link, sl_core_link_up_info_get(core_link, &link_up_info));

	sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_UP_CANCELED_MAP);
	sl_core_hw_link_down_callback(core_link);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
}

void sl_core_hw_link_up_fail_work(struct work_struct *work)
{
	struct sl_core_link         *core_link;
	struct sl_core_link_up_info  link_up_info;
	u32                          link_state;
	int                          rtn;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_FAIL]);

	sl_core_log_dbg(core_link, LOG_NAME, "up fail work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_GOING_DOWN) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up fail work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_UP_FAIL);

	/* stop timers */
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	/* cancel work */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_START]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_TIMEOUT]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CANCEL]));

	/* disable interrupts */
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
	   (core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
	   rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
					    sl_core_hw_intr_hdlr);
	   if (rtn != 0)
		   sl_core_log_warn(core_link, LOG_NAME,
				   "up fail - lane degrade unregister failed [%d]", rtn);
	}

	/* Cancel work queued from interrupt */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]));

	/* stop hardware */
	sl_core_hw_an_stop(core_link);
	sl_core_hw_link_off(core_link);

	if (!sl_core_hw_link_media_check(core_link))
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_NO_MEDIA);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	sl_core_hw_link_up_callback(core_link, sl_core_link_up_info_get(core_link, &link_up_info));

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
}

void sl_core_hw_link_down_work(struct work_struct *work)
{
	int                                 rtn;
	struct sl_core_link_fec_cw_cntrs    cw_cntrs;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;
	struct sl_core_link                *core_link;
	u32                                 link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_DOWN]);

	sl_core_log_dbg(core_link, LOG_NAME, "down work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_GOING_DOWN) {
		sl_core_log_err_trace(core_link, LOG_NAME, "down work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_DOWN);

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_HIGH_SER);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"down work link high SER disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"down work link llr max starvation disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"down work link llr starved disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"down work link fault disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"down work lane degrade disable failed [%d]", rtn);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
	   (core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
	   rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
					    sl_core_hw_intr_hdlr);
	   if (rtn != 0)
		   sl_core_log_warn(core_link, LOG_NAME,
				   "down - lane degrade unregister failed [%d]", rtn);
	}

	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_HIGH_SER_INTR]);
	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]);
	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]);
	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]);
	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]);

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn_trace(core_link, LOG_NAME, "down work hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_down_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_core_hw_link_off(core_link);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	sl_core_hw_link_down_callback(core_link);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
}

void sl_core_hw_link_high_ser_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_HIGH_SER_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "high SER intr work");

	link_state = sl_core_data_link_state_get(core_link);
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_AN:
		break;
	default:
		sl_core_log_err(core_link, LOG_NAME, "high SER intr work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
			return;
	}

	sl_core_hw_link_high_ser_intr_work_priv(core_link);
}

void sl_core_hw_link_llr_max_starvation_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;
	u32				     link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_LLR_MAX_STARVATION_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "llr max starvation intr work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_err_trace(core_link, LOG_NAME, "llr max starvation intr work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LLR_MAX_STARVATION);

	sl_core_log_warn_trace(core_link, LOG_NAME, "llr max starvation occurred");

	while (sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION) == -EALREADY) {
		if (sl_core_link_is_canceled_or_timed_out(core_link))
			return;
		if (sl_core_data_link_state_get(core_link) == SL_CORE_LINK_STATE_GOING_DOWN)
			return;
		usleep_range(10000, 12000);
		sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_MAX_STARVATION);
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LLR_MAX_STARVATION);
}

void sl_core_hw_link_llr_starved_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_LLR_STARVED_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "llr starved intr work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_UP) {
		sl_core_log_dbg(core_link, LOG_NAME, "llr starved intr work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LLR_STARVED);

	sl_core_log_warn_trace(core_link, LOG_NAME, "llr starved occurred");

	while (sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED) == -EALREADY) {
		if (sl_core_link_is_canceled_or_timed_out(core_link))
			return;
		if (sl_core_data_link_state_get(core_link) == SL_CORE_LINK_STATE_GOING_DOWN)
			return;
		usleep_range(10000, 12000);
		sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_LLR_STARVED);
	}

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LLR_STARVED);
}

void sl_core_hw_link_fault_intr_work(struct work_struct *work)
{
	int                                 rtn;
	u32                                 link_state;
	u64                                 link_down;
	u64                                 remote_fault;
	u64                                 local_fault;
	u64                                 llr_replay_max;
	struct sl_core_link                *core_link;
	struct sl_core_link_fec_cw_cntrs    cw_cntrs;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;
	u64                                 data64;
	u64                                 replay_ct_max;
	u32                                 port;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_FAULT_INTR]);
	port      = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME,
		"fault intr work (port = %u, flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		port,
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[0],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[3]);

	switch (core_link->num) {
	case 0:
		link_down =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_0_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		remote_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_0_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		local_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_0_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		llr_replay_max =
		SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_0_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		break;
	case 1:
		link_down =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_1_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		remote_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_1_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		local_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_1_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		llr_replay_max =
		SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_1_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		break;
	case 2:
		link_down =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_2_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		remote_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_2_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		local_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_2_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		llr_replay_max =
		SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_2_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		break;
	case 3:
		link_down =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_3_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		remote_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_RF_3_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		local_fault =
		SS2_PORT_PML_ERR_FLG_WORD1_PCS_LINK_DOWN_LF_3_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		llr_replay_max =
		SS2_PORT_PML_ERR_FLG_WORD1_LLR_REPLAY_AT_MAX_3_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1]);
		break;
	default:
		link_down      = 0;
		remote_fault   = 0;
		local_fault    = 0;
		llr_replay_max = 0;
		break;
	}

	sl_core_log_dbg(core_link, LOG_NAME,
		"fault intr work (llr_replay = 0x%llX, local = 0x%llX, remote = 0x%llX, down = 0x%llX)",
		llr_replay_max, local_fault, remote_fault, link_down);

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
		core_link->link.state = SL_CORE_LINK_STATE_GOING_DOWN;
		core_link->config.fault_start_callback(core_link->core_lgrp->core_ldev->num,
						       core_link->core_lgrp->num, core_link->num);
		spin_unlock(&core_link->link.data_lock);
		break;
	default:
		sl_core_log_dbg(core_link, LOG_NAME, "fault intr work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return;
	}

	sl_media_jack_led_set(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);

	if (llr_replay_max) {
		sl_core_read64(core_link, SS2_PORT_PML_CFG_LLR_SM(core_link->num), &data64);
		replay_ct_max = SS2_PORT_PML_CFG_LLR_SM_REPLAY_CT_MAX_GET(data64);
		if (replay_ct_max == 0xFF) {
			sl_core_log_err_trace(core_link, LOG_NAME, "llr replay max occurred, ignored");
			if (!local_fault && !remote_fault && !link_down)
				return;
		} else {
			sl_core_log_err_trace(core_link, LOG_NAME, "llr replay max occurred");
			sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);
			sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX_MAP);
		}
	}
	if (local_fault) {
		sl_core_log_err_trace(core_link, LOG_NAME, "local fault occurred");
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_LOCAL_FAULT);
		sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_LF_MAP);
	}
	if (remote_fault) {
		sl_core_log_err_trace(core_link, LOG_NAME, "remote fault occurred");
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_REMOTE_FAULT);
		sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_RF_MAP);
	}
	if (link_down) {
		sl_core_log_err_trace(core_link, LOG_NAME, "link down occurred");
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_LINK_DOWN);
		sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_DOWN_MAP);
	}

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"fault intr work link up disable failed [%d]", rtn);

	if ((sl_core_link_config_is_enable_ald_set(core_link)) &&
	    (core_link->core_lgrp->config.furcation == SL_MEDIA_FURCATION_X1)) {
		rtn = sl_core_hw_intr_unregister(core_link, core_link->intrs[SL_CORE_HW_INTR_LANE_DEGRADE].flgs,
						 sl_core_hw_intr_hdlr);
		if (rtn != 0)
			sl_core_log_warn(core_link, LOG_NAME,
					 "fault intr - lane degrade unregister failed [%d]", rtn);
	}

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"fault intr work hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_down_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_core_hw_link_off(core_link);

	if (!sl_core_hw_link_media_check(core_link))
		sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_NO_MEDIA);

	if (!is_flag_set(sl_core_data_lgrp_config_flags_get(core_link->core_lgrp),
							    SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE))
		if (sl_media_jack_cable_is_high_temp((sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num,
						     core_link->core_lgrp->num))->media_jack))
			sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_HIGH_TEMP);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	rtn = core_link->config.fault_callback(core_link->link.tags.up, sl_core_data_link_state_get(core_link),
		sl_core_data_link_last_down_cause_map_get(core_link),
		sl_core_data_link_info_map_get(core_link));
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"fault intr work callback failed [%d]", rtn);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);
}

static inline int sl_core_hw_link_speed_get(u32 pcs_mode, int hweight)
{
    return hweight * (pcs_mode == SL_CORE_HW_PCS_MODE_CK_400G ? 100 : 50);
}

void sl_core_hw_link_lane_degrade_intr_work(struct work_struct *work)
{
	struct sl_core_link        *core_link;
	union sl_lgrp_notif_info    info;
	struct sl_link_degrade_info ald_info;
	u32                         port;
	u64                         val64;
	u64                         data64;
	int                         rtn;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_LANE_DEGRADE_INTR]);
	port      = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "lane degrade intr work (port = %u)", port);

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_DEGRADED);

	sl_core_read64(core_link, SS2_PORT_PML_STS_PCS_LANE_DEGRADE, &data64);

	val64 = SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_RX_PLS_AVAILABLE_GET(data64);
	ald_info.rx_lane_map = (u8)val64;

	val64 = SS2_PORT_PML_STS_PCS_LANE_DEGRADE_WORD0_LP_PLS_AVAILABLE_GET(data64);
	ald_info.tx_lane_map = (u8)val64;

	if ((ald_info.rx_lane_map != core_link->degrade_info.rx_lane_map) &&
		(ald_info.rx_lane_map != 0xF))
		ald_info.is_rx_degrade = true;
	else
		ald_info.is_rx_degrade = false;

	if ((ald_info.tx_lane_map != core_link->degrade_info.tx_lane_map) &&
		(ald_info.tx_lane_map != 0xF))
		ald_info.is_tx_degrade = true;
	else
		ald_info.is_tx_degrade = false;

	ald_info.rx_link_speed = sl_core_hw_link_speed_get(core_link->pcs.settings.pcs_mode,
		hweight8(ald_info.rx_lane_map));
	ald_info.tx_link_speed = sl_core_hw_link_speed_get(core_link->pcs.settings.pcs_mode,
		hweight8(ald_info.tx_lane_map));

	sl_core_log_dbg(core_link, LOG_NAME, "is_rx_degrade: %d, lane_map: 0x%X, link_speed: %u",
			ald_info.is_rx_degrade, ald_info.rx_lane_map, ald_info.rx_link_speed);
	sl_core_log_dbg(core_link, LOG_NAME, "is_tx_degrade: %d, lane_map: 0x%X, link_speed: %u",
			ald_info.is_tx_degrade, ald_info.tx_lane_map, ald_info.tx_link_speed);

	spin_lock(&core_link->data_lock);
	core_link->degrade_info = ald_info;
	info.degrade_info       = ald_info;
	spin_unlock(&core_link->data_lock);

	rtn = sl_ctl_lgrp_notif_enqueue(sl_ctl_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num),
			core_link->num, SL_LGRP_NOTIF_LANE_DEGRADE, &info, 0);
	if (rtn)
		sl_media_log_warn_trace(core_link, LOG_NAME,
			"lane degrade ctl_lgrp_notif_enqueue failed [%d]", rtn);

	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LANE_DEGRADE);
	if (rtn)
		sl_core_log_err_trace(core_link, LOG_NAME, "intr work ald enable failed [%d]", rtn);
}
