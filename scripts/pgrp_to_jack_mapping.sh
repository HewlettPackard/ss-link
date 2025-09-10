#!/bin/bash

switch_sysfs_path="/sys/class/rossw"
nic_sysfs_path="/sys/class/cxi"
jack_sysfs_path="media/jack_num"
process_pgrp_dirs() {
        local base_dir="$1"
        local sub_path="$2"

        for dir in "$base_dir"/*; do
                dynamic_dir=$(basename "$dir")

                next_level_path="$dir/$sub_path"

                if [[ -d "$next_level_path" ]]; then
                        for subdir in "$next_level_path"/*; do
                                pgrp_num=$(basename "$subdir")

                                jack_num_path="$subdir/$jack_sysfs_path"

                                if [[ -f "$jack_num_path" ]]; then
                                        jack_num=$(cat "$jack_num_path")

                                        printf "%10s | %10s\n" "$pgrp_num" "$jack_num"
                                else
                                        printf "%10s | %10s\n" "$pgrp_num" "N/A"
                                fi
                        done
                fi
        done
}

if [[ -d "$switch_sysfs_path" ]]; then
        printf "%10s | %10s\n" "PGRP_NUM" "JACK_NUM"
        printf "%s\n" "--------------------------------------------"

        process_pgrp_dirs "$switch_sysfs_path" "pgrp"
elif [[ -d "$nic_sysfs_path" ]]; then
        printf "%10s | %10s\n" "PGRP_NUM" "JACK_NUM"
        printf "%s\n" "--------------------------------------------"

        process_pgrp_dirs "$nic_sysfs_path" "device/port"
else
        echo "no sysfs nodes"
        exit 1
fi

