# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

autoneg=1
extended_reach_force=0
fec_up_ccw_limit=-1
fec_up_check_wait_ms=-1
fec_up_settle_wait_ms=-1
fec_up_ucw_limit=-1
headshell_loopback=0
hpe_map=$(((1 << 18) | (1 << 16) | (1 << 9))) # SL_LINK_CONFIG_HPE_LINKTRAIN | SL_LINK_CONFIG_HPE_PCAL | SL_LINK_CONFIG_HPE_R2
lane_map=0
link_up_timeout_ms=25000
link_up_tries_max=10
lock=0
pause_map=0
remote_loopback=0
