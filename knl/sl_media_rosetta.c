// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/module.h>

#include <linux/sl_media.h>

#include "base/sl_media_log.h"
#include "sl_lgrp.h"
#include "sl_media_lgrp.h"
#include "sl_media_jack_rosetta.h"

#define LOG_BLOCK SL_LOG_BLOCK
#define LOG_NAME  SL_LOG_MEDIA_LOG_NAME

int sl_media_fake_headshell_insert(struct sl_lgrp *lgrp,  struct sl_media_attr *media_attr)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "fake headshell insert fail");
		return rtn;
	}
	if (!media_attr) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "invalid media attr");
		return -EINVAL;
	}

	return sl_media_jack_fake_cable_insert(lgrp->ldev_num, lgrp->num, media_attr);
}
EXPORT_SYMBOL(sl_media_fake_headshell_insert);

int sl_media_fake_headshell_remove(struct sl_lgrp *lgrp)
{
	int rtn;

	rtn = sl_lgrp_check(lgrp);
	if (rtn) {
		sl_log_err(NULL, LOG_BLOCK, LOG_NAME, "fake headshell remove fail");
		return -EINVAL;
	}

	sl_media_jack_fake_cable_remove(lgrp->ldev_num, lgrp->num);

	return 0;
}
EXPORT_SYMBOL(sl_media_fake_headshell_remove);
