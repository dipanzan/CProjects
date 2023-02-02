#!/bin/bash
hostname=( `cat "$(pwd)/cluster.txt"`)
for ((i=0; i<${#hostname[@]}; i++));
  do
    h=$(awk -F'.' '{ for(i=1;i<=NF;i++) print $i }' <<< ${hostname[i]} |head -1)
    ip=$( host ${hostname[i]} | awk '{ print $4 }')
    ssh root@${hostname[i]} "echo "$h" > /etc/hostname"
    echo "$ip $h" >> "$(pwd)/hosts"
  done
for host in ${hostname[@]}
 do
  scp hosts root@$host:/etc/hosts
  scp cluster.txt postdeploy-smarth-governor.sh buildSmartGovernor.sh root@$host:/root/
 done
rm $(pwd)/hosts
