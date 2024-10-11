/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_MAC_H_
#define _SL_CTL_MAC_H_

#include <linux/spinlock.h>
#include <linux/kobject.h>

struct sl_ctl_lgrp;

#define SL_CTL_MAC_MAGIC 0x646c6c79
#define SL_CTL_MAC_VER   1
struct sl_ctl_mac {
	u32                 magic;
	u32                 ver;

	u8                  num;

	spinlock_t          data_lock;

	struct sl_ctl_lgrp *ctl_lgrp;

	struct kobject     *parent_kobj;
	struct kobject      kobj;

	bool                is_deleting;
};

int		   sl_ctl_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num, struct kobject *sysfs_parent);
void		   sl_ctl_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num);
struct sl_ctl_mac *sl_ctl_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int sl_ctl_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctl_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctl_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

int sl_ctl_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctl_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_ctl_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

int sl_ctl_mac_reset(u8 ldev_num, u8 lgrp_num, u8 mac_num);

#endif /* _SL_CTL_MAC_H_ */
