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

#include <sstream>
#include <stdexcept>
#include <time.h>

#include <boost/bind.hpp>

#include "server.hxx"
#include "log.hxx"

namespace libt2n
{

server_connection::server_connection(int _timeout)
    : connection_id(0)
    , my_server(NULL)
    , connection()
{
    set_timeout(_timeout);
    reset_timeout();
}

/**
 * Destructor
 */
server_connection::~server_connection()
{
}

/// get pointer to logging stream, returns NULL if no logging needed
std::ostream* server_connection::get_logstream(log_level_values level)
{
    if (my_server != NULL)
    {
        std::ostream* ostr=my_server->get_logstream(level);
        if (ostr != NULL)
            (*ostr) << "connection " << get_id() << ": ";
        return ostr;
    }
    else
        return NULL;
}

/// check if timeout is expired, close connection if so
void server_connection::check_timeout()
{
    if (timeout != -1 && last_action_time+timeout < time(NULL))
    {
        LOGSTREAM(debug,"timeout on connection " << connection_id << ", closing");
        this->close();
    }
}

/// reset the timeout, e.g. if something is received
void server_connection::reset_timeout()
{
    last_action_time=time(NULL);
}

/** @brief add a callback to one connection

    @param event event the function will be called at
    @param func functor (see boost::function) that will be called
*/
void server_connection::add_callback(callback_event_type event, const boost::function<void ()>& func)
{
    if (event == new_connection)
        throw std::logic_error("new_connection callback not allowed for server_connections");

    connection::add_callback(event,func);
}

server::server()
    : callbacks(__events_end)
{
    set_default_timeout(30);
    set_logging(NULL,none);
    next_id=1;
}

server::~server()
{
    std::map<unsigned int, server_connection*>::iterator ie=connections.end();
    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; i++)
        delete i->second;

    connections.clear();
}

/**
 * Close all open connections
 */
void server::close()
{
    std::map<unsigned int, server_connection*>::iterator ie=connections.end();
    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; ++i)
        i->second->close();
}

/** @brief add a callback

    @param event event the function will be called at
    @param func functor (see boost::function) that will be called

    @note use boost::bind to bind to member functions like this:
        s.add_callback(new_connection,boost::bind(&my_class::func_to_call_back, boost::ref(*this), _1));
*/
void server::add_callback(callback_event_type event, const boost::function<void (unsigned int)>& func)
{
    callbacks[event].push_back(func);

    // add callback to all existing connections
    if (event != new_connection)
    {
        std::map<unsigned int, server_connection*>::iterator ie=connections.end();
        for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; i++)
            i->second->add_callback(event,boost::bind(func, i->first));
    }
}


/** @brief an event occured, call all server-side callbacks

    @param event event that occured
    @param conn_id connection-id parameter that will be given to the callback-function
*/
void server::do_callbacks(callback_event_type event, unsigned int conn_id)
{
    std::list<boost::function<void (unsigned int)> >::iterator i,ie=callbacks[event].end();
    for (i=callbacks[event].begin(); i != ie; i++)
        (*i)(conn_id);
}

/// add a new connection to the server
unsigned int server::add_connection(server_connection* newconn)
{
    unsigned int cid=next_id++;
    newconn->set_id(cid);
    newconn->set_server(this);
    connections[cid]=newconn;

    // add all callbacks except new_connection
    for(int e=connection_closed; e != __events_end; e++)
    {
        std::list<boost::function<void (unsigned int)> >::iterator i,ie=callbacks[e].end();
        for (i=callbacks[e].begin(); i != ie; i++)
            newconn->add_callback(static_cast<callback_event_type>(e),bind(*i,cid));
    }

    LOGSTREAM(debug,"new connection accepted, id: " << cid);

    do_callbacks(new_connection,cid);

    return cid;
}

/// activate logging to the given stream. everything above the given level is logged.
void server::set_logging(std::ostream *_logstream, log_level_values _log_level)
{
    log_level=_log_level;
    logstream=_logstream;
}

/**
    @brief Gets a connection by id
    
    @param conn_id Connection ID
    
    @retval Pointer to connection object
*/
server_connection* server::get_connection(unsigned int conn_id)
{
    std::map<unsigned int, server_connection*>::iterator p=connections.find(conn_id);
    if (p==connections.end())
        return NULL;
    else
        return p->second;
}

/// check for timeouts, remove closed connections. don't forget to call this from time to time.
void server::cleanup()
{
    std::map<unsigned int, server_connection*>::iterator ie=connections.end();
    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; i++)
        i->second->check_timeout();

    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie;)
    {
        if (i->second->is_closed() && !i->second->packet_available())
        {
            // closed and no usable data in buffer -> remove
            LOGSTREAM(debug,"removing connection " << i->first << " because it is closed and no more data waiting");

            delete i->second;
            connections.erase(i);
            i=connections.begin();
            ie=connections.end();
        }
        else
            i++;
    }
}

/** @brief get a complete data packet from any client. The packet is removed from the
            connection buffer.
    @param[out] data the data package
    @param[out] conn_id the connection id we got this packet from
    @retval true if packet found
*/
bool server::get_packet(std::string& data, unsigned int& conn_id)
{
    // todo: this is somehow unfair: the first connections in the map get checked more
    // often than the others and can thus block them out

    std::map<unsigned int, server_connection*>::iterator ie=connections.end();
    for(std::map<unsigned int, server_connection*>::iterator i=connections.begin(); i != ie; i++)
        if (i->second->get_packet(data))
        {
            LOGSTREAM(debug,"got packet (" << data.size() << " bytes) from connection " << i->first);

            conn_id=i->first;
            return true;
        }

    return false;
}

/// get pointer to logging stream, returns NULL if no logging needed
std::ostream* server::get_logstream(log_level_values level)
{
    if (logstream && log_level >= level)
        return logstream;
    else
        return NULL;
}
};
