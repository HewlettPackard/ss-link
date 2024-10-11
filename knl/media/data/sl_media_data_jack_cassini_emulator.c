// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"

#include "base/sl_media_log.h"

#include "sl_media_ldev.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_cassini_emulator.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

int sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp)
{
	struct sl_media_jack *media_jack;
	u8                    i;

	sl_media_log_dbg(media_lgrp, LOG_NAME, "jack lgrp connect");

	media_jack = sl_media_data_jack_get(media_lgrp->media_ldev->num,
			media_lgrp->num >> (SL_MEDIA_LGRPS_DIVIDE_PER_PORT));
	for (i = 0; i < SL_MEDIA_LGRPS_PER_PORT; ++i) {
		if (media_jack->cable_info[i].lgrp_num == media_lgrp->num) {
			media_lgrp->media_jack = media_jack;
			media_lgrp->cable_info = &media_jack->cable_info[i];
			return 0;
		}
	}

	return -EFAULT;
}

void sl_media_data_jack_unregister_event_notifier(void)
{
}
