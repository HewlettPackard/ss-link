/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_TEST_SERDES_H_
#define _SL_CORE_TEST_SERDES_H_

int sl_core_test_serdes_settings_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
				     s16 pre1, s16 pre2, s16 pre3, s16 cursor,
				     s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
				     u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options);
int sl_core_test_serdes_settings_unset(u8 ldev_num, u8 lgrp_num, u8 link_num);

#endif /* _SL_CORE_TEST_SERDES_H_ */
