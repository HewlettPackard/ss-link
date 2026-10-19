// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024-2026 Hewlett Packard Enterprise Development LP */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/debugfs.h>
#include <linux/err.h>

#include <linux/hpe/sl/sl_link.h>
#include <linux/hpe/sl/sl_test.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
#include "sl_link.h"
#include "log/sl_log.h"
#include "sl_test_debugfs_ldev.h"
#include "sl_test_debugfs_lgrp.h"
#include "sl_test_debugfs_link.h"
#include "sl_test_common.h"
#include "sl_test_pdev.h"

#define LOG_BLOCK "link"
#define LOG_NAME  SL_LOG_DEBUGFS_LOG_NAME

static struct dentry               *link_dir;
static struct sl_link               link;
static struct sl_link_config        link_config;
static struct sl_link_policy        link_policy;

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
	LINK_AN_LP_CAPS_GET_CMD,
	NUM_CMDS,
};

static struct cmd_entry link_cmd_list[]  = {
	[LINK_NEW_CMD]            = { .cmd = "link_new",            .desc = "create link object",                 .call = sl_test_link_new            },
	[LINK_DEL_CMD]            = { .cmd = "link_del",            .desc = "delete link object",                 .call = sl_test_link_del            },
	[LINK_CONFIG_WRITE_CMD]   = { .cmd = "link_config_write",   .desc = "write out the link config from dir", .call = sl_test_link_config_set     },
	[LINK_POLICY_WRITE_CMD]   = { .cmd = "link_policy_write",   .desc = "write out the link policy from dir", .call = sl_test_link_policy_set     },
	[UP_CMD]                  = { .cmd = "up",                  .desc = "task link to go up",                 .call = sl_test_link_up             },
	[DOWN_CMD]                = { .cmd = "down",                .desc = "task link to go down",               .call = sl_test_link_down           },
	[CANCEL_CMD]              = { .cmd = "cancel",              .desc = "cancel command",                     .call = sl_test_link_down           },
	[LINK_OPTION_SET_CMD]     = { .cmd = "link_opt_set",        .desc = "set link test options",              .call = sl_test_link_options_set    },
	[LINK_FEC_CNT_SET_CMD]    = { .cmd = "link_fec_cnt_set",    .desc = "set link test fec cntrs",            .call = sl_test_link_fec_cntr_set   },
	[LINK_AN_LP_CAPS_GET_CMD] = { .cmd = "link_an_lp_caps_get", .desc = "get the link partner capabilities",  .call = sl_test_link_an_lp_caps_get },
};

static ssize_t sl_test_link_cmd_write(struct file *f, const char __user *buf, size_t size, loff_t *pos)
{
	int     rtn;
	int     call_rtn;
	ssize_t len;
	char    cmd_buf[CMD_LEN];

	/* Don't allow partial writes */
	if (*pos != 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"partial cmd_write");
		return 0;
	}

	if (!size || size >= sizeof(cmd_buf)) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write too big (size = %ld)", size);
		return -ENOSPC;
	}

	len = simple_write_to_buffer(cmd_buf, sizeof(cmd_buf) - 1, pos, buf, size);
	if (len < 0) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			"cmd_write simple_write_to_buffer [%ld]", len);
		return len;
	}

	cmd_buf[len] = '\0';

	rtn = sl_test_debugfs_cmd_call(cmd_buf, link_cmd_list, ARRAY_SIZE(link_cmd_list), &call_rtn);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write failed (cmd_buf = %s) [%d]", cmd_buf, rtn);
		return rtn;
	}

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME, "cmd_write succeeded (cmd_buf = %s, call_rtn = %d)", cmd_buf, call_rtn);
	if (call_rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "cmd_write call failed (cmd_buf = %s) [%d]", cmd_buf, call_rtn);
		return call_rtn;
	}

	return size;
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

#define SL_TEST_OPT_USE_FEC_CNTR BIT(0)

#define STATIC_CONFIG_OPT_ENTRY(_name, _bit_field) static struct options_field_entry config_option_##_name = { \
	.options = &link_config.options,                                                                       \
	.field   = SL_LINK_CONFIG_OPT_##_bit_field,                                                            \
}

#define STATIC_POLICY_OPT_ENTRY(_name, _bit_field) static struct options_field_entry policy_option_##_name = { \
	.options = &link_policy.options,                                                                       \
	.field   = SL_LINK_POLICY_OPT_##_bit_field,                                                            \
}

#define STATIC_HPE_MAP_ENTRY(_name, _bit_field) static struct options_field_entry hpe_map_##_name##_set = { \
	.options = &link_config.hpe_map,                                                                    \
	.field   = SL_LINK_CONFIG_HPE_##_bit_field,                                                         \
}

#define STATIC_PAUSE_MAP_ENTRY(_name, _bit_field) static struct options_field_entry pause_map_##_name##_set = { \
	.options = &link_config.pause_map,                                                                      \
	.field   = SL_LINK_CONFIG_PAUSE_##_bit_field,                                                           \
}

STATIC_CONFIG_OPT_ENTRY(lock,                 LOCK);
STATIC_CONFIG_OPT_ENTRY(autoneg,              AUTONEG_ENABLE);

STATIC_POLICY_OPT_ENTRY(lock,                      LOCK);
STATIC_POLICY_OPT_ENTRY(keep_serdes_up,            KEEP_SERDES_UP);
STATIC_POLICY_OPT_ENTRY(use_unsupported_cable,     USE_UNSUPPORTED_CABLE);
STATIC_POLICY_OPT_ENTRY(use_supported_ss200_cable, USE_SUPPORTED_SS200_CABLE);
STATIC_POLICY_OPT_ENTRY(ignore_media_error,        IGNORE_MEDIA_ERROR);

STATIC_HPE_MAP_ENTRY(linktrain, LINKTRAIN);
STATIC_HPE_MAP_ENTRY(precode, PRECODING);
STATIC_HPE_MAP_ENTRY(pcal, PCAL);
STATIC_HPE_MAP_ENTRY(r3, R3);
STATIC_HPE_MAP_ENTRY(r2, R2);
STATIC_HPE_MAP_ENTRY(r1, R1);
STATIC_HPE_MAP_ENTRY(c3, C3);
STATIC_HPE_MAP_ENTRY(c2, C2);
STATIC_HPE_MAP_ENTRY(c1, C1);
STATIC_HPE_MAP_ENTRY(llr, LLR);

STATIC_PAUSE_MAP_ENTRY(asym, ASYM);
STATIC_PAUSE_MAP_ENTRY(sym, SYM);

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

static void sl_test_link_init(struct sl_link *link, u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	link->magic    = SL_LINK_MAGIC;
	link->ver      = SL_LINK_VER;
	link->size     = sizeof(*link);
	link->ldev_num = ldev_num;
	link->lgrp_num = lgrp_num;
	link->num      = link_num;
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
	rtn = sl_test_debugfs_entry_check(link_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "link debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_link_init(&link, sl_test_debugfs_ldev_num_get(), sl_test_debugfs_lgrp_num_get(), 0);

	SL_TEST_DEBUGFS_CREATE(u8, "num", link_dir, &link.num);

	config_dir = debugfs_create_dir("config", link_dir);
	rtn = sl_test_debugfs_entry_check(config_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "link config debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_link_config_init();

	SL_TEST_DEBUGFS_CREATE(u32, "link_up_timeout_ms",    config_dir, &link_config.link_up_timeout_ms);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_linktrain_set", config_dir, &hpe_map_linktrain_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_precode_set",   config_dir, &hpe_map_precode_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_pcal_set",      config_dir, &hpe_map_pcal_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_r3_set",        config_dir, &hpe_map_r3_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_r2_set",        config_dir, &hpe_map_r2_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_r1_set",        config_dir, &hpe_map_r1_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_c3_set",        config_dir, &hpe_map_c3_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_c2_set",        config_dir, &hpe_map_c2_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_c1_set",        config_dir, &hpe_map_c1_set);
	SL_TEST_DEBUGFS_CREATE(opt, "hpe_map_llr_set",       config_dir, &hpe_map_llr_set);
	SL_TEST_DEBUGFS_CREATE(opt, "pause_map_asym_set",    config_dir, &pause_map_asym_set);
	SL_TEST_DEBUGFS_CREATE(opt, "pause_map_sym_set",     config_dir, &pause_map_sym_set);
	SL_TEST_DEBUGFS_CREATE(s32, "link_up_tries_max",     config_dir, &link_config.link_up_tries_max);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_up_settle_wait_ms", config_dir, &link_config.fec_up_settle_wait_ms);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_up_check_wait_ms",  config_dir, &link_config.fec_up_check_wait_ms);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_up_ccw_limit",      config_dir, &link_config.fec_up_ccw_limit);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_up_ucw_limit",      config_dir, &link_config.fec_up_ucw_limit);
	SL_TEST_DEBUGFS_CREATE(opt, "lock",                  config_dir, &config_option_lock);
	SL_TEST_DEBUGFS_CREATE(opt, "autoneg",               config_dir, &config_option_autoneg);

	policy_dir = debugfs_create_dir("policies", link_dir);
	rtn = sl_test_debugfs_entry_check(policy_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "link policy debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	sl_test_link_policy_init();

	SL_TEST_DEBUGFS_CREATE(s32, "fec_mon_period_ms",         policy_dir, &link_policy.fec_mon_period_ms);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_mon_ucw_down_limit",    policy_dir, &link_policy.fec_mon_ucw_down_limit);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_mon_ucw_warn_limit",    policy_dir, &link_policy.fec_mon_ucw_warn_limit);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_mon_ccw_down_limit",    policy_dir, &link_policy.fec_mon_ccw_down_limit);
	SL_TEST_DEBUGFS_CREATE(s32, "fec_mon_ccw_warn_limit",    policy_dir, &link_policy.fec_mon_ccw_warn_limit);
	SL_TEST_DEBUGFS_CREATE(opt, "lock",                      policy_dir, &policy_option_lock);
	SL_TEST_DEBUGFS_CREATE(opt, "keep_serdes_up",            policy_dir, &policy_option_keep_serdes_up);
	SL_TEST_DEBUGFS_CREATE(opt, "use_unsupported_cable",     policy_dir, &policy_option_use_unsupported_cable);
	SL_TEST_DEBUGFS_CREATE(opt, "ignore_media_error",        policy_dir, &policy_option_ignore_media_error);
	SL_TEST_DEBUGFS_CREATE(opt, "use_supported_ss200_cable", policy_dir, &policy_option_use_supported_ss200_cable);

	sl_test_options_init();

	test_dir = debugfs_create_dir("test", link_dir);
	rtn = sl_test_debugfs_entry_check(test_dir);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "link test debugfs_create_dir failed [%d]", rtn);
		return rtn;
	}

	SL_TEST_DEBUGFS_CREATE(opt, "use_fec_cntr", test_dir, &test_option_use_fec_cntr);
	SL_TEST_DEBUGFS_CREATE(u64, "fec_ucw",      test_dir, &fec_ucw);
	SL_TEST_DEBUGFS_CREATE(u64, "fec_ccw",      test_dir, &fec_ccw);
	SL_TEST_DEBUGFS_CREATE(u64, "fec_gcw",      test_dir, &fec_gcw);

	dentry = debugfs_create_file("cmds", 0644, link_dir, NULL, &sl_test_link_cmds_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmds debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	dentry = debugfs_create_file("cmd", 0644, link_dir, NULL, &sl_test_link_cmd_fops);
	rtn = sl_test_debugfs_entry_check(dentry);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "cmd debugfs_create_file failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

u8 sl_test_debugfs_link_num_get(void)
{
	return link.num;
}

void sl_test_link_remove(u8 ldev_num, u8 lgrp_num, u8 link_num)
{
	int            rtn;
	struct sl_link sl_link;

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"link remove (ldev_num = %u, lgrp_num = %u, link_num = %u)",
		ldev_num, lgrp_num, link_num);

	sl_test_link_init(&sl_link, ldev_num, lgrp_num, link_num);

	rtn = sl_link_del(&sl_link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_link_del failed [%d]", rtn);
		return;
	}

	rtn = sl_test_pdev_port_kobj_put(ldev_num, lgrp_num, link_num);
	if (rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", rtn);
}

int sl_test_link_new(void)
{
	int              rtn;
	int              put_rtn;
	u8               link_num;
	struct sl_lgrp  *lgrp;
	struct sl_link  *new_link;
	struct kobject  *port_num_kobj;

	lgrp     = sl_test_lgrp_get();
	link_num = sl_test_debugfs_link_num_get();

	sl_log_dbg(NULL, LOG_BLOCK, LOG_NAME,
		"link new (ldev_num = %u, lgrp_num = %u, link_num = %u)",
		lgrp->ldev_num, lgrp->num, link_num);

	port_num_kobj = sl_test_pdev_port_kobj_get(lgrp->ldev_num, lgrp->num, link_num);
	if (!port_num_kobj) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "sl_test_pdev_port_kobj_get failed");
		return -ENODEV;
	}

	new_link = sl_link_new(lgrp, link_num, port_num_kobj);
	if (IS_ERR_OR_NULL(new_link)) {
		rtn = new_link ? PTR_ERR(new_link) : -ENOMEM;
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME,
			   "sl_link_new failed [%d]", rtn);
		put_rtn = sl_test_pdev_port_kobj_put(lgrp->ldev_num, lgrp->num, link_num);
		if (put_rtn)
			sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", put_rtn);
		return rtn;
	}

	return 0;
}

int sl_test_link_del(void)
{
	int             rtn;
	int             put_rtn;
	struct sl_link *link;

	link = sl_test_link_get();

	rtn = sl_link_del(link);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_link_del failed [%d]", rtn);
		return rtn;
	}

	put_rtn = sl_test_pdev_port_kobj_put(link->ldev_num, link->lgrp_num, link->num);
	if (put_rtn)
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "sl_test_pdev_port_kobj_put failed [%d]", put_rtn);

	return rtn;
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

int sl_test_link_an_lp_caps_get(void)
{
	struct sl_link_caps lp_caps;

	return sl_link_an_lp_caps_get(sl_test_link_get(), &lp_caps, 4000, 0);
}

int sl_test_link_options_set(void)
{
	return sl_test_fec_cntrs_use_set(sl_test_link_get(), (test_link_options & SL_TEST_OPT_USE_FEC_CNTR));
}
