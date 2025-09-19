/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_AN_LP_H_
#define _SL_CORE_HW_AN_LP_H_

#include "sl_core_link_an.h"

struct sl_core_link;
struct sl_link_caps;
struct work_struct;

void sl_core_hw_an_lp_caps_get_cmd(struct sl_core_link *core_link, u32 link_state,
				   sl_core_link_an_callback_t callback, void *tag,
				   struct sl_link_caps *caps, u32 timeout_ms, u32 flags);
void sl_core_hw_an_lp_caps_stop_cmd(struct sl_core_link *core_link);

void sl_core_hw_an_lp_caps_get_work(struct work_struct *work);
void sl_core_hw_an_lp_caps_get_timeout_work(struct work_struct *work);
void sl_core_hw_an_lp_caps_get_done_work(struct work_struct *work);

#endif /* _SL_CORE_HW_AN_LP_H_ */
