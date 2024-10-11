// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "linktool.h"
#include "linktool_iface.h"
#include "linktool_sbus.h"
#include "linktool_pmi.h"
#include "linktool_cmds.h"

// FIXME: scrub out broadcom annotations

static unsigned int linktool_cmd_fw_load_debug = 0;

void linktool_cmd_fw_load_debug_set()
{
	linktool_cmd_fw_load_debug = 1;
}

#define HW_INFO "hw_info - "
static int linktool_cmd_hw_info(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	unsigned int  dev = cmd_obj->dev_addr;
	unsigned char cores;
	unsigned char revid1;
	unsigned char revid2;
	unsigned char version;

	DEBUG(linktool_cmd_fw_load_debug, "hw_info");

	// rdc_micro_num_uc_cores = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd21a,0,12,__ERR)
	LINKTOOL_PMI_FIELD_RD(HW_INFO "uc cores read",   dev, 0, 0, 0xD21A, 0, 12, &cores);
	// rdc_revid_model = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd100,10,10,__ERR)
	LINKTOOL_PMI_FIELD_RD(HW_INFO "uc revid1 read",  dev, 0, 0, 0xD100, 10, 10, &revid1);
	// rdc_revid2 = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd10e,12,12,__ERR)
	LINKTOOL_PMI_FIELD_RD(HW_INFO "uc revid2 read",  dev, 0, 0, 0xD10E, 12, 12, &revid2);
	// rd_ams_tx_version_id = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd0d8,8,8,__ERR)
	LINKTOOL_PMI_FIELD_RD(HW_INFO "uc version read", dev, 0, 0, 0xD0D8, 8, 8, &version);

	INFO(HW_INFO "rev1 = 0x%02X, rev2 = 0x%02X, ver = 0x%02X, cores = %d",
		revid1, revid2, version, cores);

	rtn = 0;

out:
	return rtn;
}

#define LINKTOOL_WAIT_ACTIVE_COUNT 100
static int linktool_cmd_wait_active(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   dev = cmd_obj->dev_addr;
	int            x;
	unsigned char  data8;
	unsigned short data16;

	DEBUG(linktool_cmd_fw_load_debug, "wait_active");

	/* uc wait active */
	for (x = 0; x < LINKTOOL_WAIT_ACTIVE_COUNT; ++x) {
		// rdc_uc_active() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd101,14,15,__ERR)
		LINKTOOL_PMI_FIELD_RD("wait_active - active", dev, 0, 0, 0xD101, 14, 15, &data8);
		if (data8 != 1)
			continue;
		// reg_rdc_MICRO_B_COM_RMI_MICRO_SDK_STATUS0 = osprey7_v2l4p1_acc_rde_reg(sa__, 0xd21a,__ERR)
		LINKTOOL_PMI_RD("wait_active - status", dev, 0, 0, 0xD21A, &data16);
		if ((data16 & 0x3) != 0x3)
			continue;
		break;
	}
	if (x >= LINKTOOL_WAIT_ACTIVE_COUNT) {
		ERROR("wait_active - timeout");
		rtn = -1;
		goto out;
	}

	DEBUG(linktool_cmd_fw_load_debug, "wait_active - done");

	rtn = 0;

out:
	return rtn;
}

static int linktool_cmd_fw_file_load(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	FILE         *fptr;
	char          string[100];
	char         *ptr;
	unsigned int  byte;
	char          version[10] = "\0";
	int           image_size;
	int           stack_size;
	int           image_crc;
	int           x;

	DEBUG(linktool_cmd_fw_load_debug, "fw_file_load");

	fptr = fopen(cmd_obj->filename, "rb");
	if (fptr == NULL) {
		ERROR("fw_file_load - unable top open file \"%s\"", cmd_obj->filename);
		return -1;
	}

	image_size = 0;
	stack_size = 0;
	image_crc  = 0;

	while (fgets(string, sizeof(string), fptr) != NULL) {
		if ((ptr = strstr(string, "UCODE_IMAGE_VERSION")) != 0) {
			/* OSPREY7_V2L4P1_UCODE_IMAGE_VERSION "D003_04" */
			const char *start = strchr(ptr, '"') + 1;
			strncpy(version, start, strcspn(start, "\""));
		} else if ((ptr = strstr(string, "UCODE_IMAGE_SIZE")) != 0) {
			/* done if it's the data */
			if (strchr(ptr, '{'))
				break;
			image_size = strtol(ptr + 17, 0, 0);
		} else if ((ptr = strstr(string, "UCODE_STACK_SIZE")) != 0) {
			stack_size = strtol(ptr + 17, 0, 0);
		} else if ((ptr = strstr(string, "UCODE_IMAGE_CRC")) != 0) {
			image_crc = strtol(ptr + 17, 0, 0);
		}
    	}

	INFO("fw_file_load - version = \"%s\", image_size = %d bytes, stack_size = %d bytes, crc = 0x%X",
		version, image_size, stack_size, image_crc);

	cmd_obj->fw_image = malloc(image_size + 4); /* pad to full word */
	if (cmd_obj->fw_image == NULL) {
		ERROR("fw_file_load - malloc failure");
		rtn = -1;
		goto out;
	}
	memset(cmd_obj->fw_image, 0, image_size + 4);

        for (x = 0; x < image_size; ++x) {
		if (fscanf(fptr, "%10x,", &byte) != 1) {
			ERROR("fw_file_load - out of bytes");
			rtn = -1;
			goto out;
		}
		cmd_obj->fw_image[x] = (unsigned char)byte;
//		if (x < 20)
//			DEBUG(linktool_cmd_fw_load_debug, "fw_file_load - byte[%02d] = 0x%X", x, byte);
	}
	/* final check on size */
	if (fscanf(fptr, "%10x", &byte) > 0) {
		ERROR("fw_file_load - image shorter than expected");
		rtn = -1;
		goto out;
	}

	cmd_obj->image_size = image_size;
	cmd_obj->stack_size = stack_size;
	cmd_obj->image_crc  = image_crc;

	rtn = 0;

out:
	fclose(fptr);

	return rtn;
}

#define FW_LOAD_OPEN                 "fw_load_open - "
#define FW_LOAD_OPEN_INIT_DONE_COUNT 10
static int linktool_cmd_fw_load_open(struct linktool_cmd_obj *cmd_obj)
{
	int           rtn;
	unsigned int  dev = cmd_obj->dev_addr;
	int           x;
	unsigned char data8;

	DEBUG(linktool_cmd_fw_load_debug, "fw_load_open");

	/* UC assert reset */
	// wrc_micro_micro_s_rstb(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "clear",            dev, 0, 0, 0xD23E, 0, 0x0001, 0);
	// wrc_micro_micro_s_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "set",              dev, 0, 0, 0xD23E, 1, 0x0001, 0);
	// wrc_micro_cr_crc_calc_en(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "crc calc disable", dev, 0, 0, 0xD217, 0, 0x0001, 0);
	// wrc_micro_core_stack_size() = osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd22e,0x7ffc,2,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "stack size",       dev, 0, 0, 0xD22E, cmd_obj->stack_size, 0x7FFC, 2);
	// wrc_micro_core_stack_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd22e,0x8000,15,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "stack enable",     dev, 0, 0, 0xD22E, 1, 0x8000, 15);
	// wrc_uc_active(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd101,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "active clear",     dev, 0, 0, 0xD101, 0, 0x0002, 1);

	/* enable PRAM */
	LINKTOOL_SBUS_WR("enable PRAM", dev, 0, 0x80000000);

	/* UC load init */
	// wrc_micro_master_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd200,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "master clock enable", dev, 0, 0, 0xD200, 1, 0x0001, 0);
	// wrc_micro_master_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "master clear reset",  dev, 0, 0, 0xD201, 1, 0x0001, 0);
	// wrc_micro_master_rstb(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "master assert reset", dev, 0, 0, 0xD201, 0, 0x0001, 0);
	// wrc_micro_master_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "master clear reset",  dev, 0, 0, 0xD201, 1, 0x0001, 0);
	// wrc_micro_cr_access_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "cRAM enable",         dev, 0, 0, 0xD227, 1, 0x0001, 0);
	// wrc_micro_ra_init(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "init cRAM set",       dev, 0, 0, 0xD202, 1, 0x0300, 8);
	// rdc_micro_ra_initdone = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd203,15,15,__ERR)
	for (x = 0; x < FW_LOAD_OPEN_INIT_DONE_COUNT; ++x) {
		LINKTOOL_PMI_FIELD_RD(FW_LOAD_OPEN "init done check", dev, 0, 0, 0xD203, 15, 15, &data8);
		if (data8 != 0)
			break;
	}
	if (x >= FW_LOAD_OPEN_INIT_DONE_COUNT) {
		ERROR(FW_LOAD_OPEN "wait for init done timeout");
		rtn = -1;
		goto out;
	}
	// wrc_micro_ra_init(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "init cRAM clear",     dev, 0, 0, 0xD202, 0, 0x0300, 8);
	// wrc_micro_cr_crc_prtsel(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0004,2,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "crc enable",          dev, 0, 0, 0xD217, 0, 0x0004, 2);
	// wrc_micro_cr_prif_prtsel(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0008,3,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "pRAM enable",         dev, 0, 0, 0xD227, 0, 0x0008, 3);
	// wrc_micro_cr_crc_init(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "crc init",            dev, 0, 0, 0xD217, 1, 0x0002, 1);
	// wrc_micro_cr_crc_init(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "crc init clear",      dev, 0, 0, 0xD217, 0, 0x0002, 1);
	// wrc_micro_cr_crc_calc_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "crc calc enable",     dev, 0, 0, 0xD217, 1, 0x0001, 0);
	// wrc_micro_cr_ignore_micro_code_writes(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "ignore pRAM writes",  dev, 0, 0, 0xD227, 0, 0x0002, 1);
	// wrc_micro_pramif_ahb_wraddr_msw(0) = osprey7_v2l4p1_acc_wr_reg(sa__, 0xd20e,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "pRAM msw",            dev, 0, 0, 0xD20E, 0, 0xFFFF, 0);
	// wrc_micro_pramif_ahb_wraddr_lsw(0) = osprey7_v2l4p1_acc_mwr_reg(sa__, 0xd20d,0xfffc,2,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "pRAM lsw",            dev, 0, 0, 0xD20D, 0, 0xFFFC, 2);
	// wrc_micro_pram_if_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x8000,15,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "pRAM reset",          dev, 0, 0, 0xD201, 1, 0x8000, 15);
	// wrc_micro_pramif_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd20c,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_OPEN "pRAM enable",         dev, 0, 0, 0xD20C, 1, 0x0001, 0);

	rtn = 0;

out:
	return rtn;
}

#define FW_LOAD_WRITE           "fw_load_write - "
#define LINKTOOL_BROADCAST_ADDR 0xFA
static int linktool_cmd_fw_load_write(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	unsigned int x;
	unsigned int data32;

	DEBUG(linktool_cmd_fw_load_debug, "fw_load_write");

	for (x = 0; x < cmd_obj->image_size; x += 4) {
		data32 = (cmd_obj->fw_image[x] |
			(cmd_obj->fw_image[x+1] << 8) |
			(cmd_obj->fw_image[x+2] << 16) |
			(cmd_obj->fw_image[x+3] << 24));
		DEBUG(linktool_cmd_fw_load_debug, "fw_load_write - image[%d] = 0x%08X", x, data32);
		LINKTOOL_SBUS_WR("write PRAM", LINKTOOL_BROADCAST_ADDR, 10, data32);
	}

	rtn = 0;

out:
	return rtn;
}

#define FW_LOAD_CLOSE                   "fw_load_close - "
#define FW_LOAD_CLOSE_INIT_DONE_COUNT   10
#define FW_LOAD_CLOSE_WAIT_ACTIVE_COUNT 100
static int linktool_cmd_fw_load_close(struct linktool_cmd_obj *cmd_obj)
{
	int            rtn;
	unsigned int   x;
	unsigned int   dev = cmd_obj->dev_addr;
	unsigned char  data8;
	unsigned short data16;

	DEBUG(linktool_cmd_fw_load_debug, "fw_load_close");

	/* UC load close */
	// wrc_micro_cr_ignore_micro_code_writes(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0002,1,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "pRAM disable",     dev, 0, 0, 0xD227, 1, 0x0002, 1);
	// wrc_micro_cr_crc_calc_en(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd217,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "crc calc disable", dev, 0, 0, 0xD217, 0, 0x0001, 0);
	// rdc_micro_num_uc_cores = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd21a,0,12,__ERR)
	LINKTOOL_PMI_FIELD_RD(FW_LOAD_CLOSE "uc cores read",    dev, 0, 0, 0xD21A, 0, 12, &data8);
	for (x = 0; x < data8; ++x) {
		// wrc_micro_core_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd240,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(FW_LOAD_CLOSE "core clock enable", dev, 0, x, 0xD240, 1, 0x0001, 0);
	}
	// wrc_bcm_serdes_micro_core_stack_size() = bcm_serdes_acc_mwr_reg(sa__,0xd22e,0x7ffc,2,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "stack size set",  dev, 0, 0, 0xD22E, cmd_obj->stack_size, 0x7FFC, 2);
	// wrc_bcm_serdes_micro_core_stack_en = bcm_serdes_acc_mwr_reg_u8(sa__,0xd22e,0x8000,15,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "stack enable", dev, 0, 0, 0xD22E, 1, 0x8000, 15);
	// wrc_bcm_serdes_micro_pramif_en(0) = bcm_serdes_acc_mwr_reg_u8(sa__,0xd20c,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "pRAM disable", dev, 0, 0, 0xD20C, 0, 0x0001, 0);

	/* disable PRAM */
	LINKTOOL_SBUS_WR("disable PRAM", dev, 0, 0x00000000);

	/* UC de-assert reset */
	// wrc_micro_micro_s_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd23e,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "reset set",           dev, 0, 0, 0xD23E, 1, 0x0001, 0);
	// wrc_micro_master_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd200,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "master clock enable", dev, 0, 0, 0xD200, 1, 0x0001, 0);
	// wrc_micro_master_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd201,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "uc subsys enable",    dev, 0, 0, 0xD201, 1, 0x0001, 0);
	// wrc_micro_cr_access_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd227,0x0001,0,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "cRAM enable",         dev, 0, 0, 0xD227, 1, 0x0001, 0);
	// wrc_micro_ra_init(2) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "cRAM init set",       dev, 0, 0, 0xD202, 2, 0x0300, 8);
	// rdc_micro_ra_initdone() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd203,15,15,__ERR)
	for (x = 0; x < FW_LOAD_CLOSE_INIT_DONE_COUNT; ++x) {
		LINKTOOL_PMI_FIELD_RD(FW_LOAD_CLOSE "init done check", dev, 0, 0, 0xD203, 15, 15, &data8);
		if (data8 != 0)
			break;
	}
	if (x >= FW_LOAD_CLOSE_INIT_DONE_COUNT) {
		ERROR(FW_LOAD_CLOSE "wait for init done timeout");
		rtn = -1;
		goto out;
	}
	// wrc_micro_ra_init(0) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd202,0x0300,8,wr_val)
	LINKTOOL_PMI_WR(FW_LOAD_CLOSE "cRAM init clear", dev, 0, 0, 0xD202, 0, 0x0300, 8);

	/* UC enable clocks */
	// rdc_micro_num_uc_cores = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd21a,0,12,__ERR)
	LINKTOOL_PMI_FIELD_RD(FW_LOAD_CLOSE "uc cores read",   dev, 0, 0, 0xD21A, 0, 12, &data8);
	for (x = 0; x < data8; ++x) {
		// wrc_micro_core_clk_en(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd240,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(FW_LOAD_CLOSE "enable uc clock", dev, 0, x, 0xD240, 1, 0x0001, 0);
		// wrc_micro_core_rstb(1) = osprey7_v2l4p1_acc_mwr_reg_u8(sa__, 0xd241,0x0001,0,wr_val)
		LINKTOOL_PMI_WR(FW_LOAD_CLOSE "enable uc core",  dev, 0, x, 0xD241, 1, 0x0001, 0);
	}

	/* uc crc verify */
	// rdc_micro_cr_crc_checksum() = osprey7_v2l4p1_acc_rde_reg(sa__, 0xd218,__ERR)
	LINKTOOL_PMI_RD(FW_LOAD_CLOSE "crc verify", dev, 0, 0, 0xD218, &data16);
	if (data16 != cmd_obj->image_crc) {
		ERROR(FW_LOAD_CLOSE "crc failure (expect = 0x%X, calc = 0x%X)", cmd_obj->image_crc, data16);
		rtn = -1;
		goto out;
	}

	rtn = linktool_cmd_wait_active(cmd_obj);
	if (rtn != 0) {
		ERROR(FW_LOAD_CLOSE "wait active timeout");
		goto out;
	}

#if 0
	/* uc wait active */
	for (x = 0; x < FW_LOAD_CLOSE_WAIT_ACTIVE_COUNT; ++x) {
		// rdc_uc_active() = osprey7_v2l4p1_acc_rde_field_u8(sa__, 0xd101,14,15,__ERR)
		LINKTOOL_UC_RD_U8(FW_LOAD_CLOSE "wait active", dev, 0, 0, 0xD101, 14, 15, &data8);
		if (data8 != 1)
			continue;
		// reg_rdc_MICRO_B_COM_RMI_MICRO_SDK_STATUS0 = osprey7_v2l4p1_acc_rde_reg(sa__, 0xd21a,__ERR)
		LINKTOOL_UC_RD_U16(FW_LOAD_CLOSE "wait active status", dev, 0, 0, 0xD21A, &data16);
		if ((data16 & 0x3) != 0x3)
			continue;
		break;
	}
	if (x >= FW_LOAD_CLOSE_WAIT_ACTIVE_COUNT) {
		ERROR(FW_LOAD_CLOSE "wait active timeout");
		rtn = -1;
		goto out;
	}
#endif

	rtn = 0;

out:
	return rtn;
}

static int linktool_cmd_fw_info(struct linktool_cmd_obj *cmd_obj)
{
	(void)cmd_obj;

	DEBUG(linktool_cmd_fw_load_debug, "fw_info");

	// FIXME: serdes info
	// bcm_serdes_init_bcm_serdes_info

	// FIXME:
	return 0;
}

int linktool_cmd_fw_load(struct linktool_cmd_obj *cmd_obj)
{
	int          rtn;
	unsigned int dev = cmd_obj->dev_addr;

	DEBUG(linktool_cmd_fw_load_debug, "fw_load");

	rtn = linktool_iface_open_fn();
	if (rtn != 0) {
		ERROR("fw_load - iface_open_fn failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_fw_file_load(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - fw_file_load failed [%d]", rtn);
		goto out;
	}

// FIXME: need to do a first read (don't check return)
	{
	INFO("FIXME: dummy read");
	unsigned char dummy;
	LINKTOOL_PMI_FIELD_RD("dummy read", dev, 0, 0, 0xD21A, 0, 12, &dummy);
	}

	rtn = linktool_cmd_fw_load_open(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - fw_load_open failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_fw_load_write(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - fw_load_write failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_fw_load_close(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - fw_load_close failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_hw_info(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - hw_info failed [%d]", rtn);
		goto out;
	}

	rtn = linktool_cmd_fw_info(cmd_obj);
	if (rtn != 0) {
		ERROR("fw_load - fw_info failed [%d]", rtn);
		goto out;
	}

out:
	if (linktool_iface_close_fn() != 0)
		ERROR("fw_load - iface_close_fn failed [%d]", rtn);
	if (cmd_obj->fw_image != NULL)
		free(cmd_obj->fw_image);

	return rtn;
}
