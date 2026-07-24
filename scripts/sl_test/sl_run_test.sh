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

SL_TEST_REGISTRY=${SL_TEST_DIR}/systems/manifests/registry.json

# Resolve a test entry from registry by filename, applying the manifest's profile
# to select the correct scale parameters. Outputs a single JSON object in the
# legacy format expected by exec_test.
function resolve_test_entry_by_file {
	local registry=$1
	local profile=$2
	local file=$3

	jq -c --arg profile "${profile}" --arg file "${file}" \
                '.[] | select(.file == $file) | {id, file, brief, tags, requires, parameters: .scale[$profile]}' \
		"${registry}"
}

# Resolve a test entry from registry by ID (for --id flag support)
function resolve_test_entry_by_id {
	local registry=$1
	local profile=$2
	local id=$3

	jq -c --arg profile "${profile}" --argjson id "${id}" \
                '.[] | select(.id == $id) | {id, file, brief, tags, requires, parameters: .scale[$profile]}' \
		"${registry}"
}

function resolve_test_entry {
	local registry=$1
	local profile=$2
	local id=$3

	jq -c --arg profile "${profile}" --argjson id "${id}" \
		'.[] | select(.id == $id) | {id, file, tags, requires, parameters: .scale[$profile]}' \
		"${registry}"
}

function exec_test {
        local test_desc
        local test_id
        local test_file
        local test_brief
        local test_lgrp_nums
        local test_args
        local optionals
        local test_args_expanded

        test_desc="$1"
        test_id=$(jq -r '.id' <<< "${test_desc}")
        test_file=$(jq -r '.file' <<< "${test_desc}")
        test_lgrp_nums=$(jq -r '.parameters.lgrp_nums // ""' <<< "${test_desc}")
        test_args=$(jq -r '.parameters.arguments // ""' <<< "${test_desc}")
        test_brief=$(jq -r '.brief // ""' <<< "${test_desc}")
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "test_desc parse failed [${rtn}]"
                return ${rtn}
        fi

        sl_test_info_log "${FUNCNAME}" "running (test_id = ${test_id}, test_file = ${test_file})"

        if [[ "${TEST_BRIEF}" == true ]]; then
                printf "%-5s %-48s : %s\n" "${test_id}" "${test_file}" "${test_brief}"
                return 0
        fi

        test_args_expanded=($(eval "echo ${test_args}"))
        sl_test_debug_log "${FUNCNAME}" "(test_args_expanded = ${test_args_expanded[@]})"

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

function exec_test_by_ids {
        local rtn
        local manifest=$1
        local registry=$2
        local profile
        local test_desc
        local ids
        local id

        shift 2
        ids=($@)
        profile=$(jq -r '.profile' "${manifest}")

        for id in "${ids[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "(id = ${id})"

                test_desc=$(resolve_test_entry_by_id "${registry}" "${profile}" "${id}")
                if [[ -z "${test_desc}" ]]; then
                        sl_test_error_log "${FUNCNAME}" "id ${id} not found in registry"
                        return 1
                fi

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
        local manifest=$1
        local registry=$2
        local profile
        local test_desc
        local test_file
        local names
        local name

        shift 2
        names=($@)
        profile=$(jq -r '.profile' "${manifest}")

        for name in "${names[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "(name = ${name})"

                while read -r test_file; do
                        if [[ "${test_file}" == *"${name}"* ]]; then
                                test_desc=$(resolve_test_entry_by_file "${registry}" "${profile}" "${test_file}")
                                [[ -z "${test_desc}" ]] && continue

                                exec_test "${test_desc}"
                                rtn=$?
                                if [[ "${rtn}" != 0 ]]; then
                                        sl_test_error_log "${FUNCNAME}" "exec_test failed [${rtn}]"
                                        return ${rtn}
                                fi
                        fi
                done <<< $(jq -r '.tests[]' "${manifest}")
        done

        return 0
}

function exec_all_tests_registry {
        local rtn
        local manifest=$1
        local registry=$2
        local profile
        local tag_filter=$3
        local test_file
        local test_desc

        profile=$(jq -r '.profile' "${manifest}")

        while read -r test_file; do
                test_desc=$(resolve_test_entry_by_file "${registry}" "${profile}" "${test_file}")
                if [[ -z "${test_desc}" ]]; then
                        sl_test_error_log "${FUNCNAME}" "file ${test_file} not found in registry"
                        return 1
                fi

                # Apply tag filter if set
                if [[ -n "${tag_filter}" ]]; then
                        local -a filter_tags
                        IFS=',' read -ra filter_tags <<< "${tag_filter}"
                        local tag_json
                        tag_json=$(printf '"%s",' "${filter_tags[@]}")
                        tag_json="[${tag_json%,}]"
                        local matches
                        matches=$(jq -r --argjson filter "${tag_json}" \
                                'if (.tags | contains($filter)) then "yes" else "no" end' \
                                <<< "${test_desc}")
                        [[ "${matches}" != "yes" ]] && continue
                fi

                exec_test "${test_desc}"
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "exec_test failed [${rtn}]"
                        return ${rtn}
                fi
        done <<< $(jq -r '.tests[]' "${manifest}")

        return 0
}

function list_tests {
        local manifest=$1
        local registry=$2
        local tag_filter=$3
        local profile
        local id
        local file
        local tags
        local requires

        profile=$(jq -r '.profile' "${manifest}")

        printf "%-5s %-45s %-30s %s\n" "ID" "FILE" "TAGS" "REQUIRES"
        printf "%-5s %-45s %-30s %s\n" "---" "---" "---" "---"

        while IFS=$'\t' read -r id file tags requires; do
                printf "%-5s %-45s %-30s %s\n" "${id}" "${file}" "${tags}" "${requires}"
        done < <(
                jq -r --slurpfile reg "${registry}" --arg profile "${profile}" --arg tags "${tag_filter}" '
                        .tests[]
                        | . as $t
                        | ($reg[0][]
                           | select(
                               (($t | type) == "number" and .id == $t)
                               or
                               (($t | type) == "string" and .file == $t)
                           )
                           | {id, file, tags, requires, parameters: .scale[$profile]}
                          )
                        | select(
                            ($tags | length) == 0
                            or
                            (($tags | split(",")) as $flt | (.tags | contains($flt)))
                          )
                        | [.id, .file, (.tags | join(",")), (.requires | join(","))]
                        | @tsv
                ' "${manifest}"
        )
}

function print_briefs_all {
        local manifest=$1
        local registry=$2
        local tag_filter=$3
        local profile
        local id
        local file
        local brief

        profile=$(jq -r '.profile' "${manifest}")

        while IFS=$'\t' read -r id file brief; do
                printf "%-5s %-48s : %s\n" "${id}" "${file}" "${brief}"
        done < <(
                jq -r --slurpfile reg "${registry}" --arg profile "${profile}" --arg tags "${tag_filter}" '
                        .tests[]
                        | . as $t
                        | ($reg[0][]
                           | select(
                               (($t | type) == "number" and .id == $t)
                               or
                               (($t | type) == "string" and .file == $t)
                           )
                           | {id, file, brief, tags, requires, parameters: .scale[$profile]}
                          )
                        | select(
                            ($tags | length) == 0
                            or
                            (($tags | split(",")) as $flt | (.tags | contains($flt)))
                          )
                        | [.id, .file, (.brief // "")]
                        | @tsv
                ' "${manifest}"
        )
}

function print_briefs_by_ids {
        local manifest=$1
        local registry=$2
        local ids_input=$3
        local profile
        local ids_json
        local id
        local file
        local brief

        profile=$(jq -r '.profile' "${manifest}")
        ids_json=$(printf '%s\n' ${ids_input} | jq -Rsc 'split("\n")[:-1] | map(tonumber)')

        while IFS=$'\t' read -r id file brief; do
                printf "%-5s %-48s : %s\n" "${id}" "${file}" "${brief}"
        done < <(
                jq -r --arg profile "${profile}" --argjson ids "${ids_json}" '
                        $ids[] as $id
                        | (.[] | select(.id == $id) | {id, file, brief, parameters: .scale[$profile]})
                        | [.id, .file, (.brief // "")]
                        | @tsv
                ' "${registry}"
        )
}

function print_briefs_by_names {
        local manifest=$1
        local registry=$2
        local names_input=$3
        local profile
        local names_json
        local id
        local file
        local brief

        profile=$(jq -r '.profile' "${manifest}")
        names_json=$(printf '%s\n' ${names_input} | jq -Rsc 'split("\n")[:-1]')

        while IFS=$'\t' read -r id file brief; do
                printf "%-5s %-48s : %s\n" "${id}" "${file}" "${brief}"
        done < <(
                jq -r --slurpfile reg "${registry}" --arg profile "${profile}" --argjson names "${names_json}" '
                        .tests[]
                        | . as $t
                        | ($reg[0][]
                           | select(
                               (($t | type) == "number" and .id == $t)
                               or
                               (($t | type) == "string" and .file == $t)
                           )
                           | {id, file, brief, parameters: .scale[$profile]}
                          ) as $entry
                        | select(any($names[]; $entry.file | contains(.)))
                        | [$entry.id, $entry.file, ($entry.brief // "")]
                        | @tsv
                ' "${manifest}"
        )
}

usage="Usage: ${SCRIPT_NAME} [-h | --help] [-b | --brief] [-l | --list] \
[[-a | --all] | [-n | --name NAME] | [-i | --id ID] | [-t | --tags TAGS]] \
[-m | --manifest MANIFEST]"

description=$(cat <<-EOF
Run SL tests.

Only one option between all, name, id, and tags should be specified. By default all tests
are run from the default manifest ${SL_TEST_DEFAULT_MANIFEST}. Brief will not execute the
test and instead print out a brief description of the test.

Options:
-h, --help              This message.
-b, --brief             Get brief description of test (use with -a, -n, or -i)
-l, --list              Print test table (id, file, tags, requires) without running.
-a, --all               Select all tests (default).
-m, --manifest MANIFEST Specify manifest to use. See MANIFEST below.
-n, --name     NAME     Select test by name.
-i, --id       ID       Select test by ID.
-t, --tags     TAGS     Select tests by tag(s), comma-separated (e.g. cabled,notif).

MANIFEST
$(find "${SL_TEST_DIR}/systems/manifests/" -type l -o -type f -name "*.json" ! -name "registry.json")
EOF
)

options=$(getopt -o "habln:i:m:t:" --long "help,brief,all,list,name:,id:,manifest:,tags:" -- "$@")

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
TEST_TAGS=""
TEST_BRIEF=false
TEST_LIST=false
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
                -l | --list)
                        TEST_LIST=true
                        shift
                        ;;
                -a | --all)
                        RUN_ALL=true
                        shift
                        ;;
                -n | --name)
                        TEST_NAMES="$2"
                        shift 2
                        ;;
                -i | --id)
                        TEST_IDS="$2"
                        shift 2
                        ;;
                -m | --manifest)
                        TEST_MANIFEST="$2"
                        shift 2
                        ;;
                -t | --tags)
                        TEST_TAGS="$2"
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

if [ ! -f ${SL_TEST_REGISTRY} ]; then
        sl_test_error_log "${SCRIPT_NAME}" "Registry not found ${SL_TEST_REGISTRY}"
        exit 1
fi

export SL_TEST_LOG_FILE=".test_$(basename ${TEST_MANIFEST} .json).log"
touch ${SL_TEST_LOG_FILE}

test_count=$(jq -r '.tests | length' ${TEST_MANIFEST})

sl_test_info_log "${SCRIPT_NAME}" "(Total Available Tests = ${test_count})"
sl_test_info_log "${SCRIPT_NAME}" "(Manifest = ${TEST_MANIFEST})"
sl_test_info_log "${SCRIPT_NAME}" "(Log File = ${SL_TEST_LOG_FILE})"

if [[ "${TEST_LIST}" == true ]]; then
        list_tests ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_TAGS}"
        exit 0
fi

if [[ "${TEST_BRIEF}" == true ]]; then
        if [[ -n "${TEST_IDS}" ]]; then
                print_briefs_by_ids ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_IDS[@]}"
        elif [[ -n "${TEST_NAMES}" ]]; then
                print_briefs_by_names ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_NAMES[@]}"
        else
                print_briefs_all ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_TAGS}"
        fi
        SL_TEST_LOG_LEVEL=${CURRENT_LOG_LEVEL}
        exit 0
fi

if [[ -n "${TEST_TAGS}" ]]; then
        exec_all_tests_registry ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_TAGS}"
        rtn=$?
elif [[ -n "${TEST_NAMES}" ]]; then
        exec_test_by_names ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_NAMES[@]}"
        rtn=$?
elif [[ -n "${TEST_IDS}" ]]; then
        exec_test_by_ids ${TEST_MANIFEST} ${SL_TEST_REGISTRY} "${TEST_IDS[@]}"
        rtn=$?
else
        exec_all_tests_registry ${TEST_MANIFEST} ${SL_TEST_REGISTRY}
        rtn=$?
fi

sl_test_info_log "${SCRIPT_NAME}" "(passed=${passed}, failed=${failed})"

sl_test_info_log "${SCRIPT_NAME}" "exit (rtn = ${rtn})"

if [[ "${TEST_BRIEF}" == true ]]; then
        SL_TEST_LOG_LEVEL=${CURRENT_LOG_LEVEL};
fi

exit ${rtn}
