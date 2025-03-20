// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_link.h>
#include <linux/sl_test.h>
#include <linux/kobject.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_common.h"

#define LOG_BLOCK "link"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry         *link_dir;
static struct sl_link         link;
static struct sl_link_config  link_config;
static struct sl_link_policy  link_policy;
static struct kobject        *sl_link_num_dir[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static struct kobject         sl_link_num_dir_kobj[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];

enum link_cmd_index {
	LINK_NEW_CMD,
	LINK_DEL_CMD,
	LINK_CONFIG_WRITE_CMD,
	LINK_POLICY_WRITE_CMD,
	UP_CMD,
	DOWN_CMD,
	CANCEL_CMD,
	LINK_OPTION_SET_CMD,
	LINK_FEC_CNT_SET_CMD,
	NUM_CMDS,
};

static struct cmd_entry link_cmd_list[]  = {
	[LINK_NEW_CMD]          = { .cmd = "link_new",          .desc = "create link object"                 },
	[LINK_DEL_CMD]          = { .cmd = "link_del",          .desc = "delete link object"                 },
	[LINK_CONFIG_WRITE_CMD] = { .cmd = "link_config_write", .desc = "write out the link config from dir" },
	[LINK_POLICY_WRITE_CMD] = { .cmd = "link_policy_write", .desc = "write out the link policy from dir" },
	[UP_CMD]                = { .cmd = "up",                .desc = "task link to go up"                 },
	[DOWN_CMD]              = { .cmd = "down",              .desc = "task link to go down"               },
	[CANCEL_CMD]            = { .cmd = "cancel",            .desc = "cancel command"                     },
	[LINK_OPTION_SET_CMD]   = { .cmd = "link_opt_set",      .desc = "set link test options"              },
	[LINK_FEC_CNT_SET_CMD]  = { .cmd = "link_fec_cnt_set",  .desc = "set link test fec cntrs"            },
};

#define LINK_CMD_MATCH(_index, _str) \
	(strncmp((_str), link_cmd_list[_index].cmd, strlen(link_cmd_list[_index].cmd)) == 0)

static ssize_t sl_test_link_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];
	bool    match_found;

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (size > sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf), pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	match_found = LINK_CMD_MATCH(LINK_NEW_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_new();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_new failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(LINK_DEL_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_del();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_del failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(UP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_up();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_up failed [%d]", rtn);
			return rtn;
		}
		return size;
	};

	match_found = LINK_CMD_MATCH(DOWN_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_down();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_down failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(LINK_CONFIG_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_config_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_config_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(LINK_POLICY_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_policy_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_policy_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(LINK_OPTION_SET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_options_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_options_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LINK_CMD_MATCH(LINK_FEC_CNT_SET_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_link_fec_cntr_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_link_options_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
}

static const struct file_operations sl_test_link_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_link_cmd_write,
};

static int sl_test_link_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, link_cmd_list, ARRAY_SIZE(link_cmd_list));
}

static int sl_test_link_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_link_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_link_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_link_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static struct kobj_type sl_test_link_kobj_type = {
	.sysfs_ops = &kobj_sysfs_ops,
};

#define SL_TEST_OPT_USE_FEC_CNTR BIT(0)

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
STATIC_CONFIG_OPT_ENTRY(headshell_loopback, HEADSHELL_LOOPBACK_ENABLE);
STATIC_CONFIG_OPT_ENTRY(remote_loopback,    REMOTE_LOOPBACK_ENABLE);

STATIC_POLICY_OPT_ENTRY(lock,               LOCK);
STATIC_POLICY_OPT_ENTRY(keep_serdes_up,     KEEP_SERDES_UP);

static u64 fec_ucw;
static u64 fec_ccw;
static u64 fec_gcw;
static u32 test_link_options;
static struct options_field_entry test_option_use_fec_cntr;

static void sl_test_options_init(void)
{
	test_option_use_fec_cntr.options = &test_link_options;
	test_option_use_fec_cntr.field = SL_TEST_OPT_USE_FEC_CNTR;
}

static void sl_test_link_init(void)
{
	link.magic    = SL_LINK_MAGIC;
	link.ver      = SL_LINK_VER;
	link.size     = sizeof(link);
	link.ldev_num = sl_test_debugfs_ldev_num_get();
	link.lgrp_num = sl_test_debugfs_lgrp_num_get();
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
	link_config.pause_map             = 0;
	link_config.hpe_map               = 0;
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

static struct sl_link *sl_test_link_get(void)
{
	link.ldev_num = sl_test_debugfs_ldev_num_get();
	link.lgrp_num = sl_test_debugfs_lgrp_num_get();

	return &link;
}

int sl_test_debugfs_link_create(struct dentry *top_dir)
{
	int            rtn;
	struct dentry *config_dir;
	struct dentry *policy_dir;
	struct dentry *test_dir;
	struct dentry *dentry;

	link_dir = debugfs_create_dir("link", top_dir);
	if (!link_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"link debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_init();

	debugfs_create_u8("num", 0644, link_dir, &link.num);

	config_dir = debugfs_create_dir("config", link_dir);
	if (!config_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"link config debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_config_init();

	debugfs_create_u32("link_up_timeout_ms", 0644,    config_dir, &link_config.link_up_timeout_ms);
	debugfs_create_u32("link_up_tries_max", 0644,     config_dir, &link_config.link_up_tries_max);
	sl_test_debugfs_create_s32("fec_up_settle_wait_ms", 0644, config_dir, &link_config.fec_up_settle_wait_ms);
	sl_test_debugfs_create_s32("fec_up_check_wait_ms", 0644,  config_dir, &link_config.fec_up_check_wait_ms);
	sl_test_debugfs_create_s32("fec_up_ccw_limit", 0644,      config_dir, &link_config.fec_up_ccw_limit);
	sl_test_debugfs_create_s32("fec_up_ucw_limit", 0644,      config_dir, &link_config.fec_up_ucw_limit);

	rtn = sl_test_debugfs_create_opt("lock", 0644, config_dir, &config_option_lock);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link config lock debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("autoneg", 0644, config_dir, &config_option_autoneg);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link config autoneg debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("headshell_loopback", 0644, config_dir, &config_option_headshell_loopback);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link config headshell_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("remote_loopback", 0644, config_dir, &config_option_remote_loopback);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link config remote_loopback debugfs_create_file failed");
		return -ENOMEM;
	}

	policy_dir = debugfs_create_dir("policies", link_dir);
	if (!policy_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"link policy debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_link_policy_init();

	sl_test_debugfs_create_s32("fec_mon_period_ms", 0644, policy_dir,
		    &link_policy.fec_mon_period_ms);
	sl_test_debugfs_create_s32("fec_mon_ucw_down_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ucw_down_limit);
	sl_test_debugfs_create_s32("fec_mon_ucw_warn_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ucw_warn_limit);
	sl_test_debugfs_create_s32("fec_mon_ccw_down_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ccw_down_limit);
	sl_test_debugfs_create_s32("fec_mon_ccw_warn_limit", 0644, policy_dir,
		    &link_policy.fec_mon_ccw_warn_limit);

	rtn = sl_test_debugfs_create_opt("lock", 0644, policy_dir, &policy_option_lock);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link policy lock debugfs_create_file failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("keep_serdes_up", 0644, policy_dir, &policy_option_keep_serdes_up);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link policy keep_serdes_up debugfs_create_file failed");
		return -ENOMEM;
	}

	sl_test_options_init();

	test_dir = debugfs_create_dir("test", link_dir);
	if (!test_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"link test debugfs_create_dir failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("use_fec_cntr", 0644, test_dir, &test_option_use_fec_cntr);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"link test use_fec_cntr debugfs_create_file failed");
		return -ENOMEM;
	}

	debugfs_create_u64("fec_ucw", 0644, test_dir, &fec_ucw);
	debugfs_create_u64("fec_ccw", 0644, test_dir, &fec_ccw);
	debugfs_create_u64("fec_gcw", 0644, test_dir, &fec_gcw);

	dentry = debugfs_create_file("cmds", 0644, link_dir, NULL, &sl_test_link_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, link_dir, NULL, &sl_test_link_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

u8 sl_test_debugfs_link_num_get(void)
{
	return link.num;
}

static int sl_test_link_sysfs_init(u8 lgrp_num, u8 link_num)
{
	int             rtn;
	struct kobject *port_kobj;

	port_kobj = sl_test_port_sysfs_kobj_get(lgrp_num);
	if (!port_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_port_sysfs_kobj_get NULL");
		return -EBADRQC;
	}

	if (sl_link_num_dir[lgrp_num][link_num])
		return 0;

	rtn = kobject_init_and_add(&sl_link_num_dir_kobj[lgrp_num][link_num], &sl_test_link_kobj_type,
		port_kobj, "%d", link_num);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"kobject_init_and_add failed [%d]", rtn);
		kobject_put(&sl_link_num_dir_kobj[lgrp_num][link_num]);
		return -ENOMEM;
	}

	sl_link_num_dir[lgrp_num][link_num] = &sl_link_num_dir_kobj[lgrp_num][link_num];

	return 0;
}

static void sl_test_link_sysfs_remove(u8 lgrp_num, u8 link_num)
{
	if (sl_link_num_dir[lgrp_num][link_num]) {
		kobject_put(sl_link_num_dir[lgrp_num][link_num]);
		/* Need to clear kobject state_initialized */
		memset(&sl_link_num_dir_kobj[lgrp_num][link_num], 0, sizeof(sl_link_num_dir_kobj[lgrp_num][link_num]));
		sl_link_num_dir[lgrp_num][link_num] = NULL;
	}
}

static struct kobject *sl_test_link_sysfs_get(u8 lgrp_num, u8 link_num)
{
	return sl_link_num_dir[lgrp_num][link_num];
}

int sl_test_link_new(void)
{
	int             rtn;
	u8              link_num;
	struct sl_lgrp *lgrp;
	struct kobject *link_kobj;

	lgrp = sl_test_lgrp_get();
	link_num = sl_test_debugfs_link_num_get();

	rtn = sl_test_link_sysfs_init(lgrp->num, link_num);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd sl_test_link_sysfs_init failed [%d]", rtn);
		return rtn;
	}

	link_kobj = sl_test_link_sysfs_get(lgrp->num, link_num);
	if (!link_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"sl_test_link_sysfs_get NULL");
		return -EBADRQC;
	}

	return IS_ERR(sl_link_new(lgrp, link_num, link_kobj));
}

int sl_test_link_del(void)
{
	u8              lgrp_num;
	struct sl_link *link;

	link = sl_test_link_get();
	lgrp_num = sl_test_debugfs_lgrp_num_get();

	sl_link_del(link);

	sl_test_link_sysfs_remove(lgrp_num, link->num);

	return 0;
}

int sl_test_link_up(void)
{
	return sl_link_up(sl_test_link_get());
}

int sl_test_link_down(void)
{
	return sl_link_down(sl_test_link_get());
}

int sl_test_link_config_set(void)
{
	return sl_link_config_set(sl_test_link_get(), &link_config);
}

int sl_test_link_policy_set(void)
{
	return sl_link_policy_set(sl_test_link_get(), &link_policy);
}

int sl_test_link_fec_cntr_set(void)
{
	return sl_test_fec_cntrs_set(sl_test_link_get(), fec_ucw, fec_ccw, fec_gcw);
}

int sl_test_link_options_set(void)
{
	return sl_test_fec_cntrs_use_set(sl_test_link_get(), (test_link_options & SL_TEST_OPT_USE_FEC_CNTR));
}
