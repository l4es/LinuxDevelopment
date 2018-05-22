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
#ifndef __LIBT2N_SOCKET_SERVER
#define __LIBT2N_SOCKET_SERVER

#include <sys/types.h>
#include <string>
#include <set>

#include "server.hxx"
#include "socket_handler.hxx"
#include "types.hxx"

namespace libt2n
{

class socket_server_connection;

/** @brief Socket based server class

    Use this class to instantiate a server listening for client connections.
    Call fill_buffer() to read data from the network and get_packet() to retrieve
    this data. Don't forget to call cleanup() from time to time to remove closed
    connections and close idle ones.
*/
class socket_server : public socket_handler, public server
{
    friend class socket_server_connection;

    private:
        fd_set connection_set;
        std::string unix_path;
        std::set<int> sockets_set;

        void start_listening();

        void new_connection();

        bool fill_connection_buffers();
        void remove_connection_socket(int sock);

    protected:
        std::ostream* get_logstream(log_level_values level)
            { return server::get_logstream(level); }

    public:
        socket_server(int port, const std::string& ip="0.0.0.0");
        socket_server(const std::string& path, mode_t filemode=00770, const std::string& user="", const std::string& group="");

        ~socket_server();

        bool fill_buffer(long long usec_timeout=-1,long long* usec_timeout_remaining=NULL);
        std::set<int> get_sockets_set()
            { return sockets_set; };
};

/** @brief Socket based connection

    This class is used within a socket_server to represent the connection to each client.
*/
class socket_server_connection : public socket_handler, public server_connection
{
    friend class socket_server;

    private:
        socket_server_connection(int _sock, socket_type_value _stype, int _timeout)
           : socket_handler(_sock,_stype), server_connection(_timeout)
           { }

        ~socket_server_connection();

        std::ostream* get_logstream(log_level_values level)
            { return server_connection::get_logstream(level); }

        void real_write(const std::string& data)
            { socket_write(data); }

    public:
        bool fill_buffer(long long usec_timeout=-1,long long* usec_timeout_remaining=NULL);

        virtual void close();
};

}

#endif
