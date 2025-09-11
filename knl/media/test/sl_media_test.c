// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include <linux/sl_media.h>
#include <linux/sl_lgrp.h>

#include "sl_asic.h"
#include "sl_lgrp.h"
#include "sl_media_test.h"
#include "sl_media_lgrp.h"
#include "sl_media_ldev.h"
#include "sl_media_jack.h"
#include "data/sl_media_data_jack.h"
#include "data/sl_media_data_lgrp.h"

#define SL_MEDIA_TEST_NAME "sl-test-media: "

struct sl_media_test_args {
	u8              lgrp_num;
	u8              ldev_num;
	u32             flags;
};

/*
 * Tests
 */

/* Print Test List */
static int sl_media_test0(struct sl_media_test_args args)
{
	pr_info(SL_MEDIA_TEST_NAME "Test List\n");
	pr_info(SL_MEDIA_TEST_NAME "Test1 - Remove Cable\n");
	pr_info(SL_MEDIA_TEST_NAME "Test2 - Insert Backplane Cable\n");
	pr_info(SL_MEDIA_TEST_NAME "Test3 - Insert PEC Cable\n");
	pr_info(SL_MEDIA_TEST_NAME "Test4 - Insert AOC Cable\n");
	pr_info(SL_MEDIA_TEST_NAME "Test5 - Create ldev and invoke jack scan\n");
	pr_info(SL_MEDIA_TEST_NAME "Test6 - Dump Jack Info\n");

	return 0;
}

/* Remove cable */
static int sl_media_test1(struct sl_media_test_args args)
{
	struct sl_media_lgrp *media_lgrp;

	media_lgrp = sl_media_data_lgrp_get(args.ldev_num, args.lgrp_num);
	if (!media_lgrp) {
		pr_err(SL_MEDIA_TEST_NAME "media test1 - media_data_lgrp_get failed\n");
		return -EFAULT;
	}

	sl_media_data_jack_fake_media_attr_clr(media_lgrp->media_jack, media_lgrp->cable_info);

	return 0;
}

/* Insert Backplane cable */
static int sl_media_test2(struct sl_media_test_args args)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_media_attr  media_attr;

	media_lgrp = sl_media_data_lgrp_get(args.ldev_num, args.lgrp_num);
	if (!media_lgrp) {
		pr_err(SL_MEDIA_TEST_NAME "media test2 - media_data_lgrp_get failed\n");
		return -EFAULT;
	}

	media_attr.vendor     = SL_MEDIA_VENDOR_HPE;
	media_attr.type       = SL_MEDIA_TYPE_BKP;
	media_attr.length_cm  = 25;
	media_attr.speeds_map = SL_MEDIA_SPEEDS_SUPPORT_BS_200G;
	media_attr.magic      = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver        = SL_MEDIA_ATTR_VER;

	rtn = sl_media_data_jack_fake_media_attr_set(media_lgrp->media_jack,
			media_lgrp->cable_info, &media_attr);
	if (rtn) {
		pr_err(SL_MEDIA_TEST_NAME "media test2 - fake media attr set failed [%d]\n", rtn);
		return rtn;
	}

	return 0;
}

/* Insert PEC cable */
static int sl_media_test3(struct sl_media_test_args args)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_media_attr  media_attr;

	media_lgrp = sl_media_data_lgrp_get(args.ldev_num, args.lgrp_num);
	if (!media_lgrp) {
		pr_err(SL_MEDIA_TEST_NAME "media test3 - media_data_lgrp_get failed\n");
		return -EFAULT;
	}

	media_attr.vendor     = SL_MEDIA_VENDOR_TE;
	media_attr.type       = SL_MEDIA_TYPE_PEC;
	media_attr.length_cm  = 150;
	media_attr.speeds_map = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
	media_attr.magic      = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver        = SL_MEDIA_ATTR_VER;

	rtn = sl_media_data_jack_fake_media_attr_set(media_lgrp->media_jack,
			media_lgrp->cable_info, &media_attr);
	if (rtn) {
		pr_err(SL_MEDIA_TEST_NAME "media test3 - fake media attr set failed [%d]\n", rtn);
		return rtn;
	}

	return 0;
}

/* Insert AOC cable */
static int sl_media_test4(struct sl_media_test_args args)
{
	int                   rtn;
	struct sl_media_lgrp *media_lgrp;
	struct sl_media_attr  media_attr;

	media_lgrp = sl_media_data_lgrp_get(args.ldev_num, args.lgrp_num);
	if (!media_lgrp) {
		pr_err(SL_MEDIA_TEST_NAME "media test4 - media_data_lgrp_get failed\n");
		return -EFAULT;
	}

	media_attr.vendor     = SL_MEDIA_VENDOR_FINISAR;
	media_attr.type       = SL_MEDIA_TYPE_AOC;
	media_attr.length_cm  = 400;
	media_attr.speeds_map = SL_MEDIA_SPEEDS_SUPPORT_CK_400G;
	media_attr.magic      = SL_MEDIA_ATTR_MAGIC;
	media_attr.ver        = SL_MEDIA_ATTR_VER;

	rtn = sl_media_data_jack_fake_media_attr_set(media_lgrp->media_jack,
			media_lgrp->cable_info, &media_attr);
	if (rtn) {
		pr_err(SL_MEDIA_TEST_NAME "media test4 - fake media attr set failed [%d]\n", rtn);
		return rtn;
	}

	return 0;
}

/* Create ldev that will invoke jack scan */
static int sl_media_test5(struct sl_media_test_args args)
{
	int                   rtn;
	struct sl_media_ldev *media_ldev;

	media_ldev = sl_media_ldev_get(args.ldev_num);
	if (!media_ldev) {
		rtn = sl_media_ldev_new(args.ldev_num);
		if (rtn) {
			pr_err(SL_MEDIA_TEST_NAME "media test5 - media_ldev_new failed\n");
			return -EFAULT;
		}
	}

	return 0;
}

/* Dump Jack Info */
static int sl_media_test6(struct sl_media_test_args args)
{
	u8                    jack_num;
	struct sl_media_jack *media_jack;

	for (jack_num = 0; jack_num < SL_MEDIA_MAX_JACK_NUM; ++jack_num) {
		media_jack = sl_media_jack_get(args.ldev_num, jack_num);
		if (media_jack->state == SL_MEDIA_JACK_CABLE_ONLINE) {
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - jack_num  = %u\n", jack_num + 1);
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - vendor    = %s\n",
					sl_media_vendor_str(media_jack->cable_info[0].media_attr.vendor));
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - type      = %s\n",
					sl_media_type_str(media_jack->cable_info[0].media_attr.type));
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - length    = %u cm\n",
					media_jack->cable_info[0].media_attr.length_cm);
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - max_speed = %s\n",
					sl_media_speed_str(media_jack->cable_info[0].media_attr.max_speed));
		} else {
			pr_info(SL_MEDIA_TEST_NAME
					"media test6 - no online cable (jack_num = %u)\n", jack_num + 1);
		}
	}

	return 0;
}

static int (*test_table[])(struct sl_media_test_args) = {
		sl_media_test0,
		sl_media_test1,
		sl_media_test2,
		sl_media_test3,
		sl_media_test4,
		sl_media_test5,
		sl_media_test6,
};

#define SL_MEDIA_TEST_NUM_TESTS ARRAY_SIZE(test_table)

int sl_test_media_test_exec(u8 ldev_num, u64 lgrp_map, int test_num, u32 flags)
{
	u8                         lgrp_num;
	struct sl_media_test_args  args;
	int                        rtn;

	if ((test_num < 0) || (test_num >= SL_MEDIA_TEST_NUM_TESTS)) {
		pr_err(SL_MEDIA_TEST_NAME "media test_exec - Invalid test num\n");
		return -EBADRQC;
	}

	if (lgrp_map == 0) {
		pr_err(SL_MEDIA_TEST_NAME "media test_exec - Invalid lgrp_map\n");
		return -EBADRQC;
	}

	memset(&args, 0, sizeof(args));
	args.flags = flags;
	for (lgrp_num = 0; lgrp_num < SL_ASIC_MAX_LGRPS; ++lgrp_num) {
		if (!test_bit(lgrp_num, (unsigned long *)&lgrp_map))
			continue;
		args.lgrp_num = lgrp_num;
		args.ldev_num = ldev_num;
		rtn = test_table[test_num](args);
		if (rtn) {
			pr_err(SL_MEDIA_TEST_NAME "media test_exec - Test %d failed\n", test_num);
			return rtn;
		}
	}

	return 0;
}

const char *sl_media_test_type_str(u32 type)
{
	return sl_media_type_str(type);
}

int sl_media_test_type_from_str(const char *str, u32 *type)
{
	if (!str || !type)
		return -EINVAL;

	if (!strncmp(str, "headshell", 9)) {
		*type = SL_MEDIA_TYPE_HEADSHELL;
		return 0;
	}

	if (!strncmp(str, "backplane", 9)) {
		*type = SL_MEDIA_TYPE_BACKPLANE;
		return 0;
	}

	if (!strncmp(str, "electrical", 10)) {
		*type = SL_MEDIA_TYPE_ELECTRICAL;
		return 0;
	}

	if (!strncmp(str, "optical", 7)) {
		*type = SL_MEDIA_TYPE_OPTICAL;
		return 0;
	}

	if (!strncmp(str, "passive", 7)) {
		*type = SL_MEDIA_TYPE_PASSIVE;
		return 0;
	}

	if (!strncmp(str, "active", 6)) {
		*type = SL_MEDIA_TYPE_ACTIVE;
		return 0;
	}

	if (!strncmp(str, "analog", 6)) {
		*type = SL_MEDIA_TYPE_ANALOG;
		return 0;
	}

	if (!strncmp(str, "digital", 7)) {
		*type = SL_MEDIA_TYPE_DIGITAL;
		return 0;
	}

	if (!strncmp(str, "AOC", 3)) {
		*type = SL_MEDIA_TYPE_AOC;
		return 0;
	}

	if (!strncmp(str, "PEC", 3)) {
		*type = SL_MEDIA_TYPE_PEC;
		return 0;
	}

	if (!strncmp(str, "AEC", 3)) {
		*type = SL_MEDIA_TYPE_AEC;
		return 0;
	}

	if (!strncmp(str, "BKP", 3)) {
		*type = SL_MEDIA_TYPE_BKP;
		return 0;
	}

	pr_err(SL_MEDIA_TEST_NAME "no match for %s\n", str);

	return -ENOENT;
}
