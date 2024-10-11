/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_JACK_CASSINI_H_
#define _SL_MEDIA_JACK_CASSINI_H_

int sl_media_jack_cable_insert(u8 ldev_num, u8 lgrp_num, u8 jack_num,
			       u8 *eeprom_page0, u8 *eeprom_page1, u32 flags);
int sl_media_jack_cable_remove(u8 ldev_num, u8 lgrp_num, u8 jack_num);

#endif /* _SL_MEDIA_JACK_CASSINI_H_ */
