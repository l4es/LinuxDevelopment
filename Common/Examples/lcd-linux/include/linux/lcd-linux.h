/* lcd-linux.h
 *
 * $Id: lcd-linux.h,v 1.68 2010/07/04 16:02:02 mjona Exp $
 *
 * Software layer to drive LCD displays under Linux.
 *
 * External interface header file.
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

#ifndef LCD_LINUX_H
#define LCD_LINUX_H

#ifndef LCD_LINUX_MAIN
#warning
#warning LCD-Linux is still in development stage and
#warning aims at speed and optimization. For these
#warning reasons there is no guarantee of backward
#warning compatibility between different LCD-Linux
#warning versions. Be sure to use the lcd-linux.h
#warning file of the same version as the module.
#warning "http://lcd-linux.sourceforge.net/"
#warning
#endif

#define LCD_LINUX_VERSION	"0.13.9"	/* Version number */
#define LCD_LINUX_STRING	"lcd"

#define LCD_MAJOR		120	/* Major number for this device
					 * Set this to 0 for dynamic allocation
					 */
#define LCD_MINORS		8	/* Minors allocated for LCD-Linux*/

#include <linux/types.h>

#define	str(s) #s
#define	string(s) str(s)

struct lcd_parameters {
	const char	*name;		/* Driver's name */
	unsigned long	flags;		/* Flags (see documentation) */
	unsigned short	minor;		/* Minor number of the char device */
	unsigned short	tabstop;	/* Tab character length */
	unsigned short	num_cntr;	/* Controllers to drive */
	unsigned short	cntr_rows;	/* Rows per controller */
	unsigned short	cntr_cols;	/* Display columns */
	unsigned short	vs_rows;	/* Virtual screen rows */
	unsigned short	vs_cols;	/* Virtual screen columns */
	unsigned short	cgram_chars;	/* Number of user definable characters */
	unsigned short	cgram_bytes;	/* Number of bytes required to define a
					 * user definable character */
	unsigned char	cgram_char0;	/* Ascii of first user definable character */
};

/* IOCTLs */
#include <asm/ioctl.h>
#define LCDL_SET_PARAM		_IOW(LCD_MAJOR, 0x80, struct lcd_parameters *)
#define LCDL_GET_PARAM		_IOR(LCD_MAJOR, 0x81, struct lcd_parameters *)
#define LCDL_CHARSUBST		_IOW(LCD_MAJOR, 0x82, unsigned char *)
#define LCDL_RAW_MODE		_IOW(LCD_MAJOR, 0x83, unsigned int)
#define LCDL_RESET_CHARMAP	_IO(LCD_MAJOR, 0x84)
#define LCDL_SAVE_CHARMAP	_IO(LCD_MAJOR, 0x85)
#define LCDL_RESTORE_CHARMAP	_IO(LCD_MAJOR, 0x86)
#define LCDL_SWAP_CHARMAP	_IO(LCD_MAJOR, 0x87)
#define LCDL_CLEAR_DISP		_IO(LCD_MAJOR, 0x88)
#define LCDL_SET_CGRAM_CHAR	_IOW(LCD_MAJOR, 0x89, unsigned char *)
#define LCDL_GET_CGRAM_CHAR	_IOR(LCD_MAJOR, 0x8a, unsigned char *)
#define LCDL_SET_CHARMAP	_IOW(LCD_MAJOR, 0x8b, unsigned char *)
#define LCDL_GET_CHARMAP	_IOR(LCD_MAJOR, 0x8c, unsigned char *)
#define LCDL_MEMSET		_IOW(LCD_MAJOR, 0x8d, unsigned int *)
#define LCDL_MEMMOVE		_IOW(LCD_MAJOR, 0x8e, unsigned int *)
#define LCDL_IOATTR		_IOW(LCD_MAJOR, 0x8f, unsigned int)



#ifdef __KERNEL__ /* The rest is for kernel only */

#include <linux/kernel.h>
#include <linux/module.h>


struct lcd_driver {
	void	(*read_char)(unsigned int offset, unsigned short *data);
	void	(*read_cgram_char)(unsigned char index, unsigned char *pixmap);
	void	(*write_char)(unsigned int offset, unsigned short data);
	void	(*write_cgram_char)(unsigned char index, unsigned char *pixmap);
	void	(*clear_display)(void);
	void	(*address_mode)(int mode);
	int	(*validate_driver)(void);
	int	(*init_display)(void);
	int	(*cleanup_display)(void);
	int	(*init_port)(void);
	int	(*cleanup_port)(void);
	int	(*handle_custom_char)(unsigned int data);
	int	(*handle_custom_ioctl)(unsigned int cmd, unsigned long arg, unsigned int arg_in_userspace);

	/* The character map to be used */
	unsigned char *charmap;

	/* Buffer for CGRAM operations.
	 * Will be filled by the lcd-linux layer.
	 */
	unsigned char *cgram_buffer;

	/* The root where the driver can create its own proc files.
	 * Will be filled by the lcd-linux layer.
	 */
	struct proc_dir_entry *driver_proc_root;

	/* Set this field to 'driver_module_init' or call lcd_driver_setup
	 * just before registering the driver with lcd_register_driver.
	 */
	struct module *driver_module;
};

#ifdef MODULE
#define driver_module_init	THIS_MODULE
#else
#define driver_module_init	NULL
#endif

/* Always call lcd_driver_setup just before registering the driver
 * with lcd_register_driver.
 */
static inline void lcd_driver_setup(struct lcd_driver *p)
{
	p->driver_module = driver_module_init;
}

/* External interface */
struct lcd_struct;
int lcd_register_driver(struct lcd_driver *drv, struct lcd_parameters *par);
int lcd_unregister_driver(struct lcd_driver *drv, struct lcd_parameters *par);
int lcd_open(unsigned short minor, struct lcd_struct **lcd);
int lcd_close(struct lcd_struct **lcd);
int lcd_ioctl(struct lcd_struct *lcd, unsigned int cmd, ...);
ssize_t lcd_write(struct lcd_struct *lcd, const void *buffer, size_t length, loff_t offset, unsigned int with_attr);
ssize_t lcd_read(struct lcd_struct *lcd, void *buffer, size_t length, loff_t offset, unsigned int with_attr);

#endif /* __KERNEL__ */

#endif /* External interface included */
