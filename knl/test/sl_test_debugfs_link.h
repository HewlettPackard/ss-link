/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LINK_H_
#define _SL_TEST_DEBUGFS_LINK_H_

int sl_test_link_create(struct dentry *top_dir);

int sl_test_link_up(void);
int sl_test_link_down(void);
int sl_test_link_config_set(void);
int sl_test_link_policy_set(void);
u8  sl_test_link_num_get(void);

#endif /* _SL_TEST_DEBUGFS_LINK_H_ */
