# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_mac_check {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local mac_num=$3

	__sl_test_lgrp_check ${ldev_num} ${lgrp_num}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num})"
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	if (( ${mac_num} < 0 || ${mac_num} > 3 )); then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num})"
		sl_test_debug_log "${FUNCNAME}" "invalid mac"
		return 1
	fi

	return 0
}

function __sl_test_mac_cmd {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local mac_num=$3
	local cmd_str=$4

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_set failed [${rtn}]"
		return ${rtn}
	fi

	echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_set failed [${rtn}]"
		return ${rtn}
	fi

	echo ${mac_num} > ${SL_TEST_MAC_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_set failed [${rtn}]"
		return ${rtn}
	fi

	echo "${cmd_str}" > ${SL_TEST_MAC_DEBUGFS_CMD}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_mac_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local mac_num
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num mac_num cmd_str"
	local description=$(cat <<-EOF
	Send a command to the MAC.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the mac_num belongs to.
	mac_num    MAC Number to send the command to.
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

	__sl_test_mac_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "mac" $4
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	mac_num=$3
	cmd_str=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${llr_num}, cmd_str = ${cmd_str})"

	__sl_test_mac_cmd ${ldev_num} ${lgrp_num} ${mac_num} ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_mac_new {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local mac_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num mac_num"
	local description=$(cat <<-EOF
	Create a new MAC.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the mac_num belongs to.
	mac_num    MAC Number to create.

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

	__sl_test_mac_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	mac_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num})"

	__sl_test_mac_cmd ${ldev_num} ${lgrp_num} ${mac_num} "mac_new"
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
	local lgrp_num
	local mac_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num mac_num"
	local description=$(cat <<-EOF
	Create a new MAC.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the mac_num belongs to.
	mac_num    MAC Number to delete.

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

	__sl_test_mac_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	mac_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, mac_num = ${mac_num})"

	__sl_test_mac_cmd ${ldev_num} ${lgrp_num} ${mac_num} "mac_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
