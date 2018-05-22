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

#ifndef __LIBT2N_SOCKET_WRAPPER
#define __LIBT2N_SOCKET_WRAPPER

#include <functional>
#include <string>

#include <client.hxx>
#include <command_client.hxx>
#include <types.hxx>
#include <client_wrapper.hxx>
#include <socket_client.hxx>

namespace libt2n
{

/** @brief a basic implementation of ConnectionWrapper

    This is a basic version of a ConnectionWrapper which does not do any fancy
    error handling or anything, it justs executes the regular calls. Use this
    wrapper if you only want to use the singleton-feature of T2nSingletonWrapper.
*/
class BasicSocketWrapper : public ConnectionWrapper
{
    protected:
        socket_type_value socket_type;

        std::string path;
        std::string server;
        int port;

        long long connect_timeout_usec;
        int max_retries;

        std::auto_ptr<socket_client_connection> c;

        // TODO: Mark object as non-copyable as it contains an auto_ptr.
        //       This will make sure nobody every tries to put this in a STL container

    public:
        BasicSocketWrapper(int _port, const std::string& _server="127.0.0.1", 
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : ConnectionWrapper(),
              socket_type(tcp_s),
              server(_server),
              port(_port),
              connect_timeout_usec(_connect_timeout_usec),
              max_retries(_max_retries)
            { }

        BasicSocketWrapper(const std::string& _path,
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : ConnectionWrapper(),
              socket_type(unix_s),
              path(_path),
              connect_timeout_usec(_connect_timeout_usec),
              max_retries(_max_retries)
            { }

        client_connection* get_connection(void);

        bool connection_established(void)
            { return (c.get() != NULL); }

        void set_logging(std::ostream *_logstream, log_level_values _log_level);
};

/** @brief a wrapper implementing reconnect-then-throw

    This ConnectionWrapper tries to reconnect to the server if something with the connection
    goes wrong. If even reconnecting max_retries times does not help, an exception is thrown.
*/
class ReconnectSocketWrapper : public BasicSocketWrapper
{
    public:
        ReconnectSocketWrapper(int _port, const std::string& _server="127.0.0.1", 
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : BasicSocketWrapper(_port,_server,_connect_timeout_usec,_max_retries)
            { }

        ReconnectSocketWrapper(const std::string& _path,
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : BasicSocketWrapper(_path,_connect_timeout_usec,_max_retries)
            { }

        bool handle(command_client* stubBase, boost::function< void() > f);
};

/// a placeholder-client_connection which is closed all the time
class dummy_client_connection : public client_connection
{
    private:
        void real_write(const std::string& data)
            { }

    public:
        dummy_client_connection()
            : client_connection()
            { close(); }

        bool fill_buffer(long long usec_timeout=-1, long long *usec_timeout_remaining=NULL)
            { return false; }
};

/** @brief a wrapper implementing reconnect-then-ignore

    This ConnectionWrapper tries to reconnect to the server if something with the connection
    goes wrong. If even reconnecting max_retries times does not help, the complete t2n-call is
    ignored. The return value of the call will be created with the default constructor.
*/
class ReconnectIgnoreFailureSocketWrapper : public ReconnectSocketWrapper
{
    private:
        dummy_client_connection dc;

    public:
        ReconnectIgnoreFailureSocketWrapper(int _port, const std::string& _server="127.0.0.1", 
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : ReconnectSocketWrapper(_port,_server,_connect_timeout_usec,_max_retries)
            { }

        ReconnectIgnoreFailureSocketWrapper(const std::string& _path,
            long long _connect_timeout_usec=socket_client_connection::connect_timeout_usec_default, 
            int _max_retries=socket_client_connection::max_retries_default)
            : ReconnectSocketWrapper(_path,_connect_timeout_usec,_max_retries)
            { }

        client_connection* get_connection(void);
        bool handle(command_client* stubBase, boost::function< void() > f);
};

}

#endif
