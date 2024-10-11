/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LDEV_H_
#define _SL_CORE_LDEV_H_

#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "sl_kconfig.h"
#include <linux/sl_ldev.h>
#include "sl_asic.h"

struct sl_accessors;
struct sl_ops;

#define SL_CORE_LDEV_MAGIC 0x73734c44
struct sl_core_ldev {
	u32                       magic;
	u8                        num;

	u8                        platform;
	u16                       revision;

	struct sl_accessors       accessors;
	struct sl_ops             ops;

	spinlock_t                data_lock;

	struct workqueue_struct  *workqueue;
};

int		     sl_core_ldev_new(u8 ldev_num,
				      struct sl_accessors *assessors,
				      struct sl_ops *ops,
				      struct workqueue_struct *workqueue);
void		     sl_core_ldev_del(u8 ldev_num);
struct sl_core_ldev *sl_core_ldev_get(u8 ldev_num);

#endif /* _SL_CORE_LDEV_H_ */
