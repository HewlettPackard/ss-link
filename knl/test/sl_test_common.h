/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#ifndef _SL_TEST_COMMON_H_
#define _SL_TEST_COMMON_H_

#ifdef __KERNEL__
#include <linux/bitops.h>
#else
#ifndef BIT
#define BIT(_num)      (1UL << (_num))
#endif
#ifndef BIT_ULL
#define BIT_ULL(_num)  (1ULL << (_num))
#endif
#endif

#define CMD_LEN      32
#define CMD_DESC_LEN 128

struct cmd_entry {
	const char cmd[CMD_LEN];
	const char desc[CMD_DESC_LEN];
};

struct cmd_entries {
	struct cmd_entry *entries;
	size_t            num_entries;
};

struct options_field_entry {
	u32 *options;
	u32  field;
};

struct str_conv_u16 {
	int (*to_u16)(const char *str, u16 *value);
	const char* (*to_str)(u16 value);
	void (*opts)(char *buf, size_t size);
	u16 *value;

};

struct str_conv_u32 {
	int (*to_u32)(const char *str, u32 *value);
	const char* (*to_str)(u32 value);
	void (*opts)(char *buf, size_t size);
	u32 *value;
};

/* Config Admin Options */
#define SL_LINK_CONFIG_OPT_LOCK        BIT(30) /* Lock configuration from modification */
#define SL_LINK_CONFIG_OPT_ADMIN       BIT(31) /* Perform admin level operation        */

#define SL_LGRP_OPT_LOCK               BIT(30) /* Lock configuration from modification */
#define SL_LGRP_OPT_ADMIN              BIT(31) /* Perform admin level operation        */

#define SL_LLR_CONFIG_OPT_LOCK         BIT(30) /* Lock configuration from modification */
#define SL_LLR_CONFIG_OPT_ADMIN        BIT(31) /* Perform admin level operation        */

/* Policy Admin Options */
#define SL_LINK_POLICY_OPT_LOCK        BIT(30) /* Lock configuration from modification */
#define SL_LINK_POLICY_OPT_ADMIN       BIT(31) /* Perform admin level operation        */

#define SL_LLR_POLICY_OPT_LOCK         BIT(30) /* Lock configuration from modification */
#define SL_LLR_POLICY_OPT_ADMIN        BIT(31) /* Perform admin level operation        */

int sl_test_serdes_params_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      s16 pre1, s16 pre2, s16 pre3, s16 cursor,
			      s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
			      u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options);

int sl_test_serdes_params_unset(u8 ldev_num, u8 lgrp_num, u8 link_num);

const char *sl_test_state_str(u16 state);
const char *sl_test_serdes_lane_encoding_str(u16 encoding);
const char *sl_test_serdes_lane_clocking_str(u16 clocking);
const char *sl_test_serdes_lane_osr_str(u16 osr);
const char *sl_test_serdes_lane_width_str(u16 width);
const char *sl_test_media_type_str(u32 type);

int sl_test_state_from_str(const char *str, u16 *state);
int sl_test_serdes_lane_encoding_from_str(const char *str, u16 *encoding);
int sl_test_serdes_lane_clocking_from_str(const char *str, u16 *clocking);
int sl_test_serdes_lane_osr_from_str(const char *str, u16 *osr);
int sl_test_serdes_lane_width_from_str(const char *str, u16 *width);
int sl_test_media_type_from_str(const char *str, u32 *type);

void sl_test_state_opts(char *buf, size_t size);

struct kobject *sl_test_ldev_kobj_get(u8 ldev_num);
struct kobject *sl_test_lgrp_kobj_get(u8 ldev_num, u8 lgrp_num);

struct sl_ctl_ldev *sl_test_ctl_ldev_get(u8 ldev_num);
struct sl_ctl_lgrp *sl_test_ctl_lgrp_get(u8 ldev_num, u8 lgrp_num);

int sl_test_debugfs_create_opt(const char *name, umode_t mode, struct dentry *parent,
	struct options_field_entry *value);

int sl_test_cmds_show(struct seq_file *s, struct cmd_entry *cmd_list, size_t num_cmds);

int sl_test_debugfs_create_s32(const char *name, umode_t mode, struct dentry *parent, s32 *value);
int sl_test_debugfs_create_s16(const char *name, umode_t mode, struct dentry *parent, s16 *value);

int sl_test_debugfs_create_str_conv_u16(const char *name, umode_t mode, struct dentry *parent,
					struct str_conv_u16 *option);
int sl_test_debugfs_create_str_conv_u32(const char *name, umode_t mode, struct dentry *parent,
					struct str_conv_u32 *option);

int sl_test_furcation_from_str(const char *str, u32 *furcation);

void sl_test_pg_cfg_set(u8 ldev_num, u8 lgrp_num, u8 data);

#endif /* _SL_TEST_COMMON_H_ */
