MODULE_NAME := detect-process
SHELL := /bin/bash
KVER := $(shell uname -r)
KVER_COMPILED_AGAINST := 5.15.0-57-generic
KDIR ?= /lib/modules/$(KVER)/build
PWD := $(shell pwd)
CFLAGS := -gw
PROC := ""
PID := 0
obj-m += $(MODULE_NAME).o

build:
	$(MAKE) -C $(CLFAGS) $(KDIR) M=$(PWD)

clean:
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
	sudo insmod $(MODULE_NAME).ko target_process_name=$(PROC) target_process_pid=$(PID)

remove:
	sudo rmmod $(MODULE_NAME)

clear:
	sudo dmesg --clear

dinit:
	$(MAKE) remove
	$(MAKE) clean

init:
	$(MAKE) build
	$(MAKE) install target_process_name=$(PROC) target_process_pid=$(PID)