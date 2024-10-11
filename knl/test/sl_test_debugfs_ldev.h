/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LDEV_H_
#define _SL_TEST_DEBUGFS_LDEV_H_

int sl_test_ldev_create(struct dentry *top_dir);

u8  sl_test_ldev_num_get(void);

#endif /* _SL_TEST_DEBUGFS_LDEV_H_ */
