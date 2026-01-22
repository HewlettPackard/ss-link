/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_MEDIA_COUNTERS_H_
#define _SL_CTRL_MEDIA_COUNTERS_H_

#include <linux/types.h>
#include <linux/atomic.h>

struct sl_media_jack;

enum sl_ctrl_media_cause_counters {
	MEDIA_CAUSE_EEPROM_FORMAT_INVALID,             /* media eeprom format is invalid                */
	MEDIA_CAUSE_EEPROM_VENDOR_INVALID,             /* media eeprom vendor is invalid                */
	MEDIA_CAUSE_EEPROM_JACK_IO,                    /* media eeprom jack io error                    */
	MEDIA_CAUSE_ONLINE_STATUS_GET,                 /* media online status get error                 */
	MEDIA_CAUSE_ONLINE_TIMEDOUT,                   /* media online timedout                         */
	MEDIA_CAUSE_ONLINE_JACK_IO,                    /* media online jack io error                    */
	MEDIA_CAUSE_ONLINE_JACK_GET,                   /* media online jack get error                   */
	MEDIA_CAUSE_SERDES_SETTINGS_GET,               /* media serdes settings get error               */
	MEDIA_CAUSE_SCAN_STATUS_GET,                   /* media scan status get error                   */
	MEDIA_CAUSE_SCAN_HDL_GET,                      /* media scan hdl get error                      */
	MEDIA_CAUSE_SCAN_JACK_GET,                     /* media scan jack get error                     */
	MEDIA_CAUSE_MEDIA_ATTR_SET,                    /* media attribute set error                     */
	MEDIA_CAUSE_INTR_EVENT_JACK_IO,                /* media interrupt event jack io error           */
	MEDIA_CAUSE_POWER_SET,                         /* media power set error                         */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO,                /* media shift down jack io error                */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET,  /* media shift down jack io low power set error  */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET, /* media shift down jack io high power set error */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO,                  /* media shift up jack io error                  */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET,    /* media shift up jack io low power set error    */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET,   /* media shift up jack io high power ser error   */
	MEDIA_CAUSE_SHIFT_STATE_JACK_IO,               /* media shift state jack io error               */
	MEDIA_CAUSE_OFFLINE,                           /* media is offline                              */
	MEDIA_CAUSE_HIGH_TEMP,                         /* media high temperature detected               */
	SL_CTRL_MEDIA_CAUSE_COUNTERS_COUNT
};

struct sl_ctrl_media_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_MEDIA_CAUSE_COUNTER_INC(_media, _counter) \
	atomic_inc(&(_media)->cause_counters[_counter].count)

int  sl_ctrl_media_cause_counters_init(struct sl_media_jack *media_jack);
void sl_ctrl_media_cause_counters_del(struct sl_media_jack *media_jack);
int  sl_ctrl_media_cause_counter_get(struct sl_media_jack *media_jack, u32 counter, int *count);
void sl_ctrl_media_cause_counter_inc(struct sl_media_jack *media_jack, unsigned long cause_map);

#endif /* _SL_CTRL_MEDIA_COUNTERS_H_ */
