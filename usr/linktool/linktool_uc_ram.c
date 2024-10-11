// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include "linktool.h"
#include "linktool_pmi.h"
#include "linktool_cmds.h"

static unsigned int linktool_uc_ram_debug = 0;

void linktool_uc_ram_debug_set()
{
	linktool_uc_ram_debug = 1;
}

#define UC_RAM_WR8 "uc_ram_wr8 -"
int linktool_uc_ram_wr8(unsigned int dev, unsigned int lane, unsigned int addr, unsigned char data)
{
	int rtn;

	DEBUG(linktool_uc_ram_debug, "uc_ram_wr8");

	// EFUN(wrc_micro_dr_raif_prtsel(osprey7_v2l4p1_INTERNAL_grp_idx_from_lane(osprey7_v2l4p1_acc_get_physical_lane(sa__))&1)); => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0400,10,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "dr raif prtsel",    dev, lane, 0, 0xD227, lane, 0x0400, 10);
	// EFUN(wrc_micro_autoinc_wraddr_en(0));                                                                                    => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x1000,12,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "autoinc wraddr en", dev, lane, 0, 0xD202, 0, 0x1000, 12);
	// EFUN(wrc_micro_ra_wrdatasize(0x0));                                 /* Select 8bit write datasize */                     => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0003,0,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "ra wrdatasize",     dev, lane, 0, 0xD202, 0, 0x0003, 0);
	// EFUN(wrc_micro_ra_wraddr_msw((uint16_t)((addr >> 16) & 0xFFFF)));   /* Upper 16bits of RAM address to be written to */   => osprey7_v2l4p1_acc_wr_reg(sa__, 0xd205,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "ra wraddr msw",     dev, lane, 0, 0xD205, ((addr >> 16) & 0xFFFF), 0xFFFF,  0);
	// EFUN(wrc_micro_ra_wraddr_lsw(addr & 0xFFFF));                       /* Lower 16bits of RAM address to be written to */   => osprey7_v2l4p1_acc_wr_reg(sa__, 0xd204,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "ra wraddr lsw",     dev, lane, 0, 0xD204, (addr & 0xFFFF), 0xFFFF,  0);
	// EFUN(wrc_micro_ra_wrdata_lsw(wr_val));                              /* uC RAM lower 16bits write data */                 => osprey7_v2l4p1_acc_wr_reg(sa__, 0xd206,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_WR8 "ra wrdata lsw",     dev, lane, 0, 0xD206, (data & 0xFFFF), 0xFFFF,  0);

	rtn = 0;
out:
	return rtn;
}

#define UC_RAM_RD8 "uc_ram_rd8 -"
int linktool_uc_ram_rd8(unsigned int dev, unsigned int lane, unsigned int addr, unsigned char *data)
{
	int rtn;

	DEBUG(linktool_uc_ram_debug, "uc_ram_rd8");

	// EPFUN(wrc_micro_autoinc_rdaddr_en(0));                                                                          => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x2000,13,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD8 "autoinc rdaddr en", dev, lane, 0, 0xD202, 0, 0x2000, 13);
	// EPFUN(wrc_micro_ra_rddatasize(0x0));                               /* Select 8bit read datasize */              => osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0030,4,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD8 "ra rddatasize",     dev, lane, 0, 0xD202, 0, 0x0030, 4);
	// EPFUN(wrc_micro_ra_rdaddr_msw((uint16_t)((addr >> 16) & 0xFFFF))); /* Upper 16bits of RAM address to be read */ => osprey7_v2l4p1_acc_wr_reg(sa__, 0xd209,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD8 "ra rdaddr msw",     dev, lane, 0, 0xD209, ((addr >> 16) & 0xFFFF), 0xFFFF,  0);
	// EPFUN(wrc_micro_ra_rdaddr_lsw(addr & 0xFFFF));                     /* Lower 16bits of RAM address to be read */ => osprey7_v2l4p1_acc_wr_reg(sa__, 0xd208,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD8 "ra rdaddr lsw",     dev, lane, 0, 0xD208, (addr & 0xFFFF), 0xFFFF,  0);
	// EPSTM(rddata = (uint8_t) rdc_micro_ra_rddata_lsw());               /* 16bit read data */                        => osprey7_v2l4p1_acc_rde_reg(sa__, 0xd20a,__ERR)
	LINKTOOL_PMI_RD(UC_RAM_RD8 "ra rddata lsw",     dev, lane, 0, 0xD20A, data);

	rtn = 0;
out:
	return rtn;
}

#define UC_RAM_RD_BLK "uc_ram_rd_blk -"
int linktool_uc_ram_rd_blk(unsigned int dev, unsigned int start_addr, unsigned int rd_size, unsigned char *rd_buff)
{
	int            rtn;
	unsigned short data;
	unsigned int   idx;

	DEBUG(linktool_uc_ram_debug, "uc_ram_rd_blk");

	// EFUN(wrc_micro_autoinc_rdaddr_en(1));                                                                = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x2000,13,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD_BLK "autoinc rdaddr en", dev, 0xFF, 0, 0xD202, 1, 0x2000, 13);
	// EFUN(wrc_micro_ra_rddatasize(0x1));                     /* Select 16bit read datasize */             = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0030,4,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD_BLK "ra rddatasize",     dev, 0xFF, 0, 0xD202, 1, 0x0030, 4);
	// EFUN(wrc_micro_ra_rdaddr_msw((uint16_t)(addr >> 16)));  /* Upper 16bits of RAM address to be read */ = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd209,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD_BLK "ra rdaddr msw",     dev, 0xFF, 0, 0xD209, ((start_addr >> 16) & 0xFFFF), 0xFFFF,  0);
	// EFUN(wrc_micro_ra_rdaddr_lsw(addr & 0xFFFE));           /* Lower 16bits of RAM address to be read */ = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd208,wr_val)
	LINKTOOL_PMI_WR(UC_RAM_RD_BLK "ra rdaddr lsw",     dev, 0xFF, 0, 0xD208, (start_addr & 0xFFFE), 0xFFFF,  0);

	// FIXME: assume aligned address for now
	// FIXME: assume from the same block for now
	idx = 0;
	while (rd_size > 0) {
		// ESTM(read_val |= (uint32_t)(rdc_micro_ra_rddata_lsw() << defecit)); => osprey7_v2l4p1_acc_rde_reg(sa__, 0xd20a,__ERR)
		LINKTOOL_PMI_RD(UC_RAM_RD_BLK "ra rddata lsw", dev, 0xFF, 0, 0xD20A, &data);
		DEBUG(linktool_uc_ram_debug, "%s data = 0x%04X", UC_RAM_RD_BLK, data);

		rd_buff[idx] = (data & 0xFF);
		rd_buff[idx + 1] = ((data >> 8) & 0xFF);
		rd_size -= 2;
		idx += 2;
	}

	rtn = 0;
out:
	return rtn;
}
