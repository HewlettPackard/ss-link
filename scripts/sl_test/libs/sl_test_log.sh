# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

# Default log level is "info"
export SL_TEST_LOG_LEVEL=${SL_TEST_LOG_LEVEL:-2}
[ -z $TERM ] && export TERM="xterm-256color"

declare -A LOG_LEVELS
LOG_LEVELS=([0]="error" [1]="warn" [2]="info" [3]="debug")

function log {
	local LEVEL
        local FUNCTION

        LEVEL=${1}
        FUNCTION=${2}
        shift 2

	if [ ${SL_TEST_LOG_LEVEL} -ge ${LEVEL} ]; then
                printf "%-25s %-10s %-48s: %s\n" "$(date '+%Y-%m-%d %H:%M:%S.%5N')" \
                        "(${LOG_LEVELS[$LEVEL]})" "${FUNCTION}" "$@" | tee -a ${SL_TEST_LOG_FILE}
	fi
}

function sl_test_error_log {
        local function

        function=$1
        shift

        tput setaf 1 # Red
	log 0 "${function}" "$@"
        tput sgr0 # normal
}

function sl_test_warn_log {
        local function

        function=$1
        shift

        tput setaf 3 # Yellow
	log 1 "${function}" "$@"
        tput sgr0 # normal
}

function sl_test_info_log {
        local function

        function=$1
        shift

        log 2 "${function}" "$@"
}

function sl_test_debug_log {
        local function

        function=$1
        shift

	log 3 "${function}" "$@"
}

function sl_test_log_level_set {
	local level
	local log_levels_end
	local log_level
	local usage="${FUNCNAME} log_level"
	local description=$(cat <<-EOF
	Set the log level.

	Mandatory:
	log_level  Log level to set to. [${LOG_LEVELS[@]}]

	Options:
	-h, --help This message.
	EOF
	)

	options=$(getopt -o "h" --long "help" -- "$@")

	if [ "$?" != 0 ]; then
		echo "${usage}"
		echo "${description}"
	fi

	eval set -- "${options}"

	while true; do
		case "$1" in
			-h | --help)
				echo "${usage}"
				echo "${description}"
				return 0
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

	if [[ "$#" != 1 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	log_level=$1
	log_levels_end=$((${#LOG_LEVELS[@]} - 1))

	sl_test_debug_log "${FUNCNAME} (log_level = ${log_level})"

	for level in $(seq 0 ${log_levels_end}); do
		sl_test_debug_log "${FUNCNAME} (level = ${level} \"${LOG_LEVELS[$level]}\")"
		if [[ "${LOG_LEVELS[$level]}" == "${log_level}" ]]; then
			export SL_TEST_LOG_LEVEL=$level
		fi
	done
}
