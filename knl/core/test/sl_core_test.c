// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/err.h>

#include "sl_kconfig.h"
#include "sl_asic.h"

#include "base/sl_core_log.h"

#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "sl_core_str.h"
#include <linux/sl_media.h>
#include <linux/sl_test.h>

#include "sl_ctl_ldev.h"
#include "sl_ctl_lgrp.h"
#include "sl_ctl_link.h"
#include "sl_ctl_link_priv.h"
#include "sl_ctl_llr.h"

#include "test/sl_core_test.h"
#include "test/sl_core_test_fec.h"

#define SL_CORE_TEST_NAME "sl-test-core: "

struct sl_core_test_tag_data {
	u8                   ldev_num;
	u8                   lgrp_num;
	u8                   link_num;
};
static struct sl_core_test_tag_data sl_core_test_tag_data[SL_ASIC_MAX_LGRPS];

struct sl_core_test_args {
	u8                    ldev_num;
	u8                    lgrp_num;
	u8                    link_num;
	u32                   flags;
	void                 *tag;
};

#define SL_CORE_TEST_FUNC(_num) \
	static int sl_core_test##_num(struct sl_core_test_args test_args)
#define SL_CORE_TEST_FUNC_RESERVED(_num) \
	static int sl_core_test##_num(struct sl_core_test_args test_args) \
	{                                                                       \
		return -1;                                                      \
	}

SL_CORE_TEST_FUNC(0);

SL_CORE_TEST_FUNC_RESERVED(1);

SL_CORE_TEST_FUNC(2);
SL_CORE_TEST_FUNC(3);
SL_CORE_TEST_FUNC(4);
SL_CORE_TEST_FUNC(5);
SL_CORE_TEST_FUNC_RESERVED(6);
SL_CORE_TEST_FUNC_RESERVED(7);
SL_CORE_TEST_FUNC(8);
SL_CORE_TEST_FUNC(9);

SL_CORE_TEST_FUNC(10);
SL_CORE_TEST_FUNC(11);
SL_CORE_TEST_FUNC(12);
SL_CORE_TEST_FUNC(13);
SL_CORE_TEST_FUNC(14);
SL_CORE_TEST_FUNC(15);
SL_CORE_TEST_FUNC(16);
SL_CORE_TEST_FUNC(17);
SL_CORE_TEST_FUNC(18);
SL_CORE_TEST_FUNC(19);

SL_CORE_TEST_FUNC(20);
SL_CORE_TEST_FUNC(21);
SL_CORE_TEST_FUNC(22);
SL_CORE_TEST_FUNC(23);
SL_CORE_TEST_FUNC(24);
SL_CORE_TEST_FUNC(25);
SL_CORE_TEST_FUNC(26);
SL_CORE_TEST_FUNC(27);
SL_CORE_TEST_FUNC(28);
SL_CORE_TEST_FUNC(29);

SL_CORE_TEST_FUNC(30);
SL_CORE_TEST_FUNC(31);
SL_CORE_TEST_FUNC(32);
SL_CORE_TEST_FUNC(33);
SL_CORE_TEST_FUNC(34);
SL_CORE_TEST_FUNC(35);
SL_CORE_TEST_FUNC(36);
SL_CORE_TEST_FUNC(37);
SL_CORE_TEST_FUNC(38);
SL_CORE_TEST_FUNC(39);

SL_CORE_TEST_FUNC(40);
SL_CORE_TEST_FUNC(41);
SL_CORE_TEST_FUNC(42);
SL_CORE_TEST_FUNC(43);

#define SL_CORE_TEST_RESERVED        0xFFFFFFFF /* reserved test                */
#define SL_CORE_TEST_FLAG_SPECIAL    BIT(0)     /* not an API test              */
#define SL_CORE_TEST_FLAG_NO_LGRP    BIT(1)     /* API call doesn't take a lgrp */
#define SL_CORE_TEST_FLAG_NO_LINK    BIT(2)     /* API call doesn't take a link */
#define SL_CORE_TEST_FLAG_CAN_ASYNC  BIT(3)     /* API call can be blocking     */
#define SL_CORE_TEST_FLAG_ABORT      BIT(4)     /* Call ABORT                   */
#define SL_CORE_TEST_FLAG_UP_CHECK   BIT(5)     /* Check for UP first           */

struct sl_core_test {
	const char desc[32];
	int  (*test)(struct sl_core_test_args test_args);
	const u32  flags;
};

#define SL_CORE_TEST(_num, _desc, _flags)    \
	[_num] = {                           \
		.desc  = _desc,              \
		.test  = sl_core_test##_num, \
		.flags = _flags,             \
	}

static const struct sl_core_test sl_core_tests[] = {
	SL_CORE_TEST(0,  "list of tests",            SL_CORE_TEST_FLAG_SPECIAL),

	SL_CORE_TEST(1,  "reserved",                 SL_CORE_TEST_RESERVED),

	SL_CORE_TEST(2,  "lgrp NEW",                 SL_CORE_TEST_FLAG_NO_LGRP),
	SL_CORE_TEST(3,  "lgrp CONNECT ID set",      SL_CORE_TEST_FLAG_NO_LINK),
	SL_CORE_TEST(4,  "lgrp DEL",                 SL_CORE_TEST_FLAG_NO_LINK),
	SL_CORE_TEST(5,  "lgrp config SET",          SL_CORE_TEST_FLAG_NO_LINK),
	SL_CORE_TEST(6,  "reserved",                 SL_CORE_TEST_RESERVED),
	SL_CORE_TEST(7,  "reserved",                 SL_CORE_TEST_RESERVED),

	SL_CORE_TEST(8,  "link NEW",                 0),
	SL_CORE_TEST(9,  "link DEL",                 0),

	SL_CORE_TEST(10, "link state GET",           0),
	SL_CORE_TEST(11, "link info GET",            0),
	SL_CORE_TEST(12, "link UP",                  (SL_CORE_TEST_FLAG_CAN_ASYNC |
							SL_CORE_TEST_FLAG_UP_CHECK)),
	SL_CORE_TEST(13, "link DOWN",                0),

	SL_CORE_TEST(14, "link AN caps GET",         SL_CORE_TEST_FLAG_CAN_ASYNC),
	SL_CORE_TEST(15, "link 200G CONFIG - AN",    0),
	SL_CORE_TEST(16, "link 400G CONFIG - AN",    0),
	SL_CORE_TEST(17, "link 200G CONFIG - no AN", 0),
	SL_CORE_TEST(18, "link 400G CONFIG - no AN", 0),

	SL_CORE_TEST(19, "MAC new",                  0),
	SL_CORE_TEST(20, "MAC TX state GET",         0),
	SL_CORE_TEST(21, "MAC TX START",             0),
	SL_CORE_TEST(22, "MAC TX STOP",              0),
	SL_CORE_TEST(23, "MAC RX state GET",         0),
	SL_CORE_TEST(24, "MAC RX START",             0),
	SL_CORE_TEST(25, "MAC RX STOP",              0),
	SL_CORE_TEST(26, "MAC del",                  0),

	SL_CORE_TEST(27, "LLR new",                  0),
	SL_CORE_TEST(28, "LLR del",                  0),
	SL_CORE_TEST(29, "LLR config",               0),
	SL_CORE_TEST(30, "LLR state GET",            0),
	SL_CORE_TEST(31, "LLR data GET",             0),
	SL_CORE_TEST(32, "LLR SETUP",                SL_CORE_TEST_FLAG_CAN_ASYNC),
	SL_CORE_TEST(33, "LLR START",                SL_CORE_TEST_FLAG_CAN_ASYNC),
	SL_CORE_TEST(34, "LLR STOP",                 0),

	SL_CORE_TEST(35, "CTL link new",             0),
	SL_CORE_TEST(36, "CTL link del",             0),
	SL_CORE_TEST(37, "CTL LLR config SET",       0),
	SL_CORE_TEST(38, "CTL LLR start",            0),
	SL_CORE_TEST(39, "CTL LLR stop",             0),

	SL_CORE_TEST(40, "FEC cntr GET",             0),
	SL_CORE_TEST(41, "FEC cntr set rate",        0),

	SL_CORE_TEST(42, "test info_map bit off",    0),
	SL_CORE_TEST(43, "test info_map bit on",     0),

};
#define SL_CORE_NUM_TESTS ARRAY_SIZE(sl_core_tests)

#define SL_CORE_TEST_WAIT_COUNT   60
#define SL_CORE_TEST_NOT_NOTIFIED -1
#define SL_CORE_TEST_WAIT_MS      500

/*
 *  test callback
 */
struct sl_core_test_callback_data {
	bool                       received;
	u64                        info_map;
	u32                        state;
	u32                        cause;
	u32                        speed;
	u32                        fec_mode;
	u32                        fec_type;
	struct sl_link_caps        caps;
	u32                        llr_state;
	struct sl_llr_data         llr_data;
};
// FIXME: need to add ldevs
static struct sl_core_test_callback_data
	sl_core_test_callback_status[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
static int sl_core_test_link_up_callback(void *tag, u32 state, u32 cause, u64 info_map,
					 u32 speed, u32 fec_mode, u32 fec_type)
{
	struct sl_core_test_tag_data test_tag;
	char                         info_map_str[1024];

	test_tag = *(struct sl_core_test_tag_data *)tag;

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	pr_info(SL_CORE_TEST_NAME
		"link up callback (lgrp_num = %u, link_num = %u, state = %s, cause = %s, info_map = %s)\n",
		test_tag.lgrp_num, test_tag.link_num,
		sl_core_link_state_str(state), sl_link_down_cause_str(cause), info_map_str);

	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].received   = true;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].info_map   = info_map;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].state      = state;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].cause      = cause;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].speed      = speed;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].fec_mode   = fec_mode;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].fec_type   = fec_type;

	return 0;
}
static int sl_core_test_link_down_callback(void *tag)
{
	struct sl_core_test_tag_data test_tag;

	test_tag = *(struct sl_core_test_tag_data *)tag;

	pr_info(SL_CORE_TEST_NAME
		"link down callback (lgrp_num = %u, link_num = %u)\n",
		test_tag.lgrp_num, test_tag.link_num);

// FIXME

	return 0;
}
static int sl_core_test_fault_callback(void *tag, u32 state, u32 cause, u64 info_map)
{
	struct sl_core_test_tag_data test_tag;
	char                         info_map_str[1024];

	test_tag = *(struct sl_core_test_tag_data *)tag;

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	pr_info(SL_CORE_TEST_NAME
		"fault callback (lgrp_num = %u, link_num = %u, state = %s, cause = %s, info_map = %s)\n",
		test_tag.lgrp_num, test_tag.link_num,
		sl_core_link_state_str(state), sl_link_down_cause_str(cause), info_map_str);

	return 0;
}
static int sl_core_test_an_callback(void *tag, struct sl_link_caps *caps, u32 result)
{
	struct sl_core_test_tag_data test_tag;

	test_tag = *(struct sl_core_test_tag_data *)tag;

	pr_info(SL_CORE_TEST_NAME
		"an callback (lgrp_num = %u, link_num = %u, caps.tech_map = 0x%08X)\n",
		test_tag.lgrp_num, test_tag.link_num, caps->tech_map);

	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].received = true;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].caps     = *caps;

	sl_core_link_an_lp_caps_free(test_tag.ldev_num, test_tag.lgrp_num, test_tag.link_num, caps);

	return 0;
}
static int sl_core_test_llr_setup_callback(void *tag, u32 llr_state,
	u64 info_map, struct sl_llr_data *llr_data)
{
	struct sl_core_test_tag_data test_tag;
	char                         info_map_str[1024];

	test_tag = *(struct sl_core_test_tag_data *)tag;

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	pr_info(SL_CORE_TEST_NAME
		"llr setup callback (lgrp_num = %u, link_num = %u, llr_state = %u, loop_time = %lldns, info_map = %s)\n",
		test_tag.lgrp_num, test_tag.link_num, llr_state,
		llr_data->loop.average, info_map_str);

	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].received  = true;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].info_map  = info_map;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].llr_state = llr_state;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].llr_data  = *llr_data;

	sl_core_llr_data_free(test_tag.ldev_num, test_tag.lgrp_num, test_tag.link_num, llr_data);

	return 0;
}
static int sl_core_test_llr_start_callback(void *tag, u32 llr_state, u64 info_map)
{
	struct sl_core_test_tag_data test_tag;
	char                         info_map_str[1024];

	test_tag = *(struct sl_core_test_tag_data *)tag;

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	pr_info(SL_CORE_TEST_NAME
		"llr start callback (lgrp_num = %u, link_num = %u, llr_state = %s, info_map = %s)\n",
		test_tag.lgrp_num, test_tag.link_num,
		sl_core_llr_state_str(llr_state), info_map_str);

	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].received  = true;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].info_map  = info_map;
	sl_core_test_callback_status[test_tag.lgrp_num][test_tag.link_num].llr_state = llr_state;

	return 0;
}

static int sl_core_test0(struct sl_core_test_args test_args)
{
	int x;

	pr_info(SL_CORE_TEST_NAME "Available Tests: (%ld)\n", (SL_CORE_NUM_TESTS - 1));
	for (x = 1; x < SL_CORE_NUM_TESTS; ++x) {
		if (sl_core_tests[x].flags == SL_CORE_TEST_RESERVED)
			continue;
		pr_info(SL_CORE_TEST_NAME " %2d = %s\n", x, sl_core_tests[x].desc);
	}

	return 0;
}

static int sl_core_test2(struct sl_core_test_args test_args)
{
	return sl_core_lgrp_new(test_args.ldev_num, test_args.lgrp_num);
}

static int sl_core_test3(struct sl_core_test_args test_args)
{
	char connect_id[SL_LOG_CONNECT_ID_LEN + 1];

	snprintf(connect_id, sizeof(connect_id), "test-lgrp%2u", test_args.lgrp_num);

	sl_core_lgrp_connect_id_set(test_args.ldev_num, test_args.lgrp_num, connect_id);

	return 0;
}

static int sl_core_test4(struct sl_core_test_args test_args)
{
	sl_core_lgrp_del(test_args.ldev_num, test_args.lgrp_num);

	return 0;
}

static int sl_core_test5(struct sl_core_test_args test_args)
{
	struct sl_lgrp_config lgrp_config;

	memset(&lgrp_config, 0, sizeof(struct sl_lgrp_config));

	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.furcation = SL_MEDIA_FURCATION_X1;

	return sl_core_lgrp_config_set(test_args.ldev_num, test_args.lgrp_num, &lgrp_config);
}

static int sl_core_test8(struct sl_core_test_args test_args)
{
	return sl_core_link_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test9(struct sl_core_test_args test_args)
{
	sl_core_link_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

static int sl_core_test10(struct sl_core_test_args test_args)
{
	int rtn;
	u32 link_state;

	rtn = sl_core_link_state_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &link_state);
	if (rtn != 0)
		return rtn;

	pr_info(SL_CORE_TEST_NAME "link (lgrp_num = %u, link_num = %u, state = %s)\n",
		test_args.lgrp_num, test_args.link_num,
		sl_core_link_state_str(link_state));

	return 0;
}

static int sl_core_test11(struct sl_core_test_args test_args)
{
	int  rtn;
	u64  info_map;
	char info_map_str[1024];

	rtn = sl_core_info_map_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &info_map);
	if (rtn != 0)
		return rtn;

	sl_core_info_map_str(info_map, info_map_str, sizeof(info_map_str));

	pr_info(SL_CORE_TEST_NAME "link (lgrp_num = %u, link_num = %u, info = %s)\n",
		test_args.lgrp_num, test_args.link_num,
		info_map_str);

	return 0;
}

static int sl_core_test12(struct sl_core_test_args test_args)
{
	return sl_core_link_up(test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_test_link_up_callback, test_args.tag);
}

static int sl_core_test13(struct sl_core_test_args test_args)
{
	return sl_core_link_down(test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_test_link_down_callback, test_args.tag, SL_LINK_DOWN_CAUSE_COMMAND);
}

static int sl_core_test14(struct sl_core_test_args test_args)
{
// FIXME: need good caps here
	return sl_core_link_an_lp_caps_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_test_an_callback, test_args.tag, NULL, 5000, test_args.flags);
}

static int sl_core_test15(struct sl_core_test_args test_args)
{
	int                        rtn;
	struct sl_core_link_config link_config;
	struct sl_lgrp_config      lgrp_config;

	memset(&link_config, 0, sizeof(struct sl_core_link_config));
	link_config.magic              = SL_CORE_LINK_CONFIG_MAGIC;
	link_config.link_up_timeout_ms = 18000;
	link_config.fault_callback     = sl_core_test_fault_callback;
	link_config.flags              = SL_LINK_CONFIG_OPT_AUTONEG_ENABLE;

	rtn = sl_core_link_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &link_config);
	if (rtn != 0)
		return rtn;

	memset(&lgrp_config, 0, sizeof(struct sl_lgrp_config));
	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.ver       = SL_LGRP_CONFIG_VER;
	lgrp_config.furcation = SL_MEDIA_FURCATION_X1;
	lgrp_config.tech_map  = SL_LGRP_CONFIG_TECH_BS_200G;
	lgrp_config.fec_map   = SL_LGRP_CONFIG_FEC_RS;

	rtn = sl_core_lgrp_config_set(test_args.ldev_num, test_args.lgrp_num, &lgrp_config);
	if (rtn != 0)
		return rtn;

	return 0;
}

static int sl_core_test16(struct sl_core_test_args test_args)
{
	int                        rtn;
	struct sl_core_link_config link_config;
	struct sl_lgrp_config      lgrp_config;

	memset(&link_config, 0, sizeof(struct sl_core_link_config));
	link_config.magic              = SL_CORE_LINK_CONFIG_MAGIC;
	link_config.link_up_timeout_ms = 18000;
	link_config.fault_callback     = sl_core_test_fault_callback;
	link_config.flags              = SL_LINK_CONFIG_OPT_AUTONEG_ENABLE;

	rtn = sl_core_link_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &link_config);
	if (rtn != 0)
		return rtn;

	memset(&lgrp_config, 0, sizeof(struct sl_lgrp_config));
	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.ver       = SL_LGRP_CONFIG_VER;
	lgrp_config.furcation = SL_MEDIA_FURCATION_X1;
	lgrp_config.tech_map  = SL_LGRP_CONFIG_TECH_CK_400G;
	lgrp_config.fec_map   = SL_LGRP_CONFIG_FEC_RS;

	rtn = sl_core_lgrp_config_set(test_args.ldev_num, test_args.lgrp_num, &lgrp_config);
	if (rtn != 0)
		return rtn;

	return 0;
}

static int sl_core_test17(struct sl_core_test_args test_args)
{
	int                        rtn;
	struct sl_core_link_config link_config;
	struct sl_lgrp_config      lgrp_config;

	memset(&link_config, 0, sizeof(struct sl_core_link_config));
	link_config.magic              = SL_CORE_LINK_CONFIG_MAGIC;
	link_config.link_up_timeout_ms = 18000;
	link_config.fault_callback     = sl_core_test_fault_callback;

	rtn = sl_core_link_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &link_config);
	if (rtn != 0)
		return rtn;

	memset(&lgrp_config, 0, sizeof(struct sl_lgrp_config));
	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.ver       = SL_LGRP_CONFIG_VER;
	lgrp_config.furcation = SL_MEDIA_FURCATION_X1;
	lgrp_config.tech_map  = SL_LGRP_CONFIG_TECH_BS_200G;
	lgrp_config.fec_map   = SL_LGRP_CONFIG_FEC_RS;

	rtn = sl_core_lgrp_config_set(test_args.ldev_num, test_args.lgrp_num, &lgrp_config);
	if (rtn != 0)
		return rtn;

	return 0;
}

static int sl_core_test18(struct sl_core_test_args test_args)
{
	int                        rtn;
	struct sl_core_link_config link_config;
	struct sl_lgrp_config      lgrp_config;

	memset(&link_config, 0, sizeof(struct sl_core_link_config));
	link_config.magic              = SL_CORE_LINK_CONFIG_MAGIC;
	link_config.link_up_timeout_ms = 18000;
	link_config.fault_callback     = sl_core_test_fault_callback;

	rtn = sl_core_link_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &link_config);
	if (rtn != 0)
		return rtn;

	memset(&lgrp_config, 0, sizeof(struct sl_lgrp_config));
	lgrp_config.magic     = SL_LGRP_CONFIG_MAGIC;
	lgrp_config.ver       = SL_LGRP_CONFIG_VER;
	lgrp_config.furcation = SL_MEDIA_FURCATION_X1;
	lgrp_config.tech_map  = SL_LGRP_CONFIG_TECH_CK_400G;
	lgrp_config.fec_map   = SL_LGRP_CONFIG_FEC_RS;

	rtn = sl_core_lgrp_config_set(test_args.ldev_num, test_args.lgrp_num, &lgrp_config);
	if (rtn != 0)
		return rtn;

	return 0;
}

static int sl_core_test19(struct sl_core_test_args test_args)
{
	return sl_core_mac_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test20(struct sl_core_test_args test_args)
{
	int rtn;
	u32 state;

	rtn = sl_core_mac_tx_state_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &state);
	if (rtn != 0)
		return rtn;

	pr_info(SL_CORE_TEST_NAME "MAC TX (lgrp_num = %u, link_num = %u, state = %s)\n",
		test_args.lgrp_num, test_args.link_num, sl_core_mac_state_str(state));

	return 0;
}

static int sl_core_test21(struct sl_core_test_args test_args)
{
	return sl_core_mac_tx_start(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test22(struct sl_core_test_args test_args)
{
	return sl_core_mac_tx_stop(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test23(struct sl_core_test_args test_args)
{
	int rtn;
	u32 state;

	rtn = sl_core_mac_rx_state_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &state);
	if (rtn != 0)
		return rtn;

	pr_info(SL_CORE_TEST_NAME "MAC RX (lgrp_num = %u, link_num = %u, state = %s)\n",
		test_args.lgrp_num, test_args.link_num, sl_core_mac_state_str(state));

	return 0;
}

static int sl_core_test24(struct sl_core_test_args test_args)
{
	return sl_core_mac_rx_start(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test25(struct sl_core_test_args test_args)
{
	return sl_core_mac_rx_stop(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test26(struct sl_core_test_args test_args)
{
	sl_core_mac_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

static int sl_core_test27(struct sl_core_test_args test_args)
{
	return sl_core_llr_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test28(struct sl_core_test_args test_args)
{
	sl_core_llr_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

static int sl_core_test29(struct sl_core_test_args test_args)
{
	struct sl_llr_config llr_config;

	llr_config.magic            = SL_LLR_CONFIG_MAGIC;
	llr_config.ver              = SL_LLR_CONFIG_VER;
	llr_config.mode             = 0;
	llr_config.setup_timeout_ms = 7000;
	llr_config.start_timeout_ms = 5000;
	llr_config.link_dn_behavior = 0;

	return sl_core_llr_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &llr_config);
}

static int sl_core_test30(struct sl_core_test_args test_args)
{
	int rtn;
	u32 llr_state;

	rtn = sl_core_llr_state_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &llr_state);
	if (rtn != 0)
		return rtn;

	pr_info(SL_CORE_TEST_NAME "LLR (ldev_num = %u, lgrp_num = %u, link_num = %u, state = %s)\n",
		test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_llr_state_str(llr_state));

	return 0;
}

static int sl_core_test31(struct sl_core_test_args test_args)
{
	int                rtn;
	struct sl_llr_data llr_data;

	rtn = sl_core_llr_data_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &llr_data);
	if (rtn != 0)
		return rtn;

	pr_info(SL_CORE_TEST_NAME
		"LLR data (lgrp_num = %u, link_num = %u, min = %lldns, max = %lldns, average = %lldns, calc = %lldns)\n",
		test_args.lgrp_num, test_args.link_num,
		llr_data.loop.min,
		llr_data.loop.max,
		llr_data.loop.average,
		llr_data.loop.calculated);

	return 0;
}

static int sl_core_test32(struct sl_core_test_args test_args)
{
	return sl_core_llr_setup(test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_test_llr_setup_callback, test_args.tag, test_args.flags);
}

static int sl_core_test33(struct sl_core_test_args test_args)
{
	return sl_core_llr_start(test_args.ldev_num, test_args.lgrp_num, test_args.link_num,
		sl_core_test_llr_start_callback, test_args.tag, test_args.flags);
}

static int sl_core_test34(struct sl_core_test_args test_args)
{
	return sl_core_llr_stop(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, 0);
}

static int sl_core_test35(struct sl_core_test_args test_args)
{
	return sl_ctl_link_new(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, NULL);
}

static int sl_core_test36(struct sl_core_test_args test_args)
{
	sl_ctl_link_del(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);

	return 0;
}

static int sl_core_test37(struct sl_core_test_args test_args)
{
	struct sl_llr_config llr_config;

	llr_config.magic            = SL_LLR_CONFIG_MAGIC;
	llr_config.ver              = SL_LLR_CONFIG_VER;
	llr_config.mode             = 0;
	llr_config.setup_timeout_ms = 7000;
	llr_config.start_timeout_ms = 5000;
	llr_config.link_dn_behavior = 0;

	return sl_ctl_llr_config_set(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &llr_config);
}

static int sl_core_test38(struct sl_core_test_args test_args)
{
	return sl_ctl_llr_start(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test39(struct sl_core_test_args test_args)
{
	return sl_ctl_llr_stop(test_args.ldev_num, test_args.lgrp_num, test_args.link_num);
}

static int sl_core_test40(struct sl_core_test_args test_args)
{
	int rtn;
	struct sl_core_link_fec_cw_cntrs cw_cntrs = { 0 };

	rtn = sl_core_link_fec_cw_cntrs_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &cw_cntrs);

	pr_info("ucw = %llu, ccw = %llu, gcw = %llu", cw_cntrs.ucw, cw_cntrs.ccw, cw_cntrs.gcw);

	return rtn;
}

static int sl_core_test41(struct sl_core_test_args test_args)
{
	struct sl_core_link_fec_cw_cntrs rates[] = {
		{
			.ucw = 60,
			.ccw = 0,
			.gcw = 39062500,
		},
		{
			.ucw = 0,
			.ccw = 20000000,
			.gcw = 21062500,
		},
		{
			.ucw = 0,
			.ccw = 0,
			.gcw = 0,

		},

	};

	if (test_args.flags > ARRAY_SIZE(rates))
		return -EINVAL;

	return sl_core_test_fec_cw_cntrs_set(test_args.ldev_num, test_args.lgrp_num,
		test_args.link_num, &rates[test_args.flags]);
}

/* test info_map bit off */
static int sl_core_test42(struct sl_core_test_args test_args)
{
	int  rtn;
	u64  info_map;

	rtn = sl_core_info_map_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &info_map);
	if (rtn)
		return rtn;

	/* Return 0 if bit is off */
	return test_bit(test_args.flags, (unsigned long *)&info_map);
}

/* test info_map bit on */
static int sl_core_test43(struct sl_core_test_args test_args)
{
	int  rtn;
	u64  info_map;

	rtn = sl_core_info_map_get(test_args.ldev_num, test_args.lgrp_num, test_args.link_num, &info_map);
	if (rtn)
		return rtn;

	/* Return 0 if bit on */
	return !test_bit(test_args.flags, (unsigned long *)&info_map);
}

/*
 *  test launch
 */
static int sl_core_test_launch_no_lgrp(int test_num, u8 ldev_num, u8 lgrp_num, u8 link_num, u32 flags)
{
	struct sl_core_test_args test_args;

	pr_debug(SL_CORE_TEST_NAME "start (ldev_num = %d, lgrp_num = %u, link_num = %u)\n",
		ldev_num, lgrp_num, link_num);

	test_args.ldev_num = ldev_num;
	test_args.lgrp_num = lgrp_num;
	test_args.link_num = link_num;
	test_args.tag      = NULL;
	test_args.flags    = flags;

	return sl_core_tests[test_num].test(test_args);
}
static int sl_core_test_launch(int test_num, u8 ldev_num, u8 lgrp_num, u8 link_num, u32 flags)
{
	struct sl_core_test_args test_args;

	sl_core_test_tag_data[lgrp_num].ldev_num = ldev_num;
	sl_core_test_tag_data[lgrp_num].lgrp_num = lgrp_num;
	sl_core_test_tag_data[lgrp_num].link_num = link_num;

	pr_debug(SL_CORE_TEST_NAME "start (ldev_num = %d, lgrp_num = %u, link_num = %u)\n",
		ldev_num, lgrp_num, link_num);

	test_args.ldev_num = ldev_num;
	test_args.lgrp_num = lgrp_num;
	test_args.link_num = link_num;
	test_args.tag      = (void *)&(sl_core_test_tag_data[lgrp_num]);
	test_args.flags    = flags;

	return sl_core_tests[test_num].test(test_args);
}

static DEFINE_MUTEX(sl_core_test_in_progress);
static int sl_core_test_launch_rtn[SL_ASIC_MAX_LGRPS][SL_ASIC_MAX_LINKS];
int sl_core_test_exec(u8 ldev_num, u64 lgrp_map,
	unsigned long link_map, int test_num, u32 flags)
{
	int                       rtn;
	u32                       x;
	u8                        y;
	struct sl_core_test_args  test_args;
	struct sl_core_ldev      *ldev; // FIXME: for now just use existing
	int                       wait_count;

	ldev = sl_core_ldev_get(ldev_num);

	if (mutex_trylock(&sl_core_test_in_progress) == 0) {
		pr_err(SL_CORE_TEST_NAME "testing in progress\n");
		return -EBADRQC;
	}

	if ((test_num < 0) || (test_num >= SL_CORE_NUM_TESTS)) {
		pr_err(SL_CORE_TEST_NAME "invalid (test_num = %d)\n", test_num);
		rtn = -EBADRQC;
		goto out;
	}

	/* init */
	for (x = 0; x < SL_ASIC_MAX_LGRPS; ++x) {
		for (y = 0; y < SL_ASIC_MAX_LINKS; ++y) {
			sl_core_test_callback_status[x][y].received = false;
			sl_core_test_launch_rtn[x][y] = -1;
		}
	}

	pr_info(SL_CORE_TEST_NAME
		"\"%s\" (lgrp_map = 0x%016llX, link_map = 0x%08lX, flags = 0x%X)\n",
		sl_core_tests[test_num].desc, lgrp_map, link_map, flags);

	/* special test */
	if (is_flag_set(sl_core_tests[test_num].flags, SL_CORE_TEST_FLAG_SPECIAL)) {
		pr_debug(SL_CORE_TEST_NAME "special test\n");
		rtn = sl_core_tests[test_num].test(test_args);
		goto out;
	}

	if ((lgrp_map == 0) && (link_map == 0)) {
		pr_err(SL_CORE_TEST_NAME "no target(s)\n");
		rtn = -EBADRQC;
		goto out;
	}

	/* test per link grp */
	for (x = 0; x < SL_ASIC_MAX_LGRPS; ++x) {
		if (!test_bit(x, (unsigned long *)&lgrp_map))
			continue;

		if (sl_core_tests[test_num].flags == SL_CORE_TEST_RESERVED)
			continue;

		/* lgrp not required */
		if (is_flag_set(sl_core_tests[test_num].flags, SL_CORE_TEST_FLAG_NO_LGRP)) {
			sl_core_test_launch_rtn[x][0] = sl_core_test_launch_no_lgrp(test_num, ldev->num, x, 0, flags);
			if (sl_core_test_launch_rtn[x][0] != 0)
				pr_err(SL_CORE_TEST_NAME "ldev test %d failed [%d]\n",
					test_num, sl_core_test_launch_rtn[x][0]);
			continue;
		}

		/* link not required */
		if (is_flag_set(sl_core_tests[test_num].flags, SL_CORE_TEST_FLAG_NO_LINK)) {
			sl_core_test_launch_rtn[x][0] = sl_core_test_launch(test_num, ldev->num, x, 0, flags);
			if (sl_core_test_launch_rtn[x][0] != 0)
				pr_err(SL_CORE_TEST_NAME "lgrp test %d failed [%d]\n",
					test_num, sl_core_test_launch_rtn[x][0]);
			continue;
		}

		/* test per link */
		if (link_map == 0) {
			pr_err(SL_CORE_TEST_NAME "no links in link map\n");
			continue;
		}
		for (y = 0; y < SL_ASIC_MAX_LINKS; ++y) {
			if (!test_bit(y, &link_map))
				continue;

			sl_core_test_launch_rtn[x][y] = sl_core_test_launch(test_num, ldev->num, x, y, flags);
			if (sl_core_test_launch_rtn[x][y] != 0)
				pr_err(SL_CORE_TEST_NAME "link test %d failed [%d]\n",
					test_num, sl_core_test_launch_rtn[x][y]);
		}
	}

	/* if not a thread capable test then done */
	if (!is_flag_set(sl_core_tests[test_num].flags, SL_CORE_TEST_FLAG_CAN_ASYNC)) {
		pr_debug(SL_CORE_TEST_NAME "test doesn't use a thread\n");
		goto result_check;
	}

	/* wait for callback */
	for (x = 0; x < SL_ASIC_MAX_LGRPS; ++x) {
		if (!test_bit(x, (unsigned long *)&lgrp_map))
			continue;

		for (y = 0; y < SL_ASIC_MAX_LINKS; ++y) {
			if (!test_bit(y, &link_map))
				continue;

			if (sl_core_test_launch_rtn[x][y] != 0)
				continue;

			if (is_flag_set(sl_core_tests[test_num].flags, SL_CORE_TEST_FLAG_UP_CHECK)) {
				u32 link_state;

				sl_core_link_state_get(ldev->num, x, y, &link_state);
				if (link_state == SL_CORE_LINK_STATE_UP)
					continue;
			}

			pr_debug(SL_CORE_TEST_NAME
				"wait (lgrp_num = %u, link_num = %u)\n", x, y);

			wait_count = 0;
			while (wait_count++ < SL_CORE_TEST_WAIT_COUNT) {
				if (sl_core_test_callback_status[x][y].received)
					break;
				msleep(SL_CORE_TEST_WAIT_MS);
				if ((wait_count == 2) &&
					(is_flag_set(sl_core_tests[test_num].flags,
					SL_CORE_TEST_FLAG_ABORT))) {
					(void)sl_core_test_launch(13, ldev->num, x, y, 0);
				}
			}
			if (!sl_core_test_callback_status[x][y].received)
				goto out_timeout;
		}
	}

result_check:

	for (x = 0; x < SL_ASIC_MAX_LGRPS; ++x) {
		if (!test_bit(x, (unsigned long *)&lgrp_map))
			continue;

		for (y = 0; y < SL_ASIC_MAX_LINKS; ++y) {
			if (!test_bit(y, &link_map))
				continue;

			if (sl_core_test_launch_rtn[x][y] != 0) {
				rtn = sl_core_test_launch_rtn[x][y];
				goto out;
			}
		}
	}

	rtn = 0;

out:
	mutex_unlock(&sl_core_test_in_progress);

	return rtn;

out_timeout:

	pr_err(SL_CORE_TEST_NAME "timeout (lgrp_num = %u, link_num = %u)\n", x, y);

	for (x = 0; x < SL_ASIC_MAX_LGRPS; ++x) {
		if (!test_bit(x, (unsigned long *)&lgrp_map))
			continue;

		for (y = 0; y < SL_ASIC_MAX_LINKS; ++y) {
			if (!test_bit(y, &link_map))
				continue;
// FIXME: down?
		}
	}

	mutex_unlock(&sl_core_test_in_progress);

	return -ETIME;
}
