#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Verify the FEC monitor policy matches the explicit link FEC policy for bs200"

source "${SL_TEST_DIR}/sl_test_env.sh"
settings="${SL_TEST_DIR}/systems/settings/bs200_x1_lb_fec_on.sh"

LINK_NOTIF_TIMEOUT=60000 # Timeout in milliseconds
ldev_num=0
lgrp_nums=({0..63})

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
	local furcation
	local sl_test_link_nums
	local lgrp_sysfs

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${lgrp_nums[@]}"; do

		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "furcation read failed [${rtn}]"
			return ${rtn}
		fi

		__sl_test_set_links_from_furcation ${furcation} sl_test_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do

			pos="${lgrp_sysfs}/${lgrp_num}/${SL_TEST_SYSFS_TOP_LINK_DIR}/${link_num}/"
			monitor_policies="${pos}/link/fec/monitor_check/"

			monitor_ucw_down_limit_policy=$(cat "${monitor_policies}/ucw_down_limit")
			monitor_ucw_warn_limit_policy=$(cat "${monitor_policies}/ucw_warn_limit")
			monitor_ccw_down_limit_policy=$(cat "${monitor_policies}/ccw_down_limit")
			monitor_ccw_warn_limit_policy=$(cat "${monitor_policies}/ccw_warn_limit")

			sl_test_debug_log "${FUNCNAME}" \
				"(monitor_ucw_down_limit_policy = ${monitor_ucw_down_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
				"(monitor_ucw_warn_limit_policy = ${monitor_ucw_warn_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
				"(monitor_ccw_down_limit_policy = ${monitor_ccw_down_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
				"(monitor_ccw_warn_limit_policy = ${monitor_ccw_warn_limit_policy})"

			link_policies="${pos}/link/policies/"

			link_ucw_down_limit_policy=$(cat "${link_policies}/fec_mon_ucw_down_limit")
			link_ucw_warn_limit_policy=$(cat "${link_policies}/fec_mon_ucw_warn_limit")
			link_ccw_down_limit_policy=$(cat "${link_policies}/fec_mon_ccw_down_limit")
			link_ccw_warn_limit_policy=$(cat "${link_policies}/fec_mon_ccw_warn_limit")

			sl_test_debug_log "${FUNCNAME}" "(link_ucw_down_limit_policy = ${link_ucw_down_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" "(link_ucw_down_limit_policy = ${link_ucw_warn_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" "(link_ccw_down_limit_policy = ${link_ccw_down_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" "(link_ccw_warn_limit_policy = ${link_ccw_warn_limit_policy})"

			if [[ "${monitor_ucw_down_limit_policy}" != "${link_ucw_down_limit_policy}" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
					"(${monitor_ucw_down_limit_policy} != ${link_ucw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(monitor_ucw_down_limit_policy = ${monitor_ucw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(link_ucw_down_limit_policy = ${link_ucw_down_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ucw_warn_limit_policy}" != "${link_ucw_warn_limit_policy}" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
					"(${monitor_ucw_warn_limit_policy} != ${link_ucw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(monitor_ucw_warn_limit_policy = ${monitor_ucw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(link_ucw_warn_limit_policy = ${link_ucw_warn_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ccw_down_limit_policy}" != "${link_ccw_down_limit_policy}" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
					"(${monitor_ccw_down_limit_policy} !=  ${link_ccw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(monitor_ccw_down_limit_policy = ${monitor_ccw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(link_ccw_down_limit_policy = ${link_ccw_down_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ccw_warn_limit_policy}" != "${link_ccw_warn_limit_policy}" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
					"(${monitor_ccw_warn_limit_policy} != ${link_ccw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(monitor_ccw_warn_limit_policy = ${monitor_ccw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
					"(link_ccw_warn_limit_policy = ${link_ccw_warn_limit_policy})"
				return 1
			fi
		done
	done

	return 0
}

function main {

	local rtn
	local lgrp_sysfs

	sl_test_info_log "${FUNCNAME}" "(settings = ${settings})"

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

	sl_test_info_log "${FUNCNAME}" \
		"lgrp_notifs_remove (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}))"

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
		"link-up" ${LINK_NOTIF_TIMEOUT} sl_test_link_up_notif
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief] [-g | --lgrp_nums]"
description=$(cat <<-EOF
${brief}

Options:
-b, --brief	Brief test description.
-g, --lgrp_nums Link group numbers to test.
-h, --help	This message.
EOF
)

options=$(getopt -o "hg:b" --long "help,lgrp_nums:,brief" -- "$@")

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
		-b | --brief)
			echo ${brief}
			exit 0
			;;
		-g | --lgrp_nums)
			lgrp_nums=(${2})
			shift 2
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
