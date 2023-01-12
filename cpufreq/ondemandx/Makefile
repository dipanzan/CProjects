GOVERNOR_ONDEMANDX := ondemandx
GOVERNOR_DEFAULT := ondemand
SHELL := /bin/bash
KVER := $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build
PWD := $(shell pwd)
NBPROC ?= $(shell cat /proc/cpuinfo | grep processor | wc -l)
CFLAGS := -gw
obj-m += $(GOVERNOR_ONDEMANDX).o

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
	sudo insmod $(GOVERNOR_ONDEMANDX).ko

remove:
	sudo rmmod $(GOVERNOR_ONDEMANDX)

log:
	sudo dmesg -w

tail:
	tail -fn 1000 /var/log/kern.log

set:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           	sudo cpufreq-set -c $$number -g $(GOVERNOR_ONDEMANDX) ; \
		   ((number = number + 1)) ; \
	done

unset:
	@number=0 ; while [[ $$number -lt ${NBPROC} ]] ; do \
	           sudo cpufreq-set -c $$number -g $(GOVERNOR_DEFAULT) ; \
		   ((number = number + 1)) ; \
	done

init:
	$(MAKE) build
	$(MAKE) install
	$(MAKE) set

dinit:
	$(MAKE) unset
	$(MAKE) remove
	$(MAKE) clean