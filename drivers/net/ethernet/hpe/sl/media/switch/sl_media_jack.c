// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "data/sl_media_data_jack.h"

bool sl_media_jack_cable_is_high_temp_set(struct sl_media_jack *media_jack)
{
	return sl_media_data_jack_cable_is_high_temp_set(media_jack);
}

void sl_media_jack_cable_is_high_temp_clr(struct sl_media_jack *media_jack)
{
	return sl_media_data_jack_cable_is_high_temp_clr(media_jack);
}

void sl_media_jack_cable_high_temp_notif_send(struct sl_media_jack *media_jack)
{
	sl_media_data_jack_cable_high_temp_notif_send(media_jack);
}

void sl_media_jack_cable_high_temp_notif_sent_set(struct sl_media_jack *media_jack,
						  struct sl_media_lgrp_cable_info *cable_info, bool value)
{
	sl_media_data_jack_cable_high_temp_notif_sent_set(media_jack, cable_info, value);
}
