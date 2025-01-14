// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include "sl_asic.h"
#include "sl_core_link.h"
#include "base/sl_core_work_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_serdes.h"
#include "hw/sl_core_hw_an.h"
#include "hw/sl_core_hw_an_lp.h"

#define LOG_NAME SL_CORE_HW_AN_LOG_NAME

static void sl_core_an_lp_caps_get_callback(struct sl_core_link *link)
{
	int                  rtn;
	struct sl_link_caps *lp_caps_cache_entry;

	sl_core_log_dbg(link, LOG_NAME, "lp caps get callback");

	if (link->an.lp_caps_state == SL_CORE_LINK_LP_CAPS_DATA) {
		lp_caps_cache_entry = kmem_cache_alloc(link->an.lp_caps_cache, GFP_ATOMIC);
		if (lp_caps_cache_entry == NULL)
			sl_core_log_warn(link, LOG_NAME,
				"lp caps get callback - lp caps cache alloc failed");
		else
			*lp_caps_cache_entry = link->an.lp_caps;
	}

	rtn = link->an.callbacks.lp_caps_get(link->an.tag, lp_caps_cache_entry, link->an.lp_caps_state);
	if (rtn != 0)
		sl_core_log_warn(link, LOG_NAME,
			"lp caps get callback - failed [%d]", rtn);
}

void sl_core_hw_an_lp_caps_get_cmd(struct sl_core_link *link, u32 link_state,
	sl_core_link_an_callback_t callback, void *tag, struct sl_link_caps *caps,
	u32 timeout_ms, u32 flags)
{
	int rtn;

	sl_core_log_dbg(link, LOG_NAME, "lp caps get cmd");

	sl_core_hw_an_stop(link);

	link->an.link_state             = link_state;
	link->an.tag                    = tag;
	link->an.lp_caps_get_timeout_ms = timeout_ms;
	link->an.callbacks.lp_caps_get  = callback;
	link->an.lp_caps_state          = SL_CORE_LINK_LP_CAPS_RUNNING;
	link->an.done_work_num          = SL_CORE_WORK_LINK_AN_LP_CAPS_GET_DONE;
	link->an.my_caps                = *caps;

	/* test check */
	if (link->an.use_test_caps) {
		sl_core_log_warn(link, LOG_NAME,
			"lp caps get cmd - using test caps");
		link->an.lp_caps       = link->an.test_caps;
		link->an.lp_caps_state = SL_CORE_LINK_LP_CAPS_DATA;
		sl_core_data_link_state_set(link, link->an.link_state);
		sl_core_an_lp_caps_get_callback(link);
		return;
	}

	memset(&(link->an.lp_caps), 0, sizeof(link->an.lp_caps));
	link->an.lp_caps.magic = SL_CORE_LINK_AN_MAGIC;

	if (!SL_PLATFORM_IS_HARDWARE(link->core_lgrp->core_ldev)) {
		sl_core_data_link_state_set(link, link->an.link_state);
		sl_core_an_lp_caps_get_callback(link);
		return;
	}

	sl_core_timer_link_begin(link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET);

	rtn = sl_core_hw_intr_flgs_disable(link, SL_CORE_HW_INTR_AN_PAGE_RECV);
	if (rtn != 0)
		sl_core_log_warn_trace(link, LOG_NAME,
			"lp caps get cmd - an page recv disable failed [%d]", rtn);

	sl_core_work_link_queue(link, SL_CORE_WORK_LINK_AN_LP_CAPS_GET);
}

void sl_core_hw_an_lp_caps_get_work(struct work_struct *work)
{
	int                  rtn;
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET]);

	sl_core_log_dbg(core_link, LOG_NAME, "up lp caps get work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "up lp caps get work canceled");
		return;
	}

	sl_core_hw_serdes_link_down(core_link);

	rtn = sl_core_hw_serdes_link_up_an(core_link);
	if (rtn != 0) {
		sl_core_log_err_trace(core_link, LOG_NAME,
			"lp caps get work hw_serdes_link_up_an failed [%d]", rtn);
		rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET);
		if (rtn < 0)
			sl_core_log_warn_trace(core_link, LOG_NAME,
				"lp caps get work lp caps get end failed [%d]", rtn);
		sl_core_hw_serdes_link_down(core_link);
		sl_core_data_link_state_set(core_link, core_link->an.link_state);
		core_link->an.lp_caps_state = SL_CORE_LINK_LP_CAPS_ERROR;
		sl_core_an_lp_caps_get_callback(core_link);
		return;
	}

	sl_core_hw_an_init(core_link);

	sl_core_hw_an_config(core_link);

	sl_core_hw_an_tx_pages_encode(core_link, &(core_link->an.my_caps));

	rtn = sl_core_hw_an_base_page_send(core_link);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"lp caps get work an base page send failed [%d]", rtn);
}
void sl_core_hw_an_lp_caps_stop_cmd(struct sl_core_link *link)
{
	int rtn;

	sl_core_log_dbg(link, LOG_NAME, "lp caps stop cmd");

	rtn = sl_core_hw_intr_flgs_disable(link, SL_CORE_HW_INTR_AN_PAGE_RECV);
	if (rtn != 0)
		sl_core_log_warn_trace(link, LOG_NAME,
			"lp caps stop cmd - an page recv disable failed [%d]", rtn);

	cancel_work_sync(&link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET]);
	cancel_work_sync(&link->work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT]);

	sl_core_timer_link_end(link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET);

	sl_core_data_link_state_set(link, link->an.link_state);
}

void sl_core_hw_an_lp_caps_get_timeout_work(struct work_struct *work)
{
	int                  rtn;
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_TIMEOUT]);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps get timeout work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps get timeout work canceled");
		return;
	}

	rtn = sl_core_hw_intr_flgs_disable(core_link, SL_CORE_HW_INTR_AN_PAGE_RECV);
	if (rtn != 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"lp caps get timeout work an page recv disable failed [%d]", rtn);

	sl_core_hw_an_stop(core_link);
	sl_core_hw_serdes_link_down(core_link);

	sl_core_data_link_state_set(core_link, core_link->an.link_state);
	core_link->an.lp_caps_state = SL_CORE_LINK_LP_CAPS_TIMEOUT;

	sl_core_an_lp_caps_get_callback(core_link);
}

void sl_core_hw_an_lp_caps_get_done_work(struct work_struct *work)
{
	int                  rtn;
	int                  x;
	struct sl_core_link *core_link;

	core_link = container_of(work, struct sl_core_link, work[SL_CORE_WORK_LINK_AN_LP_CAPS_GET_DONE]);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps get done work");

	if (sl_core_link_is_canceled_or_timed_out(core_link)) {
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps get done work canceled");
		return;
	}

	for (x = 0; x < core_link->an.rx_count; ++x)
		sl_core_log_dbg(core_link, LOG_NAME,
			"lp caps get done work rx pages (%d = 0x%016llX)",
			x, core_link->an.rx_pages[x]);

	rtn = sl_core_timer_link_end(core_link, SL_CORE_TIMER_LINK_AN_LP_CAPS_GET);
	if (rtn < 0)
		sl_core_log_warn_trace(core_link, LOG_NAME,
			"lp caps get done work lp caps get end failed [%d]", rtn);

	sl_core_hw_an_stop(core_link);
	sl_core_hw_serdes_link_down(core_link);

	if (core_link->an.state == SL_CORE_HW_AN_STATE_COMPLETE)
		sl_core_hw_an_rx_pages_decode(core_link, &(core_link->an.lp_caps));

	sl_core_data_link_state_set(core_link, core_link->an.link_state);
	core_link->an.lp_caps_state = SL_CORE_LINK_LP_CAPS_DATA;

	sl_core_an_lp_caps_get_callback(core_link);
}
