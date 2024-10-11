/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_UC_RAM_H_
#define _LINKTOOL_UC_RAM_H_

void linktool_uc_ram_debug_set();

int linktool_uc_ram_wr8(unsigned int dev, unsigned int lane,
			unsigned int addr, unsigned char data);

int linktool_uc_ram_rd8(unsigned int dev, unsigned int lane,
			unsigned int addr, unsigned char *data);

int linktool_uc_ram_rd_blk(unsigned int dev, unsigned int start_addr,
			   unsigned int rd_size, unsigned char *rd_buff);

#endif /* _LINKTOOL_UC_RAM_H_ */
