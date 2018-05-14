/* lcdcon.c
 *
 * $Id: lcdcon.c,v 1.7 2009/03/09 17:59:23 mjona Exp $
 *
 * Console driver for hd44780 displays based on LCD-Linux.
 *
 * Copyright (C) 2007 - 2009 Mattia Jona-Lasinio (mjona@users.sourceforge.net)
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
#include <linux/kernel.h>
#include <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/console_struct.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
#include <linux/init.h>
#endif  /* Linux_2.0_support */

#include <linux/console.h>
#include <linux/vt_kern.h>

#define LCD_LINUX_MAIN
#include <linux/hd44780.h>

/* the display */
static struct lcd_struct *lcd;

/* console information */
static int lcdl_first_vc = 1;
static int lcdl_last_vc = 15;
static int lcdl_rows, lcdl_cols;
static struct vc_data *lcd_linux_display_fg;

#ifdef MODULE
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
#include <linux/kmod.h>
#else  /* Linux_2.0_support */
#include <linux/kerneld.h>  /* Linux_2.0_support */
#endif  /* Linux_2.0_support */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
MODULE_DESCRIPTION("Kernel console driver based on LCD-Linux hd44780 displays");
MODULE_AUTHOR("Mattia Jona-Lasinio <mjona@users.sourceforge.net>");
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
#endif  /* Linux_2.0_support */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
module_param(lcdl_first_vc, int, 0444);
module_param(lcdl_last_vc, int, 0444);
#else
MODULE_PARM(lcdl_first_vc, "i");
MODULE_PARM(lcdl_last_vc, "i");
#endif
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */

/*
 * Parse boot command line
 *
 * lcdcon=first,last
 */
static int __init lcdcon_setup(char *str)
{
	int ints[3];

	str = get_options(str, ARRAY_SIZE(ints), ints);

	if (ints[0] < 2)
		return (0);

	if (ints[1] < 0 || ints[1] >= MAX_NR_CONSOLES || ints[2] < 0 || ints[2] >= MAX_NR_CONSOLES)
		return (0);

	lcdl_first_vc = ints[1];
	lcdl_last_vc = ints[2];

	return (1);
}

__setup("lcdcon=", lcdcon_setup);
#endif  /* Linux_2.0_support */
#endif

static const char *lcdcon_startup(void)
{
	struct lcd_parameters par;

	lcd_open(HD44780_MINOR, &lcd);
	lcd_ioctl(lcd, LCDL_GET_PARAM, &par);
	lcd_ioctl(lcd, LCDL_CLEAR_DISP);
	lcdl_rows = par.vs_rows;
	lcdl_cols = par.vs_cols;

	return ("LCD-Linux Console");
}


static void lcdcon_init(struct vc_data *conp, int init)
{
	conp->vc_display_fg = &lcd_linux_display_fg;
	conp->vc_can_do_color = 0;

	if (init) {
		conp->vc_rows = lcdl_rows;
		conp->vc_cols = lcdl_cols;
	} else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		vc_resize(conp, conp->vc_cols, conp->vc_rows);
#else
		vc_resize(conp->vc_rows, conp->vc_cols, lcdl_first_vc, lcdl_last_vc);
#endif

	if (lcd_linux_display_fg == NULL)
		lcd_linux_display_fg = conp;
}

static void lcdcon_deinit(struct vc_data *conp)
{
	if (lcd_linux_display_fg == conp)
		lcd_linux_display_fg = NULL;

	lcd_close(&lcd);
}

/* This function clears a rectangular area sized height x width starting
 * (upper left corner of the rectangle downwards) at position s_row s_col
 */
static void lcdcon_clear(struct vc_data *conp, int s_row, int s_col, int height, int width)
{
	char buf[15];
	loff_t offset = (s_row*conp->vc_cols)+s_col;

	if (width <= 0 || height <= 0)
		return;

	if (s_col == 0 && width == conp->vc_cols)
		width = sprintf(buf, "\033[2K");
	else
		width = sprintf(buf, "\033[%dX", width);

	while (height--) {
		lcd_write(lcd, buf, width, offset, 0);
		offset += conp->vc_cols;
	}
}

static void lcdcon_putc(struct vc_data *conp, int data, int row, int col)
{
	loff_t offset = (row*conp->vc_cols)+col;

	lcd_write(lcd, &data, 1, offset, 1);
}

static void lcdcon_putcs(struct vc_data *conp, const unsigned short *buf, int len, int row, int col)
{
	loff_t offset = (row*conp->vc_cols)+col;

	lcd_write(lcd, buf, len, offset, 1);
}

static void lcdcon_cursor(struct vc_data *conp, int mode)
{
}

static int lcdcon_scroll(struct vc_data *conp, int top, int bot, int dir, int lines)
{
	loff_t offset = (conp->vc_y*conp->vc_cols)+conp->vc_x;
	char buf[3];

	buf[0] = 0x1b;
	switch (dir) {
	case SM_UP:
		buf[1] = 'A';
		break;
	case SM_DOWN:
		buf[1] = 'B';
		break;
	}
	buf[2] = lines;
	lcd_write(lcd, buf, 3, offset, 0);

	return (0);
}

/* This function takes a rectangular area sized height x width starting
 * (upper left corner of the rectangle downwards) at position s_row s_col and
 * moves it at position starting at d_row d_col
 */
static void lcdcon_bmove(struct vc_data *conp, int s_row, int s_col, int d_row, int d_col, int height, int width)
{
	int buf[3];

	if (width <= 0 || height <= 0)
		return;

	buf[0] = (d_row*conp->vc_cols)+d_col;
	buf[1] = (s_row*conp->vc_cols)+s_col;
	if (s_col == 0 && d_col == 0 && width == conp->vc_cols) {
		buf[2] = height*width;
		lcd_ioctl(lcd, LCDL_MEMMOVE, buf);
	} else {
		buf[2] = width;
		while (height--) {
			lcd_ioctl(lcd, LCDL_MEMMOVE, buf);
			buf[0] += conp->vc_cols;
			buf[1] += conp->vc_cols;
		}
	}
}

static int lcdcon_switch(struct vc_data *conp)
{
	return (1);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static int lcdcon_blank(struct vc_data *conp, int blank, int mode_switch)
#else
static int lcdcon_blank(struct vc_data *conp, int blank)
#endif
{
	if (blank)
		lcd_ioctl(lcd, LCDL_CLEAR_DISP);

	return (1);
}

static int lcdcon_set_palette(struct vc_data *conp, unsigned char *table)
{
	return (-EINVAL);
}

static int lcdcon_scrolldelta(struct vc_data *conp, int lines)
{
	return (0);
}

static int lcdcon_set_origin(struct vc_data *conp)
{
	return (0);
}

static struct consw lcd_linux_con = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	.owner			= THIS_MODULE,
#endif
	.con_startup		= lcdcon_startup,	/* Mandatory */
	.con_init		= lcdcon_init,		/* Mandatory */
	.con_deinit		= lcdcon_deinit,	/* Mandatory */
	.con_clear		= lcdcon_clear,		/* Mandatory */
	.con_putc		= lcdcon_putc,		/* Mandatory */
	.con_putcs		= lcdcon_putcs,		/* Mandatory */
	.con_cursor		= lcdcon_cursor,	/* Mandatory */
	.con_scroll		= lcdcon_scroll,	/* Mandatory */
	.con_bmove		= lcdcon_bmove,		/* Mandatory */
	.con_switch		= lcdcon_switch,	/* Mandatory */
	.con_blank		= lcdcon_blank,		/* Mandatory */
//	.con_font_set		= lcdcon_font_set,	/* Optional */
//	.con_font_get		= lcdcon_font_get,	/* Optional */
//	.con_font_default	= lcdcon_font_default,	/* Optional */
//	.con_font_copy		= lcdcon_font_copy,	/* Optional */
//	.con_resize		= lcdcon_resize,	/* Optional */
	.con_set_palette	= lcdcon_set_palette,	/* Mandatory */
	.con_scrolldelta	= lcdcon_scrolldelta,	/* Mandatory */
	.con_set_origin		= lcdcon_set_origin,	/* Optional */
//	.con_save_screen	= lcdcon_save_screen,	/* Optional */
//	.con_build_attr		= lcdcon_build_attr,	/* Optional */
//	.con_invert_region	= lcdcon_invert_region,	/* Optional */
//	.con_screen_pos		= lcdcon_screen_pos,	/* Optional */
//	.con_getxy		= lcdcon_getxy,		/* Optional */
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
static int __init lcdcon_init_module(void)
#else  /* Linux_2.0_support */
#ifdef MODULE  /* Linux_2.0_support */
int init_module(void)  /* Linux_2.0_support */
#else  /* Linux_2.0_support */
int lcdcon_init(void)  /* Linux_2.0_support */
#endif  /* Linux_2.0_support */
#endif  /* Linux_2.0_support */
{
	int ret = 0;

	if (lcdl_first_vc > lcdl_last_vc)
		return (1);

#ifdef MODULE
	if ((ret = request_module("hd44780"))) {
		if (ret != -ENOSYS) {
			printk(KERN_ERR "lcdcon: failure while loading module hd44780\n");
			return (ret);
		}
		printk(KERN_ERR "lcdcon: your kernel does not have kmod or kerneld support;\n");
		printk(KERN_ERR "lcdcon: remember to load both the lcd-linux and hd44780 modules\n");
		printk(KERN_ERR "lcdcon: before loading the lcdcon module\n");
	}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	ret = take_over_console(&lcd_linux_con, lcdl_first_vc, lcdl_last_vc, 0);
#else
	take_over_console(&lcd_linux_con, lcdl_first_vc, lcdl_last_vc, 0);
#endif

	return (ret);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void __exit lcdcon_exit_module(void)
#else
/* __exit is not defined in 2.2.x kernels */
static void lcdcon_exit_module(void)
#endif
#else  /* Linux_2.0_support */
#ifdef MODULE  /* Linux_2.0_support */
void cleanup_module(void)  /* Linux_2.0_support */
#else  /* Linux_2.0_support */
void lcdcon_exit(void)  /* Linux_2.0_support */
#endif  /* Linux_2.0_support */
#endif  /* Linux_2.0_support */
{
	give_up_console(&lcd_linux_con);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 2, 0)  /* Linux_2.0_support */
module_init(lcdcon_init_module)
module_exit(lcdcon_exit_module)
#endif  /* Linux_2.0_support */
