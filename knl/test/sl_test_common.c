// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_common.h"
#include "test/sl_media_test.h"
#include "test/sl_core_test_serdes.h"
#include "test/sl_ctl_test_ldev.h"
#include "test/sl_ctl_test_lgrp.h"

#include "sl_core_lgrp.h"
#include "hw/sl_core_hw_io.h"

#define LOG_BLOCK "common"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

#define VAL_LEN        32
#define SYSFS_NAME_LEN 64
#define OPTS_STR_LEN   256

int sl_test_serdes_params_set(u8 ldev_num, u8 lgrp_num, u8 link_num,
			      s16 pre1, s16 pre2, s16 pre3, s16 cursor,
			      s16 post1, s16 post2, u16 media, u16 osr, u16 encoding,
			      u16 clocking, u16 width, u16 dfe, u16 scramble, u32 options)
{
	return sl_core_test_serdes_settings_set(ldev_num, lgrp_num, link_num,
			pre1, pre2, pre3, cursor, post1, post2, media, osr, encoding,
			clocking, width, dfe, scramble, options);
}
EXPORT_SYMBOL(sl_test_serdes_params_set);

const char *sl_test_state_str(u16 state)
{
	return state ? "enabled" : "disabled";
}
EXPORT_SYMBOL(sl_test_state_str);

int sl_test_state_from_str(const char *str, u16 *state)
{
	if (!str || !state)
		return -EINVAL;

	if (!strncmp(str, "enabled", 6)) {
		*state = 1;
		return 0;
	}

	if (!strncmp(str, "disabled", 7)) {
		*state = 0;
		return 0;
	}

	return -ENOENT;
}
EXPORT_SYMBOL(sl_test_state_from_str);

void sl_test_state_opts(char *buf, size_t size)
{
	snprintf(buf, size, "enabled disabled\n");
}
EXPORT_SYMBOL(sl_test_state_opts);

int sl_test_serdes_params_unset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_test_serdes_settings_unset(ldev_num, lgrp_num, link_num);
}
EXPORT_SYMBOL(sl_test_serdes_params_unset);

const char *sl_test_serdes_lane_encoding_str(u16 encoding)
{
	return sl_core_test_serdes_lane_encoding_str(encoding);
}
EXPORT_SYMBOL(sl_test_serdes_lane_encoding_str);

const char *sl_test_serdes_lane_clocking_str(u16 clocking)
{
	return sl_core_test_serdes_lane_clocking_str(clocking);
}
EXPORT_SYMBOL(sl_test_serdes_lane_clocking_str);

const char *sl_test_serdes_lane_osr_str(u16 osr)
{
	return sl_core_test_serdes_lane_osr_str(osr);
}
EXPORT_SYMBOL(sl_test_serdes_lane_osr_str);

const char *sl_test_serdes_lane_width_str(u16 width)
{
	return sl_core_test_serdes_lane_width_str(width);
}
EXPORT_SYMBOL(sl_test_serdes_lane_width_str);

const char *sl_test_media_type_str(u32 type)
{
	return sl_media_test_type_str(type);
}
EXPORT_SYMBOL(sl_test_media_type_str);

int sl_test_serdes_lane_encoding_from_str(const char *str, u16 *encoding)
{
	return sl_core_test_serdes_lane_encoding_from_str(str, encoding);
}
EXPORT_SYMBOL(sl_test_serdes_lane_encoding_from_str);

int sl_test_serdes_lane_clocking_from_str(const char *str, u16 *clocking)
{
	int rtn;

	if (!str || !clocking) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "invalid arg\n");
		return -EINVAL;
	}

	rtn = sl_core_test_serdes_lane_clocking_from_str(str, clocking);

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"clocking_from_str (str = %s, clocking = %u)", str, *clocking);

	return rtn;
}
EXPORT_SYMBOL(sl_test_serdes_lane_clocking_from_str);

int sl_test_serdes_lane_osr_from_str(const char *str, u16 *osr)
{
	return sl_core_test_serdes_lane_osr_from_str(str, osr);
}
EXPORT_SYMBOL(sl_test_serdes_lane_osr_from_str);

int sl_test_serdes_lane_width_from_str(const char *str, u16 *width)
{
	return sl_core_test_serdes_lane_width_from_str(str, width);
}
EXPORT_SYMBOL(sl_test_serdes_lane_width_from_str);

int sl_test_media_type_from_str(const char *str, u32 *type)
{
	return sl_media_test_type_from_str(str, type);
}
EXPORT_SYMBOL(sl_test_media_type_from_str);

struct kobject *sl_test_ldev_kobj_get(u8 ldev_num)
{
	return sl_ctl_test_ldev_kobj_get(ldev_num);
}
EXPORT_SYMBOL(sl_test_ldev_kobj_get);

struct kobject *sl_test_lgrp_kobj_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_ctl_test_lgrp_kobj_get(ldev_num, lgrp_num);
}
EXPORT_SYMBOL(sl_test_lgrp_kobj_get);

struct sl_ctl_ldev *sl_test_ctl_ldev_get(u8 ldev_num)
{
	return sl_ctl_test_ldev_get(ldev_num);
}
EXPORT_SYMBOL(sl_test_ctl_ldev_get);

struct sl_ctl_lgrp *sl_test_ctl_lgrp_get(u8 ldev_num, u8 lgrp_num)
{
	return sl_ctl_test_lgrp_get(ldev_num, lgrp_num);
}
EXPORT_SYMBOL(sl_test_ctl_lgrp_get);

static ssize_t sl_test_option_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	char                        val_buf[VAL_LEN];
	int                         len;
	ssize_t                     bytes_read;
	struct options_field_entry *entry;

	entry = f->private_data;

	len = snprintf(val_buf, sizeof(val_buf), "%s\n",
		(*entry->options & entry->field) ? "enabled" : "disabled");
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "snprintf failed [%d]", len);
		return len;
	}

	bytes_read = simple_read_from_buffer(buf, size, pos, val_buf, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static ssize_t sl_test_option_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	char                        val_buf[VAL_LEN];
	ssize_t                     len;
	int                         rtn;
	struct options_field_entry *entry;
	u8                          enable;

	/* No partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "partial write");
		return 0;
	}

	if (count > sizeof(val_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "write too big (count = %ld)", count);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(val_buf, sizeof(val_buf), pos, buf, count);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_write_to_buffer failed [%ld]", len);
		return len;
	}

	val_buf[len] = '\0';

	rtn = kstrtou8(val_buf, 0, &enable);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "kstrtou8 failed [%d]", rtn);
		return rtn;
	}

	entry = f->private_data;

	if (enable)
		*entry->options |= entry->field;
	else
		*entry->options &= ~entry->field;

	return count;
}

static const struct file_operations sl_test_option_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_option_read,
	.write = sl_test_option_write,
};

int sl_test_debugfs_create_opt(const char *name, umode_t mode, struct dentry *parent,
	struct options_field_entry *option)
{
	struct dentry *dentry;

	dentry = debugfs_create_file(name, mode, parent, option, &sl_test_option_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL(sl_test_debugfs_create_opt);

int sl_test_cmds_show(struct seq_file *s, struct cmd_entry *cmd_list, size_t num_cmds)
{
	size_t cmd_num;

	for (cmd_num = 0; cmd_num < num_cmds; ++cmd_num)
		seq_printf(s, "%-30s: %s\n", cmd_list[cmd_num].cmd, cmd_list[cmd_num].desc);

	return 0;
}
EXPORT_SYMBOL(sl_test_cmds_show);

static ssize_t sl_test_debugfs_s32_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	int rtn;

	rtn = kstrtoint_from_user(buf, count, 0, f->private_data);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "kstrtoint_from_user failed [%d]", rtn);
		return rtn;
	}

	return count;
}

static ssize_t sl_test_debugfs_s32_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	char    val_buf[VAL_LEN];
	int     len;
	ssize_t bytes_read;

	len = snprintf(val_buf, sizeof(val_buf), "%d\n", *(s32 *)f->private_data);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "snprintf failed [%d]", len);
		return len;
	}

	bytes_read = simple_read_from_buffer(buf, size, pos, val_buf, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static const struct file_operations sl_test_fops_s32 = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_debugfs_s32_read,
	.write = sl_test_debugfs_s32_write,
};

int sl_test_debugfs_create_s32(const char *name, umode_t mode, struct dentry *parent, s32 *value)
{
	struct dentry *dentry;

	dentry = debugfs_create_file(name, mode, parent, value, &sl_test_fops_s32);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_s32 failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sl_test_debugfs_create_s32);

static ssize_t sl_test_debugfs_s16_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	int rtn;

	rtn = kstrtos16_from_user(buf, count, 0, f->private_data);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "kstrtoint_from_user failed [%d]", rtn);
		return rtn;
	}

	return count;
}

static ssize_t sl_test_debugfs_s16_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	char    val_buf[VAL_LEN];
	int     len;
	ssize_t bytes_read;

	len = snprintf(val_buf, sizeof(val_buf), "%d\n", *(s16 *)f->private_data);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "snprintf failed [%d]", len);
		return len;
	}

	bytes_read = simple_read_from_buffer(buf, size, pos, val_buf, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static const struct file_operations sl_test_fops_s16 = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_debugfs_s16_read,
	.write = sl_test_debugfs_s16_write,
};

int sl_test_debugfs_create_s16(const char *name, umode_t mode, struct dentry *parent, s16 *value)
{
	struct dentry *dentry;

	dentry = debugfs_create_file(name, mode, parent, value, &sl_test_fops_s16);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_s16 failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sl_test_debugfs_create_s16);

static ssize_t sl_test_debugfs_str_to_u16_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	ssize_t              bytes_read;
	ssize_t              len;
	char                 str[VAL_LEN];
	struct str_conv_u16 *option;

	option = f->private_data;

	len = scnprintf(str, VAL_LEN, "%s\n", option->to_str(*option->value));

	bytes_read = simple_read_from_buffer(buf, size, pos, str, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"str_to_u16_read (len = %ld, str = %s, bytes = %ld, buf = %s)",
		len, str, bytes_read, buf);

	return bytes_read;
}

static ssize_t sl_test_debugfs_str_to_u16_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	int                  rtn;
	int                  len;
	struct str_conv_u16 *option;
	char                 kbuf[VAL_LEN];

	option = f->private_data;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"str_to_u16_write (count = %ld, buf = %s)", count, buf);

	len = simple_write_to_buffer(&kbuf, sizeof(kbuf) - 1, pos, buf, count);
	if (len < 0)
		return len;

	kbuf[len] = '\0';

	rtn = option->to_u16(kbuf, option->value);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "conversion failed [%d]", rtn);
		return rtn;
	}

	return count;
}

static ssize_t sl_test_debugfs_conv_opts(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	ssize_t              bytes_read;
	char                 str[OPTS_STR_LEN];
	struct str_conv_u16 *option;

	option = f->private_data;

	option->opts(str, sizeof(str));

	bytes_read = simple_read_from_buffer(buf, size, pos, str, strnlen(str, OPTS_STR_LEN));
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static const struct file_operations sl_test_fops_str_conv_u16 = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_debugfs_str_to_u16_read,
	.write = sl_test_debugfs_str_to_u16_write,
};

static const struct file_operations sl_test_fops_conv_opts = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_debugfs_conv_opts,
};

int sl_test_debugfs_create_str_conv_u16(const char *name, umode_t mode, struct dentry *parent,
					struct str_conv_u16 *option)
{
	struct dentry *dentry;
	char           name_buf[SYSFS_NAME_LEN];

	dentry = debugfs_create_file(name, mode, parent, option, &sl_test_fops_str_conv_u16);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_str_conv_u16 failed");
		return -ENOMEM;
	}

	snprintf(name_buf, SYSFS_NAME_LEN, "%s_options", name);
	dentry = debugfs_create_file(name_buf, mode, parent, option, &sl_test_fops_conv_opts);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_str_conv_u16 opts failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sl_test_debugfs_create_str_conv_u16);

static ssize_t sl_test_debugfs_str_to_u32_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	ssize_t              bytes_read;
	ssize_t              len;
	char                 str[VAL_LEN];
	struct str_conv_u32 *option;

	option = f->private_data;

	len = scnprintf(str, VAL_LEN, "%s\n", option->to_str(*option->value));

	bytes_read = simple_read_from_buffer(buf, size, pos, str, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"simple_read_from_buffer failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static ssize_t sl_test_debugfs_str_to_u32_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	int                  rtn;
	int                  len;
	struct str_conv_u32 *option;
	char                 kbuf[VAL_LEN];

	option = f->private_data;

	len = simple_write_to_buffer(&kbuf, sizeof(kbuf) - 1, pos, buf, count);
	if (len < 0)
		return len;

	kbuf[len] = '\0';

	rtn = option->to_u32(kbuf, option->value);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "conversion failed [%d]", rtn);
		return rtn;
	}

	return count;
}

static const struct file_operations sl_test_fops_str_conv_u32 = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_debugfs_str_to_u32_read,
	.write = sl_test_debugfs_str_to_u32_write,
};

int sl_test_debugfs_create_str_conv_u32(const char *name, umode_t mode, struct dentry *parent,
					struct str_conv_u32 *option)
{
	struct dentry *dentry;
	char           name_buf[SYSFS_NAME_LEN];

	dentry = debugfs_create_file(name, mode, parent, option, &sl_test_fops_str_conv_u32);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_str_conv_u32 failed");
		return -ENOMEM;
	}

	snprintf(name_buf, SYSFS_NAME_LEN, "%s_options", name);
	dentry = debugfs_create_file(name_buf, mode, parent, option, &sl_test_fops_conv_opts);
	if (!dentry) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_str_conv_u32 opts failed");
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sl_test_debugfs_create_str_conv_u32);

int sl_test_furcation_from_str(const char *str, u32 *furcation)
{
	if (!str || !furcation)
		return -EINVAL;

	if (!strncmp(str, "unfurcated", 10)) {
		*furcation = SL_MEDIA_FURCATION_X1;
		return 0;
	}

	if (!strncmp(str, "bifurcated", 10)) {
		*furcation = SL_MEDIA_FURCATION_X2;
		return 0;
	}

	if (!strncmp(str, "quadfurcated", 12)) {
		*furcation = SL_MEDIA_FURCATION_X4;
		return 0;
	}

	return -ENOENT;
}
EXPORT_SYMBOL(sl_test_furcation_from_str);

void sl_test_pg_cfg_set(u8 ldev_num, u8 lgrp_num, u8 data)
{
	struct sl_core_lgrp *core_lgrp;
	u8                   port;
	u64                  data64;

	port      = lgrp_num;
	core_lgrp = sl_core_lgrp_get(ldev_num, lgrp_num);

	sl_log_dbg(core_lgrp, LOG_BLOCK, LOG_NAME,
		"pg_cfg_set (port = %u, data = %u)", port, data);

	sl_core_lgrp_read64(core_lgrp, PORT_PML_CFG_PORT_GROUP, &data64);
	data64 = SS2_PORT_PML_CFG_PORT_GROUP_PG_CFG_UPDATE(data64, data);
	sl_core_lgrp_write64(core_lgrp, PORT_PML_CFG_PORT_GROUP, data64);
	sl_core_lgrp_flush64(core_lgrp, PORT_PML_CFG_PORT_GROUP);
}
EXPORT_SYMBOL(sl_test_pg_cfg_set);
