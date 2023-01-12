SHELL := /bin/bash		# use bash syntax

ifneq ($(KERNELRELEASE),)
obj-m	:= src/cpufreq_dvfs.o
else
KDIR	?= /lib/modules/4.13.0-36-generic/build
PWD	:= $(shell pwd)
NBPROC 	?= $(shell cat /proc/cpuinfo | grep processor | wc -l)

build:
	$(MAKE) -C $(KDIR) M=$(PWD)
clean:
	# clean root directory
	rm -Rf ./.tmp_versions
	rm -f ./*.o
	rm -f ./.*.o.cmd
	rm -f ./*.order
	rm -f ./*.symvers
	# clean source directory
	rm -f ./src/*.ko
	rm -f ./src/*.o
	rm -f ./src/*.mod.c
	rm -f ./src/.*.ko.cmd
	rm -f ./src/*.mod.o
	rm -f ./src/.*.o.cmd

install:
	sudo insmod src/cpufreq_dvfs.ko

remove:
	sudo rmmod cpufreq_dvfs

log:
	dmesg

tail:
	tail -fn 1000 /var/log/kern.log

set:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           sudo cpufreq-set -c $$number -g dvfs ; \
		   ((number = number + 1)) ; \
	done

unset:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           sudo cpufreq-set -c $$number -g ondemand ; \
		   ((number = number + 1)) ; \
	done

endif
