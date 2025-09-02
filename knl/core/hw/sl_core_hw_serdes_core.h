/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_CORE_H_
#define _SL_CORE_HW_SERDES_CORE_H_

struct sl_core_lgrp;

int  sl_core_hw_serdes_core_init(struct sl_core_lgrp *core_lgrp);
int  sl_core_hw_serdes_core_pll(struct sl_core_lgrp *core_lgrp, u16 clocking, u16 tx_pll_bw);

#endif /* _SL_CORE_HW_SERDES_CORE_H_ */
