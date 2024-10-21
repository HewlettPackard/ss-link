// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_link.h>

#include "sl_link.h"
#include "log/sl_log.h"
#include "sl_test_debugfs.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_common.h"

static struct dentry         *link_dir;
static struct sl_link         link;
static struct sl_link_config  link_config;
static struct sl_link_policy  link_policy;

struct options_field_entry {
	u32 *options;
	u32  field;
};

#define STATIC_CONFIG_OPT_ENTRY(_name, _bit_field) static struct options_field_entry config_option_##_name = { \
	.options = &link_config.options,                                                                       \
	.field   = SL_LINK_CONFIG_OPT_##_bit_field,                                                            \
}

#define STATIC_POLICY_OPT_ENTRY(_name, _bit_field) static struct options_field_entry policy_option_##_name = { \
	.options = &link_policy.options,                                                                       \
	.field   = SL_LINK_POLICY_OPT_##_bit_field,                                                            \
}

STATIC_CONFIG_OPT_ENTRY(lock,               LOCK);
STATIC_CONFIG_OPT_ENTRY(autoneg,            AUTONEG_ENABLE);
STATIC_CONFIG_OPT_ENTRY(serdes_loopback,    SERDES_LOOPBACK_ENABLE);
STATIC_CONFIG_OPT_ENTRY(headshell_loopback, HEADSHELL_LOOPBACK_ENABLE);
STATIC_CONFIG_OPT_ENTRY(remote_loopback,    REMOTE_LOOPBACK_ENABLE);

STATIC_POLICY_OPT_ENTRY(lock,               LOCK);
STATIC_POLICY_OPT_ENTRY(keep_serdes_up,     KEEP_SERDES_UP);

#define VAL_LEN  32

static ssize_t sl_test_link_read(struct file *f, char __user *buf, size_t size, loff_t *pos)
{
	char                        val_buf[VAL_LEN];
	int                         len;
	ssize_t                     bytes_read;
	struct options_field_entry *entry;

	entry = f->private_data;

	len = snprintf(val_buf, sizeof(val_buf), "%s",
		(*entry->options & entry->field) ? "enabled" : "disabled");
	if (len < 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME, "read_bit snprintf failed [%d]", len);
		return len;
	}

	bytes_read = simple_read_from_buffer(buf, size, pos, val_buf, len);
	if (bytes_read < 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"read_bit simple_read failed [%ld]", bytes_read);
		return bytes_read;
	}

	return bytes_read;
}

static ssize_t sl_test_link_write(struct file *f, const char __user *buf, size_t count, loff_t *pos)
{
	char                        val_buf[VAL_LEN];
	ssize_t                     len;
	int                         rtn;
	struct options_field_entry *entry;
	u8                          enable;

	/* No partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME, "partial write_bit");
		return 0;
	}

	if (count > sizeof(val_buf)) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME, "write_bit too big (count = %ld)", count);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(val_buf, sizeof(val_buf), pos, buf, count);
	if (len < 0) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"write_bit simple_write_to_buffer failed [%ld]", len);
		return len;
	}

	val_buf[len] = '\0';

	rtn = kstrtou8(val_buf, 0, &enable);
	if (rtn) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME, "write_bit kstrtou8 failed [%d]", rtn);
		return rtn;
	}

	entry = f->private_data;

	if (enable)
		*entry->options |= entry->field;
	else
		*entry->options &= ~entry->field;

	return count;
}

// FIXME: possibly commonize these and put in separate file
static const struct file_operations sl_test_link_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.read  = sl_test_link_read,
	.write = sl_test_link_write,
};

static void sl_test_link_init(void)
{
	link.magic    = SL_LINK_MAGIC;
	link.ver      = SL_LINK_VER;
	link.size     = sizeof(link);

	link.ldev_num = 0;
	link.lgrp_num = 0;
	link.num      = 0;
}

static void sl_test_link_config_init(void)
{
	link_config.magic = SL_LINK_CONFIG_MAGIC;
	link_config.ver   = SL_LINK_CONFIG_VER;
	link_config.size  = sizeof(link_config);

	link_config.options |= SL_LINK_CONFIG_OPT_ADMIN;

	link_config.link_up_tries_max     = 0;
	link_config.fec_up_check_wait_ms  = 0;
	link_config.fec_up_settle_wait_ms = 0;
	link_config.fec_up_ccw_limit      = 0;
	link_config.fec_up_ucw_limit      = 0;
}

static void sl_test_link_policy_init(void)
{
	link_policy.magic = SL_LINK_POLICY_MAGIC;
	link_policy.ver   = SL_LINK_POLICY_VER;
	link_policy.size  = sizeof(link_policy);

	link_policy.options |= SL_LINK_CONFIG_OPT_ADMIN;

	link_policy.fec_mon_period_ms      = 0;
	link_policy.fec_mon_ucw_down_limit = 0;
	link_policy.fec_mon_ccw_down_limit = 0;
	link_policy.fec_mon_ucw_warn_limit = 0;
	link_policy.fec_mon_ccw_warn_limit = 0;
}

int sl_test_link_create(struct dentry *top_dir)
{
	struct dentry *dentry;
	struct dentry *config_dir;
	struct dentry *policy_dir;

	link_dir = debugfs_create_dir("link", top_dir);
	if (!link_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_init();

	debugfs_create_u8("num", 0644, link_dir, &link.num);

	config_dir = debugfs_create_dir("config", link_dir);
	if (!config_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_config_init();

	debugfs_create_u32("link_up_timeout_ms", 0644, config_dir, &link_config.link_up_timeout_ms);
	debugfs_create_u32("link_up_tries_max", 0644,  config_dir, &link_config.link_up_tries_max);
	debugfs_create_u32("fec_up_settle_wait_ms", 0644,  config_dir, &link_config.fec_up_settle_wait_ms);
	debugfs_create_u32("fec_up_check_wait_ms", 0644,   config_dir, &link_config.fec_up_check_wait_ms);
	debugfs_create_u32("fec_up_ccw_limit", 0644,  config_dir, &link_config.fec_up_ccw_limit);
	debugfs_create_u32("fec_up_ucw_limit", 0644,  config_dir, &link_config.fec_up_ucw_limit);

	dentry = debugfs_create_file("lock", 0644, config_dir,
					&config_option_lock, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config lock debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("autoneg", 0644, config_dir,
					&config_option_autoneg, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config an debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("serdes_loopback", 0644, config_dir,
					&config_option_serdes_loopback, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config serdes_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("headshell_loopback", 0644, config_dir,
					&config_option_headshell_loopback, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config headshell_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("remote_loopback", 0644, config_dir,
					&config_option_remote_loopback, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link config remote_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	policy_dir = debugfs_create_dir("policy", link_dir);
	if (!policy_dir) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link policy debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_policy_init();

	debugfs_create_u32("fec_mon_period", 0644,          policy_dir,
		    &link_policy.fec_mon_period_ms);
	debugfs_create_u32("fec_mon_ucw_down_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ucw_down_limit);
	debugfs_create_u32("fec_mon_ucw_warn_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ucw_warn_limit);
	debugfs_create_u32("fec_mon_ccw_down_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ccw_down_limit);
	debugfs_create_u32("fec_mon_ccw_warn_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ccw_warn_limit);

	dentry = debugfs_create_file("lock", 0644, policy_dir,
		&policy_option_lock, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link policy options debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("keep_serdes_up", 0644, policy_dir,
		&policy_option_keep_serdes_up, &sl_test_link_fops);
	if (!dentry) {
		sl_log_err(NULL, SL_LOG_BLOCK, SL_LOG_DEBUGFS_LOG_NAME,
			"link policy options debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

int sl_test_link_up(void)
{
	link.ldev_num = sl_test_ldev_num_get();
	link.lgrp_num = sl_test_lgrp_num_get();

	return sl_link_up(&link);
}

int sl_test_link_down(void)
{
	link.ldev_num = sl_test_ldev_num_get();
	link.lgrp_num = sl_test_lgrp_num_get();

	return sl_link_down(&link);
}

int sl_test_link_config_set(void)
{
	link.ldev_num = sl_test_ldev_num_get();
	link.lgrp_num = sl_test_lgrp_num_get();

	return sl_link_config_set(&link, &link_config);
}

int sl_test_link_policy_set(void)
{
	link.ldev_num = sl_test_ldev_num_get();
	link.lgrp_num = sl_test_lgrp_num_get();

	return sl_link_policy_set(&link, &link_policy);
}

u8 sl_test_link_num_get(void)
{
	return link.num;
}
