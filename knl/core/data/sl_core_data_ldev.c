// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <linux/sl_ldev.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "data/sl_core_data_ldev.h"
#include "data/sl_core_data_lgrp.h"
#include "hw/sl_core_hw_serdes_osprey.h"
// FIXME: include this when we have a condor
//#include "hw/sl_core_hw_serdes_condor.h"

static struct sl_core_ldev *core_ldevs[SL_ASIC_MAX_LDEVS];
static DEFINE_SPINLOCK(core_ldevs_lock);

#define LOG_NAME SL_CORE_DATA_LDEV_LOG_NAME

int sl_core_data_ldev_new(u8 ldev_num, struct sl_accessors *accessors,
	struct sl_ops *ops, struct workqueue_struct *workqueue)
{
	int                  rtn;
	struct sl_core_ldev *core_ldev;
	u16                  proto;

	core_ldev = sl_core_ldev_get(ldev_num);
	if (core_ldev) {
		sl_core_log_err(core_ldev, LOG_NAME, "exists (ldev = 0x%p)", core_ldev);
		return -EBADRQC;
	}

	core_ldev = kzalloc(sizeof(struct sl_core_ldev), GFP_KERNEL);
	if (core_ldev == NULL)
		return -ENOMEM;

	core_ldev->magic           = SL_CORE_LDEV_MAGIC;
	core_ldev->num             = ldev_num;
	core_ldev->accessors       = *accessors;
	core_ldev->ops             = *ops;
	spin_lock_init(&(core_ldev->data_lock));

	rtn = core_ldev->ops.mb_info_get(core_ldev->accessors.mb,
		&(core_ldev->platform), &(core_ldev->revision), &proto);
	if (rtn) {
		sl_core_log_err(core_ldev, LOG_NAME, "mb_info_get failed [%d]", rtn);
		return -EIO;
	}

	core_ldev->workqueue = workqueue;

	// FIXME: for now we assume it's R2, so Osprey
	core_ldev->serdes.addrs = serdes_addrs_osprey;

	sl_core_log_dbg(core_ldev, LOG_NAME, "new (ldev = 0x%p, platform = %u, revision = %u)",
		core_ldev, core_ldev->platform, core_ldev->revision);

	spin_lock(&core_ldevs_lock);
	core_ldevs[ldev_num] = core_ldev;
	spin_unlock(&core_ldevs_lock);

	return 0;
}

void sl_core_data_ldev_del(u8 ldev_num)
{
	struct sl_core_ldev *core_ldev;
	int                  lgrp_num;

	core_ldev = sl_core_ldev_get(ldev_num);
	if (!core_ldev) {
		sl_core_log_dbg(NULL, LOG_NAME, "not found (ldev_num = %u)", ldev_num);
		return;
	}

	spin_lock(&core_ldevs_lock);
	core_ldevs[ldev_num] = NULL;
	spin_unlock(&core_ldevs_lock);

	sl_core_log_dbg(core_ldev, LOG_NAME, "del (ldev = 0x%p)", core_ldev);

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num)
		sl_core_data_lgrp_del(ldev_num, lgrp_num);

	kfree(core_ldev);
}

struct sl_core_ldev *sl_core_data_ldev_get(u8 ldev_num)
{
	struct sl_core_ldev *core_ldev;

	spin_lock(&core_ldevs_lock);
	core_ldev = core_ldevs[ldev_num];
	spin_unlock(&core_ldevs_lock);

	sl_core_log_dbg(core_ldev, LOG_NAME, "get (ldev = 0x%p)", core_ldev);

	return core_ldev;
}
