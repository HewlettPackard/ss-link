/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_H_
#define _SL_SYSFS_H_

#include <linux/kobject.h>

struct sl_ctl_ldev;
struct sl_ctl_lgrp;
struct sl_ctl_link;
struct sl_ctl_llr;
struct sl_ctl_mac;

u8 lane_num_to_link_num(struct sl_ctl_lgrp *ctl_lgrp, u8 lane_num);

int  sl_sysfs_ldev_create(struct sl_ctl_ldev *ctl_ldev);
void sl_sysfs_ldev_delete(struct sl_ctl_ldev *ctl_ldev);

int  sl_sysfs_lgrp_create(struct sl_ctl_lgrp *ctl_lgrp);
void sl_sysfs_lgrp_delete(struct sl_ctl_lgrp *ctl_lgrp);

int  sl_sysfs_lgrp_config_create(struct sl_ctl_lgrp *ctl_lgrp);
void sl_sysfs_lgrp_config_delete(struct sl_ctl_lgrp *ctl_lgrp);

int  sl_sysfs_lgrp_policy_create(struct sl_ctl_lgrp *ctl_lgrp);
void sl_sysfs_lgrp_policy_delete(struct sl_ctl_lgrp *ctl_lgrp);

int  sl_sysfs_serdes_create(struct sl_ctl_lgrp *ctl_lgrp);
void sl_sysfs_serdes_delete(struct sl_ctl_lgrp *ctl_lgrp);

int  sl_sysfs_media_create(struct sl_ctl_lgrp *ctl_lgrp);
void sl_sysfs_media_delete(struct sl_ctl_lgrp *ctl_lgrp);
int  sl_sysfs_media_speeds_create(u8 ldev_num, u8 lgrp_num);

int  sl_sysfs_link_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_delete(struct sl_ctl_link *ctl_link);

int  sl_sysfs_link_config_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_config_delete(struct sl_ctl_link *ctl_link);

int  sl_sysfs_link_policy_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_policy_delete(struct sl_ctl_link *ctl_link);

int  sl_sysfs_link_fec_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_fec_delete(struct sl_ctl_link *ctl_link);

int  sl_sysfs_link_caps_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_caps_delete(struct sl_ctl_link *ctl_link);

int sl_sysfs_link_counters_create(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_counters_delete(struct sl_ctl_link *ctl_link);

int  sl_sysfs_llr_create(struct sl_ctl_llr *ctl_llr);
void sl_sysfs_llr_delete(struct sl_ctl_llr *ctl_llr);

int  sl_sysfs_llr_config_create(struct sl_ctl_llr *ctl_llr);
void sl_sysfs_llr_config_delete(struct sl_ctl_llr *ctl_llr);

int  sl_sysfs_llr_policy_create(struct sl_ctl_llr *ctl_llr);
void sl_sysfs_llr_policy_delete(struct sl_ctl_llr *ctl_llr);

int  sl_sysfs_llr_loop_time_create(struct sl_ctl_llr *ctl_llr);
void sl_sysfs_llr_loop_time_delete(struct sl_ctl_llr *ctl_llr);

int  sl_sysfs_mac_create(struct sl_ctl_mac *ctl_mac);
void sl_sysfs_mac_delete(struct sl_ctl_mac *ctl_mac);

#endif /* _SL_SYSFS_H_ */
