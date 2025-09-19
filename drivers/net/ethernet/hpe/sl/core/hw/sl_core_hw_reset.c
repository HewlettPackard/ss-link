// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022,2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <linux/types.h>

#include "sl_asic.h"
#include "sl_core_ldev.h"
#include "sl_core_lgrp.h"
#include "sl_core_link.h"
#include "base/sl_core_log.h"
#include "hw/sl_core_hw_intr.h"
#include "hw/sl_core_hw_io.h"
#include "hw/sl_core_hw_reset.h"

#define LOG_NAME SL_CORE_RESET_LOG_NAME

static void sl_core_hw_reset_on(struct sl_core_lgrp *core_lgrp, u8 link_num)
{
	u32 port;

	port = core_lgrp->num;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"on (port = %u, link_num = %u)", port, link_num);

	core_lgrp->core_ldev->ops.write64(core_lgrp->core_ldev->accessors.pci,
		SS2_PORT_PML_CFG_SUBPORT_RESET(link_num),
		SS2_PORT_PML_CFG_SUBPORT_RESET_WARM_RST_FROM_CSR_SET(1));

	(void)core_lgrp->core_ldev->ops.read64(core_lgrp->core_ldev->accessors.pci,
		SS2_PORT_PML_CFG_SUBPORT_RESET(link_num));
}

static void sl_core_hw_reset_off(struct sl_core_lgrp *core_lgrp, u8 link_num)
{
	u32 port;

	port = core_lgrp->num;

	sl_core_log_dbg(core_lgrp, LOG_NAME,
		"off (port = %u, link_num = %u)", port, link_num);

	core_lgrp->core_ldev->ops.write64(core_lgrp->core_ldev->accessors.pci,
		SS2_PORT_PML_CFG_SUBPORT_RESET(link_num),
		SS2_PORT_PML_CFG_SUBPORT_RESET_WARM_RST_FROM_CSR_SET(0));

	(void)core_lgrp->core_ldev->ops.read64(core_lgrp->core_ldev->accessors.pci,
		SS2_PORT_PML_CFG_SUBPORT_RESET(link_num));
}

void sl_core_hw_reset_link(struct sl_core_link *core_link)
{
	sl_core_log_dbg(core_link, LOG_NAME, "link");

	sl_core_hw_reset_on(core_link->core_lgrp, core_link->num);
	sl_core_hw_reset_off(core_link->core_lgrp, core_link->num);
}

void sl_core_hw_reset_lgrp(struct sl_core_lgrp *core_lgrp)
{
	u8 link_num;

	sl_core_log_dbg(core_lgrp, LOG_NAME, "lgrp");

	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num)
		sl_core_hw_reset_on(core_lgrp, link_num);
	for (link_num = 0; link_num < SL_ASIC_MAX_LINKS; ++link_num)
		sl_core_hw_reset_off(core_lgrp, link_num);
}
