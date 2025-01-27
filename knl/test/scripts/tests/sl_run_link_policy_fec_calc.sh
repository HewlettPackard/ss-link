#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Verify the FEC monitor policy matches the calculated link policy for bs200"

source "${SL_TEST_DIR}/sl_test_env.sh"
settings="${SL_TEST_DIR}/systems/settings/bs200_x1_fec_calc_il.sh"

LINK_NOTIF_TIMEOUT=60000 # Timeout in milliseconds
ldev_num=0
lgrp_nums=({0..1})

function test_cleanup {
	local rtn
        local lgrp_num
        local link_num

        for lgrp_num in "${lgrp_nums[@]}"; do
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
		__sl_test_set_links_from_furcation ${furcation} sl_test_link_nums
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${sl_test_link_nums[@]}"; do

                        pos="${lgrp_sysfs}/${lgrp_num}/${SL_TEST_SYSFS_TOP_LINK_DIR}/${link_num}/"
			monitor_check="${pos}/link/fec/monitor_check/"

			monitor_ucw_down_limit_policy=$(cat "${monitor_check}/ucw_down_limit")
			monitor_ucw_warn_limit_policy=$(cat "${monitor_check}/ucw_warn_limit")
			monitor_ccw_crit_limit_policy=$(cat "${monitor_check}/ccw_crit_limit")
			monitor_ccw_warn_limit_policy=$(cat "${monitor_check}/ccw_warn_limit")

			sl_test_debug_log "${FUNCNAME}" \
                                "(monitor_ucw_down_limit_policy = ${monitor_ucw_down_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
                                "(monitor_ucw_warn_limit_policy = ${monitor_ucw_warn_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
                                "(monitor_ccw_crit_limit_policy = ${monitor_ccw_crit_limit_policy})"
			sl_test_debug_log "${FUNCNAME}" \
                                "(monitor_ccw_warn_limit_policy = ${monitor_ccw_warn_limit_policy})"

			# All values compared to bs200 calculated values.

			if [[ "${monitor_ucw_down_limit_policy}" != "21" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
                                        "(${monitor_ucw_down_limit_policy} != ${link_ucw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(monitor_ucw_down_limit_policy = ${monitor_ucw_down_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(link_ucw_down_limit_policy = ${link_ucw_down_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ucw_warn_limit_policy}" != "10" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
                                        "(${monitor_ucw_warn_limit_policy} != ${link_ucw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(monitor_ucw_warn_limit_policy = ${monitor_ucw_warn_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(link_ucw_warn_limit_policy = ${link_ucw_warn_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ccw_crit_limit_policy}" != "4250000" ]]; then
				sl_test_error_log "${FUNCNAME}" "fec monitor mismatch"
				sl_test_error_log "${FUNCNAME}" \
                                        "(${monitor_ccw_crit_limit_policy} != ${link_ccw_crit_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(monitor_ccw_crit_limit_policy = ${monitor_ccw_crit_limit_policy})"
				sl_test_error_log "${FUNCNAME}" \
                                        "(link_ccw_crit_limit_policy = ${link_ccw_crit_limit_policy})"
				return 1
			fi

			if [[ "${monitor_ccw_warn_limit_policy}" != "2125000" ]]; then
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
	local lgrp_num
        local link_num
        local sl_test_link_nums
        local sl_test_notifs
        local lgrp_sysfs

	sl_test_info_log "${FUNCNAME}" "(settings = ${settings})"

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${lgrp_nums[@]}"; do

		sl_test_info_log "${FUNCNAME}" \
                        "lgrp_setup (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, settings = ${settings})"

		sl_test_lgrp_setup ${ldev_num} ${lgrp_num} ${settings}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_setup failed [${rtn}]"
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
                        "lgrp_links_notif_wait link-up (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, LINK_NOTIF_TIMEOUT = ${LINK_NOTIF_TIMEOUT})"

                sl_test_lgrp_links_notif_wait ${ldev_num} ${lgrp_num} \
                        "link-up" ${LINK_NOTIF_TIMEOUT} sl_test_notifs
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "lgrp_links_notif_wait failed [${rtn}]"
			return ${rtn}
		fi
	done

	sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify
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
-h, --help This message.
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
