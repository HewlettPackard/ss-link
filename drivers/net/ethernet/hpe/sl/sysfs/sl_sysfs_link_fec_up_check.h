/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_LINK_FEC_UP_CHECK_H_
#define _SL_SYSFS_LINK_FEC_UP_CHECK_H_

struct sl_core_link;

int  sl_sysfs_link_fec_up_check_create(struct sl_core_link *core_link, struct kobject *parent_kobj);
void sl_sysfs_link_fec_up_check_delete(struct sl_core_link *core_link);

#endif /* _SL_SYSFS_LINK_FEC_UP_CHECK_H_ */
