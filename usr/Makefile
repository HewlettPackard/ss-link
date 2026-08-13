# SPDX-License-Identifier: GPL-2.0
# Copyright 2023-2026 Hewlett Packard Enterprise Development LP

.PHONY: default all fw fw_clean cable_db cable_db_clean

MAKECMDGOALS ?= all

default: all

fw_clean:

all $(MAKECMDGOALS) :
	$(MAKE) -C sl_test_lgrp_notifs_read $(MAKECMDGOALS)
	$(MAKE) -C sl_test_file_write       $(MAKECMDGOALS)
