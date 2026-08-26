# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_media_db_file_create {
	local ldev_num
	local pgrps
	local media_type
	local jack_type
	local supported
	local ldev_sysfs_dir

	ldev_num=$1
	media_info_file=$2

	pgrps=({0..63})

	__sl_test_ldev_sysfs_parent_set ${ldev_num} ldev_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	for i in "${pgrps[@]}"; do
		serial_num=$(cut -d '-' -f1 ${ldev_sysfs_dir}/pgrp/${i}/media/serial_num)
		media_type=$(cat ${ldev_sysfs_dir}/pgrp/${i}/media/type)
		jack_type=$(cat ${ldev_sysfs_dir}/pgrp/${i}/media/jack_type)
		supported=$(cat ${ldev_sysfs_dir}/pgrp/${i}/media/is_supported_cable)

		if [[ "${media_type}" == "no_cable" ]]; then
			continue
		fi

		if [[ "${supported}" == "no" ]]; then
			continue
		fi

		if [[ "${jack_type}" == "unknown" ]]; then
			continue
		fi

		echo "${serial_num} ${i} ${media_type} ${jack_type} ${supported}" >> ${media_info_file}
	done

	return 0
}

function __sl_test_media_active_map_get {

	local ldev_num
	local media_info_file
	local -n pgrps_array

	ldev_num=$1
	pgrps_array=$2

	media_info_file=$(mktemp)
	__sl_test_media_db_file_create ${ldev_num} ${media_info_file}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "media_db_file_create failed [${rtn}]"
		return ${rtn}
	fi

	while IFS=' ' read -r sn pgrp type jtype support; do
		if [[ "${type}" == "AOC" ]] || [[ "${type}" == "AEC" ]] || [[ "${type}" == "ACC" ]]; then
			pgrps_array+=(${pgrp})
		fi
	done < ${media_info_file}

	rm ${media_info_file}

	return 0
}

function __sl_test_media_wb_connections_map_get {

	local ldev_num
	local -n connections_map
	local media_type_filter
	local num_connections
	local connection_num
	local media_info_file

	ldev_num=$1
	connections_map=$2
	media_type_filter=$3
	num_connections=$4

	connection_num=0
	media_info_file=$(mktemp)

	__sl_test_media_db_file_create ${ldev_num} ${media_info_file}

	# Filter media_info_file to only include lines matching media_type_filter
	if [[ "${media_type_filter}" != "all" ]]; then
		grep " ${media_type_filter} " ${media_info_file} > ${media_info_file}.filtered
		mv ${media_info_file}.filtered ${media_info_file}
	fi

	uniq_serial_nums=($(awk '{print $1}' ${media_info_file} | sort -u))

	for uniq_serial_num in "${uniq_serial_nums[@]}"; do

		mapfile -t media_info < <(grep "${uniq_serial_num}" ${media_info_file})

		for media in "${media_info[@]}"; do
			IFS=' ' read -r sn lgrp type jtype support <<< "${media}"
			connections+=(${lgrp})
		done

                # TODO: Need to account for Y cables when sysfs becomes available.
                # Right now they will be ignored.
		case $jtype in
			"qsfp-dd" )
				if [[ ${#connections[@]} -ne 4 ]]; then
					connections=()
					continue;
				fi
				;;
			"backplane" )
				if [[ ${#connections[@]} -lt 2 ]]; then
					connections=()
					continue;
				fi
				;;
			* )
				if [[ ${#connections[@]} -ne 2 ]]; then
					connections=()
					continue;
				fi
				;;
		esac

		connections_map+="${connections[*]};"
		connections=()

		((connection_num++))
		if [[ "${connection_num}" -ge "${num_connections}" ]]; then
			break
		fi
	done

	rm ${media_info_file}

	return 0
}

function sl_test_media_wb_connections_map_show {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local media_connections_map
	local media_type
	local num_connections
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num media_type num_connections"
	local description=$(cat <<-EOF
	Get a map of connected link groups.

	Link groups with matching serial numbers are connected together.

	Mandatory:
	ldev_num        Link device number to use when searching media.
	media_type      Type of media to search for.
	num_connections Number of connections to include in the map.

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

	if [[ "$#" != 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	__sl_test_ldev_check ${ldev_num}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "ldev_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num})"

	media_type=$2

	re='^[0-9]+$'
	if ! [[ $3 =~ $re ]]; then
		sl_test_error_log "${FUNCNAME}" "num_connections NaN"
		echo "${usage}"
		return 1
	fi

	num_connections=$3
	media_wb_connections_map=""

	__sl_test_media_wb_connections_map_get ${ldev_num} media_wb_connections_map "${media_type}" "${num_connections}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "media_wb_connections_map_get failed [${rtn}]"
		return ${rtn}
	fi

	IFS=';' read -ra lgrp_connections <<< "${media_wb_connections_map}"
	for lgrp_connection in "${lgrp_connections[@]}"; do
		echo "lgrps: ${lgrp_connection}"
	done

	return 0
}
