/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_H_
#define _SL_CORE_HW_SERDES_H_

struct sl_core_link;
struct sl_core_lgrp;

int  sl_core_hw_serdes_start(struct sl_core_lgrp *core_lgrp, u32 clocking);

int  sl_core_hw_serdes_fw_load(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_fw_info_get(struct sl_core_lgrp *core_lgrp);

int  sl_core_hw_serdes_core_init(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_core_pll(struct sl_core_lgrp *core_lgrp, u32 clocking);

int  sl_core_hw_serdes_link_up_an(struct sl_core_link *core_link);
int  sl_core_hw_serdes_link_up(struct sl_core_link *core_link);

void sl_core_hw_serdes_link_down(struct sl_core_link *core_link);

void sl_core_hw_serdes_state_set(struct sl_core_link *core_link, u8 state);
u8   sl_core_hw_serdes_state_get(struct sl_core_link *core_link);

#endif /* _SL_CORE_HW_SERDES_H_ */
