// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>
#include <linux/debugfs.h>

#include "log/sl_log.h"
#include "sl_test_common.h"
#include "test/sl_core_test_serdes.h"
#include "test/sl_ctl_test_ldev.h"
#include "test/sl_ctl_test_lgrp.h"

#define LOG_BLOCK "common"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

#define VAL_LEN 32

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

int sl_test_serdes_params_unset(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	return sl_core_test_serdes_settings_unset(ldev_num, lgrp_num, link_num);
}
EXPORT_SYMBOL(sl_test_serdes_params_unset);

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
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "debugfs_create_opt failed");
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

	rtn = kstrtoint_from_user(buf, count, 0, f->private_data);
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
