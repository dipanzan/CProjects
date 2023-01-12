DEFAULT_GOVERNOR := ondemand
GOVERNOR := userspacex
SHELL := /bin/bash
KVER := $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build
PWD := $(shell pwd)
NBPROC ?= $(shell cat /proc/cpuinfo | grep processor | wc -l)
CFLAGS := -gw

obj-m += $(GOVERNOR).o

build:
	$(MAKE) -C $(CLFAGS) $(KDIR) M=$(PWD)

clean:
	echo $(KDIR)
	# clean root directory
	rm -Rf ./.tmp_versions
	rm -f ./*.o
	rm -f ./.*.o.cmd
	rm -f ./*.order
	rm -f ./*.symvers
	# clean source directory
	rm -f ./*.ko
	rm -f ./*.o
	rm -f ./*.mod.c
	rm -f ./.*.ko.cmd
	rm -f ./*.mod.o
	rm -f ./.*.o.cmd
	rm -f ./*.mod

install:
	sudo insmod $(GOVERNOR).ko

remove:
	sudo rmmod $(GOVERNOR)

log:
	sudo dmesg -w

tail:
	tail -fn 1000 /var/log/kern.log

set:
	sudo cpufreq-set -c 0 -g $(GOVERNOR)

set:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           	sudo cpufreq-set -c $$number -g $(GOVERNOR) ; \
		   ((number = number + 1)) ; \
	done

unset:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           sudo cpufreq-set -c $$number -g $(DEFAULT_GOVERNOR) ; \
		   ((number = number + 1)) ; \
	done