/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_HW_POWER_H_
#define _SL_MEDIA_HW_POWER_H_

#include <linux/types.h>
#include <linux/bits.h>

enum sl_media_optical_monitor_bit_num {
	SL_MEDIA_TX_BIAS_MON_BIT_NUM           = 0,
	SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM,
	SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM,
	SL_MEDIA_NUM_OPTICAL_MONS
};

#define SL_MEDIA_TX_BIAS_MON          BIT(SL_MEDIA_TX_BIAS_MON_BIT_NUM)
#define SL_MEDIA_TX_OPTICAL_POWER_MON BIT(SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM)
#define SL_MEDIA_RX_OPTICAL_POWER_MON BIT(SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM)

struct sl_media_jack;

const char *sl_media_hw_power_optical_monitor_name_get(u8 monitor);
int         sl_media_hw_power_optical_monitors_supported_str(u8 supported, char *supported_str,
							     unsigned int supported_str_size);

int sl_media_hw_power_optical_monitors_supported_get(struct sl_media_jack *media_jack, u8 *supported);
int sl_media_hw_power_tx_get(struct sl_media_jack *media_jack, u8 lane, u16 *tx_power);
int sl_media_hw_power_tx_bias_get(struct sl_media_jack *media_jack, u8 lane, u32 *tx_bias_ua);
int sl_media_hw_power_rx_get(struct sl_media_jack *media_jack, u8 lane, u16 *rx_power);

int sl_media_hw_power_high_set(struct sl_media_jack *media_jack);
int sl_media_hw_power_low_set(struct sl_media_jack *media_jack);

#endif /* _SL_MEDIA_HW_POWER_H_ */
