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
#define DATA_PATH_INIT_TIMEOUT_S          200
#define DATA_PATH_EXPLICIT_CONTROL_ENABLE 0x01
#define DATA_PATH_ID                      0x08
#define DATA_PATH_LOWER_LANE_CONFIG       (DATA_PATH_EXPLICIT_CONTROL_ENABLE)
#define DATA_PATH_LANES_DEACTIVATED       (DATA_PATH_STATE_DEACTIVATED << 4 | DATA_PATH_STATE_DEACTIVATED)
int sl_media_data_jack_cable_downshift(struct sl_media_jack *media_jack)
{
	int rtn;
	int i;
	u8  count;
	u8  read_data[4];

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable downshift");

	/*
	 * Deinit all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0xFF);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "data path deinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}

	count = 0;
	while (1) {
		/*
		 * Allow up to 2 seconds (normally observed <= 10ms)
		 */
		if (count++ >= DATA_PATH_INIT_TIMEOUT_S) {
			sl_media_log_err(media_jack, LOG_NAME, "deinit - timed out");
			return -ETIMEDOUT;
		}

		usleep_range(10000, 11000);

		/*
		 * Read data path states (page 0x11, bytes 128-129, 2 lanes per byte)
		 */
		for (i = 0; i < 2; ++i) {
			rtn = sl_media_io_read8(media_jack, 0x11, 128 + i, &read_data[i]);
			if (rtn != sizeof(read_data[0])) {
				sl_media_log_err(media_jack, LOG_NAME, "data path states read failed [%d]", rtn);
				return rtn;
			}
		}

		/*
		 * If all lanes are deactivated, we can proceed
		 */
		if ((read_data[0] == DATA_PATH_LANES_DEACTIVATED) &&
		    (read_data[1] == DATA_PATH_LANES_DEACTIVATED)) {
			sl_media_log_dbg(media_jack, LOG_NAME, "all lanes deactivated");
			break;
		}
	}

	/*
	 * Staged Control Set 0, Data Path Configuration bytes @ page 0x10 bytes 145-148
	 * Config lanes 1 to 4
	 */
	for (i = 0; i < 4; ++i) {
		rtn = sl_media_io_write8(media_jack, 0x10, 145 + i,
				        (media_jack->appsel_no_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG);
		if (rtn) {
			sl_media_log_err(media_jack, LOG_NAME,
					 "SCS0 configuration - config lanes 1-4 - write failed [%d]", rtn);
			return rtn;
		}
	}

	/*
	 * ApplyDPInitLane8-1
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 143, 0xFF);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "apply dpinit = 0xFF - write failed [%d]", rtn);
		return rtn;
	}

	count = 0;
	while (1) {
		/*
		 * Allow up to 2 seconds (normally observed <= 10ms)
		 */
		if (count++ >= DATA_PATH_INIT_TIMEOUT_S) {
			sl_media_log_err(media_jack, LOG_NAME, "config success - timed out");
			return -ETIMEDOUT;
		}

		usleep_range(10000, 11000);

		/*
		 * Read Configuration Command Execution and Result Status Codes
		 * (page 0x11, bytes 202-203, 2 lanes per byte)
		 */
		for (i = 0; i < 2; ++i) {
			rtn = sl_media_io_read8(media_jack, 0x11, 202 + i, &read_data[i]);
			if (rtn != sizeof(read_data[0])) {
				sl_media_log_err(media_jack, LOG_NAME, "data path states read failed [%d]", rtn);
				return rtn;
			}
		}

		/*
		 * If all lanes are ConfigSuccess (0x1), we can proceed
		 */
		if ((read_data[0] == 0x11) &&
		    (read_data[1] == 0x11)) {
			sl_media_log_dbg(media_jack, LOG_NAME, "config success");
			break;
		}
	}

	/*
	 * (Re)Init all lanes (DataPathDeinit @ page 0x10 byte 128)
	 */
	rtn = sl_media_io_write8(media_jack, 0x10, 128, 0x00);
	if (rtn) {
		sl_media_log_err(media_jack, LOG_NAME, "data path deinit = 0x00 - write failed [%d]", rtn);
		return rtn;
	}

	/*
	 * waiting for firmware reload
	 */
	msleep(500);

	return 0;
}

int sl_media_data_jack_cable_hw_shift_state_get(struct sl_media_jack *media_jack)
{
	int rtn;
	int i;
	u8  lower_lane_config;
	u8  read_data[4];

	sl_media_log_dbg(media_jack, LOG_NAME, "data jack cable hw shift state get");

	lower_lane_config = (media_jack->appsel_no_200_gaui << 4) | DATA_PATH_LOWER_LANE_CONFIG;

	for (i = 0; i < 4; ++i) {
		rtn = sl_media_io_read8(media_jack, 0x10, 145 + i, &read_data[i]);
		if (rtn != sizeof(read_data[0])) {
			sl_media_log_err(media_jack, LOG_NAME,
					 "SCS0 configuration - config lanes 1-4 - read failed [%d]", rtn);
			return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_INVALID;
		}
	}

	if ((read_data[0] != lower_lane_config) ||
	    (read_data[1] != lower_lane_config) ||
	    (read_data[2] != lower_lane_config) ||
	    (read_data[3] != lower_lane_config)) {
		sl_media_log_dbg(media_jack, LOG_NAME, "cable is upshifted");
		return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_UPSHIFTED;
	}

	sl_media_log_dbg(media_jack, LOG_NAME, "cable is downshifted");
	return SL_MEDIA_JACK_CABLE_HW_SHIFT_STATE_DOWNSHIFTED;
}

/*
 * TODO: implement upshifting on Cassini
 */
int sl_media_data_jack_cable_upshift(struct sl_media_jack *media_jack)
{
	return 0;
}
