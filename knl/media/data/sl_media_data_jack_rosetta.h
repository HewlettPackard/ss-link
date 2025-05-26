/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_JACK_ROSETTA_H_
#define _SL_MEDIA_DATA_JACK_ROSETTA_H_

struct sl_media_jack;
struct sl_media_lgrp;

int  sl_media_data_jack_scan(u8 ldev_num);
void sl_media_data_jack_unregister_event_notifier(void);
int  sl_media_data_jack_online(void *hdl, u8 ldev_num, u8 jack_num);
int  sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp);
int  sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack);

bool sl_media_data_jack_cable_is_high_temp(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_temp(struct sl_media_jack *media_jack, u8 *temp);

void sl_media_data_jack_link_led_set(struct sl_media_jack *media_jack, u32 link_state);
void sl_media_data_jack_headshell_led_set(struct sl_media_jack *media_jack, u8 jack_state);

#endif /* _SL_MEDIA_DATA_JACK_ROSETTA_H_ */
