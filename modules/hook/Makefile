MODULE_NAME := hook-sys
SHELL := /bin/bash
KVER := $(shell uname -r)
KVER_COMPILED_AGAINST := 5.15.0-57-generic
KDIR ?= /lib/modules/$(KVER_COMPILED_AGAINST)/build
PWD := $(shell pwd)
CFLAGS := -gw
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
	sudo insmod $(MODULE_NAME).ko

remove:
	sudo rmmod $(MODULE_NAME)

clear:
	sudo dmesg --clear

init:
	$(MAKE) build
	$(MAKE) clear
	$(MAKE) install

dinit:
	$(MAKE) clear
	$(MAKE) remove
	$(MAKE) clean