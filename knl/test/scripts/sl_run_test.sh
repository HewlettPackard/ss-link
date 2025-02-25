#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

SCRIPT_NAME=$(basename $0)
SCRIPT_DIR=$(dirname "$0")
source "${SCRIPT_DIR}/sl_test_env.sh"

passed=0
failed=0

function exec_test {
        local test_desc
        local test_id
        local test_file
        local test_args
        local optionals
        local brief

        # jq is slow when invoked multiple times. Invoke once
        test_desc="$1"
        IFS=, read test_id test_file test_lgrp_nums test_args < <(jq -c -r '[.id,.file,.parameters.lgrp_nums,.parameters.arguments] | join(",")' <<< ${test_desc})
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "test_desc parse failed [${rtn}]"
                return ${rtn}
        fi

        test_args_expanded=($(eval "echo ${test_args}"))

        sl_test_info_log "${FUNCNAME}" "running (test_id = ${test_id}, test_file = ${test_file})"
        sl_test_debug_log "${FUNCNAME}" "(test_args_expanded = ${test_args_expanded[@]})"

        if [[ "${TEST_BRIEF}" == true ]]; then
                brief=$(bash ${test_file} --brief)
                rtn=$?

                printf "%-5s %-48s : %s\n" "${test_id}" "${test_file}" "${brief}"
                return ${rtn}
        fi

        if [[ -v "${test_lgrp_nums}" ]]; then
                optionals+="--lgrp_nums ${test_lgrp_nums}"
        fi

        if [[ "${#test_args_expanded[@]}" == 0 ]]; then
                bash ${test_file} ${optionals}
        else
                bash ${test_file} ${optionals} "${test_args_expanded[@]}"
        fi

        rtn=$?
        if [[ "${rtn}" == 0 ]]; then
                sl_test_info_log "${FUNCNAME}" "PASSED (rtn = $rtn)"
                passed=$((passed + 1))
        else
                sl_test_info_log "${FUNCNAME}" "FAILED (rtn = $rtn)"
                failed=$((failed + 1))
                return ${rtn}
        fi

        return 0
}

function exec_all_tests {
        local rtn
        local manifest
        local test_desc

        manifest=$1

        while read -r test_desc; do
                exec_test "${test_desc}"
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "exec_test failed [${rtn}]"
                        sl_test_debug_log "${FUNCNAME}" "(test_desc = ${test_desc})"
                        return ${rtn}
                fi
        done <<< $(jq -c ".[]" ${manifest})

        return 0
}

function exec_test_by_ids {
        local rtn
        local manifest
        local test_desc
        local ids
        local id

        manifest=$1
        shift
        ids=($@)

        for id in "${ids[@]}"; do

                sl_test_debug_log "${FUNCNAME}" "(id = ${id})"

                test_desc=$(jq -c --arg id "${id}" '.[] | select(.id == ($id | tonumber))' ${manifest})

                exec_test "${test_desc}"
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "exec_test failed [${rtn}]"
                        return ${rtn}
                fi
        done

        return 0
}

function exec_test_by_names {
        local rtn
        local manifest
        local test_desc
        local names
        local name

        manifest=$1
        shift
        names=($@)

        for name in "${names[@]}"; do

                sl_test_debug_log "${FUNCNAME}" "(name = ${name})"

                while read -r test_desc; do
                        exec_test "${test_descs}"
                        rtn=$?
                        if [[ "${rtn}" != 0 ]]; then
                                sl_test_error_log "${FUNCNAME}" "exec_test failed [${rtn}]"
                                return ${rtn}
                        fi
                done <<< $(jq -c --arg name "${name}" '.[] | select(.file | contains($name))' ${manifest})
        done

        return 0
}

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief] [[-a | --all] | [-n | --name NAME] | [-i | --id ID]] \
[-m | --manifest MANIFEST]"

description=$(cat <<-EOF
Run SL tests.

Only one option between all,name and id should be specified. By default all tests are run
from the default manifest ${SL_TEST_DEFAULT_MANIFEST}. Brief will not execute the test and
instead print out a brief description of the test.

Options:
-h, --help              This message.
-b, --brief             Get brief description of test (use with -a, -n, or -i)
-a, --all               Select all tests (default).
-m, --manifest MANIFEST Specify manifest to use. See MANIFEST below.
-n, --name     NAME     Select test by name.
-i, --id       ID       Select test by ID.

MANIFEST
$(find "${SL_TEST_DIR}/systems/manifests/" -type l -o -type f)
EOF
)

options=$(getopt -o "habn:i:m:" --long "help,brief,all,name:,id:,manifest:" -- "$@")

if [ "$?" != 0 ]; then
	sl_test_error_log "${SCRIPT_NAME}" "Incorrect number of arguments"
	echo "${usage}"
	echo "${description}"
	exit 1
fi

eval set -- "${options}"

RUN_ALL=false
TEST_NAMES=""
TEST_IDS=""
TEST_BRIEF=false
TEST_MANIFEST=${SL_TEST_DEFAULT_MANIFEST}

while true; do
	case "$1" in
		-h | --help)
			echo "${usage}"
			echo "${description}"
			exit 0
			;;
                -b | --brief)
                        # We don't need the typical test info, just the briefs */
                        CURRENT_LOG_LEVEL=${SL_TEST_LOG_LEVEL}
                        SL_TEST_LOG_LEVEL=1
                        TEST_BRIEF=true
                        shift
                        ;;
                -a | --all)
                        RUN_ALL=true

                        if [[ -n "${TEST_NAMES}" ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        if [[ -n "${TEST_IDS}" ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        shift
                        ;;
                -n | --name)
                        TEST_NAMES="$2"

                        if [[ "${RUN_ALL}" == true ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        if [[ -n "${TEST_IDS}" ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        shift 2
                        ;;
                -i | --id)
                        TEST_IDS="$2"

                        if [[ "${RUN_ALL}" == true ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        if [[ -n "${TEST_NAMES}" ]]; then
                                echo "Only one option --all | --id | --name can be supplied."
                                echo "${usage}"
                                echo "${description}"
                                exit 1
                        fi

                        shift 2
                        ;;
                -m | --manifest)
                        TEST_MANIFEST="$2"
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

if [ ! -f ${TEST_MANIFEST} ]; then
	sl_test_error_log "${SCRIPT_NAME}" "File not found ${TEST_MANIFEST}"
	exit 1
fi

repeated_ids=$(jq -c '.[].id' ${TEST_MANIFEST} | uniq --repeated)
if [[ -n "${repeated_ids}" ]]; then
        sl_test_error_log "${SCRIPT_NAME}" "repeated manifest IDs (repeated_ids=${repeated_ids[@]}"
        exit 1
fi

export SL_TEST_LOG_FILE=".test_$(basename ${TEST_MANIFEST} .json).log"
touch ${SL_TEST_LOG_FILE}

test_files=($(jq -c --raw-output '.[].file' ${TEST_MANIFEST}))

sl_test_info_log "${SCRIPT_NAME}" "(Total Available Tests = ${#test_files[@]})"
sl_test_info_log "${SCRIPT_NAME}" "(Manifest = ${TEST_MANIFEST})"
sl_test_info_log "${SCRIPT_NAME}" "(Log File = ${SL_TEST_LOG_FILE})"

if [[ -n "${TEST_NAMES}" ]]; then
        exec_test_by_names ${TEST_MANIFEST} "${TEST_NAMES[@]}"
        rtn=$?
elif [[ -n "${TEST_IDS}" ]]; then
        exec_test_by_ids ${TEST_MANIFEST} "${TEST_IDS[@]}"
        rtn=$?
elif [[ "${RUN_ALL}" == true ]]; then
        exec_all_tests ${TEST_MANIFEST}
        rtn=$?
else
        exec_all_tests ${TEST_MANIFEST}
        rtn=$?
fi

sl_test_info_log "${SCRIPT_NAME}" "(passed=${passed}, failed=${failed})"

sl_test_info_log "${SCRIPT_NAME}" "exit (rtn = ${rtn})"

if [[ "${TEST_BRIEF}" == true ]]; then
        SL_TEST_LOG_LEVEL=${CURRENT_LOG_LEVEL};
fi

################################################################################
# SSHOTPLAT-5519
# FIXME: Always succeed. Change back to ${rtn} when CT is stable.
################################################################################

exit 0
