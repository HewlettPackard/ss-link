// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>

#include <linux/sl_media.h>

#include "sl_media_lgrp.h"
#include "sl_media_jack.h"
#include "sl_media_data_lgrp.h"
#include "sl_media_data_ldev.h"
#include "sl_media_data_jack.h"
#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "sl_media_data_jack_sw2.h"
#include "sl_media_data_jack_nic2_emu.h"

static struct sl_media_lgrp *media_lgrps[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];
static DEFINE_SPINLOCK(media_lgrps_lock);

#define LOG_NAME SL_MEDIA_DATA_LGRP_LOG_NAME

int sl_media_data_lgrp_new(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;
	int                   rtn;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	if (media_lgrp) {
		sl_media_log_err(media_lgrp, LOG_NAME, "exists (lgrp = 0x%p)", media_lgrp);
		return -EBADRQC;
	}

	media_lgrp = kzalloc(sizeof(struct sl_media_lgrp), GFP_KERNEL);
	if (!media_lgrp)
		return -ENOMEM;

	media_lgrp->magic       = SL_MEDIA_LGRP_MAGIC;
	media_lgrp->num         = lgrp_num;
	media_lgrp->media_ldev  = sl_media_data_ldev_get(ldev_num);

	media_lgrp->speeds_kobj_init = false;
	spin_lock_init(&media_lgrp->log_lock);
	snprintf(media_lgrp->connect_id, sizeof(media_lgrp->connect_id), "lgrp%02u", lgrp_num);

	rtn = sl_media_data_jack_lgrp_connect(media_lgrp);
	if (rtn) {
		sl_media_log_err(media_lgrp, LOG_NAME, "jack lgrp connect failed [%d]", rtn);
		kfree(media_lgrp);
		return -EFAULT;
	}

	sl_media_log_dbg(media_lgrp, LOG_NAME, "new (lgrp = 0x%p)", media_lgrp);

	spin_lock(&media_lgrps_lock);
	media_lgrps[ldev_num][lgrp_num] = media_lgrp;
	spin_unlock(&media_lgrps_lock);

	return 0;
}

void sl_media_data_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(ldev_num, lgrp_num);
	if (!media_lgrp) {
		sl_media_log_dbg(NULL, LOG_NAME, "not found (lgrp_num = %u)", lgrp_num);
		return;
	}

	spin_lock(&media_lgrps_lock);
	media_lgrps[ldev_num][lgrp_num] = NULL;
	spin_unlock(&media_lgrps_lock);

	sl_media_log_dbg(media_lgrp, LOG_NAME, "del (lgrp = 0x%p)", media_lgrp);

	kfree(media_lgrp);
}

struct sl_media_lgrp *sl_media_data_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_media_lgrp *media_lgrp;

	spin_lock(&media_lgrps_lock);
	media_lgrp = media_lgrps[ldev_num][lgrp_num];
	spin_unlock(&media_lgrps_lock);

	sl_media_log_dbg(media_lgrp, LOG_NAME, "get (lgrp = 0x%p)", media_lgrp);

	return media_lgrp;
}

void sl_media_data_lgrp_connect_id_set(struct sl_media_lgrp *media_lgrp, const char *connect_id)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&(media_lgrp->log_lock), irq_flags);
	strncpy(media_lgrp->connect_id, connect_id, SL_LOG_CONNECT_ID_LEN);
	spin_unlock_irqrestore(&(media_lgrp->log_lock), irq_flags);

	sl_media_log_dbg(media_lgrp, SL_MEDIA_DATA_LGRP_LOG_NAME, "connect_id = %s", connect_id);
}
