/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_JACK_H_
#define _SL_MEDIA_DATA_JACK_H_

#include "sl_media_lgrp.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"

/*
 * These states reflect the state of media attr for both real and fake cables
 */
#define CABLE_MEDIA_ATTR_REMOVED  0
#define CABLE_MEDIA_ATTR_ADDED    1
#define CABLE_MEDIA_ATTR_STASHED  2

void                  sl_media_data_jack_del(u8 ldev_num, u8 jack_num);
int                   sl_media_data_jack_new(struct sl_media_ldev *media_ldev, u8 jack_num);
struct sl_media_jack *sl_media_data_jack_get(u8 ldev_num, u8 jack_num);

void sl_media_data_jack_unregister_event_notifier(void);
void sl_media_data_cable_serdes_settings_clr(struct sl_media_jack *media_jack);
void sl_media_data_jack_eeprom_clr(struct sl_media_jack *media_jack);
int  sl_media_data_jack_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *media_attr);
void sl_media_data_jack_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info);

void sl_media_data_jack_cable_if_present_send(struct sl_media_lgrp *media_lgrp);
void sl_media_data_jack_cable_if_not_present_send(struct sl_media_lgrp *media_lgrp);

int  sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack);

bool sl_media_data_jack_cable_is_high_temp(struct sl_media_jack *media_jack);

int  sl_media_data_jack_cable_temp_get(struct sl_media_jack *media_jack, u8 *temp);
int  sl_media_data_jack_cable_high_temp_threshold_get(struct sl_media_jack *media_jack, u8 *temp_threshold);
int  sl_media_data_jack_high_temp_link_down(struct sl_media_jack *media_jack);

int  sl_media_data_jack_scan(u8 ldev_num);
int  sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp);

int  sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr);
void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info);

int  sl_media_data_jack_online(void *hdl, u8 ldev_num, u8 jack_num);

int  sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack);

void sl_media_data_jack_led_set(struct sl_media_jack *media_jack);
void sl_media_data_jack_headshell_led_set(struct sl_media_jack *media_jack, u8 jack_state);
void sl_media_data_jack_cable_high_temp_monitor_start(struct sl_media_ldev *media_ldev);

#endif /* _SL_MEDIA_DATA_JACK_H_ */
