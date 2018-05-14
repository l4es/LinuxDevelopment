/* lcd-linux.c
 *
 * $Id: lcd-linux.c,v 1.308 2010/03/03 14:56:22 mjona Exp $
 *
 * Software layer to drive LCD displays under Linux.
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

#ifdef CONFIG_PROC_FS
#define USE_PROC
#else
#undef USE_PROC
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#include <linux/bitops.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <linux/fs.h>

#include <asm/uaccess.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#include <linux/device.h>
#endif
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/selection.h>
#include <linux/vmalloc.h>

#ifdef USE_PROC
#include <linux/proc_fs.h>
#endif

#define LCD_LINUX_MAIN
#include <linux/lcd-linux.h>

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

/*** struct_flags ***/

/* internal flags: bits [0:15] of struct_flags (16 flags allowed) */
#define NEED_WRAP	0		/* Next char will trigger a newline */
#define DECIM		1		/* Insert mode */
#define DECOM		2		/* Row origin of cursor: absolute or relative to scrolling region */
#define DECAWM		3		/* Autowrap */
#define DECSCNM		4		/* Inverted screen */
#define CRLF		5		/* Follow lf, vt, ff, with a cr */
#define INC_CURS_POS	6		/* Increment cursor position after data read/write */
#define QUES		7		/* CSI Esc sequence contains a question mark */
#define USER_SPACE	8		/* If set, the buffer pointed by arg in do_lcd_ioctl() is
					 * assumed to be in user space otherwise it is in kernel space */
#define NULL_CHARMAP	9		/* The driver doesn't provide a charmap so the
					 * lcd-linux layer provides one*/
#define CAN_DO_COLOR	10		/* The display is color capable */
#define WITH_ATTR	11		/* If set, the void * buffer in do_lcd_read/write() contains
					 * attributes and therefore is an unsigned short * otherwise it
					 * is an unsigned char *
					 */

/* input states: bits [24:27] of struct_flags (16 states allowed) */
#define NORMAL		0	/* Normal mode */
#define RAW		1	/* Raw mode (console emulation disabled) */
#define SYN		2	/* Synchronous Idle mode */
#define ESC		3	/* Escape mode */
#define CSI		4	/* CSI escape mode */
#define ESC_G0		5	/* G0 character set */
#define ESC_G1		6	/* G1 character set */
#define ESC_HASH	7	/* ESC # escape sequence */
#define ESC_PERCENT	8	/* ESC % escape sequence */
#define ARG		9	/* Waiting for arguments for the lcd-linux layer */
#define ARG_DRIVER 	10	/* Waiting for arguments for the display driver */

#define SET_ESC_STATE(p, x)	__set_br(&(p)->struct_flags, x, 16, 8)
#define SET_INPUT_STATE(p, x)	__set_br(&(p)->struct_flags, x, 24, 4)
#define SET_INIT_LEVEL(p, x)	__set_br(&(p)->struct_flags, x, 28, 2)
#define SET_PROC_LEVEL(p, x)	__set_br(&(p)->struct_flags, x, 30, 2)
#define ESC_STATE(p)		__get_br((p)->struct_flags, 16, 8)
#define INPUT_STATE(p)		__get_br((p)->struct_flags, 24, 4)
#define INIT_LEVEL(p)		__get_br((p)->struct_flags, 28, 2)
#define PROC_LEVEL(p)		__get_br((p)->struct_flags, 30, 2)

/*** attributes ***/
#define I_MASK		0x03		/* Intensity (0 = low, 1 = normal, 2 = bright) */
#define ULINE		0x04		/* Underlined text */
#define	REVERSE		0x08		/* Reversed video text */
#define BLINK		0x80		/* Blinking text */

/*** Color attributes ***/
#define FG_COLOR	0x07				/* Foreground color mask */
#define FG_BRIGHT	0x08				/* Foreground bright color */
#define FG_MASK		(FG_BRIGHT | FG_COLOR)		/* Foreground mask */
#define BG_COLOR	0x70				/* Background color mask */
#define BG_BRIGHT	0x80				/* Background bright color */
#define BG_MASK		(BG_BRIGHT | BG_COLOR)		/* Background mask */

#define NPAR	16			/* Max number of parameters in CSI escape sequence */
#define FLIP_BUF_SIZE	(1 << 6)	/* Flip buffer size (64 bytes) MUST be a power of 2 */

struct lcd_struct {
	struct list_head	lcd_list;		/* Doubly linked list */
	struct semaphore	lcd_sem;		/* Locks this structure */
	struct lcd_driver	*driver;		/* The driver associated to this struct */
	struct lcd_parameters	*par;			/* The parameters associated to this struct */
	unsigned long		struct_flags;		/* Flags for internal use only */
	unsigned int		refcount;		/* Number of references to this struct */

	unsigned short		*display;		/* The display buffer */

	unsigned short		*fb;			/* The virtual screen framebuffer */
	unsigned int		fb_size;		/* Size of the framebuffer */
	unsigned int		frame_base;		/* Offset of row 0, column 0 of a frame in fb */
	unsigned int		frame_size;		/* Size of the frame */

	unsigned int		row;			/* Current row in virtual screen */
	unsigned int		col;			/* Current column in virtual screen */
	unsigned int		s_offset;		/* Saved cursor position in virtual screen */

	unsigned int		top;			/* Top scroll row in virtual screen */
	unsigned int		bot;			/* Bottom scroll row in virtual screen */

	int			esc_args;		/* Number of arguments for a normal escape sequence */
	unsigned int		csi_args[NPAR];		/* CSI parameters */
	unsigned int		index;			/* Index in csi_args and counter for cgram characters generation */
	unsigned char		cgram_index;		/* Index of the cgram character to be created */
	unsigned char		*cgram_buffer;		/* Buffer for cgram operations in this driver */

	unsigned short		erase_char;		/* Character to be used when erasing */
	unsigned char		attr;			/* Current attributes */
	unsigned char		color;			/* Color for normal intensity mode */
	unsigned char		s_color;		/* Saved color for normal intensity mode */
	unsigned char		defcolor;		/* Default color for normal intensity mode */
	unsigned char		ulcolor;		/* Color for underline mode */
	unsigned char		halfcolor;		/* Color for low intensity mode */
	unsigned char		attributes;		/* Packed attributes */
	unsigned char		s_attributes;		/* Saved packed attributes */

	unsigned char		*s_charmap;		/* Saved character map for this driver */
	unsigned char		*flip_buf;		/* High speed flip buffer */
};

/** Function prototypes **/

/* Init/Cleanup the driver */
static int init_driver(struct lcd_struct *);
static int cleanup_driver(struct lcd_struct *);

/* Read from/Write to the driver */
static void read_data(struct lcd_struct *, unsigned short *);
static void read_cgram(struct lcd_struct *, unsigned char, unsigned char *);
static void write_data(struct lcd_struct *, unsigned short);
static void write_cgram(struct lcd_struct *, unsigned char, unsigned char *);

/* Input handlers */
static void cr(struct lcd_struct *);
static void lf(struct lcd_struct *);
static void control_char(struct lcd_struct *, unsigned char);
static void handle_csi(struct lcd_struct *, unsigned char);
static int handle_custom_esc(struct lcd_struct *, unsigned int);
static int handle_esc(struct lcd_struct *, unsigned char);
static void handle_input(struct lcd_struct *, unsigned short);

/* Low level file operations */
static ssize_t do_lcd_read(struct lcd_struct *, void *, size_t);
static ssize_t do_lcd_write(struct lcd_struct *, const void *, size_t);
static int do_lcd_open(struct lcd_struct *);
static int do_lcd_release(struct lcd_struct *);
static int do_lcd_ioctl(struct lcd_struct *, unsigned int, unsigned long);

/* Proc functions */
#ifdef USE_PROC
static void create_driver_proc_entries(struct lcd_struct *);
static void remove_driver_proc_entries(struct lcd_struct *);
#endif

/* globals */
static unsigned int major	= LCD_MAJOR;		/* Major number for LCD-Linux device */
static unsigned short minors	= LCD_MINORS;		/* Minor numbers allocated for LCD-Linux */
static LIST_HEAD(lcd_drivers);				/* Registered lcd drivers */
static struct semaphore drivers_sem;			/* Locks the lcd_drivers list */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
static struct class *lcd_linux_class;
#endif
#ifdef USE_PROC
static struct proc_dir_entry *lcd_proc_root;
#endif
/* End of globals */

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#include <linux/device.h>
MODULE_ALIAS_CHARDEV_MAJOR(LCD_MAJOR);
#endif
MODULE_DESCRIPTION("Software layer to drive LCD displays under Linux.");
MODULE_AUTHOR("Mattia Jona-Lasinio <mjona@users.sourceforge.net>");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
module_param(minors, ushort, 0444);
#else
MODULE_PARM(minors, "h");
#endif
MODULE_PARM_DESC(minors, "Minor numbers allocated for LCD-Linux (default: " string(LCD_MINORS) ")");
#else

/*
 * Parse boot command line
 *
 * lcd=minors
 */
static int __init lcd_linux_boot_init(char *cmdline)
{
	unsigned short args;

	if ((args = simple_strtoul(cmdline, NULL, 0)))
		minors = args;

	return (1);
}

__setup("lcd=", lcd_linux_boot_init);
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

/* Uncomment the following two lines
 * for non-atomic set_bit and clear_bit
 */
//#define set_bit	__set_bit
//#define clear_bit	__clear_bit

/************************************
 * Low level routines and utilities *
 ************************************/
/*
 * Set whether the address counter should be incremented
 * or decremented after a Read/Write
 */
static void address_mode(struct lcd_struct *p, int mode)
{
	struct lcd_driver *driver = p->driver;

	if (mode > 0 && ! test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (driver->address_mode)
			driver->address_mode(mode);
		set_bit(INC_CURS_POS, &p->struct_flags);
	} else if (mode < 0 && test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (driver->address_mode)
			driver->address_mode(mode);
		clear_bit(INC_CURS_POS, &p->struct_flags);
	}
}

/* WARNING!! This function returns an int because if iterator is not
 * within the visible area of the frame it returns -1
 */
static inline int vs_to_frame_(struct lcd_struct *p, unsigned int iterator)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int row = iterator/vs_cols;
	unsigned int col = iterator%vs_cols;
	unsigned int frame_base_row = p->frame_base/vs_cols;
	unsigned int frame_base_col = p->frame_base%vs_cols;
	unsigned int frame_rows = p->par->cntr_rows*p->par->num_cntr;
	unsigned int frame_cols = p->par->cntr_cols;

	if (vs_rows == frame_rows && vs_cols == frame_cols)
		return (iterator);

	if (row < frame_base_row || row >= frame_base_row+frame_rows)
		return (-1);
	if (col < frame_base_col || col >= frame_base_col+frame_cols)
		return (-1);

	return ((row-frame_base_row)*frame_cols+(col-frame_base_col));
}

/* Given 'iterator' in vs, returns the offset in vs corresponding to the nearest
 * visible offset in vs, or returns 'iterator' if it is already visible.
 */
static unsigned int round_vs_(struct lcd_struct *p, unsigned int iterator)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int row = iterator/vs_cols;
	unsigned int col = iterator%vs_cols;
	unsigned int frame_base_row = p->frame_base/vs_cols;
	unsigned int frame_base_col = p->frame_base%vs_cols;
	unsigned int frame_rows = p->par->cntr_rows*p->par->num_cntr;
	unsigned int frame_cols = p->par->cntr_cols;

	if (vs_rows == frame_rows && vs_cols == frame_cols)
		return (iterator);

	if (row < frame_base_row)
		row = frame_base_row;
	else if (row >= frame_base_row+frame_rows)
		row = frame_base_row+(frame_rows-1);

	if (col < frame_base_col)
		col = frame_base_col;
	else if (col >= frame_base_col+frame_cols)
		col = frame_base_col+(frame_cols-1);

	return ((row*vs_cols)+col);
}

#define round_vs(p, iterator)			(iterator = round_vs_(p, iterator))

/*
 * Sync the frame area starting at offset s, ending at offset e with fb content.
 */
static void redraw_screen(struct lcd_struct *p, unsigned int s, unsigned int e)
{
	unsigned int len;
	unsigned int row = p->row, col = p->col;
	unsigned int inc_set = test_bit(INC_CURS_POS, &p->struct_flags);
	unsigned int frame_cols = p->par->cntr_cols;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned long flags;

	if (s >= p->fb_size || e >= p->fb_size || e < s || e < p->frame_base)
		return;

	round_vs(p, s);
	round_vs(p, e);

	len = 1+e-s;

	if (! inc_set)
		s = e;

	p->row = s/vs_cols;
	p->col = s%vs_cols;

	flags = p->struct_flags;
	clear_bit(NEED_WRAP, &p->struct_flags);
	clear_bit(DECIM, &p->struct_flags);
	set_bit(DECAWM, &p->struct_flags);
	SET_INPUT_STATE(p, RAW);
	if (inc_set)
		while (len--)
			if (vs_to_frame_(p, (p->row*vs_cols)+p->col) < 0) {
				s += vs_cols-frame_cols;
				len -= vs_cols-frame_cols-1;
				p->row = s/vs_cols;
				p->col = s%vs_cols;
			} else {
				write_data(p, p->fb[s++]);
				if (test_bit(NEED_WRAP, &p->struct_flags)) {
					cr(p);
					lf(p);
				}
			}
	else
		while (len--)
			if (vs_to_frame_(p, (p->row*vs_cols)+p->col) < 0) {
				s -= vs_cols-frame_cols;
				len -= vs_cols-frame_cols-1;
				p->row = s/vs_cols;
				p->col = s%vs_cols;
			} else {
				write_data(p, p->fb[s--]);
				if (test_bit(NEED_WRAP, &p->struct_flags)) {
					cr(p);
					lf(p);
				}
			}
	p->struct_flags = flags;

	p->row = row; p->col = col;
}

static int show_cursor(struct lcd_struct *p)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int frame_base, frame_base_row, frame_base_col;
	unsigned int frame_rows = p->par->cntr_rows*p->par->num_cntr;
	unsigned int frame_cols = p->par->cntr_cols;
	unsigned int tmp = frame_cols/2;

	if (vs_rows == frame_rows && vs_cols == frame_cols)
		return (0);

	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
	/* cursor always on the lowest row of the display */
		frame_base_row = 0;
		frame_base_col = 0;
		if (p->row >= frame_rows)
			frame_base_row = p->row-(frame_rows-1);
		if (p->col >= frame_cols) {
			frame_base_col = p->col-(frame_cols-1);
			if (tmp) {
				tmp = (tmp-(frame_base_col%tmp))%tmp;
				if (frame_base_col+tmp <= vs_cols-frame_cols)
					frame_base_col += tmp;
			}
		}
	} else {
	/* cursor always on the uppermost row of the display */
		frame_base_row = vs_rows-frame_rows;
		frame_base_col = vs_cols-frame_cols;
		if (p->row < vs_rows-frame_rows)
			frame_base_row = p->row;
		if (p->col < vs_cols-frame_cols) {
			frame_base_col = p->col;
			if (tmp) {
				tmp = frame_base_col%tmp;
				if (frame_base_col >= tmp)
					frame_base_col -= tmp;
			}
		}
	}

	frame_base = p->frame_base;
	p->frame_base = (frame_base_row*vs_cols)+frame_base_col;

	return (frame_base != p->frame_base);
}

/*
 * Move the visible screen area at user's wish
 */
static void browse_screen(struct lcd_struct *p, unsigned char dir)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int frame_base_row = p->frame_base/vs_cols;
	unsigned int frame_base_col = p->frame_base%vs_cols;
	unsigned int frame_rows = p->par->cntr_rows*p->par->num_cntr;
	unsigned int frame_cols = p->par->cntr_cols;

	switch (dir) {
	case '1':	/* Up */
		if (! frame_base_row)
			return;
		--frame_base_row;
		break;
	case '2':	/* Down */
		if (frame_base_row >= vs_rows-frame_rows)
			return;
		++frame_base_row;
		break;
	case '3':	/* Left */
		if (! frame_base_col)
			return;
		--frame_base_col;
		break;
	case '4':	/* Right */
		if (frame_base_col >= vs_cols-frame_cols)
			return;
		++frame_base_col;
		break;
	default:
		return;
	}

	p->frame_base = (frame_base_row*vs_cols)+frame_base_col;
	redraw_screen(p, 0, p->fb_size-1);
}

static inline void __memset_short(unsigned short *buf, unsigned short c, unsigned int len)
{
	while (len--)
		*buf++ = c;
}

/*
 * A memset implementation writing to LCD instead of memory locations.
 */
static void lcd_memset(struct lcd_struct *p, unsigned int d, unsigned short c, unsigned int len)
{
	unsigned int inc_set = test_bit(INC_CURS_POS, &p->struct_flags);

	if (! len || d >= p->fb_size)
		return;

	if (inc_set && d+len > p->fb_size)
		len = p->fb_size-d;
	else if (! inc_set && len > d+1)
		len = d+1;

	if (! inc_set)
		d -= len-1;
	__memset_short(p->fb+d, c, len);

	if (show_cursor(p))
		redraw_screen(p, 0, p->fb_size-1);
	else
		redraw_screen(p, d, d+(len-1));
}

static inline void __memcpy_short(unsigned short *d, unsigned short *s, unsigned int len, int dir)
{
	if (dir > 0)
		while (len--)
			*d++ = *s++;
	else
		while (len--)
			*d-- = *s--;
}

/*
 * A memmove implementation writing to LCD instead of memory locations.
 * Copy is done in a non destructive way. Display regions may overlap.
 */
static void lcd_memmove(struct lcd_struct *p, unsigned int d, unsigned int s, unsigned int len)
{
	if (! len || d == s || d >= p->fb_size || s >= p->fb_size)
		return;

	if (d < s) {
		if (test_bit(INC_CURS_POS, &p->struct_flags)) {
			if (s+len > p->fb_size)
				len = p->fb_size-s;
		} else {
			if (len > d+1)
				len = d+1;
			d -= len-1;
			s -= len-1;
		}
		__memcpy_short(p->fb+d, p->fb+s, len, 1);
		if (show_cursor(p))
			redraw_screen(p, 0, p->fb_size-1);
		else
			redraw_screen(p, d, d+(len-1));
	} else {
		if (test_bit(INC_CURS_POS, &p->struct_flags)) {
			if (d+len > p->fb_size)
				len = p->fb_size-d;
			d += len-1;
			s += len-1;
		} else {
			if (len > s+1)
				len = s+1;
		}
		__memcpy_short(p->fb+d, p->fb+s, len, -1);
		if (show_cursor(p))
			redraw_screen(p, 0, p->fb_size-1);
		else
			redraw_screen(p, d-(len-1), d);
	}
}

static void scrup(struct lcd_struct *p, unsigned int t, unsigned int b, unsigned int nr)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int d, s;

	if (t+nr >= b)
		nr = b-t-1;
	if (b > vs_rows || t >= b || nr < 1)
		return;
	d = t*vs_cols;
	s = (t+nr)*vs_cols;
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		lcd_memmove(p, d, s, (b-t-nr)*vs_cols);
		lcd_memset(p, d+(b-t-nr)*vs_cols, p->erase_char, nr*vs_cols);
	} else {
		lcd_memmove(p, d+(b-t-nr)*vs_cols-1, s+(b-t-nr)*vs_cols-1, (b-t-nr)*vs_cols);
		lcd_memset(p, d+(b-t)*vs_cols-1, p->erase_char, nr*vs_cols);
	}
}

static void scrdown(struct lcd_struct *p, unsigned int t, unsigned int b, unsigned int nr)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int d, s;

	if (t+nr >= b)
		nr = b-t-1;
	if (b > vs_rows || t >= b || nr < 1)
		return;
	s = t*vs_cols;
	d = (t+nr)*vs_cols;
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		lcd_memmove(p, d, s, (b-t-nr)*vs_cols);
		lcd_memset(p, s, p->erase_char, nr*vs_cols);
	} else {
		lcd_memmove(p, d+(b-t-nr)*vs_cols-1, s+(b-t-nr)*vs_cols-1, (b-t-nr)*vs_cols);
		lcd_memset(p, s+nr*vs_cols-1, p->erase_char, nr*vs_cols);
	}
}

static void lcd_insert_char(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int pos = (p->row*vs_cols)+p->col;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(INC_CURS_POS, &p->struct_flags))
		lcd_memmove(p, pos+nr, pos, vs_cols-p->col-nr);
	else
		lcd_memmove(p, pos-nr, pos, p->col-(nr-1));
	lcd_memset(p, pos, p->erase_char, nr);
}

static void lcd_delete_char(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int pos = (p->row*vs_cols)+p->col;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		lcd_memmove(p, pos, pos+nr, vs_cols-(p->col+nr));
		lcd_memset(p, (p->row+1)*vs_cols-nr, p->erase_char, nr);
	} else {
		lcd_memmove(p, pos, pos-nr, p->col-(nr-1));
		lcd_memset(p, (p->row*vs_cols)+(nr-1), p->erase_char, nr);
	}
}





/******************************************************************************
 *************************      VT 102 Emulation      *************************
 ******************************************************************************/

/**********************
 * Control characters *
 **********************/
static void bs(struct lcd_struct *p)
{
	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (p->col)
			--p->col;
	} else {
		if (p->col+1 < p->par->vs_cols)
			++p->col;
	}
}

static void cr(struct lcd_struct *p)
{
	clear_bit(NEED_WRAP, &p->struct_flags);
	p->col = (test_bit(INC_CURS_POS, &p->struct_flags) ? 0 : p->par->vs_cols-1);
}

static void lf(struct lcd_struct *p)
{
	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (p->row+1 < p->bot)
			++p->row;
		else if (INPUT_STATE(p) != RAW) {
			show_cursor(p);
			scrup(p, p->top, p->bot, 1);
		}
	} else {
		if (p->row > p->top)
			--p->row;
		else if (INPUT_STATE(p) != RAW) {
			show_cursor(p);
			scrdown(p, p->top, p->bot, 1);
		}
	}
}

static void ri(struct lcd_struct *p)
{
	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (p->row > p->top)
			--p->row;
		else {
			show_cursor(p);
			scrdown(p, p->top, p->bot, 1);
		}
	} else {
		if (p->row+1 < p->bot)
			++p->row;
		else {
			show_cursor(p);
			scrup(p, p->top, p->bot, 1);
		}
	}
}

static void ff(struct lcd_struct *p)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (p->driver->clear_display) {
		p->driver->clear_display();
		__memset_short(p->fb, p->erase_char, p->fb_size);
		__memset_short(p->display, p->erase_char, p->frame_size);
		p->frame_base = 0;
	} else if (test_bit(INC_CURS_POS, &p->struct_flags))
		lcd_memset(p, 0, p->erase_char, p->fb_size);
	else
		lcd_memset(p, p->fb_size-1, p->erase_char, p->fb_size);

	if (test_bit(INC_CURS_POS, &p->struct_flags))
		p->row = p->col = 0;
	else {
		p->row = vs_rows-1;
		p->col = vs_cols-1;
	}
}

static void tab(struct lcd_struct *p)
{
	struct lcd_parameters *par = p->par;
	unsigned int i, vs_cols = par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);

	if (! par->tabstop)
		return;

	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		i = par->tabstop-(p->col%par->tabstop);
		if (p->col+i < vs_cols)
			p->col += i;
	} else {
		i = p->col%par->tabstop;
		i = (i == 0 ? par->tabstop : i);
		if (p->col >= i)
			p->col -= i;
	}
}

/*
 * Control character handler.
 */
static void control_char(struct lcd_struct *p, unsigned char val)
{
	switch (val) {
	case 0x08: 	/* BS: Back Space (^H) */
	case 0x7f:	/* DEL: Delete */
		bs(p);
		return;

	case 0x09: 	/* HT: Horizontal Tab (^I) */
		tab(p);
		return;

	case 0x0c: 	/* FF: Form Feed (^L) */
		ff(p);
		return;

	case 0x0a: 	/* LF: Line Feed (^J) */
	case 0x0b: 	/* VT: Vertical Tab (^K) */
		lf(p);
		if (! test_bit(CRLF, &p->struct_flags))
			return;

	case 0x0d: 	/* CR: Carriage Return (^M) */
		cr(p);
		return;

	case 0x16: 	/* SYN: Synchronous Idle (^V) */
		SET_INPUT_STATE(p, SYN);
		return;

	case 0x1b: 	/* ESC: Start of escape sequence */
		SET_INPUT_STATE(p, ESC);
		return;

	case 0x9b: 	/* CSI: Start of CSI escape sequence */
		memset(p->csi_args, 0, sizeof(p->csi_args));
		p->index = 0;
		SET_INPUT_STATE(p, CSI);
		return;
	}
}

static void gotoxy(struct lcd_struct *p, int new_col, int new_row)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	int min_row, max_row;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (test_bit(DECOM, &p->struct_flags)) {
		min_row = p->top;
		max_row = p->bot;
	} else {
		min_row = 0;
		max_row = vs_rows;
	}

	if (new_row < min_row)
		p->row = min_row;
	else if (new_row >= max_row)
		p->row = max_row-1;
	else
		p->row = new_row;

	if (new_col < 0)
		p->col = 0;
	else if (new_col >= vs_cols)
		p->col = vs_cols-1;
	else
		p->col = new_col;

	if (show_cursor(p))
		redraw_screen(p, 0, p->fb_size-1);
}

static void gotoxay(struct lcd_struct *p, int new_col, int new_row)
{
	gotoxy(p, new_col, test_bit(DECOM, &p->struct_flags) ? (p->top+new_row) : new_row);
}


/******************************
 * ECMA-48 CSI ESC- sequences *
 ******************************/
static void csi_at(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_cols = p->par->vs_cols;

	if (p->col+nr > vs_cols)
		nr = vs_cols-p->col;
	else if (! nr)
		++nr;
	lcd_insert_char(p, nr);
}

static void csi_J(struct lcd_struct *p, unsigned int action)
{
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int pos = (p->row*vs_cols)+p->col;

	clear_bit(NEED_WRAP, &p->struct_flags);
	switch (action) {
	case 0:		/* From cursor to end of display */
		lcd_memset(p, pos, p->erase_char, p->fb_size-pos);
		return;

	case 1:		/* From start of display to cursor */
		lcd_memset(p, 0, p->erase_char, pos+1);
		return;

	case 2:		/* Whole display */
		lcd_memset(p, 0, p->erase_char, p->fb_size);
		return;
	}
}

static void csi_K(struct lcd_struct *p, unsigned int action)
{
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int row_start = p->row*vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	switch (action) {
	case 0:		/* From cursor to end of line */
		lcd_memset(p, row_start+p->col, p->erase_char, vs_cols-p->col);
		return;

	case 1:		/* From start of line to cursor */
		lcd_memset(p, row_start, p->erase_char, p->col+1);
		return;

	case 2:		/* Whole line */
		lcd_memset(p, row_start, p->erase_char, vs_cols);
		return;
	}
}

static void csi_L(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (p->row+nr > vs_rows)
		nr = vs_rows-p->row;
	else if (! nr)
		++nr;;
	lcd_memmove(p, (p->row+nr)*vs_cols, p->row*vs_cols, (vs_rows-p->row-nr)*vs_cols);
	lcd_memset(p, p->row*vs_cols, p->erase_char, nr*vs_cols);
}

static void csi_M(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (p->row+nr > vs_rows)
		nr = vs_rows-p->row;
	else if (! nr)
		++nr;;
	lcd_memmove(p, p->row*vs_cols, (p->row+nr)*vs_cols, (vs_rows-p->row-nr)*vs_cols);
	lcd_memset(p, (vs_rows-nr)*vs_cols, p->erase_char, nr*vs_cols);
}

static void csi_P(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_cols = p->par->vs_cols;

	if (p->col+nr > vs_cols)
		nr = vs_cols-p->col;
	else if (! nr)
		++nr;
	lcd_delete_char(p, nr);
}

static void csi_X(struct lcd_struct *p, unsigned int nr)
{
	unsigned int vs_cols = p->par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (p->col+nr > vs_cols)
		nr = vs_cols-p->col;
	else if (! nr)
		++nr;
	lcd_memset(p, (p->row*vs_cols)+p->col, p->erase_char, nr);
}

static void csi_su(struct lcd_struct *p, unsigned char input)
{
	unsigned int vs_cols = p->par->vs_cols;

	clear_bit(NEED_WRAP, &p->struct_flags);
	if (input == 'u') {
		p->row = p->s_offset/vs_cols;
		p->col = p->s_offset%vs_cols;
		p->color = p->s_color;
		p->attributes = p->s_attributes;
		return;
	}
	p->s_offset = (p->row*vs_cols)+p->col;
	p->s_color = p->color;
	p->s_attributes = p->attributes;
}

static inline unsigned char reverse_color_attr(unsigned char attr)
{
	return ((attr & (BG_BRIGHT | FG_BRIGHT)) | ((attr & BG_COLOR) >> 4) | ((attr & FG_COLOR) << 4));
}

static unsigned char build_attr(struct lcd_struct *p, unsigned char color, unsigned char intensity,
				unsigned char blink, unsigned char underline, unsigned char reverse)
{
	unsigned char attr;

	if (test_bit(CAN_DO_COLOR, &p->struct_flags)) {
		attr = color;
		if (underline)
			attr = (attr & BG_MASK) | p->ulcolor;
		else if (intensity == 0)
			attr = (attr & BG_MASK) | p->halfcolor;
		if (reverse)
			attr = reverse_color_attr(attr);
		if (blink)
			attr ^= BG_BRIGHT;
		if (intensity == 2)
			attr ^= FG_BRIGHT;
	} else {
		attr = intensity;
		attr |= (underline ? ULINE : 0x00);
		attr |= (reverse ? REVERSE : 0x00);
		attr |= (blink ? BLINK : 0x00);
	}

	return (attr);
}

static void update_attr(struct lcd_struct *p)
{
	unsigned char intensity = p->attributes & I_MASK;
	unsigned char underline = (p->attributes & ULINE) ? 0x01 : 0x00;
	unsigned char reverse = (p->attributes & REVERSE) ? 0x01 : 0x00;
	unsigned char blink = (p->attributes & BLINK) ? 0x01 : 0x00;
	unsigned char decscnm = test_bit(DECSCNM, &p->struct_flags) ? 0x01 : 0x00;

	p->attr = build_attr(p, p->color, intensity, blink, underline, reverse ^ decscnm);
	p->erase_char = (build_attr(p, p->color, 1, blink, 0, decscnm) << 8) | ' ';
}

static void default_attr(struct lcd_struct *p)
{
	p->attributes = 0x01;
	p->color = p->defcolor;
}

static void lcd_invert_screen(struct lcd_struct *p, unsigned int s, unsigned int len)
{
	unsigned int l, inc_set = test_bit(INC_CURS_POS, &p->struct_flags);

	if (! len || s >= p->fb_size)
		return;
	if (inc_set && s+len > p->fb_size)
		len = p->fb_size-s;
	else if (! inc_set && len > s+1)
		len = s+1;

	l = len;
	if (test_bit(CAN_DO_COLOR, &p->struct_flags))
		while (l--) {
			p->fb[s] = (reverse_color_attr(p->fb[s] >> 8) << 8) | (p->fb[s] & 0xff);
			++s;
		}
	else
		while (l--) {
			p->fb[s] ^= REVERSE << 8;
			++s;
		}

	if (show_cursor(p))
		redraw_screen(p, 0, p->fb_size-1);
	else
		redraw_screen(p, s, s+(len-1));
}

static void csi_m(struct lcd_struct *p, unsigned int n)
{
	int i, arg;

	for (i = 0; i <= n; ++i)
		switch ((arg = p->csi_args[i]))
		{
			case 0:
				default_attr(p);
				break;

			case 1:
				p->attributes = (p->attributes & ~I_MASK) | 2;
				break;

			case 2:
				p->attributes = (p->attributes & ~I_MASK) | 0;
				break;

			case 4:
				p->attributes |= ULINE;
				break;

			case 5:
				p->attributes |= BLINK;
				break;

			case 7:
				p->attributes |= REVERSE;
				break;

			case 21: case 22:
				p->attributes = (p->attributes & ~I_MASK) | 1;
				break;

			case 24:
				p->attributes &= ~ULINE;
				break;

			case 25:
				p->attributes &= ~BLINK;
				break;

			case 27:
				p->attributes &= ~REVERSE;
				break;

			case 38:
				p->attributes |= ULINE;
				p->color = (p->color & BG_MASK) | (p->defcolor & FG_MASK);
				break;

			case 39:
				p->attributes &= ~ULINE;
				p->color = (p->color & BG_MASK) | (p->defcolor & FG_MASK);
				break;

			case 49:
				p->color = (p->defcolor & BG_MASK) | (p->color & FG_MASK);
				break;

			default:
				if (arg >= 30 && arg <= 37)
					p->color = (p->color & BG_MASK) | color_table[arg-30];
				else if (arg >= 40 && arg <= 47)
					p->color = (p->color & FG_MASK) | (color_table[arg-40] << 4);
				break;
		}

	update_attr(p);
}

static void csi_h(struct lcd_struct *p, unsigned char n)
{
	switch (n) {
		case 4:		/* Set insert mode */
			set_bit(DECIM, &p->struct_flags);
			return;

		case 5:		/* Inverted screen mode */
			if (test_bit(QUES, &p->struct_flags) && ! test_bit(DECSCNM, &p->struct_flags)) {
				lcd_invert_screen(p, 0, p->fb_size);
				set_bit(DECSCNM, &p->struct_flags);
				update_attr(p);
			}
			return;

		case 6:		/* Cursor addressing origin: relative to scrolling region */
			if (test_bit(QUES, &p->struct_flags)) {
				set_bit(DECOM, &p->struct_flags);
				gotoxay(p, 0, 0);
			}
			return;

		case 7:		/* Set autowrap */
			if (test_bit(QUES, &p->struct_flags))
				set_bit(DECAWM, &p->struct_flags);
			return;

		case 20:	/* Set cr lf */
			set_bit(CRLF, &p->struct_flags);
			return;
	}
}

static void csi_l(struct lcd_struct *p, unsigned char n)
{
	switch (n) {
		case 4:		/* Reset insert mode */
			clear_bit(DECIM, &p->struct_flags);
			return;

		case 5:		/* Normal screen mode */
			if (test_bit(QUES, &p->struct_flags) && test_bit(DECSCNM, &p->struct_flags)) {
				lcd_invert_screen(p, 0, p->fb_size);
				clear_bit(DECSCNM, &p->struct_flags);
				update_attr(p);
			}
			return;

		case 6:		/* Cursor addressing origin: absolute origin */
			if (test_bit(QUES, &p->struct_flags)) {
				clear_bit(DECOM, &p->struct_flags);
				gotoxay(p, 0, 0);
			}
			return;

		case 7:		/* Reset autowrap */
			if (test_bit(QUES, &p->struct_flags))
				clear_bit(DECAWM, &p->struct_flags);
			return;

		case 20:	/* Reset cr lf */
			clear_bit(CRLF, &p->struct_flags);
			return;
	}
}

static void csi_linux(struct lcd_struct *p)
{
	switch (p->csi_args[0]) {
	case 1:
		if (test_bit(CAN_DO_COLOR, &p->struct_flags) && p->csi_args[1] < 16) {
			p->ulcolor = color_table[p->csi_args[1]];
			if (p->attributes & ULINE)
				update_attr(p);
		}
		return;

	case 2:
		if (test_bit(CAN_DO_COLOR, &p->struct_flags) && p->csi_args[1] < 16) {
			p->halfcolor = color_table[p->csi_args[1]];
			if ((p->attributes & I_MASK) == 0)
				update_attr(p);
		}
		return;

	case 8:
		p->defcolor = p->color;
		default_attr(p);
		update_attr(p);
		return;
	}
}

static void csi_r(struct lcd_struct *p, unsigned int top, unsigned int bot)
{
	/* Minimum allowed region is 2 lines */
	if (top < bot) {
		p->top = top-1;
		p->bot = bot;
		gotoxay(p, 0, 0);
	}
}

/*
 * ECMA-48 CSI ESC- sequence handler.
 */
static void handle_csi(struct lcd_struct *p, unsigned char input)
{
	if (p->index >= NPAR) {
		SET_INPUT_STATE(p, NORMAL);
		printk(KERN_NOTICE "LCD: too many parameters in CSI escape sequence\n");
	} else if (input == '?') {
		set_bit(QUES, &p->struct_flags);
	} else if (input == ';') {
		++p->index;
	} else if (input >= '0' && input <= '9') {
		p->csi_args[p->index] = (p->csi_args[p->index]*10)+(input-'0');
	} else {
		SET_INPUT_STATE(p, NORMAL);
		if (! test_bit(INC_CURS_POS, &p->struct_flags))
			return;
		switch (input) {
		case 'h':		/* DECSET sequences and mode switches */
			csi_h(p, p->csi_args[0]);
			clear_bit(QUES, &p->struct_flags);
			return;

		case 'l':		/* DECRST sequences and mode switches */
			csi_l(p, p->csi_args[0]);
			clear_bit(QUES, &p->struct_flags);
			return;
		}
		clear_bit(QUES, &p->struct_flags);
		switch (input) {
		case '@':		/* Insert # Blank character */
			csi_at(p, p->csi_args[0]);
			return;

		case 'G': case '`':	/* Cursor to indicated column in current row */
			if (p->csi_args[0])
				--p->csi_args[0];
			gotoxy(p, p->csi_args[0], p->row);
			return;

		case 'A':		/* Cursor # rows Up */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, p->col, p->row-p->csi_args[0]);
			return;

		case 'B': case 'e':	/* Cursor # rows Down */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, p->col, p->row+p->csi_args[0]);
			return;

		case 'C': case 'a':	/* Cursor # columns Right */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, p->col+p->csi_args[0], p->row);
			return;

		case 'D':		/* Cursor # columns Left */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, p->col-p->csi_args[0], p->row);
			return;

		case 'E':		/* Cursor # rows Down, column 1 */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, 0, p->row+p->csi_args[0]);
			return;

		case 'F':		/* Cursor # rows Up, column 1 */
			if (! p->csi_args[0])
				++p->csi_args[0];
			gotoxy(p, 0, p->row-p->csi_args[0]);
			return;

		case 'd':		/* Cursor to indicated row in current column */
			if (p->csi_args[0])
				--p->csi_args[0];
			gotoxay(p, p->col, p->csi_args[0]);
			return;

		case 'H': case 'f':	/* Cursor to indicated row, column (origin 1, 1) */
			if (p->csi_args[0])
				--p->csi_args[0];
			if (p->csi_args[1])
				--p->csi_args[1];
			gotoxay(p, p->csi_args[1], p->csi_args[0]);
			return;

		case 'J':		/* Erase display */
			csi_J(p, p->csi_args[0]);
			return;

		case 'K':		/* Erase line */
			csi_K(p, p->csi_args[0]);
			return;

		case 'L':		/* Insert # blank lines */
			csi_L(p, p->csi_args[0]);
			return;

		case 'M':		/* Delete # blank lines */
			csi_M(p, p->csi_args[0]);
			return;

		case 'P':		/* Delete # characters on the current line */
			csi_P(p, p->csi_args[0]);
			return;

		case 'X':		/* Erase # characters on the current line */
			csi_X(p, p->csi_args[0]);
			return;

		case 'm':		/* Set video attributes */
			csi_m(p, p->index);
			return;

		case 's':		/* Save cursor position */
		case 'u':		/* Restore cursor position */
			csi_su(p, input);
			return;

		case ']':		/* Linux private ESC [ ] sequence */
			csi_linux(p);
			return;

		case 'r':		/* Set the scrolling region */
			if (! p->csi_args[0])
				++p->csi_args[0];
			if (! p->csi_args[1] || p->csi_args[1] > p->par->vs_rows)
		 		p->csi_args[1] = p->par->vs_rows;
			csi_r(p, p->csi_args[0], p->csi_args[1]);
			return;

					/* Ignored escape sequences */
		case 'c':
		case 'g':
		case 'n':
		case 'q':
			return;

		default:
			printk(KERN_NOTICE "LCD: unrecognized CSI escape sequence: ESC [ %u\n", input);
			return;
		}
	}
}

/*
 * Custom ESC- sequence handler.
 */
static int handle_custom_esc(struct lcd_struct *p, unsigned int _input)
{
	unsigned char input = _input & 0xff;
	struct lcd_parameters *par = p->par;

	if (_input & (~0xff)) {
		switch (ESC_STATE(p)) {
		case 's':
			if (p->index++) {
				unsigned char *cgbuf = p->cgram_buffer-par->cgram_bytes;

				cgbuf[p->index-2] = input;
				if (p->index == par->cgram_bytes+1)
					write_cgram(p, p->cgram_index, cgbuf);
			} else {
				if (! p->driver->write_cgram_char) {
					printk(KERN_ERR "LCD: %s: missing function to write to CGRAM\n", p->par->name);
					return (-1);
				}
				if (input >= par->cgram_char0 && input < par->cgram_char0+par->cgram_chars)
					p->cgram_index = input;
				else {
					printk(KERN_NOTICE "LCD: bad CGRAM index\n");
					return (-1);
				}
			}
			return (0);

		case 'G':
			if (input >= par->cgram_char0 && input < par->cgram_char0+par->cgram_chars)
				write_data(p, (p->attr << 8) | p->driver->charmap[input]);
			else {
				SET_INPUT_STATE(p, NORMAL);
				handle_input(p, (p->attr << 8) | input);
			}
			return (0);

		case 'r':
			if (input == '1')
				address_mode(p, -1);
			else if (input == '0')
				address_mode(p, 1);
			return (0);

		case 'A':
			scrup(p, p->top, p->bot, input);
			return (0);

		case 'B':
			scrdown(p, p->top, p->bot, input);
			return (0);

		case 'C':
			browse_screen(p, input);
			return (0);
		}
	}

	/* These are the custom ESC- sequences */
	switch (input) {
	case 's':	/* CGRAM select */
		if (p->cgram_buffer) {
			SET_ESC_STATE(p, input);
			p->index = 0;
			return (par->cgram_bytes+1);
		} else {
			printk(KERN_NOTICE "LCD: driver %s does not support CGRAM chars\n", par->name);
			return (0);
		}

	case 'A':	/* Scroll up */
	case 'B':	/* Scroll down */
	case 'C':	/* Browse screen */
	case 'G':	/* Enter cgram mode */
	case 'r':	/* Decrement counter after data read/write */
		SET_ESC_STATE(p, input);
		return (1);
	}

	return (-1);
}

/*
 * ESC- but not CSI sequence handler.
 */
static int handle_esc(struct lcd_struct *p, unsigned char input)
{
	int ret;

	SET_INPUT_STATE(p, NORMAL);
	switch (input) {
	case 'c':	/* Reset */
		set_bit(DECAWM, &p->struct_flags);
		set_bit(INC_CURS_POS, &p->struct_flags);
		ff(p);
		return (0);

	case 'D':	/* Line Feed */
		lf(p);
		return (0);

	case 'E':	/* New Line */
		cr(p);
		lf(p);
		return (0);

	case 'M':	/* Reverse Line Feed */
		ri(p);
		return (0);

	case '7':
	case '8':
		csi_su(p, (input == '7' ? 's' : 'u'));
		return (0);

	/* CSI: Start of CSI escape sequence */
	case '[':
		memset(p->csi_args, 0, sizeof(p->csi_args));
		p->index = 0;
		SET_INPUT_STATE(p, CSI);
		return (0);

	/* Ignored escape sequences */
	case '(':
		SET_INPUT_STATE(p, ESC_G0);
		return (1);

	case ')':
		SET_INPUT_STATE(p, ESC_G1);
		return (1);

	case '#':
		SET_INPUT_STATE(p, ESC_HASH);
		return (1);

	case '%':
		SET_INPUT_STATE(p, ESC_PERCENT);
		return (1);

	case 'H':
	case 'Z':
	case '>':
	case '=':
	case ']':
		return (0);
	}

	/* These are the custom ESC- sequences */
	if ((ret = handle_custom_esc(p, input)) > 0) {
		SET_INPUT_STATE(p, ARG);
		return (ret);
	}

	if (ret < 0 && p->driver->handle_custom_char)
		if ((ret = p->driver->handle_custom_char(input)) > 0) {
			SET_INPUT_STATE(p, ARG_DRIVER);
			return (ret);
		}

	if (ret < 0)
		printk(KERN_NOTICE "LCD: unrecognized escape sequence: ESC %u\n", input);

	return (0);
}

/*
 * Main input handler.
 */
static void handle_input(struct lcd_struct *p, unsigned short _input)
{
	unsigned char input = _input & 0xff;
	struct lcd_driver *driver = p->driver;

	switch (INPUT_STATE(p)) {
	case NORMAL:
		if (input < 0x20 || input == 0x9b)
			control_char(p, input);
		else
			write_data(p, (_input & 0xff00) | driver->charmap[input]);
		return;

	case RAW:
		write_data(p, (_input & 0xff00) | driver->charmap[input]);
		return;

	case SYN:
		write_data(p, _input);
		SET_INPUT_STATE(p, NORMAL);
		return;

	case ESC:
		p->esc_args = handle_esc(p, input);
		return;

	case ESC_G0:
	case ESC_G1:
	case ESC_HASH:
	case ESC_PERCENT:
		if (! --p->esc_args)
			SET_INPUT_STATE(p, NORMAL);
		return;

	case CSI:
		handle_csi(p, input);
		return;

	case ARG:
		if (handle_custom_esc(p, 0x100 | input) || ! --p->esc_args)
			SET_INPUT_STATE(p, NORMAL);
		return;

	case ARG_DRIVER:
		if (driver->handle_custom_char(0x100 | input) || ! --p->esc_args)
			SET_INPUT_STATE(p, NORMAL);
		return;
	}
}





/***************************************
 * Read from/Write to display routines *
 ***************************************/

/*
 * Write character data to the display.
 */
static void write_data(struct lcd_struct *p, unsigned short data)
{
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int pos;
	int frame_pos;

	if (test_bit(NEED_WRAP, &p->struct_flags)) {
		cr(p);
		lf(p);
	}

	if (test_bit(DECIM, &p->struct_flags))
		lcd_insert_char(p, 1);

	pos = (p->row*vs_cols)+p->col;
	if ((frame_pos = vs_to_frame_(p, pos)) < 0) {
		show_cursor(p);
		redraw_screen(p, 0, p->fb_size-1);
		frame_pos = vs_to_frame_(p, pos);
	}

	if (p->display[frame_pos] != data) {
		p->driver->write_char(frame_pos, data);
		p->display[frame_pos] = data;
	}

	p->fb[pos] = data;

	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		if (p->col+1 < vs_cols)
			iterator_inc(p->col, vs_cols);
		else if (test_bit(DECAWM, &p->struct_flags))
			set_bit(NEED_WRAP, &p->struct_flags);
	} else {
		if (p->col)
			iterator_dec(p->col, vs_cols);
		else if (test_bit(DECAWM, &p->struct_flags))
			set_bit(NEED_WRAP, &p->struct_flags);
	}
}

/*
 * Write an entire CGRAM character to the display.
 */
static void write_cgram(struct lcd_struct *p, unsigned char index, unsigned char *pixels)
{
	struct lcd_parameters *par = p->par;
	unsigned int inc_set = test_bit(INC_CURS_POS, &p->struct_flags);
	unsigned char *cgbuf = p->cgram_buffer+(index-par->cgram_char0)*par->cgram_bytes;

	if (! strncmp(cgbuf, pixels, par->cgram_bytes))
		return;

	if (! inc_set)
		address_mode(p, 1);

	p->driver->write_cgram_char(index, pixels);
	memcpy(cgbuf, pixels, par->cgram_bytes);

	if (! inc_set)
		address_mode(p, -1);
}

/*
 * Read character data from the display.
 */
static void read_data(struct lcd_struct *p, unsigned short *data)
{
	unsigned int vs_rows = p->par->vs_rows;
	unsigned int vs_cols = p->par->vs_cols;
	unsigned int pos = (p->row*vs_cols)+p->col;
	int frame_pos;

	if ((frame_pos = vs_to_frame_(p, pos)) < 0) {
		show_cursor(p);
		redraw_screen(p, 0, p->fb_size-1);
		frame_pos = vs_to_frame_(p, pos);
	}

	if (p->driver->read_char)
		p->driver->read_char(frame_pos, data);
	else
		*data = p->fb[pos];

	if (test_bit(INC_CURS_POS, &p->struct_flags)) {
		iterator_inc(p->col, vs_cols);
		if (! p->col) {
			if (p->row+1 < vs_rows)
				++p->row;
		}
	} else {
		iterator_dec(p->col, vs_cols);
		if (p->col+1 == vs_cols) {
			if (p->row)
				--p->row;
		}
	}
}

/*
 * Read an entire CGRAM character from the display.
 */
static void read_cgram(struct lcd_struct *p, unsigned char index, unsigned char *pixels)
{
	struct lcd_parameters *par = p->par;
	unsigned int inc_set = test_bit(INC_CURS_POS, &p->struct_flags);
	unsigned char *cgbuf = p->cgram_buffer+(index-par->cgram_char0)*par->cgram_bytes;

	if (! p->driver->read_cgram_char) {
		memcpy(pixels, cgbuf, par->cgram_bytes);
		return;
	}

	if (! inc_set)
		address_mode(p, 1);

	p->driver->read_cgram_char(index, pixels);

	if (! inc_set)
		address_mode(p, -1);
}





/****************************
 * Proc filesystem routines *
 ****************************/
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

static int proc_fb_read(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	struct lcd_struct *p = (struct lcd_struct *)data;
	unsigned int vs_cols;
	static unsigned int nr, need_wrap;
	static off_t _offset;

	down(&p->lcd_sem);
	if (! offset)
		_offset = 0;
	if ((*eof = (_offset >= p->fb_size))) {
		up(&p->lcd_sem);
		return (0);
	}
	vs_cols = p->par->vs_cols;
	if (size && need_wrap) {
		need_wrap = 0;
		temp += sprintf(temp, "\n");
		--size;
	}
	if (! nr)
		nr = vs_cols;
	*start = (char *)0;
	while (size && nr) {
		unsigned char c = (p->fb[_offset] & 0xff);

		temp += sprintf(temp, "%c", (c < 0x20 ? '?' : c));
		--size;
		--nr;
		++*start;
		++_offset;
	}
	if (! nr) {
		if (size) {
			temp += sprintf(temp, "\n");
			--size;
		} else
			need_wrap = 1;
	}
	up(&p->lcd_sem);

	return (temp-buffer);
}

static int proc_display_read(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	struct lcd_struct *p = (struct lcd_struct *)data;
	unsigned int i, frame_cols;
	int frame_pos;

	down(&p->lcd_sem);
	frame_cols = p->par->cntr_cols;
	frame_pos = vs_to_frame_(p, (p->row*p->par->vs_cols)+p->col);
	temp += sprintf(temp, "    ");
	for (i = 2; i <= frame_cols; i += 2)
		temp += sprintf(temp, " %d", i%10);
	temp += sprintf(temp, "\n");

	temp += sprintf(temp, "   +");
	for (i = 0; i < frame_cols; ++i)
		temp += sprintf(temp, "-");
	temp += sprintf(temp, "+\n");

	for (i = 0; i < p->frame_size; ++i) {
		unsigned char c = (p->display[i] & 0xff);

		if (! (i%frame_cols))
			temp += sprintf(temp, "%2d |", 1+i/frame_cols);
		if (frame_pos--)
			temp += sprintf(temp, "%c", (c < 0x20 ? '?' : c));
		else
			temp += sprintf(temp, "_");
		if (! ((i+1)%frame_cols))
			temp += sprintf(temp, "|\n");
	}

	temp += sprintf(temp, "   +");
	for (i = 0; i < frame_cols; ++i)
		temp += sprintf(temp, "-");
	temp += sprintf(temp, "+\n");
	up(&p->lcd_sem);

	return (temp-buffer);
}

static int proc_charmap_read(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	struct lcd_struct *p = (struct lcd_struct *)data;
	unsigned char *charmap;
	unsigned int i;

	down(&p->lcd_sem);
	charmap = p->driver->charmap;
	temp += sprintf(temp,	"static unsigned char charmap[] = {");
	for (i = 0; i < 255; ++i) {
		if (! (i & 7)) {
			temp += sprintf(temp, "\n");
			if (! (i & 31))
				temp += sprintf(temp, "\n/* %d - %d */\n", i, i+31);
		}
		temp += sprintf(temp, "0x%.2x, ", *charmap++);
	}
	temp += sprintf(temp, "0x%.2x\n\n};\n", *charmap);
	up(&p->lcd_sem);

	return (temp-buffer);
}

static int proc_registered_drivers(char *buffer, char **start, off_t offset, int size, int *eof, void *data)
{
	char *temp = buffer;
	struct list_head *entry;

	down(&drivers_sem);
	temp += sprintf(temp, "Registered drivers:\n");
	list_for_each(entry, &lcd_drivers) {
		struct lcd_struct *p = list_entry(entry, struct lcd_struct, lcd_list);

		down(&p->lcd_sem);
		temp += sprintf(temp, "%3d %s\n", p->par->minor, p->par->name);
		up(&p->lcd_sem);
	}
	up(&drivers_sem);

	return (temp-buffer);
}

static void create_driver_proc_entries(struct lcd_struct *p)
{
	struct proc_dir_entry *driver_proc_root = p->driver->driver_proc_root;

	SET_PROC_LEVEL(p, 0);
	if (create_proc_read_entry("framebuffer", 0, driver_proc_root, proc_fb_read, p) == NULL) {
		printk(KERN_ERR "LCD: cannot create /proc/lcd/%s/framebuffer\n", p->par->name);
		return;
	}
	SET_PROC_LEVEL(p, 1);
	if (create_proc_read_entry("display", 0, driver_proc_root, proc_display_read, p) == NULL) {
		printk(KERN_ERR "LCD: cannot create /proc/lcd/%s/display\n", p->par->name);
		return;
	}
	SET_PROC_LEVEL(p, 2);
	if (create_proc_read_entry("charmap.h", 0, driver_proc_root, proc_charmap_read, p) == NULL) {
		printk(KERN_ERR "LCD: cannot create /proc/lcd/%s/charmap.h\n", p->par->name);
		return;
	}
	SET_PROC_LEVEL(p, 3);
}

static void remove_driver_proc_entries(struct lcd_struct *p)
{
	struct proc_dir_entry *driver_proc_root = p->driver->driver_proc_root;

	switch (PROC_LEVEL(p)) {
	case 3:
		remove_proc_entry("charmap.h", driver_proc_root);
	case 2:
		remove_proc_entry("display", driver_proc_root);
	case 1:
		remove_proc_entry("framebuffer", driver_proc_root);
	}
	SET_PROC_LEVEL(p, 0);
}
#endif





/*****************************
 * Low level file operations *
 *****************************/
static ssize_t do_lcd_read(struct lcd_struct *p, void *buffer, size_t length)
{
	unsigned int i;
	unsigned short tmp;

	if (! p->refcount)
		return (-ENXIO);

	if (test_bit(WITH_ATTR, &p->struct_flags))
		for (i = 0; i < length; ++i) {
			read_data(p, &tmp);
			((unsigned short *)buffer)[i] = tmp;
		}
	else
		for (i = 0; i < length; ++i) {
			read_data(p, &tmp);
			((unsigned char *)buffer)[i] = tmp & 0xff;
		}

	return (length);
}

static ssize_t do_lcd_write(struct lcd_struct *p, const void *buffer, size_t length)
{
	unsigned int i;

	if (! p->refcount)
		return (-ENXIO);

	if (test_bit(WITH_ATTR, &p->struct_flags))
		for (i = 0; i < length; ++i)
			handle_input(p, ((const unsigned short *)buffer)[i]);
	else
		for (i = 0; i < length; ++i)
			handle_input(p, (p->attr << 8) | ((const unsigned char *)buffer)[i]);

	return (length);
}

static int do_lcd_open(struct lcd_struct *p)
{
	if (! p->refcount) {
		if (p->driver->driver_module) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
			if (! try_module_get(p->driver->driver_module))
				return (-EBUSY);
#else
			if (__MOD_IN_USE(p->driver->driver_module))
				return (-EBUSY);

			__MOD_INC_USE_COUNT(p->driver->driver_module);
#endif
		}
	}

	++p->refcount;

	return (0);
}

static int do_lcd_release(struct lcd_struct *p)
{
	if (! p->refcount)
		return (0);

	if (p->refcount == 1) {
		if (p->driver->driver_module)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
			module_put(p->driver->driver_module);
#else
			__MOD_DEC_USE_COUNT(p->driver->driver_module);
#endif
	}

	--p->refcount;

	return (0);
}

static int cgram_ioctl(struct lcd_struct *p, unsigned int cmd, unsigned char *argp)
{
	struct lcd_parameters *par = p->par;
	unsigned int length = par->cgram_bytes;
	unsigned char index = argp[0];
	unsigned char *buffer = argp+1;
	unsigned char *cgbuf = p->cgram_buffer-length;

	if (index < par->cgram_char0 || index >= par->cgram_char0+par->cgram_chars)
		return (-EINVAL);

	if (! p->driver->write_cgram_char) {
		printk(KERN_ERR "LCD: %s: missing function to write to CGRAM\n", p->par->name);
		return (-ENOSYS);
	}

	if (cmd == LCDL_SET_CGRAM_CHAR) {
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_from_user(cgbuf, buffer, length))
				return (-EFAULT);
		} else
			memcpy(cgbuf, buffer, length);
		write_cgram(p, index, cgbuf);
	} else {
		read_cgram(p, index, cgbuf);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_to_user(buffer, cgbuf, length))
				return (-EFAULT);
		} else
			memcpy(buffer, cgbuf, length);
	}

	return (0);
}

static int do_lcd_ioctl(struct lcd_struct *p, unsigned int cmd, unsigned long arg)
{
	int i;
	struct lcd_driver *driver = p->driver;
	struct lcd_parameters *par = p->par;
	unsigned char *argp = (unsigned char *)arg;

	if (! p->refcount)
		return (-ENXIO);

	switch (cmd) {
	case LCDL_SET_PARAM:
		if (argp == NULL)
			return (-EFAULT);
		if ((i = cleanup_driver(p)))
			return (i);
		i = par->minor;
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_from_user(par, argp, sizeof(struct lcd_parameters)))
				return (-EFAULT);
		} else
			memcpy(par, argp, sizeof(struct lcd_parameters));
		par->minor = i;
		return (init_driver(p));

	case LCDL_GET_PARAM:
		if (argp == NULL)
			return (-EFAULT);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_to_user(argp, par, sizeof(struct lcd_parameters)))
				return (-EFAULT);
		} else
			memcpy(argp, par, sizeof(struct lcd_parameters));
		return (0);

	case LCDL_RESET_CHARMAP:
		for (i = 0; i < 256; ++i)
			driver->charmap[i] = i;
		return (0);

	case LCDL_CHARSUBST:
		if (argp == NULL)
			return (-EFAULT);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			get_user(i, argp);
			get_user(driver->charmap[i], argp+1);
		} else {
			i = argp[0];
			driver->charmap[i] = argp[1];
		}
		return (0);

	case LCDL_SAVE_CHARMAP:
		memcpy(p->s_charmap, driver->charmap, 256);
		return (0);

	case LCDL_RESTORE_CHARMAP:
		memcpy(driver->charmap, p->s_charmap, 256);
		return (0);

	case LCDL_SWAP_CHARMAP:
		{
			unsigned char *tmp;

			tmp = driver->charmap;
			driver->charmap = p->s_charmap;
			p->s_charmap = tmp;
		}
		return (0);

	case LCDL_RAW_MODE:
		if (arg) {
			clear_bit(NEED_WRAP, &p->struct_flags);
			clear_bit(DECIM, &p->struct_flags);
			clear_bit(DECAWM, &p->struct_flags);
			SET_INPUT_STATE(p, RAW);
		} else {
			set_bit(DECAWM, &p->struct_flags);
			SET_INPUT_STATE(p, NORMAL);
		}
		return (0);

	case LCDL_IOATTR:
		if (arg)
			set_bit(WITH_ATTR, &p->struct_flags);
		else
			clear_bit(WITH_ATTR, &p->struct_flags);
		return (0);

	case LCDL_CLEAR_DISP:
		ff(p);
		return (0);

	case LCDL_SET_CGRAM_CHAR:
	case LCDL_GET_CGRAM_CHAR:
		if (argp == NULL)
			return (-EFAULT);
		if (p->cgram_buffer)
			return (cgram_ioctl(p, cmd, argp));
		else
			printk(KERN_NOTICE "LCD: driver %s does not support CGRAM chars\n", par->name);
		return (0);

	case LCDL_SET_CHARMAP:
		if (argp == NULL)
			return (-EFAULT);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_from_user(driver->charmap, argp, 256))
				return (-EFAULT);
		} else
			memcpy(driver->charmap, argp, 256);
		return (0);

	case LCDL_GET_CHARMAP:
		if (argp == NULL)
			return (-EFAULT);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_to_user(argp, driver->charmap, 256))
				return (-EFAULT);
		} else
			memcpy(argp, driver->charmap, 256);
		return (0);

	case LCDL_MEMSET:
	case LCDL_MEMMOVE:
	{
		int buf[3];

		if (argp == NULL)
			return (-EFAULT);
		if (test_bit(USER_SPACE, &p->struct_flags)) {
			if (copy_from_user(buf, argp, sizeof(buf)))
				return (-EFAULT);
		} else
			memcpy(buf, argp, sizeof(buf));

		if (cmd == LCDL_MEMSET)
			lcd_memset(p, buf[0], buf[1], buf[2]);
		else
			lcd_memmove(p, buf[0], buf[1], buf[2]);

		return (0);
	}

	default:
		if (driver->handle_custom_ioctl)
			return (driver->handle_custom_ioctl(cmd, arg, test_bit(USER_SPACE, &p->struct_flags)));
	}

	return (-ENOIOCTLCMD);
}





/**************************************************
 * Kernel register/unregister lcd driver routines *
 **************************************************/
/*
 * Find a driver in lcd_drivers linked list
 */
static struct lcd_struct *find_lcd_struct(unsigned short minor)
{
	struct list_head *entry;

	list_for_each(entry, &lcd_drivers) {
		struct lcd_struct *p = list_entry(entry, struct lcd_struct, lcd_list);

		if (p->par->minor == minor)
			return (p);
	}

	return (NULL);
}

static void list_add_sorted(struct list_head *new)
{
	struct list_head *entry;
	unsigned short new_minor = (list_entry(new, struct lcd_struct, lcd_list))->par->minor;

	list_for_each(entry, &lcd_drivers) {
		struct lcd_struct *p = list_entry(entry, struct lcd_struct, lcd_list);

		if (p->par->minor > new_minor)
			break;
	}
	list_add_tail(new, entry);
}

/* Exported function */
int lcd_register_driver(struct lcd_driver *driver, struct lcd_parameters *par)
{
	int ret;
	struct lcd_struct *p;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
	struct device *lcd_device;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	struct class_device *lcd_device;
#endif

	if (! driver || ! par || par->minor >= minors)
		return (-EINVAL);
	if (! driver->write_char || ! driver->init_port || ! driver->cleanup_port) {
		printk(KERN_ERR "LCD: missing functions\n");
		return (-EINVAL);
	}

	down(&drivers_sem);

	if (find_lcd_struct(par->minor)) {
		up(&drivers_sem);
		return (-EBUSY);
	}

	if ((p = (struct lcd_struct *)kmalloc(sizeof(struct lcd_struct), GFP_KERNEL)) == NULL) {
		printk(KERN_ERR "LCD: memory allocation failed (kmalloc)\n");
		up(&drivers_sem);
		return (-ENOMEM);
	}
	memset(p, 0, sizeof(struct lcd_struct));

	p->driver = driver;
	p->par = par;
	p->refcount = 0;
	SET_INIT_LEVEL(p, 0);
	SET_INPUT_STATE(p, NORMAL);
	set_bit(DECAWM, &p->struct_flags);
	set_bit(INC_CURS_POS, &p->struct_flags);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
	lcd_device = device_create(lcd_linux_class, NULL, MKDEV(major, par->minor), NULL, "%s", par->name);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
	lcd_device = device_create(lcd_linux_class, NULL, MKDEV(major, par->minor), "%s", par->name);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
	lcd_device = class_device_create(lcd_linux_class, NULL, MKDEV(major, par->minor), NULL, "%s", par->name);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	lcd_device = class_device_create(lcd_linux_class, MKDEV(major, par->minor), NULL, "%s", par->name);
#endif
	if (IS_ERR(lcd_device)) {
		kfree(p);
		up(&drivers_sem);
		return (PTR_ERR(lcd_device));
	}
#endif

#ifdef USE_PROC
	if (lcd_proc_root && (driver->driver_proc_root = proc_mkdir(par->name, lcd_proc_root)) == NULL)
		printk(KERN_ERR "LCD: cannot create /proc/lcd/%s/\n", par->name);
#endif

	if ((ret = init_driver(p))) {
#ifdef USE_PROC
		if (driver->driver_proc_root)
			remove_proc_entry(p->par->name, lcd_proc_root);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		device_destroy(lcd_linux_class, MKDEV(major, par->minor));
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
		class_device_destroy(lcd_linux_class, MKDEV(major, par->minor));
#endif

		kfree(p);
		up(&drivers_sem);
		return (ret);
	}

	init_MUTEX(&p->lcd_sem);

	list_add_sorted(&p->lcd_list);

	up(&drivers_sem);

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	try_module_get(THIS_MODULE);
#else
	MOD_INC_USE_COUNT;
#endif
#endif

	return (0);
}
EXPORT_SYMBOL(lcd_register_driver);

/* Exported function */
int lcd_unregister_driver(struct lcd_driver *driver, struct lcd_parameters *par)
{
	int ret;
	struct lcd_struct *p;

	if (! driver || ! par || par->minor >= minors)
		return (-EINVAL);

	down(&drivers_sem);

	if ((p = find_lcd_struct(par->minor)) == NULL || p->driver != driver) {
		printk(KERN_ERR "LCD: driver not found; lcd_unregister_driver failed\n");
		up(&drivers_sem);
		return (-ENODEV);
	}

	down(&p->lcd_sem);

	if (p->refcount) {
		printk(KERN_ERR "LCD: driver busy; lcd_unregister_driver failed\n");
		up(&p->lcd_sem);
		up(&drivers_sem);
		return (-EBUSY);
	}

	if ((ret = cleanup_driver(p))) {
		up(&p->lcd_sem);
		up(&drivers_sem);
		return (ret);
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
	device_destroy(lcd_linux_class, MKDEV(major, par->minor));
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	class_device_destroy(lcd_linux_class, MKDEV(major, par->minor));
#endif

#ifdef USE_PROC
	if (p->driver->driver_proc_root)
		remove_proc_entry(p->par->name, lcd_proc_root);
#endif

	list_del(&p->lcd_list);
	kfree(p);

	up(&drivers_sem);

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	module_put(THIS_MODULE);
#else
	MOD_DEC_USE_COUNT;
#endif
#endif

	return (0);
}
EXPORT_SYMBOL(lcd_unregister_driver);





/************************
 * Kernel I/O interface *
 ************************/
/* Exported function */
int lcd_open(unsigned short minor, struct lcd_struct **pp)
{
	int ret;
	struct lcd_struct *p;

	down(&drivers_sem);

	if (minor >= minors || (*pp = p = find_lcd_struct(minor)) == NULL) {
		printk(KERN_ERR "LCD: lcd_open failed. Device not found.\n");
		up(&drivers_sem);
		return (-ENODEV);
	}

	down(&p->lcd_sem);
	up(&drivers_sem);

	ret = do_lcd_open(p);

	up(&p->lcd_sem);

	return (ret);
}
EXPORT_SYMBOL(lcd_open);

/* Exported function */
int lcd_close(struct lcd_struct **pp)
{
	int ret;
	struct lcd_struct *p;

	if (! pp || ! (p = *pp)) {
		printk(KERN_ERR "LCD: NULL pointer in lcd_close\n");
		return (-ENODEV);
	}

	down(&p->lcd_sem);

	if (! (ret = do_lcd_release(p)))
		*pp = NULL;

	up(&p->lcd_sem);

	return (ret);
}
EXPORT_SYMBOL(lcd_close);

static inline loff_t offset_to_row_col(struct lcd_struct *p, loff_t offset)
{
	unsigned long _offset = offset;
	unsigned int vs_cols = p->par->vs_cols;

	gotoxy(p, _offset%vs_cols, _offset/vs_cols);

	return ((p->row*vs_cols)+p->col);
}

/* Exported function */
ssize_t lcd_read(struct lcd_struct *p, void *bufp, size_t length, loff_t offset, unsigned int with_attr)
{
	ssize_t ret = 0;

	if (! p) {
		printk(KERN_ERR "LCD: NULL pointer in lcd_read\n");
		return (-ENODEV);
	}
	if (! bufp)
		return (-EFAULT);
	if (offset < 0 || offset >= p->fb_size)
		return (-EINVAL);

	if (length+offset > p->fb_size)
		length = p->fb_size-offset;

	if (with_attr)
		set_bit(WITH_ATTR, &p->struct_flags);

	offset_to_row_col(p, offset);
	ret = do_lcd_read(p, bufp, length);

	if (with_attr)
		clear_bit(WITH_ATTR, &p->struct_flags);

	return (ret);
}
EXPORT_SYMBOL(lcd_read);

/* Exported function */
ssize_t lcd_write(struct lcd_struct *p, const void *bufp, size_t length, loff_t offset, unsigned int with_attr)
{
	ssize_t ret;

	if (! p) {
		printk(KERN_ERR "LCD: NULL pointer in lcd_write\n");
		return (-ENODEV);
	}
	if (! bufp)
		return (-EFAULT);
	if (offset < 0 || offset >= p->fb_size)
		return (-EINVAL);

	if (with_attr)
		set_bit(WITH_ATTR, &p->struct_flags);

	offset_to_row_col(p, offset);
	ret = do_lcd_write(p, bufp, length);

	if (with_attr)
		clear_bit(WITH_ATTR, &p->struct_flags);

	return (ret);
}
EXPORT_SYMBOL(lcd_write);

/* Exported function */
int lcd_ioctl(struct lcd_struct *p, unsigned int cmd, ...)
{
	int ret;
	unsigned long arg;
	va_list ap;

	if (! p) {
		printk(KERN_ERR "LCD: NULL pointer in lcd_ioctl\n");
		return (-ENODEV);
	}

	down(&p->lcd_sem);
	va_start(ap, cmd);
	arg = va_arg(ap, unsigned long);
	ret = do_lcd_ioctl(p, cmd, arg);
	va_end(ap);
	up(&p->lcd_sem);

	return (ret);
}
EXPORT_SYMBOL(lcd_ioctl);





/*******************
 * File operations *
 *******************/
static loff_t lcd_fops_llseek(struct file *filp, loff_t offset, int orig)
{
	struct lcd_struct *p;

	if (! (p = filp->private_data))
		return (-ENODEV);

	down(&p->lcd_sem);

	switch (orig) {
	case 0:
		break;

	case 1:
		offset += filp->f_pos;
		break;

	default:
		up(&p->lcd_sem);
		return (-EINVAL);     /* SEEK_END not supported */
	}

	if (offset >= 0 && offset < p->fb_size)
		filp->f_pos = offset_to_row_col(p, offset);
	else
		offset = -EINVAL;

	up(&p->lcd_sem);

	return (offset);
}

static ssize_t lcd_fops_read(struct file *filp, char *buffer, size_t length, loff_t *offp)
{
	ssize_t ret = 0;
	char *bufp = buffer;
	struct lcd_struct *p;

	if (! bufp)
		return (-EFAULT);
	if (! (p = filp->private_data))
		return (-ENODEV);

	down(&p->lcd_sem);

	if (*offp < 0 || *offp >= p->fb_size) {
		up(&p->lcd_sem);
		return (-EINVAL);
	}

	if (test_bit(WITH_ATTR, &p->struct_flags) && (length & 0x01))
		length &= ~0x01;

	if (length+(*offp) > p->fb_size)
		length = p->fb_size-(*offp);

	while (length) {
		ret = (length > FLIP_BUF_SIZE ? FLIP_BUF_SIZE : length);
		if (test_bit(WITH_ATTR, &p->struct_flags))
			ret /= sizeof(unsigned short);
		if ((ret = do_lcd_read(p, p->flip_buf, ret)) < 0)
			break;
		if (test_bit(WITH_ATTR, &p->struct_flags))
			ret *= sizeof(unsigned short);
		*offp = (p->row*p->par->vs_cols)+p->col;
		if (copy_to_user(bufp, p->flip_buf, ret)) {
			ret = -EFAULT;
			break;
		}
		length -= ret;
		bufp += ret;
		ret = bufp-buffer;
		if (length)
			schedule();
	}

	up(&p->lcd_sem);

	return (ret);
}

static ssize_t lcd_fops_write(struct file *filp, const char *buffer, size_t length, loff_t *offp)
{
	ssize_t ret = 0;
	const char *bufp = buffer;
	struct lcd_struct *p;

	if (! bufp)
		return (-EFAULT);
	if (! (p = filp->private_data))
		return (-ENODEV);

	down(&p->lcd_sem);

	if (*offp < 0 || *offp >= p->fb_size) {
		up(&p->lcd_sem);
		return (-EINVAL);
	}

	if (test_bit(WITH_ATTR, &p->struct_flags) && (length & 0x01))
		length &= ~0x01;

	while (length) {
		ret = (length > FLIP_BUF_SIZE ? FLIP_BUF_SIZE : length);
		if (copy_from_user(p->flip_buf, bufp, ret)) {
			ret = -EFAULT;
			break;
		}
		if (test_bit(WITH_ATTR, &p->struct_flags))
			ret /= sizeof(unsigned short);
		if ((ret = do_lcd_write(p, p->flip_buf, ret)) < 0)
			break;
		if (test_bit(WITH_ATTR, &p->struct_flags))
			ret *= sizeof(unsigned short);
		*offp = (p->row*p->par->vs_cols)+p->col;
		length -= ret;
		bufp += ret;
		ret = bufp-buffer;
		if (length)
			schedule();
	}

	up(&p->lcd_sem);

	return (ret);
}

static int lcd_fops_open(struct inode *inop, struct file *filp)
{
	unsigned short minor;
	int ret;
	struct lcd_struct *p;

	down(&drivers_sem);

	if ((minor = MINOR(inop->i_rdev)) >= minors || (filp->private_data = p = find_lcd_struct(minor)) == NULL) {
		up(&drivers_sem);
		return (-ENODEV);
	}

	down(&p->lcd_sem);
	up(&drivers_sem);

	ret = do_lcd_open(p);

	up(&p->lcd_sem);

	return (ret);
}

static int lcd_fops_release(struct inode *inop, struct file *filp)
{
	struct lcd_struct *p;
	int ret;

	if (! (p = filp->private_data))
		return (-ENODEV);

	down(&p->lcd_sem);

	if (! (ret = do_lcd_release(p)))
		filp->private_data = NULL;

	up(&p->lcd_sem);

	return (ret);
}

static int lcd_fops_ioctl(struct inode *inop, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct lcd_struct *p;
	int ret;

	if (! (p = filp->private_data))
		return (-ENODEV);

	down(&p->lcd_sem);

	set_bit(USER_SPACE, &p->struct_flags);
	ret = do_lcd_ioctl(p, cmd, arg);
	clear_bit(USER_SPACE, &p->struct_flags);

	up(&p->lcd_sem);

	return (ret);
}

static struct file_operations lcd_linux_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
	.owner		= THIS_MODULE,
#endif
	.llseek		= lcd_fops_llseek,
	.read		= lcd_fops_read,
	.write		= lcd_fops_write,
	.open		= lcd_fops_open,
	.release	= lcd_fops_release,
	.ioctl		= lcd_fops_ioctl,
};





/********************************
 * Init/Cleanup driver routines *
 ********************************/
static int do_init_driver(struct lcd_struct *p)
{
	int ret;
	struct lcd_driver *driver = p->driver;
	struct lcd_parameters *par = p->par;
	unsigned int frame_rows = par->cntr_rows*par->num_cntr;
	unsigned int frame_cols = par->cntr_cols;

	switch (INIT_LEVEL(p)) {
	case 0:
		if (frame_rows == 0 || frame_cols == 0 || ! par->name) {
			printk(KERN_ERR "LCD: wrong lcd parameters\n");
			return (-EINVAL);
		}
		if (driver->validate_driver) {
			if ((ret = driver->validate_driver()) < 0) {
				printk(KERN_ERR "LCD: validate_driver failed\n");
				return (-EINVAL);
			} else if (ret > 0) {
				set_bit(CAN_DO_COLOR, &p->struct_flags);
				p->defcolor = 0x07;
				p->ulcolor = 0x0f;
				p->halfcolor = 0x08;
			}
		}
		default_attr(p);
		update_attr(p);
		p->frame_size = frame_rows*frame_cols;
		if (par->vs_rows < frame_rows)
			par->vs_rows = frame_rows;
		if (par->vs_cols < frame_cols)
			par->vs_cols = frame_cols;
		p->fb_size = par->vs_rows*par->vs_cols;

		ret = sizeof(short)*p->fb_size;
		ret += sizeof(short)*p->frame_size;
		ret += FLIP_BUF_SIZE;
		ret += (p->driver->charmap ? 256 : 512);
		if (par->cgram_chars*par->cgram_bytes)
			ret += (1+par->cgram_chars)*par->cgram_bytes;
		if ((p->fb = (unsigned short *)vmalloc(ret)) == NULL) {
			printk(KERN_ERR "LCD: memory allocation failed (vmalloc)\n");
			return (-ENOMEM);
		}
		__memset_short(p->fb, p->erase_char, p->fb_size+p->frame_size);

		p->display = p->fb+p->fb_size;
		p->flip_buf = (unsigned char *)(p->display+p->frame_size);

		if (! p->driver->charmap) {
			set_bit(NULL_CHARMAP, &p->struct_flags);
			p->driver->charmap = p->flip_buf+FLIP_BUF_SIZE;
			for (ret = 0; ret < 256; ++ret)
				p->driver->charmap[ret] = ret;
			p->s_charmap = p->driver->charmap+256;
		} else
			p->s_charmap = p->flip_buf+FLIP_BUF_SIZE;
		memset(p->s_charmap, 0, 256);

		if (par->cgram_chars*par->cgram_bytes) {
			p->cgram_buffer = p->s_charmap+256+par->cgram_bytes;
			memset(p->cgram_buffer, 0xff, par->cgram_chars*par->cgram_bytes);
		} else
			p->cgram_buffer = NULL;
		p->driver->cgram_buffer = p->cgram_buffer;

		p->frame_base = 0;
		p->row = p->col = 0;
		p->top = 0;
		p->bot = par->vs_rows;
		SET_INIT_LEVEL(p, 1);

	case 1:
		/* Initialize the communication port */
		if ((ret = driver->init_port())) {
			printk(KERN_ERR "LCD: failure while initializing the communication port\n");
			return (ret);
		}
		SET_INIT_LEVEL(p, 2);

	case 2:
		/* Initialize LCD display */
		if (driver->init_display && (ret = driver->init_display())) {
			printk(KERN_ERR "LCD: failure while initializing the display\n");
			return (ret);
		}

#ifdef USE_PROC
		/* Create entries in /proc/lcd/"driver" */
		if (driver->driver_proc_root)
			create_driver_proc_entries(p);
#endif
		SET_INIT_LEVEL(p, 3);
	}

	return (0);
}

static int do_cleanup_driver(struct lcd_struct *p)
{
	int ret;
	struct lcd_driver *driver = p->driver;

	switch (INIT_LEVEL(p)) {
	case 3:
#ifdef USE_PROC
		if (driver->driver_proc_root)
			remove_driver_proc_entries(p);
#endif
		if (driver->cleanup_display && (ret = driver->cleanup_display())) {
			printk(KERN_ERR "LCD: failure while cleaning the display\n");
			return (ret);
		}
		SET_INIT_LEVEL(p, 2);

	case 2:
		if ((ret = driver->cleanup_port())) {
			printk(KERN_ERR "LCD: failure while cleaning the communication port\n");
			return (ret);
		}
		SET_INIT_LEVEL(p, 1);

	case 1:
		if (test_bit(NULL_CHARMAP, &p->struct_flags)) {
			p->driver->charmap = NULL;
			clear_bit(NULL_CHARMAP, &p->struct_flags);
		}
		vfree(p->fb);
		p->fb = NULL;
		SET_INIT_LEVEL(p, 0);
	}

	return (0);
}

static int init_driver(struct lcd_struct *p)
{
	int ret;

	if ((ret = do_init_driver(p))) {
		do_cleanup_driver(p);
		printk(KERN_ERR "LCD: init_driver failed\n");
	}

	return (ret);
}

static int cleanup_driver(struct lcd_struct *p)
{
	int ret;

	if ((ret = do_cleanup_driver(p))) {
		do_init_driver(p);
		printk(KERN_ERR "LCD: cleanup_driver failed\n");
	}

	return (ret);
}





/********************************
 * Init/Cleanup module routines *
 ********************************/
static int __init lcd_linux_init_module(void)
{
	int ret;

	if (! minors || minors > 256)
		minors = LCD_MINORS;

	init_MUTEX(&drivers_sem);

	if ((ret = register_chrdev(major, LCD_LINUX_STRING, &lcd_linux_fops)) < 0) {
		printk(KERN_ERR "LCD: register_chrdev failed\n");
		return (ret);
	}
	if (major == 0)
		major = ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	if (IS_ERR((lcd_linux_class = class_create(THIS_MODULE, "lcd")))) {
		ret = PTR_ERR(lcd_linux_class);
		unregister_chrdev(major, LCD_LINUX_STRING);
		return (ret);
	}
#endif

#ifdef USE_PROC
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
	if ((lcd_proc_root = proc_mkdir("lcd", NULL)) == NULL)
#else
	if ((lcd_proc_root = proc_mkdir("lcd", &proc_root)) == NULL)
#endif
		printk(KERN_ERR "LCD: cannot create /proc/lcd/\n");
	else if (create_proc_read_entry("drivers", 0, lcd_proc_root, proc_registered_drivers, NULL) == NULL)
		printk(KERN_ERR "LCD: cannot create /proc/lcd/drivers\n");
#endif

	printk(KERN_INFO "LCD: --> LCD-Linux " LCD_LINUX_VERSION " <--\n");
	printk(KERN_INFO "LCD: --> Mattia Jona-Lasinio <mjona@users.sourceforge.net> <--\n" );


	return (0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void __exit lcd_linux_cleanup_module(void)
#else
/* __exit is not defined in 2.2.x kernels */
static void lcd_linux_cleanup_module(void)
#endif
{
#ifdef USE_PROC
	if (lcd_proc_root) {
		remove_proc_entry("drivers", lcd_proc_root);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
		remove_proc_entry("lcd", NULL);
#else
		remove_proc_entry("lcd", &proc_root);
#endif
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	class_destroy(lcd_linux_class);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
	unregister_chrdev(major, LCD_LINUX_STRING);
#else
	if (unregister_chrdev(major, LCD_LINUX_STRING))
		printk(KERN_ERR "LCD: unregister_chrdev failed\n");
#endif
}

module_init(lcd_linux_init_module)
module_exit(lcd_linux_cleanup_module)
