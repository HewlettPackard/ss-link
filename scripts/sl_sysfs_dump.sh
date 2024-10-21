#!/usr/bin/bash

## HELP: cmd [start] [end]

LGRP_MIN=0
LGRP_MAX=63

if [ "$#" -eq 1 ] ; then
    START=$1
    END=$1
elif [ "$#" -eq 2 ] ; then
    START=$1
    END=$2
else
    START=${LGRP_MIN}
    END=${LGRP_MAX}
fi

list() {
    local PATH=$1
    local INDENT=$2

    ##printf "PATH=${PATH}\n"
    if [ ! -e ${PATH} ] ; then
       return;
    fi

    NAME=`/usr/bin/basename ${PATH}`
    if [ ! "${INDENT}" == "0" ] ; then
        printf "%${INDENT}s" " "
    fi
    printf "${NAME}\n"

    ((INDENT += 2))

    for FILE in "${PATH}"/* ; do
        if [ -d ${FILE} ] ; then
            continue
        fi
        NAME=`/usr/bin/basename ${FILE}`
        WIDTH=$((30-INDENT))
        printf "%${INDENT}s" " "
        printf "%-${WIDTH}s" "${NAME}"
        /usr/bin/cat ${FILE}
    done

    for FILE in "${PATH}"/* ; do
        if [ ! -d ${FILE} ] ; then
            continue
        fi
        list ${FILE} ${INDENT}
    done
}

for LGRP in $(/usr/bin/seq ${START} ${END}) ; do
    if [ -e /sys/class/rossw ] ; then
        list /sys/class/rossw/rossw0/pgrp/${LGRP}/config 0
        list /sys/class/rossw/rossw0/pgrp/${LGRP}/media 0
        list /sys/class/rossw/rossw0/pgrp/${LGRP}/serdes 0
        for LINK in $(/usr/bin/seq 0 3) ; do
            list /sys/class/rossw/rossw0/pgrp/${LGRP}/port/${LINK}/link 0
            list /sys/class/rossw/rossw0/pgrp/${LGRP}/port/${LINK}/mac 0
            list /sys/class/rossw/rossw0/pgrp/${LGRP}/port/${LINK}/llr 0
        done
    fi
    if [ -e /sys/class/cxi ] ; then
        list /sys/class/cxi/cxi0/device/port/${LGRP}/config 0
        list /sys/class/cxi/cxi0/device/port/${LGRP}/media 0
        list /sys/class/cxi/cxi0/device/port/${LGRP}/serdes 0
        list /sys/class/cxi/cxi0/device/port/${LGRP}/link 0
        list /sys/class/cxi/cxi0/device/port/${LGRP}/mac 0
        list /sys/class/cxi/cxi0/device/port/${LGRP}/llr 0
    fi
done

