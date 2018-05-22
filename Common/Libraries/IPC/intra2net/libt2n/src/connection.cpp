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

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <netinet/in.h>

#include "connection.hxx"

namespace libt2n
{

connection::~connection()
{
    // Run close() manually since it's a virtual function
    // and we are in the destructor.
    if (!is_closed())
    {
        closed=true;
        do_callbacks(connection_closed);
    }

    do_callbacks(connection_deleted);
}

void connection::close()
{
    if (!is_closed())
    {
        closed=true;
        do_callbacks(connection_closed);
    }
}

/// get the number of bytes being available as next complete packet
connection::packet_size_indicator connection::bytes_available()
{
    // no size information -> no packet
    if (buffer.size() < sizeof(packet_size_indicator))
        return 0;

    packet_size_indicator psize=ntohl(*((packet_size_indicator*)(buffer.data())));

    // enough data for one packet in buffer?
    if (buffer.size() < sizeof(packet_size_indicator)+psize)
        return 0;

    // ok, full packet there
    return psize;
}

/** @brief read a complete data packet from the buffer. The packet is removed from the
            connection buffer.
    @param[out] data the data package
    @retval true if packet found
*/
bool connection::get_packet(std::string& data)
{
    packet_size_indicator psize;

    if ((psize=bytes_available()))
    {
        data.assign(buffer,sizeof(packet_size_indicator),psize);
        buffer.erase(0,sizeof(packet_size_indicator)+psize);
        return true;
    }
    else
        return false;
}

/** @brief get (maybe incomplete) data of the next packet from the buffer. Does not remove the data
            from the connection buffer.
    @param[out] data the data package
    @retval full size of the packet when it will be complete
*/
unsigned int connection::peek_packet(std::string& data)
{
    // no size information -> no packet
    if (buffer.size() < sizeof(packet_size_indicator))
        return 0;

    packet_size_indicator psize=ntohl(*((packet_size_indicator*)(buffer.data())));

    // not the full data available?
    packet_size_indicator currsize=psize;
    if (buffer.size() < currsize+sizeof(packet_size_indicator))
        currsize=buffer.size()-sizeof(packet_size_indicator);

    data.assign(buffer,sizeof(packet_size_indicator),currsize);

    return psize;
}

/// remove all data from buffer that is not a complete packet
void connection::remove_incomplete_packets()
{
    std::string::size_type p=0;
    std::string::size_type end=buffer.size();

    while (p < end)
    {
        // not enough space for size information -> no packet
        if (p+sizeof(packet_size_indicator) > end)
            break;

        packet_size_indicator psize=ntohl(*((packet_size_indicator*)(buffer.data()+p)));

        if (p+sizeof(packet_size_indicator)+psize > end)
        {
            // incomplete packet
            break;
        }
        else
        {
            // move p to where the next packet will start
            p+=sizeof(packet_size_indicator)+psize;
        }
    }

    if (p < end)
    {
        // incomplete packets there, remove them
        buffer.erase(p);
    }
}

/// send a blob to the peer
void connection::write(const std::string& data)
{
    // prepend packet size to data
    packet_size_indicator psize=htonl(data.size());
    std::string send_data(data);
    send_data.insert(0,(char*)&psize,sizeof(packet_size_indicator));

    real_write(send_data);
}

/** @brief add a callback

    @param event event the function will be called at
    @param func functor (see boost function) that will be called

    @note use boost::bind to bind to member functions and parameters like this:
        17 is a fixed parameter that is always added to the call
        c.add_callback(connection_closed,bind(&my_class::func_to_call_back, boost::ref(*this), 17));
*/
void connection::add_callback(callback_event_type event, const boost::function<void ()>& func)
{
    callbacks[event].push_back(func);
}

/** @brief an event has occured, execute the callbacks that are registered for this event

    @param event event type that has occured
*/
void connection::do_callbacks(callback_event_type event)
{
    std::list<boost::function<void ()> >::iterator i,ie=callbacks[event].end();
    for (i=callbacks[event].begin(); i != ie; i++)
        (*i)();
}

/** @brief get the callbacks in place for one event

    @param event event the callbacks should be registered for
    @return std::list of functors (boost::function) with the callbacks

    @note if you want to get the callbacks for all events, loop from 0 to __events_end 
*/
std::list<boost::function<void ()> > connection::get_callback_list(callback_event_type event)
{
    return callbacks[event];
}

/** @brief reopen a already closed connection, removes incomplete packets from the buffer

    @note Only call when the connection is closed.

    @note Justs cares about the data of connection, reconnecting has to be
          done in a derived class.
*/
void connection::reopen()
{
    if (!is_closed())
        throw std::logic_error("connection::reopen() called with connection still open");

    closed=false;

    // incomplete buffer data is worthless with a new connection
    remove_incomplete_packets();

    do_callbacks(new_connection);
}

}
