// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include "sl_asic.h"
#include "sl_media_jack.h"
#include "sl_media_lgrp.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_lgrp.h"
#include "base/sl_media_log.h"

#define LOG_NAME SL_MEDIA_JACK_LOG_NAME

int sl_media_jack_fake_cable_insert(u8 ldev_num, u8 lgrp_num, struct sl_media_attr *media_attr)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	if (!media_lgrp) {
		sl_media_log_err(NULL, SL_MEDIA_TEST_LOG_NAME,
			"fake_cable_insert - media_data_lgrp_get failed");
		return -EFAULT;
	}

	sl_media_log_dbg(media_lgrp->media_jack, SL_MEDIA_LGRP_LOG_NAME,
		"fake_cable_insert (lgrp_num = %u)", lgrp_num);

	media_lgrp->cable_info->ldev_num = ldev_num;
	media_lgrp->cable_info->lgrp_num = lgrp_num;
	rtn = sl_media_data_jack_fake_media_attr_set(media_lgrp->media_jack,
			media_lgrp->cable_info, media_attr);
	if (rtn) {
		sl_media_log_err(media_lgrp->media_jack, SL_MEDIA_TEST_LOG_NAME,
			"fake cable insert - media attr set failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

void sl_media_jack_fake_cable_remove(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	if (!media_lgrp) {
		sl_media_log_err(NULL, SL_MEDIA_TEST_LOG_NAME,
			"fake_cable_remove - media_data_lgrp_get failed");
		return;
	}

	sl_media_log_dbg(media_lgrp->media_jack, SL_MEDIA_LGRP_LOG_NAME,
		"fake_cable_remove (lgrp_num = %u)", lgrp_num);

	sl_media_data_jack_fake_media_attr_clr(media_lgrp->media_jack, media_lgrp->cable_info);
}
