# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.
#

function sl_test_rosetta2_kill_services {

        sl_test_debug_log "${FUNCNAME}"

        services=(fabric-agent-host rossw_daemon hsnd)
        disable_services=(fabric-agent-host)

        for service in "${services[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "stopping ${service}"
                systemctl stop ${service}
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "systemctl stop ${service} failed [${rtn}]"
                        return ${rtn}
                fi
        done

        for service in "${disable_services[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "disabling ${service}"
                systemctl disable ${service} > /dev/null 2>&1
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "systemctl disable ${service} failed [${rtn}]"
                        return ${rtn}
                fi
        done

        rossw_pids=($([ -e /dev/rossw0 ] && lsof -t /dev/rossw0 | xargs))

        for pid in "${rossw_pids[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "killing process (pid = ${pid}, comm = $(ps -p ${pid} -o comm=))"
                kill ${pid}
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" \
				"kill failed [${rtn}] (pid = ${pid}, comm = $(ps -p ${pid} -o comm=))"
                        return ${rtn}
                fi
        done

        return 0
}

function sl_test_rosetta2_unload {
        local rtn

        rmmod sl-test > /dev/null 2>&1
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl-test remove failed [${rtn}]"
                return ${rtn}
        fi

        return 0
}

function sl_test_rosetta2_unload_modules {

        sl_test_debug_log "${FUNCNAME}"

        modules=(rossw roscore sl)

        for module in ${modules[@]}; do
                modinfo ${module} > /dev/null
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "missing ${module}"
                        return ${rtn}
                fi
        done

        # These may or may not be loaded
        rmmod rosnic > /dev/null 2>&1
        rmmod sl-test > /dev/null 2>&1

        for module in "${modules[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "unloading ${module}"
                modprobe -r ${module}
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "${module} module unload failed [${rtn}]"
                        return ${rtn}
                fi
        done

        return 0
}

function sl_test_rosetta2_load_modules
{
        sl_test_debug_log "${FUNCNAME}"

        modules=(sl roscore rossw sl-test)

        for module in ${modules[@]}; do
                modinfo ${module} > /dev/null
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "missing ${module}"
                        return ${rtn}
                fi
        done

        for module in "${modules[@]}"; do
                sl_test_debug_log "${FUNCNAME}" "loading ${module}"
                modprobe --first-time ${module}
                rtn=$?
                if [[ "${rtn}" != 0 ]]; then
                        sl_test_error_log "${FUNCNAME}" "${module} module load failed [${rtn}]"
                        return ${rtn}
                fi
        done

        return 0
}

function sl_test_rosetta2_init {

        hsn_active_check=0
        max_hsn_active_checks=10
        hsn_active_poll_s=1

        sl_test_debug_log "${FUNCNAME}"

        sl_test_rosetta2_kill_services
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl_test_rosetta2_kill_services failed [${rtn}]"
                return ${rtn}
        fi

        sl_test_rosetta2_unload_modules
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl_test_rosetta2_unload_modules failed [${rtn}]"
                return ${rtn}
        fi

        sl_test_rosetta2_load_modules
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl_test_rosetta2_load_modules failed [${rtn}]"
                return ${rtn}
        fi

        # HSN target guarantees hsnd and rossw_daemon are ready
        while [[ "$(systemctl is-active hsn.target)" != "active" ]]; do
                sleep ${hsn_active_poll_s}
                ((hsn_active_check++))
                if [[ ${hsn_active_check} == ${max_hsn_active_checks} ]]; then
                        break
                fi
        done

        # Creates the lgrps
        swtest init > /dev/null 2>&1
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "swtest init failed [${rtn}]"
                return ${rtn}
        fi

        return 0
}

function sl_test_rosetta2_exit {

        sl_test_debug_log "${FUNCNAME}"

        sl_test_rosetta2_kill_services
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl_test_rosetta2_kill_services failed [${rtn}]"
                return ${rtn}
        fi

        sl_test_rosetta2_unload_modules
        rtn=$?
        if [[ "${rtn}" != 0 ]]; then
                sl_test_error_log "${FUNCNAME}" "sl_test_rosetta2_unload_modules failed [${rtn}]"
                return ${rtn}
        fi

        return 0
}
