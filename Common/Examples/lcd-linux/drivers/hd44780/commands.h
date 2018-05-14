/* commands.h
 *
 * $Id: commands.h,v 1.2 2009/03/09 17:59:22 mjona Exp $
 *
 * LCD-Linux:
 * Driver for HD44780 compatible displays connected to the parallel port.
 *
 * HD44780 commands.
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

#ifndef HD44780_COMMANDS_H
#define HD44780_COMMANDS_H

/*** HD44780 Command Set ***/

/* Clear Display*/
#define CLR_DISP	0x01	/* Clear entire display; cursor at row 0, column 0 */

/* Return Home */
#define	RET_HOME	0x02	/* Cursor at row 0, column 0; display content doesn't change */

/* Entry Mode Set */
#define ENTRY_MODE_SET	0x04
#define DISP_SHIFT_ON	(ENTRY_MODE_SET | 0x01)		/* Shift display, not cursor after data write */
#define DISP_SHIFT_OFF	(ENTRY_MODE_SET | 0x00)		/* Shift cursor, not display after data write */
#define CURS_INC	(ENTRY_MODE_SET | 0x02)		/* Shift on the right after data read/write */
#define CURS_DEC	(ENTRY_MODE_SET | 0x00)		/* Shift on the left after data read/write */

/* Display on/off Control */
#define DISP_ONOFF_CNTR	0x08
#define BLINK_ON	(DISP_ONOFF_CNTR | 0x01)	/* Cursor blinking on */
#define BLINK_OFF	(DISP_ONOFF_CNTR | 0x00)	/* Cursor blinking off */
#define CURS_ON		(DISP_ONOFF_CNTR | 0x02)	/* Display Cursor */
#define CURS_OFF	(DISP_ONOFF_CNTR | 0x00)	/* Hide Cursor */
#define DISP_ON		(DISP_ONOFF_CNTR | 0x04)	/* Turn on display updating */
#define DISP_OFF	(DISP_ONOFF_CNTR | 0x00)	/* Freeze display content */

/* Cursor or Display Shift */
#define CURS_DISP_SHIFT	0x10
#define SHIFT_RIGHT	(CURS_DISP_SHIFT | 0x04)	/* Shift on the right */
#define SHIFT_LEFT	(CURS_DISP_SHIFT | 0x00)	/* Shift on the left */
#define SHIFT_DISP	(CURS_DISP_SHIFT | 0x08)	/* Shift display */
#define SHIFT_CURS	(CURS_DISP_SHIFT | 0x00)	/* Shift cursor */

/* Function Set */
#define FUNC_SET	0x20
#define FONT_5X10	(FUNC_SET | 0x04)	/* Select 5x10 dots font */
#define FONT_5X8	(FUNC_SET | 0x00)	/* Select 5x8 dots font */
#define DISP_2_LINES	(FUNC_SET | 0x08)	/* Select 2 lines display (only 5x8 font allowed) */
#define DISP_1_LINE	(FUNC_SET | 0x00)	/* Select 1 line display */
#define BUS_8_BITS	(FUNC_SET | 0x10)	/* Set 8 data bits */
#define BUS_4_BITS	(FUNC_SET | 0x00)	/* Set 4 data bits */

/* Set CGRAM Address */
#define CGRAM_IO	0x40	/* Base CGRAM address */

/* Set DDRAM Address */
#define DDRAM_IO	0x80	/* Base DDRAM address */

#endif /* commands included */
