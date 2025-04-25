/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_DEBUGFS_MAC_H_
#define _SL_TEST_DEBUGFS_MAC_H_

int sl_test_debugfs_mac_create(struct dentry *top_dir);

void sl_test_mac_remove(u8 ldev_num, u8 lgrp_num, u8 mac_num);

int sl_test_mac_new(void);
int sl_test_mac_del(void);

int sl_test_mac_tx_start(void);
int sl_test_mac_rx_start(void);

#endif /* _SL_TEST_DEBUGFS_MAC_H_ */
