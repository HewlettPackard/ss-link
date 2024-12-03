/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_MEDIA_H_
#define _LINUX_SL_MEDIA_H_

#include "uapi/sl_media.h"

struct sl_lgrp;

int sl_media_cable_insert(struct sl_lgrp *lgrp, u8 *eeprom_page0,
			  u8 *eeprom_page1, u32 flags);
int sl_media_cable_remove(struct sl_lgrp *lgrp);

int sl_media_fake_headshell_insert(struct sl_lgrp *lgrp,  struct sl_media_attr *media_attr);
int sl_media_fake_headshell_remove(struct sl_lgrp *lgrp);

const char *sl_media_furcation_str(u32 furcation);
const char *sl_media_state_str(u8 state);
const char *sl_media_cable_shift_state_str(u8 cable_shift_state);
const char *sl_media_type_str(u32 type);
const char *sl_media_vendor_str(u32 vendor);
const char *sl_media_speed_str(u32 speed);
const char *sl_media_ber_str(u8 media_interface);
const char *sl_media_host_interface_str(u32 speed, u32 type);
const char *sl_media_opt_str(u32 option);
const char *sl_media_jack_type_str(u32 jack_type, u32 density);

#endif /* _LINUX_SL_MEDIA_H_ */
