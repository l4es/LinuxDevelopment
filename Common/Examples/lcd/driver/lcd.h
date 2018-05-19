#ifndef LCD_H
#define LCD_H

#include <linux/ioctl.h>

#define LCD_IOC_MAGIC	'N'

/* 
 *  IOC_WAIT_KEY		Wait for a key event
 *  IOC_CMD				Send a comand 
 */

#define IOC_WAIT_KEY		_IO(LCD_IOC_MAGIC,0)
#define IOC_CMD				_IO(LCD_IOC_MAGIC,1)

#endif
