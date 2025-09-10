# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_sysfs_conversion_check {
	local rtn
	local filename
	local sysfs_value
	local matches
	local -n conversion_map="$1"
	local config_value=$2
	local sysfs_filename=$3

	sysfs_value=$(cat ${sysfs_filename})
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "read failed (filename = ${sysfs_filename}) [${rtn}]"
		return ${rtn}
	fi

	for key in ${!conversion_map[@]}; do
		sl_test_debug_log "${FUNCNAME}" "(key = ${key}, config_value = ${config_value})"
		if [[ "${config_value}" == "${key}" ]]; then
			sl_test_debug_log "${FUNCNAME}" \
				"(sysfs_value = ${sysfs_value}, conversion = ${conversion_map[${key}]})"
			if [[ "${sysfs_value}" == "${conversion_map[${key}]}" ]]; then
				matches=true
			fi
		fi
	done

	sl_test_debug_log "${FUNCNAME}" "(matches = ${matches})"

	if [[ "${matches}" == false ]]; then
		return 1
	fi

	return 0
}

function sl_test_type_help {
	local options
	local OPTIND
	local type
	local funcs
	local func
	local func_num
	local usage="Usage: ${FUNCNAME} [-h | --help] type"
	local description=$(cat <<-EOF
	Get help for a type of test command.

	Mandatory:
	type       Type of command to get help for [${SL_TEST_DEBUGFS_CMD_TYPES[@]}].

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
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	type=$1

	echo "${type}:"
	echo ""
	echo "functions:"
	funcs=($(typeset -F | cut -d ' ' -f3 | grep "^sl_test_${type}"))
	func_num=0
	for func in "${funcs[@]}"; do
		func_num=$((func_num + 1))
		echo "${func_num}. ${func}"
	done
	echo "help:"
	for func in "${funcs[@]}"; do
		echo "------------------------------------------------------------"
		echo ${func}
		${func} -h
		echo "------------------------------------------------------------"
		echo ""
	done
}

function sl_test_help {
	local sl_test_funcs
	local sl_test_func
	local func_num
	local description=$(cat <<-EOF
	In order to use sl_test you must first run "sl_test_init"

	See sl_test_type_help for help with specific types of functions.

	Functions:
	EOF
	)

	sl_test_funcs=($(typeset -F | cut -d ' ' -f3 | grep "^sl_test"))

	echo "${description}"
	func_num=0
	for sl_test_func in "${sl_test_funcs[@]}"; do
		func_num=$((func_num + 1))
		echo "${func_num}. ${sl_test_func}"
	done
}

function sl_pci_device_discover {
	local options
	local OPTIND
	local usage="Usage: ${FUNCNAME} [-h | --help]"
	local description=$(cat <<-EOF
	Discover the device type by PCI device ID.
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

	pci_device_id=$(lspci -n -d ${HPE_PCI_VENDOR_ID}: | cut -d ':' -f4 | cut -d ' ' -f1 | head -n1)

	case ${pci_device_id} in
		0370)
			echo "sw"
			return 0
			;;
		0371)
			echo "nic"
			return 0
			;;
		*)
			echo "none"
			return 1
			;;
	esac
}

function sl_test_cmd_help {
	local rtn
	local options
	local OPTIND
	local found=false
	local cmd_type
	local usage="Usage: ${FUNCNAME} [-h | --help] type"
	local description=$(cat <<-EOF
	Get help for a command type.

	Mandatory:
	type       Type of cmd to get help for [${SL_TEST_DEBUGFS_CMD_TYPES[@]}].

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
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	for type in "${SL_TEST_DEBUGFS_CMD_TYPES[@]}"; do
		if [[ "${type}" == "$1" ]]; then
			found=true
		fi
	done

	cmd_type=$1

	if [ "${found}" = false ]; then
		echo "${usage}"
		echo "${description}"
		return 1
	fi

	printf "%-25s %s\n" "cmd_str" "help"
	echo "----------"

	while IFS= read -r line; do
		cmd=$(echo "${line}" | cut -d ':' -f1)
		cmd_help=$(echo "${line}" | cut -d ':' -f2)
		printf "%-25s %s\n" "${cmd}" "${cmd_help}"
	done < ${SL_TEST_DEBUGFS_TOP_DIR}/${cmd_type}/cmds

	return 0
}

function sl_test_cmd_check {
	local rtn
	local options
	local OPTIND
	local cmd_type
	local cmd_str
	local found=false
	local usage="Usage: ${FUNCNAME} [-h | --help] type cmd_str"
	local description=$(cat <<-EOF
	Check if a command exists.

	Mandatory:
	type       Type of cmd_str [${SL_TEST_DEBUGFS_CMD_TYPES[@]}].
	cmd_str    Command string to check against.

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

	if [[ "$#" != 2 ]]; then
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	for type in "${SL_TEST_DEBUGFS_CMD_TYPES[@]}"; do
		if [[ "${type}" == "$1" ]]; then
			found=true
		fi
	done

	cmd_type=$1
	cmd_str=$2

	if [ "${found}" = false ]; then
		echo "${usage}"
		echo "${description}"
		return 1
	fi

	found=false
	while IFS= read -r line; do
		fields=(${line//:/ })
		if [[ "${cmd_str}" == "${fields[0]}" ]]; then
			found=true
		fi
	done < "${SL_TEST_DEBUGFS_TOP_DIR}/${cmd_type}/cmds"

	if [ "${found}" = false ]; then
		return 1
	fi

	return 0
}

function sl_test_init {
	local rtn
	local options
	local OPTIND
	local info
	local usage="Usage: ${FUNCNAME} [-h | --help] [-i | --info]"
	local description=$(cat <<-EOF
	Automatically initialize the device under test. The device type is automatically determined.

	Options:
	-h, --help This message.
	-i, --info Print basic system info.
	EOF
	)

	info=false

	options=$(getopt -o "hi" --long "help,info" -- "$@")

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
			-i | --info)
				info=true
				shift
				break
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

	shift

	if [[ "$#" != 0 ]]; then
		sl_test_debug_log "${FUNCNAME}" "incorrect number of parameters"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	eval sl_test_${SL_TEST_DEVICE_TYPE}_init
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "sl_test_${SL_TEST_DEVICE_TYPE}_init failed [${rtn}]"
		return ${rtn}
	fi

	if [[ "${info}" == "true" ]]; then
		sl_test_system_info
	fi

	return 0
}

function sl_test_unload {
	local rtn
	local options
	local OPTIND
	local usage="Usage: ${FUNCNAME} [-h | --help]"
	local description=$(cat <<-EOF
	Automatically unload the test framework from the device under test. The device type is automatically determined.

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

	eval sl_test_${SL_TEST_DEVICE_TYPE}_unload
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "sl_test_${SL_TEST_DEVICE_TYPE}_unload failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_exit {
	local rtn
	local options
	local OPTIND
	local usage="Usage: ${FUNCNAME} [-h | --help]"
	local description=$(cat <<-EOF
	Automatically tear down the device under test. The device type is automatically determined.

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

	eval sl_test_${SL_TEST_DEVICE_TYPE}_exit
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "sl_test_${SL_TEST_DEVICE_TYPE}_exit failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function __sl_test_set_links_from_furcation {

	local furcation=$1
	local -n links_array=$2

	case "${furcation,,}" in
		unfurcated)
			links_array=(0)
			;;
		bifurcated)
			links_array=({0..1})
			;;
		quadfurcated)
			links_array=({0..3})
			;;
		*)
			links_array=()
			;;
	esac

	return 0
}


function __sl_test_parse_config {
	local config=$1

	mapfile -t lines < "${config}"

	for line in "${lines[@]}"; do
		if [[ "${line}" =~ ^#.*$ || -z "${line}" ]]; then
			continue
		fi

		IFS="=" read -r key value <<< "${line}"

		key="${key// /}"
		value="${value// /}"

		export "$key"="$value"

		sl_test_debug_log "${FUNCNAME}" "${key}=${!key}"
	done
}

function __sl_test_s16_check {
	local name=$1
	local value=$2

	if ! [[ $value =~ ^-?[0-9]+$ ]]; then
		sl_test_error_log "${FUNCNAME}" "${name} not a number"
		return 1
	fi

	if (( value < -32768 || value > 32767 )); then
		sl_test_error_log "${FUNCNAME}" "(${name} = ${value}) out of range"
		return 1
	fi

	return 0
}

function __sl_test_write_cmd {
	local rtn
	local cmd=$1
	local filename=$2

	sl_test_file_write "${cmd}" "${filename}" > /dev/null 2>&1
	rtn=$?

	case ${rtn} in
		0)
			return ${rtn}
			;;
		${EALREADY})
			sl_test_warn_log "${FUNCNAME}" "already complete [${rtn}]"
			return 0;
			;;
		${EINPROGRESS})
			sl_test_warn_log "${FUNCNAME}" "in progress [${rtn}]"
			return 0;
			;;
		${EBADRQC})
			sl_test_error_log "${FUNCNAME}" "bad request [${rtn}]"
			return ${rtn}
			;;
		*)
			sl_test_error_log "${FUNCNAME}" "failed [${rtn}]"
			return ${rtn}
			;;
	esac
}

function sl_test_system_info {
	local mod
	local mods
	local eth0_ip_info

	sl_test_info_log "${FUNCNAME}" "hostname: $(hostname)"

	eth0_ip_info=($(ip -4 -br addr show dev eth0))

	sl_test_info_log "${FUNCNAME}" "ipv4/subnet: ${eth0_ip_info[2]}"

	while IFS= read -r line; do
		sl_test_info_log "${FUNCNAME}" "${line}"
	done < "${SL_TEST_BMC_IMAGE_VER}"

	mods=(sl sl-test)

	for mod in ${mods[@]}; do
		sl_test_info_log "${FUNCNAME}" "${mod} version: $(modinfo -Fversion ${mod})"
	done

	return 0
}
