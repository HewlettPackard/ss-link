#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test the link-down notification is sent and the last_down_cause is set to command in sysfs."

source "${SL_TEST_DIR}/sl_test_env.sh"

LINK_NOTIF_TIMEOUT=60000 # Timeout in milliseconds
settings="${SL_TEST_DIR}/systems/settings/ck400_x1_fec_on_il.sh"

ldev_num=0
lgrp_nums=(0)

function test_cleanup {
	local rtn
        local lgrp_num
        local link_num

        for lgrp_num in "${lgrp_nums[@]}"; do

                for link_num in "${sl_test_link_nums[@]}"; do

		        sl_test_info_log "${FUNCNAME}" \
                                "link_fec_cntr_set zeros (link_num = ${link_num})"

                        sl_test_link_fec_cntr_set ${ldev_num} ${lgrp_num} ${link_num} 0 0 0
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "fec_cntr_set failed [${rtn}]"
				return ${rtn}
			fi

		        sl_test_info_log "${FUNCNAME}" \
                                "link_opt_use_fec_cntr_set on (link_num = ${link_num})"

                        sl_test_link_opt_use_fec_cntr_set ${ldev_num} ${lgrp_num} ${link_num} off
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "fec_cntr_set failed [${rtn}]"
				return ${rtn}
			fi
		done

		sl_test_info_log "${FUNCNAME}" "lgrp_notifs_unreg (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num})"

		sl_test_lgrp_notifs_unreg ${ldev_num} ${lgrp_num}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_notifs_unreg failed [${rtn}]"
			return ${rtn}
		fi
	done


	sl_test_lgrp_cleanup ${ldev_num} "${lgrp_nums[@]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_cleanup failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function test_verify {

	local found
        local lgrp_num
	local furcation
	local sl_test_link_nums
	local notifs
        local lgrp_sysfs

	data=$1

        __sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	#TODO: This is really inefficient
	found=false
	for lgrp_num in "${lgrp_nums[@]}"; do

		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		__sl_test_set_links_from_furcation ${furcation} sl_test_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do

                        sysfs_down_cause=$(cat ${lgrp_sysfs}/${lgrp_num}/test_port/${link_num}/link/last_down_cause)

			IFS=';' read -ra notifs <<< "${data}"
			for notif in "${notifs[@]}"; do
				notif_fields=(${notif})
				notif_ldev_num=${notif_fields[2]}
				notif_lgrp_num=${notif_fields[3]}
				notif_link_num=${notif_fields[4]}
				notif_type=${notif_fields[6]}

				if [[ "${notif_ldev_num}" != ${ldev_num} ]]; then
					continue
				fi

				if [[ "${notif_lgrp_num}" != ${lgrp_num} ]]; then
					continue
				fi

				if [[ "${notif_link_num}" != ${link_num} ]]; then
					continue
				fi

				if [[ "${notif_type}" == "link-down" ]]; then
                                        if [[ "${sysfs_down_cause}" == "command" ]]; then
					        sl_test_info_log "${FUNCNAME}" \
                                                        "Expected: command, Found: ${sysfs_down_cause} (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
                                                found=true
                                                break
                                        fi
                                fi
			done

			if [[ "${found}" != true ]]; then
                                sl_test_info_log "${FUNCNAME}" \
                                        "Expected: command, Found: ${sysfs_down_cause} (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
				return 1
			fi

			found=false
		done
	done

	return 0
}

function main {

	local rtn
	local lgrp_num
	local link_num
	local mac_num
	local llr_num
	local sl_test_link_nums
	local lgrp_sysfs
	local furcation
        local sl_test_notifs
        local ucw_limit

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

        source ${settings}
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "sourced ${settings} failed [${rtn}]"
		return ${rtn}
	fi

        source ${link_config}
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "sourced ${link_config} failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${lgrp_nums[@]}"; do

		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_links_setup (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, settings = ${settings})"

		sl_test_lgrp_setup ${ldev_num} ${lgrp_num} ${settings}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "link_setup failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_info_log "${FUNCNAME}" "lgrp_notifs_reg (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num})"

		sl_test_lgrp_notifs_reg ${ldev_num} ${lgrp_num}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_notifs_reg failed [${rtn}]"
			return ${rtn}
		fi

		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_links_state_set up (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, furcation = ${furcation})"

		__sl_test_set_links_from_furcation ${furcation} sl_test_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do

		        sl_test_link_up ${ldev_num} ${lgrp_num} ${link_num}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "link_up failed [${rtn}]"
				return ${rtn}
			fi
		done

		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_links_notif_wait link-up-fail (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

                sl_test_lgrp_links_notif_wait ${ldev_num} ${lgrp_num} "link-up" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do
                        sl_test_link_down ${ldev_num} ${lgrp_num} ${link_num}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "link_down failed [${rtn}]"
				return ${rtn}
			fi
                done

                sl_test_notifs=""
                sl_test_lgrp_links_notif_wait ${ldev_num} ${lgrp_num} "link-down" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi
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

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief]"
description=$(cat <<-EOF
${brief}

Options:
-b, --brief Brief test description.
-h, --help  This message.
EOF
)

options=$(getopt -o "hb" --long "help,brief" -- "$@")

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
