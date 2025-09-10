/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_DATA_JACK_SW2_EMU_H_
#define _SL_MEDIA_DATA_JACK_SW2_EMU_H_

/*
 * These states reflect the state of media attr for both real and fake cables
 */
#define CABLE_MEDIA_ATTR_REMOVED  0
#define CABLE_MEDIA_ATTR_ADDED    1
#define CABLE_MEDIA_ATTR_STASHED  2

struct sl_media_lgrp_cable_info;

int  sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr);
void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info);

#endif /* _SL_MEDIA_DATA_JACK_SW2_EMU_H_ */
