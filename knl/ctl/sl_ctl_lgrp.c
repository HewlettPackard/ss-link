// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "sl_asic.h"
#include "sl_sysfs.h"

#include "base/sl_ctl_log.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_lgrp_notif.h"
#include "sl_ctl_link.h"
#include "sl_ctl_llr.h"
#include "sl_ctl_mac.h"
#include "sl_media_lgrp.h"
#include "sl_core_lgrp.h"

#define LOG_NAME SL_CTL_LGRP_LOG_NAME

#define SL_CTL_LGRP_DEL_TIMEOUT_MS 2000

static struct sl_ctl_lgrp *ctl_lgrps[SL_ASIC_MAX_LDEVS][SL_ASIC_MAX_LGRPS];
static DEFINE_SPINLOCK(ctl_lgrps_lock);

int sl_ctl_lgrp_new(u8 ldev_num, u8 lgrp_num, struct kobject *sysfs_parent)
{
	int                   rtn;
	struct sl_ctl_ldev   *ctl_ldev;
	struct sl_ctl_lgrp   *ctl_lgrp;

	ctl_ldev = sl_ctl_ldev_get(ldev_num);
	if (!ctl_ldev) {
		sl_ctl_log_err(NULL, LOG_NAME, "missing ldev (ldev_num = %u)", ldev_num);
		return -EBADRQC;
	}

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (ctl_lgrp) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "exists (lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	ctl_lgrp = kzalloc(sizeof(struct sl_ctl_lgrp), GFP_KERNEL);
	if (!ctl_lgrp)
		return -ENOMEM;

	snprintf(ctl_lgrp->log_connect_id, sizeof(ctl_lgrp->log_connect_id), "ctl-lgrp%02u", lgrp_num);

	ctl_lgrp->magic    = SL_CTL_LGRP_MAGIC;
	ctl_lgrp->ver      = SL_CTL_LGRP_VER;
	ctl_lgrp->num      = lgrp_num;
	ctl_lgrp->ctl_ldev = sl_ctl_ldev_get(ldev_num);

	kref_init(&ctl_lgrp->ref_cnt);
	init_completion(&ctl_lgrp->del_complete);

	spin_lock_init(&ctl_lgrp->data_lock);
	spin_lock_init(&ctl_lgrp->config_lock);
	spin_lock_init(&ctl_lgrp->log_lock);

	spin_lock_init(&(ctl_lgrp->ctl_notif.lock));

	rtn = kfifo_alloc(&ctl_lgrp->ctl_notif.fifo, SL_CTL_LGRP_NOTIF_FIFO_SIZE, GFP_KERNEL);
	if (rtn) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "create notif fifo failed");
		rtn = -ENOMEM;
		goto out_free;
	}
	INIT_WORK(&ctl_lgrp->notif_work, sl_ctl_lgrp_notif_work);

	rtn = sl_core_lgrp_new(ldev_num, lgrp_num);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_lgrp, LOG_NAME, "core_lgrp_new failed [%d]", rtn);
		goto out_kfifo_free;
	}

	rtn = sl_media_lgrp_new(ldev_num, lgrp_num);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_lgrp, LOG_NAME, "media_lgrp_new failed [%d]", rtn);
		goto out_core_lgrp;
	}

	if (sysfs_parent) {
		ctl_lgrp->parent_kobj = sysfs_parent;

		rtn = sl_sysfs_lgrp_create(ctl_lgrp);
		if (rtn) {
			sl_ctl_log_err_trace(ctl_lgrp, LOG_NAME,
				"sysfs_lgrp_create failed");
			goto out_core_lgrp;
		}
	}

	spin_lock(&ctl_lgrps_lock);
	ctl_lgrps[ldev_num][lgrp_num] = ctl_lgrp;
	spin_unlock(&ctl_lgrps_lock);

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "new (lgrp = 0x%p)", ctl_lgrp);

	return 0;

out_core_lgrp:
	sl_core_lgrp_del(ldev_num, lgrp_num);
out_kfifo_free:
	kfifo_free(&ctl_lgrp->ctl_notif.fifo);
out_free:
	kfree(ctl_lgrp);

	return rtn;
}

static void sl_ctl_lgrp_release(struct kref *ref)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	u8                  ldev_num;
	u8                  lgrp_num;
	u8                  link_num;

	ctl_lgrp = container_of(ref, struct sl_ctl_lgrp, ref_cnt);
	ldev_num = ctl_lgrp->ctl_ldev->num;
	lgrp_num = ctl_lgrp->num;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err_trace(NULL, LOG_NAME,
			"release lgrp not found (lgrp_num = %u)", lgrp_num);
		return;
	}

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "del (lgrp = 0x%p)", ctl_lgrp);

	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
		sl_ctl_link_del(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num, link_num);
		sl_ctl_llr_del(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num, link_num);
		sl_ctl_mac_del(ctl_lgrp->ctl_ldev->num, ctl_lgrp->num, link_num);
	}

	/* Must delete sysfs first to guarantee nobody is reading */
	sl_sysfs_lgrp_delete(ctl_lgrp);

	sl_core_lgrp_del(ldev_num, lgrp_num);
	sl_media_lgrp_del(ldev_num, lgrp_num);

	cancel_work_sync(&ctl_lgrp->notif_work);
	kfifo_free(&ctl_lgrp->ctl_notif.fifo);

	spin_lock(&ctl_lgrps_lock);
	ctl_lgrps[ldev_num][lgrp_num] = NULL;
	spin_unlock(&ctl_lgrps_lock);

	complete_all(&ctl_lgrp->del_complete);

	kfree(ctl_lgrp);
}

int sl_ctl_lgrp_put(struct sl_ctl_lgrp *ctl_lgrp)
{
	return kref_put(&ctl_lgrp->ref_cnt, sl_ctl_lgrp_release);
}

int sl_ctl_lgrp_del(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	unsigned long       timeleft;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err_trace(NULL, LOG_NAME,
			"del not found (ldev_num = %u, lgrp_num = %u)", ldev_num, lgrp_num);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "del (lgrp = 0x%p)", ctl_lgrp);

	if (!sl_ctl_lgrp_put(ctl_lgrp)) {
		timeleft = wait_for_completion_timeout(&ctl_lgrp->del_complete,
			msecs_to_jiffies(SL_CTL_LGRP_DEL_TIMEOUT_MS));

		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "del completion_timeout (timeleft = %lums)", timeleft);

		if (timeleft == 0) {
			sl_ctl_log_err_trace(ctl_lgrp, LOG_NAME, "del timed out (ctl_lgrp = 0x%p)", ctl_lgrp);
			return -ETIMEDOUT;
		}
	}


	return 0;
}

bool sl_ctl_lgrp_kref_get_unless_zero(struct sl_ctl_lgrp *ctl_lgrp)
{
	bool incremented;

	incremented = (kref_get_unless_zero(&ctl_lgrp->ref_cnt) != 0);

	if (!incremented)
		sl_ctl_log_warn(ctl_lgrp, LOG_NAME,
			"kref_get_unless_zero ref unavailable (ctl_lgrp = 0x%p)", ctl_lgrp);

	return incremented;
}

struct sl_ctl_lgrp *sl_ctl_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	spin_lock(&ctl_lgrps_lock);
	ctl_lgrp = ctl_lgrps[ldev_num][lgrp_num];
	spin_unlock(&ctl_lgrps_lock);

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "get (lgrp = 0x%p)", ctl_lgrp);

	return ctl_lgrp;
}

int sl_ctl_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	unsigned long       irq_flags;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctl_lgrp_kref_get_unless_zero(ctl_lgrp)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctl_lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	spin_lock_irqsave(&ctl_lgrp->log_lock, irq_flags);
	strncpy(ctl_lgrp->log_connect_id, connect_id, SL_LOG_CONNECT_ID_LEN);
	spin_unlock_irqrestore(&ctl_lgrp->log_lock, irq_flags);

	sl_media_lgrp_connect_id_set(ldev_num, lgrp_num, connect_id);

	sl_core_lgrp_connect_id_set(ldev_num, lgrp_num, connect_id);

	if (sl_ctl_lgrp_put(ctl_lgrp))
		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "connect_id set - lgrp removed (ctl_lgrp = 0x%p)", ctl_lgrp);

	return 0;
}

int sl_ctl_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config)
{
	int                 rtn;
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctl_lgrp_kref_get_unless_zero(ctl_lgrp)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctl_lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "config set:");
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  mfs       = %d",   lgrp_config->mfs);
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  furcation = 0x%X", lgrp_config->furcation);
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  fec_mode  = 0x%X", lgrp_config->fec_mode);
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  tech_map  = 0x%X", lgrp_config->tech_map);
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  fec_map   = 0x%X", lgrp_config->fec_map);
	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "  options   = 0x%X", lgrp_config->options);

	rtn = sl_core_lgrp_config_set(ldev_num, lgrp_num, lgrp_config);
	if (rtn) {
		sl_ctl_log_err_trace(ctl_lgrp, LOG_NAME, "core_lgrp_config_set failed [%d]", rtn);
		goto out;
	}

	spin_lock(&(ctl_lgrp->config_lock));
	ctl_lgrp->config = *lgrp_config;
	spin_unlock(&(ctl_lgrp->config_lock));

	rtn = 0;

out:
	if (sl_ctl_lgrp_put(ctl_lgrp))
		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "config set - lgrp removed (ctl_lgrp = 0x%p)", ctl_lgrp);

	return rtn;
}

int sl_ctl_lgrp_policy_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_policy *lgrp_policy)
{
	struct sl_ctl_lgrp *ctl_lgrp;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "NULL lgrp");
		return -EBADRQC;
	}

	if (!sl_ctl_lgrp_kref_get_unless_zero(ctl_lgrp)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctl_lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "policy set");

	spin_lock(&(ctl_lgrp->config_lock));
	ctl_lgrp->policy = *lgrp_policy;
	spin_unlock(&(ctl_lgrp->config_lock));

	if (sl_ctl_lgrp_put(ctl_lgrp))
		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "policy set - lgrp removed (ctl_lgrp = 0x%p)", ctl_lgrp);

	return 0;
}
