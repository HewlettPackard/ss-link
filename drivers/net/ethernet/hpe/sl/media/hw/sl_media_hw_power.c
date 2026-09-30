// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "base/sl_media_eeprom.h"
#include "base/sl_media_log.h"
#include "hw/sl_media_hw_power.h"
#include "hw/sl_media_hw_cable.h"

#define LOG_NAME SL_MEDIA_HW_POWER_LOG_NAME

#define SL_MEDIA_POWER_STATE_POLL_MS 20
#define SL_MEDIA_POWER_SFF_WAIT_MS   8000

static int sl_media_hw_power_wait_for_state(struct sl_media_jack *media_jack, u32 timeout_ms, u8 expected_state)
{
	int rtn;
	u8  cable_state;
	u32 waited_ms;

	sl_media_log_dbg(media_jack, LOG_NAME, "wait for state (expected_state = %u)", expected_state);

	if (timeout_ms == 0) {
		sl_media_log_warn_trace(media_jack, LOG_NAME, "wait for state - default timeout");
		timeout_ms = SL_MEDIA_POWER_CHANGE_DEFAULT_WAIT_TIME_MS;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "wait for state (timeout_ms = %u)", timeout_ms);

	for (waited_ms = 0; waited_ms < timeout_ms; waited_ms += SL_MEDIA_POWER_STATE_POLL_MS) {
		rtn = sl_media_hw_cable_state_get(media_jack, &cable_state);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "wait for state - state get failed [%d]", rtn);
			return rtn;
		}

		switch (cable_state) {
		case SL_MEDIA_CABLE_STATE_FAULT:
			sl_media_log_err_trace(media_jack, LOG_NAME, "wait for state - cable fault");
			return -EIO;
		case SL_MEDIA_CABLE_STATE_READY:
		case SL_MEDIA_CABLE_STATE_LOW_PWR:
		case SL_MEDIA_CABLE_STATE_PWR_UP:
		case SL_MEDIA_CABLE_STATE_PWR_DN:
			if (cable_state == expected_state) {
				sl_media_log_dbg(media_jack, LOG_NAME, "wait for state - done (waited_ms = %u)",
						 waited_ms);
				return 0;
			}
			msleep(SL_MEDIA_POWER_STATE_POLL_MS);
			continue;
		case SL_MEDIA_CABLE_STATE_INVALID:
		default:
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "wait for state - invalid state (cable_state = %u %s)",
					       cable_state, sl_media_jack_cable_state_str(cable_state));
			return -EBADRQC;
		}
	}

	sl_media_log_err_trace(media_jack, LOG_NAME,
			       "wait for state - timed out (waited_ms = %u)", waited_ms);

	return -ETIMEDOUT;
}

static int sl_media_hw_power_high_wait(struct sl_media_jack *media_jack)
{
	int rtn;
	u32 power_up_timeout_ms;

	if (!sl_media_data_jack_media_is_format_cmis(media_jack)) {
		msleep(SL_MEDIA_POWER_SFF_WAIT_MS);
		return 0;
	}

	power_up_timeout_ms = SL_MEDIA_POWER_CHANGE_DEFAULT_WAIT_TIME_MS;
	rtn = sl_media_eeprom_module_pwr_up_duration_ms_get(media_jack, &power_up_timeout_ms);
	if (rtn)
		sl_media_log_warn_trace(media_jack, LOG_NAME, "failed to get module power up duration [%d]", rtn);

	sl_media_log_dbg(media_jack, LOG_NAME, "high wait (power_up_timeout_ms = %u)", power_up_timeout_ms);

	return sl_media_hw_power_wait_for_state(media_jack, power_up_timeout_ms, SL_MEDIA_CABLE_STATE_READY);
}

static int sl_media_hw_power_low_wait(struct sl_media_jack *media_jack)
{
	int rtn;
	u32 power_dn_timeout_ms;

	if (!sl_media_data_jack_media_is_format_cmis(media_jack)) {
		msleep(SL_MEDIA_POWER_SFF_WAIT_MS);
		return 0;
	}

	power_dn_timeout_ms = SL_MEDIA_POWER_CHANGE_DEFAULT_WAIT_TIME_MS;
	rtn = sl_media_eeprom_module_pwr_dn_duration_ms_get(media_jack, &power_dn_timeout_ms);
	if (rtn)
		sl_media_log_warn_trace(media_jack, LOG_NAME, "failed to get module power down duration [%d]", rtn);

	sl_media_log_dbg(media_jack, LOG_NAME, "low wait (power_dn_timeout_ms = %u)", power_dn_timeout_ms);

	return sl_media_hw_power_wait_for_state(media_jack, power_dn_timeout_ms, SL_MEDIA_CABLE_STATE_LOW_PWR);
}

#define SL_MEDIA_POWER_UP_PAGE       0x00
#define SL_MEDIA_CMIS_POWER_UP_ADDR  0x1a
#define SL_MEDIA_CMIS_POWER_UP_DATA  0x00
#define SL_MEDIA_SFF_POWER_UP_ADDR   0x5D
#define SL_MEDIA_SFF_POWER_UP_DATA   0x0D
int sl_media_hw_power_high_set(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  addr;
	u8  data;
	u8  cable_state;

	sl_media_log_dbg(media_jack, LOG_NAME, "high set");

	rtn = sl_media_hw_cable_state_get(media_jack, &cable_state);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "high set - cable state get failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "high set (cable_state = %u %s)",
			 cable_state, sl_media_jack_cable_state_str(cable_state));

	switch (cable_state) {
	case SL_MEDIA_CABLE_STATE_LOW_PWR:
		sl_media_log_dbg(media_jack, LOG_NAME, "high set - low power");
		break;
	case SL_MEDIA_CABLE_STATE_PWR_UP:
		sl_media_log_dbg(media_jack, LOG_NAME, "high set - powering up");
		goto wait;
	case SL_MEDIA_CABLE_STATE_READY:
		sl_media_log_dbg(media_jack, LOG_NAME, "high set - already ready high power");
		return 0;
	case SL_MEDIA_CABLE_STATE_FAULT:
		sl_media_log_err_trace(media_jack, LOG_NAME, "high set - cable fault");
		return -EIO;
	case SL_MEDIA_CABLE_STATE_PWR_DN:
	case SL_MEDIA_CABLE_STATE_INVALID:
	default:
		sl_media_log_err(media_jack, LOG_NAME, "high set - invalid state (cable_state = %u %s)",
				 cable_state, sl_media_jack_cable_state_str(cable_state));
		return -EBADRQC;
	}

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		addr = SL_MEDIA_CMIS_POWER_UP_ADDR;
		data = SL_MEDIA_CMIS_POWER_UP_DATA;
	} else {
		addr = SL_MEDIA_SFF_POWER_UP_ADDR;
		data = SL_MEDIA_SFF_POWER_UP_DATA;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "high set - write (addr = 0x%X, data = 0x%X)", addr, data);

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_POWER_UP_PAGE, addr, data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "high set - write8 failed [%d]", rtn);
		return -EIO;
	}

	/* Flush */
	rtn = sl_media_io_read8(media_jack, SL_MEDIA_POWER_UP_PAGE, addr, &data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "high set - read8 failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "high set - read back (data = 0x%X)", data);

wait:
	rtn = sl_media_hw_power_high_wait(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "high set - wait failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

#define SL_MEDIA_POWER_DOWN_PAGE      0x00
#define SL_MEDIA_CMIS_POWER_DOWN_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_DOWN_DATA 0x10
#define SL_MEDIA_SFF_POWER_DOWN_ADDR  0x5D
#define SL_MEDIA_SFF_POWER_DOWN_DATA  0x00
int sl_media_hw_power_low_set(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  addr;
	u8  data;
	u8  cable_state;

	sl_media_log_dbg(media_jack, LOG_NAME, "low set");

	rtn = sl_media_hw_cable_state_get(media_jack, &cable_state);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "low set - cable state get failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "low set (cable_state = %u %s)",
			 cable_state, sl_media_jack_cable_state_str(cable_state));

	switch (cable_state) {
	case SL_MEDIA_CABLE_STATE_LOW_PWR:
		sl_media_log_dbg(media_jack, LOG_NAME, "low set - already low power");
		return 0;
	case SL_MEDIA_CABLE_STATE_READY:
		sl_media_log_dbg(media_jack, LOG_NAME, "low set - ready high power");
		break;
	case SL_MEDIA_CABLE_STATE_PWR_DN:
		sl_media_log_dbg(media_jack, LOG_NAME, "low set - powering down");
		goto wait;
	case SL_MEDIA_CABLE_STATE_FAULT:
		sl_media_log_err_trace(media_jack, LOG_NAME, "low set - cable fault");
		return -EIO;
	case SL_MEDIA_CABLE_STATE_PWR_UP:
	case SL_MEDIA_CABLE_STATE_INVALID:
	default:
		sl_media_log_err(media_jack, LOG_NAME, "low set - invalid state (cable_state = %u %s)",
				 cable_state, sl_media_jack_cable_state_str(cable_state));
		return -EBADRQC;
	}

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		addr = SL_MEDIA_CMIS_POWER_DOWN_ADDR;
		data = SL_MEDIA_CMIS_POWER_DOWN_DATA;
	} else {
		addr = SL_MEDIA_SFF_POWER_DOWN_ADDR;
		data = SL_MEDIA_SFF_POWER_DOWN_DATA;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "low set - write (addr = 0x%X, data = 0x%X)", addr, data);

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_POWER_DOWN_PAGE, addr, data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "low set - write8 failed [%d]", rtn);
		return -EIO;
	}

	/* Flush */
	rtn = sl_media_io_read8(media_jack, SL_MEDIA_POWER_DOWN_PAGE, addr, &data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "low set - read8 failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "low set - read back (data = 0x%X)", data);

wait:
	rtn = sl_media_hw_power_low_wait(media_jack);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "low set - wait failed [%d]", rtn);
		return rtn;
	}

	return 0;
}
