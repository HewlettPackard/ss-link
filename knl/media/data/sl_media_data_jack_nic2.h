/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_JACK_NIC2_H_
#define _SL_MEDIA_DATA_JACK_NIC2_H_

struct sl_media_jack;
struct sl_media_lgrp_cable_info;
struct sl_media_attr;

int  sl_media_data_jack_scan(u8 ldev_num);
int  sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack);
int  sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack);

int  sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr);
void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info);

int  sl_media_data_jack_cable_temp_get(struct sl_media_jack *media_jack, u8 *temp);

void sl_media_data_jack_led_set(struct sl_media_jack *media_jack);

#endif /* _SL_MEDIA_DATA_JACK_NIC2_H_ */
