/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_FEC_H_
#define _SL_SYSFS_FEC_H_

struct sl_ctl_link;

int  sl_sysfs_link_fec_current_create(struct sl_ctl_link *ctl_link);
int  sl_sysfs_link_fec_up_create(struct sl_ctl_link *ctl_link);
int  sl_sysfs_link_fec_down_create(struct sl_ctl_link *ctl_link);

void sl_sysfs_link_fec_current_delete(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_fec_up_delete(struct sl_ctl_link *ctl_link);
void sl_sysfs_link_fec_down_delete(struct sl_ctl_link *ctl_link);

#endif /* _SL_SYSFS_FEC_H_ */
