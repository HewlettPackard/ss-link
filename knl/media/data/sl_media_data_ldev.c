// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_asic.h"

#include "base/sl_media_log.h"
#include "uapi/sl_media.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "sl_media_data_ldev.h"
#include "sl_media_data_lgrp.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_rosetta.h"
#include "sl_media_data_jack_emulator.h"
#include "sl_media_data_jack_cassini.h"
#include "sl_media_data_jack_cassini_emulator.h"

static struct sl_media_ldev *media_ldevs[SL_ASIC_MAX_LDEVS];
static DEFINE_SPINLOCK(media_ldevs_lock);

#define LOG_NAME SL_MEDIA_DATA_LDEV_LOG_NAME

int sl_media_data_ldev_new(u8 ldev_num)
{
	unsigned long         irq_flags;
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

	media_ldev->magic = SL_MEDIA_LDEV_MAGIC;
	media_ldev->num   = ldev_num;

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

	spin_lock_irqsave(&media_ldevs_lock, irq_flags);
	media_ldevs[ldev_num] = media_ldev;
	spin_unlock_irqrestore(&media_ldevs_lock, irq_flags);

	return 0;
}

void sl_media_data_ldev_del(u8 ldev_num)
{
	unsigned long         irq_flags;
	struct sl_media_ldev *media_ldev;
	u8                    lgrp_num;
	u8                    jack_num;

	media_ldev = sl_media_data_ldev_get(ldev_num);
	if (!media_ldev) {
		sl_media_log_dbg(NULL, LOG_NAME, "not found (ldev_num = %u)", ldev_num);
		return;
	}

	spin_lock_irqsave(&media_ldevs_lock, irq_flags);
	media_ldevs[ldev_num] = NULL;
	spin_unlock_irqrestore(&media_ldevs_lock, irq_flags);

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
	unsigned long         irq_flags;
	struct sl_media_ldev *media_ldev;

	spin_lock_irqsave(&media_ldevs_lock, irq_flags);
	media_ldev = media_ldevs[ldev_num];
	spin_unlock_irqrestore(&media_ldevs_lock, irq_flags);

	sl_media_log_dbg(media_ldev, LOG_NAME, "get (ldev = 0x%p)", media_ldev);

	return media_ldev;
}

int sl_media_data_ldev_uc_ops_set(u8 ldev_num, struct sl_uc_ops *uc_ops,
				struct sl_uc_accessor *uc_accessor)
{
	struct sl_media_ldev *media_ldev;

	media_ldev = sl_media_ldev_get(ldev_num);
	if (!media_ldev) {
		sl_media_log_err(NULL, LOG_NAME, "ldev not found (ldev_num = %u)", ldev_num);
		return -EINVAL;
	}

	sl_media_log_dbg(media_ldev, LOG_NAME, "media_data_ldev_uc_ops_set");

	media_ldev->uc_ops = uc_ops;
	media_ldev->uc_accessor = uc_accessor;

	return 0;
}
