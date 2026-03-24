/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021,2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CORE_LGRP_H_
#define _SL_CORE_LGRP_H_

#include <linux/spinlock.h>

#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_asic.h"
#include "base/sl_core_log.h"
#include "sl_core_ldev.h"

enum sl_dt_jack_type {
	SL_DT_JACK_TYPE_LEGACY,
	SL_DT_JACK_TYPE_DD,
	SL_DT_JACK_TYPE_EXAMAX_LEFT,
	SL_DT_JACK_TYPE_EXAMAX_RIGHT,
	SL_DT_JACK_TYPE_LOW,
	SL_DT_JACK_TYPE_HIGH,
};

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

	struct {
		struct {
			u16             addr;
			u16             data;
			u8              dev_id;
			u8              lane;
			u8              pll;
			int             result;
		} rd;
		struct {
			u16             addr;
			u16             data;
			u16             mask;
			u8              dev_id;
			u8              lane;
			u8              pll;
			int             result;
		} wr;
	} pmi;

	struct {
		struct {
			u8              dev_addr;
			u32             data;
			u32             mask;
			u8              reg;
			u8              lsb;
			int             result;
		} rd;
		struct {
			u8              dev_addr;
			u32             data;
			u32             mask;
			u8              reg;
			u8              lsb;
			int             result;
		} wr;
		struct {
			u8              dev_addr;
			int             result;
		} rst;
	} sbus;

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
int sl_core_lgrp_scramble_dis_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *scramble_dis);
int sl_core_lgrp_eye_upper_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_upper);
int sl_core_lgrp_eye_lower_get(struct sl_core_lgrp *core_lgrp, u8 asic_lane_num, u8 *eye_lower);

bool sl_core_lgrp_media_lane_data_swap(u8 ldev_num, u8 lgrp_num);
u32  sl_core_lgrp_jack_part_get(u8 ldev_num, u8 lgrp_num);

int sl_core_lgrp_tx_lane_is_lol(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_tx_lol);
int sl_core_lgrp_rx_lane_is_lol(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_rx_lol);
int sl_core_lgrp_tx_lane_is_los(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_tx_los);
int sl_core_lgrp_rx_lane_is_los(u8 ldev_num, u8 lgrp_num, u8 asic_lane_num, bool *is_rx_los);

int sl_core_lgrp_pmi_rd_addr_get(struct sl_core_lgrp *core_lgrp, u16 *addr);
int sl_core_lgrp_pmi_rd_data_get(struct sl_core_lgrp *core_lgrp, u16 *data);
int sl_core_lgrp_pmi_rd_dev_id_get(struct sl_core_lgrp *core_lgrp, u8 *dev_id);
int sl_core_lgrp_pmi_rd_lane_get(struct sl_core_lgrp *core_lgrp, u8 *lane);
int sl_core_lgrp_pmi_rd_pll_get(struct sl_core_lgrp *core_lgrp, u8 *pll);
int sl_core_lgrp_pmi_rd_result_get(struct sl_core_lgrp *core_lgrp, int *result);

int sl_core_lgrp_pmi_wr_addr_get(struct sl_core_lgrp *core_lgrp, u16 *addr);
int sl_core_lgrp_pmi_wr_data_get(struct sl_core_lgrp *core_lgrp, u16 *data);
int sl_core_lgrp_pmi_wr_mask_get(struct sl_core_lgrp *core_lgrp, u16 *mask);
int sl_core_lgrp_pmi_wr_dev_id_get(struct sl_core_lgrp *core_lgrp, u8 *dev_id);
int sl_core_lgrp_pmi_wr_lane_get(struct sl_core_lgrp *core_lgrp, u8 *lane);
int sl_core_lgrp_pmi_wr_pll_get(struct sl_core_lgrp *core_lgrp, u8 *pll);
int sl_core_lgrp_pmi_wr_result_get(struct sl_core_lgrp *core_lgrp, int *result);

int sl_core_lgrp_sbus_rd_dev_addr_get(struct sl_core_lgrp *core_lgrp, u8 *dev_addr);
int sl_core_lgrp_sbus_rd_data_get(struct sl_core_lgrp *core_lgrp, u32 *data);
int sl_core_lgrp_sbus_rd_mask_get(struct sl_core_lgrp *core_lgrp, u32 *mask);
int sl_core_lgrp_sbus_rd_reg_get(struct sl_core_lgrp *core_lgrp, u8 *reg);
int sl_core_lgrp_sbus_rd_lsb_get(struct sl_core_lgrp *core_lgrp, u8 *lsb);
int sl_core_lgrp_sbus_rd_result_get(struct sl_core_lgrp *core_lgrp, int *result);

int sl_core_lgrp_sbus_wr_dev_addr_get(struct sl_core_lgrp *core_lgrp, u8 *dev_addr);
int sl_core_lgrp_sbus_wr_data_get(struct sl_core_lgrp *core_lgrp, u32 *data);
int sl_core_lgrp_sbus_wr_mask_get(struct sl_core_lgrp *core_lgrp, u32 *mask);
int sl_core_lgrp_sbus_wr_reg_get(struct sl_core_lgrp *core_lgrp, u8 *reg);
int sl_core_lgrp_sbus_wr_lsb_get(struct sl_core_lgrp *core_lgrp, u8 *lsb);
int sl_core_lgrp_sbus_wr_result_get(struct sl_core_lgrp *core_lgrp, int *result);

int sl_core_lgrp_sbus_rst_dev_addr_get(struct sl_core_lgrp *core_lgrp, u8 *dev_addr);
int sl_core_lgrp_sbus_rst_result_get(struct sl_core_lgrp *core_lgrp, int *result);

#endif /* _SL_CORE_LGRP_H_ */
