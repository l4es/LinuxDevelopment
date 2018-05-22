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
#ifndef __LIBT2N_SERVER
#define __LIBT2N_SERVER

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <list>

#include <boost/function.hpp>

#include "connection.hxx"
#include "types.hxx"

namespace libt2n
{

class server;

/**
    @brief connection on a server

    on a server every connection to a client is represented as server_connection.
    a server_connection is abstract, derived classes like socket_server_connection are used.
*/
class server_connection : public connection
{
    friend class server;

    private:
        int timeout;
        int last_action_time;
        unsigned int connection_id;

        void set_server(server* _my_server)
            { my_server=_my_server; }

        void set_id(unsigned int _connection_id)
            { connection_id=_connection_id; }

    protected:
        server *my_server;

        server_connection(int _timeout);
        virtual ~server_connection();

        std::ostream* get_logstream(log_level_values level);

    public:
        void check_timeout();
        void reset_timeout();
        void set_timeout(int _timeout)
            { timeout=_timeout; }

        /// get the id of this connection within the server object
        unsigned int get_id()
            { return connection_id; }

        void add_callback(callback_event_type event, const boost::function<void ()>& func);
};

/**
    @brief server base class

    constitutes a server. is abstract, use derived classes like socket_server.
*/
class server
{
    private:
        int default_timeout;
        log_level_values log_level;
        std::ostream *logstream;

        /// vector initialized for all callback-types, all elements in each list will be called
        std::vector<std::list<boost::function<void (unsigned int)> > > callbacks;

        unsigned int next_id;

    protected:
        std::map<unsigned int, server_connection*> connections;

        server();

        virtual bool fill_connection_buffers(void)=0;

        unsigned int add_connection(server_connection* newconn);

        void do_callbacks(callback_event_type event, unsigned int conn_id);

    public:
        virtual ~server();

        /// set the default timeout for new client connections
        void set_default_timeout(int _default_timeout)
            { default_timeout=_default_timeout; }

        /// get the current default timeout for client connections
        int get_default_timeout(void)
            { return default_timeout; }

        void set_logging(std::ostream *_logstream, log_level_values _log_level);

        server_connection* get_connection(unsigned int conn_id);

        void add_callback(callback_event_type event, const boost::function<void (unsigned int)>& func);

        /** @brief look for new data and store it in the local buffer
            @param usec_timeout wait until new data is found, max timeout usecs.
                  -1: wait endless
                   0: return instantly
            @param usec_timeout_remaining if non-NULL the function will write the
                  not used time to the given target
            @retval true if new data was found (does not mean that the received data 
                    is a complete packet though)
        */
        virtual bool fill_buffer(long long usec_timeout=-1, long long* usec_timeout_remaining=NULL)=0;

        void close();

        void cleanup();

        /** @brief get a complete data packet from any client. The packet is removed from the
                   connection buffer.
            @param[out] data the data package
            @retval true if packet found
        */
        bool get_packet(std::string& data)
            { unsigned int x; return get_packet(data,x); }

        bool get_packet(std::string& data, unsigned int& conn_id);

        std::ostream* get_logstream(log_level_values level);
};

}

#endif
