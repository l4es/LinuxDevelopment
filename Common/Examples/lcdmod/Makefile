# User editable defaults
DFLT_BASE=0x378
DFLT_DISP_ROWS=2
DFLT_DISP_COLS=16
EXTERNAL_POWER=1
WIRING_SCHEME=WIRING_LCDMOD
# You shouldn't have to edit anything below here

DEFINES=-DDFLT_BASE=$(DFLT_BASE) -DDFLT_DISP_ROWS=$(DFLT_DISP_ROWS) \
		-DDFLT_DISP_COLS=$(DFLT_DISP_COLS) -DEXTERNAL_POWER=$(EXTERNAL_POWER) \
		-D$(WIRING_SCHEME)
# file locations
INSMOD=/sbin/insmod
LSMOD=/sbin/lsmod
RMMOD=/sbin/rmmod
GREP=/bin/grep
CC=cc -Wall
KERNEL_VERSION=$(shell uname -r | sed s/[-_].\*// )
KERNEL_SRC_DIR=$(shell find /usr/src/ -maxdepth 1 -type d -name *$(KERNEL_VERSION)* | head -1 )
DESTDIR=/lib/modules/$(shell uname -r)/misc
INCLUDE= -I.
CFLAGS= -D__KERNEL__ -I$(KERNEL_SRC_DIR)/include \
		-O2 -fomit-frame-pointer -pipe \
		-ffixed-8 -DMODULE -DMODVERSIONS -finline-functions \
		-include $(KERNEL_SRC_DIR)/include/linux/modversions.h

default: lcd.o

lcd.o: lcd_module.c
	@echo -e "- Making lcd.o"
	@echo "- KERNEL_SRC_DIR: $(KERNEL_SRC_DIR)"
	@echo "- DESTDIR: $(DESTDIR)"
	$(CC) $(CFLAGS) $(DEFINES) -c lcd_module.c -o lcd.o

install: lcd.o
	if [[ -e /dev/lcd ]]; then exit; else mknod /dev/lcd c 120 0; fi
	if [[ -d $(DESTDIR) ]]; then exit; else mkdir -p $(DESTDIR); fi
	cp lcd.o $(DESTDIR)

test: install
	if [[ `$(LSMOD)|grep lcd` ]]; then $(RMMOD) lcd; fi
	$(INSMOD) lcd
	echo "Hello LCD world!" > /dev/lcd

clean:
	rm -f lcd.o
