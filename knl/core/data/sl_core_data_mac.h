/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_DATA_MAC_H_
#define _SL_CORE_DATA_MAC_H_

struct sl_core_mac;

int                 sl_core_data_mac_new(u8 ldev_num, u8 lgrp_num, u8 mac_num);
void                sl_core_data_mac_del(u8 ldev_num, u8 lgrp_num, u8 mac_num);
struct sl_core_mac *sl_core_data_mac_get(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int  sl_core_data_mac_tx_settings(struct sl_core_mac *core_mac);
int  sl_core_data_mac_rx_settings(struct sl_core_mac *core_mac);

u32  sl_core_data_mac_tx_state_get(struct sl_core_mac *core_mac);
u32  sl_core_data_mac_rx_state_get(struct sl_core_mac *core_mac);

void sl_core_data_mac_info_map_clr(struct sl_core_mac *core_mac, u32 bit_num);
void sl_core_data_mac_info_map_set(struct sl_core_mac *core_mac, u32 bit_num);
u64  sl_core_data_mac_info_map_get(struct sl_core_mac *core_mac);

#endif /* _SL_CORE_DATA_MAC_H_ */
