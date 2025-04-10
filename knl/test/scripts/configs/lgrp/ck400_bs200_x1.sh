# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

fabric_link=0
fec_map=$((1 << 0))                 # SL_LGRP_CONFIG_FEC_RS
fec_mode=$((1 << 2))                # SL_LGRP_FEC_MODE_CORRECT
furcation=0x1                       # SL_MEDIA_FURCATION_X1
lock=0
mfs=9216
r1_partner=0
serdes_loopback=0
tech_map=$(((1 << 18) | (1 << 15))) # SL_LGRP_CONFIG_TECH_CK_400G | SL_LGRP_CONFIG_TECH_BS_200G
