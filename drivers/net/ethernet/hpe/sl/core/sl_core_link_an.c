// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "data/sl_core_data_link.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "data/sl_core_data_link.h"
#include "hw/sl_core_hw_an_lp.h"
#include "hw/sl_core_hw_an.h"

#define LOG_NAME SL_CORE_LINK_AN_LOG_NAME

int sl_core_link_an_lp_caps_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
	sl_core_link_an_callback_t callback, void *tag, struct sl_link_caps *caps,
	u32 timeout_ms, u32 flags)
{
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps get");

	spin_lock(&core_link->link.data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		core_link->link.state = SL_CORE_LINK_STATE_AN;
		spin_unlock(&core_link->link.data_lock);
		sl_core_hw_an_lp_caps_get_cmd(core_link, link_state, callback, tag, caps, timeout_ms, flags);
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"lp caps get - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->link.data_lock);
		return -EBADRQC;
	}
}

u32 sl_core_link_an_lp_caps_state_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_data_link_an_lp_caps_state_get(sl_core_link_get(ldev_num, lgrp_num, link_num));
}

int sl_core_link_an_lp_caps_stop(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int                  rtn;
	u32                  link_state;
	struct sl_core_link *core_link;

	core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);

	sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop");

	rtn = sl_core_data_link_state_get(core_link, &link_state);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME,
				      "lp caps stop - failed to get link state [%d]", rtn);
		return rtn;
	}

	switch (link_state) {
	case SL_CORE_LINK_STATE_UNCONFIGURED:
	case SL_CORE_LINK_STATE_CONFIGURED:
	case SL_CORE_LINK_STATE_DOWN:
		return 0;
	case SL_CORE_LINK_STATE_AN:
		sl_core_hw_an_lp_caps_stop_cmd(core_link);
		return 0;
	case SL_CORE_LINK_STATE_UP:
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop - link is up");
		return 0;
	case SL_CORE_LINK_STATE_GOING_UP:
	case SL_CORE_LINK_STATE_GOING_DOWN:
	case SL_CORE_LINK_STATE_TIMEOUT:
		sl_core_log_dbg(core_link, LOG_NAME, "lp caps stop - in transition");
		return 0;
	default:
		sl_core_log_err(core_link, LOG_NAME,
			"lp caps stop - invalid (link_state = %u %s)",
			link_state, sl_core_link_state_str(link_state));
		return -EBADRQC;
	}
}

void sl_core_link_an_fail_cause_get(u8 ldev_num, u8 lgrp_num, u8 link_num, u32 *fail_cause, time64_t *fail_time)
{
	sl_core_data_link_an_fail_cause_get(sl_core_link_get(ldev_num, lgrp_num, link_num), fail_cause, fail_time);
}

u32 sl_core_link_an_retry_count_get(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_data_link_an_retry_count_get(sl_core_link_get(ldev_num, lgrp_num, link_num));
}

const char *sl_core_link_an_fail_cause_str(u32 fail_cause)
{
	switch (fail_cause) {
	case SL_CORE_HW_AN_FAIL_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL:
		return "lp-caps-serdes-link-up-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_LP_CAPS_NOT_COMPLETE:
		return "lp-caps-not-complete";
	case SL_CORE_HW_AN_FAIL_CAUSE_NOT_COMPLETE:
		return "not-complete";
	case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_FAIL:
		return "pages-decode-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_TEST_CAPS_NOMATCH:
		return "test-caps-nomatch";
	case SL_CORE_HW_AN_FAIL_CAUSE_SERDES_LINK_UP_FAIL:
		return "serdes-link-up-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_STATE_BAD:
		return "bp-store-state-bad";
	case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_LP_ABILITY_NOT_SET:
		return "bp-store-lp-ability-not-set";
	case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_STATE_ERROR:
		return "bp-store-state-error";
	case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_BP_NOT_SET:
		return "bp-store-bp-not-set";
	case SL_CORE_HW_AN_FAIL_CAUSE_BP_SEND_INTR_ENABLE_FAIL:
		return "bp-send-intr-enable-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_STATE_BAD:
		return "np-store-state-bad";
	case SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_BP_SET:
		return "np-store-bp-set";
	case SL_CORE_HW_AN_FAIL_CAUSE_NP_CHECK_STATE_BAD:
		return "np-check-state-bad";
	case SL_CORE_HW_AN_FAIL_CAUSE_INTR_STATE_INVALID:
		return "intr-state-invalid";
	case SL_CORE_HW_AN_FAIL_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL:
		return "intr-an-retry-np-send-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_INTR_OUT_OF_PAGES:
		return "intr-out-of-pages";
	case SL_CORE_HW_AN_FAIL_CAUSE_INTR_NP_SEND_FAIL:
		return "intr-np-send-fail";
	case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_NO_BP:
		return "pages-decode-no-bp";
	case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_OUI_INVALID:
		return "pages-decode-oui-invalid";
	default:
		return "unknown";
	}
}

