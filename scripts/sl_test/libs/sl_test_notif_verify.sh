# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#
# Notification verification helper functions for test_verify() simplification
#

################################################################################
# Parse a single notification string into field variables
#
# Notification format (semicolon-delimited):
#   field[0]: timestamp
#   field[1]: unknown
#   field[2]: ldev_num
#   field[3]: lgrp_num
#   field[4]: link_num
#   field[5]: unknown
#   field[6]: notif_type
#   field[7]: cause/reason
#   field[8]: retry_status
#   field[9]: origin
#
# Usage:
#   sl_test_notif_split "timestamp;?;2;3;4;?;link-up;?;?;?" notif_
#
# Arguments:
#   $1 - notification string (space-separated fields)
#   $2 - output variable prefix (will create variables like notif_ldev_num, notif_type, etc.)
#
# Returns: 0 on success, 1 if notification malformed
################################################################################
function sl_test_notif_split {
	local notif_str=$1
	local var_prefix=$2
	local -a fields

	if [[ -z "${notif_str}" ]] || [[ -z "${var_prefix}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} notif_str var_prefix"
		return 1
	fi

	# Split on spaces (notification fields are space-separated)
	read -ra fields <<< "${notif_str}"

	if [[ ${#fields[@]} -lt 10 ]]; then
		sl_test_error_log "${FUNCNAME}" "Malformed notification: expected 10 fields, got ${#fields[@]}"
		return 1
	fi

	# Set output variables with given prefix
	eval "${var_prefix}_timestamp='${fields[0]}'"
	eval "${var_prefix}_ldev_num='${fields[2]}'"
	eval "${var_prefix}_lgrp_num='${fields[3]}'"
	eval "${var_prefix}_link_num='${fields[4]}'"
	eval "${var_prefix}_type='${fields[6]}'"
	eval "${var_prefix}_cause='${fields[7]}'"
	eval "${var_prefix}_retry='${fields[8]}'"
	eval "${var_prefix}_origin='${fields[9]}'"

	return 0
}

################################################################################
# Find a notification matching ldev_num, lgrp_num, and link_num
#
# Usage:
#   found=$(sl_test_notif_find_by_coords "ldev_num lgrp_num link_num;..." 2 3 4)
#
# Arguments:
#   $1 - notification string (semicolon-delimited notifications)
#   $2 - ldev_num to match
#   $3 - lgrp_num to match
#   $4 - link_num to match
#
# Outputs:
#   Single matched notification string, or empty if not found
#
# Returns: 0 if found, 1 if not found
################################################################################
function sl_test_notif_find_by_coords {
	local data=$1
	local match_ldev=$2
	local match_lgrp=$3
	local match_link=$4
	local -a notifs
	local notif
	local -a fields

	if [[ -z "${data}" ]]; then
		return 1
	fi

	IFS=';' read -ra notifs <<< "${data}"
	for notif in "${notifs[@]}"; do
		[[ -z "${notif}" ]] && continue
		read -ra fields <<< "${notif}"
		if [[ ${#fields[@]} -ge 5 ]] && \
		   [[ "${fields[2]}" == "${match_ldev}" ]] && \
		   [[ "${fields[3]}" == "${match_lgrp}" ]] && \
		   [[ "${fields[4]}" == "${match_link}" ]]; then
			echo "${notif}"
			return 0
		fi
	done

	return 1
}

################################################################################
# Verify a notification's type field matches expected value(s)
#
# Usage:
#   sl_test_notif_check_type "ldev lgrp link notif;..." "link-up"
#   sl_test_notif_check_type "ldev lgrp link notif;..." "link-up|link-down"  # pipe-separated
#
# Arguments:
#   $1 - notification string (space-separated fields)
#   $2 - expected type(s), pipe-separated for multiple
#
# Returns: 0 if type matches any in list, 1 otherwise
################################################################################
function sl_test_notif_check_type {
	local notif_str=$1
	local expected_types=$2
	local -a fields

	if [[ -z "${notif_str}" ]] || [[ -z "${expected_types}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} notif_str expected_types"
		return 1
	fi

	read -ra fields <<< "${notif_str}"
	if [[ ${#fields[@]} -lt 7 ]]; then
		sl_test_error_log "${FUNCNAME}" "Malformed notification"
		return 1
	fi

	local notif_type="${fields[6]}"
	
	# Check if type matches any in the pipe-separated list
	if [[ "${notif_type}" =~ ^(${expected_types})$ ]]; then
		return 0
	fi

	return 1
}

################################################################################
# Verify specific fields in a notification match expected values
#
# Usage:
#   sl_test_notif_check_fields "ldev lgrp link notif;..." 7 "ucw" 8 "retryable" 9 "origin-up"
#
# Arguments:
#   $1 - notification string (space-separated fields)
#   $2, $3 - field_index, expected_value (repeating pattern)
#   ... - more field_index, expected_value pairs
#
# Returns: 0 if all fields match, 1 if any don't
################################################################################
function sl_test_notif_check_fields {
	local notif_str=$1
	local -a fields
	local i

	if [[ -z "${notif_str}" ]] || [[ $# -lt 3 ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} notif_str field_idx expected_val [field_idx expected_val ...]"
		return 1
	fi

	read -ra fields <<< "${notif_str}"

	# Process field_index, expected_value pairs (starting at arg 2)
	for ((i=2; i<$#; i+=2)); do
		local field_idx=${!i}
		local expected_val=${@:i+1:1}

		if [[ -z "${field_idx}" ]] || [[ -z "${expected_val}" ]]; then
			sl_test_error_log "${FUNCNAME}" "Invalid field_index or expected_value at position $i"
			return 1
		fi

		if [[ ${#fields[@]} -le ${field_idx} ]]; then
			sl_test_error_log "${FUNCNAME}" "Notification missing field ${field_idx}"
			return 1
		fi

		if [[ "${fields[${field_idx}]}" != "${expected_val}" ]]; then
			sl_test_debug_log "${FUNCNAME}" "Field ${field_idx} mismatch: expected '${expected_val}', got '${fields[${field_idx}]}'"
			return 1
		fi
	done

	return 0
}

################################################################################
# Verify all notifications have type matching one of the allowed types
#
# Usage:
#   sl_test_notif_expect_all_have_type "notif1;notif2;notif3" "link-up|link-down"
#
# Arguments:
#   $1 - notification string (semicolon-delimited)
#   $2 - allowed type(s), pipe-separated
#
# Returns: 0 if all match, 1 if any don't
################################################################################
function sl_test_notif_expect_all_have_type {
	local data=$1
	local allowed_types=$2
	local -a notifs
	local notif
	local -a fields

	if [[ -z "${data}" ]] || [[ -z "${allowed_types}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} notif_data allowed_types"
		return 1
	fi

	IFS=';' read -ra notifs <<< "${data}"
	for notif in "${notifs[@]}"; do
		[[ -z "${notif}" ]] && continue
		read -ra fields <<< "${notif}"

		if [[ ${#fields[@]} -ge 7 ]]; then
			local notif_type="${fields[6]}"
			if ! [[ "${notif_type}" =~ ^(${allowed_types})$ ]]; then
				sl_test_error_log "${FUNCNAME}" "Unexpected notification type: '${notif_type}' (expected: ${allowed_types})"
				return 1
			fi
		fi
	done

	return 0
}

################################################################################
# Filter notifications by type, returning only matching ones
#
# Usage:
#   filtered=$(sl_test_notif_filter_by_type "notif1;notif2;notif3" "link-up|link-down")
#
# Arguments:
#   $1 - notification string (semicolon-delimited)
#   $2 - type filter, pipe-separated
#
# Outputs:
#   Filtered notification string (semicolon-delimited)
#
# Returns: 0 always
################################################################################
function sl_test_notif_filter_by_type {
	local data=$1
	local filter_types=$2
	local -a notifs
	local notif
	local -a fields
	local result=""

	if [[ -z "${data}" ]] || [[ -z "${filter_types}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} notif_data filter_types"
		return 1
	fi

	IFS=';' read -ra notifs <<< "${data}"
	for notif in "${notifs[@]}"; do
		[[ -z "${notif}" ]] && continue
		read -ra fields <<< "${notif}"

		if [[ ${#fields[@]} -ge 7 ]]; then
			local notif_type="${fields[6]}"
			if [[ "${notif_type}" =~ ^(${filter_types})$ ]]; then
				result+="${notif};"
			fi
		fi
	done

	echo "${result}"
	return 0
}

################################################################################
# Verify that each link group received a notification matching criteria
#
# This is similar to sl_test_notif_verify_per_link but for link-group-level
# notifications (link_num = 255). It does NOT loop through individual links or
# discover furcation. Instead, it verifies each lgrp_num has a matching
# notification with link_num = 255.
#
# Usage:
#   # Simple: each lgrp must have "media-present" notification
#   sl_test_notif_verify_per_lgrp ldev_num "lgrp_nums" "notif_data" "media-present"
#
#   # With field checks: lgrps must have "some-event" with specific field values
#   sl_test_notif_verify_per_lgrp ldev_num "lgrp_nums" "notif_data" "some-event" 7 "field_val"
#
# Arguments:
#   $1 - ldev_num
#   $2 - lgrp_nums (space-separated)
#   $3 - notification data (from sl_test_lgrp_links_notif_wait)
#   $4 - expected type (pipe-separated for multiple)
#   $5+ - optional field_index value pairs (e.g., 7 "value")
#
# Returns: 0 if all lgrps have matching notifications, 1 if any don't match
################################################################################
function sl_test_notif_verify_per_lgrp {
	local ldev_num=$1
	local lgrp_nums_str=$2
	local notif_data=$3
	local expected_type=$4
	shift 4  # remaining args are field index/value pairs
	local -a field_checks=("$@")
	
	local -a lgrp_nums=($lgrp_nums_str)
	local lgrp_num
	local found_notif
	local -a notif_fields

	if [[ -z "${ldev_num}" ]] || [[ -z "${lgrp_nums_str}" ]] || [[ -z "${notif_data}" ]] || [[ -z "${expected_type}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} ldev_num lgrp_nums notif_data expected_type [field_idx expected_val ...]"
		return 1
	fi

	sl_test_debug_log "${FUNCNAME}" "Verifying lgrp-level notifications for ldev_num=${ldev_num}, lgrp_nums=(${lgrp_nums[*]}), expected_type=${expected_type}"

	for lgrp_num in "${lgrp_nums[@]}"; do
		# Find notification for this lgrp (link_num = 255 indicates lgrp-level notification)
		found_notif=$(sl_test_notif_find_by_coords "${notif_data}" ${ldev_num} ${lgrp_num} 255)
		if [[ $? != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "No lgrp-level notification found (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255)"
			return 1
		fi

		sl_test_debug_log "${FUNCNAME}" "Found lgrp-level notification for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255): ${found_notif}"

		# Check notification type
		if ! sl_test_notif_check_type "${found_notif}" "${expected_type}"; then
			read -ra notif_fields <<< "${found_notif}"
			sl_test_error_log "${FUNCNAME}" "Type mismatch (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255): expected '${expected_type}', got '${notif_fields[6]}'"
			return 1
		fi

		sl_test_debug_log "${FUNCNAME}" "Notification type matches expected '${expected_type}' for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255)"

		# Check additional fields if provided
		if [[ ${#field_checks[@]} -gt 0 ]]; then
			if ! sl_test_notif_check_fields "${found_notif}" "${field_checks[@]}"; then
				sl_test_error_log "${FUNCNAME}" "Field check failed for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255)"
				return 1
			fi
		fi

		sl_test_debug_log "${FUNCNAME}" "All field checks passed for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=255)"
	done

	return 0
}

################################################################################
# Verify that each link in lgrp received exactly one notification matching criteria
#
# This is a higher-level helper that abstracts the per-link loop pattern common
# to many tests. It handles iterating through lgrp_nums, deriving link_nums from
# furcation, and checking each link has a matching notification.
#
# Usage:
#   # Simple: all links must have "link-up" notifications
#   sl_test_notif_verify_per_link ldev_num "lgrp_nums" "notif_data" "link-up"
#
#   # With field checks: links must have "link-up-fail" with specific cause+origin
#   sl_test_notif_verify_per_link ldev_num "lgrp_nums" "notif_data" "link-up-fail" 7 "ucw" 9 "origin-up"
#
# Arguments:
#   $1 - ldev_num
#   $2 - lgrp_nums (space-separated, same as passed to main setup functions)
#   $3 - notification data (from sl_test_lgrp_links_notif_wait)
#   $4 - expected type (pipe-separated for multiple: "link-up|link-down")
#   $5+ - optional field_index value pairs (e.g., 7 "ucw" 8 "retryable")
#
# Returns: 0 if all links verified, 1 if any don't match
################################################################################
function sl_test_notif_verify_per_link {
	local ldev_num=$1
	local lgrp_nums_str=$2
	local notif_data=$3
	local expected_type=$4
	shift 4  # remaining args are field index/value pairs
	local -a field_checks=("$@")
	
	local -a lgrp_nums=($lgrp_nums_str)
	local lgrp_num
	local furcation
	local -a link_nums
	local link_num
	local found_notif
	local -a notif_fields
	local i

	local lgrp_sysfs

	if [[ -z "${ldev_num}" ]] || [[ -z "${lgrp_nums_str}" ]] || [[ -z "${notif_data}" ]] || [[ -z "${expected_type}" ]]; then
		sl_test_error_log "${FUNCNAME}" "Usage: ${FUNCNAME} ldev_num lgrp_nums notif_data expected_type [field_idx expected_val ...]"
		return 1
	fi

	__sl_test_lgrp_sysfs_parent_set ${ldev_num} lgrp_sysfs
	if [[ $? != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "lgrp_sysfs_parent_set failed"
		return 1
	fi

        sl_test_debug_log "${FUNCNAME}" "Verifying notifications for ldev_num=${ldev_num}, lgrp_nums=(${lgrp_nums[*]}), expected_type=${expected_type}"

	for lgrp_num in "${lgrp_nums[@]}"; do
		furcation=$(cat ${lgrp_sysfs}/${lgrp_num}/config/furcation)
		if [[ $? != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "furcation read failed for lgrp ${lgrp_num}"
			return 1
		fi

		__sl_test_set_links_from_furcation ${furcation} link_nums
		if [[ $? != 0 ]]; then
			sl_test_error_log "${FUNCNAME}" "set_links_from_furcation failed for furcation ${furcation}"
			return 1
		fi

		for link_num in "${link_nums[@]}"; do
			# Find notification for this link
			found_notif=$(sl_test_notif_find_by_coords "${notif_data}" ${ldev_num} ${lgrp_num} ${link_num})
			if [[ $? != 0 ]]; then
				sl_test_error_log "${FUNCNAME}" "No notification found (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num})"
				return 1
			fi

                        sl_test_debug_log "${FUNCNAME}" "Found notification for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num}): ${found_notif}"

			# Check notification type
			if ! sl_test_notif_check_type "${found_notif}" "${expected_type}"; then
				read -ra notif_fields <<< "${found_notif}"
				sl_test_error_log "${FUNCNAME}" "Type mismatch (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num}): expected '${expected_type}', got '${notif_fields[6]}'"
				return 1
			fi

                        sl_test_debug_log "${FUNCNAME}" "Notification type matches expected '${expected_type}' for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num})"

			# Check additional fields if provided
			if [[ ${#field_checks[@]} -gt 0 ]]; then
				# Reconstruct arguments for field check
				if ! sl_test_notif_check_fields "${found_notif}" "${field_checks[@]}"; then
					sl_test_error_log "${FUNCNAME}" "Field check failed for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num})"
					return 1
				fi
			fi

                        sl_test_debug_log "${FUNCNAME}" "All field checks passed for (ldev=${ldev_num}, lgrp=${lgrp_num}, link=${link_num})"
		done
	done

	return 0
}
