/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_LDEV_H_
#define _SL_CORE_DATA_LDEV_H_

struct sl_accessors;
struct sl_ops;
struct sl_core_ldev;
struct workqueue_struct;

int		     sl_core_data_ldev_new(u8 ldev_num,
					   struct sl_accessors *accessors,
					   struct sl_ops *ops,
					   struct workqueue_struct *workqueue);
void		     sl_core_data_ldev_del(u8 ldev_num);
struct sl_core_ldev *sl_core_data_ldev_get(u8 ldev_num);

#endif /* _SL_CORE_DATA_LDEV_H_ */
