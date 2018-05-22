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
 *
 * @copyright &copy; Copyright 2008-2009 by Intra2net AG
 * @contact opensource@intra2net.com
 *
 */
#include "async_callout.hpp"
#include <asyncio_config.hpp>

#ifdef HAVE_LIBI2NCOMMON
#include <tracefunc.hpp>
#include <logfunc.hpp>
#else
#define SCOPETRACKER(x) do{}while(false)
#endif

#include <map>


namespace AsyncIo
{

// anonymous namespace for our secret details :-)
namespace
{

typedef boost::shared_ptr< Detail::Caller > CallerPtr;

typedef std::map< unsigned long, CallerPtr > CallMap;


unsigned long l_last_id=0;

CallMap l_call_map;


/**
 * @brief creates a new id value for a call out.
 * @return the new id.
 *
 * The id value is basically just a counter.
 * It is implemented in a way that it can not be 0 and can deal with wrap arounds.
 */
unsigned long create_call_out_id_value()
{
   while ( l_call_map.find(++l_last_id) != l_call_map.end() and l_last_id != 0);
   return l_last_id;
} // eo create_call_out_id_value


/**
 * @brief add a caller instance to the local call map.
 * @param caller the caller instance.
 */
void add_call( CallerPtr caller )
{
   if (caller->joinId())
   {
      l_call_map[ caller->getCallOutId().getValue() ] = caller;
   }
} // eo add_call


/**
 * @brief removes an entry from the local call map
 * @param id_value id to remove from the map.
 * @return @a true if an entry was found and removed
 *  @a false if entry was not found in the map.
 */
bool remove_call( unsigned long id_value )
{
   CallMap::iterator it= l_call_map.find(id_value);
   if (it != l_call_map.end())
   {
      l_call_map.erase(it);
      return true;
   }
   return false;
} // eo remove_call(unsigned long)


/**
 * @brief return the caller for an id.
 * @param id_value the id to search the caller for.
 * @return caller; empty pointer if not found.
 */
CallerPtr get_call(unsigned long id_value)
{
   CallMap::iterator it= l_call_map.find(id_value);
   if (it != l_call_map.end())
   {
      return it->second;
   }
   return CallerPtr();
} // eo get_call(unsigned long)


/**
 * @brief tests if an id can be found in the call map.
 * @param id_value the id to search for.
 * @return @a true if the id is found in the call map.
 */
bool has_call( unsigned long id_value )
{
   CallMap::iterator it= l_call_map.find(id_value);
   return (it != l_call_map.end() );
} // eo has_call(unsigned long)



} // eo namespace <anonymous>



/*
** implementation of class CallOutId
*/

CallOutId::CallOutId()
: m_value(0)
{
} // eo CallOutId::CallOutId()


CallOutId::CallOutId(unsigned long value)
: m_value(value)
{
} // eo CallOutId::CallOutId(unsigned long)


/**
 * @brief thaws (activate) the referenced (and frozen) call.
 * @return @a true if the call was successfully thawed; @a false else.
 *
 * A call (referenced the id instance) can be successfully thawed if:
 * - the call was a frozen call
 * - the call is still existing; i.e. we are within the given time period
 *   for that call.
 * .
 */
bool CallOutId::thaw() const
{
   if (m_caller_weak_ptr.expired())
   {
      return false;
   }
   CallerPtr call_ptr= get_call(m_value);
   if (call_ptr)
   {
      return call_ptr->thaw();
   }
   return false;
} // eo CallOutId::thaw() const


/**
 * @brief removes the referenced call.
 * @return @a true if the call was removed; @a false else.
 *
 * @note after calling this method, the refernced call is not existing.
 * Either it was removed or it was already removed earlier (explicit or implicit).
 */
bool CallOutId::remove()
{
   if (m_caller_weak_ptr.expired())
   {
      return false;
   }
   unsigned long value= m_value;
   m_value= 0;
   return remove_call(value);
} // eo CallOutId::remove()


/**
 * @brief returns if the referenced call is still active.
 * @return @a true if the referenced call is active.
 *
 * The referenced call is active if it is still waiting to be executed.
 */
bool CallOutId::active() const
{
   return m_value!=0 and not m_caller_weak_ptr.expired() and has_call(m_value);
} // eo CallOutId::active() const


/**
 * @brief returns if the call is frozen.
 * @return @a true iff the call is frozen.
 */
bool CallOutId::frozen() const
{
    CallerPtr caller= m_caller_weak_ptr.lock();
    if (not caller)
    {
        return false;
    }
    return caller->frozen();
} // eo CallOutId::frozen() const


/**
 * @brief returns the remaining time until the call is done or thrown away (on frozen calls).
 * @return the remaining time.
 *
 * The result only makes sense if the call is still active.
 *
 * If the references call is not active then the returned time is 0.
 */
MilliTime CallOutId::remaining_time()
{
    CallerPtr caller= m_caller_weak_ptr.lock();
    if ( not active() or not caller )
    {
        return MilliTime();
    }
    MilliTime t;
    get_current_monotonic_time(t);
    MilliTime result= caller->getWhenTime();
    result-= t;
    MilliTime t_null;
    return (result < t_null ? t_null : result);
} // eo CallOutId::remaining_time()


namespace Detail
{

/*
** implementation of class Caller
*/

Caller::Caller( boost::function< void() > f, long delta_sec, long delta_msec, bool frozen)
: TimerBase()
, m_call_out_id( create_call_out_id_value() )
, m_func(f)
, m_waiting(frozen)
{
   SCOPETRACKER();
   setDeltaWhenTime( delta_sec, delta_msec);
} // eo Caller::Caller(boost::function< void() >,long)


Caller::~Caller()
{
   SCOPETRACKER();
} // eo Caller::~Caller()


void Caller::execute()
{
   SCOPETRACKER();
   // NOTE: since the func may throw an exception, we first get a shared pointer
   // (to prevent early deletion) and then we remove us from the call map.
   CallerPtr ptr= shared_from_this();
   m_call_out_id.remove();

   if (m_func and not m_waiting)
   {
      m_func(); // may throw..., but at this point it doesn't harm.
      //( it may harm at other places,...)
   }
} // eo Caller::execute()


bool Caller::thaw()
{
   if (m_waiting)
   {
      m_waiting= false;
      setDeltaWhenTime( 0, 0 );
      return true;
   }
   return false;
} // eo Caller::thaw()


bool Caller::joinId()
{
   if (m_call_out_id.m_caller_weak_ptr.expired())
   {
      m_call_out_id.m_caller_weak_ptr= shared_from_this();
      activate();
      return true;
   }
   return false;
} // eo Caller::joinId()


bool Caller::frozen() const
{
    return m_waiting;
} // eo Caller::frozen() const


} // eo namespace Detail



/**
 * @brief remove a pending call by id.
 *
 * @param id the call id which should be removed.
 * @return @a true iff the call was removed, @a false if no call with the given id was found.
 */
bool removeCallOut( CallOutId& id )
{
   return id.remove();
} // eo removeCallOut(CallOutId&)



template<>
CallOutId callOut( boost::function< void() > f, long delta_sec)
{
   CallerPtr caller( new Detail::Caller(f,delta_sec) );
   add_call(caller);
   return caller->getCallOutId();
} // eo callOut(boost::function< void() >,long)

template<>
CallOutId callOut( boost::function< void() > f, int delta_sec)
{
   return callOut<long>(f,delta_sec);
} // eo callOut(boost::function< void() >,int)


template<>
CallOutId callOut( boost::function< void() > f, double delta_sec )
{
   long delta_sec_i = (long)delta_sec;
   long delta_sec_m = (long)((delta_sec - (double)delta_sec_i)*1000.0);
   CallerPtr caller( new Detail::Caller(f,delta_sec_i, delta_sec_m) );
   add_call(caller);
   return caller->getCallOutId();
} // eo callOut(boost::function< void() >,double)


template<>
CallOutId callOut( boost::function< void() > f, float delta_sec )
{
   return callOut<double>(f,delta_sec);
} // eo callOut(boost::function< void() >,float)




template<>
CallOutId frozenCall( boost::function< void() > f, long delta_sec)
{
   CallerPtr caller( new Detail::Caller(f,delta_sec, 0, true) );
   add_call(caller);
   return caller->getCallOutId();
} // eo frozenCall(boost::function< void() >,long)

template<>
CallOutId frozenCall( boost::function< void() > f, int delta_sec)
{
   return frozenCall<long>(f,delta_sec);
} // eo frozenCall(boost::function< void() >,int)


template<>
CallOutId frozenCall( boost::function< void() > f, double delta_sec )
{
   long delta_sec_i = (long)delta_sec;
   long delta_sec_m = (long)((delta_sec - (double)delta_sec_i)*1000.0);
   CallerPtr caller( new Detail::Caller(f,delta_sec_i, delta_sec_m, true) );
   add_call(caller);
   return caller->getCallOutId();
} // eo frozenCall(boost::function< void() >,double)


template<>
CallOutId frozenCall( boost::function< void() > f, float delta_sec )
{
   return frozenCall<double>(f,delta_sec);
} // eo frozenCall(boost::function< void() >,float)


} // eo namespace AsyncIo
