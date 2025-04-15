# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_serdes_cmd {
	local rtn
	local ldev_num=$1
	local -n cmd_lgrp_nums=$2
	local -n cmd_link_nums=$3
        local lgrp_num
	local link_num
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

		for link_num in "${cmd_link_nums[@]}"; do
			echo ${link_num} > ${SL_TEST_LINK_DEBUGFS_NUM}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "serdes_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}) [${rtn}]"
				return ${rtn}
			fi

			__sl_test_write_cmd "${cmd_str}" "${SL_TEST_SERDES_DEBUGFS_CMD}"
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "cmd failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}, cmd_str = ${cmd_str}) [${rtn}]"
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
	local link_nums
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums cmd_str"
	local description=$(cat <<-EOF
	Send a command to the SerDes in the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers the link_nums belongs to.
	link_nums  Serdes numbers to send the command to.
	cmd_str    Command to send to the link_nums.

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
	link_nums=($3)
	cmd_str=$4
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
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
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), cmd_str = ${cmd_str})"

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums link_nums ${cmd_str}
	rtn=$?

	return ${rtn}
}

function __sl_test_serdes_settings_check {
	local setting=$1
	local value=$2

	if [ -z "${value}" ]; then
		sl_test_error_log "${FUNCNAME}" "${setting} missing"
		return 1
	fi

	options=($(cat ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/${setting}_options))
	for choice in ${options[@]}; do
		if [ "${value}" == "${choice}" ]; then
			return 0
		fi
	done

	sl_test_error_log "${FUNCNAME}" "${setting} check failed (${setting} = ${value}, options = ${options[*]})"

	return 1
}

function __sl_test_serdes_settings_options_get {
	local setting=$1

	options=$(cat ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/${setting}_options)

	echo ${options} | awk '{ gsub (" ", " | ", $0); print}'
}

function sl_test_serdes_settings_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local settings
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums settings"
	local description=$(cat <<-EOF
	Set SerDes parameters for the SerDes in link groups.

	Mandatory:
	ldev_num  Link device number the lgrp_nums belongs to.
	lgrp_nums Link group number the link_nums belongs to.
	link_nums Link Numbers to set serdes settings for.
	settings  Serdes parameters file. See ${SL_TEST_SERDES_SETTINGS_DIR}

	Options:
	-h, --help  This message.

	Settings Options:
	clocking = $(__sl_test_serdes_settings_options_get "clocking")
	cursor   = -32,768 < cursor < 32,767
	dfe      = $(__sl_test_serdes_settings_options_get "dfe")
	encoding = $(__sl_test_serdes_settings_options_get "encoding")
	media    = $(__sl_test_serdes_settings_options_get "media")
	osr      = $(__sl_test_serdes_settings_options_get "osr")
	post1    = -32,768 < post1 < 32,767
	post2    = -32,768 < post2 < 32,767
	pre1     = -32,768 < pre1 < 32,767
	pre2     = -32,768 < pre2 < 32,767
	pre3     = -32,768 < pre3 < 32,767
	scramble = $(__sl_test_serdes_settings_options_get "scramble")
	width    = $(__sl_test_serdes_settings_options_get "width")

	Settings File:
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
	link_nums=($3)
	settings=$4
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
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
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), settings = ${settings})"

	__sl_test_parse_config ${settings}

	__sl_test_serdes_settings_check "clocking" "${clocking}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed [${rtn}]"
		return ${rtn}
	fi

        __sl_test_s16_check "cursor" ${cursor}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "dfe" "${dfe}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "encoding" "${encoding}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "media" "${media}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "osr" "${osr}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed [${rtn}]"
		return ${rtn}
	fi

        __sl_test_s16_check "post1" ${post1}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_s16_check "post2" ${post2}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_s16_check "pre1" ${pre1}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_s16_check "pre2" ${pre2}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_s16_check "pre3" ${pre3}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "s16_check failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "scramble" "${scramble}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed (encoding = ${scramble}) [${rtn}]"
		return ${rtn}
	fi

	__sl_test_serdes_settings_check "width" "${width}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_settings_check failed (encoding = ${width}) [${rtn}]"
		return ${rtn}
	fi

	echo -n ${clocking} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/clocking
	echo -n ${cursor} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/cursor
	echo -n ${dfe} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/dfe
	echo -n ${encoding} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/encoding
	echo -n ${osr} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/osr
	echo -n ${post1} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/post1
	echo -n ${post2} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/post2
	echo -n ${pre1} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/pre1
	echo -n ${pre2} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/pre2
	echo -n ${pre3} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/pre3
	echo -n ${scramble} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/scramble
	echo -n ${width} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/width
	echo -n ${media} > ${SL_TEST_SERDES_DEBUGFS_SETTINGS_DIR}/media

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums link_nums "serdes_params_set"
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
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Unset SerDes parameters for SerDes in the link groups.

	Mandatory:
	ldev_num  Link device number the lgrp_nums belongs to.
	lgrp_nums Link group numbers the link_nums belongs to.
	link_nums Link Numbers to unset serdes settings for.

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
	link_nums=($3)
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_serdes_cmd ${ldev_num} lgrp_nums link_nums "serdes_params_unset"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "serdes_cmd failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}
