/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_PDEV_H_
#define _SL_TEST_PDEV_H_

#include <linux/types.h>

struct kobject;

int  sl_test_pdev_create(void);
void sl_test_pdev_remove(void);

int  sl_test_pdev_pgrp_add(u8 test_pdev_num, u8 pgrp_num);

struct kobject *sl_test_pdev_port_kobj_get(u8 test_pdev_num, u8 pgrp_num, u8 port_num);
int             sl_test_pdev_port_kobj_put(u8 test_pdev_num, u8 pgrp_num, u8 port_num);

#endif /* _SL_TEST_PDEV_H_ */
