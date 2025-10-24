// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_ldev.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"
#include "sl_media_lgrp.h"

#define LOG_NAME SL_CTRL_LGRP_NOTIF_LOG_NAME

#define SL_CTRL_LGRP_NOTIF_LIST_STATE_IDLE    0
#define SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING 1

static int sl_ctrl_lgrp_notif_list_state_get(struct sl_ctrl_lgrp *ctrl_lgrp)
{
	int notif_list_state;

	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	notif_list_state = ctrl_lgrp->ctrl_notif.list_state;
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);

	return notif_list_state;
}

int sl_ctrl_lgrp_notif_callback_reg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				   u32 types, void *tag)
{
	int                   rtn;
	struct sl_ctrl_lgrp  *ctrl_lgrp;
	u8                    reg_idx;
	u8                    counter;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL ctrl_lgrp");
		return -EBADRQC;
	}

	if (!sl_ctrl_lgrp_kref_get_unless_zero(ctrl_lgrp)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctrl_lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"notif callback reg (types = 0x%X, tag = 0x%p)",
			types, tag);

	counter = 0;
	while (sl_ctrl_lgrp_notif_list_state_get(ctrl_lgrp) == SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING) {
		msleep(20);
		counter++;
		if (counter >= 10) {
			sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "registration timed out");
			rtn = -ETIMEDOUT;
			goto out;
		}
	}

	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	if (ctrl_lgrp->ctrl_notif.list_state == SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING) {
		spin_unlock(&ctrl_lgrp->ctrl_notif.lock);
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "notif list is currently being used");
		rtn = -EBUSY;
		goto out;
	}
	for (reg_idx = 0; reg_idx < ARRAY_SIZE(ctrl_lgrp->ctrl_notif.reg_entry); ++reg_idx) {
		if (ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].types == 0) {
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].callback = callback;
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].tag = tag;
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].types = types;
			break;
		}
	}
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);

	if (reg_idx >= ARRAY_SIZE(ctrl_lgrp->ctrl_notif.reg_entry)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "registration list is full");
		rtn = -ENOSPC;
		goto out;
	}

	if (types & SL_LGRP_NOTIF_MEDIA_PRESENT)
		sl_media_lgrp_real_cable_if_present_send(ldev_num, lgrp_num);

	if (types & SL_LGRP_NOTIF_MEDIA_NOT_PRESENT)
		sl_media_lgrp_real_cable_if_not_present_send(ldev_num, lgrp_num);

	rtn = 0;
out:
	if (sl_ctrl_lgrp_put(ctrl_lgrp))
		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "notif callback reg - lgrp removed (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return rtn;
}

int sl_ctrl_lgrp_notif_callback_unreg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				     u32 types)
{
	int                  rtn;
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u8                   reg_idx;
	u8                   counter;

	ctrl_lgrp = sl_ctrl_lgrp_get(ldev_num, lgrp_num);
	if (!ctrl_lgrp) {
		sl_ctrl_log_err(NULL, LOG_NAME, "NULL ctrl_lgrp");
		return -EBADRQC;
	}

	if (!sl_ctrl_lgrp_kref_get_unless_zero(ctrl_lgrp)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "kref_get_unless_zero failed (ctrl_lgrp = 0x%p)", ctrl_lgrp);
		return -EBADRQC;
	}

	counter = 0;
	while (sl_ctrl_lgrp_notif_list_state_get(ctrl_lgrp) == SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING) {
		msleep(20);
		counter++;
		if (counter >= 10) {
			sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "unregistration timed out");
			rtn = -ETIMEDOUT;
			goto out;
		}
	}

	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	if (ctrl_lgrp->ctrl_notif.list_state == SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING) {
		spin_unlock(&ctrl_lgrp->ctrl_notif.lock);
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "notif list is currently being used");
		rtn = -EBUSY;
		goto out;
	}
	for (reg_idx = 0; reg_idx < ARRAY_SIZE(ctrl_lgrp->ctrl_notif.reg_entry); ++reg_idx) {
		if ((ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].types == types) &&
			(ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].callback == callback)) {
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].callback = NULL;
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].tag = NULL;
			ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].types = 0;
			break;
		}
	}
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "notif callback unreg (types = 0x%X)", types);

	rtn = 0;

out:
	if (sl_ctrl_lgrp_put(ctrl_lgrp))
		sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
			"notif callback unreg - lgrp removed (ctrl_lgrp = 0x%p)", ctrl_lgrp);

	return rtn;
}

int sl_ctrl_lgrp_notif_enqueue(struct sl_ctrl_lgrp *ctrl_lgrp, u8 link_num,
			       u32 type, union sl_lgrp_notif_info *info, u64 info_map)
{
	struct sl_lgrp_notif_msg notif_msg;
	int                      rtn;

	if (!ctrl_lgrp)
		return 0;

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME,
		"notif enqueue (link_num = %d, type = 0x%X)", link_num, type);

	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	if (kfifo_is_full(&ctrl_lgrp->ctrl_notif.fifo)) {
		spin_unlock(&ctrl_lgrp->ctrl_notif.lock);
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "notification fifo is full");
		return -ENOSPC;
	}
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);

	if (info)
		notif_msg.info = *info;
	else
		memset(&notif_msg.info, 0, sizeof(notif_msg.info));

	notif_msg.ldev_num = ctrl_lgrp->ctrl_ldev->num;
	notif_msg.lgrp_num = ctrl_lgrp->num;
	notif_msg.link_num = link_num;
	notif_msg.type     = type;
	notif_msg.info_map = info_map;

	rtn = kfifo_in_spinlocked(&ctrl_lgrp->ctrl_notif.fifo, &notif_msg, sizeof(notif_msg),
				  &ctrl_lgrp->ctrl_notif.lock);
	if (rtn != sizeof(notif_msg)) {
		sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "write to kfifo failed");
		return -EFAULT;
	}

	/* Don't catch the return as it might already be running.  This is ok */
	queue_work(ctrl_lgrp->ctrl_ldev->notif_workq, &ctrl_lgrp->notif_work);

	return 0;
}

void sl_ctrl_lgrp_notif_work(struct work_struct *notif_work)
{
	struct sl_ctrl_lgrp      *ctrl_lgrp;
	struct sl_lgrp_notif_msg  notif_msg;
	u8                        reg_idx;
	int                       rtn;
	bool                      is_empty;

	ctrl_lgrp = container_of(notif_work, struct sl_ctrl_lgrp, notif_work);

	sl_ctrl_log_dbg(ctrl_lgrp, LOG_NAME, "notif work");

	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	ctrl_lgrp->ctrl_notif.list_state = SL_CTRL_LGRP_NOTIF_LIST_STATE_SENDING;
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);

	do {
		// FIXME: add cancel here
		rtn = kfifo_out_spinlocked(&ctrl_lgrp->ctrl_notif.fifo,
					   &notif_msg, sizeof(notif_msg),
					   &ctrl_lgrp->ctrl_notif.lock);
		if (rtn != sizeof(notif_msg)) {
			sl_ctrl_log_err(ctrl_lgrp, LOG_NAME, "read from kfifo failed");
			goto out;
		}
		for (reg_idx = 0; reg_idx < SL_CTRL_LGRP_NOTIF_COUNT; ++reg_idx) {
			if (notif_msg.type & ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].types)
				(ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].callback)
				(ctrl_lgrp->ctrl_notif.reg_entry[reg_idx].tag, &notif_msg);
		}
		spin_lock(&ctrl_lgrp->ctrl_notif.lock);
		is_empty = kfifo_is_empty(&ctrl_lgrp->ctrl_notif.fifo);
		spin_unlock(&ctrl_lgrp->ctrl_notif.lock);
	} while (!is_empty);

out:
	spin_lock(&ctrl_lgrp->ctrl_notif.lock);
	ctrl_lgrp->ctrl_notif.list_state = SL_CTRL_LGRP_NOTIF_LIST_STATE_IDLE;
	spin_unlock(&ctrl_lgrp->ctrl_notif.lock);
}
