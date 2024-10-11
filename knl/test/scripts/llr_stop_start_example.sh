#!/bin/bash
# This is an example for starting/stopping LLR. Please see the numbered steps below.

set -e

DYNAMIC_DEBUG_CONTROL="/sys/kernel/debug/dynamic_debug/control"

DEBUGFS_DIR="/sys/kernel/debug/sl"

# Cmd File
DEBUGFS_CMD="${DEBUGFS_DIR}/cmd"

# link device, link group, and link files
DEBUGFS_LDEV_NUM="${DEBUGFS_DIR}/ldev/num"
DEBUGFS_LGRP_NUM="${DEBUGFS_DIR}/lgrp/num"
DEBUGFS_LINK_NUM="${DEBUGFS_DIR}/link/num"
DEBUGFS_LLR_NUM="${DEBUGFS_DIR}/llr/num"

LDEV_NUM="0"
LGRP_NUM="0"
LINK_NUM="0"
LLR_NUM="0"

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
	-r=* | --llr_num=*)
		LLR_NUM="${i#*=}"
		shift # past argument=value
		;;
	-* | --*)
		echo "Unknown option $i"
		exit 1
		;;
	*) ;;
	esac
done

function print_debugfs_position {
	echo "LDEV_NUM = $(cat ${DEBUGFS_LDEV_NUM})"
	echo "LGRP_NUM = $(cat ${DEBUGFS_LGRP_NUM})"
	echo "LINK_NUM = $(cat ${DEBUGFS_LINK_NUM})"
	echo "LLR_NUM = $(cat ${DEBUGFS_LLR_NUM})"
}

echo "User Input:"
echo "LDEV_NUM = ${LDEV_NUM}"
echo "LGRP_NUM = ${LGRP_NUM}"
echo "LINK_NUM = ${LINK_NUM}"
echo "LLR_NUM = ${LLR_NUM}"
echo "---"

##############################################################################
# (1) Set the link device, link group and link you want debugfs to apply
# changes to.
##############################################################################
echo "(1) Set the link device, link group, link, and LLR number"
echo ${LDEV_NUM} >${DEBUGFS_LDEV_NUM}
echo ${LGRP_NUM} >${DEBUGFS_LGRP_NUM}
echo ${LINK_NUM} >${DEBUGFS_LINK_NUM}
echo ${LLR_NUM} >${DEBUGFS_LLR_NUM}

print_debugfs_position

################################################################################
# (2) Enable debug print statements in dmesg to see when llr has stopped and
# started.
################################################################################
echo "(2) Enable dmesg debug level messages for llr start/stop"

echo 8 > /proc/sys/kernel/printk

echo "func sl_ctl_llr_start +p" > ${DYNAMIC_DEBUG_CONTROL}
echo "func sl_ctl_llr_stop +p" > ${DYNAMIC_DEBUG_CONTROL}

################################################################################
# (3) Stop LLR
################################################################################
echo "(3) Stop LLR"

echo "llr_stop" > ${DEBUGFS_CMD}

################################################################################
# (4) Wait 1 second
################################################################################
echo "(4) Wait 1 second"

sleep 1

################################################################################
# (5) Start LLR
################################################################################
echo "(5) Start LLR"
echo "llr_start" > ${DEBUGFS_CMD}
