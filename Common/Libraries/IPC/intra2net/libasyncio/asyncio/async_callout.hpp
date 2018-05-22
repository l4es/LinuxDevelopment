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
 * @brief provides a method for delayed execution of functions.
 *
 * @copyright &copy; Copyright 2008-2009 by Intra2net AG
 */
#ifndef ___ASYNC_CALLOUT_HPP__
#define ___ASYNC_CALLOUT_HPP__

#include "async_io.hpp"

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>


namespace AsyncIo
{

// forward declarations:
namespace Detail {

class Caller;

typedef boost::shared_ptr< Caller > CallerPtr;
typedef boost::weak_ptr< Caller > CallerWeakPtr;

} // eo namespace Detail


/**
 * @brief represents an id for a deferred call.
 *
 * Also provides methods for modifying the call
 * (like thaw or delete it).
 */
class CallOutId
{
      friend class Detail::Caller;

   public:
      CallOutId();

      unsigned long getValue() const {return m_value;}

      bool thaw() const;
      bool remove();

      bool active() const;
      bool frozen() const;

      MilliTime remaining_time();

   private:

      CallOutId(unsigned long value);

   private:

      unsigned long m_value;

      Detail::CallerWeakPtr m_caller_weak_ptr;

}; // eo class CallOutId



/*
**
*/

namespace Detail {

/**
 * @brief tool class for holding and executing a deferred call.
 *
 */
class Caller
: public TimerBase
, public boost::enable_shared_from_this< Caller >
{
   public:
      Caller( boost::function< void() > f, long delta_sec, long delta_msec=0, bool frozen=false );
      virtual ~Caller();

      CallOutId getCallOutId() const { return m_call_out_id; }

      bool thaw();

      bool joinId();

      bool frozen() const;

   protected:

      virtual void execute();

   private:

      CallOutId m_call_out_id;
      boost::function< void() > m_func;
      bool m_waiting;
}; // eo class Caller


} // eo namespace Detail

/*
**
*/

/**
 * @brief initiates a deferred call of a function.
 *
 * @param f the function which should be called.
 * @param delta_sec the delta time (in seconds) when the function should be called.
 * @return an id to identify the call (may be used for preliminary removal of the call)
 *
 * A deferred call is done by passing a function object and a delay when it should
 * be called.
 * The backend loop will call the function after the delay is passed and the backend loop is active.
 *
 * Thus the call might not not be exactly after the desired delay, but at least this time has
 * to pass until the function is called.
 *
 * The result id can be used to remove the call if it becomes obsolete.
 * This is useful when You call timeout handlers and can remove them if an event happens and
 * obsoletes the timeout call.
 *
 * @see CallOutId
 */
template< typename F >
CallOutId callOut( boost::function< void() > f, F delta_sec );

template<> CallOutId callOut( boost::function< void() > f, long delta_sec );
template<> CallOutId callOut( boost::function< void() > f, double delta_sec );
template<> CallOutId callOut( boost::function< void() > f, float delta_sec );
template<> CallOutId callOut( boost::function< void() > f, int delta_sec );


/**
 * @brief initiates a frozen call of a function.
 *
 * @param f the function which should be called.
 * @param delta_sec the delta time (in seconds) when the call will be (silently) removed.
 * @return an id to identify the call; necessary for thaw the call.
 *
 * A frozen call is a function call which will be prepared for a given period of time
 * but not executed. Within that time period that call might be activated ("thawed")
 * which means the call will be executed the next time when the backend loop is running.
 * If the prepared call is not activated, then it vanishes.
 *
 * The returned call id provides a method to thaw a frozen call.
 *
 * @see CallOutId
 */
template< typename F >
CallOutId frozenCall( boost::function< void() > f, F delta_sec);

template<> CallOutId frozenCall( boost::function< void() > f, long delta_sec );
template<> CallOutId frozenCall( boost::function< void() > f, double delta_sec );
template<> CallOutId frozenCall( boost::function< void() > f, float delta_sec );
template<> CallOutId frozenCall( boost::function< void() > f, int delta_sec );



bool removeCallOut( CallOutId& id );


} // eo namespace AsyncIo

#endif
