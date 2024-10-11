/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_SERDES_SETTINGS_H_
#define _SL_SYSFS_SERDES_SETTINGS_H_

struct sl_ctl_lgrp;

int  sl_sysfs_serdes_lane_settings_create(struct sl_ctl_lgrp *ctl_lgrp, u8 lane_num);
void sl_sysfs_serdes_lane_settings_delete(struct sl_ctl_lgrp *ctl_lgrp, u8 lane_num);

#endif /* _SL_SYSFS_SERDES_SETTINGS_H_ */
