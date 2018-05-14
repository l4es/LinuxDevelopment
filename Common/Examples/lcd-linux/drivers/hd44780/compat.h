/* compat.h
 *
 * $Id: compat.h,v 1.6 2009/03/09 17:59:22 mjona Exp $
 *
 * Missing functions for old kernel compatibility.
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 2, 0)
/* ndelay is missing in 2.0.x kernels */
static __inline__ void ndelay(unsigned long nsecs)
{
	(nsecs < 1000) ? udelay(1) : udelay(nsecs/1000);
}
/* mdelay is missing in 2.0.x kernels */
static __inline__ void mdelay(unsigned long msecs)
{
	msecs <<= 1;
	while (msecs--)
		udelay(500);
}
#elif ! defined(ndelay)
/* ndelay is missing in 2.2.x until 2.4.7 (?) kernels */
static __inline__ void ndelay(unsigned long nsecs)
{
	(nsecs < 1000) ? __const_udelay(nsecs*0x00000005) : udelay(nsecs/1000);
}
#endif

#if defined(USE_PARPORT) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 4, 0)
/* parport_find_base is missing in 2.2.x kernels
 * Thanks Nick Metcalfe (krane@alphalink.com.au)
 */
static struct parport *parport_find_base(int io)
{
	struct parport *list = parport_enumerate();

	while (list != NULL && list->base != io)
		list = list->next;

	return (list);
}
#endif
