# $Id: Makefile,v 1.61 2009/06/13 11:51:19 mjona Exp $

KERNEL_VERSION := $(shell uname -r)
KERNEL_LEVEL := $(shell echo `echo $(KERNEL_VERSION) | cut -d. -f 1-2`)

KERNEL_SRC_DIR := $(shell if [ -d /lib/modules/$(KERNEL_VERSION)/build ]; \
	then echo "/lib/modules/$(KERNEL_VERSION)/build"; \
	elif [ -d /lib/modules/$(KERNEL_VERSION)/source ]; \
	then echo "/lib/modules/$(KERNEL_VERSION)/source"; \
	elif [ -d /usr/src/linux-$(KERNEL_VERSION) ]; \
	then echo "/usr/src/linux-$(KERNEL_VERSION)"; \
	else echo "/usr/src/linux"; fi)

DESTDIR := /lib/modules/$(KERNEL_VERSION)/misc

MODULES_CONF := $(shell if [ -f /etc/conf.modules ]; \
	then echo "/etc/conf.modules"; \
	else echo "/etc/modules.conf"; fi)

CC := gcc

CPPFLAGS := -D__KERNEL__ -DMODULE
CPPFLAGS += -I$(KERNEL_SRC_DIR)/include -I$(TOPDIR)/include

CFLAGS := $(CPPFLAGS) -Wall -Wstrict-prototypes -Wno-trigraphs -O2 \
	  -fno-common -fomit-frame-pointer -pipe

# use '-fno-strict-aliasing', but only if the compiler can take it
CFLAGS += $(shell if $(CC) -fno-strict-aliasing -S -o /dev/null -xc /dev/null >/dev/null 2>&1; \
	then echo "-fno-strict-aliasing"; fi)

MODFLAGS := $(shell if [ -f $(KERNEL_SRC_DIR)/include/linux/modversions.h ]; \
	then echo "-DMODVERSIONS -include $(KERNEL_SRC_DIR)/include/linux/modversions.h"; fi)

# Supported drivers
DRIVERS := hd44780


# Targets

%.o: %.c

%.o: %.c %.ver
	$(CC) $(CFLAGS) $(MODFLAGS) -include "$(TOPDIR)/include/lcd-linux.ver" -include "$(TOPDIR)/include/`basename $@ .o`.ver" -DEXPORT_SYMTAB -c $< -o $(TOPDIR)/modules/$@

%.ko: %.c
	@echo "obj-m := `basename $@ .ko`.o" > Makefile
	$(MAKE) -C $(KERNEL_SRC_DIR) M=`pwd` modules
	@mv $@ $(TOPDIR)/modules

%.ver: %.c
ifeq (foo$(MODFLAGS), foo)
	touch $(TOPDIR)/include/$@
else
  ifeq ($(KERNEL_LEVEL), 2.0)
	@$(CC) $(CPPFLAGS) -D__GENKSYMS__ -E $< | /sbin/genksyms $(TOPDIR)/include
	@mv $(TOPDIR)/include/lcd-linux.ver lcd-linux.ver.tmp
	@sed -e 's/-/_/g' lcd-linux.ver.tmp > $(TOPDIR)/include/lcd-linux.ver
	@rm lcd-linux.ver.tmp
  else
	@$(CC) $(CPPFLAGS) -D__GENKSYMS__ -E $< | /sbin/genksyms -k `echo $(KERNEL_VERSION) | cut -d'-' -f1` > $(TOPDIR)/include/$@
  endif
endif

default:
	@echo "- KERNEL_SRC_DIR: $(KERNEL_SRC_DIR)"
	@echo "- DESTDIR: $(DESTDIR)"
ifeq ($(KERNEL_LEVEL), 2.6)
	$(MAKE) TOPDIR=`pwd` kernheaders_install
	$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C lcd-linux lcd-linux.ko
	for i in $(DRIVERS); do \
		if [ -f lcd-linux/Module.symvers ]; then \
			cp -f lcd-linux/Module.symvers drivers/$$i; \
		fi; \
		$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C drivers/$$i $$i.ko; \
	done
	$(MAKE) TOPDIR=`pwd` kernheaders_remove
else
	$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C lcd-linux lcd-linux.o
	for i in $(DRIVERS); do \
		$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C drivers/$$i $$i.o; \
	done
endif

lcdcon: default
ifeq ($(KERNEL_LEVEL), 2.6)
	$(MAKE) TOPDIR=`pwd` kernheaders_install
	if [ -f lcd-linux/Module.symvers ]; then \
		cp -f lcd-linux/Module.symvers lcdcon; \
	fi
	$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C lcdcon lcdcon.ko
	$(MAKE) TOPDIR=`pwd` kernheaders_remove
else
	$(MAKE) TOPDIR=`pwd` -f `pwd`/Makefile -C lcdcon lcdcon.o
endif

devices:
ifeq ($(KERNEL_LEVEL), 2.6)
	if [ -d /etc/udev/rules.d ] && [ ! -f /etc/udev/rules.d/60-lcd-linux.rules ]; then \
		cp lcd-linux/60-lcd-linux.rules /etc/udev/rules.d; \
	fi
else
	for i in 0 1 2 3 4 5 6 7; do \
		mknod /dev/lcd$$i c 120 $$i; \
	done
	ln -fs /dev/lcd0 /dev/lcd
endif

install: default
	@if [ ! -c /dev/lcd ]; then \
		$(MAKE) devices; \
	fi
	@if [ ! -d $(DESTDIR) ]; then \
		mkdir -p $(DESTDIR); \
		echo -e "- created directory \"$(DESTDIR)\""; \
	fi
ifeq ($(KERNEL_LEVEL), 2.6)
	mv -f modules/lcd-linux.ko modules/hd44780.ko $(DESTDIR)
else
	mv -f modules/lcd-linux.o modules/hd44780.o $(DESTDIR)
	@if [ -e "$(MODULES_CONF)" -a -z "`grep lcd-linux $(MODULES_CONF)`" ]; then \
		echo "alias char-major-120 lcd-linux" >> $(MODULES_CONF); \
		echo "- entry \"alias char-major-120 lcd-linux\" added in $(MODULES_CONF)"; \
	fi
endif
	/sbin/depmod -a

uninstall:
	rm -f /bin/lcd-console
	rm -f $(DESTDIR)/lcd-linux.* $(DESTDIR)/lcdcon.*;
	for i in $(DRIVERS); do rm -f $(DESTDIR)/$$i.*; done;
ifeq ($(KERNEL_LEVEL), 2.6)
	if [ -d /etc/udev/rules.d ]; then \
		rm -f /etc/udev/rules.d/60-lcd-linux.rules; \
	fi
else
	@if [ -e $(MODULES_CONF) -a -n "`grep lcd-linux $(MODULES_CONF)`" ]; then \
		cp -f $(MODULES_CONF) $(MODULES_CONF).lcdsave; \
		echo "- entry \"alias char-major-120 lcd-linux\" deleted from file $(MODULES_CONF)"; \
		echo "- old file $(MODULES_CONF) saved in $(MODULES_CONF).lcdsave"; \
		grep -v "lcd-linux" $(MODULES_CONF).lcdsave > $(MODULES_CONF); \
	fi
endif
	/sbin/depmod -a

test:
	$(MAKE) TOPDIR=`pwd` -f ../Makefile -C examples _test

_test:
	$(CC) -Wall -O2 -I$(TOPDIR)/include lcd-console.c -o ../lcd-console
	$(CC) -Wall -O2 test_read.c -o ../test_read
	$(CC) -Wall -O2 test_write.c -o ../test_write
	$(CC) -Wall -O2 -I$(TOPDIR)/include test_ioctl.c -o ../test_ioctl

testmod:
	if [ -n "`/sbin/lsmod|grep hd44780`" ]; then /sbin/rmmod hd44780; fi
	/sbin/modprobe hd44780
	echo -n "Hello LCD world!" > /dev/lcd

clean:
ifeq ($(KERNEL_LEVEL), 2.6)
	@if [ -f lcd-linux/Makefile ]; then $(MAKE) -C $(KERNEL_SRC_DIR) M=`pwd`/lcd-linux clean; fi
	@if [ -f lcdcon/Makefile ]; then $(MAKE) -C $(KERNEL_SRC_DIR) M=`pwd`/lcdcon clean; fi
	@for i in $(DRIVERS); do \
		if [ -f drivers/$$i/Makefile ]; then \
			$(MAKE) -C $(KERNEL_SRC_DIR) M=`pwd`/drivers/$$i clean; \
		fi; \
	done
	@if [ -f examples/Makefile ]; then $(MAKE) -C $(KERNEL_SRC_DIR) M=`pwd`/examples clean; fi
	rm -f examples/Makefile lcd-linux/Makefile lcdcon/Makefile drivers/*/Makefile modules/*.ko test_* lcd-console
	find . -name "*symvers" -exec rm -f {} \;
	find . -name "Module.markers" -exec rm -f {} \;
	find . -name "modules.order" -exec rm -f {} \;
else
	rm -f drivers/*/*.o drivers/*/.depend drivers/*/.config
	rm -f lcd-linux/*.o lcd-linux/.depend lcd-linux/.config
	rm -f modules/*.o include/*.ver *.o .depend .config test_* lcd-console
endif

distclean: clean headers_remove

release: distclean
	rm -f ../excluded-files.txt
	for i in drivers/hd44780/hd44780.c \
		 drivers/skeleton/skeleton.c \
		 lcd-linux/lcd-linux.c \
		 include/linux/lcd-linux.h; do \
		grep -v 'Linux_2.0_support' $$i > $$i.tmp; \
		mv $$i $$i.orig; \
		mv $$i.tmp $$i; \
		echo "lcd-linux-$(shell grep LCD_LINUX_VERSION include/linux/lcd-linux.h | cut -f2 -d\")/$$i.orig" >> ../excluded-files.txt; \
	done
	$(MAKE) -C .. -f $(shell basename `pwd`)/Makefile \
	LCD_LINUX_VER=`grep LCD_LINUX_VERSION include/linux/lcd-linux.h | cut -f2 -d\"` \
	SRC_DIR=$(shell basename `pwd`) _release
	for i in drivers/hd44780/hd44780.c \
		 drivers/skeleton/skeleton.c \
		 lcd-linux/lcd-linux.c \
		 include/linux/lcd-linux.h; do \
		mv $$i.orig $$i; \
	done
	rm -f ../excluded-files.txt

_release:
	rm -rf lcd-linux-$(LCD_LINUX_VER)
	mv $(SRC_DIR) lcd-linux-$(LCD_LINUX_VER)
	echo "lcd-linux-$(LCD_LINUX_VER)/lcd-linux/list-compat.h" >> ./excluded-files.txt
	echo "lcd-linux-$(LCD_LINUX_VER)/examples/test_lcd-linux.c" >> ./excluded-files.txt
	tar czf lcd-linux-$(LCD_LINUX_VER).tar.gz lcd-linux-$(LCD_LINUX_VER) --exclude CVS --exclude testing -X ./excluded-files.txt
	mv lcd-linux-$(LCD_LINUX_VER) $(SRC_DIR)

source_install:
	@if [ ! -d $(KERNEL_SRC_DIR)/drivers/lcd-linux ]; then \
		mkdir -p $(KERNEL_SRC_DIR)/drivers/lcd-linux; \
		echo -e "- created directory \"$(KERNEL_SRC_DIR)/drivers/lcd-linux\""; \
	fi
	cp -f include/linux/*.h $(KERNEL_SRC_DIR)/include/linux
	cp -rf lcd-linux/* $(KERNEL_SRC_DIR)/drivers/lcd-linux
	cp -rf drivers/hd44780/* $(KERNEL_SRC_DIR)/drivers/lcd-linux

headers_install:
	rm -f .installed_headers.txt
	for i in `find include -name "*.h"`; do \
		cp -f $$i /usr/$$i; \
		echo /usr/$$i >> .installed_headers.txt; \
	done

headers_remove:
	if [ -f .installed_headers.txt ]; then \
		rm -f `cat .installed_headers.txt`; \
		rm -f .installed_headers.txt; \
	fi

kernheaders_install:
	@cp -f $(TOPDIR)/include/linux/lcd-linux.h $(KERNEL_SRC_DIR)/include/linux
	@for i in $(DRIVERS); do \
		if [ -f $(TOPDIR)/include/linux/$${i}.h ]; then \
			cp -f $(TOPDIR)/include/linux/$${i}.h $(KERNEL_SRC_DIR)/include/linux; \
		fi; \
	done

kernheaders_remove:
	@rm -f $(KERNEL_SRC_DIR)/include/linux/lcd-linux.h
	@for i in $(DRIVERS); do \
		rm -f $(KERNEL_SRC_DIR)/include/linux/$${i}.h; \
	done
