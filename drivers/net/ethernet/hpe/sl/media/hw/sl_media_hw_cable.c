// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "base/sl_media_log.h"
#include "hw/sl_media_hw_cable.h"

#define LOG_NAME SL_MEDIA_HW_CABLE_LOG_NAME

#define SL_MEDIA_SFF_CTRL_FUNCS_PAGE 0x00
#define SL_MEDIA_SFF_POWER_SET_ADDR  0x5D
#define SL_MEDIA_SFF_POWER_SET_MASK  BIT(1)
static int sl_media_hw_sff_cable_low_power_get(struct sl_media_jack *media_jack, bool *low_power)
{
	int rtn;
	u8  data;

	/* When power set bit is 1 we are in low power mode */
	rtn = sl_media_io_read8(media_jack, SL_MEDIA_SFF_CTRL_FUNCS_PAGE,
				SL_MEDIA_SFF_POWER_SET_ADDR, &data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "sff low power read8 failed [%d]", rtn);
		*low_power = false;
		return rtn;
	}

	*low_power = data & SL_MEDIA_SFF_POWER_SET_MASK;

	sl_media_log_dbg(media_jack, LOG_NAME, "sff low power get (low_power = %u)", *low_power);

	return 0;
}

/**
 * sl_media_hw_sff_cable_state_get - Get the state of the SFF cable
 * @media_jack: Pointer to the media jack structure
 * @cable_state: Pointer to store the retrieved cable state
 *
 * We use the cable status naming from the CMIS spec.
 *
 * Return: 0 on success, negative error code on failure
 */
static int sl_media_hw_sff_cable_state_get(struct sl_media_jack *media_jack, u8 *cable_state)
{
	int  rtn;
	bool low_power;

	sl_media_log_dbg(media_jack, LOG_NAME, "sff state get");

	rtn = sl_media_hw_sff_cable_low_power_get(media_jack, &low_power);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "sff low power get failed [%d]", rtn);
		*cable_state = SL_MEDIA_CABLE_STATE_INVALID;
		return rtn;
	}

	*cable_state = low_power ? SL_MEDIA_CABLE_STATE_LOW_PWR : SL_MEDIA_CABLE_STATE_READY;

	sl_media_log_dbg(media_jack, LOG_NAME, "sff state get (cable_state = %u %s)", *cable_state,
			 sl_media_jack_cable_state_str(*cable_state));

	return 0;
}

#define SL_MEDIA_GLOBAL_STATUS_PAGE 0x00
#define SL_MEDIA_GLOBAL_STATUS_ADDR 3
#define SL_MEDIA_CABLE_STATE_MASK   0x0E
#define SL_MEDIA_CABLE_STATE_SHIFT  1
static int sl_media_hw_cmis_cable_state_get(struct sl_media_jack *media_jack, u8 *cable_state)
{
	int rtn;
	u8  data;

	sl_media_log_dbg(media_jack, LOG_NAME, "cmis state get");

	rtn = sl_media_io_read8(media_jack, SL_MEDIA_GLOBAL_STATUS_PAGE, SL_MEDIA_GLOBAL_STATUS_ADDR, &data);
	if (rtn) {
		sl_media_log_warn_trace(media_jack, LOG_NAME, "cmis state get read8 failed [%d]", rtn);
		*cable_state = SL_MEDIA_CABLE_STATE_INVALID;
		return rtn;
	}

	*cable_state = (data & SL_MEDIA_CABLE_STATE_MASK) >> SL_MEDIA_CABLE_STATE_SHIFT;

	/* Update the media jack fault cause and state anytime we discover a cable fault. It is the only time
	 * we detect a fault has happened. This is not a function error.
	 */
	if (*cable_state == SL_MEDIA_CABLE_STATE_FAULT) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "cmis state get - fault (data = 0x%X)", data);
		sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_CABLE_FAULT);
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cmis state get (cable_state = %u %s)", *cable_state,
			 sl_media_jack_cable_state_str(*cable_state));

	return 0;
}

int sl_media_hw_cable_state_get(struct sl_media_jack *media_jack, u8 *cable_state)
{
	if (!sl_media_data_jack_media_is_format_cmis(media_jack))
		return sl_media_hw_sff_cable_state_get(media_jack, cable_state);

	return sl_media_hw_cmis_cable_state_get(media_jack, cable_state);
}
