/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LGRP_H_
#define _SL_CORE_LGRP_H_

#include <linux/spinlock.h>

#include <linux/sl_lgrp.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"

#define SL_CORE_LGRP_MAGIC 0x736c474D
struct sl_core_lgrp {
	u32                             magic;
	u32                             num;

	struct sl_core_ldev            *core_ldev;

	spinlock_t                      data_lock;

	spinlock_t                      log_lock;
	char                            log_connect_id[SL_LOG_CONNECT_ID_LEN + 1];

	// FIXME: maybe make this a state?
	u8                              is_configuring;
	// FIXME: remove this when we no longer need info from CXI
	struct sl_hw_attr               hw_attr;
	struct sl_lgrp_config           config;

	struct sl_link_caps             link_caps[SL_ASIC_MAX_LINKS];

	struct {
		struct sl_dt_lgrp_info  dt;
		u32                     clocking;
		struct {
			u32             tx;
			u32             rx;
		} lane_state[SL_MAX_LANES];
		struct {
			u8              low;
			u8              high;
		} eye_limits[SL_MAX_LANES];
	} serdes;

	// FIXME: for now only enable at the lgrp level
	bool err_trace_enable;
	bool warn_trace_enable;
};

int		     sl_core_lgrp_new(u8 ldev_num, u8 lgrp_num);
void		     sl_core_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id);
void		     sl_core_lgrp_del(u8 ldev_num, u8 lgrp_num);
struct sl_core_lgrp *sl_core_lgrp_get(u8 ldev_num, u8 lgrp_num);

int sl_core_lgrp_hw_attr_set(u8 ldev_num, u8 lgrp_num, struct sl_hw_attr *hw_attr);
int sl_core_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config);

int sl_core_lgrp_pre1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre1);
int sl_core_lgrp_pre2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre2);
int sl_core_lgrp_pre3_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *pre3);
int sl_core_lgrp_cursor_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *cursor);
int sl_core_lgrp_post1_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post1);
int sl_core_lgrp_post2_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, s16 *post2);

int sl_core_lgrp_osr_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *osr);
int sl_core_lgrp_encoding_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *encoding);
int sl_core_lgrp_width_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width);
int sl_core_lgrp_dfe_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width);
int sl_core_lgrp_scramble_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *width);
int sl_core_lgrp_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper);
int sl_core_lgrp_eye_lower_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_lower);

#endif /* _SL_CORE_LGRP_H_ */
