// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024 Hewlett Packard Enterprise Development LP */

#include <linux/spinlock.h>

#include "uapi/sl_lgrp.h"

#include "sl_link.h"

#include "base/sl_core_log.h"
#include "sl_core_link.h"
#include "test/sl_core_test_an.h"

#define LOG_NAME SL_CORE_TEST_AN_LOG_NAME

int sl_core_test_an_caps_set(struct sl_link *link, struct sl_link_caps *caps)
{
	unsigned long        irq_flags;
	struct sl_core_link *core_link;

	sl_core_log_dbg(link, LOG_NAME, "test_an_caps_set");

	core_link = sl_core_link_get(link->ldev_num, link->lgrp_num, link->num);

	spin_lock_irqsave(&core_link->an.data_lock, irq_flags);
	core_link->an.test_caps     = *caps;
	core_link->an.use_test_caps = (caps->tech_map != 0);
	spin_unlock_irqrestore(&core_link->an.data_lock, irq_flags);

	return 0;
}