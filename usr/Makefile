# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2023 Hewlett Packard Enterprise Development LP
#

.PHONY: default all stage_install stage_uninstall clean

MAKECMDGOALS ?= stage_install

default: all

all $(MAKECMDGOALS) :
	$(MAKE) -C linktool $(MAKECMDGOALS)
