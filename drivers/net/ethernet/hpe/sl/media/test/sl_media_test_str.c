// SPDX-License-Identifier: GPL-2.0
/* Copyright 2021-2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include <linux/hpe/sl/sl_media.h>
#include <linux/hpe/sl/sl_lgrp.h>

#include "sl_media_test_str.h"

#define SL_MEDIA_TEST_NAME "sl-test-media: "

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
