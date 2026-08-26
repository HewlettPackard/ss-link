/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025-2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_MEDIA_COUNTERS_H_
#define _SL_CTRL_MEDIA_COUNTERS_H_

#include <linux/types.h>
#include <linux/atomic.h>

struct sl_media_jack;

enum sl_ctrl_media_temp_state_counters {
	MEDIA_TEMP_STATE_COLD,                         /* media temperature state cold                  */
	MEDIA_TEMP_STATE_WARM,                         /* media temperature state warm                  */
	MEDIA_TEMP_STATE_HOT,                          /* media temperature state hot                   */
	MEDIA_TEMP_STATE_UNKNOWN_IO,                   /* media temperature state unknown io error      */
	MEDIA_TEMP_STATE_UNKNOWN_SLOPE,                /* media temperature state unknown slope error   */
	SL_CTRL_MEDIA_TEMP_STATE_COUNTERS_COUNT
};

enum sl_ctrl_media_cause_counters {
	MEDIA_CAUSE_EEPROM_FORMAT_UNSUPPORTED,         /* media eeprom format is unsupported            */
	MEDIA_CAUSE_EEPROM_VENDOR_UNSUPPORTED,         /* media eeprom vendor is unsupported            */
	MEDIA_CAUSE_EEPROM_JACK_IO,                    /* media eeprom jack io error                    */
	MEDIA_CAUSE_JACK_GET,                          /* failure to retrieve jack info                 */
	MEDIA_CAUSE_JACK_STATUS_GET,                   /* failure to retrieve jack status               */
	MEDIA_CAUSE_CABLE_SETUP,                       /* failure during cable setup                    */
	MEDIA_CAUSE_ACTIVE_CABLE_SETUP,                /* failure during active cable setup             */
	MEDIA_CAUSE_SERDES_SETTINGS_GET,               /* media serdes settings get error               */
	MEDIA_CAUSE_MEDIA_ATTR_SET,                    /* media attribute set error                     */
	MEDIA_CAUSE_HIGH_POWER_SET_JACK_IO,            /* media high power set error                    */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO,                /* media shift down jack io error                */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET,  /* media shift down jack io low power set error  */
	MEDIA_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET, /* media shift down jack io high power set error */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO,                  /* media shift up jack io error                  */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET,    /* media shift up jack io low power set error    */
	MEDIA_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET,   /* media shift up jack io high power ser error   */
	MEDIA_CAUSE_SHIFT_STATE_JACK_IO,               /* media shift state jack io error               */
	MEDIA_CAUSE_HOT,                               /* media hot detected                            */
	MEDIA_CAUSE_WARM,                              /* media warm detected                           */
	SL_CTRL_MEDIA_CAUSE_COUNTERS_COUNT
};

struct sl_ctrl_media_counter {
	atomic_t  count;
	char     *name;
};

#define SL_CTRL_MEDIA_CAUSE_COUNTER_INC(_media, _counter) \
	atomic_inc(&(_media)->cause_counters[_counter].count)

#define SL_CTRL_MEDIA_TEMP_STATE_COUNTER_INC(_media, _counter) \
	atomic_inc(&(_media)->temp_state_counters[_counter].count)

int  sl_ctrl_media_cause_counters_init(struct sl_media_jack *media_jack);
void sl_ctrl_media_cause_counters_del(struct sl_media_jack *media_jack);
int  sl_ctrl_media_cause_counter_get(struct sl_media_jack *media_jack, u32 counter, int *count);
void sl_ctrl_media_cause_counter_inc(struct sl_media_jack *media_jack, unsigned long cause_map);

int  sl_ctrl_media_temp_state_counters_init(struct sl_media_jack *media_jack);
void sl_ctrl_media_temp_state_counters_del(struct sl_media_jack *media_jack);
int  sl_ctrl_media_temp_state_counter_get(struct sl_media_jack *media_jack, u32 counter, int *count);
void sl_ctrl_media_temp_state_counter_inc(struct sl_media_jack *media_jack, u8 state);

#endif /* _SL_CTRL_MEDIA_COUNTERS_H_ */
