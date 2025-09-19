#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test the link-up with MAC running & LLR running"

source "${SL_TEST_DIR}/sl_test_env.sh"

LINK_NOTIF_TIMEOUT=60000 # Timeout in milliseconds
settings="${SL_TEST_DIR}/systems/settings/bs200_x1_llr_on_fec_calc.sh"

ldev_num=0
lgrp_nums=({0..63})
num_cycles=100
cycles=($(seq 1 ${num_cycles}))

function test_cleanup {
	local rtn

	sl_test_lgrp_cleanup ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_cleanup failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function test_verify {
	local data
	local notifs
	local notif
	local notif_fields
	local notif_ldev_num
	local notif_lgrp_num
	local notif_link_num
	local notif_type

	data=$1

	IFS=';' read -ra notifs <<< "${data}"
	for notif in "${notifs[@]}"; do
		notif_fields=(${notif})
		notif_ldev_num=${notif_fields[2]}
		notif_lgrp_num=${notif_fields[3]}
		notif_link_num=${notif_fields[4]}
		notif_type=${notif_fields[6]}

		if [[ "${notif_type}" == "llr-running" ]]; then
			continue
		else
			sl_test_error_log "${FUNCNAME}" "failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
			sl_test_error_log "${FUNCNAME}" "Expected: llr-running, Found: ${notif_type}"
			return 1
		fi
	done

	return 0
}

function main {

	local rtn
	local lgrp_sysfs
	local furcation
	local link_nums
	local sl_test_notifs

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" \
		"lgrp_setup (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), settings = ${settings})"

	sl_test_lgrp_setup ${ldev_num} "${lgrp_nums[*]}" ${settings}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_setup failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "lgrp_notifs_reg (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

	sl_test_lgrp_notifs_reg ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_notifs_reg failed [${rtn}]"
		return ${rtn}
	fi

	# Give time for any media-present notifications to arrive. Link groups may or may not receive this notification.
	# Either way the notification queue must be empty before continuing.
	sleep 1

	sl_test_lgrp_notifs_remove ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_notifs_remove failed [${rtn}]"
		return ${rtn}
	fi

	furcation=$(cat ${lgrp_sysfs}/${lgrp_nums[0]}/config/furcation)
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "furcation read failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "(furcation = ${furcation})"

	__sl_test_set_links_from_furcation ${furcation} link_nums
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "link_up (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	sl_test_link_up ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_up failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" \
		"lgrp_links_notif_wait link-up (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

	sl_test_lgrp_links_notif_wait ${ldev_num} "${lgrp_nums[*]}" \
		"link-up" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_mac_start 0 "${lgrp_nums[*]}" 0
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_start failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_llr_setup 0 "${lgrp_nums[*]}" 0
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_setup failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" \
		"lgrp_links_notif_wait llr-setup (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

	sl_test_lgrp_links_notif_wait ${ldev_num} "${lgrp_nums[*]}" \
		"llr-setup" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_notifs=""

	for cycle in "${cycles[@]}"; do

		sl_test_info_log "${FUNCNAME}" "cycle ${cycle} of ${#cycles[@]}"

		sl_test_llr_start 0 "${lgrp_nums[*]}" 0
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "llr_start failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_info_log "${FUNCNAME}" \
			"lgrp_links_notif_wait llr-running (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

		sl_test_lgrp_links_notif_wait ${ldev_num} "${lgrp_nums[*]}" \
			"llr-running" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_llr_stop 0 "${lgrp_nums[*]}" 0
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "llr_start failed [${rtn}]"
			return ${rtn}
		fi

		sleep 3
	done

	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify "${sl_test_notifs}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-m | --max_num_media] [-b | --brief] [-g | --lgrp_nums] [-c | --count]"
description=$(cat <<-EOF
${brief}

Options:
-b, --brief         Brief test description.
-c, --count         Number of thrash cycles.
-m, --max_num_media Maximum number of media cables to test.
-g, --lgrp_nums     Link group numbers to test.
-h, --help          This message.
EOF
)

options=$(getopt -o "hm:g:bc:" --long "help,max_num_media:,lgrp_nums:,brief,count:" -- "$@")

if [ "$?" != 0 ]; then
	sl_test_error_log "${SCRIPT_NAME}" "Incorrect number of arguments"
	echo "${usage}"
	echo "${description}"
	exit 1
fi

eval set -- "${options}"

while true; do
	case "$1" in
		-h | --help)
			echo "${usage}"
			echo "${description}"
			exit 0
			;;
		-c | --count)
			num_cycles=$2
			cycles=($(seq 1 ${num_cycles}))
			shift 2
			;;
		-m | --max_num_media)
			__sl_test_media_wb_connections_map_get ${ldev_num} tmp_lgrp_nums "all" ${2}
			lgrp_nums=(${tmp_lgrp_nums//;/ })
			shift 2
			;;
		-g | --lgrp_nums)
			lgrp_nums=(${2})
			shift 2
			;;
		-b | --brief)
			echo ${brief}
			exit 0
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

if [[ "$#" != 0 ]]; then
	sl_test_error_log "${SCRIPT_NAME}" "Incorrect number of arguments"
	echo "${usage}"
	echo "${description}"
	exit 1
fi

if [[ "${#lgrp_nums}" -le "0" ]]; then
	sl_test_error_log "${SCRIPT_NAME}" "No lgrps to test"
	exit 1
fi

sl_test_info_log "${SCRIPT_NAME}" "Starting"
main $1
main_rtn=$?
if [[ "${main_rtn}" != 0 ]]; then
	sl_test_error_log "${SCRIPT_NAME}" "failed [${main_rtn}]"
else
	sl_test_info_log "${SCRIPT_NAME}" "cleanup"
	test_cleanup
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${SCRIPT_NAME}" "test_cleanup failed [${rtn}]"
	fi
fi

sl_test_info_log "${SCRIPT_NAME}" "exit (main_rtn = ${main_rtn})"
exit ${main_rtn}
