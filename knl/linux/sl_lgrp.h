/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _LINUX_SL_LGRP_H_
#define _LINUX_SL_LGRP_H_

#include <linux/sl_link.h>
#include <linux/sl_llr.h>

#include "uapi/sl_lgrp.h"
#include "uapi/sl_media.h"

struct sl_ldev;
struct sl_lgrp;
struct sl_lgrp_config;
struct kobject;

#define SL_HW_ATTR_MAGIC 0x73736861
#define SL_HW_ATTR_VER   1
struct sl_hw_attr {
        u32 magic;
        u32 ver;

        u8 cxi_num;
        u8 nic_num;
};

int sl_lgrp_hw_attr_set(struct sl_lgrp *lgrp, struct sl_hw_attr *hw_attr);

struct sl_lgrp_notif_info_link_up {
	u32 mode;
	u32 fec_type;
	u32 fec_mode;
};

union sl_lgrp_notif_info {
	struct sl_lgrp_notif_info_link_up         link_up;
	struct sl_link_caps                       lp_link_caps;
	struct sl_llr_data                        llr_data;
	struct sl_media_attr                      media_attr;
	int                                       error;
	u64                                       cause_map;
};

struct sl_lgrp_notif_msg {
	u8                       ldev_num;
	u8                       lgrp_num;
	u8                       link_num;
	u32                      type;
	union sl_lgrp_notif_info info;
	u64                      info_map;
};

#define SL_MAX_LANES 4
struct sl_dt_lgrp_info {
	u8 sbus_ring;
	u8 dev_id;
	u8 dev_addr;
	struct {
		u8 tx_source;
		u8 tx_invert;
		u8 rx_source;
		u8 rx_invert;
	} lane_info[SL_MAX_LANES];
};

struct sl_lgrp *sl_lgrp_new(struct sl_ldev *ldev, u8 lgrp_num, struct kobject *sysfs_parent);
int             sl_lgrp_del(struct sl_lgrp *lgrp);

int sl_lgrp_config_set(struct sl_lgrp *lgrp, struct sl_lgrp_config *lgrp_config);
int sl_lgrp_policy_set(struct sl_lgrp *lgrp, struct sl_lgrp_policy *lgrp_policy);

typedef void (*sl_lgrp_notif_t)(void *tag, struct sl_lgrp_notif_msg *msg);

int sl_lgrp_notif_callback_reg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
			       u32 types, void *tag);
int sl_lgrp_notif_callback_unreg(struct sl_lgrp *lgrp, sl_lgrp_notif_t callback,
				 u32 types);

int sl_lgrp_connect_id_set(struct sl_lgrp *lgrp, const char *connect_id);

const char *sl_lgrp_config_tech_str(u32 config);
const char *sl_lgrp_config_fec_str(u32 config);
const char *sl_lgrp_furcation_str(u32 furcation);
const char *sl_lgrp_notif_str(u32 notif);
const char *sl_lgrp_config_opt_str(u32 option);
const char *sl_lgrp_fec_mode_str(u32 mode);
const char *sl_lgrp_llr_mode_str(u32 mode);

#endif /* _LINUX_SL_LGRP_H_ */
