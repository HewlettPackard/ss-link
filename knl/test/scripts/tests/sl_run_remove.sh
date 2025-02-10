#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

brief="Remove sl-test"

SCRIPT_NAME=$(basename $0)
source "${SL_TEST_DIR}/sl_test_env.sh"

function test_cleanup {
        # No cleanup necessary
	return 0
}

function test_verify {

	if lsmod | grep -wq "sl_test"; then
		sl_test_info_log "${FUNCNAME}" "sl-test loaded"
		return 1
	fi

	sl_test_info_log "${FUNCNAME}" "sl-test NOT loaded"

	return 0
}

function main {

	local rtn

        sl_test_info_log "${FUNCNAME}" "test_verify"
	test_verify
	rtn=$?
	if [[ "${rtn}" == 0 ]]; then
                return 0
	fi

	sl_test_info_log "${FUNCNAME}" "sl_test_unload"
        sl_test_unload
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "unload failed [${rtn}]"
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

######################################################################################################################
# Parse Options
######################################################################################################################
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
rtn=$?
if [[ "${rtn}" != 0 ]]; then
	sl_test_error_log "${SCRIPT_NAME}" "failed [${rtn}]"
else
        test_cleanup
fi

sl_test_info_log "${SCRIPT_NAME}" "exit (rtn = ${rtn})"
exit ${rtn}
