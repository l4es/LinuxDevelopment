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

#include "asyncio_time_tools.hpp"

#include <sys/time.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <sys/syscall.h>


// define missing POSIX.1b constants...

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

namespace AsyncIo
{
namespace Utils
{


namespace
{


/**
 * @brief fetches the value from the monotonic clock source.
 * @param[out] seconds the seconds.
 * @param[out] nano_seconds the nano seconds.
 * @return @a true if the clock was successfully read.
 */
bool monotonic_clock_gettime(long int& seconds, long int& nano_seconds)
{
    struct timespec tp[1];
    int res= ::syscall(__NR_clock_gettime, CLOCK_MONOTONIC, tp);
    if (0 == res)
    {
        seconds= tp->tv_sec;
        nano_seconds= tp->tv_nsec;
    }
    return (res==0);
} // eo monotonic_clock_gettime(long int&,long int&)




void get_current_real_time(long& current_sec, long& current_msec)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    current_sec= tv.tv_sec;
    current_msec= (tv.tv_usec / 1000);
    if (current_msec >= 1000)
    {
        current_sec += (current_msec / 1000);
        current_msec%= 1000;
    }
} // eo get_current_real_time


void get_current_monotonic_time(long& current_sec, long& current_msec)
{
    long nsec;
    if (monotonic_clock_gettime(current_sec,nsec))
    {
        current_msec= nsec / 1000000L;
    }
    else
    {
        //fallback...
        get_current_real_time(current_sec,current_msec);
    }
} // eo get_current_monotonic_time



} // eo anonymous namespace

/**************************************************************************\
\**************************************************************************/


/*
 * implementation of MilliTime
 */

MilliTime::MilliTime(long sec, long msec)
: mt_sec(sec), mt_msec(msec)
{
    normalize();
} // eo MilliTime::MilliTime


void MilliTime::set(long sec, long msec)
{
    mt_sec= sec;
    mt_msec= msec;
    normalize();
} // eo MilliTime::set


/**
 * normalizes the values, so that mt_msec has a value between 0 and 999.
 */
void MilliTime::normalize()
{
    if (mt_msec < 0)
    {
        mt_sec += (mt_msec / 1000) - 1;
        mt_msec = (mt_msec % 1000) + 1000;
    }
    else if (mt_msec>=1000)
    {
        mt_sec+= (mt_msec / 1000);
        mt_msec %= 1000;
    }
} // eo MilliTime::normalize


/**
 * determine if the represented point in time is before another one.
 * @param other the other point in time.
 * @return true if the own point in time is before the other one.
 */
bool MilliTime::operator < (MilliTime& other)
{
    normalize();
    other.normalize();
    return
        (mt_sec < other.mt_sec)
        || (( mt_sec == other.mt_sec) && (mt_msec < other.mt_msec));
} // eo MilliTime::operator <


/**
 * determine if two point in times are equal.
 * @param other the point in time to compare with.
 * @return true if the represented times are equal.
 */
bool MilliTime::operator == (MilliTime& other)
{
    normalize();
    other.normalize();
    return (( mt_sec == other.mt_sec) && (mt_msec == other.mt_msec));
} // eo MilliTime::operator <

/**
 * @brief subtracts a time delta from the object.
 * @param lhs the time delta to subtract.
 * @return reference to the object itself.
 */
MilliTime& MilliTime::operator -= (const MilliTime& lhs)
{
    mt_sec  -= lhs.mt_sec;
    mt_msec -= lhs.mt_msec;
    return *this;
} // eo operator -=


/**
 * @brief adds a time delta from the object.
 * @param lhs the time delta to add.
 * @return reference to the object itself.
 */
MilliTime& MilliTime::operator += (const MilliTime& lhs)
{
    mt_sec  += lhs.mt_sec;
    mt_msec += lhs.mt_msec;
    return *this;
}  // eo operator +=



/**
 * @brief gets the current time as MilliTime structure.
 * @param[out] mt reference to the MilliTime structure which is filled with the result.
 */
void get_current_real_time(MilliTime& mt)
{
    long sec, msec;
    get_current_real_time(sec,msec);
    mt.set(sec,msec);
} // eo get_current_real_time


/**
 * @brief gets the current time as MilliTime structure.
 * @param[out] mt reference to the MilliTime structure which is filled with the result.
 */
void get_current_monotonic_time(MilliTime& mt)
{
    long sec, msec;
    get_current_monotonic_time(sec,msec);
    mt.set(sec,msec);
} // eo get_current_monotonic_time





} // eo namespace Utils
} // eo namespace AsyncIo
