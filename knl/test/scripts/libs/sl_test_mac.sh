# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_mac_check {
	local rtn
	local ldev_num=$1
	local -n mac_lgrp_nums=$2
	local -n check_mac_nums=$3
	local mac_num

	__sl_test_lgrp_check ${ldev_num} mac_lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	for mac_num in "${check_mac_nums[@]}"; do
		if (( ${mac_num} < 0 || ${mac_num} > 3 )); then
			sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, mac_num = ${mac_num})"
			sl_test_debug_log "${FUNCNAME}" "invalid mac"
			return 1
		fi
	done

	return 0
}

function __sl_test_mac_cmd {
	local rtn
	local ldev_num=$1
	local -n cmd_lgrp_nums=$2
	local -n cmd_mac_nums=$3
	local cmd_str=$4
	local lgrp_num
	local mac_num

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_set failed (ldev_num = ${ldev_num}) [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${cmd_lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}) [${rtn}]"
			return ${rtn}
		fi

		for mac_num in "${cmd_mac_nums[@]}"; do
			echo ${mac_num} > ${SL_TEST_MAC_DEBUGFS_NUM}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "mac_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num}) [${rtn}]"
				return ${rtn}
			fi

			__sl_test_write_cmd "${cmd_str}" "${SL_TEST_MAC_DEBUGFS_CMD}"
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "cmd failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num}, cmd_str = ${cmd_str}) [${rtn}]"
				return ${rtn}
			fi
		done
	done

	return 0
}

function sl_test_mac_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local mac_nums
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums mac_nums cmd_str"
	local description=$(cat <<-EOF
	Send a command to the MACs in the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the mac_nums belongs to.
	mac_nums   MAC Numbers to send the command to.
	cmd_str    Command to send to the mac.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "mac")
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
				;;
			-- )
				shift
				break
				;;
			* )
				break
				;;
		esac
	done

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	mac_nums=($3)
	cmd_str=$4
	__sl_test_mac_check ${ldev_num} lgrp_nums mac_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "mac" ${cmd_str}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), mac_nums = (${mac_nums[*]}), cmd_str = ${cmd_str})"

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_mac_new {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local mac_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums mac_nums"
	local description=$(cat <<-EOF
	Create new MACs for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the mac_nums belongs to.
	mac_nums   MAC Numbers to create.

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
				;;
			-- )
				shift
				break
				;;
			* )
				break
				;;
		esac
	done

	if [[ "$#" != 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	mac_nums=($3)
	__sl_test_mac_check ${ldev_num} lgrp_nums mac_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), mac_nums = (${mac_nums[*]}))"

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_new"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_new failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_mac_del {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local mac_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums mac_nums"
	local description=$(cat <<-EOF
	Create new MACs for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the mac_nums belongs to.
	mac_nums   MAC Numbers to delete.

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
				;;
			-- )
				shift
				break
				;;
			* )
				break
				;;
		esac
	done

	if [[ "$#" != 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	mac_nums=($3)
	__sl_test_mac_check ${ldev_num} lgrp_nums mac_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), mac_nums = (${mac_nums[*]}))"

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_mac_start {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local mac_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums mac_nums"
	local description=$(cat <<-EOF
	Start the MACs for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the mac_nums belongs to.
	mac_nums   MAC Numbers to start.

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
				;;
			-- )
				shift
				break
				;;
			* )
				break
				;;
		esac
	done

	if [[ "$#" != 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	mac_nums=($3)
	__sl_test_mac_check ${ldev_num} lgrp_nums mac_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), mac_nums = (${mac_nums[*]}))"

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_tx_start"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_tx_start failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_rx_start"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_rx_start failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_mac_stop {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local mac_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums mac_nums"
	local description=$(cat <<-EOF
	Stop the MACs for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the mac_nums belongs to.
	mac_nums   MAC Numbers to stop.

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
				;;
			-- )
				shift
				break
				;;
			* )
				break
				;;
		esac
	done

	if [[ "$#" != 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	mac_nums=($3)
	__sl_test_mac_check ${ldev_num} lgrp_nums mac_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), mac_nums = (${mac_nums[*]}))"

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_tx_stop"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_tx_stop failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_mac_cmd ${ldev_num} lgrp_nums mac_nums "mac_rx_stop"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_rx_stop failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
