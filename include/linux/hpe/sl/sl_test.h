/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_TEST_H_
#define _LINUX_SL_TEST_H_

#include <linux/hpe/sl/sl_link.h>
#include <linux/hpe/sl/sl_lgrp.h>
#include <linux/hpe/sl/sl_ldev.h>

int sl_test_an_fake_caps_set(struct sl_link *link, struct sl_link_caps *caps);

int sl_test_fec_cntrs_use_set(struct sl_link *link, bool use_test_cntrs);
int sl_test_fec_cntrs_set(struct sl_link *link, u64 ucw, u64 ccw, u64 good_cw);

#endif /* _LINUX_SL_TEST_H_ */
