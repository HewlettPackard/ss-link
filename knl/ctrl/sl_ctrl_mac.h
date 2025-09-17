/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_MAC_H_
#define _SL_CTRL_MAC_H_

#include "linux/kref.h"
#include <linux/spinlock.h>
#include <linux/kobject.h>

struct sl_ctrl_lgrp;

#define SL_CTRL_MAC_MAGIC 0x646c6c79
#define SL_CTRL_MAC_VER   1
struct sl_ctrl_mac {
	u32                  magic;
	u32                  ver;

	u8                   num;

	spinlock_t           data_lock;

	struct sl_ctrl_lgrp *ctrl_lgrp;

	struct kobject      *parent_kobj;
	struct kobject       kobj;

	struct kref          ref_cnt;
	struct completion    del_complete;
};

int		    sl_ctrl_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num, struct kobject *sysfs_parent);
int		    sl_ctrl_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num);
struct sl_ctrl_mac *sl_ctrl_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int sl_ctrl_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctrl_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctrl_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

int sl_ctrl_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctrl_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctrl_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

int sl_ctrl_mac_reset(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int sl_ctrl_mac_info_map_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u64 *info_map);

#endif /* _SL_CTRL_MAC_H_ */
