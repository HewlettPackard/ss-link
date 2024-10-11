// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "linktool.h"
#include "linktool_iface.h"

static unsigned int sbus_debug = 0;

void linktool_sbus_debug_set()
{
	sbus_debug = 1;
}

int linktool_sbus_rst(unsigned int dev_addr)
{
	int rtn;

	unsigned int data32;

	DEBUG(sbus_debug, "sbus_rst (dev_addr = 0x%02X)", dev_addr);

	rtn = linktool_iface_sbus_fn(dev_addr, 0, LINKTOOL_IFACE_CMD_RST, &data32);
	if (rtn != 0) {
		ERROR("sbus_rst - iface_sbus_fn failed [%d]", rtn);
		return rtn;
	}

	DEBUG(sbus_debug, "sbus_rst (data32 = 0x%X)", data32);

	return 0;
}

int linktool_sbus_wr(unsigned int dev_addr, unsigned char reg, unsigned int data)
{
	int rtn;

	dev_addr &= 0xFF;
	reg      &= 0xFF;

	DEBUG(sbus_debug,
		"sbus_wr (dev_addr = 0x%02X, reg = 0x%02X, data = 0x%08X)", dev_addr, reg, data);

	rtn = linktool_iface_sbus_fn(dev_addr, reg, LINKTOOL_IFACE_CMD_WR, &data);
	if (rtn != 0) {
		ERROR("sbus_wr - iface_sbus_fn failed [%d]", rtn);
		return rtn;
	}

	return 0;
}

int linktool_sbus_rd(unsigned int dev_addr, unsigned char reg, unsigned int *data)
{
	int rtn;

	dev_addr &= 0xFF;
	reg      &= 0xFF;

	DEBUG(sbus_debug, "sbus_rd (dev_addr = 0x%02X, reg = 0x%02X)", dev_addr, reg);

	rtn = linktool_iface_sbus_fn(dev_addr, reg, LINKTOOL_IFACE_CMD_RD, data);
	if (rtn != 0) {
		ERROR("sbus_rd - iface_sbus_fn failed [%d]", rtn);
		return rtn;
	}

	DEBUG(sbus_debug, "sbus_rd (data = 0x%08X)", *data);

	return 0;
}
