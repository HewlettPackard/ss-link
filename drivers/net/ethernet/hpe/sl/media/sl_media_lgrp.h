/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_MEDIA_LGRP_H_
#define _SL_MEDIA_LGRP_H_

#include <linux/spinlock.h>
#include <linux/kobject.h>

#include <linux/hpe/sl/sl_media.h>

#include "sl_lgrp.h"
#include "sl_media_ldev.h"
#include "base/sl_media_log.h"

#define SL_MEDIA_MAX_SUPPORTED_SPEEDS 8

struct sl_media_serdes_settings;

struct sl_media_lgrp_speed_kobject {
	struct sl_media_lgrp *media_lgrp;
	u32                   speed;
	struct kobject        kobj;
};

#define SL_MEDIA_LGRP_MAGIC 0x736c4D47
struct sl_media_lgrp {
	u32                                 magic;
	u8                                  num;
	struct sl_media_ldev               *media_ldev;

	struct sl_media_jack               *media_jack;
	struct sl_media_lgrp_cable_info    *cable_info;

	struct kobject                      kobj;
	struct kobject                      parent_speed_kobj;
	struct sl_media_lgrp_speed_kobject  speeds_kobj[SL_MEDIA_MAX_SUPPORTED_SPEEDS];
	u8                                  supported_speeds_num;
	bool                                speeds_kobj_init;

	spinlock_t                          log_lock;
	char                                connect_id[SL_LOG_CONNECT_ID_LEN + 1];

	// FIXME: for now only enable at the lgrp level
	bool                                err_trace_enable;
	bool                                warn_trace_enable;
};

int                   sl_media_lgrp_new(u8 ldev_num, u8 lgrp_num);
void                  sl_media_lgrp_del(u8 ldev_num, u8 lgrp_num);
struct sl_media_lgrp *sl_media_lgrp_get(u8 ldev_num, u8 lgrp_num);

void sl_media_lgrp_connect_id_set(u8 ldev_num, u8 lgrp_num, const char *connect_id);

void sl_media_lgrp_real_cable_if_present_send(u8 ldev_num, u8 lgrp_num);
void sl_media_lgrp_real_cable_if_not_present_send(u8 ldev_num, u8 lgrp_num);

void sl_media_lgrp_media_attr_get(u8 ldev_num, u8 lgrp_num, struct sl_media_attr *media_attr);
bool sl_media_lgrp_media_has_error(struct sl_media_lgrp *media_lgrp);

static inline bool SL_MEDIA_LGRP_MEDIA_TYPE_IS_ACTIVE(u32 type)
{
	return (type == SL_MEDIA_TYPE_AEC ||
		type == SL_MEDIA_TYPE_ACC ||
		type == SL_MEDIA_TYPE_AOC ||
		type == SL_MEDIA_TYPE_POC);
}

bool sl_media_lgrp_media_type_is_active(u8 ldev_num, u8 lgrp_num);

void sl_media_lgrp_media_serdes_settings_get(u8 ldev_num, u8 lgrp_num,
					     struct sl_media_serdes_settings *media_serdes_settings);

u32  sl_media_lgrp_vendor_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_type_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_shape_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_length_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_max_speed_get(struct sl_media_lgrp *media_lgrp);
void sl_media_lgrp_serial_num_get(struct sl_media_lgrp *media_lgrp, char *serial_num_str);
void sl_media_lgrp_hpe_pn_get(struct sl_media_lgrp *media_lgrp, char *hpe_pn_str);
u32  sl_media_lgrp_jack_type_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_jack_type_qsfp_density_get(struct sl_media_lgrp *media_lgrp);
u32  sl_media_lgrp_furcation_get(struct sl_media_lgrp *media_lgrp);
bool sl_media_lgrp_is_cable_not_supported(struct sl_media_lgrp *media_lgrp);
void sl_media_lgrp_date_code_get(struct sl_media_lgrp *media_lgrp, char *date_code_str);
void sl_media_lgrp_fw_ver_get(struct sl_media_lgrp *media_lgrp, u8 *fw_ver);

#endif /* _SL_MEDIA_LGRP_H_ */
