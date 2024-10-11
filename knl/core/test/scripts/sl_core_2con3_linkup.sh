#!/usr/bin/bash

## PURPOSE: test the SL core sequence to bring a link up and then down
##          between link group 2 and 3 at 200G with MAC and LLR

SWTEST="swtest"

do_cleanup() {
    # LLR del
    ${SWTEST} test 2 28 0xC 0x1 0
    # MAC del
    ${SWTEST} test 2 26 0xC 0x1 0
    # LINK del
    ${SWTEST} test 2 9 0xC 0x1 0
    # LGRP del
    ${SWTEST} test 2 4 0xC 0x1 0
}

do_cmd() {
    TEST_STR=$1
    TEST_NUM=$2

    echo "${TEST_STR}..."

    ${SWTEST} test 2 ${TEST_NUM} 0xC 0x1 0
    RTN=$?
    if [[ ${RTN} -ne 0 ]]; then
        echo "ERROR: \"${TEST_STR}\" failed [${RTN}]"
	do_cleanup
        exit 1
    fi
}

echo "SL Core LGRP 2 & 3 Link Up Test:"

do_cmd "LGRP new"              2
do_cmd "LGRP config"           5

do_cmd "LINK new"              8
do_cmd "LINK config"          17
do_cmd "LINK up"              12
do_cmd "LINK state"           10

do_cmd "MAC new"              19
do_cmd "MAC RX start"         24
do_cmd "MAC RX state"         23
do_cmd "MAC TX start"         21
do_cmd "MAC TX state"         20

do_cmd "LLR new"              27
do_cmd "LLR config"           29
do_cmd "LLR setup"            32
do_cmd "LLR start"            34
do_cmd "LLR state"            30

do_cmd "LLR stop"             34
do_cmd "LLR state"            30

do_cmd "MAC TX stop"          22
do_cmd "MAC TX state"         20
do_cmd "MAC RX stop"          25
do_cmd "MAC RX state"         23

do_cmd "LINK down"            13

echo "PASS"

do_cleanup

exit 0
