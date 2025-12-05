/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_SERDES_LOS_H_
#define _SL_SYSFS_SERDES_LOS_H_

struct sl_ctrl_lgrp;

int  sl_sysfs_serdes_lane_los_create(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num);
void sl_sysfs_serdes_lane_los_delete(struct sl_ctrl_lgrp *ctrl_lgrp, u8 asic_lane_num);

#endif /* _SL_SYSFS_SERDES_LOS_H_ */
