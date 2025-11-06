// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_ldev.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_jack.h"
#include "sl_ctrl_ldev.h"

static struct sl_media_ldev *media_ldevs[SL_ASIC_MAX_LDEVS];
static DEFINE_SPINLOCK(media_ldevs_lock);

#define LOG_NAME SL_MEDIA_DATA_LDEV_LOG_NAME

int sl_media_data_ldev_new(u8 ldev_num, struct workqueue_struct *workqueue)
{
	struct sl_media_ldev *media_ldev;
	int                   rtn;
	u8                    jack_num;

	media_ldev = sl_media_data_ldev_get(ldev_num);
	if (media_ldev) {
		sl_media_log_err(media_ldev, LOG_NAME, "exists (ldev = 0x%p)", media_ldev);
		return -EBADRQC;
	}

	media_ldev = kzalloc(sizeof(struct sl_media_ldev), GFP_KERNEL);
	if (!media_ldev)
		return -ENOMEM;

	media_ldev->magic     = SL_MEDIA_LDEV_MAGIC;
	media_ldev->num       = ldev_num;
	media_ldev->workqueue = workqueue;

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		rtn = sl_media_data_jack_new(media_ldev, jack_num);
		if (rtn) {
			sl_media_log_err(media_ldev, LOG_NAME, "jack new failed [%d]", rtn);
			kfree(media_ldev);
			return -EFAULT;
		}
	}

	rtn = sl_media_data_jack_scan(ldev_num);
	if (rtn) {
		sl_media_log_err(media_ldev, LOG_NAME, "jack scan failed [%d]", rtn);
		for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num)
			sl_media_data_jack_del(ldev_num, jack_num);
		kfree(media_ldev);
		return -EFAULT;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME, "new (ldev = 0x%p)", media_ldev);

	spin_lock(&media_ldevs_lock);
	media_ldevs[ldev_num] = media_ldev;
	spin_unlock(&media_ldevs_lock);

	sl_media_data_jack_cable_high_temp_monitor_start(media_ldev);

	return 0;
}

void sl_media_data_ldev_del(u8 ldev_num)
{
	struct sl_media_ldev *media_ldev;
	u8                    lgrp_num;
	u8                    jack_num;

	media_ldev = sl_media_data_ldev_get(ldev_num);
	if (!media_ldev) {
		sl_media_log_dbg(NULL, LOG_NAME, "not found (ldev_num = %u)", ldev_num);
		return;
	}

	cancel_delayed_work_sync(&media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_HIGH_TEMP]);

	spin_lock(&media_ldevs_lock);
	media_ldevs[ldev_num] = NULL;
	spin_unlock(&media_ldevs_lock);

	sl_media_log_dbg(media_ldev, LOG_NAME, "del (ldev = 0x%p)", media_ldev);

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num)
		sl_media_data_lgrp_del(ldev_num, lgrp_num);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num)
		sl_media_data_jack_del(ldev_num, jack_num);

	sl_media_data_jack_unregister_event_notifier();

	kfree(media_ldev);
}

struct sl_media_ldev *sl_media_data_ldev_get(u8 ldev_num)
{
	struct sl_media_ldev *media_ldev;

	spin_lock(&media_ldevs_lock);
	media_ldev = media_ldevs[ldev_num];
	spin_unlock(&media_ldevs_lock);

	sl_media_log_dbg(media_ldev, LOG_NAME, "get (ldev = 0x%p)", media_ldev);

	return media_ldev;
}
