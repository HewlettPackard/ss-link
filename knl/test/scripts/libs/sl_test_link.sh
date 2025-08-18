# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function __sl_test_link_check {
	local rtn
	local ldev_num
	local link_num
	local -n link_lgrp_nums
	local -n check_link_nums

	ldev_num=$1
	link_lgrp_nums=$2
	check_link_nums=$3

	__sl_test_lgrp_check ${ldev_num} link_lgrp_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_check failed [${rtn}]"
		return ${rtn}
	fi

	for link_num in "${check_link_nums[@]}"; do
		if (( ${link_num} < 0 || ${link_num} > 3 )); then
			sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, link_num = ${link_num})"
			sl_test_debug_log "${FUNCNAME}" "invalid link"
			return 1
		fi
	done

	return 0
}

function __sl_test_link_sysfs_parent_set {
	local rtn
	local ldev_num=$1
	local lgrp_num=$2
	local lgrp_sysfs_dir
	local -n parent_set_sysfs_dir=$3

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num})"

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	#TODO: Need to adjust for Cassini

	parent_set_sysfs_dir="${lgrp_sysfs_dir}/${lgrp_num}/${SL_TEST_SYSFS_TOP_LINK_DIR}/"

	sl_test_debug_log "${FUNCNAME}" "(parent_set_sysfs_dir = ${parent_set_sysfs_dir})"

	return 0
}

function __sl_test_link_cmd {
	local rtn
	local cmd_str
	local ldev_num
	local lgrp_num
	local link_num
	local -n cmd_lgrp_nums
	local -n cmd_link_nums

	ldev_num=$1
	cmd_lgrp_nums=$2
	cmd_link_nums=$3
	cmd_str=$4

	echo ${ldev_num} > ${SL_TEST_LDEV_DEBUGFS_NUM}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" \
			"ldev_set failed (ldev_num = ${ldev_num}) [${rtn}]"
		return ${rtn}
	fi

	for lgrp_num in "${cmd_lgrp_nums[@]}"; do

		echo ${lgrp_num} > ${SL_TEST_LGRP_DEBUGFS_NUM}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" \
				"lgrp_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}) [${rtn}]"
			return ${rtn}
		fi

		for link_num in "${cmd_link_nums[@]}"; do
			echo ${link_num} > ${SL_TEST_LINK_DEBUGFS_NUM}
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" \
					"link_set failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}) [${rtn}]"
				return ${rtn}
			fi

			__sl_test_write_cmd "${cmd_str}" "${SL_TEST_LINK_DEBUGFS_CMD}"
			rtn=$?
			if [[ "${rtn}" != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" \
					"cmd failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}, cmd_str = ${cmd_str}) [${rtn}]"
				return ${rtn}
			fi
		done
	done

	return 0
}

function sl_test_link_cmd {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local link_nums
	local lgrp_nums
	local cmd_str
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums cmd_str"
	local description=$(cat <<-EOF
	Send a command to the links in the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to send the cmd_str to
	cmd_str    Command to send to the link_nums.

	Options:
	-h, --help This message.

	Commands:
	$(sl_test_cmd_help "link")
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	cmd_str=$4

	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_cmd_check "link" ${cmd_str}
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), cmd_str = ${cmd_str})"

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums ${cmd_str}
	rtn=$?

	return ${rtn}
}

function sl_test_link_new {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Create new links for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to create.

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
	lgrp_nums=($2)
	link_nums=($3)

	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_new"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_new failed [${rtn}]"
		return ${rtn}
	fi

	# Top level directory for ports when created by the test framework
	export SL_TEST_SYSFS_TOP_LINK_DIR="test_port"

	return 0
}

function sl_test_link_del {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Delete a link.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to delete.

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
	lgrp_nums=($2)
	link_nums=($3)

	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_del"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_up {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Set the links state up for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to set to state up.

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
	lgrp_nums=($2)
	link_nums=($3)

	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "up"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "up failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_down {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Set the links state down for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to set down.

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
	lgrp_nums=($2)
	link_nums=($3)
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "down"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "down failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_an_caps_get {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local link_num
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num link_num"
        local description=$(cat <<-EOF
        Get the link partner capabilities using autoneg.
        The requested capabilities are sent as a notification.
        Mandatory:
        ldev_num   Link Device Number the lgrp_num belongs to.
        lgrp_num   Link Group Number the link_num belongs to.
        link_num   Link Number to set up.
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
	__sl_test_link_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi
	ldev_num=$1
	lgrp_num=$2
	link_num=$3
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
	__sl_test_link_cmd ${ldev_num} ${lgrp_num} ${link_num} "link_an_caps_get"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "cmd failed [${rtn}]"
		return ${rtn}
	fi
	return 0
}

function sl_test_link_opt_use_fec_cntr_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums use_fec_cntr"
	local description=$(cat <<-EOF
	Set the link option to use the software FEC counters instead of reading the hardware counters.

	Mandatory:
	ldev_num     Link Device Number the lgrp_nums belongs to.
	lgrp_nums    Link Group Numbers the link_nums belongs to.
	link_nums    Link Numbers to modify fec cntr usage.
	use_fec_cntr Set software FEC counters [on | off].

	Options:
	-h, --help   This message.

	Use FEC Cntr:
	on	     Use the software FEC counters and stop reading hardware.
	off	     Start using hardware counters, stop using software FEC counters.
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [[ "$4" == "on" ]]; then
		use_fec_cntr=1
	elif [[ "$4" == "off" ]]; then
		use_fec_cntr=0
	else
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), use_fec_cntr = ${use_fec_cntr})"

	echo ${use_fec_cntr} > ${SL_TEST_LINK_DEBUGFS_DIR}/test/use_fec_cntr
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "use_fec_cntr_set failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_opt_set"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_opt_set failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_fec_cntr_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local ucw
	local ccw
	local gcw
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums ucw ccw gcw"
	local description=$(cat <<-EOF
	Set the rate of the FEC counters.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to set FEC cntr rate for.
	ucw        Number of UCW to increment by on each read of the counter.
	ccw        Number of CCW to increment by on each read of the counter.
	gcw        Number of GCW to increment by on each read of the counter.

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

	if [[ "$#" != 6 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	re='^[0-9]+$'
	if ! [[ $4 =~ $re || $5 =~ $re || $6 =~ $re ]]; then
		sl_test_error_log "${FUNCNAME}" "ucw,ccw, or gcw NaN"
		echo "${usage}"
		return 1
	fi

	ucw=$4
	ccw=$5
	gcw=$6
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"
	sl_test_debug_log "${FUNCNAME}" "(ucw = ${ucw}, ccw = ${ccw}, gcw = ${gcw})"

	echo ${ucw} > ${SL_TEST_LINK_DEBUGFS_DIR}/test/fec_ucw
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "use_fec_cntr_set ucw failed [${rtn}]"
		return ${rtn}
	fi

	echo ${ccw} > ${SL_TEST_LINK_DEBUGFS_DIR}/test/fec_ccw
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "use_fec_cntr_set ccw failed [${rtn}]"
		return ${rtn}
	fi

	echo ${gcw} > ${SL_TEST_LINK_DEBUGFS_DIR}/test/fec_gcw
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "use_fec_cntr_set gcw failed [${rtn}]"
		return ${rtn}
	fi

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_fec_cnt_set"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_fec_cnt_set failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_state_wait {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local link_num
	local states_match
	local end
	local wait_state
	local timeout_s
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num link_num wait_state timeout_s"
	local description=$(cat <<-EOF
	Poll for the link state.

	Mandatory:
	ldev_num   Link Device Number the lgrp_num belongs to.
	lgrp_num   Link Group Number the link_num belongs to.
	link_num   Link Number to poll for wait_state on.
	wait_state The link state to wait for.
	timeout_s  How long to wait for wait_state before timing out.

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

	if [[ "$#" != 5 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_link_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	re='^[0-9]+$'
	if ! [[ $5 =~ $re ]]; then
		sl_test_error_log "${FUNCNAME}" "timeout_s NaN"
		echo "${usage}"
		return 1
	fi

	ldev_num=$1
	lgrp_num=$2
	link_num=$3
	wait_state=$4
	timeout_s=$5
	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
	sl_test_debug_log "${FUNCNAME}" \
		"(wait_state = ${wait_state}, timeout_s = ${timeout_s}, poll_interval = ${poll_interval})"

	__sl_test_link_sysfs_parent_set ${ldev_num} ${lgrp_num} link_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	states_match=false
	count=0
	while true; do
		link_state=$(cat "${link_sysfs_dir}/${link_num}/state")
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "link_state read failed [${rtn}]"
			return ${rtn}
		fi

		sl_test_debug_log "${FUNCNAME}" "(link_state = ${link_state})"
		if [[ "${wait_state}" == "${link_state}" ]]; then
			sl_test_debug_log "${FUNCNAME}" \
				"states match (wait_state = ${wait_state}  ==  link_state = ${link_state})"
			states_match=true
			break
		fi

		sl_test_debug_log "${FUNCNAME}" \
			"polling states (wait_state = ${wait_state} !=  link_state = ${link_state})"
		sleep 1
		count=$((count + 1))

		if [[ ${count} -ge ${timeout_s} ]]; then
			sl_test_error_log "${FUNCNAME}" \
				"states (wait_state = ${wait_state} !=  link_state = ${link_state})"
			sl_test_error_log "${FUNCNAME}" \
				"failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
			return 124 # TIMEOUT
		fi
	done

	if [[ "${states_match}" != true ]]; then
		sl_test_error_log "${FUNCNAME}" \
			"states (wait_state = ${wait_state} !=  link_state = ${link_state})"
		sl_test_error_log "${FUNCNAME}" \
			"failed (ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num})"
		return 1
	fi

	return 0
}

function sl_test_link_config_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums config"
	local description=$(cat <<-EOF
	Set configuration for the links in the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers the link_nums belongs to.
	link_nums  Link Numbers to configure.
	config     Link configuration. See files in ${SL_TEST_LINK_CONFIG_DIR}.

	Options:
	-h, --help This message.

	Config:
	$(find ${SL_TEST_LINK_CONFIG_DIR} -type f)
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	config=$4
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "${config}" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), config = ${config})"

	source ${config}

	for filename in ${SL_TEST_LINK_DEBUGFS_CONFIG_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "config missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "link_config failed"
			return 1
		fi

		sl_test_info_log "${FUNCNAME}" "config.${item} = ${!item}"

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "link_config ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_config_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_config_write failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_config_check {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local link_num
        local link_sysfs_dir
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num link_num config"
	local description=$(cat <<-EOF
	Check the config matches the link configuration reported in sysfs.

	Mandatory:
	ldev_num   Link device number the lgrp_num belongs to.
	lgrp_num   Link group number the link_num belongs to.
	link_num   Link Number to check configuration for.
	config     Link configuration. See files in ${SL_TEST_LINK_CONFIG_DIR}.

	Options:
	-h, --help This message.

	Config:
	$(find ${SL_TEST_LINK_CONFIG_DIR} -type f)
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_link_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		echo "${description}"
		return ${rtn}
	fi

	if [ ! -f "$4" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi

	ldev_num=$1
	lgrp_num=$2
	link_num=$3
	config=$4
	sl_test_debug_log "${FUNCNAME}" \
                "(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}, config = ${config})"

	__sl_test_link_sysfs_parent_set ${ldev_num} ${lgrp_num} link_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	read -a lines < ${config}
	for line in "${lines[@]}"; do

		key_value=(${line//=/ })

		if [[ "${#key_value[@]}" != 2 ]]; then
			sl_test_error_log "${FUNCNAME}" "invalid line format (line = ${line})"
			return 1
		fi

		# Only debugfs has a lock entry
		if [[ "${key_value[0]}" == "lock" ]]; then
			continue
		fi

		filename="${link_sysfs_dir}/${link_num}/config/${key_value[0]}"

		sysfs_value=$(cat ${filename})
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "read failed (filename = ${filename}) [${rtn}]"
			return ${rtn}
		fi

		if [[ "${key_value[1]}" != "${sysfs_value}" ]]; then
			sl_test_error_log "${FUNCNAME}" "(${item} = ${!item} !=  sysfs_${item} = ${sysfs_value})"
			sl_test_error_log "${FUNCNAME}" "link_config_check failed"
			return 1
		fi
	done

	return 0
}

function sl_test_link_policy_set {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local policy
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums policy"
	local description=$(cat <<-EOF
	Set the policy for the links in the link groups.

	Mandatory:
	ldev_num   Link device number the lgrp_nums belongs to.
	lgrp_nums  Link group numbers the link_nums belongs to.
	link_nums  Link Numbers to set the policy for.
	policy     Link policy. See files in ${SL_TEST_LINK_POLICY_DIR}.

	Options:
	-h, --help This message.

	Policy:
	$(find ${SL_TEST_LINK_POLICY_DIR} -type f)
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	policy=$4
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "${policy}" ]; then
		sl_test_error_log "${FUNCNAME}" "missing policy"
		echo "${usage}"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), policy = ${policy})"

	source ${policy}

	for filename in ${SL_TEST_LINK_DEBUGFS_POLICY_DIR}/* ; do
		item=$(basename ${filename})
		if [ -z "${!item}" ]; then
			sl_test_error_log "${FUNCNAME}" "policy missing (item = ${item})"
			sl_test_error_log "${FUNCNAME}" "link_policy failed"
			return 1
		fi

		echo ${!item} > ${filename}
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "link_policy ${item} failed [${rtn}]"
			return ${rtn}
		fi
	done

	__sl_test_link_cmd ${ldev_num} lgrp_nums link_nums "link_policy_write"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_policy_write failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function sl_test_link_policy_check {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_num
	local link_num
	local policy
        local link_sysfs_dir
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_num link_num policy"
	local description=$(cat <<-EOF
	Check the policy matches the link policy reported in sysfs.

	Mandatory:
	ldev_num   Link device number the lgrp_num belongs to.
	lgrp_num   Link group number the link_num belongs to.
	link_num   Link Number to check policy on.
	policy     Link policy. See files in ${SL_TEST_LINK_POLICY_DIR}.

	Options:
	-h, --help This message.

	Policy:
	$(find ${SL_TEST_LINK_POLICY_DIR} -type f)
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

	if [[ "$#" != 4 ]]; then
		sl_test_error_log "${FUNCNAME}" "Incorrect number of arguments"
		echo "${usage}"
		echo "${description}"
		return 0
	fi

	__sl_test_link_check $1 $2 $3
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	if [ ! -f "$4" ]; then
		sl_test_error_log "${FUNCNAME}" "missing config"
		echo "${usage}"
		return 1
	fi

	ldev_num=$1
	lgrp_num=$2
	link_num=$3
	policy=$4
	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_num = ${lgrp_num}, link_num = ${link_num}, policy = ${policy})"

	__sl_test_link_sysfs_parent_set ${ldev_num} ${lgrp_num} link_sysfs_dir
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_sysfs_parent_set failed [${rtn}]"
		return ${rtn}
	fi

	while IFS= read -r line; do

		if [[ -z ${line} ]]; then
			continue
		fi

		if [[ "${line}" =~ ^#.*$ ]]; then
			continue
		fi

		key_value=(${line//=/ })

		if [[ "${#key_value[@]}" != 2 ]]; then
			sl_test_error_log "${FUNCNAME}" "invalid line format (line = ${line})"
			return 1
		fi

		# Only debugfs has a lock entry
		if [[ "${key_value[0]}" == "lock" ]]; then
			continue
		fi

		filename="${link_sysfs_dir}/${link_num}/link/policies/${key_value[0]}"

		sysfs_value=$(cat ${filename})
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "read failed (filename = ${filename}) [${rtn}]"
			return ${rtn}
		fi

		if [[ "${key_value[1]}" != "${sysfs_value}" ]]; then
			sl_test_error_log "${FUNCNAME}" "(${item} = ${!item} !=  sysfs_${item} = ${sysfs_value})"
			sl_test_error_log "${FUNCNAME}" "link_policy_check failed"
			return 1
		fi
	done < "${policy}"

	return 0
}

function sl_test_link_cleanup {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local -n llr_nums
	local -n mac_nums
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Cleanup the links with all related objects.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to cleanup.

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

	ldev_num=$1
	lgrp_nums=($2)
	link_nums=($3)
	llr_nums=link_nums
	mac_nums=link_nums
	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" \
		"(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}), llr_nums = (${llr_nums[*]}), mac_nums = (${mac_nums[*]}))"

	sl_test_mac_del ${ldev_num} "${lgrp_nums[*]}" "${mac_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "mac_del failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_llr_del ${ldev_num} "${lgrp_nums[*]}" "${llr_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "llr_del failed [${rtn}]"
		return ${rtn}
	fi

	sl_test_link_del ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_del failed [${rtn}]"
		return ${rtn}
	fi

	return 0
}

function __sl_test_link_field_get {
	local rtn
	local ldev_num
	local lgrp_nums
	local lgrp_num
	local link_nums
	local link_num
	local link_sysfs_dir
	local field
	local -n link_fields_str

	ldev_num=$1
	lgrp_nums=$2
	link_nums=$3
	field=$4
	link_fields_str=$5

	link_fields_str=""

	lgrps_link_states=""
	link_fields=()
	for lgrp_num in ${lgrp_nums[@]}; do

		__sl_test_link_sysfs_parent_set ${ldev_num} ${lgrp_num} link_sysfs_dir
		rtn=$?
		if [[ "${rtn}" != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "link_sysfs_parent_set failed [${rtn}]"
			return ${rtn}
		fi

		link_fields_str+="${lgrp_num}:"

		for link_num in ${link_nums[@]}; do
			if [ -e "${link_sysfs_dir}/${link_num}/link/${field}" ]; then
				link_fields+=$(cat "${link_sysfs_dir}/${link_num}/link/${field}")
			fi
		done

		link_fields_str+="${link_fields[*]};"
		link_fields=()

	done

	return 0
}

function sl_test_link_state_get {
	local rtn
	local options
	local OPTIND
	local ldev_num
	local lgrp_nums
	local link_nums
	local link_sysfs_dir
	local usage="Usage: ${FUNCNAME} [-h | --help] ldev_num lgrp_nums link_nums"
	local description=$(cat <<-EOF
	Get the links state for the link groups.

	Mandatory:
	ldev_num   Link Device Number the lgrp_nums belongs to.
	lgrp_nums  Link Group Numbers the link_nums belongs to.
	link_nums  Link Numbers to get state for.

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
	lgrp_nums=($2)
	link_nums=($3)

	__sl_test_link_check ${ldev_num} lgrp_nums link_nums
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "link_check failed [${rtn}]"
		echo "${usage}"
		return ${rtn}
	fi

	sl_test_debug_log "${FUNCNAME}" "(ldev_num = ${ldev_num}, lgrp_nums = (${lgrp_nums[*]}), link_nums = (${link_nums[*]}))"

	__sl_test_link_field_get ${ldev_num} "${lgrp_nums[*]}" "${link_nums[*]}" "state" lgrps_link_states

	IFS=';' read -ra lgrp_link_states_map <<< "${lgrps_link_states}"

	for lgrp_link_states in ${lgrp_link_states_map[@]}; do
		IFS=':' read -r lgrp link_states <<< "${lgrp_link_states}"
		printf "%02d: %s\n" ${lgrp} "${link_states}"
	done
}
