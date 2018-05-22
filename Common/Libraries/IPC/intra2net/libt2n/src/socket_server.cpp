/*
Copyright (C) 2006 by Intra2net AG - Gerd v. Egidy

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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <sstream>

#include "socket_server.hxx"
#include "t2n_exception.hxx"
#include "log.hxx"

using namespace std;

namespace libt2n
{

/** @brief create a new tcp-based server
    @param port tcp port you want to listen on
    @param ip the local ip you want to listen on. "0.0.0.0" means all local ips (default).
*/
socket_server::socket_server(int port, const std::string& ip)
    : server(), socket_handler(0,tcp_s)
{
    /* Create the socket. */
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        EXCEPTIONSTREAM(error,t2n_server_error,"error opening socket: " << strerror(errno));

    set_socket_options(sock);

    /* Give the socket a name. */
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    if (inet_aton(ip.c_str(),&sockaddr.sin_addr) == 0)
        EXCEPTIONSTREAM(error,t2n_server_error,"failed listening on invalid ip " << ip);

    if (bind (sock, (struct sockaddr *) &sockaddr, sizeof (sockaddr)) < 0)
    {
        // FIXME: Calls virtual function socket_server::get_logstream() in constructor
        EXCEPTIONSTREAM(error,t2n_server_error,"error binding socket: " << strerror(errno));
    }

    start_listening();
}

/** @brief create a new unix-socked-based server
    @param path path of the socket
    @param filemode permissions you want to open the socket with
    @param user local username for the socket
    @param group local groupname for the socket
*/
socket_server::socket_server(const std::string& path, mode_t filemode, const std::string& user, const std::string& group)
    : server(), socket_handler(0,unix_s)
{
    unix_path=path;

    // TODO: Every EXCEPTIONSTREAM in here calls virtual function get_logstream()

    /* Create the socket. */
    sock = socket (PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        EXCEPTIONSTREAM(error,t2n_server_error,"error opening socket: " << strerror(errno));

    set_socket_options(sock);

    /* Give the socket a name. */
    struct sockaddr_un unix_name;
    unix_name.sun_family = AF_UNIX;
    strncpy (unix_name.sun_path, unix_path.c_str(),sizeof(unix_name.sun_path));

    /* just to make sure there is no other socket file */
    unlink (unix_name.sun_path);

    if (bind (sock, (struct sockaddr *) &unix_name, sizeof (unix_name)) < 0)
        EXCEPTIONSTREAM(error,t2n_server_error,"error binding socket: " << strerror(errno));

    /* change permissions */
    if (chmod (unix_name.sun_path, filemode) != 0) 
        EXCEPTIONSTREAM(error,t2n_server_error,"error changing permission: " << strerror(errno));

    if (!user.empty() && !group.empty())
    {
        // TODO maybe use current user/group if one of them is empty

        struct passwd *socket_user = getpwnam (user.c_str());
        if (socket_user == NULL) 
            EXCEPTIONSTREAM(error,t2n_server_error,"error getting socket user: " << strerror(errno));

        struct group *socket_group = getgrnam (group.c_str());
        if (socket_group == NULL) 
            EXCEPTIONSTREAM(error,t2n_server_error,"error getting socket group: " << strerror(errno));

        if (chown (unix_name.sun_path, socket_user->pw_uid, socket_group->gr_gid) != 0) 
            EXCEPTIONSTREAM(error,t2n_server_error,"error changing socket ownership: " << strerror(errno));
    }

    start_listening();
}

/**
 * Destructor
 */
socket_server::~socket_server()
{
    // close all client connections
    server::close();

    // server socket will be closed by destructor of socket_handler

    if (get_type()==unix_s)
        unlink(unix_path.c_str());

    // disconnect connection<->server pointer
    std::map<unsigned int, server_connection*>::iterator it, it_end = connections.end();
    for (it = connections.begin(); it != it_end; ++it)
    {
        socket_server_connection *conn = dynamic_cast<socket_server_connection*>(it->second);
        if (conn)
            conn->my_server = NULL;
    }
}

/// start listening on a new server socket (called by the constructors)
void socket_server::start_listening()
{
    if (listen (sock, 5) < 0)
        EXCEPTIONSTREAM(error,t2n_server_error,"error listening to socket: " << strerror(errno));

    /* clear & insert server sock into the fd_tab to prepare select */
    FD_ZERO(&connection_set);
    FD_SET (sock, &connection_set);
    sockets_set.insert(sock);
}

/// handle a new connection from a client
void socket_server::new_connection()
{
    struct sockaddr_un clientname;

    unsigned int size = sizeof (clientname);
    int newsock = accept (sock,(struct sockaddr *) &clientname,&size);
    if (newsock < 0)
    {
        // return on non-fatal errors (list taken from man-page)
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED || errno == EINTR ||
            errno == EMFILE || errno == ENFILE || errno == ENOBUFS || errno == ENOMEM ||
            errno == EPROTO || errno ==  EPERM || errno == ETIMEDOUT)
        {
            LOGSTREAM(error,"non-fatal accept error: " << strerror(errno));
            return;
        }

        /* fatal error: will usually kill or restart the server */
        EXCEPTIONSTREAM(error,t2n_server_error,"fatal error accepting connection: " << strerror(errno));
    }

    FD_SET (newsock, &connection_set);
    sockets_set.insert(newsock);

    socket_server_connection *nc=new socket_server_connection(newsock, get_type(), get_default_timeout());
    nc->set_socket_options(newsock);

    add_connection(nc);

    return;
}

/** @brief look for new connections and new data in any of the existing connections
    @param usec_timeout wait until new data is found, max timeout usecs.
            -1: wait endless
            0: return instantly
    @param usec_timeout_remaining if non-NULL the function will write the
            not used time to the given target
    @retval true if new data was found (does not mean that the received data 
            is a complete packet though)
*/
bool socket_server::fill_buffer(long long usec_timeout,long long* usec_timeout_remaining)
{
    fd_set used_fdset=connection_set;

    /* set timeout */
    struct timeval tval;
    struct timeval *timeout_ptr;

    if (usec_timeout == -1)
        timeout_ptr = NULL;
    else
    {
        timeout_ptr = &tval;

        // timeout von long long usec in int sec + int usec umrechnen
        tval.tv_sec = usec_timeout / 1000000;
        tval.tv_usec = usec_timeout % 1000000;
    }

    int ret=select (FD_SETSIZE, &used_fdset, NULL, NULL, timeout_ptr);

    // return the timeout we did not use
    if (usec_timeout > 0 && usec_timeout_remaining != NULL)
        *usec_timeout_remaining=(tval.tv_sec*1000000)+tval.tv_usec;

    if (ret < 0)
    {
        if (errno == EINTR)
        {
            // select interrupted by signal
            ret=0;
        }
        else
            EXCEPTIONSTREAM(error,t2n_server_error,"select error: " << strerror(errno));
    }

    if (ret > 0)
    {
        // we have data pending

        // check for new connection
        if (FD_ISSET (sock, &used_fdset))
        {
            new_connection();
        }

        // check all connections for pending data
        return fill_connection_buffers();
    }

    return false;
}

/// call fill_buffer() on all connections, called from fill_buffer()
bool socket_server::fill_connection_buffers()
{
    bool data_found = false;

    std::map<unsigned int, server_connection*>::iterator ie=connections.end();
    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; i++)
        if (!i->second->server_connection::is_closed())
        {
            // shutdown all connections which throw exceptions to protect the server
            try
            {
                if (i->second->fill_buffer(0))
                    data_found=true;
            }
            catch (t2n_transfer_error &e)
                { i->second->close(); }
        }

    return data_found;
}

/// remove the socket of a connection after the connection has been closed
void socket_server::remove_connection_socket(int sock)
{
    FD_CLR(sock, &connection_set);
    sockets_set.erase(sock);
}

/**
 * Destructor
 */
socket_server_connection::~socket_server_connection()
{
    // Only notify parent server about going down.
    // The real socket will be closed by the destructor of the base classes.
    if (my_server && sock != -1)
    {
        socket_server *srv = dynamic_cast<socket_server*>(my_server);
        if (srv)
            srv->remove_connection_socket(sock);
    }
}

/// close this connection. complete data waiting in the buffer can still be retrieved.
void socket_server_connection::close()
{
    if (my_server && sock != -1)
    {
        socket_server *srv = dynamic_cast<socket_server*>(my_server);
        if (srv)
            srv->remove_connection_socket(sock);
    }

    if (!server_connection::is_closed())
    {
        socket_handler::close();
        server_connection::close();
    }
}

bool socket_server_connection::fill_buffer(long long usec_timeout,long long* usec_timeout_remaining)
{
    bool new_data = socket_handler::fill_buffer(buffer,usec_timeout,usec_timeout_remaining);
    if (new_data)
        reset_timeout();
    return new_data;
}
}
