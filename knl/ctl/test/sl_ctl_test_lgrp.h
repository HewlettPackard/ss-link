/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023-2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_TEST_LGRP_H_
#define _SL_CTL_TEST_LGRP_H_

struct kobject     *sl_ctl_test_lgrp_kobj_get(u8 ldev_num, u8 lgrp_num);
struct sl_ctl_lgrp *sl_ctl_test_lgrp_get(u8 ldev_num, u8 lgrp_num);

#endif /* _SL_CTL_TEST_LGRP_H_ */
