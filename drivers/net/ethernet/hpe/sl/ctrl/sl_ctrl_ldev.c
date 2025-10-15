// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/completion.h>
#include <linux/delay.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "base/sl_ctrl_log.h"
#include "sl_core_ldev.h"
#include "sl_media_ldev.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"

#define LOG_NAME SL_CTRL_LDEV_LOG_NAME

#define SL_CTRL_LDEV_DEL_TIMEOUT_MS 2000

static struct sl_ctrl_ldev *ctrl_ldevs[SL_ASIC_MAX_LDEVS];
static DEFINE_SPINLOCK(ctrl_ldevs_lock);

#define CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE  0 /* 0 is default (256 work items in queue). */
int sl_ctrl_ldev_new(u8 ldev_num, struct workqueue_struct *workq,
		    struct sl_ldev_attr *ldev_attr)
{
	int                   rtn;
	struct sl_ctrl_ldev   *ctrl_ldev;

	ctrl_ldev = sl_ctrl_ldev_get(ldev_num);
	if (ctrl_ldev) {
		sl_ctrl_log_err(ctrl_ldev, LOG_NAME, "exists (ldev = 0x%p)", ctrl_ldev);
		return -EBADRQC;
	}

	ctrl_ldev = kzalloc(sizeof(struct sl_ctrl_ldev), GFP_KERNEL);
	if (!ctrl_ldev)
		return -ENOMEM;

	ctrl_ldev->magic = SL_CTRL_LDEV_MAGIC;
	ctrl_ldev->ver   = SL_CTRL_LDEV_VER;
	ctrl_ldev->num   = ldev_num;
	ctrl_ldev->attr  = *ldev_attr;

	kref_init(&ctrl_ldev->ref_cnt);
	init_completion(&ctrl_ldev->del_complete);

	spin_lock_init(&ctrl_ldev->data_lock);

	if (IS_ERR_OR_NULL(workq)) {
		ctrl_ldev->workq = alloc_workqueue("%s%u", WQ_MEM_RECLAIM,
						   CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE, "sl-ldev", ldev_num);
		if (IS_ERR(ctrl_ldev->workq)) {
			rtn = PTR_ERR(ctrl_ldev->workq);
			sl_ctrl_log_err(ctrl_ldev, LOG_NAME, "alloc_workqueue failed [%d]", rtn);
			goto out;
		}

		sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "created workqueue (workq = 0x%p)", ctrl_ldev->workq);
		ctrl_ldev->create_workq = true;
	} else {
		ctrl_ldev->workq = workq;
		sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "client workqueue (workq = 0x%p)", ctrl_ldev->workq);
	}

	ctrl_ldev->notif_workq = alloc_ordered_workqueue("%s%u-notif", WQ_MEM_RECLAIM, "sl-ldev", ldev_num);
	if (IS_ERR(ctrl_ldev->notif_workq)) {
		rtn = PTR_ERR(ctrl_ldev->notif_workq);
		sl_ctrl_log_err(ctrl_ldev, LOG_NAME, "alloc_ordered_workqueue notif failed [%d]", rtn);
		goto out_del_wq;
	}

	rtn = sl_core_ldev_new(ldev_num, ctrl_ldev->attr.accessors,
		ctrl_ldev->attr.ops, ctrl_ldev->workq);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_ldev, LOG_NAME, "core_ldev_new failed [%d]", rtn);
		goto out_del_notif_wq;
	}

	rtn = sl_media_ldev_new(ldev_num, ctrl_ldev->workq);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_ldev, LOG_NAME, "media_ldev_new failed [%d]", rtn);
		goto out_del_notif_wq;
	}

	spin_lock(&ctrl_ldevs_lock);
	ctrl_ldevs[ldev_num] = ctrl_ldev;
	spin_unlock(&ctrl_ldevs_lock);

	sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "new (ldev = 0x%p)", ctrl_ldev);

	return 0;

out_del_notif_wq:
	destroy_workqueue(ctrl_ldev->notif_workq);

out_del_wq:
	if (ctrl_ldev->create_workq)
		destroy_workqueue(ctrl_ldev->workq);
out:
	kfree(ctrl_ldev);

	return rtn;
}

#define SERDES_INIT_MAX_TRIES 3
#define SERDES_INIT_DELAY_MS  1000
int sl_ctrl_ldev_serdes_init(u8 ldev_num)
{
	int rtn;
	int tries;
	struct sl_ctrl_ldev *ctrl_ldev;

	ctrl_ldev = sl_ctrl_ldev_get(ldev_num);
	if (!ctrl_ldev) {
		sl_ctrl_log_err(NULL, LOG_NAME, "ldev not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "serdes init (ldev = 0x%p)", ctrl_ldev);

	if (!sl_ctrl_ldev_kref_get_unless_zero(ctrl_ldev)) {
		sl_ctrl_log_err(ctrl_ldev, LOG_NAME, "kref_get_unless_zero failed (ldev = 0x%p)", ctrl_ldev);
		return -EBADRQC;
	}

	tries = SERDES_INIT_MAX_TRIES;
	do {
		rtn = sl_core_ldev_serdes_init(ldev_num);
		if (rtn == 0)
			goto out;
		tries--;
		if (tries)
			msleep(SERDES_INIT_DELAY_MS);
	} while (tries);

out:
	if (sl_ctrl_ldev_put(ctrl_ldev))
		sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "serdes init - ldev removed (ldev = 0x%p)", ctrl_ldev);

	return rtn;
}

static void sl_ctrl_ldev_release(struct kref *ref)
{
	struct sl_ctrl_ldev *ctrl_ldev;
	u8                  ldev_num;
	u8                  lgrp_num;

	ctrl_ldev = container_of(ref, struct sl_ctrl_ldev, ref_cnt);
	ldev_num = ctrl_ldev->num;

	sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "release (ldev = 0x%p)", ctrl_ldev);

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num)
		sl_ctrl_lgrp_del(ldev_num, lgrp_num);

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_ldev_delete(ctrl_ldev);

	sl_core_ldev_del(ldev_num);
	sl_media_ldev_del(ldev_num);

	destroy_workqueue(ctrl_ldev->notif_workq);

	if (ctrl_ldev->create_workq)
		destroy_workqueue(ctrl_ldev->workq);

	spin_lock(&ctrl_ldevs_lock);
	ctrl_ldevs[ldev_num] = NULL;
	spin_unlock(&ctrl_ldevs_lock);

	complete_all(&ctrl_ldev->del_complete);

	kfree(ctrl_ldev);
}

int sl_ctrl_ldev_put(struct sl_ctrl_ldev *ctrl_ldev)
{
	return kref_put(&ctrl_ldev->ref_cnt, sl_ctrl_ldev_release);
}

int sl_ctrl_ldev_del(u8 ldev_num)
{
	struct sl_ctrl_ldev *ctrl_ldev;
	unsigned long       timeleft;

	ctrl_ldev = sl_ctrl_ldev_get(ldev_num);
	if (!ctrl_ldev) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME,
			"del not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "del (ldev = 0x%p)", ctrl_ldev);

	if (!sl_ctrl_ldev_put(ctrl_ldev)) {
		timeleft = wait_for_completion_timeout(&ctrl_ldev->del_complete,
			msecs_to_jiffies(SL_CTRL_LDEV_DEL_TIMEOUT_MS));

		sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctrl_log_err_trace(ctrl_ldev, LOG_NAME,
				"del timed out (ctrl_ldev = 0x%p)", ctrl_ldev);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

bool sl_ctrl_ldev_kref_get_unless_zero(struct sl_ctrl_ldev *ctrl_ldev)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctrl_ldev->ref_cnt) != 0);

	if (!incremented)
		sl_ctrl_log_warn(ctrl_ldev, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ldev = 0x%p)", ctrl_ldev);

	return incremented;
}

struct sl_ctrl_ldev *sl_ctrl_ldev_get(u8 ldev_num)
{
	struct sl_ctrl_ldev *ctrl_ldev;

	spin_lock(&ctrl_ldevs_lock);
	ctrl_ldev = ctrl_ldevs[ldev_num];
	spin_unlock(&ctrl_ldevs_lock);

	sl_ctrl_log_dbg(ctrl_ldev, LOG_NAME, "get (ldev = 0x%p)", ctrl_ldev);

	return ctrl_ldev;
}

void sl_ctrl_ldev_exit(void)
{
	u8 ldev_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num)
		sl_ctrl_ldev_del(ldev_num);
}
