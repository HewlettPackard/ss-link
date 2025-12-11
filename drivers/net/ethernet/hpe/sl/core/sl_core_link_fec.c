// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include "sl_core_link.h"
#include "sl_core_str.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_fec.h"
#include "data/sl_core_data_link.h"

#define LOG_NAME SL_CORE_LINK_FEC_LOG_NAME

static int sl_core_link_fec_cw_get(struct sl_core_link *core_link, struct sl_core_link_fec_cw_cntrs *cw_cntrs)
{
	int rtn;
	u32 link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_cw_get");

	spin_lock(&core_link->data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
		rtn = sl_core_hw_fec_cw_cntrs_get(core_link, cw_cntrs);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "fec_ccw_get failed (link_state = %u %s) [%d]",
					      link_state, sl_core_link_state_str(link_state), rtn);
			spin_unlock(&core_link->data_lock);
			return rtn;
		}

		sl_core_log_dbg(core_link, LOG_NAME,
				"fec_cw_get (ccw = %llu, ucw = %llu gcw = %llu)",
				cw_cntrs->ccw, cw_cntrs->ucw, cw_cntrs->gcw);

		spin_unlock(&core_link->data_lock);

		return 0;
	default:
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_cw_get incorrect state (link_state = %u %s)",
				      link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->data_lock);

		return -ENOLINK;
	}
}

static int sl_core_link_fec_lane_cntrs_get(struct sl_core_link *core_link,
					   struct sl_core_link_fec_lane_cntrs *lane_cntrs)
{
	int rtn;
	u32 link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_lane_cntrs_get");

	spin_lock(&core_link->data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
		rtn = sl_core_hw_fec_lane_cntrs_get(core_link, lane_cntrs);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME,
					      "fec_lane_cntrs_get failed (link_state = %u %s) [%d]",
					      link_state, sl_core_link_state_str(link_state), rtn);
			spin_unlock(&core_link->data_lock);
			return rtn;
		}

		sl_core_log_dbg(core_link, LOG_NAME,
				"fec_lane_cntrs_get (lane0 = %llu, lane1 = %llu, lane2 = %llu, lane3 = %llu)",
				lane_cntrs->lanes[0], lane_cntrs->lanes[1],
				lane_cntrs->lanes[2], lane_cntrs->lanes[3]);

		spin_unlock(&core_link->data_lock);

		return 0;
	default:
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_lane_cntrs_get incorrect state (link_state = %u %s)",
				      link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->data_lock);

		return -ENOLINK;
	}
}

static int sl_core_link_fec_tail_cntrs_get(struct sl_core_link *core_link,
					   struct sl_core_link_fec_tail_cntrs *tail_cntrs)
{
	int rtn;
	u32 link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_tail_cntrs_get");

	spin_lock(&core_link->data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
		rtn = sl_core_hw_fec_tail_cntrs_get(core_link, tail_cntrs);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME,
					      "fec_tail_cntrs_get failed (link_state = %u %s) [%d]",
					      link_state, sl_core_link_state_str(link_state), rtn);
			spin_unlock(&core_link->data_lock);
			return rtn;
		}
		sl_core_log_dbg(core_link, LOG_NAME,
				"fec_tail_cntrs_get (ccw_bin0 = %llu, ccw_bin1 = %llu, ccw_bin2 = %llu, ccw_bin3 = %llu, "
				"ccw_bin4 = %llu, ccw_bin5 = %llu, ccw_bin6 = %llu, ccw_bin7 = %llu)",
				tail_cntrs->ccw_bins[0], tail_cntrs->ccw_bins[1],
				tail_cntrs->ccw_bins[2], tail_cntrs->ccw_bins[3],
				tail_cntrs->ccw_bins[4], tail_cntrs->ccw_bins[5],
				tail_cntrs->ccw_bins[6], tail_cntrs->ccw_bins[7]);
		spin_unlock(&core_link->data_lock);

		return 0;
	default:
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_tail_cntrs_get incorrect state (link_state = %u %s)",
				      link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->data_lock);
		return -ENOLINK;
	}
}

int sl_core_link_fec_ccw_get(struct sl_core_link *core_link, u64 *ccw)
{
	int                              rtn;
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	rtn = sl_core_link_fec_cw_get(core_link, &cw_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_ccw_get failed [%d]", rtn);
		return rtn;
	}

	*ccw = cw_cntrs.ccw;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_ccw_get (ccw = %llu)", *ccw);

	return 0;
}

int sl_core_link_fec_ucw_get(struct sl_core_link *core_link, u64 *ucw)
{
	int                              rtn;
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	rtn = sl_core_link_fec_cw_get(core_link, &cw_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_ucw_get failed [%d]", rtn);
		return rtn;
	}

	*ucw = cw_cntrs.ucw;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_ucw_get (ucw = %llu)", *ucw);

	return 0;
}

int sl_core_link_fec_gcw_get(struct sl_core_link *core_link, u64 *gcw)
{
	int                              rtn;
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	rtn = sl_core_link_fec_cw_get(core_link, &cw_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_gcw_get failed [%d]", rtn);
		return rtn;
	}

	*gcw = cw_cntrs.gcw;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_gcw_get (gcw = %llu)", *gcw);

	return 0;
}

int sl_core_link_fec_lane_cntr_get(struct sl_core_link *core_link, u8 lane_num, u64 *lane_cntr)
{
	int                                rtn;
	struct sl_core_link_fec_lane_cntrs lane_cntrs;

	rtn = sl_core_link_fec_lane_cntrs_get(core_link, &lane_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_lane_cntr_get failed [%d]", rtn);
		return rtn;
	}

	*lane_cntr = lane_cntrs.lanes[lane_num];

	sl_core_log_dbg(core_link, LOG_NAME, "fec_lane_cntr_get (lane %u = %llu)", lane_num, *lane_cntr);

	return 0;
}

int sl_core_link_fec_tail_cntr_get(struct sl_core_link *core_link, u8 tail_cntr_num, u64 *tail_cntr)
{
	int                               rtn;
	struct sl_core_link_fec_tail_cntrs tail_cntrs;

	rtn = sl_core_link_fec_tail_cntrs_get(core_link, &tail_cntrs);
	if (rtn) {
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_tail_cntr_get failed [%d]", rtn);
		return rtn;
	}

	*tail_cntr = tail_cntrs.ccw_bins[tail_cntr_num];

	sl_core_log_dbg(core_link, LOG_NAME, "fec_tail_cntr_get (tail_cntr %u = %llu)", tail_cntr_num, *tail_cntr);

	return 0;
}

int sl_core_link_fec_data_get(struct sl_core_link *core_link,
			      struct sl_core_link_fec_cw_cntrs *cw_cntrs,
			      struct sl_core_link_fec_lane_cntrs *lane_cntrs,
			      struct sl_core_link_fec_tail_cntrs *tail_cntr)
{
	int rtn;
	u32 link_state;

	sl_core_log_dbg(core_link, LOG_NAME, "fec_data_get");

	spin_lock(&core_link->data_lock);
	link_state = core_link->link.state;
	switch (link_state) {
	case SL_CORE_LINK_STATE_UP:
		rtn = sl_core_hw_fec_data_get(core_link, cw_cntrs, lane_cntrs, tail_cntr);
		if (rtn) {
			sl_core_log_err_trace(core_link, LOG_NAME, "fec_data_get failed (link_state = %u %s) [%d]",
					      link_state, sl_core_link_state_str(link_state), rtn);
			spin_unlock(&core_link->data_lock);
			return rtn;
		}
		spin_unlock(&core_link->data_lock);

		return 0;
	default:
		sl_core_log_err_trace(core_link, LOG_NAME, "fec_data_get incorrect state (link_state = %u %s)",
				      link_state, sl_core_link_state_str(link_state));
		spin_unlock(&core_link->data_lock);

		return -ENOLINK;
	}
}

