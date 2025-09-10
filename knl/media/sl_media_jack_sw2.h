/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_JACK_SW2_H_
#define _SL_MEDIA_JACK_SW2_H_

int  sl_media_jack_fake_cable_insert(u8 ldev_num, u8 lgrp_num, struct sl_media_attr *media_attr);
void sl_media_jack_fake_cable_remove(u8 ldev_num, u8 lgrp_num);

#endif /* _SL_MEDIA_JACK_SW2_H_ */
