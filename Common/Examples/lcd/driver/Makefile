#
# Makefile for LCD driver and testprograms
#

CC	=	gcc

#
# Uncomment the correct line for your kernel-version!
#
# KERNEL	=	-DKERNEL20
KERNEL	=	-DKERNEL24

#
# New sleep routine ripped from radio-aimslab.c
# Try sleep_delay to maybe reduce system load. It will schedule() for
# longer delays, udelay just calls the normal udelay() function
#
#USLEEP	=	udelay
USLEEP	=	sleep_delay

INSTALLDIR = /lib/modules/`uname -r`/misc/

#
# If you want some debugging output then add -DDEBUG to the CFLAGS
# Attention: This will fill your syslog quite fast!
#
CFLAGS  =       $(KERNEL) -D__KERNEL__ -I/usr/src/linux/include -Wall \
                -Wstrict-prototypes -O2 -fomit-frame-pointer -pipe \
                -ffixed-8 -DMODULE -DMODVERSIONS -finline-functions \
                -include /usr/src/linux/include/linux/modversions.h \
		-DUSLEEP=$(USLEEP)
LDLAGS	=	-O2 -Wall -I/usr/src/linux/include -Wstrict-prototypes \
		-D__KERNEL__ -DMODULE -Xlinker \
		-include /usr/src/linux/include/linux/modversions.h
OBJS	=	lcd_module.o lcd.o

lcd.o:	lcd_module.c c_table.h
	$(CC) $(CFLAGS) -c lcd_module.c -o lcd.o

.PHONY:	install clean

install:
	[ -d $(INSTALLDIR) ] || mkdir -p $(INSTALLDIR)
	install -c -m644 lcd.o $(INSTALLDIR)

clean:
	rm -f *.o
