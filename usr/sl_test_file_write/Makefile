# SPDX-License-Identifier: GPL-2.0
# Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP

include ../../config.mak

BINNAME  := sl_test_file_write
CC        = $(CROSS_COMPILE)gcc
CFLAGS   += -Wall -Wextra -Werror

ifeq ($(DEBUG),1)
	CFLAGS += -g -O1
else
	CFLAGS += -O3
endif

SRCS  = $(BINNAME).c

OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.c,%.dep,$(SRCS))

default test: all

include ../../common/buildsys/common_exe.mak
include ../../common/buildsys/common.mak

-include $(DEPS)
