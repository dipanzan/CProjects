#!/bin/bash
com="$1"
file="$2"
root="/tmp/log"
out="/tmp/out"
mkdir -p {$root,$out}
source /opt/intel-performance-snapshoot/apsvars.sh
echo 'y' | /opt/storage-snapshot/sps-stop.sh
sleep 3 
echo 'y' | /opt/storage-snapshot/sps-start.sh

$com

outDat=$(echo 'y' | /opt/storage-snapshot/sps-stop.sh | grep -i 'Stopped gathering system data.' | awk '{print $9}')

mv -f $outDat "$out/$file".dat
outHTML=$(aps --report="$root" |grep -i 'Graphical representation of this data is available in the HTML report:' | awk -F ':' '{print $2}')
mv $outHTML "$out/$file".html
