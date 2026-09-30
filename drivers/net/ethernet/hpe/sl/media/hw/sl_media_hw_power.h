/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_HW_POWER_H_
#define _SL_MEDIA_HW_POWER_H_

#include <linux/types.h>

struct sl_media_jack;

int sl_media_hw_power_high_set(struct sl_media_jack *media_jack);
int sl_media_hw_power_low_set(struct sl_media_jack *media_jack);

#endif /* _SL_MEDIA_HW_POWER_H_ */
