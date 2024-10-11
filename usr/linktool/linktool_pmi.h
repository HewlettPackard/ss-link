/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_PMI_H_
#define _LINKTOOL_PMI_H_

void linktool_pmi_debug_set();

int linktool_pmi_rd(unsigned int dev_addr, unsigned int lane, unsigned int pll,
	unsigned int reg, unsigned int *data);
int linktool_pmi_wr(unsigned int dev_addr, unsigned int lane, unsigned int pll,
	unsigned int reg, unsigned int data, unsigned int mask);

#endif /* _LINKTOOL_PMI_H_ */
