// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "base/sl_ctl_log.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_fec.h"

#define LOG_NAME SL_CTL_LINK_FEC_LOG_NAME

int sl_ctl_link_fec_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_fec_info *fec_info)
{
	struct sl_ctl_link *ctl_link;
	int                 x;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"info get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	*fec_info = sl_ctl_link_fec_data_info_get(ctl_link);

	sl_ctl_log_dbg(ctl_link, LOG_NAME,
		"info get (ucw = %llu, ccw = %llu, gcw = %llu, period = %ums)",
		fec_info->ucw, fec_info->ccw, fec_info->gcw, fec_info->period_ms);
	for (x = 0; x < SL_CTL_NUM_FEC_LANES; ++x)
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "info get (lane %d = 0x%llX)", x, fec_info->lanes[x]);

	return 0;
}

int sl_ctl_link_fec_tail_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_fec_tail *fec_tail)
{
	struct sl_ctl_link *ctl_link;
	int                 x;

	ctl_link = sl_ctl_link_get(ldev_num, lgrp_num, link_num);
	if (!ctl_link) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"tail get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	*fec_tail = sl_ctl_link_fec_data_tail_get(ctl_link);

	sl_ctl_log_dbg(ctl_link, LOG_NAME, "tail get (period = %ums))", fec_tail->period_ms);
	for (x = 0; x < SL_CTL_NUM_CCW_BINS; ++x)
		sl_ctl_log_dbg(ctl_link, LOG_NAME, "tail get (bin %d = %llu)", x, fec_tail->ccw_bins[x]);

	return 0;
}

#define SL_CTL_SERDES_RATE_25	25781250000
#define SL_CTL_SERDES_RATE_50	53125000000
#define SL_CTL_SERDES_RATE_100	106250000000

/* CCW BER Calculation Multiplier (10^6). */
#define SL_CTL_LINK_FEC_CCW_BER_MULT 1000000
#define SL_CTL_LINK_FEC_CCW_BER_EXP 6

/* UCW BER Calculation Multiplier (10^12). */
#define SL_CTL_LINK_FEC_UCW_BER_MULT 1000000000000
#define SL_CTL_LINK_FEC_UCW_BER_EXP 12

#define MAX_CCW_COUNT (U64_MAX / (HZ * SL_CTL_LINK_FEC_CCW_BER_MULT))
#define MAX_UCW_COUNT (U64_MAX / (HZ * SL_CTL_LINK_FEC_UCW_BER_MULT))

int sl_ctl_link_fec_ber_calc(struct sl_fec_info *fec_info, struct sl_ber *ucw_ber, struct sl_ber *ccw_ber)
{
	u64 total;
	u64 ccw_rate;	/* Rate * 10^10 */
	u64 ucw_rate;	/* Rate * 10^10 */
	int mag;

	sl_ctl_log_dbg(NULL, LOG_NAME, "fec_ber_calc");

	/* Total must be > 0 to calculate */
	total = fec_info->ccw + fec_info->gcw;
	if (total == 0) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"fec invalid (total = %llu)", total);
		return -EINVAL;
	}

	if (fec_info->ccw > MAX_CCW_COUNT) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"fec ccw invalid (%llu > %llu)", fec_info->ccw, MAX_CCW_COUNT);
		return -EINVAL;
	}

	if (fec_info->ucw > MAX_UCW_COUNT) {
		sl_ctl_log_err(NULL, LOG_NAME,
			"fec ucw invalid (%llu > %llu)", fec_info->ucw, MAX_UCW_COUNT);
		return -EINVAL;
	}

	/* B = BER
	 * n = [U/C]CW count
	 * g = Good CW count
	 * T = total CW = ccw_count + g
	 * t = time = (jiffies / HZ)
	 *
	 * B = (n/T) / t; B = (n * HZ) / (T * jiffies)
	 */

	/* We use RS(544/514) with 10 bit symbols which is a total of 5440 bits per CW. */
	total = fec_info->ccw + (fec_info->gcw * 5440);

	/* CCW count * CCW multiplier */
	ccw_rate = (fec_info->ccw * HZ * SL_CTL_LINK_FEC_CCW_BER_MULT) / (total * fec_info->period_ms);

	/* Keep at least 2 significant digits */
	mag = 0;
	while ((ccw_rate / 100) > 9) {
		ccw_rate /= 100;
		mag += 2;
	}

	ccw_ber->mant = ccw_rate;
	ccw_ber->exp = mag - SL_CTL_LINK_FEC_CCW_BER_EXP;	/* Divide by CCW multiplier */

	/* UCW count * UCW multiplier */
	ucw_rate = (fec_info->ucw * HZ * SL_CTL_LINK_FEC_UCW_BER_MULT) / (total * fec_info->period_ms);

	/* Keep at least 2 significant digits */
	mag = 0;
	while ((ucw_rate / 100) > 9) {
		ucw_rate /= 100;
		mag += 2;
	}

	ucw_ber->mant = ucw_rate;
	ucw_ber->exp = mag - SL_CTL_LINK_FEC_UCW_BER_EXP;	/* Divide by UCW multiplier */

	sl_ctl_log_dbg(NULL, LOG_NAME, "fec_ber_calc (CCW = %ue%d, UCW = %ue%d)",
		ccw_ber->mant, ccw_ber->exp, ucw_ber->mant, ucw_ber->exp);

	return 0;
}
