// SPDX-License-Identifier: GPL-2.0
/* Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP */

#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <getopt.h>
#include <string.h>

#include "linktool.h"
#include "linktool_cmds.h"
#include "linktool_sbus.h"
#include "linktool_pmi.h"
#include "linktool_iface.h"
#include "linktool_uc_ram.h"
#include "linktool_lane_up_tx.h"
#include "linktool_lane_up_rx.h"
#include "linktool_lane_up_misc.h"

#define LINKTOOL_MAJOR_VER 0
#define LINKTOOL_MINOR_VER 7

static unsigned int linktool_debug = 0;

typedef int (*linktool_cmd_func_t)(struct linktool_cmd_obj *cmd_obj);

static struct linktool_cmd_obj cmd_obj;

struct linktool_cmd {
	const char *const   name;
	const char *const   desc;
	linktool_cmd_func_t func;
};

static const struct linktool_cmd linktool_cmds[LINKTOOL_CMD_COUNT] = {
	[LINKTOOL_CMD_SBUS_RD] = {
		.name = "sbus_rd",
		.desc = "read reg using sbus",
		.func = linktool_cmd_sbus_rd,
	},
	[LINKTOOL_CMD_SBUS_WR] = {
		.name = "sbus_wr",
		.desc = "write reg using sbus",
		.func = linktool_cmd_sbus_wr,
	},
	[LINKTOOL_CMD_PMI_RD] = {
		.name = "pmi_rd",
		.desc = "read reg using pmi",
		.func = linktool_cmd_pmi_rd,
	},
	[LINKTOOL_CMD_PMI_WR] = {
		.name = "pmi_wr",
		.desc = "write reg using pmi",
		.func = linktool_cmd_pmi_wr,
	},
	[LINKTOOL_CMD_INFO_GET] = {
		.name = "info",
		.desc = "get device info",
		.func = linktool_cmd_info_get,
	},
	[LINKTOOL_CMD_FW_LOAD] = {
		.name = "fw_load",
		.desc = "firmware load",
		.func = linktool_cmd_fw_load,
	},
	[LINKTOOL_CMD_CORE_INIT] = {
		.name = "core_init",
		.desc = "initialize the core",
		.func = linktool_cmd_core_init,
	},
	[LINKTOOL_CMD_LANE_UP] = {
		.name = "lane_up",
		.desc = "bring lane up",
		.func = linktool_cmd_lane_up,
	},
	[LINKTOOL_CMD_LANE_DOWN] = {
		.name = "lane_down",
		.desc = "bring lane down",
		.func = linktool_cmd_lane_down,
	},
};

enum opts {
	LINKTOOL_OPT_REG       = 0,
	LINKTOOL_OPT_DATA,
	LINKTOOL_OPT_PLL,
	LINKTOOL_OPT_LANE,
	LINKTOOL_OPT_LANE_MAP,
	LINKTOOL_OPT_PRE3,
	LINKTOOL_OPT_PRE2,
	LINKTOOL_OPT_PRE1,
	LINKTOOL_OPT_CURSOR,
	LINKTOOL_OPT_POST1,
	LINKTOOL_OPT_POST2,
	LINKTOOL_OPT_WIDTH,
	LINKTOOL_OPT_OSR,
	LINKTOOL_OPT_ENCODING,
	LINKTOOL_OPT_MEDIA,
	LINKTOOL_OPT_REFCLK,
	LINKTOOL_OPT_COMCLK,
	LINKTOOL_OPT_DIVIDER,
};

static const struct option opts[] = {
	[LINKTOOL_OPT_REG]      = {"reg",      required_argument, 0, 0},
	[LINKTOOL_OPT_DATA]     = {"data",     required_argument, 0, 0},
	[LINKTOOL_OPT_PLL]      = {"pll",      required_argument, 0, 0},
	[LINKTOOL_OPT_LANE]     = {"lane",     required_argument, 0, 0},
	[LINKTOOL_OPT_LANE_MAP] = {"lane_map", required_argument, 0, 0},
	[LINKTOOL_OPT_PRE3]     = {"pre3",     required_argument, 0, 0},
	[LINKTOOL_OPT_PRE2]     = {"pre2",     required_argument, 0, 0},
	[LINKTOOL_OPT_PRE1]     = {"pre1",     required_argument, 0, 0},
	[LINKTOOL_OPT_CURSOR]   = {"cursor",   required_argument, 0, 0},
	[LINKTOOL_OPT_POST1]    = {"post1",    required_argument, 0, 0},
	[LINKTOOL_OPT_POST2]    = {"post2",    required_argument, 0, 0},
	[LINKTOOL_OPT_WIDTH]    = {"width",    required_argument, 0, 0},
	[LINKTOOL_OPT_OSR]      = {"osr",      required_argument, 0, 0},
	[LINKTOOL_OPT_ENCODING] = {"encoding", required_argument, 0, 0},
	[LINKTOOL_OPT_MEDIA]    = {"media",    required_argument, 0, 0},
	[LINKTOOL_OPT_REFCLK]   = {"refclk",   required_argument, 0, 0},
	[LINKTOOL_OPT_COMCLK]   = {"comclk",   required_argument, 0, 0},
	[LINKTOOL_OPT_DIVIDER]  = {"divider",  required_argument, 0, 0},
	{"cmd",      required_argument, 0, 'c'},
	{"addr",     required_argument, 0, 'a'},
	{"file",     required_argument, 0, 'f'},
	{"debug",    required_argument, 0, 'd'},
	{"help",     no_argument,       0, 'h'},
	{0, 0, 0, 0}
};

typedef void (*linktool_dbg_func_t)();

#define LINKTOOL_DEBUG_IFACE         0x0001
#define LINKTOOL_DEBUG_SBUS          0x0002
#define LINKTOOL_DEBUG_PMI           0x0004
#define LINKTOOL_DEBUG_UC            0x0008
#define LINKTOOL_DEBUG_INFO          0x0010
#define LINKTOOL_DEBUG_LOAD          0x0020
#define LINKTOOL_DEBUG_CORE          0x0040
#define LINKTOOL_DEBUG_LANE_UP       0x0100
#define LINKTOOL_DEBUG_LANE_UP_RX    0x0200
#define LINKTOOL_DEBUG_LANE_UP_TX    0x0400
#define LINKTOOL_DEBUG_LANE_UP_MISC  0x0800
#define LINKTOOL_DEBUG_LANE_DOWN     0x1000

struct linktool_dbg {
	unsigned int        val;
	const char *const   desc;
	linktool_dbg_func_t func;
};

static const struct linktool_dbg linktool_dbg_list[] = {
	{ .val  = LINKTOOL_DEBUG_IFACE,
	  .desc = "iface",
	  .func = linktool_iface_debug_set, },
	{ .val  = LINKTOOL_DEBUG_SBUS,
	  .desc = "sbus",
	  .func = linktool_sbus_debug_set, },
	{ .val  = LINKTOOL_DEBUG_PMI,
	  .desc = "pmi",
	  .func = linktool_pmi_debug_set, },
	{ .val  = LINKTOOL_DEBUG_UC,
	  .desc = "uc",
	  .func = linktool_uc_ram_debug_set, },
	{ .val  = LINKTOOL_DEBUG_INFO,
	  .desc = "info",
	  .func = linktool_cmd_info_get_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LOAD,
	  .desc = "fw load",
	  .func = linktool_cmd_fw_load_debug_set, },
	{ .val  = LINKTOOL_DEBUG_CORE,
	  .desc = "core init",
	  .func = linktool_cmd_core_init_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LANE_UP,
	  .desc = "lane up",
	  .func = linktool_cmd_lane_up_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LANE_UP_RX,
	  .desc = "lane up rx",
	  .func = linktool_lane_up_rx_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LANE_UP_TX,
	  .desc = "lane up tx",
	  .func = linktool_lane_up_tx_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LANE_UP_MISC,
	  .desc = "lane up misc",
	  .func = linktool_lane_up_misc_debug_set, },
	{ .val  = LINKTOOL_DEBUG_LANE_DOWN,
	  .desc = "lane down",
	  .func = linktool_cmd_lane_down_debug_set, },
};
#define LINKTOOL_DBG_COUNT (sizeof(linktool_dbg_list)/sizeof(struct linktool_dbg))

void usage(char *name)
{
	unsigned int x;

	printf("v%d.%d\n", LINKTOOL_MAJOR_VER, LINKTOOL_MINOR_VER);
	printf("usage:  %s [opts]\n", basename(name));
	printf("  opts:  -c | --cmd <cmd>      : command to run\n");
	for (x = 1; x < LINKTOOL_CMD_COUNT; ++x)
		printf("                                    %-9s = %s\n",
			linktool_cmds[x].name, linktool_cmds[x].desc);
	printf("         -a | --addr <addr>    : device address\n");
	printf("         --reg <reg>           : register (default = 0x%X)\n", cmd_obj.reg);
	printf("         --data <data>         : data to write\n");
	printf("         --pll <pll>           : pll  (default = %u)\n", cmd_obj.pll);
	printf("         --lane <lane>         : lane (default = %u)\n", cmd_obj.lane);
	printf("         -f | --file <file>    : firmware file\n");
	printf("         --refclk <speed>      : reference clock speed (default = %u)\n", cmd_obj.clock_info.refclk);
	printf("         --comclk <speed>      : common clock speed    (default = %u)\n", cmd_obj.clock_info.comclk);
	printf("         --divider <num>       : clock divider         (default = %u)\n", cmd_obj.clock_info.divider);
	printf("         --lane_map            : bit map of lanes      (default = 0x%X)\n", cmd_obj.link_config.lane_map);
	printf("         --pre3 <num>          : pre3   (default = %u)\n", cmd_obj.link_config.pre3);
	printf("         --pre2 <num>          : pre2   (default = %u)\n", cmd_obj.link_config.pre2);
	printf("         --pre1 <num>          : pre1   (default = %u)\n", cmd_obj.link_config.pre1);
	printf("         --cursor <num>        : cursor (default = %u)\n", cmd_obj.link_config.cursor);
	printf("         --post1 <num>         : post1  (default = %u)\n", cmd_obj.link_config.post1);
	printf("         --post2 <num>         : post2  (default = %u)\n", cmd_obj.link_config.post2);
	printf("         --width <width>       : width mode (40 | 80 | 160) (default = %u)\n", cmd_obj.link_config.width);
	printf("         --osr <num>           : OSR mode (default = %u)\n", cmd_obj.link_config.osr);
	printf("                                   OSX1    = 1\n");
	printf("                                   OSX2    = 2\n");
	printf("                                   OSX42P5 = 4\n");
	printf("         --encoding <num>      : lane encoding (default = %u)\n", cmd_obj.link_config.encoding);
	printf("                                   NRZ  = 1\n");
	printf("                                   PAM4 = 4\n");
	printf("         --media <num>         : media type (default = %u)\n", cmd_obj.link_config.media);
	printf("                                   backplane  = 0\n");
	printf("                                   copper     = 1\n");
	printf("                                   optical    = 2\n");
	printf("         -d | --debug <mask>   : mask to enable debug messages\n");
	for (x = 0; x < LINKTOOL_DBG_COUNT; ++x)
		printf("                                   0x%04X = turn on %s\n",
			linktool_dbg_list[x].val, linktool_dbg_list[x].desc);
	printf("         -h | --help           : print this usage\n");
}

int main(int argc, char *argv[])
{
	int                     opt;
	int                     idx;
	int                     cmd;
	unsigned int            x;
	unsigned int            val;

	memset(&cmd_obj, 0, sizeof(cmd_obj));

	/* set defaults */
	cmd_obj.clock_info.refclk  = 312500000;
	cmd_obj.clock_info.comclk  = 156250000;
	cmd_obj.clock_info.divider = 170;

	// from AAPL - SPEED
	//    200) OPTS_SPEED=(-osr osx2    -width-mode 80  -encoding 4 -tx-eq-num-taps 6)
	//    400) OPTS_SPEED=(-osr osx1    -width-mode 160 -encoding 4 -tx-eq-num-taps 6)
	//    an)  OPTS_SPEED=(-osr osx42p5 -width-mode 40  -encoding 2 -tx-eq-num-taps 6 -scrambling-dis 1)

	// from AAPL - CABLE
	//    1) OPTS_CABLE=(-elb -media-type 0 -pre 0 -pre2 0 -pre3 0 -cursor 100)
	//    4) OPTS_CABLE=()
	//    5) OPTS_CABLE=(-elb -media-type 1 -pre 0 -pre2 0 -pre3 0 -cursor 80)

	// Washington NIC0 200G PEC: -osr osx2    -width-mode 80  -encoding 4 -tx-eq-num-taps 6                   -elb -media-type 0 -pre 0 -pre2 0 -pre3 0 -cursor 100
	//	tx_width = 80, rx_width = 80, tx_encoding = 1, rx_encoding = 1, tx_osr = 2000, rx_osr = 2000
	//	pre3 = 0, pre2 = 0, pre1 = 0, cursor = 100, post1 = 0, post2 = 0
	// blackhawk_sa_set_tx_rx_width_pam_osr - START - tx_width = 80, rx_width = 0, tx_encoding = 1, rx_encoding = 1, tx_osr = 2000, rx_osr = 2000
	// blackhawk_sa_set_tx_rx_width_pam_osr - tx_reg_val = 0x1, rx_reg_val = 0x1

	// Washington NIC0 400G PEC: -osr osx1    -width-mode 160 -encoding 4 -tx-eq-num-taps 6                   -elb -media-type 0 -pre 0 -pre2 0 -pre3 0 -cursor 100

	// Washington NIC0 AN PEC:   -osr osx42p5 -width-mode 40  -encoding 2 -tx-eq-num-taps 6 -scrambling-dis 1 -elb -media-type 0 -pre 0 -pre2 0 -pre3 0 -cursor 100
	//	tx_width = 40, rx_width = 40, tx_encoding = 0, rx_encoding = 0, tx_osr = 42500, rx_osr = 42500
	//	pre3 = 0, pre2 = 0, pre1 = 0, cursor = 100, post1 = 0, post2 = 0
	// blackhawk_sa_set_tx_rx_width_pam_osr - tx_width = 40, rx_width = 40, tx_encoding = 0, rx_encoding = 0, tx_osr = 42500, rx_osr = 42500
	// blackhawk_sa_set_tx_rx_width_pam_osr - tx_reg_val = 0x21, rx_reg_val = 0x21

	// BCM_SERDES_NRZ  = 0
	// BCM_SERDES_PAM4 = 1

	// BCM_SERDES_OSX1      =  1000 "osx1"
	// BCM_SERDES_OSX2      =  2000 "osx2"
	// BCM_SERDES_OSX42P5   = 42500 "osx42p5"

	// enum osprey7_v2l4p1_osr_mode_enum {
	//   OSPREY7_V2L4P1_OSX1      = 0,
	//   OSPREY7_V2L4P1_OSX2      = 1,
	//   OSPREY7_V2L4P1_OSX42P5   = 33,
	// };

	// enum srds_media_type_enum {
	//    MEDIA_TYPE_PCB_TRACE_BACKPLANE = 0,
	//    MEDIA_TYPE_COPPER_CABLE        = 1,
	//    MEDIA_TYPE_OPTICS              = 2,
	//    MEDIA_TYPE_OPTICS_ZR           = 3
	//};

	/* defaults for autoneg */
	cmd_obj.link_config.lane_map        = 0xF;
	cmd_obj.link_config.pre3            = 0;
	cmd_obj.link_config.pre2            = 0;
	cmd_obj.link_config.pre1            = 0;
	cmd_obj.link_config.cursor          = 100;
	cmd_obj.link_config.post1           = 0;
	cmd_obj.link_config.post2           = 0;
	cmd_obj.link_config.width           = 40;
	cmd_obj.link_config.osr             = 4;
	cmd_obj.link_config.encoding        = 1;
	cmd_obj.link_config.media           = 0;

	cmd = 0;
	while ((opt = getopt_long(argc, argv, "c:a:f:d:h", opts, &idx)) >= 0 ) {
		switch (opt) {
		case 0:
			val = strtol(optarg, NULL, 0);
			switch (idx) {
			case LINKTOOL_OPT_REG:
				cmd_obj.reg = strtol(optarg, NULL, 0);
				break;
			case LINKTOOL_OPT_DATA:
				cmd_obj.data = strtol(optarg, NULL, 0);
				break;
			case LINKTOOL_OPT_PLL:
				cmd_obj.pll = strtol(optarg, NULL, 0);
				break;
			case LINKTOOL_OPT_LANE:
				cmd_obj.lane = strtol(optarg, NULL, 0);
				break;
			case LINKTOOL_OPT_LANE_MAP:
				if ((val < 1) || (val > 0xF)) {
					usage(argv[0]);
					return -1;
				}
				cmd_obj.link_config.lane_map = val;
				break;
			case LINKTOOL_OPT_PRE3:
				cmd_obj.link_config.pre3 = val;
				break;
			case LINKTOOL_OPT_PRE2:
				cmd_obj.link_config.pre2 = val;
				break;
			case LINKTOOL_OPT_PRE1:
				cmd_obj.link_config.pre1 = val;
				break;
			case LINKTOOL_OPT_CURSOR:
				cmd_obj.link_config.cursor = val;
				break;
			case LINKTOOL_OPT_POST1:
				cmd_obj.link_config.post1 = val;
				break;
			case LINKTOOL_OPT_POST2:
				cmd_obj.link_config.post2 = val;
				break;
			case LINKTOOL_OPT_WIDTH:
				switch (val) {
				case 40:
				case 80:
				case 160:
					cmd_obj.link_config.width = val;
					break;
				default:
					usage(argv[0]);
					return -1;
				}
				break;
			case LINKTOOL_OPT_OSR:
				switch (val) {
				case 1:
					cmd_obj.link_config.osr = 0;
					break;
				case 2:
					cmd_obj.link_config.osr = 1;
					break;
				case 4:
					cmd_obj.link_config.osr = 33;
					break;
				default:
					usage(argv[0]);
					return -1;
				}
				break;
			case LINKTOOL_OPT_ENCODING:
				switch (val) {
				case 1:
					cmd_obj.link_config.encoding = 0;
					break;
				case 4:
					cmd_obj.link_config.encoding = 1;
					break;
				default:
					usage(argv[0]);
					return -1;
				}
				break;
			case LINKTOOL_OPT_MEDIA:
				switch (val) {
				case 0:
				case 1:
				case 2:
					cmd_obj.link_config.encoding = val;
					break;
				default:
					usage(argv[0]);
					return -1;
				}
				break;
			case LINKTOOL_OPT_REFCLK:
				cmd_obj.clock_info.refclk = val;
				break;
			case LINKTOOL_OPT_COMCLK:
				cmd_obj.clock_info.comclk = val;
				break;
			case LINKTOOL_OPT_DIVIDER:
				cmd_obj.clock_info.divider = val;
				break;
			default:
				usage(argv[0]);
				return -1;
			}
			break;
		case 'c':
			for (cmd = 1; cmd < LINKTOOL_CMD_COUNT; ++cmd)
				if (strcmp(optarg, linktool_cmds[cmd].name) == 0)
					break;
			break;
		case 'a':
			cmd_obj.dev_addr = strtol(optarg, NULL, 0);
			break;
		case 'f':
			cmd_obj.filename = optarg;
			break;
		case 'd':
			linktool_debug = strtol(optarg, NULL, 0);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return -1;
		};
	};

	DEBUG(linktool_debug, "linktool (cmd = %d, dev_addr = 0x%X, reg = 0x%X, pll = %u, lane = %u)",
		cmd, cmd_obj.dev_addr, cmd_obj.reg, cmd_obj.pll, cmd_obj.lane);
	DEBUG(linktool_debug, "linktool clock info (refclk = %u, comclk = %u, divider = %u)",
		cmd_obj.clock_info.refclk, cmd_obj.clock_info.comclk, cmd_obj.clock_info.divider);
	DEBUG(linktool_debug, "linktool link config (pre1 = %u, pre2 = %u, pre3 = %u, cursor = %u, post1 = %u, post2 = %u)",
		cmd_obj.link_config.pre1, cmd_obj.link_config.pre2, cmd_obj.link_config.pre3,
		cmd_obj.link_config.cursor, cmd_obj.link_config.post1, cmd_obj.link_config.post2);
	DEBUG(linktool_debug, "linktool link config (lane_map = 0x%X)", cmd_obj.link_config.lane_map);
	DEBUG(linktool_debug, "linktool link config (width = %u, osr = %u, encoding = %u, media = %u)",
		cmd_obj.link_config.width, cmd_obj.link_config.osr, cmd_obj.link_config.encoding,
		cmd_obj.link_config.media);

	switch (cmd) {
	case LINKTOOL_CMD_SBUS_RD:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_SBUS_WR:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_PMI_RD:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_PMI_WR:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_INFO_GET:
		/* no other args required */
		break;
	case LINKTOOL_CMD_FW_LOAD:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		if (cmd_obj.filename == NULL) {
			ERROR("file required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_CORE_INIT:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		if (cmd_obj.clock_info.refclk == 0) {
			ERROR("reference clock required");
			return -1;
		}
		if (cmd_obj.clock_info.comclk == 0) {
			ERROR("common clock required");
			return -1;
		}
		break;
	case LINKTOOL_CMD_LANE_UP:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		if (cmd_obj.link_config.lane_map == 0) {
			ERROR("lane map required");
			return -1;
		}
		// FIXME: add other things here
		break;
	case LINKTOOL_CMD_LANE_DOWN:
		if (cmd_obj.dev_addr == 0) {
			ERROR("device address required");
			return -1;
		}
		if (cmd_obj.link_config.lane_map == 0) {
			ERROR("lane map required");
			return -1;
		}
		break;
	default:
		ERROR("command not found");
		usage(argv[0]);
		return -1;
	}

	if (linktool_debug != 0) {
		for (x = 0; x < LINKTOOL_DBG_COUNT; ++x)
			if (linktool_debug & linktool_dbg_list[x].val)
				linktool_dbg_list[x].func();
	}

	return linktool_cmds[cmd].func(&cmd_obj);
}
