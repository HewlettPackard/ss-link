/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_HW_CABLE_H_
#define _SL_MEDIA_HW_CABLE_H_

#include <linux/types.h>

struct sl_media_jack;

int sl_media_hw_cable_state_get(struct sl_media_jack *media_jack, u8 *state);

#endif /* _SL_MEDIA_HW_CABLE_H_ */
