# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_ldev_check {
	ldev_num=$1

	if [[ "${ldev_num}" != "0" ]]; then
		sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"
		sl_test_error_log "${FUNCNAME}" "invalid ldev"
		return 1
	fi

	return 0
}

function __sl_test_ldev_sysfs_parent_set {
	local ldev_num=$1
	local -n parent_set_sysfs_dir=$2

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	if [[ ! -n "${SL_TEST_SYSFS_TOP_DIR}" ]]; then
		sl_test_error_log "${FUNCNAME}" "SL_TEST_SYSFS_TOP_DIR not set"
		return 1
	fi

	if [ -e "${SL_TEST_SYSFS_ROSSW_TOP_DIR}" ]; then
		parent_set_sysfs_dir="${SL_TEST_SYSFS_ROSSW_TOP_DIR}/rossw${ldev_num}/"
	elif [ -e "${SL_TEST_SYSFS_CXI_TOP_DIR}" ]; then
		parent_set_sysfs_dir="${SL_TEST_SYSFS_CXI_TOP_DIR}/cxi${ldev_num}/"
	else
		sl_test_error_log "${FUNCNAME}" "sysfs_parent_set failed"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" "(parent_set_sysfs_dir = ${parent_set_sysfs_dir})"

	return 0
}

function __sl_test_ldev_cmd {
	local rtn
	local ldev_num=$1
	local cmd_str=$2

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_set failed [${rtn}]"
		return ${rtn}
	fi

	echo "${cmd_str}" > ${SL_TEST_LDEV_DEBUGFS_CMD}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_ldev_sysfs_parent_set {
	local rtn
	local OPTIND
	local options
	local ldev_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num"
	local description=$(cat <<-EOF
	Set the sysfs parent directory for the link device.

	Mandatory:
	ldev_num   Link device number to set sysfs for.

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

	if [[ "$#" != 1 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_ldev_check $1
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	__sl_test_ldev_sysfs_parent_set ${ldev_num}
	rtn=$?

	return ${rtn}
}

function sl_test_ldev_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num cmd_str"
	local description=$(cat <<-EOF
	Send a command to the link device.

	Mandatory:
	ldev_num   Link device number to send cmd_str to.
	cmd_str    Command to send to the ldev_num.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "ldev")
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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_ldev_check $1
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "ldev" $2
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	cmd_str=$2
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, cmd_str = ${cmd_str})"

	__sl_test_ldev_cmd ${ldev_num} ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_ldev_new {
	local rtn
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num"
	local description=$(cat <<-EOF
	Create a new link device.

	Mandatory:
	ldev_num   Link device number to create.

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

	if [[ "$#" != 1 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_ldev_check $1
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	__sl_test_ldev_cmd ${ldev_num} "ldev_new"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_new failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_ldev_del {
	local rtn
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num"
	local description=$(cat <<-EOF
	Delete a link device.

	Mandatory:
	ldev_num   Link device number to delete.

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
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

	if [[ "$#" != 1 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_ldev_check $1
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	ldev_num=$1
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	__sl_test_ldev_cmd ${ldev_num} "ldev_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_del failed [${rtn}]"
		return ${rtn}
	fi
}
