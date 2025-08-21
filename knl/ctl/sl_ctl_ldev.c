// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/completion.h>
#include <linux/delay.h>

#include "sl_asic.h"
#include "sl_sysfs.h"

#include "base/sl_ctl_log.h"

#include "sl_core_ldev.h"
#include "sl_media_ldev.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"

#define LOG_NAME SL_CTL_LDEV_LOG_NAME

#define SL_CTL_LDEV_DEL_TIMEOUT_MS 2000

static struct sl_ctl_ldev *ctl_ldevs[SL_ASIC_MAX_LDEVS];
static DEFINE_SPINLOCK(ctl_ldevs_lock);

int sl_ctl_ldev_new(u8 ldev_num, struct workqueue_struct *workq,
		    struct sl_ldev_attr *ldev_attr)
{
	int                   rtn;
	struct sl_ctl_ldev   *ctl_ldev;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);
	if (ctl_ldev) {
		sl_ctl_log_err(ctl_ldev, LOG_NAME, "exists (ldev = 0x%p)", ctl_ldev);
		return -EBADRQC;
	}

	ctl_ldev = kzalloc(sizeof(struct sl_ctl_ldev), GFP_KERNEL);
	if (!ctl_ldev)
		return -ENOMEM;

	ctl_ldev->magic = SL_CTL_LDEV_MAGIC;
	ctl_ldev->ver   = SL_CTL_LDEV_VER;
	ctl_ldev->num   = ldev_num;
	ctl_ldev->attr  = *ldev_attr;

	kref_init(&ctl_ldev->ref_cnt);
	init_completion(&ctl_ldev->del_complete);

	spin_lock_init(&ctl_ldev->data_lock);

	if (IS_ERR_OR_NULL(workq)) {
		ctl_ldev->workq = alloc_workqueue("%s%u", WQ_MEM_RECLAIM,
			CONFIG_SL_DEFAULT_WQ_MAX_ACTIVE, "sl-ldev", ldev_num);
		if (IS_ERR(ctl_ldev->workq)) {
			rtn = PTR_ERR(ctl_ldev->workq);
			sl_ctl_log_err(ctl_ldev, LOG_NAME, "alloc_workqueue failed [%d]", rtn);
			goto out;
		}

		sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "created workqueue (workq = 0x%p)", ctl_ldev->workq);
		ctl_ldev->create_workq = true;
	} else {
		ctl_ldev->workq = workq;
		sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "client workqueue (workq = 0x%p)", ctl_ldev->workq);
	}

	ctl_ldev->notif_workq = alloc_ordered_workqueue("%s%u-notif", WQ_MEM_RECLAIM, "sl-ldev", ldev_num);
	if (IS_ERR(ctl_ldev->notif_workq)) {
		rtn = PTR_ERR(ctl_ldev->notif_workq);
		sl_ctl_log_err(ctl_ldev, LOG_NAME, "alloc_ordered_workqueue notif failed [%d]", rtn);
		goto out_del_wq;
	}

	rtn = sl_core_ldev_new(ldev_num, ctl_ldev->attr.accessors,
		ctl_ldev->attr.ops, ctl_ldev->workq);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_ldev, LOG_NAME, "core_ldev_new failed [%d]", rtn);
		goto out_del_notif_wq;
	}

	rtn = sl_media_ldev_new(ldev_num);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_ldev, LOG_NAME, "media_ldev_new failed [%d]", rtn);
		goto out_del_notif_wq;
	}

	spin_lock(&ctl_ldevs_lock);
	ctl_ldevs[ldev_num] = ctl_ldev;
	spin_unlock(&ctl_ldevs_lock);

	sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "new (ldev = 0x%p)", ctl_ldev);

	return 0;

out_del_notif_wq:
	destroy_workqueue(ctl_ldev->notif_workq);

out_del_wq:
	if (ctl_ldev->create_workq)
		destroy_workqueue(ctl_ldev->workq);
out:
	kfree(ctl_ldev);

	return rtn;
}

#define SERDES_INIT_MAX_TRIES 3
#define SERDES_INIT_DELAY_MS  1000
int sl_ctl_ldev_serdes_init(u8 ldev_num)
{
	int rtn;
	int tries;
	struct sl_ctl_ldev *ctl_ldev;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);
	if (!ctl_ldev) {
		sl_ctl_log_err(NULL, LOG_NAME, "ldev not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "serdes init (ldev = 0x%p)", ctl_ldev);

	if (!sl_ctl_ldev_kref_get_unless_zero(ctl_ldev)) {
		sl_ctl_log_err(ctl_ldev, LOG_NAME, "kref_get_unless_zero failed (ldev = 0x%p)", ctl_ldev);
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
	if (sl_ctl_ldev_put(ctl_ldev))
		sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "serdes init - ldev removed (ldev = 0x%p)", ctl_ldev);

	return rtn;
}

static void sl_ctl_ldev_release(struct kref *ref)
{
	struct sl_ctl_ldev *ctl_ldev;
	u8                  ldev_num;
	u8                  lgrp_num;

	ctl_ldev = container_of(ref, struct sl_ctl_ldev, ref_cnt);
	ldev_num = ctl_ldev->num;

	sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "release (ldev = 0x%p)", ctl_ldev);

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num)
		sl_ctl_lgrp_del(ldev_num, lgrp_num);

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_ldev_delete(ctl_ldev);

	sl_core_ldev_del(ldev_num);
	sl_media_ldev_del(ldev_num);

	destroy_workqueue(ctl_ldev->notif_workq);

	if (ctl_ldev->create_workq)
		destroy_workqueue(ctl_ldev->workq);

	spin_lock(&ctl_ldevs_lock);
	ctl_ldevs[ldev_num] = NULL;
	spin_unlock(&ctl_ldevs_lock);

	complete_all(&ctl_ldev->del_complete);

	kfree(ctl_ldev);
}

int sl_ctl_ldev_put(struct sl_ctl_ldev *ctl_ldev)
{
	return kref_put(&ctl_ldev->ref_cnt, sl_ctl_ldev_release);
}

int sl_ctl_ldev_del(u8 ldev_num)
{
	struct sl_ctl_ldev *ctl_ldev;
	unsigned long       timeleft;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);
	if (!ctl_ldev) {
		sl_ctl_log_err_trace(NULL, LOG_NAME,
			"del not found (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "del (ldev = 0x%p)", ctl_ldev);

	if (!sl_ctl_ldev_put(ctl_ldev)) {
		timeleft = wait_for_completion_timeout(&ctl_ldev->del_complete,
			msecs_to_jiffies(SL_CTL_LDEV_DEL_TIMEOUT_MS));

		sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctl_log_err_trace(ctl_ldev, LOG_NAME,
				"del timed out (ctl_ldev = 0x%p)", ctl_ldev);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

bool sl_ctl_ldev_kref_get_unless_zero(struct sl_ctl_ldev *ctl_ldev)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctl_ldev->ref_cnt) != 0);

	if (!incremented)
		sl_ctl_log_warn(ctl_ldev, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ldev = 0x%p)", ctl_ldev);

	return incremented;
}

struct sl_ctl_ldev *sl_ctl_ldev_get(u8 ldev_num)
{
	struct sl_ctl_ldev *ctl_ldev;

	spin_lock(&ctl_ldevs_lock);
	ctl_ldev = ctl_ldevs[ldev_num];
	spin_unlock(&ctl_ldevs_lock);

	sl_ctl_log_dbg(ctl_ldev, LOG_NAME, "get (ldev = 0x%p)", ctl_ldev);

	return ctl_ldev;
}

void sl_ctl_ldev_exit(void)
{
	u8 ldev_num;

	for (ldev_num = 0; ldev_num < SL_ASIC_MAX_LDEVS; ++ldev_num)
		sl_ctl_ldev_del(ldev_num);
}
