// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/overflow.h>

#include "base/sl_ctrl_log.h"
#include "sl_ctrl_lgrp.h"
#include "sl_ctrl_link.h"
#include "sl_ctrl_link_fec.h"
#include "sl_core_link.h"

#define LOG_NAME SL_CTRL_LINK_FEC_LOG_NAME

int sl_ctrl_link_fec_info_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_fec_info *fec_info)
{
	struct sl_ctrl_link *ctrl_link;
	int                 x;

	ctrl_link = sl_ctrl_link_get_ref(ldev_num, lgrp_num, link_num);
	if (!ctrl_link) {
		sl_ctrl_log_err(NULL, LOG_NAME,
			"info get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	*fec_info = sl_ctrl_link_fec_data_info_get(ctrl_link);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME,
		"info get (ucw = %llu, ccw = %llu, gcw = %llu, period = %ums)",
		fec_info->ucw, fec_info->ccw, fec_info->gcw, fec_info->period_ms);
	for (x = 0; x < SL_CTRL_NUM_FEC_LANES; ++x)
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "info get (lane %d = 0x%llX)", x, fec_info->lanes[x]);

	sl_ctrl_link_put_ref(ctrl_link);

	return 0;
}

int sl_ctrl_link_fec_tail_get(u8 ldev_num, u8 lgrp_num, u8 link_num, struct sl_fec_tail *fec_tail)
{
	struct sl_ctrl_link *ctrl_link;
	int                 x;

	ctrl_link = sl_ctrl_link_get_ref(ldev_num, lgrp_num, link_num);
	if (!ctrl_link) {
		sl_ctrl_log_err(NULL, LOG_NAME,
			"tail get NULL link (ldev_num = %u, lgrp_num = %u, link_num = %u)",
			ldev_num, lgrp_num, link_num);
		return -EBADRQC;
	}

	*fec_tail = sl_ctrl_link_fec_data_tail_get(ctrl_link);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "tail get (period = %ums))", fec_tail->period_ms);
	for (x = 0; x < SL_CTRL_NUM_CCW_BINS; ++x)
		sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "tail get (bin %d = %llu)", x, fec_tail->ccw_bins[x]);

	sl_ctrl_link_put_ref(ctrl_link);

	return 0;
}

int sl_ctrl_link_fec_data_get(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			      struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			      struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	return sl_core_link_fec_data_get(sl_core_link_get(ldev_num, lgrp_num, link_num), cw_cntrs, lane_cntrs,
					 tail_cntrs);
}

int sl_ctrl_link_fec_mon_state_get(struct sl_ctrl_link *ctrl_link, u32 *state)
{
	spin_lock(&ctrl_link->fec_mon_timer_lock);
	*state = ctrl_link->fec_mon_state;
	spin_unlock(&ctrl_link->fec_mon_timer_lock);

	sl_ctrl_log_dbg(ctrl_link, LOG_NAME, "fec monitor (state = %u, %s)", *state,
			sl_ctrl_link_fec_mon_state_str(*state));

	return 0;
}

const char *sl_ctrl_link_fec_mon_state_str(u32 state)
{
	switch (state) {
	case SL_CTRL_LINK_FEC_MON_OFF:
		return "off";
	case SL_CTRL_LINK_FEC_MON_ON:
		return "on";
	default:
		return "unknown";
	}
}

/* CCW BER Calculation Multiplier (10^6). */
#define SL_CTRL_LINK_FEC_CCW_BER_MULT 1000000ULL
#define SL_CTRL_LINK_FEC_CCW_BER_EXP 6

/* UCW BER Calculation Multiplier (10^12). */
#define SL_CTRL_LINK_FEC_UCW_BER_MULT 1000000000000ULL
#define SL_CTRL_LINK_FEC_UCW_BER_EXP 12

int sl_ctrl_link_fec_ber_calc(struct sl_fec_info *fec_info, struct sl_ber *ucw_ber, struct sl_ber *ccw_ber)
{
	u64 total_cw;
	u64 total_bits;
	u64 denominator;
	u64 numerator;
	u64 ccw_rate;	/* Rate * 10^10 */
	u64 ucw_rate;	/* Rate * 10^10 */
	int mag;

	sl_ctrl_log_dbg(NULL, LOG_NAME, "fec_ber_calc");

	if (!fec_info->period_ms) {
		sl_ctrl_log_err(NULL, LOG_NAME, "fec_ber_calc invalid period (period_ms = %d)", fec_info->period_ms);
		return -EINVAL;
	}

	if (check_add_overflow(fec_info->ccw, fec_info->gcw, &total_cw)) {
		sl_ctrl_log_err(NULL, LOG_NAME, "fec_ber_calc total_cw overflow");
		return -ERANGE;
	}

	if (!total_cw) {
		sl_ctrl_log_err(NULL, LOG_NAME, "fec_ber_calc invalid total (total_cw = %llu)", total_cw);
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

	/* RS(544/514) uses 5440 bits per codeword. */
	if (check_mul_overflow(total_cw, 5440ULL, &total_bits) ||
	    check_mul_overflow(total_bits, (u64)fec_info->period_ms, &denominator) ||
	    check_mul_overflow(fec_info->ccw, (u64)HZ, &numerator) ||
	    check_mul_overflow(numerator, SL_CTRL_LINK_FEC_CCW_BER_MULT, &numerator))
		return -ERANGE;

	ccw_rate = numerator / denominator;

	/* Keep at least 2 significant digits */
	mag = 0;
	while ((ccw_rate / 100) > 9) {
		ccw_rate /= 100;
		mag += 2;
	}

	ccw_ber->mant = ccw_rate;
	ccw_ber->exp = mag - SL_CTRL_LINK_FEC_CCW_BER_EXP;	/* Divide by CCW multiplier */

	if (check_mul_overflow(fec_info->ucw, (u64)HZ, &numerator) ||
	    check_mul_overflow(numerator, SL_CTRL_LINK_FEC_UCW_BER_MULT, &numerator))
		return -ERANGE;

	ucw_rate = numerator / denominator;

	/* Keep at least 2 significant digits */
	mag = 0;
	while ((ucw_rate / 100) > 9) {
		ucw_rate /= 100;
		mag += 2;
	}

	ucw_ber->mant = ucw_rate;
	ucw_ber->exp = mag - SL_CTRL_LINK_FEC_UCW_BER_EXP;	/* Divide by UCW multiplier */

	sl_ctrl_log_dbg(NULL, LOG_NAME, "fec_ber_calc (CCW = %ue%d, UCW = %ue%d)",
		ccw_ber->mant, ccw_ber->exp, ucw_ber->mant, ucw_ber->exp);

	return 0;
}
