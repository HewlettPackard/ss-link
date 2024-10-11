/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_SERDES_H_
#define _SL_TEST_DEBUGFS_SERDES_H_

int sl_test_serdes_create(struct dentry *parent);

int sl_test_serdes_set(void);
int sl_test_serdes_unset(void);

#endif /* _SL_TEST_DEBUGFS_SERDES_H_ */
