/* skeleton.c
 *
 * $Id: skeleton.c,v 1.29 2009/03/09 17:59:23 mjona Exp $
 *
 * LCD driver for HD44780 compatible displays connected to the parallel port.
 *
 * Module template.
 *
 * Copyright (C) 2004 - 2009  Mattia Jona-Lasinio (mjona@users.sourceforge.net)
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

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/init.h>

#define LCD_LINUX_MAIN
#include <linux/lcd-linux.h>

/* Allocate the character translation table */
//static unsigned char charmap[256];

static void example_read_char(unsigned int offset, unsigned short *data)
{
	/* Do what you need to read a character from the display
	 * at position 'offset'.
	 * Return the read character in '*data'. '*data' can contain
	 * the character attributes in the most significant byte.
	 */
}

static void example_read_cgram_char(unsigned char index, unsigned char *pixels)
{
	/* Do what you need to read a cgram char from the display
	 * with index 'index' and store the bitmap in 'pixels'.
	 */
}

static void example_write_char(unsigned int offset, unsigned short data)
{
	/* Do what you need to write the character 'data' to the display
	 * at position 'offset'. 'data' contains the character attributes
	 * in the most significant byte.
	 */
}

static void example_write_cgram_char(unsigned char index, unsigned char *pixels)
{
	/* Do what you need to create a cgram char in the display,
	 * at index 'index' with the bitmap stored in 'pixels'.
	 */
}

static int example_init_display(void)
{
	/* Do what you need to initialize the display.
	 * Return < 0 on error.
	 */
	return (0);
}

static int example_cleanup_display(void)
{
	/* Do what you need to cleanup the display.
	 * Return < 0 on error.
	 */
	return (0);
}

static void example_clear_display(void)
{
	/* Do what you need to clear the display content.
	 */
}

static void example_address_mode(int mode)
{
	/* Do what you need to set the display increment/decrement
	 * address mode after a data read/write.
	 *	mode > 0 : increment
	 *	mode < 0 : decrement
	 */
}

static int example_validate_driver(void)
{
	/* Check for consistency of the driver parameters
	 * Return 0 if everything is consistent and the display is monochrome.
	 * Return > 0 if everything is consistent and the display is color capable.
	 * Return < 0 on error.
	 */
	return (0);
}

static int example_init_port(void)
{
	/* Do what you need to open and init the communication port
	 * between computer and display.
	 * Return 0 if the port is correctly opened.
	 * Return < 0 on error.
	 */
	return (0);
}

static int example_cleanup_port(void)
{
	/* Do what you need to close the communication port
	 * between computer and display
	 * Return 0 if the port is correctly closed.
	 * Return < 0 on error.
	 */
	return (0);
}

static int example_handle_custom_char(unsigned int input)
{
	/* Handle special escape sequences not handled by the lcd-linux layer
	 * The escape sequence is of the form ESC 'input';
	 * The character is coded into the lower byte of 'input'.
	 * Use (input & 0xff) to obtain it.
	 * If input > 0xff (use input & (~0xff) to test it), the character
	 * (input & 0xff) should be interpreted as an argument of the escape
	 * sequence.
	 * Return the number of arguments the escape sequence needs or < 0
	 * if the sequence cannot be handled.
	 */
	return (0);
}

static int example_handle_custom_ioctl(unsigned int ioctl_cmd, unsigned long ioctl_arg, unsigned int user_space)
{
	/* Handle driver-specific ioctls not handled by the lcd-linux layer.
	 * If 'user_space' is nonzero then 'ioctl_arg' is a pointer in user-space.
	 * Return the appropriate errno on failure.
	 * Return -ENOIOCTLCMD if the ioctl cannot be handled.
	 */
	return (0);
}

static struct lcd_parameters par = {
	.name		= "example_driver",
	.minor		= 3,
	.num_cntr	= 1,
	.cntr_rows	= 10,
	.cntr_cols	= 40,
	.flags		= 0,
	.tabstop	= 4,
	.cgram_chars	= 8,
	.cgram_bytes	= 8,
	.cgram_char0	= 0,
};

static struct lcd_driver example = {
	.read_char		= example_read_char,
	.read_cgram_char	= example_read_cgram_char,
	.write_char		= example_write_char,
	.write_cgram_char	= example_write_cgram_char,
	.init_display		= example_init_display,
	.cleanup_display	= example_cleanup_display,
	.clear_display		= example_clear_display,
	.address_mode		= example_address_mode,
	.validate_driver	= example_validate_driver,
	.init_port		= example_init_port,
	.cleanup_port		= example_cleanup_port,
	.handle_custom_char	= example_handle_custom_char,
	.handle_custom_ioctl	= example_handle_custom_ioctl,

//	.charmap		= charmap,
};

/* Initialization */
static int __init example_init_module(void)
{
	lcd_driver_setup(&example);

	/* Register the display driver */
	return (lcd_register_driver(&example, &par));
}

/* Cleanup */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
static void __exit example_cleanup_module(void)
#else
/* __exit is not defined in 2.2.x kernels */
static void example_cleanup_module(void)
#endif
{
	/* Unregister the display driver */
	lcd_unregister_driver(&example, &par);
}

module_init(example_init_module)
module_exit(example_cleanup_module)
