/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LGRP_NOTIF_H_
#define _SL_CTL_LGRP_NOTIF_H_

#include <linux/kfifo.h>
#include <linux/sl_lgrp.h>
#include <linux/sl_llr.h>

#include "uapi/sl_lgrp.h"
#include "uapi/sl_media.h"

struct sl_lgrp_notif_info_link_up;
struct sl_lgrp_notif_info_link_up_fail;
struct sl_lgrp_notif_info_link_async_down;
struct sl_ctl_lgrp;

struct sl_lgrp_notif_info_cache {
	union {
		struct sl_lgrp_notif_info_link_up         link_up;
		struct sl_lgrp_notif_info_link_up_fail    link_up_fail;
		struct sl_lgrp_notif_info_link_async_down link_async_down;
		struct sl_llr_data                        llr_data;
		struct sl_media_attr                      media_attr;
		int                                       error;
	};
};

struct sl_ctl_lgrp_notif_reg_entry {
	sl_lgrp_notif_t           callback;
	void                     *tag;
	u32                       types;
};

#define SL_CTL_LGRP_NOTIF_FIFO_SIZE (8 * sizeof(struct sl_ctl_lgrp_notif_reg_entry))
#define SL_CTL_LGRP_NOTIF_COUNT     16

struct sl_ctl_lgrp_notif {
	u8                                 list_state;
	struct sl_ctl_lgrp_notif_reg_entry reg_entry[SL_CTL_LGRP_NOTIF_COUNT];
	struct kfifo                       fifo;
	struct kmem_cache                 *info_cache;
	spinlock_t                         lock;
};

int  sl_ctl_lgrp_notif_callback_reg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				    u32 types, void *tag);
int  sl_ctl_lgrp_notif_callback_unreg(u8 ldev_num, u8 lgrp_num, sl_lgrp_notif_t callback,
				      u32 types);

int  sl_ctl_lgrp_notif_enqueue(struct sl_ctl_lgrp *ctl_lgrp, u8 link_num,
			       u32 type, void *info, int info_size, u64 info_map);
void sl_ctl_lgrp_notif_info_free(u8 ldev_num, u8 lgrp_num, void *info);

void sl_ctl_lgrp_notif_work(struct work_struct *notif_work);

#endif /* _SL_CTL_LGRP_NOTIF_H_ */
