// SPDX-License-Identifier: GPL-2.0
/* Copyright 2026 Hewlett Packard Enterprise Development LP */

#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/version.h>

#if KERNEL_VERSION(6, 12, 0) > LINUX_VERSION_CODE
#include <asm/unaligned.h>
#else
#include <linux/unaligned.h>
#endif

#include "sl_common.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "base/sl_media_eeprom.h"
#include "base/sl_media_log.h"
#include "hw/sl_media_hw_power.h"
#include "hw/sl_media_hw_cable.h"
#include "hw/sl_media_hw_cmis.h"
#include "hw/sl_media_hw_sff.h"

#define LOG_NAME SL_MEDIA_HW_POWER_LOG_NAME

#define SL_MEDIA_POWER_STATE_POLL_MS 20
#define SL_MEDIA_POWER_SFF_WAIT_MS   8000

static const char * const sl_media_hw_power_optical_monitors_strings[] = {
	[SL_MEDIA_TX_BIAS_MON_BIT_NUM]          = "tx-bias",
	[SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM] = "tx-power",
	[SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM] = "rx-power",
};

static const struct sl_str_map sl_media_hw_power_optical_monitors_str_map = {
	.strings     = sl_media_hw_power_optical_monitors_strings,
	.num_strings = ARRAY_SIZE(sl_media_hw_power_optical_monitors_strings),
};

const char *sl_media_hw_power_optical_monitor_name_get(u8 monitor)
{
	if (monitor >= SL_MEDIA_NUM_OPTICAL_MONS)
		return "Unknown";

	return sl_media_hw_power_optical_monitors_str_map.strings[monitor];
}

int sl_media_hw_power_optical_monitors_supported_str(u8 supported, char *supported_str,
						     unsigned int supported_str_size)
{
	return sl_str_map_create(supported, &sl_media_hw_power_optical_monitors_str_map,
				 supported_str, supported_str_size);
}

int sl_media_hw_power_optical_monitors_supported_get(struct sl_media_jack *media_jack,
						     u8 *supported)
{
	int rtn;
	u8  format;
	u8  version;

	sl_media_log_dbg(media_jack, LOG_NAME, "optical monitors get");

	rtn = sl_media_eeprom_format_get(media_jack, &format, &version);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "optical monitors get format get failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "optical monitors get (format = %u %s, version = 0x%X)",
			 format, sl_media_mgmt_if_str(format), version);

	switch (format) {
	case SL_MEDIA_MGMT_IF_SFF8636:
		rtn = sl_media_hw_sff_power_optical_monitors_supported_get(media_jack, supported);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "sff optical monitors supported get failed [%d]", rtn);
			return rtn;
		}

		sl_media_log_dbg(media_jack, LOG_NAME, "optical monitors get (supported = 0x%X)", *supported);
		return 0;
	case SL_MEDIA_MGMT_IF_CMIS:
		rtn = sl_media_hw_cmis_power_optical_monitors_supported_get(media_jack, supported);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "cmis optical monitors supported get failed [%d]", rtn);
			return rtn;
		}

		sl_media_log_dbg(media_jack, LOG_NAME, "optical monitors get (supported = 0x%X)", *supported);
		return 0;
	default:
		sl_media_log_err(media_jack, LOG_NAME,
				 "optical monitors get unknown cable format (format = %u %s)", format,
				 sl_media_mgmt_if_str(format));
		return -EMEDIUMTYPE;
	}
}

static int sl_media_hw_power_optical_monitor_addr_get(struct sl_media_jack *media_jack,
						      u8 monitor, u8 lane, u8 *page, u8 *offset)
{
	int rtn;
	u8  format;
	u8  version;

	rtn = sl_media_eeprom_format_get(media_jack, &format, &version);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "optical monitor addr get format get failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "optical monitor addr get (format = %u %s, version = 0x%X)", format,
			 sl_media_mgmt_if_str(format), version);

	switch (format) {
	case SL_MEDIA_MGMT_IF_CMIS:
		rtn = sl_media_hw_cmis_power_optical_monitor_addr_get(media_jack, monitor, lane, page, offset);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "cmis optical monitor addr get failed [%d]", rtn);
			return rtn;
		}

		return 0;
	case SL_MEDIA_MGMT_IF_SFF8636:
		rtn = sl_media_hw_sff_power_optical_monitor_addr_get(media_jack, monitor, lane, page, offset);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "sff optical monitor addr get failed [%d]", rtn);
			return rtn;
		}

		return 0;
	default:
		sl_media_log_err(media_jack, LOG_NAME,
				 "optical monitor addr get unknown cable format (format = %u %s)",
				 format, sl_media_mgmt_if_str(format));
		return -EMEDIUMTYPE;
	}
}

static int sl_media_hw_power_optical_monitor_get(struct sl_media_jack *media_jack,
						 u8 monitor, u8 lane, u16 *value)
{
	u8  data[sizeof(*value)];
	u8  offset;
	u8  page;
	u8  supported;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "optical monitor get (monitor = 0x%X %s, lane = %u)", monitor,
			 sl_media_hw_power_optical_monitor_name_get(monitor), lane);

	*value = 0;

	rtn = sl_media_hw_power_optical_monitors_supported_get(media_jack, &supported);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "optical monitor get (monitor = 0x%X %s) supported get failed",
				       monitor, sl_media_hw_power_optical_monitor_name_get(monitor));
		return rtn;
	}

	if (!(supported & BIT(monitor))) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "optical monitor get not supported (monitor = 0x%X %s)", monitor,
				       sl_media_hw_power_optical_monitor_name_get(monitor));
		return -EOPNOTSUPP;
	}

	rtn = sl_media_hw_power_optical_monitor_addr_get(media_jack, monitor, lane, &page, &offset);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "optical monitor get addr failed (monitor = 0x%X %s) [%d]",
				       monitor, sl_media_hw_power_optical_monitor_name_get(monitor), rtn);
		return rtn;
	}

	rtn = sl_media_io_read(media_jack, page, offset, data, sizeof(data));
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "optical monitor get failed (monitor = 0x%X %s, page = 0x%X, offset = 0x%X) [%d]",
				       monitor, sl_media_hw_power_optical_monitor_name_get(monitor),
				       page, offset, rtn);
		return rtn;
	}

	/* Monitor data is Big-Endian and needs to be converted. */
	*value = get_unaligned_be16(data);

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "optical monitor get (monitor = 0x%X %s, offset = 0x%X, value = 0x%X)",
			 monitor, sl_media_hw_power_optical_monitor_name_get(monitor), offset, *value);

	return 0;
}

int sl_media_hw_power_tx_get(struct sl_media_jack *media_jack, u8 lane, u16 *tx_power)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "tx power get (lane = %u)", lane);

	return sl_media_hw_power_optical_monitor_get(media_jack, SL_MEDIA_TX_OPTICAL_POWER_MON_BIT_NUM, lane, tx_power);
}

int sl_media_hw_power_tx_bias_get(struct sl_media_jack *media_jack, u8 lane, u32 *tx_bias_ua)
{
	u16 value;
	u8  format;
	u8  version;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "tx bias get (lane = %u)", lane);

	rtn = sl_media_eeprom_format_get(media_jack, &format, &version);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "tx bias get format get failed [%d]", rtn);
		return rtn;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "tx bias get format (format = %u %s, version = %u)",
			 format, sl_media_mgmt_if_str(format), version);

	rtn = sl_media_hw_power_optical_monitor_get(media_jack, SL_MEDIA_TX_BIAS_MON_BIT_NUM, lane, &value);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "tx bias get monitor failed [%d]", rtn);
		return rtn;
	}

	switch (format) {
	case SL_MEDIA_MGMT_IF_SFF8636:
		rtn = sl_media_hw_sff_power_tx_bias_get(media_jack, value, tx_bias_ua);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "sff tx bias get failed [%d]", rtn);
			return rtn;
		}

		return 0;
	case SL_MEDIA_MGMT_IF_CMIS:
		rtn = sl_media_hw_cmis_power_tx_bias_get(media_jack, value, tx_bias_ua);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "cmis tx bias get failed [%d]", rtn);
			return rtn;
		}

		return 0;
	default:
		sl_media_log_err(media_jack, LOG_NAME,
				 "tx bias get unknown cable format (format = %u %s)",
				 format, sl_media_mgmt_if_str(format));
		return -EMEDIUMTYPE;
	}
}

int sl_media_hw_power_rx_get(struct sl_media_jack *media_jack, u8 lane, u16 *rx_power)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "rx power get (lane = %u)", lane);

	return sl_media_hw_power_optical_monitor_get(media_jack, SL_MEDIA_RX_OPTICAL_POWER_MON_BIT_NUM, lane, rx_power);
}

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
