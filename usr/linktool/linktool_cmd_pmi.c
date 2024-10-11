// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_iface.h"
#include "linktool_pmi.h"
#include "linktool_cmds.h"

#define PMI_LOG "sbus -"

int linktool_cmd_pmi_rd(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	unsigned int dev  = cmd_obj->dev_addr;
	unsigned int reg  = cmd_obj->reg;
	unsigned int pll  = cmd_obj->pll;
	unsigned int lane = cmd_obj->lane;
	unsigned int data;

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(PMI_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_pmi_rd(dev, lane, pll, reg, &data);
	if (rtn != 0) {
		ERROR(PMI_LOG "pmi_rd failed [%d]", rtn);
		goto out;
	}

	printf("pmi RD 0x%02X:0x%02X (pll = %d, lane = %d) = 0x%04X\n",
		dev, reg, pll, lane, data);

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(PMI_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}

int linktool_cmd_pmi_wr(struct linktool_cmd_obj *cmd_obj)
{
	int rtn;
	unsigned int dev  = cmd_obj->dev_addr;
	unsigned int reg  = cmd_obj->reg;
	unsigned int pll  = cmd_obj->pll;
	unsigned int lane = cmd_obj->lane;
	unsigned int data = cmd_obj->data;

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR(PMI_LOG "iface_open_fn failed [%d]", rtn);
		goto out;
	}

	printf("pmi WR 0x%02X:0x%02X (pll = %d, lane = %d) = 0x%04X\n",
		dev, reg, pll, lane, data);

	// FIXME: for now all bits get written
	rtn = linktool_pmi_wr(dev, lane, pll, reg, data, 0xFFFF);
	if (rtn != 0) {
		ERROR(PMI_LOG "sbur_wr failed [%d]", rtn);
		goto out;
	}

	rtn = 0;

out:
	if (linktool_iface_close_fn() != 0)
		ERROR(PMI_LOG "iface_close_fn failed [%d]", rtn);

        return rtn;

}
