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
 * @copyright Copyright 2008-2009 by Intra2net AG
 * @contact Intra2net Opensource Team \<opensource@intra2net.com\>
 *
 * @todo unlink unix server socket on close.
 */

#include "async_socket.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <limits.h>
#include <asyncio_system_tools.cpp>


namespace AsyncIo
{


namespace
{

/**
    Struct for holding sockaddr.

    It's called "MegaAddr" because we use a trick to extend the historical UNIX domain socket
    path length of 108 bytes to PATH_MAX by declaring a large enough buffer
    at the end of the union.

    This works as long as the POSIX functions don't verify the buffer length.
    For glibc on linux this is true.

    UPDATE 2011-08-26:
    Removed that buffer as the gcc stack protector chokes on this.

    Now we are "limited" to 108 bytes again. The longest socket path
    currently in use on the Intranator is 68 bytes. It looks already
    longer than Richard Stallman's beard, so it should be fine.
*/
union MegaAddr {
   struct sockaddr         m_addr;
   struct sockaddr_un      m_addr_un; // NOTE (historically) too small...
};


} // eo namespace <anonymous>



/*
** implementation of ServerSocketBaseImplementation
*/


ServerSocketBaseImplementation::ServerSocketBaseImplementation()
: IOImplementation()
{
} // eo ServerSocketBaseImplementation::ServerSocketBaseImplementation()


/**
 * @brief handled incoming connections on the server port.
 *
 * accepts the new connection, stores the peer address in an internal buffer
 * and calls a (derived) acceptNewConnection method to create an appropriate
 * IO class instance.
 * If no io instance is created the connection is closed.
 */
void ServerSocketBaseImplementation::doRead()
{
   MegaAddr addr;
   socklen_t addrlen = sizeof(addr);

   // reset errno:
   m_errno= 0;

   // reset the mark:
   resetReadMark();

   int fd = ::accept( readFd(), &addr.m_addr, &addrlen);
   if (fd < 0)
   {
      // handle errors.
      m_errno= errno;
      switch (m_errno)
      {
         case EBADF:
         case EINVAL:
         case ENOTSOCK:
            // should not happen...
            close();
            break;
         default:;
      }
      return;
   }

   if (addrlen >= sizeof(addr))
   {
      //something went terribly wrong!!
      // the resulting address structure is larger than it ever could be...
      ::close(fd);
      return;
   }

   IOImplementationPtr connection= acceptNewConnection(fd, &addr.m_addr);
   if(!connection)
   {
      ::close(fd);
      return;
   }
   if (m_new_connection_base_callback)
   {
      m_new_connection_base_callback(connection);
   }
} // eo ServerSocketBaseImplementation::doRead()


/**
 * @brief handles write events.
 *
 * since server sockets never ever get a write mark, something must
 * be wrong and the connection is closed!
 */
void ServerSocketBaseImplementation::doWrite()
{
   // if this method is called, something went wrong...
   close();
   //TODO throw something?!
} // eo ServerSocketBaseImplementation::doWrite()



/**
 * @brief sets a function which is called when a new connection was established.
 *
 * The function gets a (shared) pointer to the new connection as parameter and is
 * expected to store it when it accepts the connection.
 * (Else the connection gets deleted after the function was called.)
 *
 * @param func the function which should be called on new connections.
 */
void ServerSocketBaseImplementation::setNewConnectionBaseCallback(
   const NewConnectionBaseCallbackFunc& func)
{
   m_new_connection_base_callback= func;
} // eo ServerSocketBaseImplementation::setNewConnectionBaseCallback(NewConnectionBaseCallbackFunc&)



/**
 * @brief callback for new connections.
 *
 * The base method always returns an empty pointer.
 *
 * Derived classes must override this method and do something useful with
 * the passed file descriptor.
 *
 * @param fd the file descriptor of the new connection.
 * @param addr pointer to the address structure filled from ::accept()
 * @return shared pointer to the new IO class instance (; empty if not accepted)
 */
IOImplementationPtr ServerSocketBaseImplementation::acceptNewConnection(
   int fd, boost::any addr)
{
   // needs to be defined in derived class!
   return IOImplementationPtr();
} // eo ServerSocketBaseImplementation::acceptNewConnection(int,boost::any)



/*
** implementation of UnixIOSocket
*/


UnixIOSocket::UnixIOSocket()
: m_peer_pid((unsigned int)-1)
, m_peer_uid((unsigned int)-1)
, m_peer_gid((unsigned int)-1)
{
} // eo UnixIOSocket::UnixIOSocket()



UnixIOSocket::UnixIOSocket(const std::string& path)
: m_peer_pid((unsigned int)-1)
, m_peer_uid((unsigned int)-1)
, m_peer_gid((unsigned int)-1)
{
    open(path);
} // eo UnixIOSocket::UnixIOSocket(const std::string&)


UnixIOSocket::UnixIOSocket(int fd,const std::string& path)
: IOImplementation(fd,fd)
, m_path(path)
, m_peer_pid((unsigned int)-1)
, m_peer_uid((unsigned int)-1)
, m_peer_gid((unsigned int)-1)
{
    update_peer_information(fd);
} // eo UnixIOSocket::UnixIOSocket(int,const std::string&)


/**
 * @brief opens a (client) connection to an unix domain socket.
 *
 * @param path the path the server is listening on.
 * @return @a true iff the connection was successfully opened.
 */
bool UnixIOSocket::open(const std::string& path)
{
   if (opened()) close();

   m_errno= 0;
   m_path.clear();

   if (path.empty() || path.size() >= PATH_MAX)
   {
      return false;
   }

   int fd= ::socket(PF_UNIX, SOCK_STREAM, 0);
   if (fd<0)
   {
      m_errno= errno;
      return false;
   }

   {
      MegaAddr addr;
      addr.m_addr_un.sun_family= AF_UNIX;

      strncpy(addr.m_addr_un.sun_path, path.c_str(), sizeof(addr.m_addr_un.sun_path));
      addr.m_addr_un.sun_path[sizeof(addr.m_addr_un.sun_path)-1] = '\0';

      if (::connect(fd,(sockaddr*)&addr.m_addr_un, SUN_LEN(&addr.m_addr_un)) < 0)           //lint !e413
      {
         m_errno= errno;
         ::close(fd);
         return false;
      }
   }
   m_path= path;
   setReadFd(fd);
   setWriteFd(fd);
   return true;
} // eo UnixIOSocket::open(const std::string&,int)


/**
 * @brief update the peer info (pid,gid,uid).
 * @param fd the socket to update the information for.
 * @return @a true if succesfully determined the peer info.
 *
 * @note if the peer info could not be detected the values are set to
 * (unsigned int)(-1).
 */
bool UnixIOSocket::update_peer_information(int fd)
{
    m_peer_pid=(unsigned int)-1;
    m_peer_gid=(unsigned int)-1;
    m_peer_uid=(unsigned int)-1;

    //TODO add more versions of getting the peer information here...
    // BSD systems seems to have SO_PEERCRED,too (does someone know how it is used there?)

 #ifdef __linux__
    { // the linux way to get peer info (pid,gid,uid):
       struct ucred cred;
       socklen_t cred_len = sizeof(cred);
       if (getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&cred,&cred_len) == 0)
       {
          m_peer_pid= cred.pid;
          m_peer_uid= cred.uid;
          m_peer_gid= cred.gid;
          return true;
       }
    }
 #else
 #warning dont know how to determine peer info.
 #endif
    return false;
} // end of UnixIOSocketupdate_peer_information(int)


/*
** implementation of UnixServerSocketBase
*/


UnixServerSocketBase::UnixServerSocketBase()
{
} // eo UnixServerSocketBase::UnixServerSocketBase


UnixServerSocketBase::UnixServerSocketBase(const std::string& path, int mode)
{
   open(path,mode);
} // eo UnixServerSocketBase::UnixServerSocketBase(const std::string&,int)


/**
 * @brief opens the server part of an unix domain socket.
 *
 * @param path the path the new port should listen on.
 * @param mode the mode for the path.
 * @return @a true iff the port was successfully opened.
 */
bool UnixServerSocketBase::open(const std::string& path, int mode)
{
   if (opened()) close();
   m_errno= 0;
   if (path.empty() || path.size() >= PATH_MAX)
   {
      return false;
   }

   int fd= ::socket(PF_UNIX, SOCK_STREAM, 0);
   if (fd<0)
   {
      m_errno= errno;
      return false;
   }

   {
      MegaAddr addr;
      addr.m_addr_un.sun_family= AF_UNIX;

      strncpy(addr.m_addr_un.sun_path, path.c_str(), sizeof(addr.m_addr_un.sun_path));
      addr.m_addr_un.sun_path[sizeof(addr.m_addr_un.sun_path)-1] = '\0';

      Utils::unlink(path); // just in case...
      // NOTE this is a place which might require some updates for multithreaded
      // usage! (setting the umask affects all threads...)
      mode_t old_mask= ::umask( (mode & 0777) ^ 0777);
      if (::bind(fd,(sockaddr*)&addr.m_addr_un, SUN_LEN(&addr.m_addr_un)) < 0)              //lint !e413
      {
         m_errno= errno;
         ::umask(old_mask);
         ::close(fd);
         return false;
      }
      ::umask(old_mask);
   }
   m_path= path;

   {
      int res= ::listen(fd,8);
      if (res < 0)
      {
         m_errno= errno;
         ::close(fd);
         return false;
      }
   }
   setReadFd(fd);
   return true;
} // eo UnixServerSocketBase::open(const std::string&,int)


/**
 * @brief called from base class to create a new connection instance.
 *
 * This method accepts only connections to unix domain sockets.
 * It also tries to determine the peer pid, uid and gid.
 *
 * @param fd the file descriptor of a freshly accepted connection.
 * @param addr contains "pointer to struct sockaddr"
 * @return @a a (shared) pointer to the new connection instance; empty if none was
 * created.
 */
IOImplementationPtr UnixServerSocketBase::acceptNewConnection(int fd, boost::any addr)
{
   struct sockaddr *addr_ptr= NULL;
   try {
      addr_ptr = boost::any_cast<struct sockaddr*>(addr);
   }
   catch (boost::bad_any_cast&)
   {
      return IOImplementationPtr();
   }
   // check for the right family:
   if (addr_ptr->sa_family != AF_UNIX)
   {
      return IOImplementationPtr();
   }
   struct sockaddr_un *un_ptr = reinterpret_cast<struct sockaddr_un*>(addr_ptr);
   std::string peer_path( un_ptr->sun_path );

   UnixIOSocketPtr ptr( createIOSocket(fd, peer_path) );

   return ptr;
} // eo UnixServerSocketBase::acceptNewConnection(int,boost::any);


/**
 * @brief "real" creator of the connection instance.
 *
 * called by UnixServerSocketBase::acceptNewConnection to create the new io instance.
 *
 * @param fd file descriptor for the socket
 * @param path path as delivered by peer.
 * @return (shared) pointer to the new io instance.
 */
UnixIOSocketPtr UnixServerSocketBase::createIOSocket(
   int fd, const std::string& path
)
{
   return UnixIOSocketPtr
   (
       new UnixIOSocket(fd, path)
   );
} // eo UnixServerSocketBase::createIOSocket(int,const std::string&)



}// eo namespace AsyncIo
