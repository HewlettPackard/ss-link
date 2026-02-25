// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include "sl_media_jack.h"

int sl_media_jack_cable_temp_hw_check(struct sl_media_jack *media_jack)
{
	return false;
}

void sl_media_jack_cable_hot_notif_send(struct sl_media_jack *media_jack)
{
}

void sl_media_jack_cable_warm_notif_send(struct sl_media_jack *media_jack)
{
}

void sl_media_jack_cable_cold_notif_send(struct sl_media_jack *media_jack)
{
}

void sl_media_jack_cable_hot_notif_sent_set(struct sl_media_jack *media_jack,
					    struct sl_media_lgrp_cable_info *cable_info, bool value)
{
}

void sl_media_jack_cable_warm_notif_sent_set(struct sl_media_jack *media_jack,
					     struct sl_media_lgrp_cable_info *cable_info, bool value)
{
}

void sl_media_jack_cable_cold_notif_sent_set(struct sl_media_jack *media_jack,
					     struct sl_media_lgrp_cable_info *cable_info, bool value)
{
}
