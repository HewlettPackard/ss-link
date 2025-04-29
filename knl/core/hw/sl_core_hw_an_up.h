/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_AN_UP_H_
#define _SL_CORE_HW_AN_UP_H_

struct sl_core_link;
struct work_struct;

void sl_core_hw_an_up_start_work(struct work_struct *work);

void sl_core_hw_an_up_work(struct work_struct *work);
void sl_core_hw_an_up_done_work(struct work_struct *work);

#endif /* _SL_CORE_HW_AN_UP_H_ */
