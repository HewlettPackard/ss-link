#!/bin/bash
# This is an example for setting the FEC policy. Please see the numbered steps below.

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

# Debugfs Policy files
DEBUGFS_LINK_POLICY_DIR="${DEBUGFS_DIR}/link/policy/"

DEBUGFS_CCW_DOWN_THRESH="${DEBUGFS_LINK_POLICY_DIR}/fec_mon_ccw_down_limit"
DEBUGFS_CCW_WARN_THRESH="${DEBUGFS_LINK_POLICY_DIR}/fec_mon_ccw_warn_limit"
DEBUGFS_UCW_DOWN_THRESH="${DEBUGFS_LINK_POLICY_DIR}/fec_mon_ucw_down_limit"
DEBUGFS_UCW_WARN_THRESH="${DEBUGFS_LINK_POLICY_DIR}/fec_mon_ucw_warn_limit"
DEBUGFS_FEC_MON_PERIOD="${DEBUGFS_LINK_POLICY_DIR}/fec_mon_period_ms"
DEBUGFS_POLICY_LOCK="${DEBUGFS_LINK_POLICY_DIR}/lock"
DEBUGFS_KEEP_SERDES_UP="${DEBUGFS_LINK_POLICY_DIR}/keep_serdes_up"

# Sysfs Policy Files
SYSFS_LINK_POLICY_DIR="${SYSFS_PORT_DIR}/link/policy"
SYSFS_CCW_DOWN_THRESH="${SYSFS_LINK_POLICY_DIR}/fec_mon_ccw_down_limit"
SYSFS_CCW_WARN_THRESH="${SYSFS_LINK_POLICY_DIR}/fec_mon_ccw_warn_limit"
SYSFS_UCW_DOWN_THRESH="${SYSFS_LINK_POLICY_DIR}/fec_mon_ucw_down_limit"
SYSFS_UCW_WARN_THRESH="${SYSFS_LINK_POLICY_DIR}/fec_mon_ucw_warn_limit"
SYSFS_FEC_MON_PERIOD="${SYSFS_LINK_POLICY_DIR}/fec_mon_period_ms"
SYSFS_POLICY_LOCK="${SYSFS_LINK_POLICY_DIR}/lock"
SYSFS_KEEP_SERDES_UP="${SYSFS_LINK_POLICY_DIR}/keep_serdes_up"

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

function print_sysfs_fec {
	if [ -e "${SYSFS_PORT_DIR}" ]; then
		echo "CCW Down Threshold: $(cat ${SYSFS_CCW_DOWN_THRESH})"
		echo "CCW Warn Threshold: $(cat ${SYSFS_CCW_WARN_THRESH})"
		echo "UCW Down Threshold: $(cat ${SYSFS_UCW_DOWN_THRESH})"
		echo "UCW Warn Threshold: $(cat ${SYSFS_UCW_WARN_THRESH})"
		echo "FEC Monitor Period: $(cat ${SYSFS_FEC_MON_PERIOD}) ms"
                echo "lock: $(cat ${SYSFS_POLICY_LOCK})"
                echo "keep serdes up: $(cat ${SYSFS_KEEP_SERDES_UP})"
	else
		echo "Error: Current policy cannot be read. Missing ${SYSFS_PORT_DIR}"
	fi
}

function print_debugfs_fec {
	echo "CCW Down Threshold: $(cat ${DEBUGFS_CCW_DOWN_THRESH})"
	echo "CCW Warn Threshold: $(cat ${DEBUGFS_CCW_WARN_THRESH})"
	echo "UCW Down Threshold: $(cat ${DEBUGFS_UCW_DOWN_THRESH})"
	echo "UCW Warn Threshold: $(cat ${DEBUGFS_UCW_WARN_THRESH})"
	echo "FEC Monitor Period: $(cat ${DEBUGFS_FEC_MON_PERIOD}) ms"
	echo "lock: $(cat ${DEBUGFS_POLICY_LOCK})"
	echo "keep serdes up: $(cat ${DEBUGFS_KEEP_SERDES_UP})"
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
# (2) Print the policy currently running.
##############################################################################
echo "---"
echo "(2) Print the policy currently running"

print_sysfs_fec

##############################################################################
# (3) Create the new policy to debugfs. This will not be written until
# commanded.
##############################################################################
echo "---"
echo "(3) Create new policy"
echo 0 >${DEBUGFS_CCW_WARN_THRESH}
echo 0 >${DEBUGFS_CCW_DOWN_THRESH}
echo 123 >${DEBUGFS_UCW_DOWN_THRESH}
echo 55 >${DEBUGFS_UCW_WARN_THRESH}
echo 1000 >${DEBUGFS_FEC_MON_PERIOD}
echo 0 >${DEBUGFS_POLICY_LOCK}
echo 0 >${DEBUGFS_KEEP_SERDES_UP}

##############################################################################
# (4) Print the policy that will be written.
##############################################################################
echo "---"
echo "(4) Print the new policy before it is written"

print_debugfs_fec

##############################################################################
# (5) Command debugfs to write the policy out.
##############################################################################
echo "---"
echo "(5) Write out the policy"
echo "write_link_policy" >${DEBUGFS_CMD}
echo "Return Code: $?"

##############################################################################
# (6) Print the policy currently running.
##############################################################################
echo "---"
echo "(6) Print the policy currently running"

print_sysfs_fec
