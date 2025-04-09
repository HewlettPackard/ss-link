# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

fabric_link=0
fec_map=$((1 << 0))   # SL_LGRP_CONFIG_FEC_RS
fec_mode=0
furcation=0x1         # SL_MEDIA_FURCATION_X1
lock=0
mfs=1500
r1_partner=0
serdes_loopback=1
tech_map=$((1 << 15)) # SL_LGRP_CONFIG_TECH_BS_200G
