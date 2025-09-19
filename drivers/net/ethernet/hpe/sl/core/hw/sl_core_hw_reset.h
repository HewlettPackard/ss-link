/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2022,2023 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_HW_RESET_H_
#define _SL_CORE_HW_RESET_H_

struct sl_core_link;
struct sl_core_lgrp;

void sl_core_hw_reset_link(struct sl_core_link *core_link);
void sl_core_hw_reset_lgrp(struct sl_core_lgrp *core_lgrp);

#endif /* _SL_CORE_HW_RESET_H_ */
