// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/types.h>

#include "sl_media_jack.h"
#include "sl_ctrl_media_counters.h"
#include "base/sl_ctrl_log.h"

#define LOG_NAME SL_CTRL_MEDIA_LOG_NAME

#define SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(_media, _counter) \
	(_media)->cause_counters[_counter].name = #_counter

int sl_ctrl_media_cause_counters_init(struct sl_media_jack *media_jack)
{
	sl_ctrl_log_dbg(media_jack, LOG_NAME, "media cause counters init");

	media_jack->cause_counters = kzalloc(sizeof(*media_jack->cause_counters) *
					     SL_CTRL_MEDIA_CAUSE_COUNTERS_COUNT, GFP_KERNEL);
	if (!media_jack->cause_counters)
		return -ENOMEM;

	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_EEPROM_FORMAT_INVALID);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_EEPROM_FORMAT_INVALID);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_EEPROM_VENDOR_INVALID);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_EEPROM_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_ONLINE_STATUS_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_ONLINE_TIMEDOUT);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_ONLINE_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_ONLINE_JACK_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SERDES_SETTINGS_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SCAN_STATUS_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SCAN_HDL_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SCAN_JACK_GET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_MEDIA_ATTR_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_INTR_EVENT_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_POWER_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_SHIFT_STATE_JACK_IO);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_OFFLINE);
	SL_CTRL_MEDIA_CAUSE_COUNTER_INIT(media_jack, MEDIA_CAUSE_HIGH_TEMP);

	return 0;
}

int sl_ctrl_media_cause_counter_get(struct sl_media_jack *media_jack, u32 counter, int *count)
{
	*count = atomic_read(&media_jack->cause_counters[counter].count);

	sl_ctrl_log_dbg(media_jack, LOG_NAME, "get (cause counter = %u %s, count = %d)", counter,
			media_jack->cause_counters[counter].name, *count);

	return 0;
}

void sl_ctrl_media_cause_counters_del(struct sl_media_jack *media_jack)
{
	kfree(media_jack->cause_counters);
}

void sl_ctrl_media_cause_counter_inc(struct sl_media_jack *media_jack, unsigned long cause_map)
{
	unsigned long which;

	sl_ctrl_log_dbg(media_jack, LOG_NAME, "media cause counter inc");

	BUILD_BUG_ON(SL_CTRL_MEDIA_CAUSE_COUNTERS_COUNT > 32);

	for_each_set_bit(which, &cause_map, sizeof(cause_map) * BITS_PER_BYTE) {
		switch (BIT(which)) {
		case SL_MEDIA_FAULT_CAUSE_EEPROM_FORMAT_INVALID:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_EEPROM_FORMAT_INVALID);
			break;
		case SL_MEDIA_FAULT_CAUSE_EEPROM_VENDOR_INVALID:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_EEPROM_VENDOR_INVALID);
			break;
		case SL_MEDIA_FAULT_CAUSE_EEPROM_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_EEPROM_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_ONLINE_STATUS_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_ONLINE_STATUS_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_ONLINE_TIMEDOUT:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_ONLINE_TIMEDOUT);
			break;
		case SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_ONLINE_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_ONLINE_JACK_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_ONLINE_JACK_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SERDES_SETTINGS_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SERDES_SETTINGS_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SCAN_STATUS_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SCAN_STATUS_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SCAN_HDL_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SCAN_HDL_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SCAN_JACK_GET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SCAN_JACK_GET);
			break;
		case SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_MEDIA_ATTR_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_INTR_EVENT_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_INTR_EVENT_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_POWER_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_POWER_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET);
			break;
		case SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_SHIFT_STATE_JACK_IO);
			break;
		case SL_MEDIA_FAULT_CAUSE_OFFLINE:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_OFFLINE);
			break;
		case SL_MEDIA_FAULT_CAUSE_HIGH_TEMP:
			SL_CTRL_MEDIA_CAUSE_COUNTER_INC(media_jack, MEDIA_CAUSE_HIGH_TEMP);
			break;
		default:
			sl_ctrl_log_warn_trace(media_jack, LOG_NAME,
					       "cause_counter_inc unknown fault cause (bit = %lu)", which);
			break;
		}
	}
}
