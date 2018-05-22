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
#ifndef __LIBT2N_SOCKET_CLIENT
#define __LIBT2N_SOCKET_CLIENT

#include "client.hxx"
#include "socket_handler.hxx"

struct sockaddr;

namespace libt2n
{
/** @brief a connection from client to server using sockets.

    Use this class to connect from a client to a server.
 */
class socket_client_connection : public client_connection, public socket_handler
{
    public:
        static const int max_retries_default=3;
        static const long long connect_timeout_usec_default=30000000;

    private:
        void real_write(const std::string& data)
            { socket_write(data); }

        void tcp_connect(int max_retries);
        void unix_connect(int max_retries);
        void connect_with_timeout(struct sockaddr *sock_addr,unsigned int sockaddr_size);

        int max_retries;
        long long connect_timeout_usec;

        std::string path;
        std::string server;
        int port;

        std::string lastErrorMsg;

    protected:

        std::ostream* get_logstream(log_level_values level)
            { return client_connection::get_logstream(level); }

    public:
        socket_client_connection(int _port, const std::string& _server="127.0.0.1", 
            long long _connect_timeout_usec=connect_timeout_usec_default, 
            int _max_retries=max_retries_default,
            std::ostream *_logstream=NULL, log_level_values _log_level=none);
        socket_client_connection(const std::string& _path,
            long long _connect_timeout_usec=connect_timeout_usec_default, 
            int _max_retries=max_retries_default,
            std::ostream *_logstream=NULL, log_level_values _log_level=none);

        ~socket_client_connection();

        /** @brief read data from the socket and copy it into buffer
            @param usec_timeout wait until new data is found, max timeout usecs.
                  -1: wait endless
                   0: return instantly
            @param usec_timeout_remaining if non-NULL the function will write the
                  not used time to the given target
            @retval true if new data was found (does not mean that the received data 
                    is a complete packet though)
        */
        bool fill_buffer(long long usec_timeout=-1, long long *usec_timeout_remaining=NULL)
            { return socket_handler::fill_buffer(buffer,usec_timeout,usec_timeout_remaining); }

        virtual void close();

        void reconnect();

        std::string get_last_error_msg(void)
            { return lastErrorMsg; }
};

}

#endif
