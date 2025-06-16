// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#include "sl_kconfig.h"
#include "sl_media_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_lgrp.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_serdes_link.h"
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_up.h"
#include "hw/sl_core_hw_link.h"
#include "sl_ctl_link_counters.h"
#include "sl_ctl_link.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

#define SL_CORE_HW_AN_RESTART_DELAY_MS     200
#define SL_CORE_HW_AN_RESTART_DELAY_MS_MAX (SL_CORE_HW_AN_RESTART_DELAY_MS * 10)

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
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_TEST_CAPS_NOMATCH);
		sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH_MAP);
		rtn = sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link down internal failed [%d]", rtn);

		return;
	}

	sl_core_hw_link_up_after_an_start(core_link);
}

void sl_core_hw_an_up_start_work(struct work_struct *work)
{
	struct sl_core_link *core_link;
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_UP_START]);

	sl_core_log_dbg(core_link, LOG_NAME, "up start work (link = 0x%p)", core_link);

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_AN) {
		sl_core_log_dbg(core_link, LOG_NAME, "up start work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
		return;
	}

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

	queue_work(core_link->core_lgrp->core_ldev->workqueue, &(core_link->work[SL_CORE_WORK_LINK_AN_UP]));
}

static void sl_core_hw_an_up(struct sl_core_link *core_link)
{
	int                 rtn;
	struct sl_link_caps my_caps;
	struct sl_ctl_link *ctl_link;

	sl_core_log_dbg(core_link, LOG_NAME, "up");

	ctl_link = sl_ctl_link_get(core_link->core_lgrp->core_ldev->num,
	core_link->core_lgrp->num, core_link->num);
	SL_CTL_LINK_COUNTER_INC(ctl_link, LINK_HW_AN_ATTEMPT);

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
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_UP]);

	sl_core_log_dbg(core_link, LOG_NAME, "up work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_AN) {
		sl_core_log_dbg(core_link, LOG_NAME, "up work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
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
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_SERDES_LINK_UP_FAIL);

		sl_core_link_up_fail(core_link);
		if (rtn)
			sl_core_log_err_trace(core_link, LOG_NAME, "link down internal failed [%d]", rtn);

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
	u32                  link_state;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_UP_DONE]);

	sl_core_log_dbg(core_link, LOG_NAME, "up done work");

	link_state = sl_core_data_link_state_get(core_link);
	if (link_state != SL_CORE_LINK_STATE_AN) {
		sl_core_log_dbg(core_link, LOG_NAME, "up done work - invalid state (%u %s)",
			link_state, sl_core_link_state_str(link_state));
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
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_NOT_COMPLETE);
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
		sl_core_data_link_an_fail_cause_set(core_link,
			SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_FAIL);
		goto out_down;
	}

	sl_core_hw_link_up_after_an_start(core_link);

	return;

out_down:

	sl_core_data_link_last_up_fail_cause_map_set(core_link, SL_LINK_DOWN_CAUSE_AUTONEG_MAP);
	sl_core_link_up_fail(core_link);
}
