#!/bin/bash

set -e

LGRP_NUM=24
DYNAMIC_DEBUG_CONTROL="/sys/kernel/debug/dynamic_debug/control"
DEBUGFS_DIR="/sys/kernel/debug/sl"
SYSFS_DIR="/sys/class/rossw/rossw0/pgrp/${LGRP_NUM}"
DEBUGFS_CMD="${DEBUGFS_DIR}/cmd"

# set the nums
echo 0           > ${DEBUGFS_DIR}/ldev/num
echo ${LGRP_NUM} > ${DEBUGFS_DIR}/lgrp/num
echo 0           > ${DEBUGFS_DIR}/link/num

# bring the link down
echo "down > ${DEBUGFS_CMD}"

# set the params to something that won't work
echo "1" > ${DEBUGFS_DIR}/serdes/pre1
echo "2" > ${DEBUGFS_DIR}/serdes/pre2
echo "3" > ${DEBUGFS_DIR}/serdes/pre3
echo "4" > ${DEBUGFS_DIR}/serdes/cursor
echo "5" > ${DEBUGFS_DIR}/serdes/post1
echo "6" > ${DEBUGFS_DIR}/serdes/post2
echo "serdes_params_set" > ${DEBUGFS_CMD}

# bring the link up
echo "up > ${DEBUGFS_CMD}"

# link state -> expected to not be up
"Link state = $(cat ${SYSFS_DIR}/0/link/state)"

# unset the params
echo "serdes_params_unset" > ${DEBUGFS_CMD}

# bring the link up
echo "up > ${DEBUGFS_CMD}"

# link state -> expected to be up
"Link state = $(cat ${SYSFS_DIR}/0/link/state)"
