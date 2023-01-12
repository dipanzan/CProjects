#!/bin/bash

ONDEMAND_GOVERNOR="ondemand"
USERSPACE_GOVERNOR="userspacex"

SCALING_MIN_FREQ=400000
SCALING_MAX_FREQ=4463000

TARGET_CPU=$1

function set_governor()
{
    echo "setting governor: $1"
    sudo cpupower -c all frequency-set -g $1
    echo ""
}

function set_min_for_all_and_max_for()
{
    sudo cpupower -c all frequency-set -f $SCALING_MIN_FREQ
    sudo cpupower -c $1 frequency-set -f $SCALING_MAX_FREQ
}

set_governor $USERSPACE_GOVERNOR
set_min_for_all_and_max_for $TARGET_CPU
