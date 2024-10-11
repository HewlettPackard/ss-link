/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_COMMON_H_
#define _SL_TEST_COMMON_H_

#ifdef __KERNEL__
#include <linux/bitops.h>
#else
#ifndef BIT
#define BIT(_num)      (1UL << (_num))
#endif
#ifndef BIT_ULL
#define BIT_ULL(_num)  (1ULL << (_num))
#endif
#endif

/* Config Admin Options */
#define SL_LINK_CONFIG_OPT_LOCK        BIT(30) /* Lock configuration from modification */
#define SL_LINK_CONFIG_OPT_ADMIN       BIT(31) /* Perform admin level operation        */

/* Policy Admin Options */
#define SL_LINK_POLICY_OPT_LOCK        BIT(30) /* Lock configuration from modification */
#define SL_LINK_POLICY_OPT_ADMIN       BIT(31) /* Perform admin level operation        */

int sl_test_serdes_params_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      s16 pre1, s16 pre2, s16 pre3, s16 cursor,
			      s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
			      u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options);

int sl_test_serdes_params_unset(u8 ldev_num, u8 lgrp_num, u8 link_num);

#endif /* _SL_TEST_COMMON_H_ */
