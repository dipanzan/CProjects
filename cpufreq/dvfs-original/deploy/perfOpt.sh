#!/bin/bash
tmpID=$(df -hT |grep  -i '/tmp' | awk '{print $1}')

# File system
# Nb of files
ulimit -S 4096
ulimit -H 32832
# max file size
sysctl -w fs.file-max=6544018

# EXT4 Tuning
sed -i '/tmp\s/ s/defaults/noatime/' /etc/fstab
mount -o rw,remount $tmpID
tune2fs -m 0 $tmpID

# Network

# 1MB queue for listen state
sysctl -w net.core.somaxconn=1024

# 16 MB for rcv/sen packet queue
sysctl -w net.core.rmem_max=16777216
sysctl -w net.core.wmem_max=16777216
# Mor ports
sysctl -w net.ipv4.ip_local_port_range="1024 65535"

# Start Swap when memory reaches to 90 precnent

sysctl -w vm.swappiness=10

# allow any pid to get ASMASP memory

sysctl -w vm.overcommit_memory=1
sysctl -w vm.overcommit_ratio=50
