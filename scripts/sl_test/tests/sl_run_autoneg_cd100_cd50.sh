#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test auto-negotiation selects fastest speed: cd100 and cd50 enabled (should select cd100g)."

source "${SL_TEST_DIR}/sl_test_env.sh"

settings="${SL_TEST_DIR}/systems/settings/cd100_x1.sh"
autoneg=1
hpe_map_linktrain_set=1

LINK_NOTIF_TIMEOUT=275000 # Timeout in milliseconds
expected_speed="cd100g"

# Tech map for auto-negotiation
tech_map_ck400g_set=0
tech_map_bs200g_set=0
tech_map_ck200g_set=0
tech_map_cd100g_set=1
tech_map_bj100g_set=0
tech_map_ck100g_set=0
tech_map_cd50g_set=1

ldev_num=0
lgrp_nums=(23 54)

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
	local data=$1
	local lgrp_sysfs
	local port
	local link_num
	local actual_speed
	local rtn

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	# Verify link-up notifications received
	sl_test_notif_verify_per_link ${ldev_num} "${lgrp_nums[*]}" "${data}" "link-up"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return ${rtn}
	fi

	# Read actual negotiated speeds from sysfs
	for lgrp in "${lgrp_nums[@]}"; do
		for link_num in {0..3}; do
			local link_path="${lgrp_sysfs}/${lgrp}/port/${link_num}/link"
			if [[ ! -f "${link_path}/speed" ]]; then
				continue
			fi

			actual_speed=$(cat "${link_path}/speed")
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "Failed to read speed for lgrp ${lgrp} link ${link_num}"
				return ${rtn}
			fi

			sl_test_info_log "${FUNCNAME}" "lgrp ${lgrp} link ${link_num}: expected_speed=${expected_speed}, actual_speed=${actual_speed}"

			if [[ "${actual_speed}" != "${expected_speed}" ]]; then
				sl_test_error_log "${FUNCNAME}" "Speed mismatch for lgrp ${lgrp} link ${link_num}: expected ${expected_speed}, got ${actual_speed}"
				return 1
			fi
		done
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
		"lgrp_setup (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), tech_map_cd100g=${tech_map_cd100g_set} cd50g=${tech_map_cd50g_set})"

	sl_test_lgrp_setup ${ldev_num} "${lgrp_nums[*]}" ${settings}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_setup failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "lgrp_notifs_reg (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]})))"

	sl_test_lgrp_notifs_reg ${ldev_num} "${lgrp_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_notifs_reg failed [${rtn}]"
		return ${rtn}
	fi

	# Give time for any media-present notifications to arrive
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
		"link-up" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify "${sl_test_notifs}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-m | --max_num_media] [-b | --brief] [-g | --lgrp_nums]"
description=$(cat <<-EOF
	${brief}

Options:
-b, --brief         Brief test description.
-m, --max_num_media Number of link group connections to automatically discover.
-g, --lgrp_nums     Link group numbers to test.
-h, --help          This message.
EOF
)

options=$(getopt -o "hm:g:b" --long "help,max_num_media:,lgrp_nums:,brief" -- "$@")

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
		-m | --max_num_media)
			__sl_test_media_wb_connections_map_get ${ldev_num} tmp_lgrp_nums "PEC" ${2}
			lgrp_nums=(${tmp_lgrp_nums//;/ })
			shift 2
			;;
		-g | --lgrp_nums)
			lgrp_nums=(${2})
			shift 2
			;;
		-b | --brief)
			echo "${brief}"
			exit 0
			;;
		-- ) shift; break ;;
		* ) break ;;
	esac
done

if [[ "${#lgrp_nums[@]}" == 0 ]]; then
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
