/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_MAC_H_
#define _SL_CORE_MAC_H_

#include <linux/spinlock.h>

struct sl_core_lgrp;

enum sl_core_mac_state {
	SL_CORE_MAC_STATE_INVALID = 0,
	SL_CORE_MAC_STATE_OFF,
	SL_CORE_MAC_STATE_ON,
};

#define SL_CORE_MAC_MAGIC 0x736d4C5E
struct sl_core_mac {
	u32                              magic;
	u8                               num;

	struct sl_core_lgrp             *core_lgrp;

	spinlock_t                       data_lock;
	u64                              info_map;

	u32             tx_state;
	u32             rx_state;

	struct {
		u8      tx_ifg_adj;
		u8      tx_ifg_mode;
		u8      tx_pad_idle_thresh;
		u8      tx_idle_delay;
		u8      tx_priority_thresh;
		u8      tx_cdt_thresh_2;
		u8      tx_cdt_thresh;
		u8      tx_cdt_init_val;
		u8      tx_pcs_credits;
		u8      tx_short_preamble;
		u8      rx_flit_packing_cnt;
		u8      rx_short_preamble;
		u8      llr_if_credits;
	} settings;
};

int                 sl_core_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num);
void                sl_core_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num);
struct sl_core_mac *sl_core_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int sl_core_mac_tx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_core_mac_tx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_core_mac_tx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

int sl_core_mac_rx_start(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_core_mac_rx_stop(u8 ldev_num, u8 lgrp_num, u8 mac_num);
int sl_core_mac_rx_state_get(u8 ldev_num, u8 lgrp_num, u8 mac_num, u32 *state);

#endif /* _SL_CORE_MAC_H_ */
