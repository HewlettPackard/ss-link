/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MODULE_H_
#define _SL_MODULE_H_

struct device;

char          *sl_version_str_get(void);
struct device *sl_device_get(void);

#endif /* _SL_MODULE_H_ */
