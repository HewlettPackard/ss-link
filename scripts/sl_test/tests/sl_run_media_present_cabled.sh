#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test media-present notification is received for cabled links after registration."

source "${SL_TEST_DIR}/sl_test_env.sh"

MEDIA_PRESENT_NOTIF_TIMEOUT=5000 # Timeout in milliseconds
settings="${SL_TEST_DIR}/systems/settings/bs200_x1.sh"
MAX_NUM_MEDIA=-1 # Default: unlimited

ldev_num=0
lgrp_nums=()

function find_lgrp_with_cable {
	local -n result_arr=$1
	local max_count=$2
	local pgrp_media_path
	local media_state
	local lgrp_num
	local found_count=0
	local found_lgrp=false

	# Scan through available link groups to find one with online media (cable installed)
	for lgrp_num in {0..63}; do
		# Check if we've reached the maximum number requested
		if [[ ${max_count} -gt 0 ]] && [[ ${found_count} -ge ${max_count} ]]; then
			break
		fi

		pgrp_media_path="/sys/class/rossw/rossw0/pgrp/${lgrp_num}/media/state"
		
		if [[ -f "${pgrp_media_path}" ]]; then
			media_state=$(cat "${pgrp_media_path}" 2>/dev/null)
			if [[ "${media_state}" == "online" ]]; then
				sl_test_info_log "${FUNCNAME}" "Found link group ${lgrp_num} with cable (media state: ${media_state})"
				result_arr+=( ${lgrp_num} )
				((found_count++))
				found_lgrp=true
			fi
		fi
	done

	if [[ "${found_lgrp}" != true ]]; then
		sl_test_error_log "${FUNCNAME}" "No link groups with cables (media state: online) found"
		return 1
	fi

	return 0
}

function test_cleanup {
	local rtn

	# Only cleanup if we found link groups
	if [[ ${#lgrp_nums[@]} -gt 0 ]]; then
		sl_test_lgrp_cleanup ${ldev_num} "${lgrp_nums[*]}"
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_cleanup failed [${rtn}]"
			return ${rtn}
		fi
	fi

	return 0
}

function test_verify {
	local data=$1
	
	# Verify each link in the link groups received a media-present notification
	sl_test_notif_verify_per_lgrp ${ldev_num} "${lgrp_nums[*]}" "${data}" "media-present"
	return $?
}

function main {

	local rtn
	local lgrp_sysfs
	local sl_test_media_present_notifs

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	# Discover link groups with cables installed (respecting MAX_NUM_MEDIA limit)
	find_lgrp_with_cable lgrp_nums ${MAX_NUM_MEDIA}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "find_lgrp_with_cable failed [${rtn}]"
		return ${rtn}
	fi

	if [[ ${#lgrp_nums[@]} -eq 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "No link groups with cables found"
		return 1
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

	# Media-present notifications should arrive shortly after registration for cabled links.
	# Wait for media-present notifications from all links in the link groups.
	sl_test_info_log "${FUNCNAME}" \
		"lgrp_links_notif_wait media-present (ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), MEDIA_PRESENT_NOTIF_TIMEOUT = ${MEDIA_PRESENT_NOTIF_TIMEOUT})"

	sl_test_lgrp_links_notif_wait -d ${ldev_num} "${lgrp_nums[*]}" \
		"media-present" ${MEDIA_PRESENT_NOTIF_TIMEOUT} sl_test_media_present_notifs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify "${sl_test_media_present_notifs}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		return 1
	fi

	return 0
}

SCRIPT_NAME=$(basename $0)

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-m | --max_num_media] [-b | --brief] [-g | --lgrp_nums]"
description=$(cat <<-EOF
Test media-present notification is received for cabled links.

This test:
 1. Sets up link groups with cabled links
 2. Registers for notifications
 3. Waits for media-present notifications to arrive
 4. Verifies that all received notifications are media-present type

Mandatory:
  (none)

Options:
  -h, --help                This message.
  -m, --max_num_media       Maximum number of media to process.
  -b, --brief               Print brief test description.
  -g, --lgrp_nums           Print link group numbers.
EOF
)

while true; do
        case "$1" in
                -h | --help)
                        echo "${usage}"
                        echo "${description}"
                        exit 0
                        ;;
                -b | --brief)
                        echo "${brief}"
                        exit 0
                        ;;
                -g | --lgrp_nums)
                        echo "${lgrp_nums[*]}"
                        exit 0
                        ;;
                -m | --max_num_media)
                        shift
                        MAX_NUM_MEDIA=$1
                        shift
                        ;;
                --)
                        shift
                        break
                        ;;
                -*)
                        echo "Unknown option $1"
                        echo "${usage}"
                        exit 1
                        ;;
                *)
                        break
                        ;;
        esac
done

sl_test_info_log "${SCRIPT_NAME}" "Starting"
main
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
