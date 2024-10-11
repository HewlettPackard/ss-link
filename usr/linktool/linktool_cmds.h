/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */

#ifndef _LINKTOOL_CMDS_H_
#define _LINKTOOL_CMDS_H_

#include "linktool_sbus.h"
#include "linktool_pmi.h"

#define LINKTOOL_NUM_LANES 4

enum {
	LINKTOOL_CMD_NONE         = 0,
	LINKTOOL_CMD_SBUS_RD,
	LINKTOOL_CMD_SBUS_WR,
	LINKTOOL_CMD_PMI_RD,
	LINKTOOL_CMD_PMI_WR,
	LINKTOOL_CMD_INFO_GET,
	LINKTOOL_CMD_FW_LOAD,
	LINKTOOL_CMD_CORE_INIT,
	LINKTOOL_CMD_LANE_UP,
	LINKTOOL_CMD_LANE_DOWN,

	LINKTOOL_CMD_COUNT        /* must be last */
};

struct linktool_clock_info {
	unsigned int   refclk;
	unsigned int   comclk;
	unsigned int   divider;
};

struct linktool_link_config {
	unsigned int   lane_map;
	unsigned int   pre3;
	unsigned int   pre2;
	unsigned int   pre1;
	unsigned int   cursor;
	unsigned int   post1;
	unsigned int   post2;
	unsigned int   width;
	unsigned int   osr;
	unsigned int   encoding;
	unsigned int   media;
};

struct linktool_uc_info {
	unsigned char  lane_count;
	unsigned int   lane_static_var_ram_base;
	unsigned int   lane_static_var_ram_size;
	unsigned int   lane_var_ram_base;
	unsigned int   lane_var_ram_size;
	unsigned short grp_ram_size;
};

struct linktool_cmd_obj {
	unsigned int   dev_addr;
	unsigned int   reg;
	unsigned int   data;
	unsigned int   pll;
	unsigned int   lane;
	unsigned int   temperature;
	char          *filename;
	unsigned char *fw_image;
	unsigned int   image_size;
	unsigned int   stack_size;
	unsigned int   image_crc;

	struct linktool_clock_info  clock_info;
	struct linktool_link_config link_config;
	struct linktool_uc_info     uc_info;
};

void linktool_cmd_info_get_debug_set();
void linktool_cmd_fw_load_debug_set();
void linktool_cmd_core_init_debug_set();
void linktool_cmd_lane_up_debug_set();
void linktool_cmd_lane_down_debug_set();

int linktool_cmd_sbus_rd(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_sbus_wr(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_pmi_rd(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_pmi_wr(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_info_get(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_fw_load(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_core_init(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_lane_up(struct linktool_cmd_obj *cmd_obj);
int linktool_cmd_lane_down(struct linktool_cmd_obj *cmd_obj);

#define LINKTOOL_PMI_WR(_str, _dev_addr, _lane, _pll, _reg, _data, _mask, _lsb)      \
	{                                                                            \
	rtn = linktool_pmi_wr(_dev_addr, _lane, _pll, _reg, (_data << _lsb), _mask); \
	if (rtn != 0) {                                                              \
		ERROR("%s failed [%d]", _str, rtn);                                  \
		goto out;                                                            \
	}                                                                            \
	}
#define LINKTOOL_PMI_RD(_str, _dev_addr, _lane, _pll, _reg, _data)     \
	{                                                              \
	unsigned int _data32;                                          \
	rtn = linktool_pmi_rd(_dev_addr, _lane, _pll, _reg, &_data32); \
	if (rtn != 0) {                                                \
		ERROR("%s failed [%d]", _str, rtn);                    \
		goto out;                                              \
	}                                                              \
	*_data = _data32;                                              \
	}
#define LINKTOOL_PMI_FIELD_RD(_str, _dev_addr, _lane, _pll, _reg, _shl, _shr, _data) \
	{                                                                            \
	unsigned int   _data32;                                                      \
	unsigned short _data16;                                                      \
	rtn = linktool_pmi_rd(_dev_addr, _lane, _pll, _reg, &_data32);               \
	if (rtn != 0) {                                                              \
		ERROR("%s failed [%d]", _str, rtn);                                  \
		goto out;                                                            \
	}                                                                            \
	_data16 = (unsigned short)(_data32 << _shl);                                 \
	_data16 = _data16 >> _shr;                                                   \
	*_data = _data16;                                                            \
	}

#define LINKTOOL_SBUS_RST(_str, _dev_addr)          \
	{                                           \
	rtn = linktool_sbus_rst(_dev_addr);         \
	if (rtn != 0) {                             \
		ERROR("%s failed [%d]", _str, rtn); \
		goto out;                           \
	}                                           \
	}
#define LINKTOOL_SBUS_WR(_str, _dev_addr, _reg, _data)  \
	{                                               \
	rtn = linktool_sbus_wr(_dev_addr, _reg, _data); \
	if (rtn != 0) {                                 \
		ERROR("%s failed [%d]", _str, rtn);     \
		goto out;                               \
	}                                               \
	}
#define LINKTOOL_SBUS_FIELD_WR(_str, _dev_addr, _reg, _lsb, _mask, _data) \
	{                                                                 \
	unsigned int _data32;                                             \
	rtn = linktool_sbus_rd(_dev_addr, _reg, &_data32);                \
	if (rtn != 0) {                                                   \
		ERROR("%s RD failed [%d]", _str, rtn);                    \
		goto out;                                                 \
	}                                                                 \
	_data32 = ((_data32 & (~(_mask << _lsb))) | (_data << _lsb));     \
	rtn = linktool_sbus_wr(_dev_addr, _reg, _data32);                 \
	if (rtn != 0) {                                                   \
		ERROR("%s WR failed [%d]", _str, rtn);                    \
		goto out;                                                 \
	}                                                                 \
	}
#define LINKTOOL_SBUS_RD(_str, _dev_addr, _reg, _data)  \
	{                                               \
	rtn = linktool_sbus_rd(_dev_addr, _reg, _data); \
	if (rtn != 0) {                                 \
		ERROR("%s failed [%d]", _str, rtn);     \
		goto out;                               \
	}                                               \
	}
#define LINKTOOL_SBUS_FIELD_RD(_str, _dev_addr, _reg, _lsb, _mask, _data) \
	{                                                                 \
	unsigned int _data32;                                             \
	rtn = linktool_sbus_rd(_dev_addr, _reg, &_data32);                \
	if (rtn != 0) {                                                   \
		ERROR("%s failed [%d]", _str, rtn);                       \
		goto out;                                                 \
	}                                                                 \
	*_data = ((_data32 >> _lsb) & _mask);                             \
	}

#endif /* _LINKTOOL_CMDS_H_ */
