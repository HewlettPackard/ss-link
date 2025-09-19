# SPDX-License-Identifier: GPL-2.0
# Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP

.PHONY: default all

MAKECMDGOALS ?= all

default: all

all $(MAKECMDGOALS) :
	$(MAKE) -C sl_test_lgrp_notifs_read $(MAKECMDGOALS)
	$(MAKE) -C sl_test_file_write       $(MAKECMDGOALS)
