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
 * @brief some functions to deal with (smart) pointer
 *
 * @author Reinhard Pfau \<Reinhard.Pfau@gmx.de\>
 * @copyright &copy; Copyright 2009 by Intra2net AG
 */

#ifndef _ASYNCIO_POINTER_FUNC_HPP_
#define _ASYNCIO_POINTER_FUNC_HPP_

#include <asyncio_config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#ifdef HAVE_LIBI2NCOMMON
#  include <pointer_func.hpp>
#endif

namespace AsyncIo
{
namespace Utils
{

#ifdef HAVE_LIBI2NCOMMON

// NOTE: this typedef is needed by I2N code; please keep it!

typedef I2n::SharedBase SharedBase;

#else

class SharedBase
: public boost::enable_shared_from_this< SharedBase >
{
  public:
    typedef boost::shared_ptr< SharedBase > PtrType;

  public:
    SharedBase() {}
    virtual ~SharedBase() {}

    template< class D >
    boost::shared_ptr< D > get_ptr_as()
    {
      try
      {
        return boost::dynamic_pointer_cast< D >( shared_from_this() );
      }
      catch( boost::bad_weak_ptr& e )
      {
        // we ignore this error.
      }
      return boost::shared_ptr< D >();
    }
    
}; // end of SharedBase

#endif


} // end of namespace Utils
} // end of namespace AsyncIo


#endif
