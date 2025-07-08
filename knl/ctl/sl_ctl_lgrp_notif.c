// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>

#include "base/sl_ctl_log.h"
#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_lgrp_notif.h"
#include "sl_media_lgrp.h"

#define LOG_NAME SL_CTL_LGRP_NOTIF_LOG_NAME

#define SL_CTL_LGRP_NOTIF_LIST_STATE_IDLE    0
#define SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING 1

static int sl_ctl_lgrp_notif_list_state_get(struct sl_ctl_lgrp *ctl_lgrp)
{
	int notif_list_state;

	spin_lock(&ctl_lgrp->ctl_notif.lock);
	notif_list_state = ctl_lgrp->ctl_notif.list_state;
	spin_unlock(&ctl_lgrp->ctl_notif.lock);

	return notif_list_state;
}

int sl_ctl_lgrp_notif_callback_reg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				   u32 types, void *tag)
{
	int                   rtn;
	struct sl_ctl_lgrp   *ctl_lgrp;
	u8                    reg_idx;
	u8                    counter;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "NULL ctl lgrp");
		return -EBADRQC;
	}

	if (!sl_ctl_lgrp_kref_get_unless_zero(ctl_lgrp)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctl_lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME,
		"notif callback reg (types = 0x%X, tag = 0x%p)",
		types, tag);

	counter = 0;
	while (sl_ctl_lgrp_notif_list_state_get(ctl_lgrp) == SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING) {
		msleep(20);
		counter++;
		if (counter >= 10) {
			sl_ctl_log_err(ctl_lgrp, LOG_NAME, "registration timed out");
			rtn = -ETIMEDOUT;
			goto out;
		}
	}

	spin_lock(&ctl_lgrp->ctl_notif.lock);
	if (ctl_lgrp->ctl_notif.list_state == SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING) {
		spin_unlock(&ctl_lgrp->ctl_notif.lock);
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "notif list is currently being used");
		rtn = -EBUSY;
		goto out;
	}
	for (reg_idx = 0; reg_idx < ARRAY_SIZE(ctl_lgrp->ctl_notif.reg_entry); ++reg_idx) {
		if (ctl_lgrp->ctl_notif.reg_entry[reg_idx].types == 0) {
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].callback = callback;
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].tag = tag;
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].types = types;
			break;
		}
	}
	spin_unlock(&ctl_lgrp->ctl_notif.lock);

	if (reg_idx >= ARRAY_SIZE(ctl_lgrp->ctl_notif.reg_entry)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "registration list is full");
		rtn = -ENOSPC;
		goto out;
	}

	if (types & SL_LGRP_NOTIF_MEDIA_PRESENT)
		sl_media_lgrp_real_cable_if_present_send(ldev_num, lgrp_num);

	if (types & SL_LGRP_NOTIF_MEDIA_NOT_PRESENT)
		sl_media_lgrp_real_cable_if_not_present_send(ldev_num, lgrp_num);

	rtn = 0;
out:
	if (sl_ctl_lgrp_put(ctl_lgrp))
		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "notif callback reg - lgrp removed (ctl_lgrp = 0x%p)", ctl_lgrp);

	return rtn;
}

int sl_ctl_lgrp_notif_callback_unreg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				     u32 types)
{
	int                 rtn;
	struct sl_ctl_lgrp *ctl_lgrp;
	u8                  reg_idx;
	u8                  counter;

	ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);
	if (!ctl_lgrp) {
		sl_ctl_log_err(NULL, LOG_NAME, "NULL ctl lgrp");
		return -EBADRQC;
	}

	if (!sl_ctl_lgrp_kref_get_unless_zero(ctl_lgrp)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctl_lgrp = 0x%p)", ctl_lgrp);
		return -EBADRQC;
	}

	counter = 0;
	while (sl_ctl_lgrp_notif_list_state_get(ctl_lgrp) == SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING) {
		msleep(20);
		counter++;
		if (counter >= 10) {
			sl_ctl_log_err(ctl_lgrp, LOG_NAME, "unregistration timed out");
			rtn = -ETIMEDOUT;
			goto out;
		}
	}

	spin_lock(&ctl_lgrp->ctl_notif.lock);
	if (ctl_lgrp->ctl_notif.list_state == SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING) {
		spin_unlock(&ctl_lgrp->ctl_notif.lock);
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "notif list is currently being used");
		rtn = -EBUSY;
		goto out;
	}
	for (reg_idx = 0; reg_idx < ARRAY_SIZE(ctl_lgrp->ctl_notif.reg_entry); ++reg_idx) {
		if ((ctl_lgrp->ctl_notif.reg_entry[reg_idx].types == types) &&
				(ctl_lgrp->ctl_notif.reg_entry[reg_idx].callback == callback)) {
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].callback = NULL;
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].tag = NULL;
			ctl_lgrp->ctl_notif.reg_entry[reg_idx].types = 0;
			break;
		}
	}
	spin_unlock(&ctl_lgrp->ctl_notif.lock);

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "notif callback unreg (types = 0x%X)", types);

	rtn = 0;

out:
	if (sl_ctl_lgrp_put(ctl_lgrp))
		sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "notif callback unreg - lgrp removed (ctl_lgrp = 0x%p)", ctl_lgrp);

	return rtn;
}

int sl_ctl_lgrp_notif_enqueue(struct sl_ctl_lgrp *ctl_lgrp, u8 link_num,
			      u32 type, union sl_lgrp_notif_info *info, u64 info_map)
{
	struct sl_lgrp_notif_msg notif_msg;
	int                      rtn;

	if (!ctl_lgrp)
		return 0;

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME,
		"notif enqueue (link_num = %d, type = 0x%X)", link_num, type);

	spin_lock(&ctl_lgrp->ctl_notif.lock);
	if (kfifo_is_full(&ctl_lgrp->ctl_notif.fifo)) {
		spin_unlock(&ctl_lgrp->ctl_notif.lock);
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "notification fifo is full");
		return -ENOSPC;
	}
	spin_unlock(&ctl_lgrp->ctl_notif.lock);

	if (info)
		notif_msg.info = *info;
	else
		memset(&notif_msg.info, 0, sizeof(notif_msg.info));

	notif_msg.ldev_num = ctl_lgrp->ctl_ldev->num;
	notif_msg.lgrp_num = ctl_lgrp->num;
	notif_msg.link_num = link_num;
	notif_msg.type     = type;
	notif_msg.info_map = info_map;

	rtn = kfifo_in_spinlocked(&ctl_lgrp->ctl_notif.fifo, &notif_msg, sizeof(notif_msg),
			&ctl_lgrp->ctl_notif.lock);
	if (rtn != sizeof(notif_msg)) {
		sl_ctl_log_err(ctl_lgrp, LOG_NAME, "write to kfifo failed");
		return -EFAULT;
	}

	/* Don't catch the return as it might already be running.  This is ok */
	queue_work(ctl_lgrp->ctl_ldev->notif_workq, &ctl_lgrp->notif_work);

	return 0;
}

void sl_ctl_lgrp_notif_work(struct work_struct *notif_work)
{
	struct sl_ctl_lgrp       *ctl_lgrp;
	struct sl_lgrp_notif_msg  notif_msg;
	u8                        reg_idx;
	int                       rtn;
	bool                      is_empty;

	ctl_lgrp = container_of(notif_work, struct sl_ctl_lgrp, notif_work);

	sl_ctl_log_dbg(ctl_lgrp, LOG_NAME, "notif work");

	spin_lock(&ctl_lgrp->ctl_notif.lock);
	ctl_lgrp->ctl_notif.list_state = SL_CTL_LGRP_NOTIF_LIST_STATE_SENDING;
	spin_unlock(&ctl_lgrp->ctl_notif.lock);

	do {
		// FIXME: add cancel here
		rtn = kfifo_out_spinlocked(&ctl_lgrp->ctl_notif.fifo,
					   &notif_msg, sizeof(notif_msg),
					   &ctl_lgrp->ctl_notif.lock);
		if (rtn != sizeof(notif_msg)) {
			sl_ctl_log_err(ctl_lgrp, LOG_NAME, "read from kfifo failed");
			goto out;
		}
		for (reg_idx = 0; reg_idx < SL_CTL_LGRP_NOTIF_COUNT; ++reg_idx) {
			if (notif_msg.type & ctl_lgrp->ctl_notif.reg_entry[reg_idx].types)
				(ctl_lgrp->ctl_notif.reg_entry[reg_idx].callback)
				(ctl_lgrp->ctl_notif.reg_entry[reg_idx].tag, &notif_msg);
		}
		spin_lock(&ctl_lgrp->ctl_notif.lock);
		is_empty = kfifo_is_empty(&ctl_lgrp->ctl_notif.fifo);
		spin_unlock(&ctl_lgrp->ctl_notif.lock);
	} while (!is_empty);

out:
	spin_lock(&ctl_lgrp->ctl_notif.lock);
	ctl_lgrp->ctl_notif.list_state = SL_CTL_LGRP_NOTIF_LIST_STATE_IDLE;
	spin_unlock(&ctl_lgrp->ctl_notif.lock);
}
