/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_SBUS_H_
#define _LINKTOOL_SBUS_H_

void linktool_sbus_debug_set();

int linktool_sbus_rst(unsigned int dev_addr);
int linktool_sbus_wr(unsigned int dev_addr, unsigned char reg, unsigned int data);
int linktool_sbus_rd(unsigned int dev_addr, unsigned char reg, unsigned int *data);

#endif /* _LINKTOOL_SBUS_H_ */
