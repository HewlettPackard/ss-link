/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_SERDES_SWIZZLE_H_
#define _SL_SYSFS_SERDES_SWIZZLE_H_

#include <linux/types.h>

struct sl_ctl_lgrp;

int  sl_sysfs_serdes_lane_swizzle_create(struct sl_ctl_lgrp *ctl_lgrp, u8 lane_num);
void sl_sysfs_serdes_lane_swizzle_delete(struct sl_ctl_lgrp *ctl_lgrp, u8 lane_num);

#endif /* _SL_SYSFS_SERDES_SWIZZLE_H_ */
