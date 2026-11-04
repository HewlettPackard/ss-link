// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/errno.h>
#include <linux/types.h>

#include "sl_media_jack.h"
#include "base/sl_media_eeprom_sff.h"
#include "base/sl_media_log.h"
#include "hw/sl_media_hw_power.h"
#include "hw/sl_media_hw_sff.h"

#define LOG_NAME SL_MEDIA_HW_SFF_LOG_NAME

#define SFF_OPTICAL_MONITORS_PAGE            0
#define SFF_OPTICAL_MONITORS_RX_POWER_OFFSET 34
#define SFF_OPTICAL_MONITORS_TX_BIAS_OFFSET  42
#define SFF_OPTICAL_MONITORS_TX_POWER_OFFSET 50
#define SFF_OPTICAL_MONITOR_LANES            4

#define SFF_TX_POWER_MONITOR_SUPPORTED BIT(2)
#define SFF_RX_POWER_MONITOR_SUPPORTED BIT(3)

int sl_media_hw_sff_power_tx_bias_get(struct sl_media_jack *media_jack, u16 value, u32 *tx_bias_ua)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "power tx bias get");

	/* SFF always needs to be scaled by 1. */
	*tx_bias_ua = (value * 2U) << 1;

	return 0;
}

int sl_media_hw_sff_power_optical_monitors_supported_get(struct sl_media_jack *media_jack,
							 u8 *supported)
{
	u8  diag_monitoring_type;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "power optical monitors supported get");

	/* Bias is always supported */
	*supported = SL_MEDIA_TX_BIAS_MON;

	rtn = sl_media_eeprom_sff_diag_monitoring_type_get(media_jack, &diag_monitoring_type);
	if (rtn)
		sl_media_log_warn_trace(media_jack, LOG_NAME, "power optical monitors supported get failed [%d]", rtn);

	if (diag_monitoring_type & SFF_TX_POWER_MONITOR_SUPPORTED)
		*supported |= SL_MEDIA_TX_OPTICAL_POWER_MON;

	/* Only average RX power reported as mW is supported, not OMA. */
	if (diag_monitoring_type & SFF_RX_POWER_MONITOR_SUPPORTED)
		*supported |= SL_MEDIA_RX_OPTICAL_POWER_MON;

	sl_media_log_dbg(media_jack, LOG_NAME, "power optical monitors supported get supported = 0x%X", *supported);

	return 0;
}

int sl_media_hw_sff_power_optical_monitor_addr_get(struct sl_media_jack *media_jack,
						   u8 monitor, u8 lane, u8 *page, u8 *offset)
{
	/* SFF supports fewer lanes than CMIS */
	if (lane >= SFF_OPTICAL_MONITOR_LANES) {
		sl_media_log_err(media_jack, LOG_NAME, "optical monitor get invalid lane (lane = %u)", lane);
		*page   = 0;
		*offset = 0;
		return -EINVAL;
	}

	*page = SFF_OPTICAL_MONITORS_PAGE;

	switch (monitor) {
	case SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM:
		*offset = SFF_OPTICAL_MONITORS_TX_POWER_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get tx power (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	case SL_MEDIA_TX_BIAS_MON_BIT_NUM:
		*offset = SFF_OPTICAL_MONITORS_TX_BIAS_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get tx bias (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	case SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM:
		*offset = SFF_OPTICAL_MONITORS_RX_POWER_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get rx power (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	default:
		*offset = 0;
		sl_media_log_err(media_jack, LOG_NAME,
				 "optical monitor addr get invalid monitor (monitor = 0x%X)", monitor);
		return -EINVAL;
	}
}
