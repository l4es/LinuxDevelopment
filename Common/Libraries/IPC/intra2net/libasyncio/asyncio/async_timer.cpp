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
/** @file
 *
 * (c) Copyright 2007 by Intra2net AG
 */

//#define NOISEDEBUG


#include "async_timer.hpp"


#ifdef NOISEDEBUG
#include <iostream>
#include <iomanip>
#define DOUT(msg) std::cout << msg << std::endl
#define FODOUT(obj,msg) std::cout << typeid(*obj).name() << "[" << obj << "]:" << msg << std::endl
#define ODOUT(msg) std::cout << typeid(*this).name() << "[" << this << "]:" << msg << std::endl
#else
#define DOUT(msg) do {} while (0)
#define FODOUT(obj,msg) do {} while (0)
#define ODOUT(msg) do {} while (0)
#endif


namespace AsyncIo
{

using namespace Utils;

/*
 * Implementation of SimpleTimer
 */

SimpleTimer::SimpleTimer()
{
} // eo SimpleTimer::SimpleTimer()


SimpleTimer::SimpleTimer(const MilliTime& delta, const TimerSignal::slot_function_type& action)
{
   addAction(action);
   startTimer(delta);
} // eo SimpleTimer::SimpleTimer(const MilliTime&, const TimerSignal::slot_type&)


SimpleTimer::SimpleTimer(long milli_seonds_delta, const TimerSignal::slot_function_type& action)
{
   addAction(action);
   startTimerMS(milli_seonds_delta);
} // eo SimpleTimer::SimpleTimer(long, const TimerSignal::slot_type&)


void SimpleTimer::execute()
{
   ODOUT("execute()!");
   m_timer_signal();
   ODOUT("  signal with this");
   m_timer_signal_p(this);
   ODOUT("execute done.");
} // eo SimpleTimer::execute


void SimpleTimer::addAction( const TimerSignal::slot_function_type& action )
{
   m_timer_signal.connect(action);
} // eo SimpleTimer::addAction(const TimerSignal::slot_type&)


void SimpleTimer::addActionP( const TimerSignalP::slot_function_type& action )
{
   m_timer_signal_p.connect(action);
} // eo SimpleTimer::addAction(const TimerSignalP::slot_type&)


void SimpleTimer::startTimer( const MilliTime& delta )
{
   m_delta= delta;
   setDeltaWhenTime( delta );
   activate(true);
#ifdef NOISEDEBUG
   MilliTime now, t;
   get_current_monotonic_time(now);
   t= getWhenTime();
   MilliTime dt= t-now;
   ODOUT("startTimer");
   ODOUT("  now: sec="<< now.mt_sec << ", msec="<<now.mt_msec);
   ODOUT("  t:   sec="<< t.mt_sec << ", msec="<<t.mt_msec);
   ODOUT("  dt:  sec="<< dt.mt_sec << ", msec="<<dt.mt_msec);
#endif
} // eo SimpleTimer::startTimer(const MilliTime&)


void SimpleTimer::startTimerMS( long milli_seconds )
{
   startTimer( MilliTime(0,milli_seconds) );
} // eo SimpleTimer::stratTimerMS(long)


void SimpleTimer::stopTimer()
{
   deactivate();
} // eo SimpleTimer::stopTimer


} // eo namespace AsyncIo
