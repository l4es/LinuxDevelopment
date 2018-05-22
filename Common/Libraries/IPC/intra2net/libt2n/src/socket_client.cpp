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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include <sstream>

#include "socket_client.hxx"
#include "t2n_exception.hxx"
#include "log.hxx"

using namespace std;

namespace libt2n
{

/// returns a closed connection if connection could not be established, call get_last_error_msg() for details
socket_client_connection::socket_client_connection(int _port, const std::string& _server, 
            long long _connect_timeout_usec, int _max_retries,
            std::ostream *_logstream, log_level_values _log_level)
    : client_connection(), socket_handler(0,tcp_s)
{
    max_retries=_max_retries;
    connect_timeout_usec=_connect_timeout_usec;

    server=_server;
    port=_port;

    set_logging(_logstream,_log_level);

    try
    {
        tcp_connect(max_retries);
    }
    catch (t2n_communication_error &e)
    {
        lastErrorMsg=e.what();
        LOGSTREAM(debug,"tcp connect error: " << lastErrorMsg);
        // FIXME: Don't call virtual function in constructor. Currently not dangerous but bad design.
        close();
    }

    if (!connection::is_closed())
        do_callbacks(new_connection);
}

/// returns a closed connection if connection could not be established, call get_last_error_msg() for details
socket_client_connection::socket_client_connection(const std::string& _path,
            long long _connect_timeout_usec, int _max_retries,
            std::ostream *_logstream, log_level_values _log_level)
    : client_connection(), socket_handler(0,unix_s)
{
    max_retries=_max_retries;
    connect_timeout_usec=_connect_timeout_usec;

    path=_path;

    set_logging(_logstream,_log_level);

    try
    {
        unix_connect(max_retries);
    }
    catch (t2n_communication_error &e)
    {
        lastErrorMsg=e.what();
        LOGSTREAM(debug,"unix connect error: " << lastErrorMsg);
        // FIXME: Don't call virtual function in constructor. Currently not dangerous
        close();
    }

    if (!connection::is_closed())
        do_callbacks(new_connection);
}

/**
 * Destructor. Closes an open connection.
 */
socket_client_connection::~socket_client_connection()
{
    // Destructor of socket_handler will close the socket!
}


/// establish a connection via tcp
void socket_client_connection::tcp_connect(int max_retries)
{
    struct sockaddr_in sock_addr;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);

    // find the target ip
    if (inet_aton(server.c_str(),&sock_addr.sin_addr)==0)
    {
        struct hostent *server_hent;
        server_hent=gethostbyname(server.c_str());
        if (server_hent == NULL)
            throw t2n_connect_error(string("can't find server ")+server);

        memcpy(&sock_addr.sin_addr,server_hent->h_addr_list[0],sizeof(sock_addr.sin_addr));
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (!sock)
        throw t2n_connect_error(string("socket() error: ")+strerror(errno));

    try
    {
        connect_with_timeout((struct sockaddr *) &sock_addr,sizeof(sock_addr));
    }
    catch (t2n_connect_error &e)
    {
        // recurse if retries left
        if (max_retries > 0)
        {
            LOGSTREAM(debug,"retrying connect after connect error");
            tcp_connect(max_retries-1);
        }
        else
            throw t2n_connect_error("no more retries left after connect error");
    }
}

/// establish a connection via unix-socket
void socket_client_connection::unix_connect(int max_retries)
{
    struct sockaddr_un unix_addr;
    size_t path_size = path.size();

    unix_addr.sun_family = AF_UNIX;

    if (path_size >= sizeof(unix_addr.sun_path))
    {
        throw t2n_connect_error((std::string)"path '"
                                + path
                                + "' exceeds permissible UNIX socket path length");
    }

    memcpy(unix_addr.sun_path, path.c_str(), path_size);
    unix_addr.sun_path[path_size] = '\0';

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (!sock)
        throw t2n_connect_error(string("socket() error: ")+strerror(errno));

    try
    {
        connect_with_timeout((struct sockaddr *) &unix_addr, sizeof(unix_addr));
    }
    catch (t2n_connect_error &e)
    {
        // recurse if retries left
        if (max_retries > 0)
        {
            LOGSTREAM(debug,"retrying connect after connect error");
            unix_connect(max_retries-1);
        }
        else
            throw t2n_connect_error("no more retries left after connect error");
    }
}

/// execute a connect on a prepared socket (tcp or unix) respecting timeouts
void socket_client_connection::connect_with_timeout(struct sockaddr *sock_addr,unsigned int sockaddr_size)
{
    set_socket_options(sock);

   /* non-blocking mode */
    int flflags;
    flflags=fcntl(sock,F_GETFL,0);
    if (flflags < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));

    flflags &= (O_NONBLOCK ^ 0xFFFF);
    if (fcntl(sock,F_SETFL,flflags) < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));


    LOGSTREAM(debug,"connect_with_timeout()");
    int ret=::connect(sock,sock_addr, sockaddr_size);

    if (ret < 0)
    {
        if (errno==EINPROGRESS)
        {
            LOGSTREAM(debug,"connect_with_timeout(): EINPROGRESS");

            /* set timeout */
            struct timeval tval;
            struct timeval *timeout_ptr;

            if (connect_timeout_usec == -1)
                timeout_ptr = NULL;
            else
            {
                timeout_ptr = &tval;

                // convert timeout from long long usec to int sec + int usec
                tval.tv_sec = connect_timeout_usec / 1000000;
                tval.tv_usec = connect_timeout_usec % 1000000;
            }

            fd_set connect_socket_set;
            FD_ZERO(&connect_socket_set);
            FD_SET(sock,&connect_socket_set);

            int ret;
            while ((ret=select(FD_SETSIZE, NULL, &connect_socket_set, NULL, timeout_ptr)) &&
                    ret < 0 && errno==EINTR);

            if (ret < 0)
                throw t2n_connect_error(string("connect() error (select): ")+strerror(errno));

            socklen_t sopt=sizeof(int);
            int valopt;
            ret=getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &sopt);
            if (ret < 0 || valopt)
                throw t2n_connect_error(string("connect() error (getsockopt): ")+strerror(errno));
        }
        else
            throw t2n_connect_error(string("connect() error: ")+strerror(errno));
    }

    LOGSTREAM(debug,"connect_with_timeout(): success");
}

void socket_client_connection::close()
{
    if (!client_connection::is_closed())
    {
        socket_handler::close();
        client_connection::close();
    }
}

/** @brief try to reconnect the current connection with the same connection credentials (host and port or path)

    @note will throw an exeption if reconnecting not possible
*/
void socket_client_connection::reconnect()
{
    LOGSTREAM(debug,"reconnect()");

    // close the current connection if still open
    close();

    socket_type_value type=get_type();

    if (type == tcp_s)
        tcp_connect(max_retries);
    else if (type == unix_s)
        unix_connect(max_retries);

    // connection is open now, otherwise an execption would have been thrown
    reopen();

    LOGSTREAM(debug,"reconnect() done, client_connection::is_closed() now " << client_connection::is_closed());
}

}
