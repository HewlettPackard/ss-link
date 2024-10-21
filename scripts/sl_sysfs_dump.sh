#!/usr/bin/bash

thick_line() {
    for x in `seq 0 59` ; do
        printf "="
    done
    printf "\n"
}

thin_line() {
    printf " "
    for x in `seq 0 67` ; do
        printf "-"
    done
    printf "\n"
}

## Cassini

dump_cassini() {
    NUM=$1
    LGRP=$2/port/0

    thick_line

    printf "cassini:                      %s\n" $NUM
    printf "mod_ver:                      %s\n" `cat $LGRP/sl_info/mod_ver`
    printf "mod_hash:                     %s\n" `cat $LGRP/sl_info/mod_hash`

    printf "lgrp\n"
    printf " config\n"
    printf "  fec map:                    %s\n" `cat $LGRP/config/fec_map`
    printf "  tech map:                   %s\n" `cat $LGRP/config/tech_map`
    printf "  mfs:                        %s\n" `cat $LGRP/config/mfs`
    printf "  furcation:                  %s\n" `cat $LGRP/config/furcation`
    printf "  fec mode:                   %s\n" `cat $LGRP/config/fec_mode`
    printf "  err_trace_enable:           %s\n" `cat $LGRP/config/err_trace_enable`
    printf " policies\n"
    printf "  fabric_link:                %s\n" `cat ${PGRP_PATH}/policies/fabric_link`
    printf "  r1_partner:                 %s\n" `cat ${PGRP_PATH}/policies/r1_partner`

    printf "link\n"
    printf " state:                       %s\n" `cat $LGRP/link/state`
    printf " up_count:                    %s\n" `cat $LGRP/link/up_count`
    printf " up_time:                     %sms\n" `cat $LGRP/link/up_time_ms`
    printf " total_time:                  %sms\n" `cat $LGRP/link/total_time_ms`
    printf " speed:                       "; cat $LGRP/link/speed
    printf " config\n"
    printf "  fec_up_settle_wait:         %sms\n" `cat $LGRP/link/config/fec_up_settle_wait`
    printf "  fec_up_check_wait:          %sms\n" `cat $LGRP/link/config/fec_up_check_wait`
    printf "  fec_up_ucw_limit:           %s\n" `cat $LGRP/link/config/fec_up_ucw_limit`
    printf "  fec_up_ccw_limit:           %s\n" `cat $LGRP/link/config/fec_up_ccw_limit`
    printf "  lock:                       %s\n" `cat $LGRP/link/config/lock`
    printf "  pause:                      %s\n" `cat $LGRP/link/config/pause`
    printf "  hpe:                        %s\n" `cat $LGRP/link/config/hpe`
    printf "  autoneg:                    %s\n" `cat $LGRP/link/config/autoneg`
    printf "  loopback:                   %s\n" `cat $LGRP/link/config/loopback`
    printf " policy\n"
    printf "  fec_mon_period:             %sms\n" `cat $LGRP/link/policy/fec_mon_period_ms`
    printf "  fec_mon_ucw_down_limit:     %s\n" `cat $LGRP/link/policy/fec_mon_ucw_down_limit`
    printf "  fec_mon_ucw_warn_limit:     %s\n" `cat $LGRP/link/policy/fec_mon_ucw_warn_limit`
    printf "  fec_mon_ccw_down_limit:     %s\n" `cat $LGRP/link/policy/fec_mon_ccw_down_limit`
    printf "  fec_mon_ccw_warn_limit:     %s\n" `cat $LGRP/link/policy/fec_mon_ccw_warn_limit`
    printf "  lock:                       %s\n" `cat $LGRP/link/policy/lock`
    printf "  keep_serdes_up:             %s\n" `cat $LGRP/link/policy/keep_serdes_up`
    printf " last_down_cause:             %s\n" `cat $LGRP/link/last_down_cause`
    printf " last_down_time               %s\n" `cat $LGRP/link/last_down_time`
    printf " fec current\n"
    printf "  ccw:                        %s\n" `cat $LGRP/link/fec/current/ccw`
    printf "  ucw:                        %s\n" `cat $LGRP/link/fec/current/ucw`
    printf "  gcw:                        %s\n" `cat $LGRP/link/fec/current/gcw`
    printf " fec up\n"
    printf "  ccw:                        %s\n" `cat $LGRP/link/fec/up/ccw`
    printf "  ucw:                        %s\n" `cat $LGRP/link/fec/up/ucw`
    printf "  gcw:                        %s\n" `cat $LGRP/link/fec/up/gcw`
    printf " fec down\n"
    printf "  ccw:                        %s\n" `cat $LGRP/link/fec/down/ccw`
    printf "  ucw:                        %s\n" `cat $LGRP/link/fec/down/ucw`
    printf "  gcw:                        %s\n" `cat $LGRP/link/fec/down/gcw`

    printf "mac\n"
    printf " rx_state:                    %s\n" `cat $LGRP/mac/rx_state`
    printf " tx_state:                    %s\n" `cat $LGRP/mac/tx_state`

    printf "llr\n"
    printf " state:                       %s\n" `cat $LGRP/llr/state`
    printf " policies\n"
    printf "  infinite tries:             %s\n" `cat $LGRP/llr/policies/infinite_tries`
    printf " config\n"
    printf "  setup timeout:              %sms\n" `cat $LGRP/llr/config/setup_timeout_ms`
    printf "  start timeout:              %sms\n" `cat $LGRP/llr/config/start_timeout_ms`
    printf "  down behavior:              %s\n" `cat $LGRP/llr/config/down_behavior`
    printf " loop time\n"
    printf "  calc:                       %sns\n" `cat $LGRP/llr/loop/calc_ns`
    printf "  min:                        %sns\n" `cat $LGRP/llr/loop/min_ns`
    printf "  max:                        %sns\n" `cat $LGRP/llr/loop/max_ns`
    printf "  average:                    %sns\n" `cat $LGRP/llr/loop/average_ns`
    for LOOP in `seq 0 9` ; do
        printf "  measurement $LOOP:                %sns\n" `cat $LGRP/llr/loop/time_${LOOP}_ns`
    done

    printf "media\n"
    printf " state:                       %s\n" `cat $LGRP/media/state`
    printf " vendor:                      %s\n" `cat $LGRP/media/vendor`
    printf " type:                        %s\n" `cat $LGRP/media/type`
    printf " length_cm:                   %s\n" `cat $LGRP/media/length_cm`
    printf " max_speed:                   %s\n" `cat $LGRP/media/max_speed`
    printf " speeds:\n"
    for subdir in $LGRP/media/speeds/* ; do
            if [ -d "$subdir" ]; then
                    printf "    %s:\n" "$(basename "$subdir")"
                    for file in "$subdir"/* ; do
                            printf "      %s: %s\n" "$(basename "$file")" "$(cat "$file")"
                    done
            fi
    done
    printf " serial_num:                  %s\n" `cat $LGRP/media/serial_num`
    printf " hpe_part_num:                %s\n" `cat $LGRP/media/hpe_part_num`
    printf " jack_num:                    %s\n" `cat $LGRP/media/jack_num`
    printf " jack_type:                   %s\n" `cat $LGRP/media/jack_type`
    printf " furcation:                   %s\n" `cat $LGRP/media/furcation`
    printf " supported:                   %s\n" `cat $LGRP/media/supported`
    
    printf "serdes\n"
    printf " state:                       %s\n" `cat $LGRP/serdes/state`
    printf " fw signature:                %s\n" `cat $LGRP/serdes/fw_signature`
    printf " fw version:                  %s\n" `cat $LGRP/serdes/fw_version`
    printf " hw rev 1:                    %s\n" `cat $LGRP/serdes/hw_rev_1`
    printf " hw rev 2:                    %s\n" `cat $LGRP/serdes/hw_rev_2`
    printf " hw version:                  %s\n" `cat $LGRP/serdes/hw_version`

    declare -a lane_info=(
        state/rx
        state/tx
        settings/pre1
        settings/pre2
        settings/pre3
        settings/cursor
        settings/post1
        settings/post2
        settings/media
        settings/osr
        settings/encoding
        settings/clocking
        settings/width
        settings/dfe
        settings/scramble
        settings/link_training
        eye/limit_high
        eye/limit_low
        eye/value_lower
        eye/value_upper
    )

    thin_line
    printf " %-30s  " "lane"
    for LANE in `seq 0 3` ; do
        printf "%9s" $LANE
    done
    printf "\n";
    thin_line
    for INFO in ${lane_info[@]} ; do
        printf "  %-30s " "$INFO:"
        for LANE in `seq 0 3` ; do
            printf "%9s" `cat $LGRP/serdes/$LANE/$INFO`
        done
        printf "\n"
    done
    thick_line
}

TOP=/sys/class/cxi
if [ -d ${TOP} ] ; then
    for CXI in `seq 0 15` ; do
        PARENT=${TOP}/cxi${CXI}/device
        ##echo "$PARENT"
        if [ -e ${PARENT} ] ; then
            dump_cassini ${CXI} ${PARENT}
        fi
    done
fi

## Rosetta

dump_rosetta_pgrp_serdes_lane() {
    local LANE_NUM=$1
    local LANE_PATH=$2/serdes/${LANE_NUM}

    printf "  serdes lane: ${LANE_NUM}\n"

    declare -a pgrp_serdes_lane_settings=(
        clocking
        dfe
        encoding
        cursor
        media
        osr
        post1
        post2
        pre1
        pre2
        pre3
        scramble
        width
	link_training
    )

    printf "   settings\n"
    for CONFIG in ${pgrp_serdes_lane_settings[@]} ; do
        printf "    %-30s " "${CONFIG}:"
        cat ${LANE_PATH}/settings/${CONFIG}
    done

    declare -a pgrp_serdes_lane_eye=(
        limit_high
        limit_low
        value_lower
        value_upper
    )

    printf "   eye\n"
    for EYE in ${pgrp_serdes_lane_eye[@]} ; do
        printf "    %-30s " "${EYE}:"
        cat ${LANE_PATH}/eye/${EYE}
    done

    declare -a pgrp_serdes_lane_state=(
        rx
        tx
    )

    printf "   state\n"
    for STATE in ${pgrp_serdes_lane_state[@]} ; do
        printf "    %-30s " "${STATE}:"
        cat ${LANE_PATH}/state/${STATE}
    done
}

dump_rossetta_pgrp() {
    local PGRP_NUM=$1
    local PGRP_PATH=$2

    printf "  port group: ${PGRP_NUM}\n"

    declare -a pgrp_config=(
        fec_map
        tech_map
        furcation
        mfs
        fec_mode
    )

    printf "   config\n"
    for CONFIG in ${pgrp_config[@]} ; do
        printf "    %-30s " "${CONFIG}:"
        cat ${PGRP_PATH}/config/${CONFIG}
    done

    printf "   policies\n"
    printf "    fabric_link:                   "; cat ${PGRP_PATH}/policies/fabric_link
    printf "    r1_partner:                    "; cat ${PGRP_PATH}/policies/r1_partner

    declare -a pgrp_media=(
        furcation
        hpe_part_num
        jack_num
        jack_type
        length
        max_speed
        serial_num
        speeds
        state
        supported
        type
        vendor
    )

    printf "   media\n"
    for MEDIA in ${pgrp_media[@]} ; do
        printf "    %-30s " "${MEDIA}:"
        cat ${PGRP_PATH}/media/${MEDIA}
    done

    declare -a pgrp_serdes_info=(
        hw_rev_1
        hw_rev_2
        hw_version
        fw_signature
        fw_version
        state
    )

    printf "   serdes\n"
    for INFO in ${pgrp_serdes_info[@]} ; do
        printf "    %-30s " "${INFO}:"
        cat ${PGRP_PATH}/serdes/${INFO}
    done

    for LANE in `seq 0 3` ; do
        dump_rosetta_pgrp_serdes_lane ${LANE} ${PGRP_PATH}
    done
}

dump_rossetta_link() {
    local LINK_PATH=$1
    local NUM=$2

    printf "  link: ${NUM}\n"
    printf "   mac\n"
    printf "    state\n"
    printf "     rx:                           %s\n" `cat $LINK_PATH/mac/rx_state`
    printf "     tx:                           %s\n" `cat $LINK_PATH/mac/tx_state`

    printf "   llr\n"
    printf "    state:                         %s\n" `cat $LINK_PATH/llr/state`
    printf "    policies\n"
    printf "     infinite tries:               %s\n" `cat $LINK_PATH/llr/policies/infinite_tries`
    printf "    config\n"
    printf "     setup timeout:                %sms\n" `cat $LINK_PATH/llr/config/setup_timeout_ms`
    printf "     start timeout:                %sms\n" `cat $LINK_PATH/llr/config/start_timeout_ms`
    printf "     down behavior:                %s\n" `cat $LINK_PATH/llr/config/down_behavior`
    printf "    loop time\n"
    printf "     calc:                         %sns\n" `cat $LINK_PATH/llr/loop/calc_ns`
    printf "     min:                          %sns\n" `cat $LINK_PATH/llr/loop/min_ns`
    printf "     max:                          %sns\n" `cat $LINK_PATH/llr/loop/max_ns`
    printf "     average:                      %sns\n" `cat $LINK_PATH/llr/loop/average_ns`
    for LOOP in `seq 0 9` ; do
        printf "     measurement $LOOP:                %sns\n" `cat $LINK_PATH/llr/loop/time_${LOOP}_ns`
    done

    printf "  fec current data\n"
    printf "   ccw: "; cat $LINK_PATH/link/fec/current/ccw
    printf "   ucw: "; cat $LINK_PATH/link/fec/current/ucw
    printf "   gcw: "; cat $LINK_PATH/link/fec/current/gcw
    printf "   ccw tail:  bin#  value\n"
    for TAIL in `seq 0 9` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/current/tail/bin0${TAIL}
    done
    for TAIL in `seq 10 14` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/current/tail/bin${TAIL}
    done
    printf "   ccw fecl:  lane  fecl  value\n"
    for LANE in `seq 0 3` ; do
        for FECL in `seq 0 3` ; do
            printf "%12d%6d     " ${LANE} ${FECL}; cat $LINK_PATH/link/fec/current/lane/${LANE}/fecl${FECL}
        done
    done

    printf "  fec up cached data\n"
    printf "   ccw: "; cat $LINK_PATH/link/fec/up/ccw
    printf "   ucw: "; cat $LINK_PATH/link/fec/up/ucw
    printf "   gcw: "; cat $LINK_PATH/link/fec/up/gcw
    printf "   ccw tail:  bin#  value\n"
    for TAIL in `seq 0 9` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/up/tail/bin0${TAIL}
    done
    for TAIL in `seq 10 14` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/up/tail/bin${TAIL}
    done
    printf "   ccw fecl:  lane  fecl  value\n"
    for LANE in `seq 0 3` ; do
        for FECL in `seq 0 3` ; do
            printf "%12d%6d     " ${LANE} ${FECL}; cat $LINK_PATH/link/fec/up/lane/${LANE}/fecl${FECL}
        done
    done

    printf "  fec down cached data\n"
    printf "   ccw: "; cat $LINK_PATH/link/fec/down/ccw
    printf "   ucw: "; cat $LINK_PATH/link/fec/down/ucw
    printf "   gcw: "; cat $LINK_PATH/link/fec/down/gcw
    printf "   ccw tail:  bin#  value\n"
    for TAIL in `seq 0 9` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/down/tail/bin0${TAIL}
    done
    for TAIL in `seq 10 14` ; do
        printf "               %02d    " ${TAIL}; cat $LINK_PATH/link/fec/down/tail/bin${TAIL}
    done
    printf "   ccw fecl:  lane  fecl  value\n"
    for LANE in `seq 0 3` ; do
        for FECL in `seq 0 3` ; do
            printf "%12d%6d     " ${LANE} ${FECL}; cat $LINK_PATH/link/fec/down/lane/${LANE}/fecl${FECL}
        done
    done
}

TOP=/sys/class/rossw
if [ -d ${TOP} ] ; then
    for DEV in `seq 0 1` ; do
        ROSS=${TOP}/rossw${DEV}
        ##echo "${ROSS}"
        if [ -e ${ROSS} ] ; then
            thick_line
            printf "rossetta: %s\n" ${DEV}
            printf "  mod_ver:  "; cat ${ROSS}/sl_info/mod_ver
            printf "  mod_hash: "; cat ${ROSS}/sl_info/mod_hash
            for GRP in `seq 0 63` ; do
                PGRP=${ROSS}/pgrp/${GRP}
                ##echo "${PGRP}"
                if [ -e ${PGRP} ] ; then
                    dump_rossetta_pgrp ${GRP} ${PGRP}
                fi
                for LINK_NUM in `seq 0 3` ; do
                    LINK=${ROSS}/pgrp/${GRP}/port/${LINK_NUM}
                    ##echo "${LINK}"
                    if [ -e ${LINK} ] ; then
                        dump_rossetta_link ${LINK} ${LINK_NUM}
                    fi
                done
            done
            thick_line
        fi
    done
fi
