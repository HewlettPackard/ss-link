// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/delay.h>

#include "sl_asic.h"

#include "base/sl_media_log.h"

#include "sl_media_io.h"
#include "sl_media_jack.h"
#include "sl_media_data_jack.h"
#include "sl_media_data_jack_cassini.h"

#define LOG_NAME SL_MEDIA_DATA_JACK_LOG_NAME

int sl_media_data_jack_scan(u8 ldev_num)
{
	struct sl_media_jack *media_jack;

	sl_media_log_dbg(NULL, LOG_NAME, "jack scan");

	media_jack = sl_media_data_jack_get(ldev_num, 0);
	media_jack->cable_info[0].ldev_num = ldev_num;
	media_jack->cable_info[0].lgrp_num = 0; /* hardcoding 0 since only one lgrp*/

	return 0;
}


int sl_media_data_jack_fake_media_attr_set(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info, struct sl_media_attr *fake_media_attr)
{
	return 0;
}

void sl_media_data_jack_fake_media_attr_clr(struct sl_media_jack *media_jack,
		struct sl_media_lgrp_cable_info *cable_info)
{

}

#define DATA_PATH_STATE_DEACTIVATED       0x1
#define DATA_PATH_EXPLICIT_CONTROL_ENABLE 0x01
#define DATA_PATH_ID                      0x08
#define DATA_PATH_LOWER_LANE_CONFIG       (DATA_PATH_EXPLICIT_CONTROL_ENABLE)
#define DATA_PATH_LANES_DEACTIVATED       (DATA_PATH_STATE_DEACTIVATED << 4 | DATA_PATH_STATE_DEACTIVATED)
int sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack)
{
	int rtn;
	int i;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable downshift");

	/*
	 * Deinit all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0xFF);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * enable low power mode
	 */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * Staged Control Set 0, Data Path Configuration bytes @ page 0x10 bytes 145-148
	 * Config lanes 1 to 4
	 */
	for (i = 0; i < 4; ++i) {
		rtn = sl_media_io_write8(media_jack, 0x10, 145 + i,
				        (media_jack->appsel_no_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
			sl_media_log_err_trace(media_jack, LOG_NAME,
					 "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
			return rtn;
		}
	}
	msleep(100);

	/*
	 * ApplyDPInitLane8-1
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 143, 0xFF);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * enable high power mode
	 */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * (Re)Init all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0x00);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_DOWN_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(2000);

	return 0;
}

int sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack)
{
	int rtn;
	int i;
	u8  downshift_lower_lane_config;
	u8  upshift_lower_lane_config;
	u8  read_data[4];

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable hw shift state get");

	downshift_lower_lane_config = (media_jack->appsel_no_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;
	upshift_lower_lane_config = (media_jack->appsel_no_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;

	for (i = 0; i < 4; ++i) {
		rtn = sl_media_io_read8(media_jack, 0x10, 145 + i, &read_data[i]);
		if (rtn != sizeof(read_data[0])) {
			sl_media_log_err_trace(media_jack, LOG_NAME,
					 "SCS0 configuration - config lanes 1-4 - read failed [%d]", rtn);
			return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
		}
	}

	if ((read_data[0] == downshift_lower_lane_config) &&
	    (read_data[1] == downshift_lower_lane_config) &&
	    (read_data[2] == downshift_lower_lane_config) &&
	    (read_data[3] == downshift_lower_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is downshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED;
	} else if ((read_data[0] == upshift_lower_lane_config) &&
		   (read_data[1] == upshift_lower_lane_config) &&
		   (read_data[2] == upshift_lower_lane_config) &&
		   (read_data[3] == upshift_lower_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is upshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is in invalid state");
	return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
}

int sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack)
{
	int rtn;
	int i;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable upshift");

	/*
	 * Deinit all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0xFF);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * enable low power mode
	 */
	rtn = sl_media_data_jack_cable_low_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_LOW_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "low power mode - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * Staged Control Set 0, Data Path Configuration bytes @ page 0x10 bytes 145-148
	 * Config lanes 1 to 4
	 */
	for (i = 0; i < 4; ++i) {
		rtn = sl_media_io_write8(media_jack, 0x10, 145 + i,
					(media_jack->appsel_no_400_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG);
		if (rtn) {
			sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
			sl_media_log_err_trace(media_jack, LOG_NAME,
					 "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
			return rtn;
		}
	}
	msleep(100);

	/*
	 * ApplyDPInitLane8-1
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 143, 0xFF);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * enable high power mode
	 */
	rtn = sl_media_data_jack_cable_high_power_set(media_jack);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_HIGH_POWER_SET);
		sl_media_log_err_trace(media_jack, LOG_NAME, "high power mode - write failed [%d]", rtn);
		return rtn;
	}
	msleep(500);

	/*
	 * (Re)Init all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0x00);
	if (rtn) {
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_SHIFT_UP_JACK_IO);
		sl_media_log_err_trace(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(2000);

	return 0;
}

int sl_media_data_jack_cable_soft_reset(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable soft reset");

	rtn = sl_media_io_write8(media_jack, 0x00, 0x1a, 0x08);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "soft reset - write failed [%d] (0x1a <- 0x08)", rtn);
		return rtn;
	}

	msleep(500);

	rtn = sl_media_io_write8(media_jack, 0x00, 0x1a, 0x00);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "soft reset - write failed [%d] (0x1a <- 0x00)", rtn);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(5000);

	return 0;
}

#define SL_MEDIA_CMIS_POWER_UP_PAGE 0x00
#define SL_MEDIA_CMIS_POWER_UP_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_UP_DATA 0x00
int sl_media_data_jack_cable_high_power_set(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "high power set");

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_CMIS_POWER_UP_PAGE,
	      SL_MEDIA_CMIS_POWER_UP_ADDR, SL_MEDIA_CMIS_POWER_UP_DATA);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = true;

	return 0;
}

#define SL_MEDIA_CMIS_POWER_DOWN_PAGE 0x00
#define SL_MEDIA_CMIS_POWER_DOWN_ADDR 0x1a
#define SL_MEDIA_CMIS_POWER_DOWN_DATA 0x10
int sl_media_data_jack_cable_low_power_set(struct sl_media_jack *media_jack)
{
	int rtn;

	sl_media_log_dbg(media_jack, LOG_NAME, "low power set");

	rtn = sl_media_io_write8(media_jack, SL_MEDIA_CMIS_POWER_DOWN_PAGE,
	      SL_MEDIA_CMIS_POWER_DOWN_ADDR, SL_MEDIA_CMIS_POWER_DOWN_DATA);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME, "write8 failed [%d]", rtn);
		return -EIO;
	}

	media_jack->is_high_powered = false;

	return 0;
}

bool sl_media_data_jack_cable_is_high_temp(struct sl_media_jack *media_jack)
{
	int rtn;
	u8  data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable is high temp");

	if (!sl_media_lgrp_cable_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num))
		return false;

	rtn = sl_media_io_read8(media_jack, 0, 9, &data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
			"high temp page read failed [%d]", rtn);
		sl_media_jack_fault_cause_set(media_jack, SL_MEDIA_FAULT_CAUSE_HIGH_TEMP_JACK_IO);
		return false;
	}

	return ((data & SL_MEDIA_JACK_CABLE_HIGH_TEMP_ALARM_MASK) != 0);
}

int sl_media_data_jack_cable_temp_get(struct sl_media_jack *media_jack, u8 *temp)
{
	int rtn;
	u8  data;

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable temp get");

	if (!sl_media_lgrp_cable_type_is_active(media_jack->cable_info[0].ldev_num,
						media_jack->cable_info[0].lgrp_num))
		return -EBADRQC;

	rtn = sl_media_io_read8(media_jack, 0, 14, &data);
	if (rtn) {
		sl_media_log_err_trace(media_jack, LOG_NAME,
			"temp get read failed [%d]", rtn);
		return -EIO;
	}

	*temp = data;
	return 0;
}
