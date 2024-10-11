#!/bin/bash
# This is an example for setting the FEC configuration. Please see the
# numbered steps below.

set -e

DEBUGFS_DIR="/sys/kernel/debug/sl"

# This Directory will only exist on C2. R2 does not support SYSFS.
SYSFS_PORT_DIR="/sys/class/cxi/cxi0/device/port/0/"

# link device, link group, and link files
DEBUGFS_LDEV_NUM="${DEBUGFS_DIR}/ldev/num"
DEBUGFS_LGRP_NUM="${DEBUGFS_DIR}/lgrp/num"
DEBUGFS_LINK_NUM="${DEBUGFS_DIR}/link/num"

# Cmd File
DEBUGFS_CMD="${DEBUGFS_DIR}/cmd"

# Debugfs Config files
DEBUGFS_LINK_CONFIG_DIR="${DEBUGFS_DIR}/link/config/"

DEBUGFS_FEC_UP_CCW_THRESH="${DEBUGFS_LINK_CONFIG_DIR}/fec_up_ccw_limit"
DEBUGFS_FEC_UP_CHECK_WIN="${DEBUGFS_LINK_CONFIG_DIR}/fec_up_check_wait_ms"
DEBUGFS_FEC_UP_SETTLE_WIN="${DEBUGFS_LINK_CONFIG_DIR}/fec_up_settle_wait_ms"
DEBUGFS_FEC_UP_UCW_THRESH="${DEBUGFS_LINK_CONFIG_DIR}/fec_up_ucw_limit"
DEBUGFS_LANE_MAP="${DEBUGFS_LINK_CONFIG_DIR}/lane_map"
DEBUGFS_LINK_UP_TIMEOUT_MS="${DEBUGFS_LINK_CONFIG_DIR}/link_up_timeout_ms"
DEBUGFS_LINK_UP_TRIES_MAX="${DEBUGFS_LINK_CONFIG_DIR}/link_up_tries_max"
DEBUGFS_CONFIG_LOCKED="${DEBUGFS_LINK_CONFIG_DIR}/lock"
DEBUGFS_AUTONEG="${DEBUGFS_LINK_CONFIG_DIR}/autoneg"
DEBUGFS_HEADSHELL_LOOPBACK="${DEBUGFS_LINK_CONFIG_DIR}/headshell_loopback"
DEBUGFS_PCAL="${DEBUGFS_LINK_CONFIG_DIR}/pcal"
DEBUGFS_PRECODING="${DEBUGFS_LINK_CONFIG_DIR}/precoding"
DEBUGFS_REMOTE_LOOPBACK="${DEBUGFS_LINK_CONFIG_DIR}/remote_loopback"
DEBUGFS_SERDES_LOOPBACK="${DEBUGFS_LINK_CONFIG_DIR}/serdes_loopback"

# Sysfs Config Files
SYSFS_LINK_CONFIG_DIR="${SYSFS_PORT_DIR}/link/config/"

SYSFS_FEC_UP_CCW_THRESH="${SYSFS_LINK_CONFIG_DIR}/fec_up_ccw_limit"
SYSFS_FEC_UP_CHECK_WIN="${SYSFS_LINK_CONFIG_DIR}/fec_up_check_wait_ms"
SYSFS_FEC_UP_SETTLE_WIN="${SYSFS_LINK_CONFIG_DIR}/fec_up_settle_wait_ms"
SYSFS_FEC_UP_UCW_THRESH="${SYSFS_LINK_CONFIG_DIR}/fec_up_ucw_limit"
SYSFS_CONFIG_LOCKED="${SYSFS_LINK_CONFIG_DIR}/lock"
SYSFS_AUTONEG="${SYSFS_LINK_CONFIG_DIR}/autoneg"
SYSFS_HEADSHELL_LOOPBACK="${SYSFS_LINK_CONFIG_DIR}/headshell_loopback"
SYSFS_PCAL="${SYSFS_LINK_CONFIG_DIR}/pcal"
SYSFS_PRECODING="${SYSFS_LINK_CONFIG_DIR}/precoding"
SYSFS_REMOTE_LOOPBACK="${SYSFS_LINK_CONFIG_DIR}/remote_loopback"
SYSFS_SERDES_LOOPBACK="${SYSFS_LINK_CONFIG_DIR}/serdes_loopback"

LDEV_NUM="0"
LGRP_NUM="0"
LINK_NUM="0"

for i in "$@"; do
	case $i in
	-d=* | --ldev_num=*)
		LDEV_NUM="${i#*=}"
		shift # past argument=value
		;;
	-g=* | --lgrp_num=*)
		LGRP_NUM="${i#*=}"
		shift # past argument=value
		;;
	-l=* | --link_num=*)
		LINK_NUM="${i#*=}"
		shift # past argument=value
		;;
	-* | --*)
		echo "Unknown option $i"
		exit 1
		;;
	*) ;;
	esac
done

function print_fec_sysfs {
	if [ -e "${SYSFS_PORT_DIR}" ]; then
		echo "FEC Up CCW Threshold: $(cat ${SYSFS_FEC_UP_CCW_THRESH})"
		echo "FEC Up Check Window: $(cat ${SYSFS_FEC_UP_CHECK_WIN}) ms"
		echo "FEC Up Settle Window: $(cat ${SYSFS_FEC_UP_SETTLE_WIN}) ms"
		echo "FEC Up UCW Threshold: $(cat ${SYSFS_FEC_UP_UCW_THRESH})"
                echo "lock: $(cat ${SYSFS_CONFIG_LOCKED})"
                echo "autoneg: $(cat ${SYSFS_AUTONEG})"
                echo "pcal: $(cat ${SYSFS_PCAL})"
                echo "precoding: $(cat ${SYSFS_PRECODING})"
                echo "remote_loopback: $(cat ${SYSFS_REMOTE_LOOPBACK})"
                echo "serdes_loopback: $(cat ${SYSFS_SERDES_LOOPBACK})"
                echo "headshell_loopback: $(cat ${SYSFS_HEADSHELL_LOOPBACK})"
	else
		echo "Error: Current config cannot be read. Missing ${SYSFS_PORT_DIR}"
	fi
}

function print_debugfs_config {
	echo "FEC Up CCW Thresh: $(cat ${DEBUGFS_FEC_UP_CCW_THRESH})"
	echo "FEC Up Check Window: $(cat ${DEBUGFS_FEC_UP_CHECK_WIN}) ms"
	echo "FEC UP Settle Window: $(cat ${DEBUGFS_FEC_UP_SETTLE_WIN}) ms"
	echo "Fec Up UCW Thresh: $(cat ${DEBUGFS_FEC_UP_UCW_THRESH})"
	echo "Lane Map: $(cat ${DEBUGFS_LANE_MAP})"
	echo "Link Up Timeout: $(cat ${DEBUGFS_LINK_UP_TIMEOUT_MS}) ms"
	echo "Link Up Max Tries: $(cat ${DEBUGFS_LINK_UP_TRIES_MAX})"
        echo "lock: $(cat ${DEBUGFS_CONFIG_LOCKED})"
        echo "autoneg: $(cat ${DEBUGFS_AUTONEG})"
        echo "pcal: $(cat ${DEBUGFS_PCAL})"
        echo "precoding: $(cat ${DEBUGFS_PRECODING})"
        echo "remote_loopback: $(cat ${DEBUGFS_REMOTE_LOOPBACK})"
        echo "serdes_loopback: $(cat ${DEBUGFS_SERDES_LOOPBACK})"
        echo "headshell_loopback: $(cat ${DEBUGFS_HEADSHELL_LOOPBACK})"
}

function print_debugfs_position {
	echo "LDEV_NUM = $(cat ${DEBUGFS_LDEV_NUM})"
	echo "LGRP_NUM = $(cat ${DEBUGFS_LGRP_NUM})"
	echo "LINK_NUM = $(cat ${DEBUGFS_LINK_NUM})"
}

echo "User Input:"
echo "LDEV_NUM = ${LDEV_NUM}"
echo "LGRP_NUM = ${LGRP_NUM}"
echo "LINK_NUM = ${LINK_NUM}"
echo "---"

##############################################################################
# (1) Set the link device, link group and link you want debugfs to apply
# changes to.
##############################################################################
echo "(1) Set the link device, link group, and link"
echo ${LDEV_NUM} >${DEBUGFS_LDEV_NUM}
echo ${LGRP_NUM} >${DEBUGFS_LGRP_NUM}
echo ${LINK_NUM} >${DEBUGFS_LINK_NUM}

print_debugfs_position

##############################################################################
# (2) Print the config currently running.
##############################################################################
echo "---"
echo "(2) Print the config currently running"

print_fec_sysfs

##############################################################################
# (3) Set the link down.
##############################################################################
echo "---"
echo "(3) Set the link down"

echo "down" >${DEBUGFS_CMD}
echo "Return Code: $?"

##############################################################################
# (4) Create the new config to debugfs. This will not be written until
# commanded.
##############################################################################
echo "---"
echo "(4) Create new config"
echo 18000 >${DEBUGFS_LINK_UP_TIMEOUT_MS}
echo 0 >${DEBUGFS_LINK_UP_TRIES_MAX}
echo 5000000 >${DEBUGFS_FEC_UP_CCW_THRESH}
echo 21 >${DEBUGFS_FEC_UP_UCW_THRESH}
echo 500 >${DEBUGFS_FEC_UP_SETTLE_WIN}
echo 2000 >${DEBUGFS_FEC_UP_CHECK_WIN}
echo 1 >${DEBUGFS_CONFIG_LOCKED}
echo 1 >${DEBUGFS_AUTONEG}
echo 0 >${DEBUGFS_PCAL}
echo 0 >${DEBUGFS_PRECODING}
echo 0 >${DEBUGFS_REMOTE_LOOPBACK}
echo 0 >${DEBUGFS_SERDES_LOOPBACK}
echo 0 >${DEBUGFS_HEADSHELL_LOOPBACK}

##############################################################################
# (5) Print the config that will be written.
##############################################################################
echo "---"
echo "(5) Print the new config before it is written"

print_debugfs_config

##############################################################################
# (6) Command debugfs to write the config out.
##############################################################################
echo "---"
echo "(6) Write out the config"

echo "write_link_config" >${DEBUGFS_CMD}
echo "Return Code: $?"

##############################################################################
# (7) Set the link back up
##############################################################################
echo "---"
echo "(7) Set the link up"
echo "up" >${DEBUGFS_CMD}
echo "Return Code: $?"

##############################################################################
# (8) Print the config currently running.
##############################################################################
echo "---"
echo "(8) Print the config currently running"

print_fec_sysfs
