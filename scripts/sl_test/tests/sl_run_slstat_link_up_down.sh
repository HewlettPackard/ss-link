#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test slstat output is captured when a cabled link is commanded up and then down."

source "${SL_TEST_DIR}/sl_test_env.sh"

LINK_NOTIF_TIMEOUT=275000 # Timeout in milliseconds
settings="${SL_TEST_DIR}/systems/settings/bs200_x1.sh"

ldev_num=0
lgrp_nums=(23)

SLSTAT_OUTPUT_FILE=".test_$(basename $0 .sh).out"

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
	local expected_type=$1
	local data=$2

	sl_test_notif_verify_per_link ${ldev_num} "${lgrp_nums[*]}" "${data}" "${expected_type}"
	return $?
}

# Down notifications can arrive as either "link-down" or "link-async-down" depending on how the link went down.
function test_link_down_notif_wait {
	local timeout_ms=$1
	local -n out_notifs=$2
	local rtn

	sl_test_lgrp_links_notif_wait -d ${ldev_num} "${lgrp_nums[*]}" \
		"link-down" ${timeout_ms} out_notifs
	rtn=$?
	if [[ "${rtn}" == 0 ]]; then
		return 0
	fi

	sl_test_lgrp_links_notif_wait -d ${ldev_num} "${lgrp_nums[*]}" \
		"link-async-down" ${timeout_ms} out_notifs
	return $?
}

function test_slstat_capture {
	local lgrp_num
	local link_num

	{
		echo "=== slstat (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]})) ==="
		for lgrp_num in "${lgrp_nums[@]}"; do
			for link_num in "${link_nums[@]}"; do
				slstat ${ldev_num}:${lgrp_num}:${link_num} use_test
			done
		done
	} >> ${SLSTAT_OUTPUT_FILE}
	return $?
}

function main {

	local rtn
	local lgrp_sysfs
	local furcation
	local link_nums
	local sl_test_up_notifs
	local sl_test_down_notifs

	> ${SLSTAT_OUTPUT_FILE}

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

	sl_test_lgrp_links_notif_wait -d ${ldev_num} "${lgrp_nums[*]}" \
		"link-up" ${LINK_NOTIF_TIMEOUT} sl_test_up_notifs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify link-up"
	test_verify "link-up" "${sl_test_up_notifs}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	sl_test_info_log "${FUNCNAME}" "slstat capture (link up)"
	test_slstat_capture
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "slstat capture (link up) failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "link_down (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	sl_test_link_down ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_down failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" \
		"lgrp_links_notif_wait link-down|link-async-down (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

	test_link_down_notif_wait ${LINK_NOTIF_TIMEOUT} sl_test_down_notifs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify link-down|link-async-down"
	test_verify "link-down|link-async-down" "${sl_test_down_notifs}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	sl_test_info_log "${FUNCNAME}" "slstat capture (link down)"
	test_slstat_capture
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "slstat capture (link down) failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "(slstat output file = ${SLSTAT_OUTPUT_FILE})"
	cat ${SLSTAT_OUTPUT_FILE}

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief] [-g | --lgrp_nums] [-m | --max_num_media]"
description=$(cat <<-EOF
${brief}

Options:
-b, --brief         Brief test description.
-g, --lgrp_nums     Link group numbers to test.
-m, --max_num_media Number of link group connections to automatically discover.
-h, --help          This message.
EOF
)

options=$(getopt -o "hg:bm:" --long "help,lgrp_nums:,brief,max_num_media:" -- "$@")

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
		-g | --lgrp_nums)
			lgrp_nums=(${2})
			shift 2
			;;
		-m | --max_num_media)
			__sl_test_media_wb_connections_map_get ${ldev_num} tmp_lgrp_nums "PEC" ${2}
			lgrp_nums=(${tmp_lgrp_nums//;/ })
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
