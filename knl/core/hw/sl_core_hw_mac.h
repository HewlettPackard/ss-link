/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_MAC_H_
#define _SL_CORE_HW_MAC_H_

struct sl_core_mac;

void sl_core_hw_mac_tx_config(struct sl_core_mac *core_mac);
void sl_core_hw_mac_tx_start(struct sl_core_mac *core_mac);
void sl_core_hw_mac_tx_stop(struct sl_core_mac *core_mac);
u64  sl_core_hw_mac_tx_state_get(struct sl_core_mac *core_mac);

void sl_core_hw_mac_rx_config(struct sl_core_mac *core_mac);
void sl_core_hw_mac_rx_start(struct sl_core_mac *core_mac);
void sl_core_hw_mac_rx_stop(struct sl_core_mac *core_mac);
u64  sl_core_hw_mac_rx_state_get(struct sl_core_mac *core_mac);

#endif /* _SL_CORE_HW_MAC_H_ */
