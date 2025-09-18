/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_TEST_FEC_H_
#define _SL_CORE_TEST_FEC_H_

struct sl_core_link_fec_cw_cntrs;

int sl_core_test_fec_cntrs_use_set(u8 ldev_num, u8 lgrp_num, u8 link_num, bool use_test_cntrs);
int sl_core_test_fec_cw_cntrs_set(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_fec_cw_cntrs *cw_cntrs);
int sl_core_test_fec_cw_cntrs_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_core_link_fec_cw_cntrs *cw_cntrs);

#endif /* _SL_CORE_TEST_FEC_H_ */
