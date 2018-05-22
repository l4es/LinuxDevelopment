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
 * @brief socket classes for the AsyncIo framework.
 *
 *  @copyright Copyright 2008-2009 by Intra2net AG
 *  @contact Intra2net Opensource Team \<opensource@intra2net.com\>
 */

#ifndef __ASYNC_SOCKET_HPP__
#define __ASYNC_SOCKET_HPP__

#include "async_io.hpp"
#include "asyncio_system_tools.hpp"

#include <string>
#include <boost/any.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>
#include <boost/function.hpp>


namespace AsyncIo
{


typedef boost::shared_ptr< IOImplementation > IOImplementationPtr;


/**
 * @brief base class for server sockets.
 *
 * Contains all the stuff which is common for different types of server sockets.
 */
class ServerSocketBaseImplementation
: public IOImplementation
{
   public:
      typedef boost::function< void(IOImplementationPtr) > NewConnectionBaseCallbackFunc;

   public:

      void setNewConnectionBaseCallback( const NewConnectionBaseCallbackFunc& func);

   protected:
      ServerSocketBaseImplementation();


      virtual void doRead();
      virtual void doWrite();

      virtual IOImplementationPtr acceptNewConnection(int fd, boost::any addr);


   protected:

      NewConnectionBaseCallbackFunc m_new_connection_base_callback;

}; // eo class ServerSocketBaseImplementation


typedef boost::shared_ptr< ServerSocketBaseImplementation > ServerSocketBaseImplementationPtr;


/*
** unix domain sockets
*/


template<
   class IOClass
>
class UnixServerSocket;


/**
 * @brief specialized IO class for unix domain sockets.
 *
 */
class UnixIOSocket
: public IOImplementation
{
   public:
      UnixIOSocket();
      UnixIOSocket(const std::string& path);

      bool open(const std::string& path);

   protected:
      friend class UnixServerSocketBase;
      friend class UnixServerSocket<UnixIOSocket>;

      UnixIOSocket(int fd, const std::string& path);

      bool update_peer_information(int fd);

   protected:

      std::string  m_path;
      unsigned int m_peer_pid;
      unsigned int m_peer_uid;
      unsigned int m_peer_gid;

}; // eo class UnixIOSocket

typedef boost::shared_ptr< UnixIOSocket > UnixIOSocketPtr;



/**
 * @brief specialized server socket class for unix domain sockets.
 *
 */
class UnixServerSocketBase
: public ServerSocketBaseImplementation
{
   public:
      UnixServerSocketBase();
      UnixServerSocketBase(const std::string& path, int mode=0600);

      bool open(const std::string& path, int mode= 0600);

   protected:

      virtual IOImplementationPtr acceptNewConnection(int fd, boost::any addr);

      virtual UnixIOSocketPtr createIOSocket(int fd, const std::string& path);

   protected:

      std::string m_path;

}; // eo class UnixServerSocketBase


/**
 * @brief unix server socket class which "produces" connections of a determined type.
 *
 * @param IOClass the type of the connections.
 */
template<
   class IOClass
>
class UnixServerSocket
: public UnixServerSocketBase
{
      BOOST_STATIC_ASSERT(( boost::is_base_of<UnixIOSocket,IOClass>::value ));

   public:
      typedef boost::shared_ptr< IOClass > IOClassPtr;
      typedef boost::function< void(IOClassPtr) > NewConnectionCallbackFunc;

   public:

      UnixServerSocket()
      : UnixServerSocketBase()
      {}

      UnixServerSocket(const std::string& path, int mode=0600)
      : UnixServerSocketBase(path,mode)
      {}

      void setNewConnectionCallback( const NewConnectionCallbackFunc& func)
      {
         if (func)
         {
            UnixServerSocketBase::setNewConnectionBaseCallback(
               boost::bind(
                  func,
                  boost::bind<IOClassPtr, IOImplementationPtr>(
                     &UnixServerSocket::my_ptr_cast,
                     _1
                  )
               )
            );
         }
         else
         {
            UnixServerSocketBase::setNewConnectionBaseCallback(
               NewConnectionBaseCallbackFunc()
            );
         }
      }

   protected:

      virtual UnixIOSocketPtr createIOSocket(
         int fd, const std::string& path)
      {
         return UnixIOSocketPtr(
            new IOClass(fd, path)
         );
      }

      static IOClassPtr my_ptr_cast(IOImplementationPtr ptr)
      {
         return boost::dynamic_pointer_cast<IOClass>(ptr);
      }

}; // eo class UnixServerSocket



}// end of namespace AsyncIo


#endif
