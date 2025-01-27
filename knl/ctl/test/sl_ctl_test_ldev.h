/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023-2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTL_TEST_LDEV_H_
#define _SL_CTL_TEST_LDEV_H_

struct kobject     *sl_ctl_test_ldev_kobj_get(u8 ldev_num);
struct sl_ctl_ldev *sl_ctl_test_ldev_get(u8 ldev_num);

#endif /* _SL_CTL_TEST_LDEV_H_ */
