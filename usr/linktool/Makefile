# SPDX-License-Identifier: GPL-2.0
# Copyright 2023,2024 Hewlett Packard Enterprise Development LP

include ../../config.mak

BINNAME  := linktool
CC        = $(CROSS_COMPILE)gcc
CFLAGS   += -D_FILE_OFFSET_BITS=64
CFLAGS   += -Wall -Wextra -Werror
CFLAGS   += -I$(SRCTOP)/../slingshot_base_link
CFLAGS   += -I$(SRCTOP)/../hms-sbl-iface/lib/sbliface
CFLAGS   += -I$(SRCTOP)/../libcxi/install/include
CFLAGS   += -I$(SRCTOP)/../cassini-headers/install/include
CFLAGS   += -I$(SRCTOP)/../cxi-driver/include/
CFLAGS   += -DSBL_EXTERNAL_BUILD=1
CFLAGS   += -DSBL_PLATFORM_CAS_HW=1
CFLAGS   += -L$(SRCTOP)/../hms-sbl-iface/lib/sbliface/obj
CFLAGS   += -L$(SRCTOP)/../libcxi/install/lib
CFLAGS   += -lsbliface -lcxi
## FIXME: need rpath?
LIBS     += 
ifeq ($(DEBUG),1)
	CFLAGS += -g -O1
else
	CFLAGS += -O3
endif

SRCS  = linktool.c
SRCS += linktool_addr.c
SRCS += linktool_sbus.c
SRCS += linktool_pmi.c
SRCS += linktool_uc_ram.c
SRCS += linktool_cmd_sbus.c
SRCS += linktool_cmd_pmi.c
SRCS += linktool_lane_up_misc.c
SRCS += linktool_lane_up_tx.c
SRCS += linktool_lane_up_rx.c
SRCS += linktool_cmd_lane_up.c
SRCS += linktool_cmd_lane_down.c
SRCS += linktool_cmd_info_get.c
SRCS += linktool_cmd_fw_load.c
SRCS += linktool_cmd_core_init.c
SRCS += linktool_iface.c

OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.c,%.dep,$(SRCS))

default: all

include ../../common/buildsys/common_exe.mak
include ../../common/buildsys/common.mak

-include $(DEPS)
