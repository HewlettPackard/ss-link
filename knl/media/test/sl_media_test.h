/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_TEST_H_
#define _SL_MEDIA_TEST_H_

#include "uapi/sl_media.h"
#include "sl_lgrp.h"

int  sl_test_media_test_exec(u8 ldev_num, u64 lgrp_map, int test_num, u32 flags);

#endif /* _SL_MEDIA_TEST_H_ */
