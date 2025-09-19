/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_MAC_H_
#define _LINUX_SL_MAC_H_

#include <linux/kobject.h>

struct sl_lgrp;
struct sl_mac;

enum sl_mac_state {
	SL_MAC_STATE_OFF,
	SL_MAC_STATE_ON,
};

struct sl_mac *sl_mac_new(struct sl_lgrp *lgrp, u8 mac_num, struct kobject *sysfs_parent);
int            sl_mac_del(struct sl_mac *mac);

int sl_mac_tx_start(struct sl_mac *mac);
int sl_mac_tx_stop(struct sl_mac *mac);
int sl_mac_tx_state_get(struct sl_mac *mac, u32 *state);

int sl_mac_rx_start(struct sl_mac *mac);
int sl_mac_rx_stop(struct sl_mac *mac);
int sl_mac_rx_state_get(struct sl_mac *mac, u32 *state);

int sl_mac_reset(struct sl_mac *mac);

const char *sl_mac_state_str(u32 state);

#endif /* _LINUX_SL_MAC_H_ */
