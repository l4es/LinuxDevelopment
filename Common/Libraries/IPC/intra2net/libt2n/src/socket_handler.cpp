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
#include <iostream>
#include <algorithm>

#include "socket_handler.hxx"
#include "t2n_exception.hxx"
#include "log.hxx"

using namespace std;

namespace libt2n
{

socket_handler::socket_handler(int _sock, socket_type_value _socket_type)
: sock(_sock)
, recv_buffer_size( default_recv_buffer_size )
, write_block_size( default_write_block_size )
, write_timeout( default_write_timeout )
, socket_type(_socket_type)
{
}

/**
 * Destructor. Closes open socket
 */
socket_handler::~socket_handler()
{
    if (sock != -1)
    {
        shutdown(sock,SHUT_RDWR);
        ::close(sock);

        sock = -1;
    }
}

/// close the underlying socket connection. Don't call directly, use the version provided
/// by the connection class you are using.
void socket_handler::close()
{
    LOGSTREAM(debug,"close connection");
    // graceful shutdown
    shutdown(sock,SHUT_RDWR);
    ::close(sock);

    sock = -1;
}

/// set options like fast reuse and keepalive every socket should have
void socket_handler::set_socket_options(int sock)
{
    int i=1;

    /* fast reuse enable */
    if (setsockopt(sock,SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"error setting socket option: " << strerror(errno));

    /* keepalive enable */
    if (setsockopt(sock,SOL_SOCKET, SO_KEEPALIVE, &i, sizeof(i)) < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"error setting socket option: " << strerror(errno));

    /* close on exec */
    int fdflags;
    fdflags=fcntl(sock,F_GETFD, 0);
    if (fdflags < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));

    fdflags |= FD_CLOEXEC;
    if (fcntl(sock,F_SETFD,fdflags) < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));

    /* non-blocking mode */
    int flflags;
    flflags=fcntl(sock,F_GETFL,0);
    if (flflags < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));

    flflags |= O_NONBLOCK;
    if (fcntl(sock,F_SETFL,flflags) < 0)
        EXCEPTIONSTREAM(error,t2n_communication_error,"fcntl error on socket: " << strerror(errno));
}

/// is the underlying socket connection still open?
bool socket_handler::is_closed()
{
    int r=fcntl(sock,F_GETFL);

    return !(r & O_ACCMODE);
}


/**
 * @brief set a new size for the receive buffer.
 * @param new_recv_buffer_size the new size for the receive buffer.
 *
 * The receive buffer determines the amount of data which is tried to read at once
 * from the underlying socket.
 *
 * The value is normalized to be at least 512 bytes and at max 32K bytes.
 */
void socket_handler::set_recv_buffer_size(unsigned int new_recv_buffer_size)
{
    recv_buffer_size= std::max( 512u, std::min( 32u * 1024u, new_recv_buffer_size ));
} //


/**
 * @brief set new size for the data chunks when writeing.
 * @param new_write_block_size the new chunk size.
 *
 * The write block size determines the amount of data which is tried to write
 * to the socket when data needs to be sended.
 * Since writing data is done in a loop, this does not limit the amunt of data which can
 * be written.
 *
 * The value is normalized to be at least 512 bytes and at max 32K bytes.
 */
void socket_handler::set_write_block_size(unsigned int new_write_block_size)
{
    write_block_size= std::max( 512u, std::min( 32u * 1024u, new_write_block_size ));
} //


/**
 * @brief set new timeout for writing a block
 * @param new_write_timeout the new timeout in usecs, -1: wait endless
 *
 * The write timeout determines the maximum amount of time that is waited
 * between writing each block. If the timeout is exceeded, write will
 * throw t2n_transfer_error
 */
void socket_handler::set_write_timeout(long long new_write_timeout)
{
    write_block_size=new_write_timeout;
} //


/** @brief check if new data is waiting on the raw socket
    @param[in,out] usec_timeout wait until new data is found, max timeout usecs.
            -1: wait endless
            0: return instantly
    @param[out] usec_timeout_remaining microseconds from the timeout that were not used
*/
bool socket_handler::data_waiting(long long usec_timeout,long long* usec_timeout_remaining)
{
    // just our socket
    fd_set active_fd_set;
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

    /* set timeout */
    struct timeval tval;
    struct timeval *timeout_ptr;

    if (usec_timeout == -1)
        timeout_ptr = NULL;
    else
    {
        timeout_ptr = &tval;

        // convert timeout from long long usec to int sec + int usec
        tval.tv_sec = usec_timeout / 1000000;
        tval.tv_usec = usec_timeout % 1000000;
    }

    int ret=select (FD_SETSIZE, &active_fd_set, NULL, NULL, timeout_ptr);

    // return the timeout we did not use
    // todo: this is linux specific according to man 2 select
    if (usec_timeout > 0 && usec_timeout_remaining != NULL)
        *usec_timeout_remaining=(tval.tv_sec*1000000)+tval.tv_usec;

    if (ret > 0)
        return true;
    else
        return false;
}

/** @brief read data from the raw socket and copy it into the provided buffer
    @param buffer the buffer where to append the new data
    @param[in,out] usec_timeout wait until new data is found, max timeout usecs.
            -1: wait endless
            0: return instantly
    @param[out] usec_timeout_remaining microseconds from the timeout that were not used
*/
bool socket_handler::fill_buffer(std::string& buffer, long long usec_timeout, long long *usec_timeout_remaining)
{
    // fast path for timeout==0
    if (usec_timeout==0 || data_waiting(usec_timeout,usec_timeout_remaining))
        return fill_buffer(buffer);
    else
        return false;
}

/** @brief read data from the raw socket and copy it into the provided buffer. Returns
           instantly if no data is waiting.
    @param buffer the buffer where to append the new data
*/
bool socket_handler::fill_buffer(std::string& buffer)
{
    bool try_again=false;

    char socket_buffer[recv_buffer_size];

    int nbytes = read (sock, socket_buffer, recv_buffer_size);

    if (nbytes < 0)
    {
        if (errno == EAGAIN)
            return false;                // no data was waiting
        else if (errno == EINTR)
        {
            // interrupted, try again
            LOGSTREAM(debug,"EINTR received on read(), trying again");
            try_again=true;
        }
        else
            EXCEPTIONSTREAM(error,t2n_transfer_error,"error reading from socket : " << strerror(errno));
    }

    // End-of-file
    if (nbytes == 0 && !try_again)
    {
        LOGSTREAM(debug,"0 bytes received on read(), closing connection");
        close();
        return false;
    }

    // Data read -> store it
    if (nbytes > 0)
    {
        buffer.append(socket_buffer,nbytes);
        LOGSTREAM(debug,nbytes << " bytes read");
    }

    // more data waiting -> recurse
    if (data_waiting(0))
        fill_buffer(buffer);

    if (nbytes > 0)
        return true;
    else
        return false;
}

/// writes raw data to the socket. Don't use directly, use the write() function provided by the 
/// connection because it encapsulates the data.
void socket_handler::socket_write(const std::string& data)
{
    unsigned int offset = 0;
    while (offset < data.size())
    {
        unsigned int write_size=write_block_size;

        if (offset+write_size > data.size())
            write_size = data.size()-offset;

        int rtn;
        while ((rtn=::write(sock, data.data()+offset, write_size)) == -1 &&
               (errno == EAGAIN || errno == EINTR))
        {
            wait_ready_to_write(sock,write_timeout);
            LOGSTREAM(debug,"resuming write() call after EAGAIN or EINTR");
        }

        if (rtn == -1)
            EXCEPTIONSTREAM(error,t2n_transfer_error,"write() returned " << strerror(errno));
        else if (rtn != write_size)
        {
            LOGSTREAM(debug,"write() wrote " << rtn << " bytes, should have been "
                << write_size << " (complete: " << data.size() << ")");

            write_size = rtn;
        }

        offset += write_size;
    }

    LOGSTREAM(debug,"wrote " << data.size() << " bytes");

    return;
} // eo socket_handler::socket_write(const std::string&)

/// wait until the socket is ready to write again
void socket_handler::wait_ready_to_write(int socket, long long write_block_timeout)
{
    // prepare socket sets
    fd_set write_set[1];
    fd_set except_set[1];
    FD_ZERO(write_set);
    FD_ZERO(except_set);
    FD_SET(socket, write_set);
    FD_SET(socket, except_set);

    // prepare timeout struct
    struct timeval tval;
    struct timeval *timeout_ptr;

    if (write_block_timeout == -1)
        timeout_ptr = NULL;
    else
    {
        timeout_ptr = &tval;

        // convert timeout from long long usec to int sec + int usec
        tval.tv_sec = write_block_timeout / 1000000;
        tval.tv_usec = write_block_timeout % 1000000;
    }

    // let's wait for the socket to become writable again...
    int rtn;
    while ((rtn=::select(socket+1, NULL, write_set, except_set, timeout_ptr)) ==-1 && errno == EINTR);

    if (rtn > 0 && (!FD_ISSET(socket,write_set)) && FD_ISSET(socket, except_set))
    {
        // if we are selected but cannot write and have an exception
        // we have serious trouble...
        EXCEPTIONSTREAM(error,t2n_transfer_error,"exception on socket; cannot write any more.");
    }

    if (rtn==0)
        EXCEPTIONSTREAM(error,t2n_transfer_error,"timeout on select() for write");

    if (rtn==-1)
        EXCEPTIONSTREAM(error,t2n_transfer_error,"cannot select() for write: " << strerror(errno));
}


}
