#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Verify the FEC monitor turns on and off with the link policy while the link is up for bs200"

source "${SL_TEST_DIR}/sl_test_env.sh"
loopback_serdes=1
link_config="${SL_TEST_DIR}/configs/link/fec_on.sh"
link_policy="${SL_TEST_DIR}/policies/link/fec_mon_on.sh"
settings="${SL_TEST_DIR}/systems/settings/bs200_x1.sh"

LINK_NOTIF_TIMEOUT=60000 # Timeout in milliseconds
ldev_num=0
lgrp_nums=({0..63})
count=3
fec_mon_on_period_ms=1000

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
	local rtn
	local expected_mon_state=$1
	local expected_period_ms=$2
	local furcation
	local sl_test_link_nums
	local lgrp_sysfs
	local pos
	local link_state
	local mon_state
	local policy_period_ms

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
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do

			pos="${lgrp_sysfs}/${lgrp_num}/${SL_TEST_SYSFS_TOP_LINK_DIR}/${link_num}"

			link_state=$(cat "${pos}/link/state")
			policy_period_ms=$(cat "${pos}/link/policies/fec_mon_period_ms")
			mon_state=$(cat "${pos}/link/fec/monitor_state")

			sl_test_debug_log "${FUNCNAME}" \
				"(lgrp_num = ${lgrp_num}, link_num = ${link_num}, link_state = ${link_state}, policy_period_ms = ${policy_period_ms}, mon_state = ${mon_state})"

			if [[ "${link_state}" != "up" ]]; then
				sl_test_error_log "${FUNCNAME}" \
					"link not up (lgrp_num = ${lgrp_num}, link_num = ${link_num}, link_state = ${link_state})"
				return 1
			fi

			if [[ "${policy_period_ms}" != "${expected_period_ms}" ]]; then
				sl_test_error_log "${FUNCNAME}" \
					"policy mismatch (lgrp_num = ${lgrp_num}, link_num = ${link_num}, fec_mon_period_ms = ${policy_period_ms} != ${expected_period_ms})"
				return 1
			fi

			if [[ "${mon_state}" != "${expected_mon_state}" ]]; then
				sl_test_error_log "${FUNCNAME}" \
					"monitor state mismatch (lgrp_num = ${lgrp_num}, link_num = ${link_num}, monitor_state = ${mon_state} != ${expected_mon_state})"
				return 1
			fi
		done
	done

	return 0
}

function test_fec_mon_period_set_verify {
	local rtn
	local fec_mon_period_ms=$1
	local expected_mon_state=$2

	sl_test_info_log "${FUNCNAME}" "(fec_mon_period_ms = ${fec_mon_period_ms})"

	# Remaining debugfs policy fields keep the values written during setup
	echo ${fec_mon_period_ms} > ${SL_TEST_LINK_DEBUGFS_POLICY_DIR}/fec_mon_period_ms
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "fec_mon_period_ms write failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_policy_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_policy_write failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify (expected_mon_state = ${expected_mon_state})"

	test_verify ${expected_mon_state} ${fec_mon_period_ms}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "test_verify failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function main {

	local rtn
	local lgrp_sysfs
	local iter

	sl_test_info_log "${FUNCNAME}" "(settings = ${settings}, count = ${count})"

	re='^[0-9]+$'
	if ! [[ ${count} =~ ${re} ]]; then
		sl_test_error_log "${FUNCNAME}" "count NaN (count = ${count})"
		return 1
	fi

	re='^[1-9][0-9]*$'
	if ! [[ ${fec_mon_on_period_ms} =~ ${re} ]]; then
		sl_test_error_log "${FUNCNAME}" "period_ms not positive (period_ms = ${fec_mon_on_period_ms})"
		return 1
	fi

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
		sl_test_error_log "${FUNCNAME}" "lgrp_setup failed [${rtn}]"
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
	rtn=$?
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

	sl_test_info_log "${FUNCNAME}" "test_verify initial (expected_mon_state = on)"

	test_verify "on" $(source ${link_policy} && echo ${fec_mon_period_ms})
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "test_verify initial failed [${rtn}]"
		return ${rtn}
	fi

	for (( iter = 0; iter < count; iter++ )); do

		sl_test_info_log "${FUNCNAME}" "iteration (iter = ${iter}, count = ${count})"

		test_fec_mon_period_set_verify 0 "off"
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "fec monitor off failed (iter = ${iter}) [${rtn}]"
			return ${rtn}
		fi

		test_fec_mon_period_set_verify ${fec_mon_on_period_ms} "on"
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "fec monitor on failed (iter = ${iter}) [${rtn}]"
			return ${rtn}
		fi
	done

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief] [-g | --lgrp_nums] [-c | --count] [-p | --period_ms]"
description=$(cat <<-EOF
${brief}

Options:
-b, --brief	Brief test description.
-c, --count	Number of FEC monitor off/on cycles (default = ${count}).
-g, --lgrp_nums Link group numbers to test.
-h, --help	This message.
-p, --period_ms fec_mon_period_ms used to turn the FEC monitor on (default = ${fec_mon_on_period_ms}).
EOF
)

options=$(getopt -o "hg:bc:p:" --long "help,lgrp_nums:,brief,count:,period_ms:" -- "$@")

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
		-c | --count)
			count=${2}
			shift 2
			;;
		-g | --lgrp_nums)
			lgrp_nums=(${2})
			shift 2
			;;
		-p | --period_ms)
			fec_mon_on_period_ms=${2}
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
