/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LLR_H_
#define _SL_TEST_DEBUGFS_LLR_H_

int sl_test_llr_create(struct dentry *top_dir);

int sl_test_llr_start(void);
int sl_test_llr_stop(void);

#endif /* _SL_TEST_DEBUGFS_LLR_H_ */
