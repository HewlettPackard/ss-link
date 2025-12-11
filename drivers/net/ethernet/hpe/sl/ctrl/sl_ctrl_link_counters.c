// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>

#include "sl_ctrl_link.h"
#include "sl_ctrl_link_counters.h"
#include "base/sl_ctrl_log.h"
#include "hw/sl_core_hw_an.h"

#define LOG_NAME SL_CTRL_LINK_LOG_NAME

#define SL_CTRL_LINK_COUNTER_INIT(_link, _counter) \
	(_link)->counters[_counter].name = #_counter

#define SL_CTRL_LINK_CAUSE_COUNTER_INIT(_link, _counter) \
	(_link)->cause_counters[_counter].name = #_counter

#define SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(_link, _counter) \
	(_link)->an_cause_counters[_counter].name = #_counter

int sl_ctrl_link_counters_init(struct sl_ctrl_link *ctrl_link)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link counters init");

	ctrl_link->counters = kzalloc(sizeof(struct sl_ctrl_link_counter) * SL_CTRL_LINK_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_link->counters)
		return -ENOMEM;

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_RETRY);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CANCEL_CMD);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_CANCELED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_RESET_CMD);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_FAULT);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_RECOVERING);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_CCW_WARN_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UCW_WARN_CROSSED);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_UCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_CCW_CAUSE);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_DOWN_UCW_CAUSE);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL_CCW_LIMIT_CROSSED);
	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_UP_FAIL_UCW_LIMIT_CROSSED);

	SL_CTRL_LINK_COUNTER_INIT(ctrl_link, LINK_HW_AN_ATTEMPT);

	return 0;
}

int sl_ctrl_link_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count)
{
	*count = atomic_read(&ctrl_link->counters[counter].count);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (counter = %u %s, count = %d)", counter,
			ctrl_link->counters[counter].name, *count);

	return 0;
}

void sl_ctrl_link_counters_del(struct sl_ctrl_link *ctrl_link)
{
	kfree(ctrl_link->counters);
}

int sl_ctrl_link_cause_counters_init(struct sl_ctrl_link *ctrl_link)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link cause counters init");

	ctrl_link->cause_counters = kzalloc(sizeof(*ctrl_link->cause_counters) *
					    SL_CTRL_LINK_CAUSE_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_link->cause_counters)
		return -ENOMEM;

	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UCW);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_LF);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_RF);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_DOWN);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UP_TRIES);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_AUTONEG_NOMATCH);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_AUTONEG);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_CONFIG);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_INTR_ENABLE);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_TIMEOUT);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_CANCELED);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UNSUPPORTED_CABLE);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_COMMAND);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_DOWNSHIFT);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_LLR_REPLAY_MAX);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UPSHIFT);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_AUTONEG_CONFIG);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_PCS_FAULT);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_SERDES_PLL);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_SERDES_CONFIG);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_SERDES_SIGNAL);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_SERDES_QUALITY);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_NO_MEDIA);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_CCW);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_HIGH_TEMP);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_INTR_REGISTER);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_MEDIA_ERROR);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UP_CANCELED);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_UNSUPPORTED_SPEED);
	SL_CTRL_LINK_CAUSE_COUNTER_INIT(ctrl_link, LINK_CAUSE_SS200_CABLE);

	return 0;
}

int sl_ctrl_link_cause_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count)
{
	*count = atomic_read(&ctrl_link->cause_counters[counter].count);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (cause counter = %u %s, count = %d)", counter,
			ctrl_link->cause_counters[counter].name, *count);

	return 0;
}

void sl_ctrl_link_cause_counters_del(struct sl_ctrl_link *ctrl_link)
{
	kfree(ctrl_link->cause_counters);
}

int sl_ctrl_link_an_cause_counters_init(struct sl_ctrl_link *ctrl_link)
{
	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link an cause counters init");

	ctrl_link->an_cause_counters = kzalloc(sizeof(*ctrl_link->an_cause_counters) *
					    SL_CTRL_LINK_AN_CAUSE_COUNTERS_COUNT, GFP_KERNEL);
	if (!ctrl_link->an_cause_counters)
		return -ENOMEM;

	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_LP_CAPS_NOT_COMPLETE);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_NOT_COMPLETE);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_TEST_CAPS_NOMATCH);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_SERDES_LINK_UP_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_BAD);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_BP_STORE_LP_ABILITY_NOT_SET);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_ERROR);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_BP_STORE_BP_NOT_SET);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_BP_SEND_INTR_ENABLE_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_NP_STORE_STATE_BAD);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_NP_STORE_BP_SET);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_NP_CHECK_STATE_BAD);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_INTR_STATE_INVALID);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_INTR_OUT_OF_PAGES);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_INTR_NP_SEND_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_FAIL);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_NO_BP);
	SL_CTRL_LINK_AN_CAUSE_COUNTER_INIT(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_OUI_INVALID);

	return 0;
}

int sl_ctrl_link_an_cause_counters_get(struct sl_ctrl_link *ctrl_link, u32 counter, int *count)
{
	*count = atomic_read(&ctrl_link->an_cause_counters[counter].count);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "get (an cause counter = %u %s, count = %d)", counter,
			ctrl_link->an_cause_counters[counter].name, *count);

	return 0;
}

void sl_ctrl_link_an_cause_counters_del(struct sl_ctrl_link *ctrl_link)
{
	kfree(ctrl_link->an_cause_counters);
}

void sl_ctrl_link_cause_counter_inc(struct sl_ctrl_link *ctrl_link, u64 cause_map)
{
	unsigned long which;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link cause counter inc");

	for_each_set_bit(which, (unsigned long *)&cause_map, sizeof(cause_map) * BITS_PER_BYTE) {
		switch (BIT(which)) {
		case SL_LINK_DOWN_CAUSE_UCW:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UCW);
			break;
		case SL_LINK_DOWN_CAUSE_LF:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_LF);
			break;
		case SL_LINK_DOWN_CAUSE_RF:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_RF);
			break;
		case SL_LINK_DOWN_CAUSE_DOWN:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_DOWN);
			break;
		case SL_LINK_DOWN_CAUSE_UP_TRIES:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UP_TRIES);
			break;
		case SL_LINK_DOWN_CAUSE_AUTONEG_NOMATCH:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_AUTONEG_NOMATCH);
			break;
		case SL_LINK_DOWN_CAUSE_AUTONEG:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_AUTONEG);
			break;
		case SL_LINK_DOWN_CAUSE_CONFIG:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_CONFIG);
			break;
		case SL_LINK_DOWN_CAUSE_INTR_ENABLE:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_INTR_ENABLE);
			break;
		case SL_LINK_DOWN_CAUSE_TIMEOUT:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_TIMEOUT);
			break;
		case SL_LINK_DOWN_CAUSE_CANCELED:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_CANCELED);
			break;
		case SL_LINK_DOWN_CAUSE_UNSUPPORTED_CABLE:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UNSUPPORTED_CABLE);
			break;
		case SL_LINK_DOWN_CAUSE_COMMAND:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_COMMAND);
			break;
		case SL_LINK_DOWN_CAUSE_DOWNSHIFT:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_DOWNSHIFT);
			break;
		case SL_LINK_DOWN_CAUSE_LLR_REPLAY_MAX:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_LLR_REPLAY_MAX);
			break;
		case SL_LINK_DOWN_CAUSE_UPSHIFT:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UPSHIFT);
			break;
		case SL_LINK_DOWN_CAUSE_AUTONEG_CONFIG:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_AUTONEG_CONFIG);
			break;
		case SL_LINK_DOWN_CAUSE_PCS_FAULT:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_PCS_FAULT);
			break;
		case SL_LINK_DOWN_CAUSE_SERDES_PLL:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_SERDES_PLL);
			break;
		case SL_LINK_DOWN_CAUSE_SERDES_CONFIG:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_SERDES_CONFIG);
			break;
		case SL_LINK_DOWN_CAUSE_SERDES_SIGNAL:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_SERDES_SIGNAL);
			break;
		case SL_LINK_DOWN_CAUSE_SERDES_QUALITY:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_SERDES_QUALITY);
			break;
		case SL_LINK_DOWN_CAUSE_NO_MEDIA:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_NO_MEDIA);
			break;
		case SL_LINK_DOWN_CAUSE_CCW:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_CCW);
			break;
		case SL_LINK_DOWN_CAUSE_HIGH_TEMP:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_HIGH_TEMP);
			break;
		case SL_LINK_DOWN_CAUSE_INTR_REGISTER:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_INTR_REGISTER);
			break;
		case SL_LINK_DOWN_CAUSE_MEDIA_ERROR:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_MEDIA_ERROR);
			break;
		case SL_LINK_DOWN_CAUSE_UP_CANCELED:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UP_CANCELED);
			break;
		case SL_LINK_DOWN_CAUSE_UNSUPPORTED_SPEED:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_UNSUPPORTED_SPEED);
			break;
		case SL_LINK_DOWN_CAUSE_SS200_CABLE:
			SL_CTRL_LINK_CAUSE_COUNTER_INC(ctrl_link, LINK_CAUSE_SS200_CABLE);
			break;
		default:
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					       "cause_counter_inc unknown cause (bit = %lu)", which);
			break;
		}
	}
}

void sl_ctrl_link_an_cause_counter_inc(struct sl_ctrl_link *ctrl_link, u32 cause_map)
{
	unsigned long which;

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "link an cause counter inc");

	for_each_set_bit(which, (unsigned long *)&cause_map, sizeof(cause_map) * BITS_PER_BYTE) {
		switch (BIT(which)) {
		case SL_CORE_HW_AN_FAIL_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_LP_CAPS_SERDES_LINK_UP_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_LP_CAPS_NOT_COMPLETE:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_LP_CAPS_NOT_COMPLETE);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_NOT_COMPLETE:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_NOT_COMPLETE);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_TEST_CAPS_NOMATCH:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_TEST_CAPS_NOMATCH);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_SERDES_LINK_UP_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_SERDES_LINK_UP_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_STATE_BAD:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_BAD);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_LP_ABILITY_NOT_SET:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_BP_STORE_LP_ABILITY_NOT_SET);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_STATE_ERROR:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_BP_STORE_STATE_ERROR);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_BP_STORE_BP_NOT_SET:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_BP_STORE_BP_NOT_SET);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_BP_SEND_INTR_ENABLE_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_BP_SEND_INTR_ENABLE_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_STATE_BAD:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_NP_STORE_STATE_BAD);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_NP_STORE_BP_SET:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_NP_STORE_BP_SET);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_NP_CHECK_STATE_BAD:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_NP_CHECK_STATE_BAD);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_INTR_STATE_INVALID:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_INTR_STATE_INVALID);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_INTR_AN_RETRY_NP_SEND_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_INTR_OUT_OF_PAGES:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_INTR_OUT_OF_PAGES);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_INTR_NP_SEND_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_INTR_NP_SEND_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_FAIL:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_FAIL);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_NO_BP:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_NO_BP);
			break;
		case SL_CORE_HW_AN_FAIL_CAUSE_PAGES_DECODE_OUI_INVALID:
			SL_CTRL_LINK_AN_CAUSE_COUNTER_INC(ctrl_link, LINK_AN_CAUSE_PAGES_DECODE_OUI_INVALID);
			break;
		default:
			sl_ctrl_log_warn_trace(ctrl_link, LOG_NAME,
					       "an cause_counter_inc unknown fault cause (bit = %lu)", which);
			break;
		}
	}
}
