/* hd44780.c
 *
 * $Id: hd44780.c,v 1.181 2010/03/03 14:56:22 mjona Exp $
 *
 * LCD-Linux:
 * Driver for HD44780 compatible displays connected to the parallel port.
 *
 * Copyright (C) 2005 - 2009  Mattia Jona-Lasinio (mjona@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/version.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a, b, c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef LINUX_VERSION_CODE
#error - LINUX_VERSION_CODE undefined in 'linux/version.h'
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
#include <generated/autoconf.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif

#if defined(MODULE) && defined(CONFIG_PARPORT_MODULE) || defined(CONFIG_PARPORT)
#define USE_PARPORT
#else
#undef USE_PARPORT
#endif

#ifdef CONFIG_PROC_FS
#define USE_PROC
#else
#undef USE_PROC
#endif

#include <linux/bitops.h>

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/delay.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <linux/init.h>

#ifdef USE_PARPORT
#include <linux/parport.h>
#else
#include <asm/io.h>
#include <linux/ioport.h>
#endif

#ifdef USE_PROC
#include <linux/proc_fs.h>
#endif

#define LCD_LINUX_MAIN
#include <linux/hd44780.h>

#include "charmap.h"
#include "commands.h"
#include "compat.h"
#include "config.h"
#include "hardware.h"

/** Function prototypes **/

/* Initialization */
static int hd44780_validate_driver(void);
static int hd44780_init_port(void);
static int hd44780_cleanup_port(void);
static int hd44780_init_display(void);
static int hd44780_cleanup_display(void);

/* Write */
static void hd44780_address_mode(int);
static void hd44780_clear_display(void);
static void hd44780_write_char(unsigned int, unsigned short);
static void hd44780_write_cgram_char(unsigned char, unsigned char *);

/* Read */
static void check_bf(unsigned long);
static void hd44780_read_char(unsigned int, unsigned short *);
static void hd44780_read_cgram_char(unsigned char, unsigned char *);

/* Input handling */
static int hd44780_handle_custom_char(unsigned int);
static int hd44780_handle_custom_ioctl(unsigned int, unsigned long, unsigned int);

/* Proc operations */
#ifdef USE_PROC
static void create_proc_entries(void);
static void remove_proc_entries(void);
#endif

static inline void __set_br(unsigned long *x, unsigned long val, unsigned long start, unsigned long len)
{
	unsigned long mask = (1 << len)-1;

	if (val <= mask) {
		mask <<= start;
		val <<= start;
		*x = (*x & ~mask) | val;
	}
}

static inline unsigned long __get_br(unsigned long x, unsigned long start, unsigned long len)
{
	return ((x >> start) & ((1 << len)-1));
}

/* hd44780_flags */
#define _CHECK_BF	0	/* Do busy-flag checking */
#define _4BITS_BUS	1	/* The bus is 4 bits long */
#define _5X10_FONT	2	/* Use 5x10 font */
#define CURSOR_BLINK	3	/* Make the cursor blinking */
#define SHOW_CURSOR	4	/* Make the cursor visible */
#define DISPLAY_ON	5	/* Display status: on or off */
#define INC_ADDR	6	/* Increment address after data read/write */
#define BACKLIGHT	7	/* Display backlight: on or off */
#if defined(USE_PARPORT) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#define PP_REVERSE	8	/* Parport state: forward or reverse */
#endif
#define CGRAM_STATE	9	/* Controller status bitmask (bits 9->15): DDRAM or CGRAM access */

#define SET_ESC_STATE(x)	__set_br(&hd44780_flags, x, 16, 8)
#define SET_PROC_LEVEL(x)	__set_br(&hd44780_flags, x, 24, 2)
#define ESC_STATE		__get_br(hd44780_flags, 16, 8)
#define PROC_LEVEL		__get_br(hd44780_flags, 24, 2)

/* globals */
static unsigned short io = DFLT_BASE;		/* I/O port address */
static unsigned int disp_size;			/* Display size (rows*columns) */
static unsigned int disp_curr;			/* Current controller being addressed */
static unsigned int disp_offset[MAX_NUM_CNTR];	/* Physical cursor position on the display */
static unsigned long hd44780_flags;		/* Driver flags for internal use only */
static unsigned long all;			/* Mask to send commands to all controllers */

#ifdef USE_PARPORT
static struct pardevice *pd;			/* Parallel port device */
#endif

static struct lcd_parameters par = {
	.name		= HD44780_STRING,
	.minor		= HD44780_MINOR,
	.flags		= DFLT_FLAGS,
	.tabstop	= DFLT_TABSTOP,
	.num_cntr	= DFLT_NUM_CNTR,
	.cntr_rows	= DFLT_CNTR_ROWS,
	.cntr_cols	= DFLT_CNTR_COLS,
	.vs_rows	= DFLT_VS_ROWS,
	.vs_cols	= DFLT_VS_COLS,
	.cgram_chars	= 8,
	.cgram_bytes	= 8,
	.cgram_char0	= 0,
};
/* End of globals */

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#include <linux/device.h>
MODULE_ALIAS_CHARDEV(LCD_MAJOR, HD44780_MINOR);
#endif
#include <linux/kmod.h>

static unsigned short flags	= DFLT_FLAGS;
static unsigned short tabstop	= DFLT_TABSTOP;
static unsigned short num_cntr	= DFLT_NUM_CNTR;
static unsigned short cntr_rows	= DFLT_CNTR_ROWS;
static unsigned short cntr_cols	= DFLT_CNTR_COLS;
static unsigned short vs_rows	= DFLT_VS_ROWS;
static unsigned short vs_cols	= DFLT_VS_COLS;
static unsigned short minor	= HD44780_MINOR;

MODULE_DESCRIPTION("LCD parallel port driver for HD44780 compatible controllers.");
MODULE_AUTHOR("Mattia Jona-Lasinio <mjona@users.sourceforge.net>");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
module_param(io,	ushort, 0444);
module_param(flags,	ushort, 0444);
module_param(num_cntr,	ushort, 0444);
module_param(cntr_rows,	ushort, 0444);
module_param(cntr_cols,	ushort, 0444);
module_param(vs_rows,	ushort, 0444);
module_param(vs_cols,	ushort, 0444);
module_param(tabstop,	ushort, 0444);
module_param(minor,	ushort, 0444);
#else
MODULE_PARM(io,		"h");
MODULE_PARM(flags,	"h");
MODULE_PARM(num_cntr,	"h");
MODULE_PARM(cntr_rows,	"h");
MODULE_PARM(cntr_cols,	"h");
MODULE_PARM(vs_rows,	"h");
MODULE_PARM(vs_cols,	"h");
MODULE_PARM(tabstop,	"h");
MODULE_PARM(minor,	"h");
#endif
MODULE_PARM_DESC(io,		"Parallel port base address (default: " string(DFLT_BASE) ")");
MODULE_PARM_DESC(flags,		"Various flags (see Documentation)");
MODULE_PARM_DESC(num_cntr,	"Number of controllers on the LCD (default: " string(DFLT_NUM_CNTR) ", max: " string(MAX_NUM_CNTR) ")");
MODULE_PARM_DESC(cntr_rows,	"Number of rows per controller on the LCD: 1, 2, 4 (default: " string(DFLT_CNTR_ROWS) ")");
MODULE_PARM_DESC(cntr_cols,	"Number of columns on the LCD (default: " string(DFLT_CNTR_COLS) ", max: " string(MAX_CNTR_COLS) ")");
MODULE_PARM_DESC(vs_rows,	"Number of rows of the virtual screen (default: " string(DFLT_VS_ROWS) ")");
MODULE_PARM_DESC(vs_cols,	"Number of columns of the virtual screen (default: " string(DFLT_VS_COLS) ")");
MODULE_PARM_DESC(tabstop,	"Tab character length (default: " string(DFLT_TABSTOP) ")");
MODULE_PARM_DESC(minor,		"Assigned minor number (default: " string(HD44780_MINOR) ")");
#else

/*
 * Parse boot command line
 *
 * hd44780=cntr_rows,cntr_cols,vs_rows,vs_cols,flags,num_cntr,io,minor,tabstop
 */
static int __init hd44780_boot_init(char *cmdline)
{
	char *str = cmdline;
	int idx = 0;
	unsigned short *args[] = {
		&par.cntr_rows,
		&par.cntr_cols,
		&par.vs_rows,
		&par.vs_cols,
		&par.flags,
		&par.num_cntr,
		&io,
		&par.minor,
		&par.tabstop,
	};

	while (*cmdline && idx < (sizeof(args)/sizeof(unsigned short *))) {
		switch (*str) {
		case ',':
			*str++ = 0;
		case 0:
			if (strlen(cmdline))
				*args[idx] = simple_strtoul(cmdline, NULL, 0);
			++idx;
			cmdline = str;
			break;
		default:
			++str;
			break;
		}
	}

	return (1);
}

__setup("hd44780=", hd44780_boot_init);
#endif /* MODULE */

/* Macros for iterator handling */
static inline unsigned int iterator_inc_(unsigned int iterator, const unsigned int module)
{
	return ((++iterator)%module);
}

static inline unsigned int iterator_dec_(unsigned int iterator, const unsigned int module)
{
	return (iterator ? --iterator : module-1);
}

#define iterator_inc(iterator, module)		(iterator = iterator_inc_(iterator, module))
#define iterator_dec(iterator, module)		(iterator = iterator_dec_(iterator, module))

/* Control signals for read and write functions */
struct hd44780_lines {
	unsigned long cntr_on;	/* Byte to output on the parport control register (enable high) */
	unsigned long cntr_off;	/* Byte to output on the parport control register (enable low) */
	unsigned long data_on;	/* Byte to output on the parport data register (enable high) */
	unsigned long data_off;	/* Byte to output on the parport data register (enable low) */
};

static inline void set_lines(unsigned long bitmask, struct hd44780_lines *s)
{
	unsigned char data_type[4] = {
		0x00	| RW,		/* INST, READ: must be the first entry */
		0x00	| 0x00,		/* INST, WRITE */
		RS	| RW,		/* DATA, READ */
		RS	| 0x00,		/* DATA, WRITE */
	};

	s->data_off = s->data_on = 0x00;
	s->cntr_off = s->cntr_on = (0x0b ^ (data_type[bitmask >> 8] |
			(test_bit(BACKLIGHT, &hd44780_flags) ? BL : 0x00)));

	switch (par.num_cntr) {
	case 7:
		if (test_bit(6, &bitmask))
			s->data_on ^= E6;
	case 6:
		if (test_bit(5, &bitmask))
			s->data_on ^= E5;
	case 5:
		if (test_bit(4, &bitmask))
			s->data_on ^= E4;
	case 4:
		if (test_bit(3, &bitmask))
			s->data_on ^= E3;
	case 3:
		if (test_bit(2, &bitmask))
			s->cntr_on ^= E2;
	case 2:
		if (test_bit(1, &bitmask))
			s->cntr_on ^= E1;
	case 1:
		if (test_bit(0, &bitmask))
			s->cntr_on ^= E0;
	}
}

/* Low level read from the display */
static inline unsigned char __read_display(unsigned long bitmask)
{
	unsigned char byte;
	struct hd44780_lines s;

	set_lines(bitmask, &s);

#if defined(USE_PARPORT) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	if (! test_bit(PP_REVERSE, &hd44780_flags)) {
		parport_data_reverse(pd->port);
		set_bit(PP_REVERSE, &hd44780_flags);
	}
#else
	set_bit(5, &s.cntr_on);
	set_bit(5, &s.cntr_off);
#endif

#ifdef USE_PARPORT
	parport_write_control(pd->port, s.cntr_off);
	ndelay(T_AS);
	parport_write_control(pd->port, s.cntr_on);
	ndelay(T_EH);
	byte = parport_read_data(pd->port);
	parport_write_control(pd->port, s.cntr_off);
#else
	outb(s.cntr_off, io+2);
	ndelay(T_AS);
	outb(s.cntr_on, io+2);
	ndelay(T_EH);
	byte = inb(io);
	outb(s.cntr_off, io+2);
#endif

	ndelay(T_EL);

	return (byte);
}

/* Low level write to the display */
static inline void __write_display(unsigned char data, unsigned long bitmask)
{
	struct hd44780_lines s;

	set_lines(bitmask, &s);

#if defined(USE_PARPORT) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	if (test_bit(PP_REVERSE, &hd44780_flags)) {
		parport_data_forward(pd->port);
		clear_bit(PP_REVERSE, &hd44780_flags);
	}
#else
	clear_bit(5, &s.cntr_on);
	clear_bit(5, &s.cntr_off);
#endif

#ifdef USE_PARPORT
	parport_write_control(pd->port, s.cntr_off);
	parport_write_data(pd->port, data);
	ndelay(T_AS);
	parport_write_control(pd->port, s.cntr_on);
	if (par.num_cntr > 3)
		parport_write_data(pd->port, s.data_on | data);
	ndelay(T_EH);
	parport_write_control(pd->port, s.cntr_off);
	if (par.num_cntr > 3)
		parport_write_data(pd->port, data);
#else
	outb(s.cntr_off, io+2);
	outb(data, io);
	ndelay(T_AS);
	outb(s.cntr_on, io+2);
	if (par.num_cntr > 3)
		outb(s.data_on | data, io);
	ndelay(T_EH);
	outb(s.cntr_off, io+2);
	if (par.num_cntr > 3)
		outb(data, io);
#endif

	ndelay(T_EL);
}

/* Read data from the display (8 bit bus, busy wait) */
static void read_display_0(unsigned char *byte, unsigned long bitmask)
{
	*byte = __read_display(bitmask);
	udelay(T_READ);
}

/* Read data from the display (8 bit bus, check busy flag) */
static void read_display_1(unsigned char *byte, unsigned long bitmask)
{
	if (bitmask >> 8)
		check_bf(bitmask);
	*byte = __read_display(bitmask);
}

/* Read data from the display (4 bit bus, busy wait) */
static void read_display_2(unsigned char *byte, unsigned long bitmask)
{
	*byte = HIGH_NIBBLE_READ(__read_display(bitmask));
	udelay(T_READ);
	*byte |= LOW_NIBBLE_READ(__read_display(bitmask));
	udelay(T_READ);
}

/* Read data from the display (4 bit bus, check busy flag) */
static void read_display_3(unsigned char *byte, unsigned long bitmask)
{
	if (bitmask >> 8)
		check_bf(bitmask);
	*byte = HIGH_NIBBLE_READ(__read_display(bitmask));
	if (bitmask >> 8)
		check_bf(bitmask);
	*byte |= LOW_NIBBLE_READ(__read_display(bitmask));
}

/* Output commands or data to the display (8 bit bus, busy wait) */
static void write_display_0(unsigned char byte, unsigned long bitmask)
{
	__write_display(byte, bitmask);
	udelay(T_WRITE);
}

/* Output commands or data to the display (8 bit bus, check busy flag) */
static void write_display_1(unsigned char byte, unsigned long bitmask)
{
	check_bf(bitmask);
	__write_display(byte, bitmask);
}

/* Output commands or data to the display (4 bit bus, busy wait) */
static void write_display_2(unsigned char byte, unsigned long bitmask)
{
	__write_display(HIGH_NIBBLE_WRITE(byte), bitmask);
	udelay(T_WRITE);
	__write_display(LOW_NIBBLE_WRITE(byte), bitmask);
	udelay(T_WRITE);
}

/* Output commands or data to the display (4 bit bus, check busy flag) */
static void write_display_3(unsigned char byte, unsigned long bitmask)
{
	check_bf(bitmask);
	__write_display(HIGH_NIBBLE_WRITE(byte), bitmask);
	check_bf(bitmask);
	__write_display(LOW_NIBBLE_WRITE(byte), bitmask);
}

static void (*read_display_tbl[4])(unsigned char *byte, unsigned long bitmask) = {
	read_display_0,
	read_display_1,
	read_display_2,
	read_display_3
};

static void (*write_display_tbl[4])(unsigned char byte, unsigned long bitmask) = {
	write_display_0,
	write_display_1,
	write_display_2,
	write_display_3
};

static void (*read_display)(unsigned char *byte, unsigned long bitmask);
static void (*write_display)(unsigned char byte, unsigned long bitmask);

static inline void set_io_functions(unsigned long mode)
{
	read_display = read_display_tbl[mode];
	write_display = write_display_tbl[mode];
}

/* Read Address Counter AC from the display */
static unsigned char read_ac(unsigned long bitmask)
{
	unsigned char byte;

	read_display(&byte, bitmask);

	return (byte);
}


/* Uncomment the following line to enable check busy flag statistics.
 * You will find the average number of busy flag polls per check
 * in the 'status' proc file.
 * Should this number be larger than (about) 5, you should increase
 * the busy flag poll time T_BF or the timing constants in config.h
 */
//#define	BF_STATS	1

#if defined(BF_STATS)
static unsigned long __cnt1, __cnt2;
#endif

static inline unsigned long cycle_check_bf(unsigned long *bitmask)
{
	unsigned int i;

#if defined(BF_STATS)
	++__cnt1;
#endif

	for (i = 0; i < par.num_cntr; ++i)
		if (test_bit(i, bitmask)) {
			unsigned long tmp = read_ac(1 << i);

			if (! test_bit(7, &tmp))
				clear_bit(i, bitmask);
		}

	return (*bitmask);
}

/* Do busy-flag check */
static void check_bf(unsigned long bitmask)
{
	unsigned int timeout = 20;

#if defined(BF_STATS)
	++__cnt2;
#endif

	bitmask &= 0xff;
	while (cycle_check_bf(&bitmask) && --timeout)
		udelay(T_BF);

	if (! timeout) {
		static unsigned char do_check_bf = 5;

		if (! --do_check_bf) {
			printk(KERN_NOTICE "hd44780: too many busy flag errors\n");
			printk(KERN_NOTICE "hd44780: disabling busy flag checking\n");
			clear_bit(_CHECK_BF, &hd44780_flags);
			set_io_functions(hd44780_flags & 3);
		}
	}
}

/* Send commands to the display */
static void write_command(unsigned char command, unsigned long bitmask)
{
	write_display(command, (1 << 8) | bitmask);

	if (command <= 0x03)
		mdelay(2);
}

/* Update display attributes */
static void update_attr(unsigned long old, unsigned long new)
{
	unsigned char old_cmd, new_cmd;

	old_cmd = new_cmd = (test_bit(DISPLAY_ON, &hd44780_flags) ? DISP_ON : DISP_OFF);

	old_cmd |= CURS_OFF;
	old_cmd |= BLINK_OFF;

	new_cmd |= (test_bit(SHOW_CURSOR, &hd44780_flags) ? CURS_ON : CURS_OFF);
	new_cmd |= (test_bit(CURSOR_BLINK, &hd44780_flags) ? BLINK_ON : BLINK_OFF);

	write_command(old_cmd, 1 << old);
	write_command(new_cmd, 1 << new);
}

static inline void set_cursor(unsigned int offset)
{
	unsigned int disp_number = offset/disp_size;
	unsigned int local_offset = offset%disp_size;

	if (disp_offset[disp_number] != local_offset || test_bit(CGRAM_STATE+disp_number, &hd44780_flags)) {
		unsigned int disp_row = local_offset/par.cntr_cols;
		unsigned int disp_column = local_offset%par.cntr_cols;

		write_command(DDRAM_IO | ((disp_row%2)*0x40) | (((disp_row >= 2)*par.cntr_cols)+disp_column), 1 << disp_number);
		clear_bit(CGRAM_STATE+disp_number, &hd44780_flags);
		disp_offset[disp_number] = local_offset;
	}

	if (disp_number != disp_curr) {
		if (test_bit(SHOW_CURSOR, &hd44780_flags) || test_bit(CURSOR_BLINK, &hd44780_flags))
			update_attr(disp_curr, disp_number);
		disp_curr = disp_number;
	}
}

/* HD44780 DDRAM addresses are consecutive only when
 * the cursor moves on the same row of the display.
 * Every time the row of the cursor changes we invalidate
 * the cursor position to force hardware cursor repositioning.
 */
static inline void mov_cursor(unsigned int disp_number)
{
	if (test_bit(INC_ADDR, &hd44780_flags)) {
		iterator_inc(disp_offset[disp_number], disp_size);
		if (disp_offset[disp_number]%par.cntr_cols == 0)
			disp_offset[disp_number] = disp_size;
	} else {
		iterator_dec(disp_offset[disp_number], disp_size);
		if (disp_offset[disp_number]%par.cntr_cols == par.cntr_cols-1)
			disp_offset[disp_number] = disp_size;
	}
}

static struct lcd_driver hd44780 = {
	.read_char		= hd44780_read_char,
	.read_cgram_char	= hd44780_read_cgram_char,
	.write_char		= hd44780_write_char,
	.write_cgram_char	= hd44780_write_cgram_char,
	.address_mode		= hd44780_address_mode,
	.clear_display		= hd44780_clear_display,
	.validate_driver	= hd44780_validate_driver,
	.init_display		= hd44780_init_display,
	.cleanup_display	= hd44780_cleanup_display,
	.init_port		= hd44780_init_port,
	.cleanup_port		= hd44780_cleanup_port,
	.handle_custom_char	= hd44780_handle_custom_char,
	.handle_custom_ioctl	= hd44780_handle_custom_ioctl,

	.charmap		= charmap,
};

static void hd44780_read_char(unsigned int offset, unsigned short *data)
{
	unsigned int disp_number = offset/disp_size;
	unsigned char tmp;

	set_cursor(offset);
	read_display(&tmp, (2 << 8) | (1 << disp_number));
	*data = tmp;
	mov_cursor(disp_number);
}

static void hd44780_read_cgram_char(unsigned char index, unsigned char *pixels)
{
	unsigned int i;

	/* In any case we read from controller 1 only since
	 * controller 2 should contain the same information.
	 */
	write_command(CGRAM_IO | (index << 3), 0x01);
	set_bit(CGRAM_STATE+0, &hd44780_flags);

	for (i = 0; i < 8; ++i) {
		read_display(pixels+i, (2 << 8) | 0x01);
		pixels[i] &= 0x1f;
	}

}

static void hd44780_write_char(unsigned int offset, unsigned short data)
{
	unsigned int disp_number = offset/disp_size;

	set_cursor(offset);
	write_display(data & 0xff, (3 << 8) | (1 << disp_number));
	mov_cursor(disp_number);
}

static void hd44780_write_cgram_char(unsigned char index, unsigned char *pixels)
{
	unsigned int i, ac;
	unsigned char *cgbuf = hd44780.cgram_buffer+(index << 3);

	switch (par.num_cntr) {
	case 7:
		set_bit(CGRAM_STATE+6, &hd44780_flags);
	case 6:
		set_bit(CGRAM_STATE+5, &hd44780_flags);
	case 5:
		set_bit(CGRAM_STATE+4, &hd44780_flags);
	case 4:
		set_bit(CGRAM_STATE+3, &hd44780_flags);
	case 3:
		set_bit(CGRAM_STATE+2, &hd44780_flags);
	case 2:
		set_bit(CGRAM_STATE+1, &hd44780_flags);
	case 1:
		set_bit(CGRAM_STATE+0, &hd44780_flags);
	}

	for (i = 0, ac = 8; i < 8; ++i) {
		pixels[i] &= 0x1f;
		if (cgbuf[i] != pixels[i]) {
			if (ac != i) {
				write_command(CGRAM_IO | (index << 3) | i, all);
				ac = i;
			}
			write_display(pixels[i], (3 << 8) | all);
			++ac;
			cgbuf[i] = pixels[i];
		}
	}
}

/* Increment/decrement address mode after a data read/write */
static void hd44780_address_mode(int mode)
{
	if (mode > 0 && ! test_bit(INC_ADDR, &hd44780_flags)) {
		write_command(CURS_INC | DISP_SHIFT_OFF, all);
		set_bit(INC_ADDR, &hd44780_flags);
	} else if (mode < 0 && test_bit(INC_ADDR, &hd44780_flags)) {
		write_command(CURS_DEC | DISP_SHIFT_OFF, all);
		clear_bit(INC_ADDR, &hd44780_flags);
	}
}

static void hd44780_clear_display(void)
{
	write_command(CLR_DISP, all);
	if (! test_bit(INC_ADDR, &hd44780_flags))
		write_command(CURS_DEC | DISP_SHIFT_OFF, all);
	memset(disp_offset, 0, sizeof(disp_offset));
}

static int hd44780_validate_driver(void)
{
	if (par.cntr_rows != 1 && par.cntr_rows != 2 && par.cntr_rows != 4)
		par.cntr_rows = DFLT_CNTR_ROWS;

	if (par.cntr_rows != 1)
		par.flags &= ~HD44780_5X10_FONT;

	if (! par.num_cntr || par.num_cntr > MAX_NUM_CNTR)
		par.num_cntr = DFLT_NUM_CNTR;

	if (par.num_cntr > 2) {
		par.flags |= HD44780_4BITS_BUS;
		par.flags &= ~HD44780_CHECK_BF;
		hd44780.read_char = NULL;
		hd44780.read_cgram_char = NULL;
	}

	if (! par.cntr_cols || par.cntr_cols > MAX_CNTR_COLS/par.cntr_rows)
		par.cntr_cols = MAX_CNTR_COLS/par.cntr_rows;

	disp_size = par.cntr_rows*par.cntr_cols;
	all = (1 << par.num_cntr)-1;

	/* These parameters depend on the hardware and cannot be changed */
	par.cgram_chars = 8;
	par.cgram_bytes = 8;
	par.cgram_char0 = 0;

	return (0);
}

/* Send init commands to the display */
static void write_init_command(void)
{
	unsigned char command = 0x30;

	set_io_functions(0); /* Force 8 bits, busy waiting */
	if (test_bit(_4BITS_BUS, &hd44780_flags))
		command = HIGH_NIBBLE_WRITE(command);
	write_command(command, all);
	mdelay(10);	/* Wait more than 4.1 ms */
	write_command(command, all);
	udelay(200);	/* Wait more than 100 us */

	set_io_functions(hd44780_flags & 2); /* Force busy waiting */
	if (test_bit(_4BITS_BUS, &hd44780_flags)) {
		write_command(0x32, all);
		command = BUS_4_BITS;
	} else {
		write_command(0x30, all);
		command = BUS_8_BITS;
	}
	command |= ((par.cntr_rows == 1) ? DISP_1_LINE : DISP_2_LINES);
	command |= (test_bit(_5X10_FONT, &hd44780_flags) ? FONT_5X10 : FONT_5X8);
	set_io_functions(hd44780_flags & 3); /* Normal operation mode */
	write_command(command, all);
}

static int hd44780_init_display(void)
{
	if (par.flags & HD44780_CHECK_BF)
		set_bit(_CHECK_BF, &hd44780_flags);
	else
		clear_bit(_CHECK_BF, &hd44780_flags);

	if (par.flags & HD44780_4BITS_BUS)
		set_bit(_4BITS_BUS, &hd44780_flags);
	else
		clear_bit(_4BITS_BUS, &hd44780_flags);

	if (par.flags & HD44780_5X10_FONT)
		set_bit(_5X10_FONT, &hd44780_flags);
	else
		clear_bit(_5X10_FONT, &hd44780_flags);

	write_init_command();
	hd44780_address_mode(1);
	hd44780_clear_display();
	disp_curr = 0;
	write_command(DISP_ON | CURS_OFF | BLINK_OFF, all);
	set_bit(DISPLAY_ON, &hd44780_flags);
	clear_bit(SHOW_CURSOR, &hd44780_flags);
	clear_bit(CURSOR_BLINK, &hd44780_flags);

	/* Set the CGRAM to default values */
	hd44780_write_cgram_char(0, cg0);
	hd44780_write_cgram_char(1, cg1);
	hd44780_write_cgram_char(2, cg2);
	hd44780_write_cgram_char(3, cg3);
	hd44780_write_cgram_char(4, cg4);
	hd44780_write_cgram_char(5, cg5);
	hd44780_write_cgram_char(6, cg6);
	hd44780_write_cgram_char(7, cg7);
	init_charmap();

#if defined(BF_STATS)
	__cnt1 = __cnt2 = 1;
#endif

	return (0);
}

static int hd44780_cleanup_display(void)
{
	hd44780_clear_display();

	return (0);
}

static int hd44780_init_port(void)
{
#ifdef USE_PARPORT
	struct parport *pp;

	if ((pp = parport_find_base(io)) == NULL) {
		printk(KERN_ERR "hd44780: no parport found at %#x\n", io);
		return (-ENXIO);
	}

	if ((pd = parport_register_device(pp, HD44780_STRING, NULL, NULL, NULL, 0, NULL)) == NULL) {
		printk(KERN_ERR "hd44780: parport busy\n");
		return (-EBUSY);
	}

	if (parport_claim(pd) < 0) {
		parport_unregister_device(pd);
		printk(KERN_ERR "hd44780: could not claim port\n");
		return (-EAGAIN);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	parport_data_forward(pd->port);
	clear_bit(PP_REVERSE, &hd44780_flags);
#endif
	parport_write_control(pd->port, 0x0b);
	parport_write_data(pd->port, 0x00);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	if (request_region(io, 3, HD44780_STRING) == NULL) {
		printk(KERN_ERR "hd44780: request_region failed, io = %#x\n", io);
		return (-EACCES);
	}

	outb(0x0b, io+2);
	outb(0x00, io);
#else
	if (check_region(io, 3)) {
		printk(KERN_ERR "hd44780: check_region failed, io = %#x\n", io);
		return (-EACCES);
	}

	request_region(io, 3, HD44780_STRING);

	outb(0x0b, io+2);
	outb(0x00, io);
#endif

	return (0);
}

static int hd44780_cleanup_port(void)
{
#ifdef USE_PARPORT
	parport_release(pd);
	parport_unregister_device(pd);
#else
	release_region(io, 3);
#endif

	return (0);
}

static void display_attr(unsigned char input)
{
	unsigned int i;

	switch (ESC_STATE) {
	case 'a':	/* Turn on/off the display cursor */
		if (input == '1')
			set_bit(SHOW_CURSOR, &hd44780_flags);
		else if (input == '0')
			clear_bit(SHOW_CURSOR, &hd44780_flags);
		break;
	case 'b':	/* Turn on/off the display cursor blinking */
		if (input == '1')
			set_bit(CURSOR_BLINK, &hd44780_flags);
		else if (input == '0')
			clear_bit(CURSOR_BLINK, &hd44780_flags);
		break;
	case 'h':	/* Turn on/off the display */
		if (input == '1')
			set_bit(DISPLAY_ON, &hd44780_flags);
		else if (input == '0')
			clear_bit(DISPLAY_ON, &hd44780_flags);
		break;
	}

	for (i = 0; i < par.num_cntr; ++i) {
		unsigned char command;

		command = (test_bit(DISPLAY_ON, &hd44780_flags) ? DISP_ON : DISP_OFF);
		if (i == disp_curr) {
			command |= (test_bit(SHOW_CURSOR, &hd44780_flags) ? CURS_ON : CURS_OFF);
			command |= (test_bit(CURSOR_BLINK, &hd44780_flags) ? BLINK_ON : BLINK_OFF);
		} else
			command |= (CURS_OFF | BLINK_OFF);

		if (ESC_STATE == 'h' || i == disp_curr)
			write_command(command, 1 << i);
	}
}

static int hd44780_handle_custom_char(unsigned int _input)
{
	unsigned char input = _input & 0xff;

	if (_input & (~0xff)) {
		switch (ESC_STATE) {
		case 'a':	/* Turn on/off the display cursor */
		case 'b':	/* Turn on/off the display cursor blinking */
		case 'h':	/* Turn on/off the the display */
			display_attr(input);
			return (0);
		case 'l':	/* Turn on/off the backlight */
			if (input == '1')
				set_bit(BACKLIGHT, &hd44780_flags);
			else if (input == '0')
				clear_bit(BACKLIGHT, &hd44780_flags);
			read_ac(0);
			return (0);
		}
	}

	switch (input) {
	case 'a':	/* Turn on/off the display cursor */
	case 'b':	/* Turn on/off the display cursor blinking */
	case 'h':	/* Turn on/off the display */
	case 'l':	/* Turn on/off the backlight */
		SET_ESC_STATE(input);
		return (1);
	case 'd':	/* Shift display cursor Right */
		write_command(SHIFT_CURS | SHIFT_RIGHT, all);
		return (0);
	case 'e':	/* Shift display cursor Left */
		write_command(SHIFT_CURS | SHIFT_LEFT, all);
		return (0);
	case 'f':	/* Shift display Right */
		write_command(SHIFT_DISP | SHIFT_RIGHT, all);
		return (0);
	case 'g':	/* Shift display Left */
		write_command(SHIFT_DISP | SHIFT_LEFT, all);
		return (0);
	}

	return (-1);
}

static int hd44780_handle_custom_ioctl(unsigned int num, unsigned long arg, unsigned int user_space)
{
	unsigned int i;
	unsigned char *buffer = (unsigned char *)arg;

	if (num != HD44780_READ_AC)
		return (-ENOIOCTLCMD);

	if (par.num_cntr > 2) {
		printk(KERN_ERR "hd44780: reading from the display is not allowed.\n");
		return (-ENOSYS);
	}

	if (user_space)
		for (i = 0; i < par.num_cntr; ++i)
			put_user(read_ac(1 << i), buffer+i);
	else
		for (i = 0; i < par.num_cntr; ++i)
			buffer[i] = read_ac(1 << i);

	return (0);
}

#ifdef USE_PROC
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
/* create_proc_read_entry is missing in 2.2.x kernels */
static struct proc_dir_entry *create_proc_read_entry(const char *name, mode_t mode,
			struct proc_dir_entry *parent, read_proc_t *read_proc, void *data)
{
	struct proc_dir_entry *res = create_proc_entry(name, mode, parent);

	if (res) {
		res->read_proc = read_proc;
		res->data = data;
	}

	return (res);
}
#endif

static int hd44780_proc_status(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;

#if defined(BF_STATS)
	unsigned long i, d;

	i = __cnt1/__cnt2;
	d = __cnt1-(i*__cnt2);
	d = (d*10000)/__cnt2;
	temp += sprintf(temp, "Busy flag polls per check (Avg.): ");
	if (test_bit(_CHECK_BF, &hd44780_flags))
		temp += sprintf(temp, "%ld.%04ld\n\n", i, d);
	else
		temp += sprintf(temp, "-\n\n");
	__cnt1 = __cnt2 = 1;
#endif

	/* Print display configuration */
	temp += sprintf(temp,
			"I/O base:\t%#x\n"
			"Interface:\t%u bits\n"
			"Display rows:\t%d\n"
			"Display cols:\t%d\n"
			"Screen rows:\t%d\n"
			"Screen cols:\t%d\n"
			"Read:\t\t%sabled\n"
			"Busy flag chk:\t%sabled\n"
			"Assigned minor:\t%u\n",
			io, (test_bit(_4BITS_BUS, &hd44780_flags) ? 4 : 8),
			par.cntr_rows*par.num_cntr, par.cntr_cols,
			par.vs_rows, par.vs_cols,
			((par.num_cntr > 2) ? "Dis" : "En"),
			(test_bit(_CHECK_BF, &hd44780_flags) ? "En" : "Dis"),
			par.minor);

	return (temp-buffer);
}

static int hd44780_proc_cgram(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	unsigned int i;

	temp += sprintf(temp,	"static void init_charmap(void)\n{\n"
				"\t/*\n"
				"\t * charmap[char mapped to cg0] = 0;\n"
				"\t * charmap[char mapped to cg1] = 1;\n"
				"\t * charmap[char mapped to cg2] = 2;\n"
				"\t * charmap[char mapped to cg3] = 3;\n"
				"\t * charmap[char mapped to cg4] = 4;\n"
				"\t * charmap[char mapped to cg5] = 5;\n"
				"\t * charmap[char mapped to cg6] = 6;\n"
				"\t * charmap[char mapped to cg7] = 7;\n"
				"\t */\n"
				"}\n\n");

	if (hd44780.read_cgram_char)
		for (i = 0; i < 8; ++i)
			hd44780_read_cgram_char(i, hd44780.cgram_buffer+(i << 3));

	for (i = 0; i < 8; ++i) {
		unsigned int j;

		temp += sprintf(temp, "static unsigned char cg%u[] = { ", i);
		for (j = 0; j < 8; ++j)
			temp += sprintf(temp, "0x%.2x%s", hd44780.cgram_buffer[(i << 3)+j], (j == 7 ? " };\n" : ", "));
	}

	return (temp-buffer);
}

static int hd44780_proc_cgram_bitmap(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	unsigned int i, j;

	if (hd44780.read_cgram_char)
		for (i = 0; i < 8; ++i)
			hd44780_read_cgram_char(i, hd44780.cgram_buffer+(i << 3));

	for (i = 0; i < 8; ++i)
		temp += sprintf(temp, "    %u   ", i);
	temp += sprintf(temp, "\n");

	for (i = 0; i < 8; ++i) {
		temp += sprintf(temp, " +");
		for (j = 0; j < 5; ++j)
			temp += sprintf(temp, "-");
		temp += sprintf(temp, "+");
	}
	temp += sprintf(temp, "\n");

	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 8; ++j) {
			int k = 5;
			char cgchar = hd44780.cgram_buffer[(j << 3)+i];

			temp += sprintf(temp, " |");
			while (k--) {
				temp += sprintf(temp, "%c", (cgchar & 0x10) ? '#' : ' ');
				cgchar <<= 1;
			}
			temp += sprintf(temp, "|");
		}
		temp += sprintf(temp, "\n");
	}

	for (i = 0; i < 8; ++i) {
		temp += sprintf(temp, " +");
		for (j = 0; j < 5; ++j)
			temp += sprintf(temp, "-");
		temp += sprintf(temp, "+");
	}
	temp += sprintf(temp, "\n");

	return (temp-buffer);
}

static void create_proc_entries(void)
{
	SET_PROC_LEVEL(0);
	if (create_proc_read_entry("status", 0, hd44780.driver_proc_root, hd44780_proc_status, NULL) == NULL) {
		printk(KERN_ERR "hd44780: cannot create /proc/lcd/%s/status\n", par.name);
		return;
	}
	SET_PROC_LEVEL(1);
	if (create_proc_read_entry("cgram.h", 0, hd44780.driver_proc_root, hd44780_proc_cgram, NULL) == NULL) {
		printk(KERN_ERR "hd44780: cannot create /proc/lcd/%s/cgram.h\n", par.name);
		return;
	}
	SET_PROC_LEVEL(2);
	if (create_proc_read_entry("cgram_bitmap", 0, hd44780.driver_proc_root, hd44780_proc_cgram_bitmap, NULL) == NULL) {
		printk(KERN_ERR "hd44780: cannot create /proc/lcd/%s/cgram_bitmap\n", par.name);
		return;
	}
	SET_PROC_LEVEL(3);
}

static void remove_proc_entries(void)
{
	switch (PROC_LEVEL) {
	case 3:
		remove_proc_entry("cgram_bitmap", hd44780.driver_proc_root);
	case 2:
		remove_proc_entry("cgram.h", hd44780.driver_proc_root);
	case 1:
		remove_proc_entry("status", hd44780.driver_proc_root);
	}
	SET_PROC_LEVEL(0);
}
#endif

/* Initialization */
static int __init hd44780_init_module(void)
{
	int ret;

#ifdef MODULE
	if ((ret = request_module("lcd-linux"))) {
		printk(KERN_ERR "hd44780: request_module() returned %d\n", ret);
		if (ret < 0) {
			if (ret != -ENOSYS) {
				printk(KERN_ERR "hd44780: failure while loading module lcd-linux\n");
				return (ret);
			}
			printk(KERN_ERR "hd44780: your kernel does not have kmod or kerneld support;\n");
			printk(KERN_ERR "hd44780: remember to load the lcd-linux module before\n");
			printk(KERN_ERR "hd44780: loading the hd44780 module\n");
		}
	}

	if (flags	!= DFLT_FLAGS)		par.flags	= flags;
	if (tabstop	!= DFLT_TABSTOP)	par.tabstop	= tabstop;
	if (num_cntr	!= DFLT_NUM_CNTR)	par.num_cntr	= num_cntr;
	if (cntr_rows	!= DFLT_CNTR_ROWS)	par.cntr_rows	= cntr_rows;
	if (cntr_cols	!= DFLT_CNTR_COLS)	par.cntr_cols	= cntr_cols;
	if (vs_rows	!= DFLT_VS_ROWS)	par.vs_rows	= vs_rows;
	if (vs_cols	!= DFLT_VS_COLS)	par.vs_cols	= vs_cols;
	if (minor	!= HD44780_MINOR)	par.minor	= minor;
#endif

	lcd_driver_setup(&hd44780);
	if ((ret = lcd_register_driver(&hd44780, &par)))
		return (ret);

#ifdef USE_PROC
	if (hd44780.driver_proc_root)
		create_proc_entries();
#endif

	printk(KERN_INFO "hd44780: --> HD44780 driver (LCD-Linux " HD44780_VERSION ") <--\n");
	printk(KERN_INFO "hd44780: --> Mattia Jona-Lasinio <mjona@users.sourceforge.net> <--\n" );

	return (0);
}

/* Cleanup */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void __exit hd44780_cleanup_module(void)
#else
/* __exit is not defined in 2.2.x kernels */
static void hd44780_cleanup_module(void)
#endif
{
#ifdef USE_PROC
	if (hd44780.driver_proc_root)
		remove_proc_entries();
#endif

	lcd_unregister_driver(&hd44780, &par);
}

module_init(hd44780_init_module)
module_exit(hd44780_cleanup_module)
