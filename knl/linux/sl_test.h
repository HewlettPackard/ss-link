/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_TEST_H_
#define _LINUX_SL_TEST_H_

#include <linux/sl_link.h>
#include <linux/sl_lgrp.h>
#include <linux/sl_ldev.h>

int sl_test_ctl(struct sl_ldev *ldev, u64 lgrp_map, u64 link_map, u32 test_num, u32 flags);

int sl_test_core(struct sl_ldev *ldev, u64 lgrp_map, u64 link_map, u32 test_num, u32 flags);

int sl_test_an_fake_caps_set(struct sl_link *link, struct sl_link_caps *caps);

int sl_test_fec_cntrs_use_set(struct sl_link *link, bool use_test_cntrs);

int sl_test_fec_cntrs_set(struct sl_link *link, u64 ucw, u64 ccw, u64 good_cw);

int sl_test_media(struct sl_ldev *ldev, u64 lgrp_map, u32 test_num, u32 flags);

#endif /* _LINUX_SL_TEST_H_ */
