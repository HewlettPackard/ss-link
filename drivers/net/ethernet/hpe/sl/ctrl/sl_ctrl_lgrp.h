/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_CTRL_LGRP_H_
#define _SL_CTRL_LGRP_H_

#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>
#include <linux/wait.h>
#include <linux/completion.h>

#include "sl_asic.h"
#include "base/sl_ctrl_log.h"
#include "sl_ctrl_lgrp_notif.h"

struct sl_ctrl_ldev;
struct sl_lgrp_config;
struct sl_lgrp_policy;

struct sl_lgrp_serdes_lane_kobj {
	struct sl_ctrl_lgrp *ctrl_lgrp;
	u8                   asic_lane_num;
	struct kobject       kobj;
};

#define SL_CTRL_LGRP_MAGIC 0x5c677270
#define SL_CTRL_LGRP_VER   1
struct sl_ctrl_lgrp {
	u32                              magic;
	u32                              ver;

	u8                               num;

	struct sl_lgrp_config            config;
	struct sl_lgrp_policy            policy;
	spinlock_t                       config_lock;

	char                             log_connect_id[SL_LOG_CONNECT_ID_LEN + 1];
	spinlock_t                       log_lock;

	struct sl_ctrl_ldev             *ctrl_ldev;

	struct sl_ctrl_lgrp_notif        ctrl_notif;
	struct work_struct               notif_work;

	struct kobject                  *parent_kobj;
	struct kobject                   policy_kobj;
	struct kobject                   config_kobj;
	struct kobject                   serdes_kobj;
	struct kobject                   serdes_lane_kobj;
	struct kobject                   serdes_lane_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_state_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_swizzle_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_settings_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_eye_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_los_kobjs[SL_ASIC_MAX_LANES];
	struct sl_lgrp_serdes_lane_kobj  serdes_lane_lol_kobjs[SL_ASIC_MAX_LANES];

	// FIXME: for now only enable at the lgrp level
	bool                             err_trace_enable;
	bool                             warn_trace_enable;

	spinlock_t                       data_lock;

	struct kref                      ref_cnt;
	struct completion                del_complete;
};

int		     sl_ctrl_lgrp_new(u8 ldev_num, u8 lgrp_num, struct kobject *sysfs_parent);
int		     sl_ctrl_lgrp_del(u8 ldev_num, u8 lgrp_num);
bool		     sl_ctrl_lgrp_kref_get_unless_zero(struct sl_ctrl_lgrp *ctrl_lgrp);
int		     sl_ctrl_lgrp_put(struct sl_ctrl_lgrp *ctrl_lgrp);
struct sl_ctrl_lgrp *sl_ctrl_lgrp_get(u8 ldev_num, u8 lgrp_num);

int sl_ctrl_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id);

int sl_ctrl_lgrp_config_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_config *lgrp_config);
int sl_ctrl_lgrp_policy_set(u8 ldev_num, u8 lgrp_num, struct sl_lgrp_policy *lgrp_policy);

#endif /* _SL_CTRL_LGRP_H_ */
