#!/bin/bash

SCRIPT_DIR=$(dirname "$0")

LOG_FILE=$SCRIPT_DIR/ctl_tests.log

function exec_tests {

	cleanup_file=$1
	shift
	test_files=("$@")

	for f in "${test_files[@]}"; do
		echo "Running: $(basename $f)..." >> $LOG_FILE
		echo -ne "Running: $(basename $f)..."

		swtest -f $f >> $LOG_FILE
		rtn=$?

		if [ $rtn -ne 0 ]; then
			echo "Result: \"$f\" FAILED (rtn = $rtn)" >> $LOG_FILE
			echo " FAILED (rtn = $rtn)"
			if [ -f $cleanup_file ]; then
				swtest -f $cleanup_file >> $LOG_FILE
				rtn=$?
				if [ $rtn -ne 0 ]; then
					echo "Failed to cleanup test $f"
					exit $rtn
				fi
			fi
			exit $rtn
		else
			echo "Result: \"$f\" PASSED" >> $LOG_FILE
			echo " PASSED"
			if [ -f $cleanup_file ]; then
				swtest -f $cleanup_file >> $LOG_FILE
				rtn=$?
				if [ $rtn -ne 0 ]; then
					echo "Failed to cleanup test $f"
					exit $rtn
				fi

			fi
		fi
	done
}

echo "---[ New Run ]---" >> $LOG_FILE

# We cannot run if the switch has been initialized. The best I can do is check
# if we are currently online or not

switch_mode=$(swtest switch mode)
if [ "$switch_mode" != "offline" ]; then
	echo "FAILED switch mode incorrect. Expected offline, Actual $switch_mode"
	exit -1
fi

tests=( $SCRIPT_DIR/no_device/*.swtest )

# No cleanup necessary "none"
exec_tests "none" "${tests[@]}"

tests=( $SCRIPT_DIR/*.swtest )
exec_tests $SCRIPT_DIR/test.cleanup "${tests[@]}"

echo "Full logs available at $LOG_FILE"
