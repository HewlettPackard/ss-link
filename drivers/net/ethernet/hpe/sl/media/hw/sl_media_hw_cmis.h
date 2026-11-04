/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_HW_CMIS_H_
#define _SL_MEDIA_HW_CMIS_H_

#include <linux/types.h>

struct sl_media_jack;

int sl_media_hw_cmis_power_tx_bias_get(struct sl_media_jack *media_jack, u16 value, u32 *tx_bias_ua);
int sl_media_hw_cmis_power_optical_monitors_supported_get(struct sl_media_jack *media_jack,
							  u8 *supported);
int sl_media_hw_cmis_power_optical_monitor_addr_get(struct sl_media_jack *media_jack,
						    u8 monitor, u8 lane, u8 *page, u8 *offset);

#endif /* _SL_MEDIA_HW_CMIS_H_ */
