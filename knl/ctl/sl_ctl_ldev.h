/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_LDEV_H_
#define _SL_CTL_LDEV_H_

#include <linux/kobject.h>
#include <linux/workqueue.h>
#include <linux/sl_ldev.h>

#define SL_CTL_LDEV_MAGIC 0x736c6382
#define SL_CTL_LDEV_VER   2
struct sl_ctl_ldev {
	u32                      magic;
	u32                      ver;

	u8                       num;

	struct sl_ldev_attr      attr;
	struct workqueue_struct *workq;
	bool                     create_workq;
	u64                      lgrp_map;

	struct kobject          *parent_kobj;
	struct kobject           sl_info_kobj;

	spinlock_t               data_lock;
	bool                     is_deleting;
};

int		    sl_ctl_ldev_new(u8 ldev_num, u64 lgrp_map,
				    struct workqueue_struct *workq,
				    struct sl_ldev_attr *ldev_attr,
				    struct kobject *sysfs_parent);
void		    sl_ctl_ldev_del(u8 ldev_num);
struct sl_ctl_ldev *sl_ctl_ldev_get(u8 ldev_num);

void		    sl_ctl_ldev_exit(void);

#endif /* _SL_CTL_LDEV_H_ */
