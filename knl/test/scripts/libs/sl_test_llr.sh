# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_llr_check {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local llr_num=$3


	__sl_test_lgrp_check ${ldev_num} ${lgrp_num}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num})"
		sl_test_error_log "${FUNCNAME}" \
                        "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	if (( ${llr_num} < 0 || ${llr_num} > 3 )); then
		sl_test_debug_log "${FUNCNAME}" \
                        "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num})"
		sl_test_debug_log "${FUNCNAME}" "invalid llr"
		return 1
	fi

	return 0
}

function __sl_test_llr_cmd {
	local ldev_num=$1
	local lgrp_num=$2
	local llr_num=$3
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

	echo ${llr_num} > ${SL_TEST_LLR_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_set failed [${rtn}]"
		return ${rtn}
	fi

	echo "${cmd_str}" > ${SL_TEST_LLR_DEBUGFS_CMD}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_llr_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local llr_num
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num llr_num cmd_str"
	local description=$(cat <<-EOF
	Send a command to the LLR.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the link_num belongs to.
	llr_num    LLR Number to send the command to.
	cmd_str    Command to send to the llr.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "llr")
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

	__sl_test_llr_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "llr" $4
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	llr_num=$3
	cmd_str=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num}, cmd_str = ${cmd_str})"

	__sl_test_llr_cmd ${ldev_num} ${lgrp_num} ${llr_num} ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_llr_new {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local llr_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num llr_num"
	local description=$(cat <<-EOF
	Create a new LLR.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the llr_num belongs to.
	llr_num    LLR Number to create.

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

	__sl_test_llr_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	llr_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num})"

	__sl_test_llr_cmd ${ldev_num} ${lgrp_num} ${llr_num} "llr_new"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_new failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_llr_del {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local llr_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num llr_num"
	local description=$(cat <<-EOF
	Delete LLR.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the llr_num belongs to.
	llr_num    LLR Number to delete.

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

	__sl_test_llr_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	lgrp_num=$2
	llr_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num})"

	__sl_test_llr_cmd ${ldev_num} ${lgrp_num} ${llr_num} "llr_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_llr_config_set {
	local rtn
	local options
	local OPTIND
	ldev_num=$1
	lgrp_num=$2
	llr_num=$3
	config=$4
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num llr_num config"
	local description=$(cat <<-EOF
	Set configuration for LLR.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the llr_num belongs to.
	llr_num    LLR Number to configure.
	config     LLR configuration. See files in ${SL_TEST_LLR_CONFIG_DIR}.

	Options:
	-h, --help This message.

	Config:
	$(find ${SL_TEST_LLR_CONFIG_DIR} -type f)
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

	__sl_test_llr_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "$4" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi

	ldev_num=$1
	lgrp_num=$2
	llr_num=$3
	config=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, llr_num = ${llr_num}, config = ${config})"

	source ${config}

	for filename in ${SL_TEST_LLR_DEBUGFS_CONFIG_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "config missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "llr_config failed"
			return 1
		fi

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "llr_config ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_llr_cmd ${ldev_num} ${lgrp_num} ${llr_num} "llr_config_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
