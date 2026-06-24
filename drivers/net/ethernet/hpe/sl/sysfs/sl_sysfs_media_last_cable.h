/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_SYSFS_MEDIA_LAST_CABLE_H_
#define _SL_SYSFS_MEDIA_LAST_CABLE_H_

struct sl_ctrl_lgrp;
struct sl_media_lgrp;

int sl_sysfs_media_last_cable_create(struct sl_ctrl_lgrp *ctrl_lgrp, struct sl_media_lgrp *media_lgrp);

#endif /* _SL_SYSFS_MEDIA_LAST_CABLE_H_ */
