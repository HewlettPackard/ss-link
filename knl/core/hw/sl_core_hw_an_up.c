// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_media_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_serdes_link.h"
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_up.h"
#include "hw/sl_core_hw_link.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

#define SL_CORE_HW_AN_RESTART_DELAY_MS     200
#define SL_CORE_HW_AN_RESTART_DELAY_MS_MAX (SL_CORE_HW_AN_RESTART_DELAY_MS * 10)

static void sl_core_an_up_fail_callback(struct sl_core_link *core_link)
{
	int                   rtn;
	u64                   up_fail_cause_map;
	struct sl_media_lgrp *media_lgrp;

	sl_core_log_dbg(core_link, LOG_NAME, "up fail callback");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up callback canceled");
		return;
	}

	up_fail_cause_map = sl_core_data_link_last_up_fail_cause_map_get(core_link);

	if (!is_flag_set(sl_core_data_lgrp_config_flags_get(core_link->core_lgrp),
		SL_LGRP_CONFIG_OPT_SERDES_LOOPBACK_ENABLE)) {
		media_lgrp = sl_media_lgrp_get(core_link->core_lgrp->core_ldev->num, core_link->core_lgrp->num);
		if (!sl_media_jack_is_cable_online(media_lgrp->media_jack)) {
			up_fail_cause_map &= ~SL_LINK_DOWN_RETRYABLE;
			up_fail_cause_map |= SL_LINK_DOWN_CAUSE_NO_MEDIA;
		}

		sl_core_data_link_last_up_fail_cause_map_set(core_link, up_fail_cause_map);
	}

	rtn = core_link->link.callbacks.up(core_link->link.tags.up, sl_core_data_link_state_get(core_link),
		up_fail_cause_map, sl_core_data_link_info_map_get(core_link), sl_core_data_link_speed_get(core_link),
		sl_core_data_link_fec_mode_get(core_link), sl_core_data_link_fec_type_get(core_link));
	if (rtn != 0)
		sl_core_log_warn(core_link, LOG_NAME, "up callback failed [%d]", rtn);
}

static void sl_core_hw_an_up_start_test_caps(struct sl_core_link *core_link)
{
	int rtn;
	int bit;

	sl_core_log_dbg(core_link, LOG_NAME, "up start test caps");

	core_link->core_lgrp->link_caps[core_link->num].pause_map =
		(core_link->config.pause_map & core_link->an.test_caps.pause_map);
	core_link->core_lgrp->link_caps[core_link->num].tech_map  =
		(core_link->core_lgrp->config.tech_map & core_link->an.test_caps.tech_map);
	sl_core_log_dbg(core_link, LOG_NAME,
		"up start test caps input (tech map = 0x%08X, 0x%08X, 0x%08X)",
		core_link->core_lgrp->config.tech_map,
		core_link->an.test_caps.tech_map,
		core_link->core_lgrp->link_caps[core_link->num].tech_map);
	if (core_link->core_lgrp->link_caps[core_link->num].tech_map) {
		bit = 0;
		while (core_link->core_lgrp->link_caps[core_link->num].tech_map >>= 1)
			bit++;
		core_link->core_lgrp->link_caps[core_link->num].tech_map = BIT(bit);
	}
	sl_core_log_dbg(core_link, LOG_NAME,
		"up start test caps result (tech map = 0x%08X)",
			core_link->core_lgrp->link_caps[core_link->num].tech_map);
	core_link->core_lgrp->link_caps[core_link->num].fec_map =
		(core_link->core_lgrp->config.fec_map & core_link->an.test_caps.fec_map);
	core_link->core_lgrp->link_caps[core_link->num].hpe_map =
		(core_link->config.hpe_map & core_link->an.test_caps.hpe_map);

	if (core_link->core_lgrp->link_caps[core_link->num].tech_map == 0) {
		sl_core_log_err_trace(core_link, LOG_NAME, "up start test caps no match");
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn_trace(core_link, LOG_NAME,
				"up start test caps link up end failed [%d]", rtn);
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH_MAP);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_an_up_fail_callback(core_link);
		return;
	}

	sl_core_hw_link_up_after_an_start(core_link);
}

void sl_core_hw_an_up_start(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "up start (link = 0x%p)", core_link);

	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_AN);

	sl_core_hw_an_stop(core_link);

	/* init caps */
	memset(&(core_link->core_lgrp->link_caps[core_link->num]), 0, sizeof(struct sl_link_caps));

	core_link->an.done_work_num = SL_CORE_WORK_LINK_AN_UP_DONE;

	sl_core_data_link_timeouts(core_link);

	sl_core_timer_link_begin(core_link, SL_CORE_TIMER_LINK_UP);

	if (core_link->an.use_test_caps) {
		sl_core_hw_an_up_start_test_caps(core_link);
		return;
	}

	sl_core_work_link_queue(core_link, SL_CORE_WORK_LINK_AN_UP);
}

static void sl_core_hw_an_up(struct sl_core_link *core_link)
{
	int                 rtn;
	struct sl_link_caps my_caps;

	sl_core_log_dbg(core_link, LOG_NAME, "up");

	sl_core_hw_an_init(core_link);

	sl_core_hw_an_config(core_link);

	my_caps.tech_map  = core_link->core_lgrp->config.tech_map;
	my_caps.fec_map   = core_link->core_lgrp->config.fec_map;
	my_caps.pause_map = core_link->config.pause_map;
	my_caps.hpe_map   = core_link->config.hpe_map;
	sl_core_hw_an_tx_pages_encode(core_link, &(my_caps));

	rtn = sl_core_hw_an_base_page_send(core_link);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up an base page send failed [%d]", rtn);
}

void sl_core_hw_an_up_work(struct work_struct *work)
{
	int                  rtn;
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_UP]);

	sl_core_log_dbg(core_link, LOG_NAME, "up work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up work canceled");
		return;
	}

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"up work an page recv disable failed [%d]", rtn);

	sl_core_hw_serdes_link_down(core_link);

	rtn = sl_core_hw_serdes_link_up_an(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up work hw_serdes_link_up_an failed [%d]", rtn);
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
		if (rtn < 0)
			sl_core_log_warn_trace(core_link, LOG_NAME,
				"up work link up end failed [%d]", rtn);
		sl_core_hw_serdes_link_down(core_link);
		sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
		sl_core_an_up_fail_callback(core_link);
		return;
	}

	core_link->an.restart_sleep_ms = SL_CORE_HW_AN_RESTART_DELAY_MS;

	sl_core_hw_an_up(core_link);
}

static void sl_core_an_up_restart(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "up restart");

	sl_core_hw_an_up(core_link);
}

void sl_core_hw_an_up_done_work(struct work_struct *work)
{
	int                  rtn;
	int                  x;
	struct sl_core_link *core_link;
	struct sl_link_caps  my_caps;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_UP_DONE]);

	sl_core_log_dbg(core_link, LOG_NAME, "up done work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up done work canceled");
		return;
	}

	sl_core_hw_an_stop(core_link);

	for (x = 0; x < core_link->an.rx_count; ++x)
		sl_core_log_dbg(core_link, LOG_NAME,
			"up done work rx pages (%d = 0x%016llX)",
			x, core_link->an.rx_pages[x]);

	if (core_link->an.state != SL_CORE_HW_AN_STATE_COMPLETE) {
		/* try again if asked to */
		if (is_flag_set(core_link->config.flags, SL_LINK_CONFIG_OPT_AUTONEG_CONTINUOUS_ENABLE)) {
			sl_core_log_dbg(core_link, LOG_NAME,
				"up done work restart (delay = %dms)", core_link->an.restart_sleep_ms);
			msleep(core_link->an.restart_sleep_ms);
			if ((core_link->an.restart_sleep_ms) &&
				(core_link->an.restart_sleep_ms < SL_CORE_HW_AN_RESTART_DELAY_MS_MAX))
				core_link->an.restart_sleep_ms += SL_CORE_HW_AN_RESTART_DELAY_MS;
			sl_core_an_up_restart(core_link);
			return;
		}
		sl_core_log_err_trace(core_link, LOG_NAME, "up done work auto neg not complete");
		goto out_down;
	}

	my_caps.tech_map  = core_link->core_lgrp->config.tech_map;
	my_caps.fec_map   = core_link->core_lgrp->config.fec_map;
	my_caps.pause_map = core_link->config.pause_map;
	my_caps.hpe_map   = core_link->config.hpe_map;
	rtn = sl_core_hw_an_rx_pages_decode(core_link, &my_caps,
		&(core_link->core_lgrp->link_caps[core_link->num]));
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"up done work hw_an_rx_pages_decode failure [%d]", rtn);
		goto out_down;
	}

	sl_core_hw_link_up_after_an_start(core_link);

	return;

out_down:

	rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_UP);
	if (rtn < 0)
		sl_core_log_warn(core_link, LOG_NAME,
			"up done work link up timer end failed [%d]", rtn);
	sl_core_hw_serdes_link_down(core_link);
	sl_core_data_link_last_down_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_AUTONEG_MAP);
	sl_core_data_link_state_set(core_link, SL_CORE_LINK_STATE_DOWN);
	sl_core_an_up_fail_callback(core_link);
}
