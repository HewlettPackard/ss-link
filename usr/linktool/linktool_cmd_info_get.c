// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_addr.h"
#include "linktool_iface.h"
#include "linktool_sbus.h"
#include "linktool_pmi.h"
#include "linktool_cmds.h"

// FIXME: scrub out broadcom annotations

static unsigned int linktool_cmd_info_get_debug = 0;

void linktool_cmd_info_get_debug_set()
{
	linktool_cmd_info_get_debug = 1;
}

static int linktool_info_fill(info_t *info)
{
	int          rtn;
	unsigned int data32;
	unsigned int x;
	unsigned int y;
	unsigned int z;

	for (x = 0; x < info->chips; ++x) {
		DEBUG(linktool_cmd_info_get_debug, "info get --------------------- chip = %d", x);
		for (y = 0; y < info->rings; ++y) {
			DEBUG(linktool_cmd_info_get_debug, "info get --------------------- ring = %d", y);
			// Process ID
			rtn = linktool_sbus_rd(make_addr3(x, y,
				SBUS_CONTROLLER_ADDRESS), 0xfe, &data32);
			if (rtn != 0) {
				ERROR("info get - process rd failed [%d]", rtn);
				return -1;
			}
			DEBUG(linktool_cmd_info_get_debug, "info get (data = 0x%X)", data32);
			if ((data32 & 0xFF) != IP_REV_7NM) {
				ERROR("info get - unrecognized hw version");
				return -1;
			}
			info->ip_rev[x][y][SBUS_CONTROLLER_ADDRESS] = data32 & 0xFFFF;
			info->process_id[x][y] = TSMC_07;

			// MAX bus address
			rtn = linktool_sbus_rd(make_addr4(x, y,
				SBUS_CONTROLLER_ADDRESS, SBUS_BROADCAST), 0x02, &data32);
			if (rtn != 0) {
				ERROR("info get - max addr rd failed [%d]", rtn);
				return -1;
			}
			DEBUG(linktool_cmd_info_get_debug, "info get (data = 0x%X)", data32);
			info->max_dev_addr[x][y] = data32 - 1;

			DEBUG(linktool_cmd_info_get_debug, "info get ===== devices =====");
			for (z = 1; z <= info->max_dev_addr[x][y]; ++z) {
				DEBUG(linktool_cmd_info_get_debug, "info get (dev = 0x%X)", z);
				rtn = linktool_sbus_rd(z, 0xff, &data32);
				if (rtn != 0) {
					ERROR("info get - dev rd failed [%d]", rtn);
					return -1;
				}
				if ((data32 & 0xFF) == OSPREY) {
					DEBUG(linktool_cmd_info_get_debug, "info get - found OSPREY");
					info->dev_addr[x][y] = z;
					break;
				}
				DEBUG(linktool_cmd_info_get_debug, "info get (data = 0x%X)", data32);
			}
		}
	}

	return 0;
}

static void linktool_info_print(info_t *info)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < info->chips; ++x) {
		INFO("chip %d:", x);
		for (y = 0; y < info->rings; ++y) {
			INFO("  ring %d:", y);
			INFO("    ip rev     = 0x%X", info->ip_rev[x][y][SBUS_CONTROLLER_ADDRESS]);
			INFO("    process id = %d", info->process_id[x][y]);
			INFO("    max addr   = %d", info->max_dev_addr[x][y]);
			INFO("    dev addr   = %d", info->dev_addr[x][y]);
		}
	}
}

int linktool_cmd_info_get(struct linktool_cmd_obj *cmd_obj)
{
	int    rtn;
	info_t info;

	(void)cmd_obj;

	DEBUG(linktool_cmd_info_get_debug, "cmd_info_get");

	memset(&info, 0, sizeof(info));
	info.chips = 1; // we know this on a C2
	info.rings = 1; // we know this on a C2

	linktool_iface_open_fn();
// FIXME: need ability to point at a specific CXI device
	rtn = linktool_info_fill(&info);
	linktool_iface_close_fn();

	if (rtn == 0)
		linktool_info_print(&info);

	return rtn;
}
