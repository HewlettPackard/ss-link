// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_iface.h"
#include "linktool_sbus.h"
#include "linktool_cmds.h"

#define SBUS_LOG "sbus -"

int linktool_cmd_sbus_rd(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	unsigned int dev = cmd_obj->dev_addr;
	unsigned int reg = cmd_obj->reg;
	unsigned int data;

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(SBUS_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_sbus_rd(dev, reg, &data);
	if (rtn != 0) {
		ERROR(SBUS_LOG "sbur_rd failed [%d]", rtn);
		goto out;
	}

	printf("sbus RD 0x%02X:0x%02X = 0x%08X\n", dev, reg, data);

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(SBUS_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}

int linktool_cmd_sbus_wr(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	unsigned int dev  = cmd_obj->dev_addr;
	unsigned int reg  = cmd_obj->reg;
	unsigned int data = cmd_obj->data;

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(SBUS_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	printf("sbus WR 0x%02X:0x%02X = 0x%08X\n", dev, reg, data);

	rtn = linktool_sbus_wr(dev, reg, data);
	if (rtn != 0) {
		ERROR(SBUS_LOG "sbur_wr failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(SBUS_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}
