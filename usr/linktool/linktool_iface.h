/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_IFACE_H_
#define _LINKTOOL_IFACE_H_

#define LINKTOOL_IFACE_CORE 0x20

#define LINKTOOL_IFACE_CMD_RST 0
#define LINKTOOL_IFACE_CMD_WR  1
#define LINKTOOL_IFACE_CMD_RD  2

void         linktool_iface_debug_set();

int          linktool_iface_open_fn();
unsigned int linktool_iface_sbus_fn(unsigned int addr,
			unsigned char reg, unsigned char command,
			unsigned int *data);
int          linktool_iface_close_fn();

#endif /* _LINKTOOL_IFACE_H_ */
