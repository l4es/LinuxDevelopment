/*
The software in this package is distributed under the GNU General
Public License version 2 (with a special exception described below).

A copy of GNU General Public License (GPL) is included in this distribution,
in the file COPYING.GPL.

As a special exception, if other files instantiate templates or use macros
or inline functions from this file, or you compile this file and link it
with other works to produce a work based on this file, this file
does not by itself cause the resulting work to be covered
by the GNU General Public License.

However the source code for this file must still be made available
in accordance with section (3) of the GNU General Public License.

This exception does not invalidate any other reasons why a work based
on this file might be covered by the GNU General Public License.
*/
/**
 * @file
 * @brief
 *
 * @author Reinhard Pfau \<reinhard.pfau@gmx.de\>
 *
 * @copyright &copy; Copyright 2008-2009 Intra2Net AG
 * @contact Intra2net Opensource team \<opensource@intra2net.com\>
 */

#ifndef __ASYNCIO__TIME_TOOLS_HPP__
#define __ASYNCIO__TIME_TOOLS_HPP__

namespace AsyncIo
{
namespace Utils
{

/**
 * @brief structure for storing (a point in time as) seconds and milliseconds.
 *
 * This structure is used at various places within libasyncio to represent points in
 * time or time durations.
 */
struct MilliTime
{
    long mt_sec;
    long mt_msec;

    MilliTime(long sec=0, long msec=0);

    void set(long sec, long msec=0);

    inline long long get_milliseconds() const
    {
        return ((long long)mt_sec * 1000L + mt_msec);
    } // eo get_milliseconds

    void normalize();

    bool operator < (MilliTime& other);
    bool operator == (MilliTime& other);

    MilliTime& operator -= (const MilliTime& lhs);
    MilliTime& operator += (const MilliTime& lhs);

}; // eo struct MilliTime


inline MilliTime operator + (const MilliTime& rhs, const MilliTime& lhs)
{
    MilliTime t(rhs);
    return t+= lhs;
} // eo operator + (const MilliTime& rhs, const MilliTime lhs)

inline MilliTime operator - (const MilliTime& rhs, const MilliTime& lhs)
{
    MilliTime t(rhs);
    return t-= lhs;
} // eo operator - (const MilliTime& rhs, const MilliTime lhs)


inline bool operator <= (MilliTime& rhs, MilliTime& lhs)
{
    return (rhs<lhs) || (rhs==lhs);
} // eo operator <= (MilliTime& rhs, MilliTime& lhs)



/*
** tool functions:
*/


void get_current_real_time(MilliTime& mt);
void get_current_monotonic_time(MilliTime& mt);


} // eo namespace Utils
} // eo namespace AsyncIo



#endif
