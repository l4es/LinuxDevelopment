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
#ifndef __LIBT2N_SOCKET_HANDLER
#define __LIBT2N_SOCKET_HANDLER

#include <iostream>

#include "types.hxx"

namespace libt2n
{

/** @brief handles socket based communication.
    Don't use directly, use socket_server or socket_client_connection instead.
*/
class socket_handler
{
    private:
        static const unsigned int default_recv_buffer_size=2048;
        static const unsigned int default_write_block_size=4096;
        static const long long default_write_timeout=30000000;

        socket_type_value socket_type;

        bool data_waiting(long long usec_timeout,long long *usec_timeout_remaining=NULL);
        void wait_ready_to_write(int socket, long long write_block_timeout);

    protected:
        int sock;
        unsigned int recv_buffer_size;
        unsigned int write_block_size;
        long long write_timeout;

        socket_handler(int _sock, socket_type_value _socket_type);
        ~socket_handler();

        void set_socket_options(int sock);

        virtual std::ostream* get_logstream(log_level_values level)
            { return NULL; }

        void socket_write(const std::string& data);

        virtual void close();

        bool fill_buffer(std::string& buffer, long long usec_timeout, long long* usec_timeout_remaining=NULL);
        bool fill_buffer(std::string& buffer);

    public:
        /// is this a tcp or unix socket connection
        socket_type_value get_type()
            { return socket_type; }

        int get_socket()
            { return sock; }

        bool is_closed();

        void set_recv_buffer_size(unsigned int new_recv_buffer_size);
        void set_write_block_size(unsigned int new_write_block_size);
        void set_write_timeout(long long new_write_timeout);

        unsigned int get_recv_buffer_size() const { return recv_buffer_size; }
        unsigned int get_write_block_size() const { return write_block_size; }
        long long get_write_timeout() const { return write_timeout; }
};

}

#endif
