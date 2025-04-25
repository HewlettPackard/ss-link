/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_LLR_H_
#define _SL_TEST_DEBUGFS_LLR_H_

int sl_test_debugfs_llr_create(struct dentry *top_dir);

void sl_test_llr_remove(u8 ldev_num, u8 lgrp_num, u8 llr_num);

int sl_test_llr_new(void);
int sl_test_llr_del(void);

int sl_test_llr_config_set(void);
int sl_test_llr_policy_set(void);
int sl_test_llr_start(void);
int sl_test_llr_stop(void);

#endif /* _SL_TEST_DEBUGFS_LLR_H_ */
