#!/bin/bash

# Should be run as root
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

# Install required packages per distro and enable cpufreq polices
if [ -f /etc/debian_version ]; then
    echo "This is debian based distro"
    export DEBIAN_FRONTEND=noninteractive
  	apt-get update
  	apt-get install -y build-essential automake gfortran sysstat rsync vim cpufreqd cpufrequtils python3 ganglia-monitor screen iptables
    service ganglia-monitor start
    service ganglia-monitor status
    modprobe acpi_cpufreq
    modprobe cpufreq_ondemand
    modprobe cpufreq_ondemand
    modprobe cpufreq_powersave
    iptables -t nat -F
    iptables -t filter -F
    iptables -t mangle -F
    rm /etc/localtime
    ln -s /usr/share/zoneinfo/Europe/Paris /etc/localtime

elif [ -f /etc/redhat-release ]; then
    echo "This is RedHat based distro"
    yum install epel-release -y
    yum install -y gcc-gfortran.x86_64 sysstat dstat
  	yum groupinstall 'Development Tools' -y
    yum install rsync vim cpufrequtils sysstat python34 -y
    yum install ganglia-gmond -y
    service gmond start
    service gmond status
    modprobe acpi_cpufreq
    modprobe cpufreq_ondemand
    modprobe cpufreq_ondemand
    modprobe cpufreq_powersave
    iptables -t nat -F
    iptables -t filter -F
    iptables -t mangle -F
    rm /etc/localtime
    ln -s /usr/share/zoneinfo/Europe/Paris /etc/localtime
else
    echo "This is something else"
fi

chown -R root.root /tmp/NPB/
chmod 755 -R /tmp/NPB/

# Compile MPI
cd /tmp/NPB/mpich
./configure && make && make install
ldconfig
export LD_LIBRARY_PATH=/tmp/NPB/mpich/lib/.libs/:$LD_LIBRARY_PATH
echo 'export LD_LIBRARY_PATH=/tmp/NPB/mpich/lib/.libs/:$LD_LIBRARY_PATH' >> /root/.bashrc


# Prepare npb benchmark config
cd /tmp/NPB/npb-mpi/
cp config/suite.def.template config/suite.def
cp config/make.def.template config/make.def
sed -i -e 's/f77/mpif77/g' -e 's/cc/mpicc/g' config/make.def

# Compile benchmark per class of benchmark with different nb proceses(core). we are runing all npb with static class C.
cd /tmp/NPB/npb-mpi
# compile_bench(bench, nprocs, class)
compile_bench() {
  local bench="${1}"
  local nprocs="${2}"
  local class="${3}"
  local opt="$(cat /proc/cpuinfo | grep -i processor |wc -l)"
  make -j $opt "${bench}" NPROCS="${nprocs}" CLASS="${class}"
}


# start compilation
for class in C; do
compile_bench ft 2 "${class}"
compile_bench ft 4 "${class}"
compile_bench ft 8 "${class}"
compile_bench ft 16 "${class}"
compile_bench ft 32 "${class}"
compile_bench ft 64 "${class}"
compile_bench ft 128 "${class}"

compile_bench mg 2 "${class}"
compile_bench mg 4 "${class}"
compile_bench mg 8 "${class}"
compile_bench mg 16 "${class}"
compile_bench mg 32 "${class}"
compile_bench mg 64 "${class}"
compile_bench mg 128 "${class}"


compile_bench lu 2 "${class}"
compile_bench lu 4 "${class}"
compile_bench lu 8 "${class}"
compile_bench lu 16 "${class}"
compile_bench lu 32 "${class}"
compile_bench lu 64 "${class}"
compile_bench lu 128 "${class}"



compile_bench is 2 "${class}"
compile_bench is 4 "${class}"
compile_bench is 8 "${class}"
compile_bench is 16 "${class}"
compile_bench is 32 "${class}"
compile_bench is 64 "${class}"
compile_bench is 128 "${class}"



compile_bench ep 2 "${class}"
compile_bench ep 4 "${class}"
compile_bench ep 8 "${class}"
compile_bench ep 16 "${class}"
compile_bench ep 32 "${class}"
compile_bench ep 64 "${class}"
compile_bench ep 128 "${class}"


compile_bench sg 2 "${class}"
compile_bench sg 4 "${class}"
compile_bench sg 8 "${class}"
compile_bench sg 16 "${class}"
compile_bench sg 32 "${class}"
compile_bench sg 64 "${class}"
compile_bench sg 128 "${class}"



compile_bench sp 4 "${class}"
compile_bench sp 16 "${class}"
compile_bench sp 25 "${class}"
compile_bench sp 36 "${class}"
compile_bench sp 49 "${class}"
compile_bench sp 64 "${class}"
compile_bench sp 81 "${class}"
compile_bench sp 100 "${class}"
compile_bench sp 121 "${class}"
compile_bench sp 169 "${class}"
compile_bench sp 195 "${class}"

compile_bench bt 4 "${class}"
compile_bench bt 16 "${class}"
compile_bench bt 25 "${class}"
compile_bench bt 36 "${class}"
compile_bench bt 49 "${class}"
compile_bench bt 64 "${class}"
compile_bench bt 81 "${class}"
compile_bench bt 100 "${class}"
compile_bench bt 121 "${class}"
compile_bench bt 169 "${class}"
compile_bench bt 195 "${class}"
done

# ENV
echo 'source /opt/intel-performance-snapshoot/apsvars.sh' >> /root/.bashrc
# run sar for launching sar.sh script. scripts change the measurment rate to lower value than allowe (0.5s)
if [[ -z $(ps aux|grep -i sar.sh |grep -v 'grep') ]];then
	screen -d -m sh -c '/tmp/NPB/bin/sar.sh'
fi