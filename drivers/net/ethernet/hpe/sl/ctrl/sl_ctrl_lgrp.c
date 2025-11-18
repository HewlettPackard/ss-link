// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_asic.h"
#include "sl_sysfs.h"
#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_llr.h"
#include "sl_ctrl_mac.h"
#include "sl_media_lgrp.h"
#include "sl_core_lgrp.h"

#define LOG_NAME SL_CTRL_LGRP_LOG_NAME

#define SL_CTRL_LGRP_DEL_TIMEOUT_MS 2000

static struct sl_ctrl_lgrp *ctrl_lgrps[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];
static DEFINE_SPINLOCK(ctrl_lgrps_lock);

int sl_ctrl_lgrp_new(u8 ldev_num, u8 lgrp_num, struct kobject *sysfs_parent)
{
	int                  rtn;
	struct sl_ctrl_ldev *ctrl_ldev;
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_ldev = sl_ctrl_ldev_get(ldev_num);
	if (!ctrl_ldev) {
		sl_ctrl_log_err(NULL, LOG_NAME, "missing ldev (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (ctrl_lgrp) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "exists (lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	ctrl_lgrp = kzalloc(sizeof(struct sl_ctrl_lgrp), GFP_KERNEL);
	if (!ctrl_lgrp)
		return -ENOMEM;

	snprintf(ctrl_lgrp->log_connect_id,
		 sizeof(ctrl_lgrp->log_connect_id), "ctrl-lgrp%02u", lgrp_num);

	ctrl_lgrp->magic     = SL_CTRL_LGRP_MAGIC;
	ctrl_lgrp->ver       = SL_CTRL_LGRP_VER;
	ctrl_lgrp->num       = lgrp_num;
	ctrl_lgrp->ctrl_ldev = sl_ctrl_ldev_get(ldev_num);

	kref_init(&ctrl_lgrp->ref_cnt);
	init_completion(&ctrl_lgrp->del_complete);

	spin_lock_init(&ctrl_lgrp->data_lock);
	spin_lock_init(&ctrl_lgrp->config_lock);
	spin_lock_init(&ctrl_lgrp->log_lock);

	spin_lock_init(&(ctrl_lgrp->ctrl_notif.lock));

	rtn = kfifo_alloc(&ctrl_lgrp->ctrl_notif.fifo,
			  SL_CTRL_LGRP_NOTIF_MSG_COUNT * sizeof(struct sl_lgrp_notif_msg),
			  GFP_KERNEL);
	if (rtn) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "create notif fifo failed");
		rtn = -ENOMEM;
		goto out_free;
	}
	INIT_WORK(&ctrl_lgrp->notif_work, sl_ctrl_lgrp_notif_work);

	rtn = sl_core_lgrp_new(ldev_num, lgrp_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME, "core_lgrp_new failed [%d]", rtn);
		goto out_kfifo_free;
	}

	rtn = sl_media_lgrp_new(ldev_num, lgrp_num);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME, "media_lgrp_new failed [%d]", rtn);
		goto out_core_lgrp;
	}

	if (sysfs_parent) {
		ctrl_lgrp->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_lgrp_create(ctrl_lgrp);
		if (rtn) {
			sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME,
					      "sysfs_lgrp_create failed");
			goto out_core_lgrp;
		}
	}

	spin_lock(&ctrl_lgrps_lock);
	ctrl_lgrps[ldev_num][lgrp_num] = ctrl_lgrp;
	spin_unlock(&ctrl_lgrps_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "new (lgrp = 0x%p)", ctrl_lgrp);

	return 0;

out_core_lgrp:
	sl_core_lgrp_del(ldev_num, lgrp_num);
out_kfifo_free:
	kfifo_free(&ctrl_lgrp->ctrl_notif.fifo);
out_free:
	kfree(ctrl_lgrp);

	return rtn;
}

static void sl_ctrl_lgrp_release(struct kref *ref)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u8                   ldev_num;
	u8                   lgrp_num;
	u8                   link_num;

	ctrl_lgrp = container_of(ref, struct sl_ctrl_lgrp, ref_cnt);
	ldev_num = ctrl_lgrp->ctrl_ldev->num;
	lgrp_num = ctrl_lgrp->num;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME,
			"release lgrp not found (lgrp_num = %u)", lgrp_num);
		return;
	}

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "del (lgrp = 0x%p)", ctrl_lgrp);

	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
		sl_ctrl_link_del(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num, link_num);
		sl_ctrl_llr_del(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num, link_num);
		sl_ctrl_mac_del(ctrl_lgrp->ctrl_ldev->num, ctrl_lgrp->num, link_num);
	}

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_lgrp_delete(ctrl_lgrp);

	sl_core_lgrp_del(ldev_num, lgrp_num);
	sl_media_lgrp_del(ldev_num, lgrp_num);

	cancel_work_sync(&ctrl_lgrp->notif_work);
	kfifo_free(&ctrl_lgrp->ctrl_notif.fifo);

	spin_lock(&ctrl_lgrps_lock);
	ctrl_lgrps[ldev_num][lgrp_num] = NULL;
	spin_unlock(&ctrl_lgrps_lock);

	complete_all(&ctrl_lgrp->del_complete);

	kfree(ctrl_lgrp);
}

int sl_ctrl_lgrp_put(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	return kref_put(&ctrl_lgrp->ref_cnt, sl_ctrl_lgrp_release);
}

int sl_ctrl_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	unsigned long        timeleft;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err_trace(NULL, LOG_NAME,
			"del not found (ldev_num = %u, lgrp_num = %u)", ldev_num, lgrp_num);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "del (lgrp = 0x%p)", ctrl_lgrp);

	if (!sl_ctrl_lgrp_put(ctrl_lgrp)) {
		timeleft = wait_for_completion_timeout(&ctrl_lgrp->del_complete,
			msecs_to_jiffies(SL_CTRL_LGRP_DEL_TIMEOUT_MS));

		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME, "del timed out (ctrl_lgrp = 0x%p)", ctrl_lgrp);
			return -ETIMEDOUT;
		}
	}


	return 0;
}

bool sl_ctrl_lgrp_kref_get_unless_zero(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctrl_lgrp->ref_cnt) != 0);

	if (!incremented)
		sl_ctrl_log_warn(ctrl_lgrp, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return incremented;
}

struct sl_ctrl_lgrp *sl_ctrl_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;

	spin_lock(&ctrl_lgrps_lock);
	ctrl_lgrp = ctrl_lgrps[ldev_num][lgrp_num];
	spin_unlock(&ctrl_lgrps_lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "get (lgrp = 0x%p)", ctrl_lgrp);

	return ctrl_lgrp;
}

int sl_ctrl_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;
	unsigned long       irq_flags;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctrl_lgrp_kref_get_unless_zero(ctrl_lgrp)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctrl_lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	spin_lock_irqsave(&ctrl_lgrp->log_lock, irq_flags);
	strncpy(ctrl_lgrp->log_connect_id, connect_id, SL_LOG_CONNECT_ID_LEN);
	spin_unlock_irqrestore(&ctrl_lgrp->log_lock, irq_flags);

	sl_media_lgrp_connect_id_set(ldev_num, lgrp_num, connect_id);

	sl_core_lgrp_connect_id_set(ldev_num, lgrp_num, connect_id);

	if (sl_ctrl_lgrp_put(ctrl_lgrp))
		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "connect_id set - lgrp removed (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return 0;
}

int sl_ctrl_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config)
{
	int                 rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctrl_lgrp_kref_get_unless_zero(ctrl_lgrp)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctrl_lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "config set:");
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  mfs       = %d",   lgrp_config->mfs);
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  furcation = 0x%X", lgrp_config->furcation);
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  fec_mode  = 0x%X", lgrp_config->fec_mode);
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  tech_map  = 0x%X", lgrp_config->tech_map);
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  fec_map   = 0x%X", lgrp_config->fec_map);
	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "  options   = 0x%X", lgrp_config->options);

	rtn = sl_core_lgrp_config_set(ldev_num, lgrp_num, lgrp_config);
	if (rtn) {
		sl_ctrl_log_err_trace(ctrl_lgrp, LOG_NAME, "core_lgrp_config_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&(ctrl_lgrp->config_lock));
	ctrl_lgrp->config = *lgrp_config;
	spin_unlock(&(ctrl_lgrp->config_lock));

	rtn = 0;

out:
	if (sl_ctrl_lgrp_put(ctrl_lgrp))
		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "config set - lgrp removed (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return rtn;
}

int sl_ctrl_lgrp_config_get(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	spin_lock(&ctrl_lgrp->config_lock);
	*lgrp_config = ctrl_lgrp->config;
	spin_unlock(&ctrl_lgrp->config_lock);

	return 0;
}

int sl_ctrl_lgrp_policy_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_policy *lgrp_policy)
{
	struct sl_ctrl_lgrp *ctrl_lgrp;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctrl_lgrp_kref_get_unless_zero(ctrl_lgrp)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "kref_get_unless_zero failed (lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "policy set");

	spin_lock(&(ctrl_lgrp->config_lock));
	ctrl_lgrp->policy = *lgrp_policy;
	spin_unlock(&(ctrl_lgrp->config_lock));

	if (sl_ctrl_lgrp_put(ctrl_lgrp))
		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "policy set - lgrp removed (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return 0;
}
