# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

##########################################################################################
# SL Test Scripts Location
##########################################################################################

export SL_TEST_DIR=$(dirname "${BASH_SOURCE}")
export SL_TEST_LIBS_DIR="${SL_TEST_DIR}/libs/"

##########################################################################################
# SL Test System/Settings Directories
##########################################################################################

export SL_TEST_CONFIGS_DIR="${SL_TEST_DIR}/configs/"
export SL_TEST_POLICY_DIR="${SL_TEST_DIR}/policies/"
export SL_TEST_SERDES_SETTINGS_DIR="${SL_TEST_DIR}/settings/serdes/"
export SL_TEST_SYSTEMS_DIR="${SL_TEST_DIR}/systems/"

export SL_TEST_SYSTEMS_SETTINGS_DIR="${SL_TEST_SYSTEMS_DIR}/settings/"

SL_TEST_LGRP_CONFIG_DIR="${SL_TEST_CONFIGS_DIR}/lgrp/"
SL_TEST_LINK_CONFIG_DIR="${SL_TEST_CONFIGS_DIR}/link/"
SL_TEST_LLR_CONFIG_DIR="${SL_TEST_CONFIGS_DIR}/llr/"
SL_TEST_CONFIG_TEST_DIR="${SL_TEST_CONFIGS_DIR}/test/"

SL_TEST_LGRP_POLICY_DIR="${SL_TEST_POLICY_DIR}/lgrp/"
SL_TEST_LINK_POLICY_DIR="${SL_TEST_POLICY_DIR}/link/"
SL_TEST_LLR_POLICY_DIR="${SL_TEST_POLICY_DIR}/llr/"
SL_TEST_POLICY_TEST_DIR="${SL_TEST_POLICY_DIR}/test/"

##########################################################################################
# SL Test Debugfs Directories
##########################################################################################

SL_TEST_DEBUGFS_TOP_DIR="/sys/kernel/debug/sl"

SL_TEST_LDEV_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/ldev/"
SL_TEST_LGRP_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/lgrp/"
SL_TEST_LINK_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/link/"
SL_TEST_LLR_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/llr/"
SL_TEST_MAC_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/mac/"
SL_TEST_SERDES_DEBUGFS_DIR="${SL_TEST_DEBUGFS_TOP_DIR}/serdes/"

SL_TEST_DEBUGFS_CMD_TYPES=("ldev" "lgrp" "link" "llr" "mac" "serdes")
SL_TEST_LDEV_DEBUGFS_CMD="${SL_TEST_LDEV_DEBUGFS_DIR}/cmd"
SL_TEST_LGRP_DEBUGFS_CMD="${SL_TEST_LGRP_DEBUGFS_DIR}/cmd"
SL_TEST_LINK_DEBUGFS_CMD="${SL_TEST_LINK_DEBUGFS_DIR}/cmd"
SL_TEST_LLR_DEBUGFS_CMD="${SL_TEST_LLR_DEBUGFS_DIR}/cmd"
SL_TEST_MAC_DEBUGFS_CMD="${SL_TEST_MAC_DEBUGFS_DIR}/cmd"
SL_TEST_SERDES_DEBUGFS_CMD="${SL_TEST_SERDES_DEBUGFS_DIR}/cmd"

# Used by sl_test_lgrp_notifs_read to find the file location so must be exported
export SL_TEST_LGRP_DEBUGFS_NOTIFS="${SL_TEST_LGRP_DEBUGFS_DIR}/notifs"

SL_TEST_LDEV_DEBUGFS_NUM="${SL_TEST_LDEV_DEBUGFS_DIR}/num"
SL_TEST_LGRP_DEBUGFS_NUM="${SL_TEST_LGRP_DEBUGFS_DIR}/num"
SL_TEST_LINK_DEBUGFS_NUM="${SL_TEST_LINK_DEBUGFS_DIR}/num"
SL_TEST_LLR_DEBUGFS_NUM="${SL_TEST_LLR_DEBUGFS_DIR}/num"
SL_TEST_MAC_DEBUGFS_NUM="${SL_TEST_MAC_DEBUGFS_DIR}/num"
SL_TEST_SERDES_DEBUGFS_NUM="${SL_TEST_SERDES_DEBUGFS_DIR}/num"

SL_TEST_LGRP_DEBUGFS_CONFIG_DIR="${SL_TEST_LGRP_DEBUGFS_DIR}/config/"
SL_TEST_LGRP_DEBUGFS_POLICY_DIR="${SL_TEST_LGRP_DEBUGFS_DIR}/policies/"
SL_TEST_LINK_DEBUGFS_CONFIG_DIR="${SL_TEST_LINK_DEBUGFS_DIR}/config/"
SL_TEST_LINK_DEBUGFS_POLICY_DIR="${SL_TEST_LINK_DEBUGFS_DIR}/policies/"
SL_TEST_LLR_DEBUGFS_CONFIG_DIR="${SL_TEST_LLR_DEBUGFS_DIR}/config/"
SL_TEST_LLR_DEBUGFS_POLICY_DIR="${SL_TEST_LLR_DEBUGFS_DIR}/policies/"
SL_TEST_MAC_DEBUGFS_CONFIG_DIR="${SL_TEST_MAC_DEBUGFS_DIR}/config/"
SL_TEST_MAC_DEBUGFS_POLICY_DIR="${SL_TEST_MAC_DEBUGFS_DIR}/policies/"
SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR="${SL_TEST_SERDES_DEBUGFS_DIR}/settings/"

##########################################################################################
# SL Test functions
##########################################################################################

# Device specific functions
HPE_PCI_VENDOR_ID=1590
source "${SL_TEST_LIBS_DIR}/sl_test_common.sh"

export SL_TEST_DEVICE_TYPE=$(sl_pci_device_discover)

source "${SL_TEST_LIBS_DIR}/devices/sl_test_cassini2.sh"
source "${SL_TEST_LIBS_DIR}/devices/sl_test_rosetta2.sh"

# Common functions
source "${SL_TEST_LIBS_DIR}/sl_test_log.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_util.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_ldev.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_lgrp.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_link.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_llr.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_mac.sh"
source "${SL_TEST_LIBS_DIR}/sl_test_serdes.sh"

##########################################################################################
# SL Test sysfs Directories
##########################################################################################

SL_TEST_SYSFS_CXI_TOP_DIR="/sys/class/cxi/"
SL_TEST_SYSFS_ROSSW_TOP_DIR="/sys/class/rossw/"

if [[ "${SL_TEST_DEVICE_TYPE}" == "cassini2" ]]; then
        SL_TEST_SYSFS_TOP_DIR="${SL_TEST_SYSFS_CXI_TOP_DIR}"
elif [[ "${SL_TEST_DEVICE_TYPE}" == "rosetta2" ]]; then
        SL_TEST_SYSFS_TOP_DIR="${SL_TEST_SYSFS_ROSSW_TOP_DIR}"
else
        SL_TEST_SYSFS_TOP_DIR=""
fi

export SL_TEST_SYSFS_TOP_LINK_DIR="port"

export SL_TEST_SYSFS_TOP_DIR

##########################################################################################
# SL Number of LGRPS
##########################################################################################
if [[ "${SL_TEST_DEVICE_TYPE}" == "cassini2" ]]; then
	SL_TEST_LGRP_NUM_END=0
elif [[ "${SL_TEST_DEVICE_TYPE}" == "rosetta2" ]]; then
	SL_TEST_LGRP_NUM_END=63
else
	SL_TEST_LGRP_NUM_END=0
fi

export SL_TEST_LGRP_NUM_END

##########################################################################################
# SL test logging
##########################################################################################

export SL_TEST_LOG_DIR=${SL_TEST_DIR}/logs/

##########################################################################################
# Setup execution path
##########################################################################################

export PATH=${PATH}:${SL_TEST_DIR}:${SL_TEST_DIR}/tests/

##########################################################################################
# Manifest + Test Parameters to sl_run_test.sh
##########################################################################################

export SL_TEST_DEFAULT_MANIFEST=${SL_TEST_DIR}/systems/manifests/quick.json

export SL_TEST_LDEV_NUM=${SL_TEST_LDEV_NUM:=0}

[[ -z ${SL_TEST_LGRP_NUMS[@]} ]] || SL_TEST_LGRP_NUMS=({0..63})
export SL_TEST_LGRP_NUMS
