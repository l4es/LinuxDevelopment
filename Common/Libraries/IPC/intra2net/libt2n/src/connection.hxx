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
#ifndef __LIBT2N_CONNECTION
#define __LIBT2N_CONNECTION

#include <string>
#include <vector>
#include <list>
#include <iostream>

#include <boost/function.hpp>

#include <netinet/in.h>

#include "types.hxx"

namespace libt2n
{

/** @brief a connection between client and server. abstact.
*/
class connection
{
    private:
        bool closed;

        /// vector initialized for all callback-types, all elements in each list will be called
        std::vector<std::list<boost::function<void ()> > > callbacks;

    protected:
        connection()
            : closed(false), callbacks(__events_end)
            { }

        std::string buffer;

        typedef uint32_t packet_size_indicator;

        packet_size_indicator bytes_available();

        virtual void real_write(const std::string& data)=0;

        virtual std::ostream* get_logstream(log_level_values level)=0;

        void do_callbacks(callback_event_type event);

        void reopen(void);
        void remove_incomplete_packets();

    public:
        virtual ~connection();

        /// is this connection closed or not
        bool is_closed()
            { return closed; }

        /// close this connection
        virtual void close();

        /** @brief look for new data and store it in the local buffer
            @param usec_timeout wait until new data is found, max timeout usecs.
                  -1: wait endless
                   0: return instantly
            @param usec_timeout_remaining if non-NULL the function will write the
                  not used time to the given target
            @retval true if new data was found (does not mean that the received data 
                    is a complete packet though)
        */
        virtual bool fill_buffer(long long usec_timeout=-1,long long* usec_timeout_remaining=NULL)=0;

        bool get_packet(std::string& data);

        unsigned int peek_packet(std::string& data);

        /// returns true if a complete data packet is in the buffer. retrieve it with get_packet().
        bool packet_available()
            { return bytes_available(); }

        void write(const std::string& data);

        void add_callback(callback_event_type event, const boost::function<void ()>& func);

        std::list<boost::function<void ()> > get_callback_list(callback_event_type event);
};

}

#endif
