/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LDEV_H_
#define _SL_CORE_LDEV_H_

#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/sl_ldev.h>

#include "sl_kconfig.h"
#include "sl_asic.h"

struct sl_accessors;
struct sl_ops;

struct sl_serdes_fw_info {
	u32 signature;
	u8  version;
	u32 lane_static_var_ram_base;
	u32 lane_static_var_ram_size;
	u32 lane_var_ram_base;
	u16 lane_var_ram_size;
	u16 grp_ram_size;
	u8  lane_count;
};

struct sl_serdes_hw_info {
	u8 num_cores;
	u8 rev_id_1;
	u8 rev_id_2;
	u8 version;
	u8 num_micros;
	u8 num_lanes;
	u8 num_plls;
};

#define SL_CORE_LDEV_MAGIC 0x73734c44
struct sl_core_ldev {
	u32                       magic;
	u8                        num;

	u8                        platform;
	u16                       revision;

	struct sl_accessors       accessors;
	struct sl_ops             ops;

	spinlock_t                data_lock;

	struct {
		const struct firmware   *fw;
		struct sl_serdes_hw_info hw_info[SL_ASIC_MAX_SERDES];
		struct sl_serdes_fw_info fw_info[SL_ASIC_MAX_SERDES];
		bool                     is_ready;
	} serdes;

	struct workqueue_struct  *workqueue;
};

int                  sl_core_ldev_new(u8 ldev_num,
				      struct sl_accessors *assessors,
				      struct sl_ops *ops,
				      struct workqueue_struct *workqueue);
int                  sl_core_ldev_serdes_init(u8 ldev_num);
void                 sl_core_ldev_del(u8 ldev_num);
struct sl_core_ldev *sl_core_ldev_get(u8 ldev_num);

void sl_core_ldev_serdes_is_ready_set(struct sl_core_ldev *core_ldev, bool ready);
bool sl_core_ldev_serdes_is_ready(struct sl_core_ldev *core_ldev);

#endif /* _SL_CORE_LDEV_H_ */
