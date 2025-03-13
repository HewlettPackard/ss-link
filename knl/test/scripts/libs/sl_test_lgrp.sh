# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_lgrp_check {
	local ldev_num=$1
	local -n check_lgrp_nums=$2
	local lgrp_num

	__sl_test_ldev_check ${ldev_num}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${check_lgrp_nums[@]}"; do
		if (( ${lgrp_num} < 0 || ${lgrp_num} > ${SL_TEST_LGRP_NUM_END} )); then
			sl_test_debug_log "${FUNCNAME}" \
				"(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num})"
			sl_test_debug_log "${FUNCNAME}" "invalid lgrp"
			return 1
		fi
	done

	return 0
}

function __sl_test_lgrp_sysfs_parent_set {
	local rtn
	local ldev_num=$1
	local ldev_sysfs_dir
	local -n parent_set_sysfs_dir=$2

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	__sl_test_ldev_sysfs_parent_set ${ldev_num} ldev_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	if [ -e "${SL_TEST_SYSFS_ROSSW_TOP_DIR}" ]; then
		parent_set_sysfs_dir="${ldev_sysfs_dir}/pgrp/"
	elif [ -e "${SL_TEST_SYSFS_CXI_TOP_DIR}" ]; then
		parent_set_sysfs_dir="${ldev_sysfs_dir}/device/port/"
	else
		sl_test_error_log "${FUNCNAME}" "sysfs_parent_set failed"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" "(parent_set_sysfs_dir = ${parent_set_sysfs_dir})"

	return 0
}

function __sl_test_lgrp_cmd {
	local rtn
	local ldev_num=$1
	local -n cmd_lgrp_nums=$2
	local cmd_str=$3
	local lgrp_num

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" \
			"lgrp_set failed (ldev_num = ${ldev_num}) [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${cmd_lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" \
				"lgrp_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}) [${rtn}]"
			return ${rtn}
		fi

		echo "${cmd_str}" > ${SL_TEST_LGRP_DEBUGFS_CMD}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" \
				"cmd failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, cmd_str = ${cmd_str}) [${rtn}]"
			return ${rtn}
		fi
	done

	return 0
}

function sl_test_lgrp_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums cmd_str"
	local description=$(cat <<-EOF
	Send a command to the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to send the cmd_str to.
	cmd_str    Command to send to the lgrp_num.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "lgrp")
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
	cmd_str=$3
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "lgrp" ${cmd_str}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), cmd_str = ${cmd_str})"

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_lgrp_new {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Create new link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_num belongs to.
	lgrp_nums  Link group numbers to create.

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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_new"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_new failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_del {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Delete link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to delete.

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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_notifs_remove {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local lgrp_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Remove all notifications out of the queue.

	Mandatory:
	ldev_num   Link device number lgrp_list belongs to.
	lgrp_nums  Link group numbers to delete.

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

	ldev_num=$1
	lgrp_nums=($2)

	for lgrp_num in "${lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" \
				"lgrp_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}) [${rtn}]"
			return ${rtn}
		fi

		temp_file=$(mktemp)
		sl_test_lgrp_notifs_read -r > ${temp_file} 2>&1
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_warn_log "${FUNCNAME}" "lgrp_notifs_read failed [${rtn}]"
		fi

		while IFS= read -r line; do
			sl_test_debug_log "${FUNCNAME}" "${line}"
		done < ${temp_file}

		rm ${temp_file}
	done
}

function sl_test_lgrp_cleanup {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local lgrp_nums
	local link_nums
	local furcation
	local lgrp_sysfs
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Cleanup the link groups.

	Mandatory:
	ldev_num   Link device number lgrp_list belongs to.
	lgrp_nums  Link group numbers to delete.

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

	ldev_num=$1
	lgrp_nums=($2)

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_lgrp_notifs_remove ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_warn_log "${FUNCNAME}" "lgrp_notifs_remove failed [${rtn}]"
	fi

	sl_test_lgrp_notifs_unreg ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_warn_log "${FUNCNAME}" "lgrp_notifs_unreg failed [${rtn}]"
	fi

	furcation=$(cat ${lgrp_sysfs}/${lgrp_nums[0]}/config/furcation)
	__sl_test_set_links_from_furcation ${furcation} link_nums
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_link_opt_use_fec_cntr_set ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}" off
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "fec_cntr_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_link_cleanup ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_cleanup failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_lgrp_del ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_config_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local config
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums config"
	local description=$(cat <<-EOF
	Set configuration for the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to configure.
	config     Link group configuration. See files in ${SL_TEST_LGRP_CONFIG_DIR}.

	Options:
	-h, --help This message.

	Config:
	$(find ${SL_TEST_LGRP_CONFIG_DIR} -type f)
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
	config=$3
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "${config}" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), config = ${config})"

	source ${config}

	for filename in ${SL_TEST_LGRP_DEBUGFS_CONFIG_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "config missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "lgrp_config failed"
			return 1
		fi

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_config ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_config_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_config_write failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_policy_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local policy
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums policy"
	local description=$(cat <<-EOF
	Set policies for the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to set policy for.
	policy     Link group policy. See files in ${SL_TEST_LGRP_POLICY_DIR}.

	Options:
	-h, --help This message.

	Policy:
	$(find ${SL_TEST_LGRP_POLICY_DIR} -type f)
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
	policy=$3
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "$3" ]; then
		sl_test_error_log "${FUNCNAME}" "missing policy"
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), policy = ${policy})"

	source ${policy}

	for filename in ${SL_TEST_LGRP_DEBUGFS_POLICY_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "policy missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "lgrp_policy failed"
			return 1
		fi

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_policy ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_policy_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_policy_write failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_notifs_reg {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local policy
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Register for notifications for the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to turn notifications on for.

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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_notifs_reg"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_notifs_reg failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_notifs_unreg {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local policy
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Unregister for notifications for the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to turn notifications off for.

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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

	__sl_test_lgrp_cmd ${ldev_num} lgrp_nums "lgrp_notifs_unreg"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_notifs_unreg failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_notifs_show {
	local rtn
	local ldev_num
	local lgrp_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums"
	local description=$(cat <<-EOF
	Show linkg group notifications. You must first register for the notifications.
	See sl_test_lgrp_notifs_reg.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers to turn show notifications for.

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

	if [[ "$#" != 2 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi


	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"
	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_set failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_set failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_lgrp_notifs_read -r
		rtn=$?
	done

	return ${rtn}
}

function sl_test_lgrp_setup {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local -n mac_nums
	local -n llr_nums
	local settings
	local furcation
	local lgrp_sysfs
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums settings"
	local description=$(cat <<-EOF
	Setup a link groups and its links using the provided settings.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers to setup.
	settings   Settings file containing all the configurations and policies. See ${SL_TEST_SYSTEMS_SETTINGS_DIR}.

	Options:
	-h, --help This message.

	Settings
	$(find ${SL_TEST_SYSTEMS_SETTINGS_DIR} -type f)
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
	settings=$3
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "$3" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi


	sl_test_debug_log "${FUNCNAME}" \
		"input (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), settings = ${settings})"

	source ${settings}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "source failed (settings = ${settings}) [${rtn}]"
		return ${rtn}
	fi

	sl_test_lgrp_new ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_new failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_lgrp_config_set ${ldev_num} "${lgrp_nums[*]}" "${lgrp_config}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_config_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_lgrp_policy_set ${ldev_num} "${lgrp_nums[*]}" "${lgrp_policy}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_policy_set failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	furcation=$(cat ${lgrp_sysfs}/${lgrp_nums[0]}/config/furcation)
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "furcation read failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_set_links_from_furcation ${furcation} link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
		return ${rtn}
	fi

	mac_nums=link_nums
	llr_nums=link_nums

	sl_test_link_new ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_new failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_mac_new ${ldev_num} "${lgrp_nums[*]}" "${mac_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_new failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_llr_new ${ldev_num} "${lgrp_nums[*]}" "${llr_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_new failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_link_config_set ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}" "${link_config}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_config_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_link_policy_set ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}" "${link_policy}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_policy_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_llr_config_set ${ldev_num} "${lgrp_nums[*]}" "${llr_nums[*]}" "${llr_config}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_config_set failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_lgrp_links_notif_wait {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_num
	local wait_link_nums
	local notif
	local expect
	local timeout_ms
	local notif_found
	local notif_str
	local -n notifs
	local furcation
	local temp_file
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums expect timeout_ms notifs"
	local description=$(cat <<-EOF
	Wait for the notification (expect) for all the links in a link group. Notifications will be added to the
	passed in notifs variable.

	Mandatory:
	ldev_num   Link Device Number the <lgrp_num> belongs to.
	lgrp_nums  Link Group Numbers to wait on.
	expect     Notification type to expect and wait on. See "sl_test_lgrp_notifs_read -h" for more info.
	timeout_ms Number of milliseconds to wait before exiting with ETIMEDOUT.
        notifs     Bash variable to store an array of notifications to.

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

	if [[ "$#" != 5 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	__sl_test_lgrp_check ${ldev_num} lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	expect=$3
	timeout_ms=$4
	notifs=$5

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_set failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${lgrp_nums[@]}"; do
		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_set failed [${rtn}]"
			return ${rtn}
		fi

		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "furcation read failed [${rtn}]"
			return ${rtn}
		fi

		__sl_test_set_links_from_furcation ${furcation} wait_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		notif_found=(false false false false)

		temp_file=$(mktemp)
		for link_num in "${wait_link_nums[@]}"; do

			sl_test_debug_log "${FUNCNAME}" "waiting (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"

			notif_str=$(sl_test_lgrp_notifs_read -e ${expect} -t ${timeout_ms} 2> ${temp_file})
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				while IFS= read -r line; do
					sl_test_error_log "${FUNCNAME}" "${line}"
				done < ${temp_file}
				sl_test_error_log "${FUNCNAME}" "lgrp_notifs_read failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}) [${rtn}]"
				rm ${temp_file}
				return ${rtn}
			fi

			sl_test_debug_log "${FUNCNAME}" "${notif_str}"

			notif_found[${link_num}]=true

			notifs+="${notif_str};"
		done
		rm ${temp_file}

		for link_num in "${wait_link_nums[@]}"; do
			if [[ "${notif_found[${link_num}]}" == false ]]; then
				sl_test_error_log "${FUNCNAME}" "missing notification (link_num = ${link_num}"
				return 1;
			fi
		done
	done

	return 0
}
