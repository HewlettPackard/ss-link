// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2026 Hewlett Packard Enterprise Development LP */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/string.h>

#include <linux/hpe/sl/sl_media.h>
#include <linux/hsnxcvr-api.h>

#include "sl_asic.h"
#include "base/sl_media_log.h"
#include "base/sl_media_eeprom.h"
#include "sl_media_jack.h"
#include "sl_media_io.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_ldev.h"
#include "data/sl_media_data_lgrp.h"
#include "data/sl_media_data_cable_db_ops.h"
#include "sl_core_link.h"
#include "sl_ctrl_link_priv.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_lgrp_notif.h"

// FIXME: need to separate this file out

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

int sl_media_data_jack_cable_attr_set(struct sl_media_jack *media_jack, struct sl_media_attr *media_attr)
{
	u8  i;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable attr set (jack_num = %u)", media_jack->num);

	for (i = 0; i < media_jack->port_count; ++i) {
		media_jack->cable_info[i].ldev_num = media_jack->media_ldev->num;
		media_jack->cable_info[i].lgrp_num = media_jack->asic_port[i];
		sl_media_log_dbg(media_jack, LOG_NAME,
				 "cable attr set (cable_info = %u, lgrp_num = %u)", i, media_jack->asic_port[i]);
		rtn = sl_media_data_jack_media_attr_set(media_jack, &media_jack->cable_info[i], media_attr);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME, "cable attr set media_attr_set failed [%d]", rtn);
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_MEDIA_ATTR_SET);
			return rtn;
		}
	}

	return 0;
}

void sl_media_data_jack_cable_attr_errors_update(struct sl_media_jack *media_jack, u32 errors)
{
	u8 x;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable attr errors update");

	for (x = 0; x < media_jack->port_count; ++x)
		media_jack->cable_info[x].media_attr.errors |= errors;
}

void sl_media_data_jack_cable_attr_send(struct sl_media_jack *media_jack)
{
	struct sl_media_lgrp *media_lgrp;
	u8                    x;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable attr send");

	for (x = 0; x < media_jack->port_count; ++x) {
		media_lgrp = sl_media_data_lgrp_get(media_jack->cable_info[x].ldev_num,
						    media_jack->cable_info[x].lgrp_num);
		if (media_lgrp)
			sl_media_data_jack_cable_if_present_send(media_lgrp);
	}
}

#define SL_MEDIA_TEMPERATURE_CELSIUS_MIN 10
#define SL_MEDIA_TEMPERATURE_CELSIUS_MAX 200
#define SL_MEDIA_CMIS_TEMP_VALUE_PAGE    0
#define SL_MEDIA_CMIS_TEMP_VALUE_OFFSET  14
#define SL_MEDIA_SFF_TEMP_VALUE_PAGE     0
#define SL_MEDIA_SFF_TEMP_VALUE_OFFSET   22
static int sl_media_data_jack_temp_value_get(struct sl_media_jack *media_jack, u8 *data)
{
	u8  i;
	u8  value;
	u8  page;
	u8  offset;
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp value get");

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		page   = SL_MEDIA_CMIS_TEMP_VALUE_PAGE;
		offset = SL_MEDIA_CMIS_TEMP_VALUE_OFFSET;
	} else {
		page   = SL_MEDIA_SFF_TEMP_VALUE_PAGE;
		offset = SL_MEDIA_SFF_TEMP_VALUE_OFFSET;
	}

	for (i = 0; i < 3; ++i) {
		rtn = sl_media_io_read8(media_jack, page, offset, &value);
		if (rtn)
			continue;

		if (value < SL_MEDIA_TEMPERATURE_CELSIUS_MIN || value > SL_MEDIA_TEMPERATURE_CELSIUS_MAX)
			continue;

		*data = value;
		return 0;
	}

	return -EINVAL;
}

int sl_media_data_jack_lgrp_connect(struct sl_media_lgrp *media_lgrp)
{
	struct sl_media_jack *media_jack;
	u8                    jack_num;
	u8                    i;

	sl_media_log_dbg(media_lgrp, LOG_NAME,
			 "lgrp connect (max_jack_num = %u)", SL_MEDIA_MAX_JACK_NUM);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(media_lgrp->media_ldev->num, jack_num);

		sl_media_log_dbg(media_lgrp, LOG_NAME,
				 "lgrp connect (jack_num = %u, media_jack->port_count = %u)",
				 jack_num, media_jack->port_count);

		for (i = 0; i < media_jack->port_count; ++i) {
			sl_media_log_dbg(media_lgrp, LOG_NAME,
					 "lgrp connect (jack_num = %u, asic_port%u = %u)",
					 jack_num, i, media_jack->asic_port[i]);
			if (media_jack->asic_port[i] == media_lgrp->num) {
				sl_media_log_dbg(media_lgrp, LOG_NAME, "lgrp connect found");
				media_lgrp->media_jack = media_jack;
				media_lgrp->cable_info = &media_jack->cable_info[i];
				return 0;
			}
		}
	}

	sl_media_log_dbg(media_lgrp, LOG_NAME, "lgrp connect not found");
	return -EFAULT;
}

#define SL_MEDIA_DATA_PATH_STATE_DEACTIVATED       0x1
#define SL_MEDIA_DATA_PATH_EXPLICIT_CONTROL_ENABLE 0x00
#define SL_MEDIA_DATA_PATH_ID                      0x08
#define SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG       (SL_MEDIA_DATA_PATH_EXPLICIT_CONTROL_ENABLE)
#define SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG       (SL_MEDIA_DATA_PATH_EXPLICIT_CONTROL_ENABLE | SL_MEDIA_DATA_PATH_ID)
#define SL_MEDIA_DATA_PATH_LANES_DEACTIVATED       (SL_MEDIA_DATA_PATH_STATE_DEACTIVATED << 4 | SL_MEDIA_DATA_PATH_STATE_DEACTIVATED)
int sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack, u8 version)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable downshift");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	/* deinit all lanes */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	if (version == 3)
		i2c_data.data[0] = 0x00;
	else
		i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* enable low power mode */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* Staged Control Set 0, Config lanes 1 to 4 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 145;
	i2c_data.data[0] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/* Config lanes 5 to 8 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 149;
	i2c_data.data[0] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 5-8 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/* ApplyDPInitLane8-1 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 143;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* enable high power mode */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(8000); /* allow firmware load */

	/* ReInit all lanes */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	if (version == 3)
		i2c_data.data[0] = 0xFF;
	else
		i2c_data.data[0] = 0x00;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}

	/* waiting for firmware reload */
	msleep(3000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	return 0;
}

int sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack)
{
	int                  rtn;
	u8                   downshift_lower_lane_config;
	u8                   downshift_upper_lane_config;
	u8                   upshift_lower_lane_config;
	u8                   upshift_upper_lane_config;
	u8                   read_data_lower[4];
	u8                   read_data_upper[4];
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable hw shift state get");

	downshift_lower_lane_config = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	downshift_upper_lane_config = (media_jack->appsel_num_200_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;

	upshift_lower_lane_config = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	upshift_upper_lane_config = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;

	i2c_data.addr   = 0;
	i2c_data.page   = 0x10;
	i2c_data.bank   = 0;
	i2c_data.offset = 145;
	i2c_data.len    = 4;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 1-4 - read failed [%d]", rtn);
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_IO_ERROR;
	}
	memcpy(read_data_lower, i2c_data.data, sizeof(read_data_lower));

	i2c_data.addr   = 0;
	i2c_data.page   = 0x10;
	i2c_data.bank   = 0;
	i2c_data.offset = 149;
	i2c_data.len    = 4;
	rtn = hsnxcvr_i2c_read(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_STATE_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 5-8 - read failed [%d]", rtn);
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_IO_ERROR;
	}
	memcpy(read_data_upper, i2c_data.data, sizeof(read_data_upper));

	if ((read_data_lower[0] == downshift_lower_lane_config) &&
		(read_data_lower[1] == downshift_lower_lane_config) &&
		(read_data_lower[2] == downshift_lower_lane_config) &&
		(read_data_lower[3] == downshift_lower_lane_config) &&
		(read_data_upper[0] == downshift_upper_lane_config) &&
		(read_data_upper[1] == downshift_upper_lane_config) &&
		(read_data_upper[2] == downshift_upper_lane_config) &&
		(read_data_upper[3] == downshift_upper_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is downshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED;
	} else if ((read_data_lower[0] == upshift_lower_lane_config) &&
		(read_data_lower[1] == upshift_lower_lane_config) &&
		(read_data_lower[2] == upshift_lower_lane_config) &&
		(read_data_lower[3] == upshift_lower_lane_config) &&
		(read_data_upper[0] == upshift_upper_lane_config) &&
		(read_data_upper[1] == upshift_upper_lane_config) &&
		(read_data_upper[2] == upshift_upper_lane_config) &&
		(read_data_upper[3] == upshift_upper_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is upshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is in unknown shift state");
	return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UNKNOWN;
}

int sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack, u8 version)
{
	int                  rtn;
	struct xcvr_i2c_data i2c_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable upshift");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	/* Deinit all lanes */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	if (version == 3)
		i2c_data.data[0] = 0x00;
	else
		i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* enable low power mode */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* Staged Control Set 0, Config lanes 1 to 4 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 145;
	i2c_data.data[0] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_LOWER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/* Config lanes 5 to 8 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 149;
	i2c_data.data[0] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[1] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[2] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.data[3] = (media_jack->appsel_num_400_gaui << 4) | SL_MEDIA_DATA_PATH_UPPER_LANE_CONFIG;
	i2c_data.len     = 4;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "SCS0 configuration - config lanes 5-8 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(100);

	/* ApplyDPInitLane8-1 */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 143;
	i2c_data.data[0] = 0xFF;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(500);

	/* enable high power mode */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}
	msleep(8000); /* allow firmware load */

	/* ReInit all lanes */
	i2c_data.addr    = 0;
	i2c_data.page    = 0x10;
	i2c_data.bank    = 0;
	i2c_data.offset  = 128;
	if (version == 3)
		i2c_data.data[0] = 0xFF;
	else
		i2c_data.data[0] = 0x00;
	i2c_data.len     = 1;
	rtn = hsnxcvr_i2c_write(media_jack->hdl, &i2c_data);
	if (rtn) {
		sl_media_jack_io_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
		return rtn;
	}

	/* waiting for firmware reload */
	msleep(3000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	return 0;
}

#define SL_MEDIA_SFF_ENHANCED_OPTIONS_OFFSET         221
#define SL_MEDIA_SFF_ENHANCED_OPTIONS_SOFT_RESET_BIT BIT(0)
#define SL_MEDIA_SFF_SOFT_RESET_ADDR                 93
#define SL_MEDIA_SFF_SOFT_RESET_BIT                  BIT(7)
int sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  read_data;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable soft reset");

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_BUSY);

	if (!sl_media_data_jack_media_is_format_cmis(media_jack)) {
		// FIXME: this needs a getter
		if (!(media_jack->eeprom_page0[SL_MEDIA_SFF_ENHANCED_OPTIONS_OFFSET] &
		      SL_MEDIA_SFF_ENHANCED_OPTIONS_SOFT_RESET_BIT)) {
			sl_media_log_dbg(media_jack, LOG_NAME,
					 "cable soft reset not supported (enhanced_options = 0x%02X)",
					 media_jack->eeprom_page0[SL_MEDIA_SFF_ENHANCED_OPTIONS_OFFSET]);
			sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
			return 0;
		}

		/* read-modify-write to trigger software reset, self-clearing */
		rtn = sl_media_io_read8(media_jack, 0, SL_MEDIA_SFF_SOFT_RESET_ADDR, &read_data);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "cable soft reset read failed [%d]", rtn);
			sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
			return rtn;
		}

		rtn = sl_media_io_write8(media_jack, 0, SL_MEDIA_SFF_SOFT_RESET_ADDR,
					 read_data | SL_MEDIA_SFF_SOFT_RESET_BIT);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "cable soft reset write failed [%d]", rtn);
			sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
			return rtn;
		}
	} else {
		/* CMIS: reset is self clearing */
		rtn = sl_media_io_write8(media_jack, 0x00, 0x1a, 0x08);
		if (rtn) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					       "cable soft reset write failed [%d]", rtn);
			sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);
			return rtn;
		}
	}

	/* wait for firmware reload */
	msleep(8000);

	sl_media_data_jack_headshell_busy_set(media_jack, SL_MEDIA_JACK_HEADSHELL_IDLE);

	sl_media_log_dbg(media_jack, LOG_NAME, "cable soft reset done");

	return 0;
}

#define SL_MEDIA_POWER_UP_PAGE      0x00
#define SL_MEDIA_CMIS_POWER_UP_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_UP_DATA 0x00
#define SL_MEDIA_SFF_POWER_UP_ADDR  0x5D
#define SL_MEDIA_SFF_POWER_UP_DATA  0x0D
int sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  addr;
	u8  data;

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		addr = SL_MEDIA_CMIS_POWER_UP_ADDR;
		data = SL_MEDIA_CMIS_POWER_UP_DATA;
	} else {
		addr = SL_MEDIA_SFF_POWER_UP_ADDR;
		data = SL_MEDIA_SFF_POWER_UP_DATA;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "high power set (addr = 0x%X, data = 0x%X)", addr, data);

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_POWER_UP_PAGE, addr, data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power set write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = true;

	return 0;
}

#define SL_MEDIA_POWER_DOWN_PAGE      0x00
#define SL_MEDIA_CMIS_POWER_DOWN_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_DOWN_DATA 0x10
#define SL_MEDIA_SFF_POWER_DOWN_ADDR  0x5D
#define SL_MEDIA_SFF_POWER_DOWN_DATA  0x00
int sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  addr;
	u8  data;

	if (sl_media_data_jack_media_is_format_cmis(media_jack)) {
		addr = SL_MEDIA_CMIS_POWER_DOWN_ADDR;
		data = SL_MEDIA_CMIS_POWER_DOWN_DATA;
	} else {
		addr = SL_MEDIA_SFF_POWER_DOWN_ADDR;
		data = SL_MEDIA_SFF_POWER_DOWN_DATA;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "low power set (addr = 0x%X, data = 0x%X)", addr, data);

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_POWER_DOWN_PAGE, addr, data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power set write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = false;

	return 0;
}

#define SL_MEDIA_LED_OFF        XCVR_LED_OFF
#define SL_MEDIA_LED_ON_GRN     XCVR_LED_A_STEADY
#define SL_MEDIA_LED_FAST_GRN   XCVR_LED_A_FAST
#define SL_MEDIA_LED_ON_AMBER   XCVR_LED_B_STEADY
#define SL_MEDIA_LED_FAST_AMBER XCVR_LED_B_FAST
// FIXME: check to make sure this function does the correct thing in all cases
void sl_media_data_jack_led_set(struct sl_media_jack *media_jack)
{
	int                   rtn;
	struct sl_core_link  *core_link;
	struct sl_core_lgrp  *core_lgrp;
	u32                   link_state;
	bool                  link_going_up;
	bool                  link_up;
	u8                    link_num;
	u8                    ldev_num;
	u8                    lgrp_num;
	u8                    max_links;
	u8                    i;
	u8		      jack_state;
	u8		      temperature_state;

	sl_media_log_dbg(media_jack, LOG_NAME, "led set");

	rtn = sl_media_data_jack_cable_temp_state_get(media_jack, &temperature_state);
	if (rtn)
		sl_media_log_warn_trace(media_jack, LOG_NAME,
					"led set cable_temp_state_get failed [%d]", rtn);

	if (temperature_state == SL_MEDIA_JACK_TEMP_STATE_HOT) {
		sl_media_log_dbg(media_jack, LOG_NAME, "led set temperature_state HOT");
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_ON_AMBER);
		return;
	}

	rtn = sl_media_jack_state_get(media_jack, &jack_state);
	if (rtn)
		sl_media_log_warn_trace(media_jack, LOG_NAME,
					"led set media_jack_state_get failed [%d]", rtn);

	sl_media_log_dbg(media_jack, LOG_NAME, "led set (jack_state = %u)", jack_state);

	switch (jack_state) {
	case SL_MEDIA_JACK_CABLE_REMOVED:
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_OFF);
		return;
	case SL_MEDIA_JACK_CABLE_ERROR:
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_FAST_AMBER);
		return;
	}

	link_going_up = false;
	link_up       = false;
	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;

		core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);
		if (!core_lgrp)
			continue;

		switch (core_lgrp->config.furcation) {
		case SL_MEDIA_FURCATION_X1:
			max_links = 1;
			break;
		case SL_MEDIA_FURCATION_X2:
			max_links = 2;
			break;
		case SL_MEDIA_FURCATION_X4:
			max_links = 4;
			break;
		default:
			max_links = 0;
		}

		for (link_num = 0; link_num < max_links; ++link_num) {
			core_link = sl_core_link_get(ldev_num, lgrp_num, link_num);
			if (!core_link)
				continue;

			rtn = sl_core_link_state_get(ldev_num, lgrp_num, link_num, &link_state);
			if (rtn) {
				sl_media_log_err_trace(media_jack, LOG_NAME,
						       "led set link_state_get failed (ldev=%u, lgrp=%u, link=%u) [%d] ",
						       ldev_num, lgrp_num, link_num, rtn);
				return;
			}

			switch (link_state) {
			case SL_CORE_LINK_STATE_GOING_UP:
			case SL_CORE_LINK_STATE_AN:
				link_going_up = true;
				break;
			case SL_CORE_LINK_STATE_UP:
				link_up = true;
				break;
			default:
				break;
			}
		}
	}

	if (link_going_up)
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_FAST_GRN);
	else if (link_up)
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_ON_GRN);
	else
		sl_media_io_led_set(media_jack, SL_MEDIA_LED_OFF);
}

static int sl_media_data_jack_cable_hot_link_down(struct sl_media_jack *media_jack)
{
	int                  rtn;
	int                  i;
	struct sl_ctrl_link *ctrl_link;
	u8                   ldev_num;
	u8                   lgrp_num;
	u8                   link_num;

	sl_media_log_dbg(media_jack, LOG_NAME, "cable hot link down");

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;
		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {
			ctrl_link = sl_ctrl_link_get(ldev_num, lgrp_num, link_num);
			if (!ctrl_link)
				continue;
			if (!sl_ctrl_link_kref_get_unless_zero(ctrl_link)) {
				sl_media_log_dbg(media_jack, LOG_NAME,
						 "cable hot link down kref unavailable (ctrl_link = 0x%p)", ctrl_link);
				continue;
			}

			rtn = sl_ctrl_link_async_down(ctrl_link, SL_LINK_DOWN_CAUSE_MEDIA_HOT_FAULT_MAP, true);
			if (rtn)
				sl_media_log_err_trace(media_jack, LOG_NAME,
						       "cable hot link down async_down failed [%d]", rtn);
			if (sl_ctrl_link_put(ctrl_link))
				sl_media_log_dbg(media_jack, LOG_NAME,
						 "cable hot link down link removed (link = 0x%p)", ctrl_link);
		}
	}
	return 0;
}

#define SL_MEDIA_TEMP_MONITOR_TIME_MS 15000
static void sl_media_data_jack_cable_monitor_temp_delayed_work(struct work_struct *work)
{
	int                   rtn;
	u8                    jack_num;
	u8                    prev_temp_state;
	u8                    curr_temp_state;
	struct sl_media_jack *media_jack;
	struct sl_media_ldev *media_ldev;
	struct delayed_work  *delayed_work_ptr;

	delayed_work_ptr  = container_of(work, struct delayed_work, work);
	media_ldev = container_of(delayed_work_ptr, struct sl_media_ldev,
				  delayed_work[SL_MEDIA_WORK_CABLE_MON_TEMP]);

	sl_media_log_dbg(media_ldev, LOG_NAME, "cable monitor temp work (ldev = 0x%p)", media_ldev);

	sl_media_data_ldev_temp_mon_state_set(media_ldev, SL_MEDIA_TEMP_MON_CHECKING);

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_data_jack_get(media_ldev->num, jack_num);
		if (!media_jack)
			continue;

		if (sl_media_data_jack_is_headshell_busy(media_jack))
			continue;

		if (!sl_media_lgrp_get(media_jack->cable_info[0].ldev_num, media_jack->cable_info[0].lgrp_num))
			continue;

		if (!SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(media_jack->cable_info[0].media_attr.type)) {
			sl_media_log_dbg(media_ldev, LOG_NAME,
					 "cable monitor temp work not active cable (jack_num = %u)", jack_num);
			continue;
		}

		rtn = sl_media_data_jack_cable_temp_state_get(media_jack, &prev_temp_state);
		if (rtn) {
			sl_media_log_warn_trace(media_jack, LOG_NAME,
						"cable monitor temp work cable_temp_state_get failed [%d]", rtn);
			continue;
		}

		curr_temp_state = sl_media_data_jack_cable_temp_hw_check(media_jack);

		if (curr_temp_state == SL_MEDIA_JACK_TEMP_STATE_COLD) {
			if (prev_temp_state == SL_MEDIA_JACK_TEMP_STATE_COLD) {
				sl_media_data_jack_cable_cold_notif_send(media_jack);
				continue;
			}

			if (!sl_media_jack_is_high_powered(media_jack)) {
				rtn = sl_media_jack_cable_high_power_set(media_jack->media_ldev->num, media_jack->num);
				if (rtn) {
					sl_media_log_err_trace(media_jack, LOG_NAME,
							       "cable monitor temp work high power set failed [%d]", rtn);
					sl_media_jack_state_set(media_jack, SL_MEDIA_JACK_CABLE_ERROR);
					sl_media_data_jack_led_set(media_jack);
					sl_media_data_jack_cable_attr_errors_update(media_jack,
										    SL_MEDIA_ERROR_CABLE_HEADSHELL_FAULT);
					sl_media_data_jack_cable_attr_send(media_jack);
				}
			}

			sl_media_log_warn(media_jack, LOG_NAME,
					  "cable cold alert (temperature = %dc, down_limit = %dc)",
					  media_jack->temperature_value_c, media_jack->temperature_down_limit_c);

			sl_media_jack_fault_cause_clr(media_jack);
			sl_media_data_jack_cable_cold_notif_send(media_jack);
			sl_media_data_jack_cable_temp_state_set(media_jack, SL_MEDIA_JACK_TEMP_STATE_COLD);
			continue;
		}

		if (curr_temp_state == SL_MEDIA_JACK_TEMP_STATE_WARM) {
			if (prev_temp_state == SL_MEDIA_JACK_TEMP_STATE_WARM) {
				sl_media_data_jack_cable_warm_notif_send(media_jack);
				continue;
			}

			sl_media_log_warn(media_jack, LOG_NAME,
					  "cable warm alert (temperature = %dc, limit = %dc)",
					  media_jack->temperature_value_c, media_jack->temperature_warn_limit_c);

			sl_media_data_jack_cable_warm_notif_send(media_jack);
			sl_media_data_jack_cable_temp_state_set(media_jack, SL_MEDIA_JACK_TEMP_STATE_WARM);
			continue;
		}

		if (curr_temp_state == SL_MEDIA_JACK_TEMP_STATE_HOT) {
			if (prev_temp_state == SL_MEDIA_JACK_TEMP_STATE_HOT) {
				sl_media_data_jack_cable_hot_notif_send(media_jack);
				continue;
			}

			sl_media_log_warn(media_jack, LOG_NAME,
					  "cable hot alert (temperature = %dc, limit = %dc)",
					  media_jack->temperature_value_c, media_jack->temperature_down_limit_c);

			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_HOT);
			sl_media_data_jack_cable_hot_notif_send(media_jack);
			sl_media_data_jack_cable_temp_state_set(media_jack, SL_MEDIA_JACK_TEMP_STATE_HOT);

			rtn = sl_media_data_jack_cable_hot_link_down(media_jack);
			if (rtn)
				sl_media_log_err_trace(media_ldev, LOG_NAME,
						       "cable monitor temp work hot_link_down is failed [%d]", rtn);
			continue;
		}

		sl_media_data_jack_cable_temp_state_set(media_jack, curr_temp_state);
	}

	queue_delayed_work(media_ldev->workqueue, &media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_TEMP],
			   msecs_to_jiffies(SL_MEDIA_TEMP_MONITOR_TIME_MS));

	sl_media_data_ldev_temp_mon_state_set(media_ldev, SL_MEDIA_TEMP_MON_SLEEPING);
}

bool sl_media_data_jack_cable_is_hot_client_ready(struct sl_media_jack *media_jack,
						  struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_client_ready;

	spin_lock(&media_jack->data_lock);
	is_client_ready = cable_info->hot_client_ready;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is hot client ready (ready = %s)", is_client_ready ?
			 "yes" : "no");

	return is_client_ready;
}

bool sl_media_data_jack_cable_is_warm_client_ready(struct sl_media_jack *media_jack,
						   struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_client_ready;

	spin_lock(&media_jack->data_lock);
	is_client_ready = cable_info->warm_client_ready;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is warm client ready (ready = %s)", is_client_ready ?
			 "yes" : "no");

	return is_client_ready;
}

bool sl_media_data_jack_cable_is_cold_client_ready(struct sl_media_jack *media_jack,
						   struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_client_ready;

	spin_lock(&media_jack->data_lock);
	is_client_ready = cable_info->cold_client_ready;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is cold client ready (ready = %s)", is_client_ready ?
			 "yes" : "no");

	return is_client_ready;
}

#define SL_MEDIA_TEMPERATURE_CELSIUS_SLOPE 10
int sl_media_data_jack_cable_temp_hw_check(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  current_temp_c;
	int prev_temp_c;

	sl_media_log_dbg(media_jack, LOG_NAME, "temp hw check");

	rtn = sl_media_data_jack_temp_value_get(media_jack, &current_temp_c);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "temp hw check temp_value_get failed [%d]", rtn);
		media_jack->temperature_value_c = -1;
		return SL_MEDIA_JACK_TEMP_STATE_UNKNOWN_IO;
	}

	sl_media_log_dbg(media_jack, LOG_NAME,
			 "temp hw check (temperature = 0x%X, warn limit = 0x%x, down limit = 0x%X)",
			 current_temp_c, media_jack->temperature_warn_limit_c, media_jack->temperature_down_limit_c);

	prev_temp_c = media_jack->temperature_value_c;
	media_jack->temperature_value_c = current_temp_c;

	if (prev_temp_c < 0)
		goto out;

	if (current_temp_c < prev_temp_c - SL_MEDIA_TEMPERATURE_CELSIUS_SLOPE ||
	    current_temp_c > prev_temp_c + SL_MEDIA_TEMPERATURE_CELSIUS_SLOPE) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
				       "temperature hw check slope failure (temperature = %dc, previous = %dc, slope = %dc)",
				       current_temp_c, prev_temp_c, SL_MEDIA_TEMPERATURE_CELSIUS_SLOPE);
		return SL_MEDIA_JACK_TEMP_STATE_UNKNOWN_SLOPE;
	}

out:
	if (current_temp_c > media_jack->temperature_down_limit_c)
		return SL_MEDIA_JACK_TEMP_STATE_HOT;

	if (current_temp_c > media_jack->temperature_warn_limit_c &&
	    current_temp_c <= media_jack->temperature_down_limit_c)
		return SL_MEDIA_JACK_TEMP_STATE_WARM;

	return SL_MEDIA_JACK_TEMP_STATE_COLD;
}

bool sl_media_data_jack_cable_is_hot_notif_sent(struct sl_media_jack *media_jack,
						struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_notif_sent;

	spin_lock(&media_jack->data_lock);
	is_notif_sent = cable_info->hot_notif_sent;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is cold notif sent (sent = %s)", is_notif_sent ? "yes" : "no");

	return is_notif_sent;
}

bool sl_media_data_jack_cable_is_warm_notif_sent(struct sl_media_jack *media_jack,
						 struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_notif_sent;

	spin_lock(&media_jack->data_lock);
	is_notif_sent = cable_info->warm_notif_sent;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is warm temp notif sent (sent = %s)", is_notif_sent ? "yes" : "no");

	return is_notif_sent;
}

bool sl_media_data_jack_cable_is_cold_notif_sent(struct sl_media_jack *media_jack,
						 struct sl_media_lgrp_cable_info *cable_info)
{
	bool is_notif_sent;

	spin_lock(&media_jack->data_lock);
	is_notif_sent = cable_info->cold_notif_sent;
	spin_unlock(&media_jack->data_lock);

	sl_media_log_dbg(media_jack, LOG_NAME, "is cold notif sent (sent = %s)", is_notif_sent ? "yes" : "no");

	return is_notif_sent;
}

int sl_media_data_jack_cable_temp_get(struct sl_media_jack *media_jack, u8 *temp)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable temp get");

	if (!sl_media_lgrp_media_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable temp get not active cable");
		return -EBADRQC;
	}

	if (media_jack->temperature_value_c < 0)
		return -EIO;

	*temp = media_jack->temperature_value_c;
	return 0;
}

int sl_media_data_jack_cable_temp_warn_limit_get(struct sl_media_jack *media_jack, u8 *temp_warn_limit_c)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable temp warn limit get");

	if (!sl_media_lgrp_media_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable temp warn limit get not active cable");
		return -EBADRQC;
	}

	if (media_jack->temperature_warn_limit_c < 0)
		return -EIO;

	*temp_warn_limit_c = media_jack->temperature_warn_limit_c;
	return 0;
}

int sl_media_data_jack_cable_temp_down_limit_get(struct sl_media_jack *media_jack, u8 *temp_down_limit_c)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cable temp down limit get");

	if (!sl_media_lgrp_media_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable temp down limit get not active cable");
		return -EBADRQC;
	}

	if (media_jack->temperature_down_limit_c < 0)
		return -EIO;

	*temp_down_limit_c = media_jack->temperature_down_limit_c;
	return 0;
}

void sl_media_data_jack_cable_hot_notif_send(struct sl_media_jack *media_jack)
{
	u8  i;
	u8  ldev_num;
	u8  lgrp_num;
	int rtn;

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;
		if (sl_media_data_jack_cable_is_hot_client_ready(media_jack, &media_jack->cable_info[i]) &&
		    !sl_media_data_jack_cable_is_hot_notif_sent(media_jack, &media_jack->cable_info[i])) {
			sl_media_log_dbg(media_jack, LOG_NAME, "cable hot notif send");
			rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(ldev_num, lgrp_num),
							 SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_HOT,
							 NULL, 0);
			if (rtn) {
				sl_media_log_warn_trace(media_jack, LOG_NAME,
							"cable hot notif send enqueue failed [%d]",
							rtn);
			} else {
				sl_media_data_jack_cable_hot_notif_sent_set(media_jack, &media_jack->cable_info[i],
									    true);
				sl_media_data_jack_cable_warm_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     false);
				sl_media_data_jack_cable_cold_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     false);
			}
		}
	}
}

void sl_media_data_jack_cable_warm_notif_send(struct sl_media_jack *media_jack)
{
	u8  i;
	u8  ldev_num;
	u8  lgrp_num;
	int rtn;

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;
		if (sl_media_data_jack_cable_is_warm_client_ready(media_jack, &media_jack->cable_info[i]) &&
		    !sl_media_data_jack_cable_is_warm_notif_sent(media_jack, &media_jack->cable_info[i])) {
			sl_media_log_dbg(media_jack, LOG_NAME, "cable warm notif send");
			rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(ldev_num, lgrp_num),
							 SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_WARM,
							 NULL, 0);
			if (rtn) {
				sl_media_log_warn_trace(media_jack, LOG_NAME,
							"cable warm notif send enqueue failed [%d]",
							rtn);
			} else {
				sl_media_data_jack_cable_warm_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     true);
				sl_media_data_jack_cable_hot_notif_sent_set(media_jack, &media_jack->cable_info[i],
									    false);
				sl_media_data_jack_cable_cold_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     false);
			}
		}
	}
}

void sl_media_data_jack_cable_cold_notif_send(struct sl_media_jack *media_jack)
{
	u8  i;
	u8  ldev_num;
	u8  lgrp_num;
	int rtn;

	for (i = 0; i < media_jack->port_count; ++i) {
		ldev_num = media_jack->cable_info[i].ldev_num;
		lgrp_num = media_jack->cable_info[i].lgrp_num;
		if (sl_media_data_jack_cable_is_cold_client_ready(media_jack, &media_jack->cable_info[i]) &&
		    !sl_media_data_jack_cable_is_cold_notif_sent(media_jack, &media_jack->cable_info[i])) {
			sl_media_log_dbg(media_jack, LOG_NAME, "cable cold notif send");
			rtn = sl_ctrl_lgrp_notif_enqueue(sl_ctrl_lgrp_get(ldev_num, lgrp_num),
							 SL_LGRP_NOTIF_NO_LINK, SL_LGRP_NOTIF_MEDIA_COLD,
							 NULL, 0);
			if (rtn) {
				sl_media_log_warn_trace(media_jack, LOG_NAME,
							"cold notif send enqueue failed [%d]",
							rtn);
			} else {
				sl_media_data_jack_cable_cold_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     true);
				sl_media_data_jack_cable_warm_notif_sent_set(media_jack, &media_jack->cable_info[i],
									     false);
				sl_media_data_jack_cable_hot_notif_sent_set(media_jack, &media_jack->cable_info[i],
									    false);
			}
		}
	}
}

void sl_media_data_jack_cable_hot_notif_sent_set(struct sl_media_jack *media_jack,
						 struct sl_media_lgrp_cable_info *cable_info, bool value)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "hot notif sent set (value = %s)", value ? "true" : "false");

	spin_lock(&media_jack->data_lock);
	cable_info->hot_notif_sent = value;
	spin_unlock(&media_jack->data_lock);
}

void sl_media_data_jack_cable_warm_notif_sent_set(struct sl_media_jack *media_jack,
						  struct sl_media_lgrp_cable_info *cable_info, bool value)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "warm notif sent set (value = %s)", value ? "true" : "false");

	spin_lock(&media_jack->data_lock);
	cable_info->warm_notif_sent = value;
	spin_unlock(&media_jack->data_lock);
}

void sl_media_data_jack_cable_cold_notif_sent_set(struct sl_media_jack *media_jack,
						  struct sl_media_lgrp_cable_info *cable_info, bool value)
{
	sl_media_log_dbg(media_jack, LOG_NAME, "cold notif sent set (value = %s)", value ?
			 "true" : "false");

	spin_lock(&media_jack->data_lock);
	cable_info->cold_notif_sent = value;
	spin_unlock(&media_jack->data_lock);
}

void sl_media_data_jack_cable_temp_monitor_start(struct sl_media_ldev *media_ldev)
{
	sl_media_log_dbg(media_ldev, LOG_NAME, "cable temp monitor start");

	INIT_DELAYED_WORK(&media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_TEMP],
			  sl_media_data_jack_cable_monitor_temp_delayed_work);

	queue_delayed_work(media_ldev->workqueue, &media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_TEMP],
			   msecs_to_jiffies(SL_MEDIA_TEMP_MONITOR_TIME_MS));
}

void sl_media_data_jack_cable_temp_monitor_stop(struct sl_media_ldev *media_ldev)
{
	cancel_delayed_work_sync(&media_ldev->delayed_work[SL_MEDIA_WORK_CABLE_MON_TEMP]);
}
