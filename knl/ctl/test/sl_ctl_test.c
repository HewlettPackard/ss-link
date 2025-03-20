// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/completion.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include "sl_asic.h"

#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include "test/sl_ctl_test.h"
#include "test/sl_ctl_test_fec.h"
#include "test/sl_core_test_fec.h"

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_priv.h"
#include "sl_ctl_link_fec.h"
#include "sl_ctl_link_counters.h"
#include "sl_ctl_llr.h"
#include "sl_ctl_mac.h"

#define SL_CTL_TEST_NOTIF_TIMEOUT_S 20
#define SL_CTL_TEST_NAME            "sl-test-ctl: "

struct sl_ctl_test_args {
	u8              ldev_num;
	u8              lgrp_num;
	u8              link_num;
	u32             flags;
};

#define SL_CTL_TEST_FUNC(_num) \
	static int sl_ctl_test##_num(struct sl_ctl_test_args test_args)

#define SL_CTL_TEST_FUNC_RESERVED(_num)	\
	static int sl_ctl_test##_num(struct sl_ctl_test_args test_args) \
	{                                                               \
		return -1;                                              \
	}

SL_CTL_TEST_FUNC(0);
SL_CTL_TEST_FUNC(1);
SL_CTL_TEST_FUNC(2);
SL_CTL_TEST_FUNC(3);
SL_CTL_TEST_FUNC(4);
SL_CTL_TEST_FUNC(5);
SL_CTL_TEST_FUNC(6);
SL_CTL_TEST_FUNC(7);
SL_CTL_TEST_FUNC(8);
SL_CTL_TEST_FUNC(9);
SL_CTL_TEST_FUNC_RESERVED(10);
SL_CTL_TEST_FUNC(11);
SL_CTL_TEST_FUNC(12);
SL_CTL_TEST_FUNC(13);
SL_CTL_TEST_FUNC(14);
SL_CTL_TEST_FUNC(15);
SL_CTL_TEST_FUNC(16);
SL_CTL_TEST_FUNC(17);
SL_CTL_TEST_FUNC(18);
SL_CTL_TEST_FUNC(19);
SL_CTL_TEST_FUNC(20);
SL_CTL_TEST_FUNC(21);
SL_CTL_TEST_FUNC(22);
SL_CTL_TEST_FUNC_RESERVED(23);
SL_CTL_TEST_FUNC(24);
SL_CTL_TEST_FUNC(25);
SL_CTL_TEST_FUNC(26);
SL_CTL_TEST_FUNC(27);
SL_CTL_TEST_FUNC(28);
SL_CTL_TEST_FUNC(29);
SL_CTL_TEST_FUNC(30);
SL_CTL_TEST_FUNC(31);
SL_CTL_TEST_FUNC(32);
SL_CTL_TEST_FUNC(33);
SL_CTL_TEST_FUNC(34);
SL_CTL_TEST_FUNC(35);
SL_CTL_TEST_FUNC(36);
SL_CTL_TEST_FUNC(37);
SL_CTL_TEST_FUNC(38);

#define SL_CTL_TEST_RESERVED 0xFFFFFFFF /* reserved test */

/* Prerequisites for the test to run. */
#define SL_CTL_TEST_FLAG_PREREQ_NO_HW    BIT(0)
#define SL_CTL_TEST_FLAG_PREREQ_LDEV     BIT(1)
#define SL_CTL_TEST_FLAG_PREREQ_LGRP_NUM BIT(2)
#define SL_CTL_TEST_FLAG_PREREQ_LGRP     BIT(3)
#define SL_CTL_TEST_FLAG_PREREQ_LINK_NUM BIT(4)
#define SL_CTL_TEST_FLAG_PREREQ_LINK     BIT(5)

/* Flags for waiting on notifications */
#define SL_CTL_TEST_FLAG_LGRP_NOTIF_WAIT BIT(6)
#define SL_CTL_TEST_FLAG_LINK_NOTIF_WAIT BIT(7)

/* Test Flag Helpers */
#define SL_CTL_TEST_FLAG_WAIT_MASK (SL_CTL_TEST_FLAG_LGRP_NOTIF_WAIT | SL_CTL_TEST_FLAG_LINK_NOTIF_WAIT)
#define SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT (SL_CTL_TEST_FLAG_PREREQ_LINK | SL_CTL_TEST_FLAG_LINK_NOTIF_WAIT)

struct sl_ctl_test {
	const char desc[42];
	int  (*func)(struct sl_ctl_test_args test_args);
	const u32  flags;
	const u32  notif;
};

#define SL_CTL_TEST(_num, _desc, _flags, _notif) \
	[_num] = {                               \
		.desc  = _desc,                  \
		.func  = sl_ctl_test##_num,      \
		.flags = _flags,                 \
		.notif = _notif,                 \
	}

static const struct sl_ctl_test sl_ctl_tests[] = {
	SL_CTL_TEST(0, "List of tests", SL_CTL_TEST_FLAG_PREREQ_NO_HW, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(1, "Create new lgrp", SL_CTL_TEST_FLAG_PREREQ_LGRP_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(2, "Set lgrp config", SL_CTL_TEST_FLAG_PREREQ_LGRP, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(3, "Create new link", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(4, "Set link config", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(5, "Delete lgrp", SL_CTL_TEST_FLAG_PREREQ_LGRP_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(6, "Delete link", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(7, "Get link status", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(8, "Set link up", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_UP),
	SL_CTL_TEST(9, "Set link async down", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_ASYNC_DOWN),
	SL_CTL_TEST(10, "reserved", SL_CTL_TEST_FLAG_PREREQ_NO_HW, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(11, "Verify BER calculations", SL_CTL_TEST_FLAG_PREREQ_NO_HW, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(12, "Verify link config", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(13, "Verify FEC rate check", SL_CTL_TEST_FLAG_PREREQ_NO_HW, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(14, "Save link counter to a", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(15, "Save link counter to b", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(16, "Link counter incremented by 1", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(17, "Link counter remained the same", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(18, "Sleep for FEC monitor period", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(19, "Verify UCW down notification", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT,
	     SL_LGRP_NOTIF_LINK_ASYNC_DOWN),
	SL_CTL_TEST(20, "Verify CCW crit notification", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT,
	     SL_LGRP_NOTIF_LINK_ASYNC_DOWN),
	SL_CTL_TEST(21, "Verify UCW warn notification", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_UCW_WARN),
	SL_CTL_TEST(22, "Verify CCW warn notification", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_CCW_WARN),
	SL_CTL_TEST(23, "reserved", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(24, "Print fec tail array", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(25, "Print fec info", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(26, "Set link up expect up fail", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_UP_FAIL),
	SL_CTL_TEST(27, "Set link policy", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(28, "Set down cause", SL_CTL_TEST_FLAG_PREREQ_NO_HW, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(29, "Stop FEC cntr inc", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(30, "Get FEC policy", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(31, "FEC Down limit off", SL_CTL_TEST_FLAG_PREREQ_LINK_WAIT, SL_LGRP_NOTIF_LINK_CCW_WARN),
	SL_CTL_TEST(32, "Delete MAC", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(33, "Delete LLR", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(34, "Create new MAC", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(35, "Create new LLR", SL_CTL_TEST_FLAG_PREREQ_LINK_NUM, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(36, "Set LLR config", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(37, "Verify link policy", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
	SL_CTL_TEST(38, "Set Test FEC Cntrs use", SL_CTL_TEST_FLAG_PREREQ_LINK, SL_LGRP_NOTIF_INVALID),
};

#define SL_CTL_TEST_NUM_TESTS ARRAY_SIZE(sl_ctl_tests)

// FIXME: need to correct sl_ctl_test namspace

static u64 test_down_cause_map;

enum test_link_config {
	TEST_LINK_CFG_FEC_UP_OFF,
	TEST_LINK_CFG_FEC_UP_ON,
	NUM_TEST_LINK_CFGS,
};

static struct sl_llr_config test_llr_config = {
	.ver              = SL_LLR_CONFIG_VER,
	.magic            = SL_LLR_CONFIG_MAGIC,
	.mode             = SL_LGRP_LLR_MODE_OFF,
	.setup_timeout_ms = 0,
	.start_timeout_ms = 0,
	.link_dn_behavior = SL_LLR_LINK_DN_BEHAVIOR_DISCARD,
};

static struct sl_link_config test_link_configs[NUM_TEST_LINK_CFGS] = {
	[TEST_LINK_CFG_FEC_UP_OFF] = {
		.magic                  = SL_LINK_CONFIG_MAGIC,
		.ver                    = SL_LINK_CONFIG_VER,

		.link_up_timeout_ms     = 18000,
		.link_up_tries_max      = 0,

		.fec_up_ucw_limit       = 42,
		.fec_up_ccw_limit       = 17000000,
		.fec_up_settle_wait_ms  = 0, /* OFF */
		.fec_up_check_wait_ms   = 0, /* OFF */

		.pause_map              = 0,
		.hpe_map                = 0,

		.options                = 0,
	},
	[TEST_LINK_CFG_FEC_UP_ON] = {
		.magic                  = SL_LINK_CONFIG_MAGIC,
		.ver                    = SL_LINK_CONFIG_VER,

		.link_up_timeout_ms     = 18000,
		.link_up_tries_max      = 0,

		.fec_up_ucw_limit       = 42,
		.fec_up_ccw_limit       = 17000000,
		.fec_up_settle_wait_ms  = 250,
		.fec_up_check_wait_ms   = 1000,

		.pause_map              = 0,
		.hpe_map                = 0,

		.options                = 0,
	},
};

enum test_link_policy {
	TEST_LINK_POL_FEC_MON_OFF,
	TEST_LINK_POL_FEC_MON_ON,
	TEST_LINK_POL_FEC_MON_ON_DOWN_OFF_CCW_WARN_ON,
	NUM_TEST_LINK_POLICIES,
};

static struct sl_link_policy test_link_policies[NUM_TEST_LINK_POLICIES] = {
	[TEST_LINK_POL_FEC_MON_OFF] = {
		.magic           = SL_LINK_POLICY_MAGIC,
		.ver             = SL_LINK_POLICY_VER,

		.fec_mon_ucw_down_limit = 42,
		.fec_mon_ccw_crit_limit = 17000000,
		.fec_mon_ucw_warn_limit = 31,
		.fec_mon_ccw_warn_limit = 12750000,
		.fec_mon_period_ms      = 0,        /* OFF */
	},
	[TEST_LINK_POL_FEC_MON_ON] = {
		.magic           = SL_LINK_POLICY_MAGIC,
		.ver             = SL_LINK_POLICY_VER,

		.fec_mon_ucw_down_limit = 42,
		.fec_mon_ccw_crit_limit = 17000000,
		.fec_mon_ucw_warn_limit = 31,
		.fec_mon_ccw_warn_limit = 12750000,
		.fec_mon_period_ms      = 1000,     /* 1s */
	},
	//TODO: Maybe test other configurations as well.
	[TEST_LINK_POL_FEC_MON_ON_DOWN_OFF_CCW_WARN_ON] = {
		.magic           = SL_LINK_POLICY_MAGIC,
		.ver             = SL_LINK_POLICY_VER,

		.fec_mon_ucw_down_limit = 0,
		.fec_mon_ccw_crit_limit = 0,
		.fec_mon_ucw_warn_limit = 0,
		.fec_mon_ccw_warn_limit = 12750000,
		.fec_mon_period_ms      = 1000,     /* 1s */
	},

};

struct sl_link_setup_data {
	struct sl_link_config *link_config;
	struct sl_llr_config  *llr_config;
	struct sl_link_policy *policy;
};

enum test_lgrp_config {
	TEST_LGRP_CONFIG_X1_200G,
	NUM_TEST_LGRP_CONFIGS,
};

static struct sl_lgrp_config lgrp_configs[NUM_TEST_LGRP_CONFIGS] = {
	[TEST_LGRP_CONFIG_X1_200G] = {
		.magic      = SL_LGRP_CONFIG_MAGIC,
		.ver        = SL_LGRP_CONFIG_VER,

		.mfs        = 1500,
		.furcation  = SL_MEDIA_FURCATION_X1,
		.fec_mode   = 0,

		.tech_map   = SL_LGRP_CONFIG_TECH_BS_200G,
		.fec_map    = SL_LGRP_CONFIG_FEC_RS,

		.options    = 0,
	},
};

static int               link_counters_a[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS][SL_CTL_LINK_COUNTERS_COUNT];
static int               link_counters_b[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS][SL_CTL_LINK_COUNTERS_COUNT];
static u32               lgrp_notif[SL_ASIC_MAX_LGRPS];
static u32               link_notif[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static struct completion lgrp_notif_complete[SL_ASIC_MAX_LGRPS];
static struct completion link_notif_complete[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static u32               current_test_num;
static int               test_rtns[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];

static bool sl_ctl_link_ber_eq(struct sl_ber *first, struct sl_ber *second)
{
	return ((first->mant == second->mant) && (first->exp == second->exp));
}

//TODO: Eventually check entire configuration, not just FEC.
static bool sl_ctl_link_config_fec_eq(struct sl_link_config *first, struct sl_link_config *second)
{
	//TODO: Check other fields
	return ((first->fec_up_settle_wait_ms == second->fec_up_settle_wait_ms) &&
		(first->fec_up_check_wait_ms == second->fec_up_check_wait_ms) &&
		(first->fec_up_ucw_limit == second->fec_up_ucw_limit) &&
		(first->fec_up_ccw_limit == second->fec_up_ccw_limit));
}

//TODO: Eventually check entire policy, not just FEC.
static bool sl_ctl_link_policy_fec_eq(struct sl_link_policy *first, struct sl_link_policy *second)
{
	return ((first->fec_mon_ucw_down_limit == second->fec_mon_ucw_down_limit) &&
		(first->fec_mon_ucw_warn_limit == second->fec_mon_ucw_warn_limit) &&
		(first->fec_mon_ccw_crit_limit == second->fec_mon_ccw_crit_limit) &&
		(first->fec_mon_ccw_warn_limit == second->fec_mon_ccw_warn_limit));
}

/*
 * Tests
 */

/* List of tests */
static int sl_ctl_test0(struct sl_ctl_test_args test_args)
{
	int i;

	pr_info(SL_CTL_TEST_NAME "Available tests: (%ld)\n", (SL_CTL_TEST_NUM_TESTS - 1));
	for (i = 1; i < SL_CTL_TEST_NUM_TESTS; ++i) {
		if (sl_ctl_tests[i].flags == SL_CTL_TEST_RESERVED)
			continue;
		pr_info(SL_CTL_TEST_NAME " %2d = %s\n", i, sl_ctl_tests[i].desc);
	}

	return 0;
}

static int sl_ctl_test1(struct sl_ctl_test_args test_args)
{
	int                   rtn;
	struct sl_ctl_lgrp   *ctl_lgrp;

	rtn = sl_ctl_lgrp_new(test_args.ldev_num, test_args.lgrp_num, NULL);
	if (rtn) {
		pr_err(SL_CTL_TEST_NAME " ctl_lgrp_new failed [%d]\n", rtn);
		return rtn;
	}

	ctl_lgrp = sl_ctl_lgrp_get(test_args.ldev_num, test_args.lgrp_num);

	pr_info(SL_CTL_TEST_NAME "lgrp create (lgrp = %p)\n", ctl_lgrp);

	return 0;
}

/* Config lgrp */
static int sl_ctl_test2(struct sl_ctl_test_args test_args)
{
	int rtn;

	rtn = sl_ctl_lgrp_config_set(test_args.ldev_num,
		test_args.lgrp_num, &lgrp_configs[TEST_LGRP_CONFIG_X1_200G]);
	if (rtn)
		return rtn;

	return rtn;
}

/* Create new link */
static int sl_ctl_test3(struct sl_ctl_test_args test_args)
{
	return sl_ctl_link_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, NULL);
}

/* Set link config */
static int sl_ctl_test4(struct sl_ctl_test_args test_args)
{
	enum test_link_config config_num;

	if ((test_args.flags > NUM_TEST_LINK_CFGS) || (test_args.flags < 0)) {
		pr_err(SL_CTL_TEST_NAME "Invalid link config (config_num = 0x%X)", test_args.flags);
		return -EINVAL;
	}

	config_num = test_args.flags;

	sl_ctl_link_config_set(test_args.ldev_num, test_args.lgrp_num,
		test_args.link_num, &test_link_configs[config_num]);

	return 0;
}

/* Delete lgrp */
static int sl_ctl_test5(struct sl_ctl_test_args test_args)
{
	sl_ctl_lgrp_del(test_args.ldev_num, test_args.lgrp_num);

	return 0;
}

/* Delete link */
static int sl_ctl_test6(struct sl_ctl_test_args test_args)
{
	sl_ctl_link_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

/* Get link status */
static int sl_ctl_test7(struct sl_ctl_test_args test_args)
{
	int                  rtn;
	u32                  state;
	u32                  core_state;
	u64                  core_imap;
	char                 core_imap_str[SL_LINK_INFO_STRLEN];

	sl_ctl_link_state_get_cmd(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &state);
	core_state = SL_CORE_LINK_STATE_INVALID;
	core_imap  = 0;

	rtn = sl_core_link_state_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &core_state);
	if (rtn)
		pr_warn(SL_CTL_TEST_NAME "sl_core_link_state_get failed [%d]\n", rtn);

	rtn = sl_core_info_map_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &core_imap);
	if (rtn)
		pr_warn(SL_CTL_TEST_NAME "sl_core_info_map_get failed [%d]\n", rtn);

	sl_core_info_map_str(core_imap, core_imap_str, SL_LINK_INFO_STRLEN);

	pr_info(SL_CTL_TEST_NAME "Status (link_state = %s, core_link_state = %s, core_info_map %s)",
		sl_link_state_str(state), sl_core_link_state_str(core_state), core_imap_str);

	return 0;
}

/* Set link up */
static int sl_ctl_test8(struct sl_ctl_test_args test_args)
{
	return sl_ctl_link_up(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

/* Set link down */
static int sl_ctl_test9(struct sl_ctl_test_args test_args)
{
	return sl_ctl_link_down(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

/* Verify BER calculations */
struct expected_ber {
	struct sl_fec_info fec;
	struct sl_ber      ucw;
	struct sl_ber      ccw;
};
static struct expected_ber expected_bers[] = {
	{
		/* BS_200G default config */
		.fec = { .ucw = 21, .ccw = 51000000, .gcw = 39053125, .period_ms = HZ },
		.ucw = { .mant = 98, .exp = -12 },
		.ccw = { .mant = 240, .exp = -6 },	/* Accounted for margin of error */
	},
	{
		/* BS_200G default config */
		.fec = { .ucw = 22, .ccw = 51000001, .gcw = 39053124, .period_ms = HZ },
		.ucw = { .mant = 103, .exp = -12 },
		.ccw = { .mant = 240, .exp = -6 },	/* Accounted for margin of error */
	},
};
static int sl_ctl_test11(struct sl_ctl_test_args test_args)
{
	int           i;
	int           err;
	int           test_rtn;
	struct sl_ber calc_ucw;
	struct sl_ber calc_ccw;

	test_rtn = 0;

	for (i = 0; i < ARRAY_SIZE(expected_bers); ++i) {
		err = sl_ctl_link_fec_ber_calc(&expected_bers[i].fec, &calc_ucw, &calc_ccw);
		if (err) {
			pr_err(SL_CTL_TEST_NAME "link_fec_ber_calc failed [%d]", err);
			return 1;
		}

		if (!sl_ctl_link_ber_eq(&expected_bers[i].ucw, &calc_ucw)) {
			pr_warn(SL_CTL_TEST_NAME "link_ber_eq ucw not equal (%de%.2d != %de%.2d)\n",
				calc_ucw.mant, calc_ucw.exp, expected_bers[i].ucw.mant, expected_bers[i].ucw.exp);
			test_rtn = 1;
		}
		if (!sl_ctl_link_ber_eq(&expected_bers[i].ccw, &calc_ccw)) {
			pr_warn(SL_CTL_TEST_NAME "link_ber_eq ccw not equal (%de%.2d != %de%.2d)\n",
				calc_ccw.mant, calc_ccw.exp, expected_bers[i].ccw.mant, expected_bers[i].ccw.exp);
			test_rtn = 1;
		}
	}

	return test_rtn;
}

/* Verify link config */
static int sl_ctl_test12(struct sl_ctl_test_args test_args)
{
	enum test_link_config config_num;
	struct sl_link_config config;
	struct sl_ctl_link    *ctl_link;

	if ((test_args.flags > NUM_TEST_LINK_CFGS) || (test_args.flags < 0)) {
		pr_err(SL_CTL_TEST_NAME "Invalid link config (config_num = 0x%X)", test_args.flags);
		return -EINVAL;
	}

	config_num = test_args.flags;
	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	sl_ctl_link_config_get(ctl_link, &config);

	/* When the configurations are equal return true, but for a test to pass is return code 0 (false) so invert. */
	return !sl_ctl_link_config_fec_eq(&config, &test_link_configs[config_num]);
}

/* Verify FEC rate check */
struct expected_fec_rate {
	s32                ucw_limit;
	s32                ccw_limit;
	struct sl_fec_info info;
};
static struct expected_fec_rate exceed_fec_rates[] = {
	{
		/* BS_200G - Threshold set to exceed */
		.ucw_limit = 21,
		.ccw_limit = 51000000,
		.info = {
			.ucw       = 21,
			.ccw       = 51000000,
			.gcw       = 39053126,
			.period_ms = HZ,
		}
	},
	{
		/* Minimum we can detect */
		.ucw_limit = 1,
		.ccw_limit = 1,
		.info = {
			.ucw       = 1,
			.ccw       = 1,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
	{
		/* Maximum we can detect */
		.ucw_limit = INT_MAX,
		.ccw_limit = INT_MAX,
		.info = {
			.ucw       = UINT_MAX,
			.ccw       = UINT_MAX,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
};
static struct expected_fec_rate no_exceed_fec_rates[] = {
	{
		/* Threshold off */
		.ucw_limit = 0,
		.ccw_limit = 0,
		.info = {
			.ucw       = 0,
			.ccw       = 0,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
	{
		/* All Zero */
		.ucw_limit = 0,
		.ccw_limit = 0,
		.info = {
			.ucw       = 0,
			.ccw       = 0,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = 0,
		}
	},
	{
		/* Configured limitold with invalid rate */
		.ucw_limit = 1,
		.ccw_limit = 1,
		.info = {
			.ucw       = 0,
			.ccw       = 0,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = 0,
		}
	},
	{
		/* Period of 0 */
		.ucw_limit = 1,
		.ccw_limit = 1,
		.info = {
			.ucw       = 1,
			.ccw       = 1,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = 0,
		}
	},
	{
		/* Threshold off, but UCW rate shows errors */
		.ucw_limit = 0,
		.ccw_limit = 0,
		.info = {
			.ucw       = UINT_MAX,
			.ccw       = 0,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
	{
		/* Threshold off, but CCW rate shows errors */
		.ucw_limit = 0,
		.ccw_limit = 0,
		.info = {
			.ucw       = 0,
			.ccw       = UINT_MAX,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
	{
		/* Minimum limitold */
		.ucw_limit = 1,
		.ccw_limit = 1,
		.info = {
			.ucw       = 0,
			.ccw       = 0,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms  = HZ,
		}
	},
	{
		/* Maximum limitold */
		.ucw_limit = INT_MAX,
		.ccw_limit = INT_MAX,
		.info = {
			.ucw       = UINT_MAX - 1,
			.ccw       = UINT_MAX - 1,
			.gcw       = 0,	/* Not used in limit check, only for BER calc */
			.period_ms = HZ,
		}
	},
};
static int sl_ctl_test13(struct sl_ctl_test_args test_args)
{
	int i;
	int test_rtn;

	test_rtn = 0;

	/* Threshold should be exceeded */
	for (i = 0; i < ARRAY_SIZE(exceed_fec_rates); ++i) {
		pr_debug(SL_CTL_TEST_NAME "UCW Threshold check (ucw_limit = %u, info = (ucw =%llu, period = %ums))",
			exceed_fec_rates[i].ucw_limit, exceed_fec_rates[i].info.ucw,
			exceed_fec_rates[i].info.period_ms);
		if (!SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(exceed_fec_rates[i].ucw_limit, &exceed_fec_rates[i].info)) {
			pr_warn(SL_CTL_TEST_NAME "UCW Should exceed %u > %llu\n", exceed_fec_rates[i].ucw_limit,
				exceed_fec_rates[i].info.ucw);
			test_rtn = 1;
		}
		pr_debug(SL_CTL_TEST_NAME "CCW Threshold check (ccw_limit = %u, info = (ccw =%llu, period = %ums))",
			exceed_fec_rates[i].ccw_limit, exceed_fec_rates[i].info.ccw,
			exceed_fec_rates[i].info.period_ms);
		if (!SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(exceed_fec_rates[i].ccw_limit, &exceed_fec_rates[i].info)) {
			pr_warn(SL_CTL_TEST_NAME "CCW Should exceed %u > %llu\n", exceed_fec_rates[i].ccw_limit,
				exceed_fec_rates[i].info.ccw);
			test_rtn = 1;
		}
	}

	/* Threshold should not be exceeded */
	for (i = 0; i < ARRAY_SIZE(no_exceed_fec_rates); ++i) {
		pr_debug(SL_CTL_TEST_NAME "UCW Threshold check (ucw_limit = %u, info = (ucw =%llu, period = %ums))",
			no_exceed_fec_rates[i].ucw_limit, no_exceed_fec_rates[i].info.ucw,
			no_exceed_fec_rates[i].info.period_ms);
		if (SL_CTL_LINK_FEC_UCW_LIMIT_CHECK(no_exceed_fec_rates[i].ucw_limit,
					 &no_exceed_fec_rates[i].info)) {
			pr_warn(SL_CTL_TEST_NAME "UCW Should NOT exceed %u < %llu\n",
				no_exceed_fec_rates[i].ucw_limit, no_exceed_fec_rates[i].info.ucw);
			test_rtn = 1;
		}

		pr_debug(SL_CTL_TEST_NAME "CCW Threshold check (ccw_limit = %u, info = (ccw =%llu, period = %ums))",
			no_exceed_fec_rates[i].ccw_limit, no_exceed_fec_rates[i].info.ccw,
			no_exceed_fec_rates[i].info.period_ms);
		if (SL_CTL_LINK_FEC_CCW_LIMIT_CHECK(no_exceed_fec_rates[i].ccw_limit,
					 &no_exceed_fec_rates[i].info)) {
			pr_warn(SL_CTL_TEST_NAME "CCW Should NOT exceed %u < %llu\n",
				no_exceed_fec_rates[i].ccw_limit, no_exceed_fec_rates[i].info.ccw);
			test_rtn = 1;
		}
	}

	return test_rtn;
}

/* Save link counter to a */
static int sl_ctl_test14(struct sl_ctl_test_args test_args)
{
	int                 counter;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	for_each_set_bit(counter, (unsigned long *)&test_args.flags, sizeof(test_args.flags) * BITS_PER_BYTE) {
		link_counters_a[test_args.lgrp_num][test_args.link_num][counter] =
			sl_ctl_link_counters_get(ctl_link, counter);
	}

	return 0;
}

/* Save link counter to b */
static int sl_ctl_test15(struct sl_ctl_test_args test_args)
{
	int                 counter;
	struct sl_ctl_link *ctl_link;

	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	for_each_set_bit(counter, (unsigned long *)&test_args.flags, sizeof(test_args.flags) * BITS_PER_BYTE) {
		link_counters_b[test_args.lgrp_num][test_args.link_num][counter] =
			sl_ctl_link_counters_get(ctl_link, counter);
	}

	return 0;
}

/* Link counter incremented by 1 */
static int sl_ctl_test16(struct sl_ctl_test_args test_args)
{
	int counter;

	for_each_set_bit(counter, (unsigned long *)&test_args.flags, sizeof(test_args.flags) * BITS_PER_BYTE) {
		if (link_counters_b[test_args.lgrp_num][test_args.link_num][counter] -
			link_counters_a[test_args.lgrp_num][test_args.link_num][counter] != 1) {

			pr_debug(SL_CTL_TEST_NAME "[%d] b = %d, a = %d", counter,
				link_counters_b[test_args.lgrp_num][test_args.link_num][counter],
				link_counters_a[test_args.lgrp_num][test_args.link_num][counter]);

			return -1;
		}
	}

	return 0;
}

/* Link counter remained the same */
static int sl_ctl_test17(struct sl_ctl_test_args test_args)
{
	int counter;

	for_each_set_bit(counter, (unsigned long *)&test_args.flags, sizeof(test_args.flags) * BITS_PER_BYTE) {
		if (link_counters_b[test_args.lgrp_num][test_args.link_num][counter] -
			link_counters_a[test_args.lgrp_num][test_args.link_num][counter] != 0) {

			pr_debug(SL_CTL_TEST_NAME "[%d] b = %d, a = %d", counter,
				link_counters_b[test_args.lgrp_num][test_args.link_num][counter],
				link_counters_a[test_args.lgrp_num][test_args.link_num][counter]);

			return -1;
		}
	}

	return 0;
}

/* Sleep for FEC monitor period */
static int sl_ctl_test18(struct sl_ctl_test_args test_args)
{
	struct sl_link_policy  policy;
	struct sl_ctl_link    *ctl_link;

	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	sl_ctl_link_policy_get(ctl_link, &policy);
	msleep(policy.fec_mon_period_ms);

	return 0;
}

/* Verify UCW error notification */
static int sl_ctl_test19(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ccw = 0;
	cw_cntrs.ucw = 62;
	cw_cntrs.gcw = 0;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

/* Verify CCW error notification */
static int sl_ctl_test20(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ucw = 0;
	cw_cntrs.ccw = 25500000;
	cw_cntrs.gcw = 0;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

/* Verify UCW warning notification */
static int sl_ctl_test21(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ucw = 41; /* -1 from down limit. */
	cw_cntrs.ccw = 0;
	cw_cntrs.gcw = 0;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

/* Verify CCW warning notification */
static int sl_ctl_test22(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ucw = 0;
	cw_cntrs.ccw = 16999999; /* -1 from down limit. */
	cw_cntrs.gcw = 0;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

/* Print fec tail array */
static int sl_ctl_test24(struct sl_ctl_test_args test_args)
{
	int                rtn;
	struct sl_fec_tail tail;

	rtn = sl_ctl_link_fec_tail_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &tail);

	pr_info(SL_CTL_TEST_NAME
		"FEC Tails: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
		tail.ccw_bins[0], tail.ccw_bins[1], tail.ccw_bins[2], tail.ccw_bins[3],
		tail.ccw_bins[4], tail.ccw_bins[5], tail.ccw_bins[6], tail.ccw_bins[7],
		tail.ccw_bins[8], tail.ccw_bins[9], tail.ccw_bins[10], tail.ccw_bins[11],
		tail.ccw_bins[12], tail.ccw_bins[13], tail.ccw_bins[14]);

	return rtn;
}

/* Print fec info */
static int sl_ctl_test25(struct sl_ctl_test_args test_args)
{
	int                rtn;
	struct sl_fec_info fec_info;

	rtn = sl_ctl_link_fec_info_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &fec_info);

	pr_info(SL_CTL_TEST_NAME "FEC: GCW = %llu, CCW = %llu, UCW = %llu, period = %ums\n",
		fec_info.gcw, fec_info.ccw, fec_info.ucw, fec_info.period_ms);
	pr_info(SL_CTL_TEST_NAME
		"FEC LANES: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
		fec_info.lanes[0], fec_info.lanes[1], fec_info.lanes[2], fec_info.lanes[3], fec_info.lanes[4],
		fec_info.lanes[5], fec_info.lanes[6], fec_info.lanes[7], fec_info.lanes[8], fec_info.lanes[9],
		fec_info.lanes[10], fec_info.lanes[11], fec_info.lanes[12], fec_info.lanes[13], fec_info.lanes[14],
		fec_info.lanes[15]);

	return rtn;
}

/* Set link up expect down */
static int sl_ctl_test26(struct sl_ctl_test_args test_args)
{
	return sl_ctl_link_up(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

/* Set link policy */
static int sl_ctl_test27(struct sl_ctl_test_args test_args)
{
	enum test_link_policy test_policy;

	if ((test_args.flags > NUM_TEST_LINK_POLICIES) || (test_args.flags < 0)) {
		pr_err(SL_CTL_TEST_NAME "Invalid link policy (test_policy = 0x%X)", test_args.flags);
		return -EINVAL;
	}

	test_policy = test_args.flags;

	return sl_ctl_link_policy_set(test_args.ldev_num, test_args.lgrp_num,
		test_args.link_num, &test_link_policies[test_policy]);
}

/* Set down cause */
static int sl_ctl_test28(struct sl_ctl_test_args test_args)
{
	/* Set the down cause used in test_notif_matches */
	test_down_cause_map = test_args.flags;

	return 0;
}

/* Stop FEC cntr inc */
static int sl_ctl_test29(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ucw = 0;
	cw_cntrs.ccw = 0;
	cw_cntrs.gcw = 0;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

//TODO: Extend to other policy fields.
/* Get FEC policy config */
static int sl_ctl_test30(struct sl_ctl_test_args test_args)
{
	struct sl_link_policy  policy;
	struct sl_ctl_link    *ctl_link;

	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	sl_ctl_link_policy_get(ctl_link, &policy);

	pr_info(SL_CTL_TEST_NAME
		"fec_mon_period = %dms, ccw_crit_limit = %d, ucw_down_limit = %d, ccw_warn_limit = %d, ucw_warn_limit = %d\n",
		policy.fec_mon_period_ms,
		policy.fec_mon_ccw_crit_limit, policy.fec_mon_ucw_down_limit,
		policy.fec_mon_ccw_warn_limit, policy.fec_mon_ucw_warn_limit);

	return 0;
}

/* FEC Down limit off */
static int sl_ctl_test31(struct sl_ctl_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs cw_cntrs;

	cw_cntrs.ucw = 0;
	cw_cntrs.ccw = 20000000;
	cw_cntrs.gcw = 19062500;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);
}

/* Delete MAC */
static int sl_ctl_test32(struct sl_ctl_test_args test_args)
{
	sl_ctl_mac_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

/* Delete LLR */
static int sl_ctl_test33(struct sl_ctl_test_args test_args)
{
	sl_ctl_llr_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

/* Create new MAC */
static int sl_ctl_test34(struct sl_ctl_test_args test_args)
{
	return sl_ctl_mac_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, NULL);
}

/* Create new LLR */
static int sl_ctl_test35(struct sl_ctl_test_args test_args)
{
	return sl_ctl_llr_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, NULL);
}

/* Set LLR Configuration */
static int sl_ctl_test36(struct sl_ctl_test_args test_args)
{
	return sl_ctl_llr_config_set(test_args.ldev_num, test_args.lgrp_num,
		test_args.link_num, &test_llr_config);
}

/* Verify link policy */
static int sl_ctl_test37(struct sl_ctl_test_args test_args)
{
	enum test_link_policy policy_num;
	struct sl_link_policy policy;
	struct sl_ctl_link    *ctl_link;

	if ((test_args.flags > NUM_TEST_LINK_POLICIES) || (test_args.flags < 0)) {
		pr_err(SL_CTL_TEST_NAME "Invalid link policy (policy_num = 0x%X)", test_args.flags);
		return -EINVAL;
	}

	policy_num = test_args.flags;
	ctl_link = sl_ctl_link_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	sl_ctl_link_policy_get(ctl_link, &policy);

	/* When the configurations are equal return true, but for a test to pass is return code 0 (false) so invert. */
	return !sl_ctl_link_policy_fec_eq(&policy, &test_link_policies[policy_num]);
}

/* Set Test FEC Cntrs use */
static int sl_ctl_test38(struct sl_ctl_test_args test_args)
{
	sl_ctl_test_fec_cntrs_use_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, test_args.flags);

	return 0;
}

/*
 * sl_ctl_test_notif_matches - Check if notification messages matches expected test notification.
 * @test: Test with expected notification.
 * @msg: Received notification message.
 *
 * Some tests require the down reason to also match in the notification. See &sl_ctl_test28 to set.
 *
 * return: 1 when notification matches, 0 otherwise.
 */
static int sl_ctl_test_notif_matches(const struct sl_ctl_test *test, struct sl_lgrp_notif_msg *msg)
{
	int match;

	u64 up_fail_cause_map;

	if ((test->notif == SL_LGRP_NOTIF_LINK_UP_FAIL) && test_down_cause_map) {
		up_fail_cause_map = msg->info.cause_map;
		pr_debug(SL_CTL_TEST_NAME "[%02u:%u] down_notif (cause_map = 0x%llX)",
			msg->lgrp_num, msg->link_num, up_fail_cause_map);
		match = ((test->notif == msg->type) && (test_down_cause_map == up_fail_cause_map));
		return match;
	}

	return (test->notif == msg->type);
}

static void sl_ctl_test_lgrp_callback_hdlr(void *tag, struct sl_lgrp_notif_msg *msg)
{
	struct sl_ctl_lgrp *ctl_lgrp;
	bool                is_lgrp_notif;

	ctl_lgrp = tag;
	is_lgrp_notif = (msg->link_num == SL_LGRP_NOTIF_NO_LINK);

	pr_debug(SL_CTL_TEST_NAME "[%02u:%u] received %s notification %s\n", msg->lgrp_num, msg->link_num,
		is_lgrp_notif ? "lgrp" : "link", sl_lgrp_notif_str(msg->type));

	if (is_lgrp_notif) {
		if (sl_ctl_test_notif_matches(&sl_ctl_tests[current_test_num], msg)) {
			pr_debug(SL_CTL_TEST_NAME "[%02u:-] completing lgrp notif %s\n", msg->lgrp_num,
				sl_lgrp_notif_str(msg->type));
			lgrp_notif[ctl_lgrp->num] = msg->type;
			complete_all(&lgrp_notif_complete[msg->lgrp_num]);
		} else {
			pr_debug(SL_CTL_TEST_NAME "[%02u:-] lgrp notif no match (notif = %s, test_num = %u)\n",
				msg->lgrp_num, sl_lgrp_notif_str(msg->type), current_test_num);
		}
	} else {
		if (sl_ctl_test_notif_matches(&sl_ctl_tests[current_test_num], msg)) {
			pr_debug(SL_CTL_TEST_NAME "[%u:%u] completing link notif %s\n", msg->link_num, msg->lgrp_num,
				sl_lgrp_notif_str(msg->type));
			link_notif[msg->lgrp_num][msg->link_num] = msg->type;
			complete_all(&link_notif_complete[msg->lgrp_num][msg->link_num]);
		} else {
			pr_debug(SL_CTL_TEST_NAME "[%02u:-] link notif no match (notif = %s, test_num = %u)\n",
				msg->lgrp_num, sl_lgrp_notif_str(msg->type), current_test_num);
		}
	}
}

static void sl_ctl_test_init(void)
{
	u8 lgrp_num;
	u8 link_num;

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		init_completion(&lgrp_notif_complete[lgrp_num]);
		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num)
			init_completion(&link_notif_complete[lgrp_num][link_num]);
	}
}

static int sl_ctl_test_wait_on_lgrp_notifs(int test_num, u64 lgrp_map)
{
	int rtn;
	u8  lgrp_num;
	u8  num_notifs_rx;

	num_notifs_rx = 0;

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {

		if (!test_bit(lgrp_num, (unsigned long *)&lgrp_map))
			continue;

		pr_debug(SL_CTL_TEST_NAME "[%02u:-] Waiting on lgrp notification %s\n", lgrp_num,
			sl_lgrp_notif_str(sl_ctl_tests[test_num].notif));

		/* Timeouts return 0 */
		rtn = wait_for_completion_interruptible_timeout(&lgrp_notif_complete[lgrp_num],
			SL_CTL_TEST_NOTIF_TIMEOUT_S * HZ);

		if (rtn > 0) {
			num_notifs_rx++;
		} else if (rtn == 0) {
			pr_err(SL_CTL_TEST_NAME "[%02u:-] wait_for_completion timedout [%d]", lgrp_num, rtn);
			goto out;
		} else {
			pr_err(SL_CTL_TEST_NAME "[%02u:-] wait_for_completion failed [%d]", lgrp_num, rtn);
			goto out;
		}
	}

out:
	pr_debug(SL_CTL_TEST_NAME "lgrp_notifs (num_lgrps = %lu, num_notifs_rx = %u)",
		hweight64(lgrp_map), num_notifs_rx);

	return (num_notifs_rx == hweight64(lgrp_map));
}

static int sl_ctl_test_wait_on_link_notifs(int test_num, u64 lgrp_map, u64 link_map)
{
	int rtn;
	u8  lgrp_num;
	u8  link_num;
	u8  num_notifs_rx;
	u8  total_links;

	num_notifs_rx = 0;
	total_links = 0;

	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {

		if (!test_bit(lgrp_num, (unsigned long *)&lgrp_map))
			continue;

		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {

			if (!test_bit(link_num, (unsigned long *)&link_map))
				continue;

			total_links++;

			pr_debug(SL_CTL_TEST_NAME "[%02u:%u] Waiting on notification %s\n", lgrp_num, link_num,
				sl_lgrp_notif_str(sl_ctl_tests[test_num].notif));

			/* Timeouts return 0 */
			rtn = wait_for_completion_interruptible_timeout(&link_notif_complete[lgrp_num][link_num],
				SL_CTL_TEST_NOTIF_TIMEOUT_S * HZ);

			if (rtn > 0) {
				num_notifs_rx++;
			} else if (rtn == 0) {
				pr_err(SL_CTL_TEST_NAME "[%02u:%u] wait_for_completion timedout [%d]",
					lgrp_num, link_num, rtn);
				goto out;
			} else {
				pr_err(SL_CTL_TEST_NAME "[%02u:%u] wait_for_completion failed [%d]",
					lgrp_num, link_num, rtn);
				goto out;
			}
		}
	}

out:
	pr_debug(SL_CTL_TEST_NAME "link_notifs (total_links = %u num_notifs_rx = %u)",
		total_links, num_notifs_rx);

	return (num_notifs_rx == total_links);
}

static int sl_ctl_test_wait_on_notifs(int test_num, u64 lgrp_map, u64 link_map)
{
	if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_LGRP_NOTIF_WAIT) {
		if (!sl_ctl_test_wait_on_lgrp_notifs(test_num, lgrp_map))
			return -1;
		return 0;
	}

	if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_LINK_NOTIF_WAIT) {
		if (!sl_ctl_test_wait_on_link_notifs(test_num, lgrp_map, link_map))
			return -1;
		return 0;
	}

	return 0;
}

int sl_ctl_test_exec(u8 ldev_num, u64 lgrp_map, u64 link_map, int test_num, u32 flags)
{
	int                      test_rtn;    /* Final test result */
	int                      notif_unreg; /* Result of unregistering notifications */
	u8                       lgrp_num;
	u8                       link_num;
	struct sl_ctl_test_args  args;
	bool                     is_lgrp_callback_registered;
	struct sl_ctl_lgrp      *ctl_lgrp;

	if ((test_num < 0) || (test_num >= SL_CTL_TEST_NUM_TESTS)) {
		pr_err(SL_CTL_TEST_NAME "invalid (test_num = %d)\n", test_num);
		return -EBADRQC;
	}

	memset(&args, 0, sizeof(args));

	pr_debug(SL_CTL_TEST_NAME "Test (%s)", sl_ctl_tests[test_num].desc);

	current_test_num = test_num;
	sl_ctl_test_init();

	is_lgrp_callback_registered = false;

	args.flags = flags;
	if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_NO_HW)
		return sl_ctl_tests[test_num].func(args);

	if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_LDEV)
		return sl_ctl_tests[test_num].func(args);

	if (lgrp_map == 0) {
		pr_err(SL_CTL_TEST_NAME "No lgrp target(s)\n");
		return -EBADRQC;
	}

	test_rtn = 0;
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {

		if (!test_bit(lgrp_num, (unsigned long *)&lgrp_map))
			continue;

		ctl_lgrp = sl_ctl_lgrp_get(ldev_num, lgrp_num);

		/*
		 * Test requires a ldev.
		 */
		args.lgrp_num = lgrp_num;
		if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_LGRP_NUM) {
			test_rtns[args.lgrp_num][0] = sl_ctl_tests[test_num].func(args);
			continue;
		}

		test_rtn = sl_ctl_lgrp_notif_callback_reg(ldev_num, lgrp_num,
			sl_ctl_test_lgrp_callback_hdlr, SL_LGRP_NOTIF_ALL, ctl_lgrp);
		if (test_rtn) {
			/* Test cannot continue without notifications */
			pr_err(SL_CTL_TEST_NAME "sl_ctl_lgrp_notif_callback_reg failed [%d]\n", test_rtn);
			goto out;
		}

		is_lgrp_callback_registered = true;

		/*
		 * Test requires a lgrp.
		 */
		if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_LGRP) {
			test_rtns[args.lgrp_num][0] = sl_ctl_tests[test_num].func(args);
			continue;
		}

		if (link_map == 0) {
			pr_err(SL_CTL_TEST_NAME "No link target(s)\n");
			return -EBADRQC;
		}

		for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num) {

			if (!test_bit(link_num, (unsigned long *)&link_map))
				continue;

			args.link_num = link_num;
			if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_LINK_NUM) {
				test_rtns[args.lgrp_num][args.link_num] = sl_ctl_tests[test_num].func(args);
				continue;
			}

			/*
			 * Test requires a link.
			 */
			if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_PREREQ_LINK)
				test_rtns[args.lgrp_num][args.link_num] = sl_ctl_tests[test_num].func(args);
		}
	}

	/* Wait on notifications, set rtn to error */
	if (sl_ctl_tests[test_num].flags & SL_CTL_TEST_FLAG_WAIT_MASK) {
		test_rtn = sl_ctl_test_wait_on_notifs(test_num, lgrp_map, link_map);
		if (test_rtn)
			pr_warn(SL_CTL_TEST_NAME "sl_ctl_test_wait_on_notifs failed [%d]\n", test_rtn);
	}

	/* Unregister callbacks if necessary */
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {

		if (!test_bit(lgrp_num, (unsigned long *)&lgrp_map))
			continue;

		if (is_lgrp_callback_registered) {
			notif_unreg = sl_ctl_lgrp_notif_callback_unreg(ldev_num, lgrp_num,
					      sl_ctl_test_lgrp_callback_hdlr, SL_LGRP_NOTIF_ALL);
			if (notif_unreg)
				pr_warn(SL_CTL_TEST_NAME "sl_lgrp_notif_callback_unreg failed [%d]\n", notif_unreg);
		}
	}

	/* If we didn't fail waiting on notifications check all test returns */
	if (!test_rtn)
		test_rtn = memcmp(test_rtns, (int[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS]) { 0 }, sizeof(test_rtns));

	pr_info(SL_CTL_TEST_NAME "Test Result (test_num %u, test_rtn %d)", test_num, test_rtn);
out:
	return test_rtn;
}
