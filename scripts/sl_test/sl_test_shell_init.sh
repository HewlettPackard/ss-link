# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.

source /usr/bin/sl_test_scripts/sl_test_env.sh

if [[ "$1" == "--no-reload" ]]; then
	insmod "${SL_TEST_KMOD_DIR}/sl-test.ko"
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "insmod failed [${rtn}]"
	fi
else
	sl_run_init.sh
	rtn=$?
	if [[ "${rtn}" != 0 ]]; then
		sl_test_error_log "${FUNCNAME}" "init failed [${rtn}]"
	fi
fi

export PS1="\u@\h (sl_test):\$ "

echo "sl-test environment setup complete."
echo "Type \"exit\" to return to original shell."
