# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function sl_test_dynamic_debug {
	local rtn
	local options
	local OPTIND
	local type
	local state
	local names
	local usage="Usage: ${FUNCNAME} [-h | --help] type state names..."
	local description=$(cat <<-EOF
	Set dynamic debug for a symbol name.

	Mandatory:
	type       Type of symbol [func | file].
	state      Turn debug on or off for the symbol [on | off].
	names...   Space separated list of symbols.

	Options:
	-h, --help This message.

	Type:
	func       The symbol is a function.
	file       The symbol is a file.

	State:
	on         Turn debug on for the symbol.
	off        Turn debug off for the symbol.
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

	if [[ "$#" -lt 3 ]]; then
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	if [[ "$1" != "func" && "$1" != "file" ]]; then
		sl_test_debug_log "${FUNCNAME}" "unknown <type>"
		echo "${usage}"
		return 1
	fi

	if [[ "$2" == "on" ]]; then
		p="+p"
	elif [[ "$2" == "off" ]]; then
		p="-p"
	else
		sl_test_debug_log "${FUNCNAME}" "unknown <state>"
		echo "${usage}"
		return 1
	fi

	type=$1
	state=$2
	names=(${@:3})

	sl_test_debug_log "${FUNCNAME}" "(type = ${type}, state = ${state}, names = \"${names[*]}\")"

	echo 8 > /proc/sys/kernel/printk
	for name in ${names[@]}; do
		echo "${type} ${name} ${p}" > /sys/kernel/debug/dynamic_debug/control
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" \
                                "dynamic_debug failed (type = ${type}, state = ${state}, name = ${name}) [${rtn}]"
			return ${rtn}
		fi
	done

	return 0
}

function sl_test_dynamic_debug_on_show {
	local rtn
	local options
	local OPTIND
	local usage="Usage: ${FUNCNAME} [-h | --help]"
	local description=$(cat <<-EOF
	Show which dynamic debug functions have been turned on.

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

	if [[ "$#" != 0 ]]; then
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	grep "=p" /sys/kernel/debug/dynamic_debug/control
}
