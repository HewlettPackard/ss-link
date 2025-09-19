/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_SERDES_LINK_H_
#define _SL_CORE_HW_SERDES_LINK_H_

struct sl_core_link;

int  sl_core_hw_serdes_link_up_an(struct sl_core_link *core_link);
int  sl_core_hw_serdes_link_up(struct sl_core_link *core_link);
void sl_core_hw_serdes_link_down(struct sl_core_link *core_link);

#endif /* _SL_CORE_HW_SERDES_LINK_H_ */
