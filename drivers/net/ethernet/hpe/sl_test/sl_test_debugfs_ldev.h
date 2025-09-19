/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LDEV_H_
#define _SL_TEST_DEBUGFS_LDEV_H_

int             sl_test_debugfs_ldev_create(struct dentry *top_dir);
int             sl_test_ldev_exit(void);

int             sl_test_ldev_new(void);
int             sl_test_ldev_del(void);

struct sl_ldev *sl_test_ldev_get(void);
u8              sl_test_debugfs_ldev_num_get(void);

#endif /* _SL_TEST_DEBUGFS_LDEV_H_ */
