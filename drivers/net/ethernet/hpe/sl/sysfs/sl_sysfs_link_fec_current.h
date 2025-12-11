/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_LINK_FEC_CURRENT_H_
#define _SL_SYSFS_LINK_FEC_CURRENT_H_

struct sl_core_link;

int  sl_sysfs_link_fec_current_create(struct sl_core_link *core_link, struct kobject *parent_kobj);
void sl_sysfs_link_fec_current_delete(struct sl_core_link *core_link);

#endif /* _SL_SYSFS_LINK_FEC_CURRENT_H_ */
