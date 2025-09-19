# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2025 Hewlett Packard Enterprise Development LP. All rights reserved.

source /usr/bin/sl_test_scripts/sl_test_env.sh

sl_test_init
rtn=$?
if [[ "${rtn}" != 0 ]]; then
	sl_test_error_log "${FUNCNAME}" "init failed [${rtn}]"
fi

export PS1="\u@\h (sl_test):\$ "

echo "sl-test environment setup complete."
echo "Type \"exit\" to return to original shell."
