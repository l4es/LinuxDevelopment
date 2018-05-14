/* config.h
 *
 * $Id: config.h,v 1.16 2009/09/27 21:27:03 mjona Exp $
 *
 * Configure file for LCD-Linux. Here you must specify your hardware setup and
 * timings constants. The default values will probably be right for you.
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
 *
 */

/* Setup the default user defined characters in CGRAM */
#include "cgram/default.h"
/* #include "cgram/swedish.h" */
/* #include "cgram/cgram.h" */

/* Don't modify the default timing constants
 * unless you know what you are doing.
 */

/* In case of a 4 bit bus, indicate your
 * wiring configuration for the data bits
 *
 * 0 -> D3, D2, D1, D0
 * 1 -> D4, D3, D2, D1
 * 2 -> D5, D4, D3, D2
 * 3 -> D6, D5, D4, D3
 * 4 -> D7, D6, D5, D4
 */
#define SETUP			4

#define HIGH_NIBBLE_WRITE(x)	(((x) >> (4-SETUP)) & (0x0f << SETUP))
#define LOW_NIBBLE_WRITE(x)	(((x) << SETUP) & (0x0f << SETUP))
#define HIGH_NIBBLE_READ(x)	(((x) & (0x0f << SETUP)) << (4-SETUP))
#define LOW_NIBBLE_READ(x)	(((x) & (0x0f << SETUP)) >> SETUP)

/* Execution times (in microseconds) */
#define T_READ			60	/* Read execution time (min 43 us) */
#define T_WRITE			60	/* Write execution time (min 43 us) */
#define T_BF			4	/* Busy flag polling time (min 1 us) */

/* Timings in nanoseconds */
#define T_AS			200	/* Address set-up time (min 140 ns) */
#define T_EH			500	/* Enable high time (min 450 ns) */
#define T_EL			800	/* Enable low time (min 500 ns) */

/* Various constants */
#define DFLT_BASE		0x378	/* Default I/O base address of the parallel port */
#define DFLT_NUM_CNTR		1	/* Default number of controllers the display has */
#define DFLT_CNTR_ROWS		2	/* Default number of rows per controller */
#define DFLT_CNTR_COLS		40	/* Default number of columns the display has */
#define DFLT_VS_ROWS		25	/* Default number of rows for the virtual screen */
#define DFLT_VS_COLS		80	/* Default number of columns for the virtual screen */
#define DFLT_TABSTOP		4	/* Default length of tabs */
#define DFLT_FLAGS		HD44780_CHECK_BF /* Default flags: check BF and 8 bit bus */

#define MAX_NUM_CNTR		7	/* We support up to 7 controllers */
#define MAX_CNTR_ROWS		4	/* The HD44780 supports up to 4 lines as a fake 2 lines mode */
#define MAX_CNTR_COLS		80	/* The HD44780 supports up to 80 characters (1*80; 2*40; etc) */
