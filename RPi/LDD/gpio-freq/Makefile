ifneq (${KERNELRELEASE},)

	obj-m  = gpio-freq.o
else

	MODULE_DIR := $(shell pwd)
	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

#	KERNEL_DIR = /usr/local/src/linux-rpi-3.6.11
#	ARCH       = arm
#	CROSS_COMPILE = /usr/local/cross/rpi/bin/arm-linux-

	CFLAGS := -Wall -g

all: modules

modules:
	${MAKE} -C ${KERNEL_DIR} SUBDIRS=${MODULE_DIR}  modules

clean:
	rm -f *.o *.ko *.mod.c .*.o .*.ko .*.mod.c .*.cmd *~
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions

endif
