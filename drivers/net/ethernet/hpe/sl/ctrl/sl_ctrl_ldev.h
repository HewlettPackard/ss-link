/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LDEV_H_
#define _SL_CTRL_LDEV_H_

#include <linux/kobject.h>
#include <linux/workqueue.h>
#include <linux/completion.h>

#include <linux/hpe/sl/sl_ldev.h>

#include "data/sl_media_data_cable_db.h"

#define SL_CTRL_LDEV_MAGIC 0x736c6382
#define SL_CTRL_LDEV_VER   2

enum sl_cable_types {
	SL_CABLE_TYPE_PEC,
	SL_CABLE_TYPE_AOC,
	SL_CABLE_TYPE_AEC,
	SL_CABLE_TYPE_POC,
	SL_CABLE_TYPES_NUM,
};

#define SL_CABLE_VENDORS_NUM 11

struct sl_ctrl_ldev_cable_hpe_pn_kobj {
	struct sl_ctrl_ldev *ctrl_ldev;
	u32                  cable_idx;
	struct kobject       kobj;
};

struct sl_ctrl_ldev {
	u32                       magic;
	u32                       ver;

	u8                        num;

	struct sl_ldev_attr       attr;
	struct workqueue_struct  *workq;
	struct workqueue_struct  *notif_workq;
	bool                      create_workq;
	u64                       lgrp_map;

	struct kobject           *parent_kobj;
	struct kobject            sl_info_kobj;

	spinlock_t                data_lock;

	struct kobject            supported_cables_kobj;
	struct kobject            cable_types_kobj[SL_CABLE_TYPES_NUM];
	struct kobject            cable_vendors_kobj[SL_CABLE_TYPES_NUM][SL_CABLE_VENDORS_NUM];
	struct sl_ctrl_ldev_cable_hpe_pn_kobj cable_hpe_pns_kobj[ARRAY_SIZE(cable_db)];

	struct kref               ref_cnt;
	struct completion         del_complete;
};

int                  sl_ctrl_ldev_new(u8 ldev_num,
				      struct workqueue_struct *workq,
				      struct sl_ldev_attr *ldev_attr);
int                  sl_ctrl_ldev_serdes_init(u8 ldev_num);
int                  sl_ctrl_ldev_del(u8 ldev_num);
bool                 sl_ctrl_ldev_kref_get_unless_zero(struct sl_ctrl_ldev *ctrl_ldev);
int                  sl_ctrl_ldev_put(struct sl_ctrl_ldev *ctrl_ldev);
struct sl_ctrl_ldev *sl_ctrl_ldev_get(u8 ldev_num);

void                 sl_ctrl_ldev_exit(void);

#endif /* _SL_CTRL_LDEV_H_ */
