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
 * provides timer objects based on the TimerBase class 
 *
 * (c) Copyright 2007 by Intra2net AG
 */

#ifndef __ASYNC_TIMER_HPP__
#define __ASYNC_TIMER_HPP__

#include "async_io.hpp"

#include <boost/signal.hpp>

namespace AsyncIo
{


class SimpleTimer : public TimerBase
{
   public:

      typedef boost::signal<void()>       TimerSignal;
      typedef boost::signal<void(SimpleTimer*)> TimerSignalP;

   public:

      SimpleTimer();

      // convenience constructors for simple timeouts:
      SimpleTimer(const MilliTime& delta, const TimerSignal::slot_function_type& action);
      SimpleTimer(long milli_seonds_delta, const TimerSignal::slot_function_type& action);

      // add actions:
      void addAction( const TimerSignal::slot_function_type& action );
      void addActionP( const TimerSignalP::slot_function_type& action );

      // start the timer:
      void startTimer( const MilliTime& delta );
      void startTimerMS( long milli_seconds );

      // stop the timer:
      void stopTimer();

   protected:
      MilliTime    m_delta;

      TimerSignal  m_timer_signal;
      TimerSignalP m_timer_signal_p;

      virtual void execute();
}; // eo class SimpleTimer


} // eo namespace AsyncIo

#endif
