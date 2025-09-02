/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_H_
#define _SL_CORE_HW_SERDES_H_

struct sl_core_link;
struct sl_core_lgrp;

#define SL_CORE_HW_SERDES_TX_PLL_BW_DEFAULT      0x30
#define SL_CORE_HW_SERDES_TX_PLL_BW_100Glane_AEC 0x24
#define SL_CORE_HW_SERDES_TX_PLL_BW_100Glane_AOC 0x00

#define LGRP_TO_SERDES(_lgrp_num) ((_lgrp_num) >> 1)

int  sl_core_hw_serdes_init(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_swizzles(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_hw_info_get(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_info_get(struct sl_core_lgrp *core_lgrp);
void sl_core_hw_serdes_state_set(struct sl_core_link *core_link, u8 state);
u8   sl_core_hw_serdes_state_get(struct sl_core_link *core_link);

#endif /* _SL_CORE_HW_SERDES_H_ */
