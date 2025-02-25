# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_serdes_check {
	local rtn
	local ldev_num=$1
	local -n serdes_lgrp_nums=$2
	local -n serdes_nums=$3
	local serdes_num

	__sl_test_lgrp_check ${ldev_num} serdes_lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	for serdes_num in "${serdes_nums[@]}"; do
		if (( ${serdes_num} < 0 || ${serdes_num} > 3 )); then
			sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, serdes_num = ${serdes_num})"
			sl_test_debug_log "${FUNCNAME}" "invalid serdes"
			return 1
		fi
	done

	return 0
}

function __sl_test_serdes_cmd {
	local rtn
	local ldev_num=$1
	local -n cmd_lgrp_nums=$2
	local -n cmd_serdes_nums=$3
        local lgrp_num
	local serdes_num
	local cmd_str=$4

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_set failed (ldev_num = ${ldev_num}) [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${cmd_lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}) [${rtn}]"
			return ${rtn}
		fi

		for serdes_num in "${cmd_serdes_nums[@]}"; do
			echo ${serdes_num} > ${SL_TEST_SERDES_DEBUGFS_NUM}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "serdes_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num}) [${rtn}]"
				return ${rtn}
			fi

			echo "${cmd_str}" > ${SL_TEST_SERDES_DEBUGFS_CMD}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "cmd failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num}, cmd_str = ${cmd_str}) [${rtn}]"
				return ${rtn}
			fi
		done
	done

	return 0
}

function sl_test_serdes_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local serdes_nums
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums serdes_nums cmd_str"
	local description=$(cat <<-EOF
	Send a command to the SerDes in the link groups.

	Mandatory:
	ldev_num     Link device number the lgrp_nums belongs to.
	lgrp_nums    Link group numbers the serdes_nums belongs to.
	serdes_nums  Serdes numbers to send the command to.
	cmd_str      Command to send to the serdes_nums.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "serdes")
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
		echo ""
		sl_test_cmd_help "serdes"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	serdes_nums=($3)
	cmd_str=$4
	__sl_test_serdes_check ${ldev_num} lgrp_nums serdes_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "serdes" ${cmd_str}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), serdes_nums = (${serdes_nums[*]}), cmd_str = ${cmd_str})"

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums serdes_nums ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_serdes_settings_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local serdes_nums
	local settings
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums serdes_nums settings"
	local description=$(cat <<-EOF
	Set SerDes parameters for the SerDes in link groups.

	Mandatory:
	ldev_num    Link device number the lgrp_nums belongs to.
	lgrp_nums   Link group number the serdes_nums belongs to.
	serdes_nums Serdes numbers to apply parameters to.
	settings    Serdes parameters file. See ${SL_TEST_SERDES_SETTINGS_DIR}

	Options:
	-h, --help  This message.

	Settings:
	$(find ${SL_TEST_SERDES_SETTINGS_DIR} -type f)
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
	serdes_nums=($3)
	settings=$4
	__sl_test_serdes_check ${ldev_num} lgrp_nums serdes_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "${settings}" ]; then
		sl_test_error_log "${FUNCNAME}" "missing settings"
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), serdes_nums = (${serdes_nums[*]}), settings = ${settings})"

	source ${settings}

	for filename in ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "settings missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "serdes_params failed"
			return 1
		fi

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "serdes_params ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums serdes_nums "serdes_params_set"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_serdes_settings_unset {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local serdes_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums serdes_nums"
	local description=$(cat <<-EOF
	Unset SerDes parameters for SerDes in the link groups.

	Mandatory:
	ldev_num    Link device number the lgrp_nums belongs to.
	lgrp_nums   Link group numbers the serdes_nums belongs to.
	serdes_nums Serdes numbers to unset parameters on.

	Options:
	-h, --help  This message.
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
	serdes_nums=($3)
	__sl_test_serdes_check ${ldev_num} lgrp_nums serdes_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), serdes_nums = (${serdes_nums[*]}))"

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums serdes_nums "serdes_params_unset"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
