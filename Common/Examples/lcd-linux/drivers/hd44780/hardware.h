/* hardware.h
 *
 * $Id: hardware.h,v 1.8 2009/04/04 12:44:24 mjona Exp $
 *
 * Parallel Port Wiring scheme definition.
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

/* Data Pins: address = IO */
#define PIN_2		0x01
#define PIN_3		0x02
#define PIN_4		0x04
#define PIN_5		0x08
#define PIN_6		0x10
#define PIN_7		0x20
#define PIN_8		0x40
#define PIN_9		0x80

/* Control Pins: address = IO+2 */
#define PIN_1		0x01
#define PIN_14		0x02
#define PIN_16		0x04
#define PIN_17		0x08

/* Wiring
 *
 * parallel 16		RS (LCD 4)
 * parallel 14		RW (LCD 5)
 * parallel 1		EN (LCD 6)
 */
/***** Control register pins *****/
#define RS		PIN_16		/* RS line */
#define RW		PIN_14		/* RW line (set to 0x00 if not used, and pull down the RW line on the display) */
#define BL		0x00		/* Backlight control (set to 0x00 if absent) */

#define E0		PIN_1		/* Enable line for controller 0 */
#define E1		PIN_17		/* Enable line for controller 1 (set to 0x00 if absent) */
#define E2		0x00		/* Enable line for controller 2 (set to 0x00 if absent) */

/***** Data register pins *****/
#define E3		0x00		/* Enable line for controller 3 (set to 0x00 if absent) */
#define E4		0x00		/* Enable line for controller 4 (set to 0x00 if absent) */
#define E5		0x00		/* Enable line for controller 5 (set to 0x00 if absent) */
#define E6		0x00		/* Enable line for controller 6 (set to 0x00 if absent) */
