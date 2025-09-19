/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LGRP_H_
#define _SL_TEST_DEBUGFS_LGRP_H_

int             sl_test_debugfs_lgrp_create(struct dentry *top_dir);
struct kobject *sl_test_port_sysfs_kobj_get(u8 lgrp_num);
void            sl_test_port_sysfs_exit(u8 ldev_num);

int             sl_test_lgrp_new(void);
int             sl_test_lgrp_del(void);

int             sl_test_lgrp_config_set(void);
int             sl_test_lgrp_policy_set(void);

int             sl_test_lgrp_notif_reg(void);
int             sl_test_lgrp_notif_unreg(void);

u8              sl_test_debugfs_lgrp_num_get(void);
struct sl_lgrp *sl_test_lgrp_get(void);

#endif /* _SL_TEST_DEBUGFS_LGRP_H_ */
