/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_LINK_FEC_H_
#define _SL_SYSFS_LINK_FEC_H_

struct sl_ctrl_link;

int  sl_sysfs_link_fec_create(struct sl_ctrl_link *ctrl_link);
void sl_sysfs_link_fec_delete(struct sl_ctrl_link *ctrl_link);

#endif /* _SL_SYSFS_LINK_FEC_H_ */
