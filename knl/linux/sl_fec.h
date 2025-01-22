/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_FEC_H_
#define _LINUX_SL_FEC_H_

struct sl_link;

#define SL_CTL_NUM_FEC_LANES 16
struct sl_fec_info {
	u64 ucw;                          /* uncorrected code word count */
	u64 ccw;                          /* corrected code word count   */
	u64 gcw;                          /* good code word count        */
	u64 lanes[SL_CTL_NUM_FEC_LANES];  /* error count per lane        */
	u32 period_ms;                    /* collection period           */
	struct {
		s32 ccw_crit_limit;       /* ccw crit limit used in the monitor */
		s32 ccw_warn_limit;       /* ccw warn limit used in the monitor */
		s32 ucw_down_limit;       /* ucw down limit used in the monitor */
		s32 ucw_warn_limit;       /* ucw warn limit used in the monitor */
	} monitor;
};
int sl_fec_info_get(struct sl_link *link, struct sl_fec_info *fec_info);

#define SL_CTL_NUM_CCW_BINS 15
struct sl_fec_tail {
	u64 ccw_bins[SL_CTL_NUM_CCW_BINS];
	u32 period_ms;                    /* collection period */ 
};
int sl_fec_tail_get(struct sl_link *link, struct sl_fec_tail *fec_tail);

struct sl_ber {
	u32 mant;
	s32 exp;
};
int sl_fec_ber_calc(struct sl_fec_info *fec_info, struct sl_ber *ucw_ber, struct sl_ber *ccw_ber);

#endif /* _LINUX_SL_FEC_H_ */
