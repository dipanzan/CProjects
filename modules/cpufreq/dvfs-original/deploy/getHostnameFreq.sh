#!/bin/bash
hostname=( `cat "$(pwd)/cluster.txt"`)
for h in ${hostname[@]}
do
        freq=$(ssh root@$h 'cat /sys/devices/system/cpu/cpu1/cpufreq/scaling_available_frequencies')
        printf "%s:%s\n" "$h" "$freq"
done
