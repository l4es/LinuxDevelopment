/* default.h
 *
 * $Id: default.h,v 1.1.1.1 2005/08/23 13:30:14 mjona Exp $
 *
 * Default user defined characters for lcdmod.
 *
 * Copyright (C) by Michael McLellan (mikey@cs.auckland.ac.nz)
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

static void init_charmap(void)
{
}

static unsigned char cg0[] = { 0x1f, 0x1f, 0x11, 0x0f, 0x11, 0x1e, 0x01, 0x1f };
static unsigned char cg1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f };
static unsigned char cg2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f };
static unsigned char cg3[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f };
static unsigned char cg4[] = { 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f };
static unsigned char cg5[] = { 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
static unsigned char cg6[] = { 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
static unsigned char cg7[] = { 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
