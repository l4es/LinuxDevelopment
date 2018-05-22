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

#include "async_pipe.hpp"

#include <functional>
#include <boost/bind.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>


namespace AsyncIo
{


/*
 * Implementation of SimplePipe
 */

SimplePipe::SimplePipe()
{
   m_signal_read.connect(boost::bind(&SimplePipe::slotReceived,this));
} // eo SimplePipe::SimplePipe()


SimplePipe::~SimplePipe()
{
} // eo SimplePipe::~SimplePipe()


/**
 * makes a pipe.
 * This method connects itself with a newly created peer object with a bidirectional pipe.
 * @return the peer pipe object.
 */
bool SimplePipe::makePipe(SimplePipe& peer)
{
   close(); // just in case...

   int fds[2];

   int res= ::socketpair( AF_UNIX, SOCK_STREAM, 0, fds);

   if (res)
   {
      m_errno= errno;
      return false;
   }
   else
   {
      m_errno= 0;
   }

   peer.close(); // just in case

   setWriteFd(fds[0]);
   setReadFd(fds[0]);

   peer.setWriteFd(fds[1]);
   peer.setReadFd(fds[1]);

   return true;
} // eo SimplePipe.:makePipe()


/**
 * sends a string through the pipe.
 * @param data the data which should be sent to the other side.
 */
void SimplePipe::sendString(const std::string& data)
{
   lowSend(data);
} // eo SimplePipe::sendString(const std::string&)


/**
 * emits the signal signalReceived with the received data.
 * This slot is connected to IOImplementation::m_signal_read.
 */
void SimplePipe::slotReceived()
{
   std::string data;
   data.swap(m_input_buffer);
   signal_received_string(data);
} // eo SimplePipe::slotReceived()


} // eo namespace AsyncIo
