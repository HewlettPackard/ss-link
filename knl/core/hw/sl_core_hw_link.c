// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/preempt.h>

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
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_up.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_settings.h"
#include "hw/sl_core_hw_pcs.h"
#include "hw/sl_core_hw_fec.h"
#include "hw/sl_core_hw_io.h"

#define LOG_NAME SL_CORE_HW_LINK_LOG_NAME

static void sl_core_hw_link_off(struct sl_core_link *core_link)
{
	struct sl_core_mac *core_mac;

	preempt_disable();

	core_mac = sl_core_mac_get(core_link->core_lgrp->core_ldev->num,
				   core_link->core_lgrp->num, core_link->num);
	if (core_mac) {
		sl_core_hw_mac_tx_stop(core_mac);
		sl_core_hw_mac_rx_stop(core_mac);
	}

	sl_core_hw_pcs_stop(core_link);

	preempt_enable();

	sl_core_hw_serdes_link_down(core_link);
}

void sl_core_hw_link_up_callback(struct sl_core_link *core_link)
{
	int rtn;

	sl_core_log_dbg(core_link, LOG_NAME, "up callback (callback = 0x%p, state = %d)",
		core_link->link.callbacks.up, core_link->link.state);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up callback canceled");
		return;
	}

	rtn = core_link->link.callbacks.up(core_link->link.tag, core_link->link.state,
		(core_link->link.state == SL_CORE_LINK_STATE_UP) ? SL_LINK_DOWN_CAUSE_NONE : core_link->link.last_down_cause,
		core_link->info_map, core_link->pcs.settings.speed,
		core_link->fec.settings.mode, core_link->fec.settings.type);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME, "up callback failed [%d]", rtn);
}

void sl_core_hw_link_up_cmd(struct sl_core_link *core_link,
	sl_core_link_up_callback_t callback, void *tag)
{
	struct sl_media_lgrp *media_lgrp;

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_NUM_BITS);

	sl_core_log_dbg(core_link, LOG_NAME,
		"up cmd (link = 0x%p, callback = 0x%p, flags = 0x%X)",
		core_link, callback, core_link->config.flags);

	core_link->link.tag          = tag;
	core_link->link.callbacks.up = callback;
	sl_core_link_is_canceled_clr(core_link);

	media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
	if (sl_media_lgrp_is_cable_not_supported(media_lgrp) &&
		!(core_link->policy.options & SL_LINK_POLICY_OPT_USE_UNSUPPORTED_CABLE)) {
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	if (is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_ENABLE))
		sl_core_hw_an_up_start(core_link);
	else
		sl_core_hw_link_up_start(core_link);
}

void sl_core_hw_link_up_start(struct sl_core_link *core_link)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	sl_core_log_dbg(core_link, LOG_NAME, "up start (link = 0x%p)", core_link);

	core_link->core_lgrp->link_caps[core_link->num].tech_map  = core_link->core_lgrp->config.tech_map;
	core_link->core_lgrp->link_caps[core_link->num].fec_map   = core_link->core_lgrp->config.fec_map;
	core_link->core_lgrp->link_caps[core_link->num].pause_map = core_link->config.pause_map;
	core_link->core_lgrp->link_caps[core_link->num].hpe_map   = core_link->config.hpe_map;
	rtn = sl_core_data_link_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"up start data_link_settings failed [%d]", rtn);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_CONFIG);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_core_data_link_timeouts(core_link);

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP);

	sl_core_hw_link_off(core_link);

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up start link up disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up start link non fatal disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up start link fault disable failed [%d]", rtn);

	/* clear faults here to see if faults happen during up */
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
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

	sl_core_work_link_queue(core_link, SL_CORE_WORK_LINK_UP);

}

void sl_core_hw_link_up_after_an_start(struct sl_core_link *core_link)
{
	int rtn;

	sl_core_log_dbg(core_link, LOG_NAME, "up after an (link = 0x%p)", core_link);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_GOING_UP);

	rtn = sl_core_data_link_settings(core_link);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"up after an data_link_settings failed [%d]", rtn);
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn(core_link, LOG_NAME,
				"up after an link up end failed [%d]", rtn);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_CONFIG);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_core_data_link_timeouts(core_link);

	sl_core_hw_link_off(core_link);

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up after an link up disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up after an link non fatal disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up after an link fault disable failed [%d]", rtn);

	/* clear faults here to see if faults happen during up */
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_FAULT);

	sl_core_work_link_queue(core_link, SL_CORE_WORK_LINK_UP);

}

void sl_core_hw_link_up_work(struct work_struct *work)
{
	int                   rtn;
	struct sl_core_link  *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP]);

	sl_core_log_dbg(core_link, LOG_NAME, "up work (link = 0x%p)", core_link);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up work canceled");
		return;
	}

	sl_core_hw_serdes_link_down(core_link);

	sl_core_hw_pcs_config(core_link);

	if (core_link->pcs.settings.pcs_mode == SL_CORE_HW_PCS_MODE_BS_200G) {
		rtn = sl_media_jack_cable_downshift(core_link->core_lgrp->core_ldev->num,
				core_link->core_lgrp->num);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "downshift failed [%d]", rtn);
			rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
			if (rtn < 0)
				sl_core_log_warn(core_link, LOG_NAME, "up work link up end failed [%d]", rtn);
			sl_core_hw_serdes_link_down(core_link);
			sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_DOWNSHIFT_FAILED);
			sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
			sl_core_hw_link_up_callback(core_link);
			return;
		}
	}

	sl_core_hw_pcs_tx_start(core_link);

	rtn = sl_core_hw_serdes_link_up(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up work hw_serdes_link_up failed [%d]", rtn);
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn(core_link, LOG_NAME, "up work link up end failed [%d]", rtn);
		sl_core_hw_serdes_link_down(core_link);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_SERDES);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_core_hw_intr_flgs_clr(core_link, SL_CORE_HW_INTR_LINK_UP);
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0) {
		sl_core_log_err(core_link, LOG_NAME,
			"up work link up enable failed [%d]", rtn);
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn(core_link, LOG_NAME,
				"up work link up end failed [%d]", rtn);
		sl_core_hw_serdes_link_down(core_link);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_core_hw_pcs_rx_start(core_link);
}

void sl_core_hw_link_up_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "up intr work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up intr work canceled");
		return;
	}

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
}

static void sl_core_hw_link_up_success(struct sl_core_link *core_link)
{
	int                                rtn;
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "up success");

	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	if (rtn != 0) {
		if (rtn != -EALREADY)
			sl_core_log_err(core_link, LOG_NAME,
				"up success link non fatal enable failed [%d]", rtn);
		sl_core_hw_link_off(core_link);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}
	rtn = sl_core_hw_intr_flgs_enable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0) {
		if (rtn != -EALREADY)
			sl_core_log_err(core_link, LOG_NAME,
				"up success link fault enable failed [%d]", rtn);
		sl_core_hw_link_off(core_link);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_INTR_ENABLE);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_ctl_link_fec_down_cache_clear(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
		core_link->core_lgrp->num, core_link->num));

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn) {
		sl_core_log_warn(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
		sl_ctl_link_fec_up_cache_clear(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num));
	} else {
		sl_ctl_link_fec_up_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_UP);
	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_UP);
	sl_core_hw_link_up_callback(core_link);
}

void sl_core_hw_link_up_check_work(struct work_struct *work)
{
	int                  rtn;
	u32                  port;
	u64                  data64;
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_CHECK]);

	port = core_link->core_lgrp->num;

	sl_core_log_dbg(core_link, LOG_NAME, "up check work (port = %u)", port);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up check work canceled");
		return;
	}

	/* check PCS */
	if (!sl_core_hw_pcs_is_ok(core_link)) {
		sl_core_log_warn(core_link, LOG_NAME, "up check work pcs is not ok");
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn(core_link, LOG_NAME,
				"up check work link up end failed [%d]", rtn);
		sl_core_hw_link_off(core_link);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_DOWN);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
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

	rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	if (rtn < 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up check work link up end failed [%d]", rtn);

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

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]);
	ctl_link = sl_ctl_link_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, core_link->num);

	sl_core_log_dbg(core_link, LOG_NAME, "up fec settle work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up fec settle work canceled");
		return;
	}

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
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

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_FEC_CHECK]);
	ctl_link = sl_ctl_link_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num, core_link->num);

	sl_core_log_dbg(core_link, LOG_NAME, "up fec check work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up fec check work");
		return;
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_CHECK);

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_data_store(ctl_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_ctl_link_fec_data_calc(ctl_link);
	fec_info = sl_ctl_link_fec_data_info_get(ctl_link);

	rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	if (rtn < 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up fec check work link up end failed [%d]", rtn);

	/* UCW has precedence over CCW */
	if (SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(core_link->fec.settings.up_ucw_limit, &fec_info)) {
		sl_core_log_warn(core_link, LOG_NAME,
			"UCW exceeded limit (UCW = %llu CCW = %llu)",
			fec_info.ucw, fec_info.ccw);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_FEC_OK);
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_UCW_HIGH);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_UCW);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	if (SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(core_link->fec.settings.up_ccw_limit, &fec_info)) {
		sl_core_log_warn(core_link, LOG_NAME,
			"CCW exceeded limit (UCW = %llu CCW = %llu)",
			fec_info.ucw, fec_info.ccw);
		sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_FEC_OK);
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_CCW_HIGH);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_CCW);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_hw_link_up_callback(core_link);
		return;
	}

	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_FEC_OK);

	sl_core_hw_link_up_success(core_link);
}

void sl_core_hw_link_up_timeout_work(struct work_struct *work)
{
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_UP_TIMEOUT]);

	sl_core_log_dbg(core_link, LOG_NAME, "up timeout work");

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_TIMEOUT);

	sl_core_link_is_timed_out_set(core_link);

	/* disable interrupts */
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);

	/* stop timers */
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	/* cancel work - except for timeout work */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_NON_FATAL_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));

	/* stop hardware */
	sl_core_hw_an_stop(core_link);
	sl_core_hw_link_off(core_link);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_AN_DONE);
	sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LINK_UP_TIMEOUT);

	sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_TIMEOUT);
	sl_core_link_is_timed_out_clr(core_link);
	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	sl_core_hw_link_up_callback(core_link);
}

void sl_core_hw_link_up_cancel_cmd(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "up cancel cmd");

	sl_core_link_is_canceled_set(core_link);

	/* disable interrupts */
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);

	/* stop timers */
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_SETTLE);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_FEC_CHECK);
	sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP_XCVR_HIGH_POWER);

	/* cancel work */
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_AN_UP_DONE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_TIMEOUT]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_SETTLE]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_UP_FEC_CHECK]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_NON_FATAL_INTR]));
	cancel_work_sync(&(core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]));

	/* stop hardware */
	sl_core_hw_an_stop(core_link);
	sl_core_hw_link_off(core_link);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LINK_UP);
	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LINK_UP_TIMEOUT);

	sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_COMMAND);
	sl_core_link_is_canceled_clr(core_link);
	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
	sl_core_hw_link_up_callback(core_link);
}

void sl_core_hw_link_down_cmd(struct sl_core_link *core_link)
{
	int                                rtn;
	struct sl_core_link_fec_cw_cntrs   cw_cntrs;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;

	sl_core_log_dbg(core_link, LOG_NAME, "down cmd");

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_NON_FATAL);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"down cmd - link non fatal disable failed [%d]", rtn);
	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_FAULT);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"down cmd - link fault disable failed [%d]", rtn);

	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_NON_FATAL_INTR]);
	cancel_work_sync(&core_link->work[SL_CORE_WORK_LINK_FAULT_INTR]);

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_down_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_core_hw_link_off(core_link);

	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LINK_UP);
	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LINK_UP_TIMEOUT);

	sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_COMMAND);
	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
}

void sl_core_hw_link_down_wait(struct sl_core_link *core_link)
{
	int  try_count;
	char info_map_str[64];

	sl_core_log_dbg(core_link, LOG_NAME, "down wait");

	try_count = 0;
	while (sl_core_data_link_state_get(core_link) != SL_CORE_LINK_STATE_DOWN) {
		usleep_range(1000, 2000);
		if (try_count++ > 10 * 1000) /* apporox 10 to 20 seconds */ {
			sl_core_info_map_str(core_link->info_map, info_map_str, sizeof(info_map_str));
			sl_core_log_warn(core_link, LOG_NAME,
				"down wait tries exceeded - failed to get to down (state = %u %s, info = %s)",
				core_link->link.state, sl_core_link_state_str(core_link->link.state),
				info_map_str);
			return;
		}
	}
}

void sl_core_hw_link_non_fatal_intr_work(struct work_struct *work)
{
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_NON_FATAL_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME, "non fatal fault intr work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "non fatal fault intr work canceled");
		return;
	}

	sl_core_log_warn(core_link, LOG_NAME, "hi serdes just occurred");
}

void sl_core_hw_link_fault_intr_work(struct work_struct *work)
{
	int                                 rtn;
	u64                                 link_down;
	u64                                 remote_fault;
	u64                                 local_fault;
	u64                                 llr_starvation;
	u64                                 llr_replay_max;
	struct sl_core_link                *core_link;
	struct sl_core_link_fec_cw_cntrs    cw_cntrs;
	struct sl_core_link_fec_lane_cntrs  lane_cntrs;
	struct sl_core_link_fec_tail_cntrs  tail_cntrs;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_FAULT_INTR]);

	sl_core_log_dbg(core_link, LOG_NAME,
		"fault intr work (flgs = 0x%016llX, 0x%016llX, 0x%016llX, 0x%016llX)",
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[0],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[1],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2],
		core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[3]);

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "fault intr work canceled");
		return;
	}

#ifdef BUILDSYS_FRAMEWORK_ROSETTA
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
		llr_starvation =
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_MAX_STARVATION_LIMIT_0_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2]);
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
		llr_starvation =
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_MAX_STARVATION_LIMIT_1_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2]);
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
		llr_starvation =
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_MAX_STARVATION_LIMIT_2_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2]);
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
		llr_starvation =
		SS2_PORT_PML_ERR_FLG_WORD2_LLR_MAX_STARVATION_LIMIT_3_GET(core_link->intrs[SL_CORE_HW_INTR_LINK_FAULT].source[2]);
		break;
	}
#else /* Cassini */
	// FIXME: need to add GETs to cassini headers
	link_down = 0;
	remote_fault = 0;
	local_fault = 0x1;
	llr_starvation = 0;
	llr_replay_max = 0;
#endif /* BUILDSYS_FRAMEWORK_ROSETTA */

	/* llr starvation indicator is first */
	if (llr_starvation) {
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LLR_STARVED);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_LLR_STARVED);
		goto out_down;
	}

	/* llr replay max indicator is second */
	if (llr_replay_max) {
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_LLR_REPLAY_MAX);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX);
		goto out_down;
	}

	/* fault indicators are checked worst first */
	if (local_fault) {
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_LOCAL_FAULT);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_LF);
		goto out_down;
	}
	if (remote_fault) {
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_REMOTE_FAULT);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_RF);
		goto out_down;
	}
	if (link_down) {
		sl_core_data_link_info_map_set(core_link, SL_CORE_INFO_MAP_PCS_LINK_DOWN);
		sl_core_data_link_last_down_cause_set(core_link, SL_LINK_DOWN_CAUSE_DOWN);
		goto out_down;
	}

out_down:

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_LINK_UP);
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"fault intr work link up disable failed [%d]", rtn);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_GOING_DOWN);
	sl_core_data_link_info_map_clr(core_link, SL_CORE_INFO_MAP_LINK_UP);

	if (core_link->config.fault_intr_hdlr)
		core_link->config.fault_intr_hdlr(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num);

	rtn = sl_core_hw_fec_data_get(core_link, &cw_cntrs, &lane_cntrs, &tail_cntrs);
	if (rtn)
		sl_core_log_warn(core_link, LOG_NAME, "hw_fec_data_get failed [%d]", rtn);
	else
		sl_ctl_link_fec_down_cache_store(sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
			core_link->core_lgrp->num, core_link->num), &cw_cntrs, &lane_cntrs, &tail_cntrs);

	sl_core_hw_link_off(core_link);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);

	rtn = core_link->config.fault_callback(core_link->link.tag,
		SL_CORE_LINK_STATE_DOWN, core_link->link.last_down_cause,
		sl_core_data_link_info_map_get(core_link));
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"fault intr work callback failed [%d]", rtn);
}
