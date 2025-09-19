/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LINK_H_
#define _SL_TEST_DEBUGFS_LINK_H_

int sl_test_debugfs_link_create(struct dentry *top_dir);

int sl_test_port_num_entry_get_unless_zero(u8 lgrp_num, u8 link_num);
int sl_test_port_num_entry_put(u8 lgrp_num, u8 link_num);

void sl_test_link_remove(u8 ldev_num, u8 lgrp_num, u8 link_num);

int sl_test_link_new(void);
int sl_test_link_del(void);

u8  sl_test_debugfs_link_num_get(void);

int sl_test_link_up(void);
int sl_test_link_down(void);
int sl_test_link_config_set(void);
int sl_test_link_policy_set(void);
int sl_test_link_options_set(void);
int sl_test_link_fec_cntr_set(void);
int sl_test_link_an_lp_caps_get(void);

int sl_test_port_num_entry_init(u8 lgrp_num, u8 link_num);
struct kobject *sl_test_port_num_sysfs_get(u8 lgrp_num, u8 link_num);

#endif /* _SL_TEST_DEBUGFS_LINK_H_ */
