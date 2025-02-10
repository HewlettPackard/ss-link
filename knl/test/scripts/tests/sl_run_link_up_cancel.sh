#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Test the link down during state starting will send both a link-up-fail and link down notification. Check all causes."

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

	local link_up_fail_found
	local link_down_found
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
	link_up_fail_found=false
	link_down_found=false
	for lgrp_num in "${lgrp_nums[@]}"; do

		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		__sl_test_set_links_from_furcation ${furcation} sl_test_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do
			IFS=';' read -ra notifs <<< "${data}"
			for notif in "${notifs[@]}"; do
				notif_fields=(${notif})
				notif_ldev_num=${notif_fields[2]}
				notif_lgrp_num=${notif_fields[3]}
				notif_link_num=${notif_fields[4]}
				notif_type=${notif_fields[6]}
                                notif_down_cause=${notif_fields[7]}

				if [[ "${notif_ldev_num}" != ${ldev_num} ]]; then
					continue
				fi

				if [[ "${notif_lgrp_num}" != ${lgrp_num} ]]; then
					continue
				fi

				if [[ "${notif_link_num}" != ${link_num} ]]; then
					continue
				fi

				if [[ "${notif_type}" == "link-up-fail" && "${notif_down_cause}" == "canceled" ]]; then
					sl_test_info_log "${FUNCNAME}" \
                                                "Expected: \"link-up-fail\" && \"canceled\", Found: \"${notif_type}\" && \"${notif_down_cause}\" (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
					sl_test_debug_log "${FUNCNAME}" "notif = ${notif}"
                                        link_up_fail_found=true
                                        continue
				fi

				if [[ "${notif_type}" == "link-down" && "${notif_down_cause}" == "none" ]]; then
					sl_test_info_log "${FUNCNAME}" \
                                                "Expected: \"link-down\" && \"none\", Found: \"${notif_type}\" && \"${notif_down_cause}\" (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
					sl_test_debug_log "${FUNCNAME}" "notif = ${notif}"
                                        link_down_found=true
                                        continue
				fi
                        done

			if [[ "${link_up_fail_found}" != true ]] && [[ "${link_down_found}" != true ]]; then
                                sl_test_error_log "${FUNCNAME} Expected: (link_up_fail_found = true, link_down_found = true)"
                                sl_test_error_log "${FUNCNAME} Found: (link_up_fail_found = ${link_up_fail_found}, link_down_found = ${link_down_found})"
				return 1
			fi

			link_up_fail_found=false
                        link_down_found=false
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
        local sl_test_link_up_fail_notif
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

		        sl_test_info_log "${FUNCNAME}" "link_up (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"

			sl_test_link_up ${ldev_num} ${lgrp_num} ${link_num}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "link_up failed [${rtn}]"
				return ${rtn}
			fi

		        sl_test_info_log "${FUNCNAME}" "link_up (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"

                        # Cancels link up
                        sl_test_link_down ${ldev_num} ${lgrp_num} ${link_num}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "link_down failed [${rtn}]"
				return ${rtn}
			fi
		done

		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_links_notif_wait link-up-fail (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

                sl_test_lgrp_links_notif_wait ${ldev_num} ${lgrp_num} \
                        "link-up-fail" ${LINK_NOTIF_TIMEOUT} sl_test_link_up_fail_notif
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_links_notif_wait link-down (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

                sl_test_lgrp_links_notif_wait ${ldev_num} ${lgrp_num} \
                        "link-down" ${LINK_NOTIF_TIMEOUT} sl_test_link_down_notif
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi
        done


	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify "${sl_test_link_up_fail_notif};${sl_test_link_down_notif}"
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
