// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/debugfs.h>
#include <linux/sl_link.h>
#include <linux/sl_llr.h>
#include <linux/sl_test.h>
#include <linux/kobject.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
#include "sl_llr.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_debugfs_llr.h"
#include "sl_test_common.h"

#define LOG_BLOCK "llr"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry *llr_dir;
struct sl_llr         llr;
struct sl_llr_config  llr_config;
struct sl_llr_policy  llr_policy;

enum llr_cmd_index {
	LLR_NEW_CMD,
	LLR_DEL_CMD,
	LLR_CONFIG_WRITE_CMD,
	LLR_POLICY_WRITE_CMD,
	LLR_START_CMD,
	LLR_STOP_CMD,
	NUM_CMDS,
};

static struct cmd_entry llr_cmd_list[]  = {
	[LLR_NEW_CMD]          = { .cmd = "llr_new",          .desc = "create llr object"                  },
	[LLR_DEL_CMD]          = { .cmd = "llr_del",          .desc = "delete llr object"                  },
	[LLR_CONFIG_WRITE_CMD] = { .cmd = "llr_config_write", .desc = "write out the llr config from dir"  },
	[LLR_POLICY_WRITE_CMD] = { .cmd = "llr_policy_write", .desc = "write out the llr policy from dir"  },
	[LLR_START_CMD]        = { .cmd = "llr_start",        .desc = "start LLR"                          },
	[LLR_STOP_CMD]         = { .cmd = "llr_stop",         .desc = "stop LLR"                           },
};

#define LLR_CMD_MATCH(_index, _str) (strncmp((_str), llr_cmd_list[_index].cmd, strlen(llr_cmd_list[_index].cmd)) == 0)

static ssize_t sl_test_llr_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
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

	match_found = LLR_CMD_MATCH(LLR_NEW_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_new();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_new failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LLR_CMD_MATCH(LLR_DEL_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_del();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_del failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LLR_CMD_MATCH(LLR_CONFIG_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_config_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_config_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LLR_CMD_MATCH(LLR_POLICY_WRITE_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_policy_set();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_policy_set failed [%d]", rtn);
			return rtn;
		}
		return size;
	}


	match_found = LLR_CMD_MATCH(LLR_START_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_start();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_start failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	match_found = LLR_CMD_MATCH(LLR_STOP_CMD, cmd_buf);
	if (match_found) {
		rtn = sl_test_llr_stop();
		if (rtn < 0) {
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
				"sl_test_llr_stop failed [%d]", rtn);
			return rtn;
		}
		return size;
	}

	sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
		"cmd_write no cmd found (cmd_buf = %s)", cmd_buf);

	return -EBADRQC;
}

static const struct file_operations sl_test_llr_cmd_fops = {
	.owner = THIS_MODULE,
	.open  = simple_open,
	.write = sl_test_llr_cmd_write,
};

static int sl_test_llr_cmds_show(struct seq_file *s, void *unused)
{
	return sl_test_cmds_show(s, llr_cmd_list, ARRAY_SIZE(llr_cmd_list));
}

static int sl_test_llr_cmds_open(struct inode *inode, struct file *file)
{
	return single_open(file, sl_test_llr_cmds_show, inode->i_private);
}

static const struct file_operations sl_test_llr_cmds_fops = {
	.owner   = THIS_MODULE,
	.open    = sl_test_llr_cmds_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

#define STATIC_CONFIG_OPT_ENTRY(_name, _bit_field) static struct options_field_entry config_option_##_name = { \
	.options = &llr_config.options,                                                                        \
	.field   = SL_LLR_CONFIG_OPT_##_bit_field,                                                             \
}

#define STATIC_POLICY_OPT_ENTRY(_name, _bit_field) static struct options_field_entry policy_option_##_name = { \
	.options = &llr_policy.options,                                                                        \
	.field   = SL_LLR_POLICY_OPT_##_bit_field,                                                             \
}

STATIC_CONFIG_OPT_ENTRY(lock, LOCK);
STATIC_POLICY_OPT_ENTRY(lock, LOCK);

static void sl_test_llr_init(struct sl_llr *llr, u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	llr->magic    = SL_LLR_MAGIC;
	llr->ver      = SL_LLR_VER;
	llr->ldev_num = ldev_num;
	llr->lgrp_num = lgrp_num;
	llr->num      = link_num;
}

static void sl_test_llr_config_init(void)
{
	llr_config.magic             = SL_LLR_CONFIG_MAGIC;
	llr_config.ver               = SL_LLR_CONFIG_VER;
	llr_config.size              = sizeof(llr_config);
	llr_config.options          |= SL_LLR_CONFIG_OPT_ADMIN;
	llr_config.mode              = 0;
	llr_config.link_dn_behavior  = 0;
	llr_config.setup_timeout_ms  = 0;
	llr_config.start_timeout_ms  = 0;
}

static void sl_test_llr_policy_init(void)
{
	llr_policy.magic    = SL_LLR_POLICY_MAGIC;
	llr_policy.ver      = SL_LLR_POLICY_VER;
	llr_policy.size     = sizeof(llr_policy);
	llr_policy.options |= SL_LLR_POLICY_OPT_ADMIN;
}

static struct sl_llr *sl_test_llr_get(void)
{
	llr.ldev_num = sl_test_debugfs_ldev_num_get();
	llr.lgrp_num = sl_test_debugfs_lgrp_num_get();

	return &llr;
}

int sl_test_debugfs_llr_create(struct dentry *top_dir)
{
	int            rtn;
	struct dentry *config_dir;
	struct dentry *policy_dir;
	struct dentry *dentry;

	llr_dir = debugfs_create_dir("llr", top_dir);
	if (!llr_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"llr debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_llr_init(&llr, sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 0);

	debugfs_create_u8("num", 0644, llr_dir, &llr.num);

	config_dir = debugfs_create_dir("config", llr_dir);
	if (!config_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"llr config debugfs_create_dir failed");
		return -ENOMEM;
	}

	sl_test_llr_config_init();

	debugfs_create_u32("mode", 0644, config_dir, &llr_config.mode);
	debugfs_create_u32("down_behavior", 0644, config_dir, &llr_config.link_dn_behavior);
	debugfs_create_u32("setup_timeout_ms", 0644, config_dir, &llr_config.setup_timeout_ms);
	debugfs_create_u32("start_timeout_ms", 0644, config_dir, &llr_config.start_timeout_ms);

	rtn = sl_test_debugfs_create_opt("lock", 0644, config_dir, &config_option_lock);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"llr config lock debugfs_create_file failed");
		return -ENOMEM;
	}

	sl_test_llr_policy_init();

	policy_dir = debugfs_create_dir("policies", llr_dir);
	if (!policy_dir) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"llr policy debugfs_create_dir failed");
		return -ENOMEM;
	}

	rtn = sl_test_debugfs_create_opt("lock", 0644, policy_dir, &policy_option_lock);
	if (rtn) {
		sl_log_err_trace(NULL, LOG_BLOCK, LOG_NAME,
			"llr policy lock debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmds", 0644, llr_dir, NULL, &sl_test_llr_cmds_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmds debugfs_create_file failed");
		return -ENOMEM;
	}

	dentry = debugfs_create_file("cmd", 0644, llr_dir, NULL, &sl_test_llr_cmd_fops);
	if (!dentry) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd debugfs_create_file failed");
		return -ENOMEM;
	}

	return 0;
}

static u8 sl_test_debugfs_llr_num_get(void)
{
	return llr.num;
}

void sl_test_llr_remove(u8 ldev_num, u8 lgrp_num, u8 llr_num)
{
	int           rtn;
	struct sl_llr sl_llr;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"llr remove (ldev_num = %u, lgrp_num = %u, llr_num = %u)",
		ldev_num, lgrp_num, llr_num);

	sl_test_llr_init(&sl_llr, ldev_num, lgrp_num, llr_num);

	rtn = sl_llr_del(&sl_llr);
	if (rtn)
		sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "sl_llr_del failed [%d]", rtn);
}

int sl_test_llr_new(void)
{
	u8              llr_num;
	struct sl_lgrp *lgrp;
	struct kobject *llr_kobj;

	lgrp = sl_test_lgrp_get();
	llr_num = sl_test_debugfs_llr_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"llr new (lgrp_num = %u, llr_num = %u)",
		lgrp->num, llr_num);

	// NOTE: assumes the link is made first
	llr_kobj = sl_test_link_sysfs_get(lgrp->num, llr_num);
	if (!llr_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"sl_test_link_sysfs_get NULL");
		return -EBADRQC;
	}

	return IS_ERR(sl_llr_new(lgrp, llr_num, llr_kobj));
}

int sl_test_llr_del(void)
{
	sl_llr_del(sl_test_llr_get());

	return 0;
}

int sl_test_llr_start(void)
{
	return sl_llr_start(sl_test_llr_get());
}

int sl_test_llr_stop(void)
{
	return sl_llr_stop(sl_test_llr_get());
}

int sl_test_llr_config_set(void)
{
	return sl_llr_config_set(sl_test_llr_get(), &llr_config);
}

int sl_test_llr_policy_set(void)
{
	return sl_llr_policy_set(sl_test_llr_get(), &llr_policy);
}
