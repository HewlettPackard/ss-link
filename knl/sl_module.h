/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MODULE_COMMON_H_
#define _SL_MODULE_COMMON_H_

struct device;

char          *sl_version_str_get(void);
struct device *sl_device_get(void);

#endif /* _SL_MODULE_COMMON_H_ */
