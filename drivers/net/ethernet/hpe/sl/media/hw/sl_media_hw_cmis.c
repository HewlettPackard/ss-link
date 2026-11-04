// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/errno.h>
#include <linux/types.h>

#include "sl_media_jack.h"
#include "base/sl_media_eeprom_cmis.h"
#include "base/sl_media_log.h"
#include "hw/sl_media_hw_cmis.h"
#include "hw/sl_media_hw_power.h"

#define LOG_NAME SL_MEDIA_HW_CMIS_LOG_NAME

#define CMIS_OPTICAL_MONITORS_PAGE            0x11
#define CMIS_OPTICAL_MONITORS_TX_POWER_OFFSET 154
#define CMIS_OPTICAL_MONITORS_TX_BIAS_OFFSET  170
#define CMIS_OPTICAL_MONITORS_RX_POWER_OFFSET 186

#define CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_RESERVED 3
#define CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_SHIFT    3
#define CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_MASK     0x3

#define SL_MEDIA_MON_MASK (SL_MEDIA_TX_BIAS_MON          | \
			   SL_MEDIA_TX_OPTICAL_POWER_MON | \
			   SL_MEDIA_RX_OPTICAL_POWER_MON)

int sl_media_hw_cmis_power_tx_bias_get(struct sl_media_jack *media_jack, u16 value, u32 *tx_bias_ua)
{
	u8  advertisement;
	u8  scale;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "power tx bias get");

	rtn = sl_media_eeprom_cmis_monitors_advertisement_get(media_jack, &advertisement);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "power tx bias get advertisement failed [%d]", rtn);
		return rtn;
	}

	scale = (advertisement >> CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_SHIFT) &
		CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_MASK;

	if (scale == CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_RESERVED) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "power tx bias get reserved scaling");
		return -EOPNOTSUPP;
	}

	*tx_bias_ua = (value * 2U) << scale;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "power tx bias get (value = %u uA, scale = %u)",
			 *tx_bias_ua, scale);

	return 0;
}

int sl_media_hw_cmis_power_optical_monitors_supported_get(struct sl_media_jack *media_jack,
							  u8 *supported)
{
	int  rtn;
	u8   advertised_mon_support;
	u8   scale;
	bool flat_memory;

	sl_media_log_dbg(media_jack, LOG_NAME, "power optical monitors supported get");

	*supported = 0;

	/* CMIS needs eeprom pages for optical monitors */
	rtn = sl_media_eeprom_cmis_flat_memory_get(media_jack, &flat_memory);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "power optical monitors supported get flat memory failed [%d]", rtn);
		return rtn;
	}

	if (flat_memory) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "power optical monitors supported get flat memory not supported");
		return -EOPNOTSUPP;
	}

	rtn = sl_media_eeprom_cmis_monitors_advertisement_get(media_jack, &advertised_mon_support);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "power optical monitors supported get advertisement failed [%d]", rtn);
		return rtn;
	}

	scale = (advertised_mon_support >> CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_SHIFT) &
		CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_MASK;

	advertised_mon_support &= SL_MEDIA_MON_MASK;

	if (!advertised_mon_support) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "power optical monitors supported get no monitor support");
		return -EOPNOTSUPP;
	}

	/* A reserved scaling factor prevents interpreting TX bias. */
	if ((advertised_mon_support & SL_MEDIA_TX_BIAS_MON) &&
	    scale == CMIS_TX_BIAS_CURRENT_SCALING_FACTOR_RESERVED) {
		sl_media_log_warn_trace(media_jack, LOG_NAME,
					"power optical monitors supported get reserved bias scaling");
		advertised_mon_support &= ~SL_MEDIA_TX_BIAS_MON;
	}

	*supported = advertised_mon_support;

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "power optical monitors supported get (supported = 0x%X)", *supported);

	return 0;
}

int sl_media_hw_cmis_power_optical_monitor_addr_get(struct sl_media_jack *media_jack,
						    u8 monitor, u8 lane, u8 *page, u8 *offset)
{
	sl_media_log_dbg(media_jack, LOG_NAME,
			 "optical monitor addr get (monitor = 0x%X %s, lane = 0x%X)",
			 monitor, sl_media_hw_power_optical_monitor_name_get(monitor), lane);

	*page = CMIS_OPTICAL_MONITORS_PAGE;

	switch (monitor) {
	case SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM:
		*offset = CMIS_OPTICAL_MONITORS_TX_POWER_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	case SL_MEDIA_TX_BIAS_MON_BIT_NUM:
		*offset = CMIS_OPTICAL_MONITORS_TX_BIAS_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	case SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM:
		*offset = CMIS_OPTICAL_MONITORS_RX_POWER_OFFSET + lane * sizeof(u16);
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "optical monitor addr get (page = 0x%X, offset = 0x%X)", *page, *offset);
		return 0;
	default:
		sl_media_log_err(media_jack, LOG_NAME,
				 "optical monitor addr get invalid monitor (monitor = 0x%X %s)",
				 monitor, sl_media_hw_power_optical_monitor_name_get(monitor));
		return -EINVAL;
	}
}
