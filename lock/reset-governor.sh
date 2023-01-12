#!/bin/bash

ONDEMAND_GOVERNOR="ondemand"

function set_governor()
{
    echo "setting governor: $1"
    sudo cpupower -c all frequency-set -g $1
    echo ""
}
set_governor $ONDEMAND_GOVERNOR


