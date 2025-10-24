/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LGRP_NOTIF_H_
#define _SL_CTRL_LGRP_NOTIF_H_

#include <linux/kfifo.h>

#include <linux/hpe/sl/sl_lgrp.h>
#include <linux/hpe/sl/sl_llr.h>

struct sl_ctrl_lgrp;

struct sl_ctrl_lgrp_notif_reg_entry {
	sl_lgrp_notif_t  callback;
	void            *tag;
	u32              types;
};

#define SL_CTRL_LGRP_NOTIF_FIFO_SIZE (8 * sizeof(struct sl_ctrl_lgrp_notif_reg_entry))
#define SL_CTRL_LGRP_NOTIF_COUNT     16

struct sl_ctrl_lgrp_notif {
	u8                                  list_state;
	struct sl_ctrl_lgrp_notif_reg_entry reg_entry[SL_CTRL_LGRP_NOTIF_COUNT];
	struct kfifo                        fifo;
	spinlock_t                          lock;
};

int  sl_ctrl_lgrp_notif_callback_reg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				     u32 types, void *tag);
int  sl_ctrl_lgrp_notif_callback_unreg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				       u32 types);

int  sl_ctrl_lgrp_notif_enqueue(struct sl_ctrl_lgrp *ctrl_lgrp, u8 link_num,
				u32 type, union sl_lgrp_notif_info *info, u64 info_map);

void sl_ctrl_lgrp_notif_work(struct work_struct *notif_work);

#endif /* _SL_CTRL_LGRP_NOTIF_H_ */
