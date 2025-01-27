# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_serdes_check {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local serdes_num=$3

	__sl_test_lgrp_check ${ldev_num} ${lgrp_num}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num})"
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	if (( ${serdes_num} < 0 || ${serdes_num} > 3 )); then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num})"
		sl_test_debug_log "${FUNCNAME}" "invalid serdes"
		return 1
	fi

	return 0
}

function __sl_test_serdes_cmd {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local serdes_num=$3
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

	echo ${serdes_num} > ${SL_TEST_SERDES_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_set failed [${rtn}]"
		return ${rtn}
	fi

	echo "${cmd_str}" > ${SL_TEST_SERDES_DEBUGFS_CMD}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_serdes_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local serdes_num
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num serdes_num cmd_str"
	local description=$(cat <<-EOF
	Send a command to the SerDes.

	Mandatory:
	ldev_num     Link device number the lgrp_num belongs to.
	lgrp_num     Link group number the serdes_num belongs to.
	serdes_num   Serdes number to send the command to.
	cmd_str      Command to send to the serdes_num.

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

	__sl_test_serdes_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "serdes" $4
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	serdes_num=$3
	cmd_str=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num}, cmd_str = ${cmd_str})"

	__sl_test_serdes_cmd ${ldev_num} ${lgrp_num} ${serdes_num} ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_serdes_settings_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local serdes_num
	local settings
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num serdes_num settings"
	local description=$(cat <<-EOF
	Set SerDes parameters.

	Mandatory:
	ldev_num    Link device number the <lgrp_num> belongs to.
	lgrp_num    Link group number the <serdes_num> belongs to.
	serdes_num  Serdes number to apply parameters to.
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

	__sl_test_serdes_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "$4" ]; then
		sl_test_error_log "${FUNCNAME}" "missing settings"
		echo "${usage}"
		return 1
	fi

	ldev_num=$1
	lgrp_num=$2
	serdes_num=$3
	settings=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num}, settings = ${settings})"

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

	__sl_test_serdes_cmd ${ldev_num} ${lgrp_num} ${serdes_num} "serdes_params_set"
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
	local lgrp_num
	local serdes_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num serdes_num"
	local description=$(cat <<-EOF
	Unset SerDes parameters.

	Mandatory:
	ldev_num   Link device number the lgrp_num belongs to.
	lgrp_num   Link group number the serdes_num belongs to.
	serdes_num Serdes number to unset parameters on.

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

	__sl_test_serdes_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	serdes_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, serdes_num = ${serdes_num})"

	__sl_test_serdes_cmd ${ldev_num} ${lgrp_num} ${serdes_num} "serdes_params_unset"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
