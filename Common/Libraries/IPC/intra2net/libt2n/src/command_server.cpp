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
#include <stdexcept>
#include <iostream>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include <boost/bind.hpp>

#include "command_server.hxx"
#include "container.hxx"
#include "log.hxx"

#include <config.h>

using namespace std;

namespace libt2n
{

command_server::command_server(server& _s)
    : s(_s), guard_handle(0)
{
    // register callback
    s.add_callback(new_connection,bind(&command_server::send_hello, boost::ref(*this), _1));
}

/**
 * Destructor
 */
command_server::~command_server()
{
}

/// send a hello message to a new connection
void command_server::send_hello(unsigned int conn_id)
{
    server_connection* sc=s.get_connection(conn_id);

    if (!sc)
        return;         // connection not existing, so no hello

    std::ostringstream hello;

    hello << "T2Nv" << PROTOCOL_VERSION << ';';

    int byteordercheck=1;
    hello.write((char*)&byteordercheck,sizeof(byteordercheck));

    hello << ';';

    sc->write(hello.str());
}

/// handle a command including deserialization and answering
void command_server::handle_packet(const std::string& packet, server_connection* conn)
{
    OBJLOGSTREAM(s,debug,"handling packet from connection " << conn->get_id());

    // deserialize packet
    std::istringstream ifs(packet);
    boost::archive::binary_iarchive ia(ifs);
    command_container ccont;
    result_container res;

    try
    {
        ia >> ccont;
    }
    catch(boost::archive::archive_exception &e)
    {
        std::ostringstream msg;
        msg << "archive_exception while deserializing on server-side, "
               "code " << e.code << " (" << e.what() << ")";
        res.set_exception(new t2n_serialization_error(msg.str()));
    }

    if (!res.has_exception())
    {
        std::ostream* ostr;
        if ((ostr=s.get_logstream(fulldebug))!=NULL)
        {
            (*ostr) << "decoded packet data: " << std::endl;
            boost::archive::xml_oarchive xo(*ostr);
            xo << BOOST_SERIALIZATION_NVP(ccont);
        }

        command* cmd=cast_command(ccont.get_command());

        if (cmd)
        {
            try
            {
                res.set_result((*cmd)());
            }
            catch (t2n_exception &e)
                { res.set_exception(e.clone()); }
        }
        else
        {
            std::ostringstream msg;
            if (ccont.get_command()!=NULL)
                msg << "illegal command of type " << typeid(ccont.get_command()).name() << " called";
            else
                msg << "NULL command called";
            res.set_exception(new t2n_command_error(msg.str()));
        }
    }

    std::ostringstream ofs;
    boost::archive::binary_oarchive oa(ofs);

    try
    {
        oa << res;
    }
    catch(boost::archive::archive_exception &e)
    {
        std::ostringstream msg;
        msg << "archive_exception while serializing on server-side, "
               "code " << e.code << " (" << e.what() << ")";
        res.set_exception(new t2n_serialization_error(msg.str()));
        oa << res;
    }

    std::ostream* ostr;
    if ((ostr=s.get_logstream(fulldebug))!=NULL)
    {
        (*ostr) << "returning result, decoded data: " << std::endl;
        boost::archive::xml_oarchive xo(*ostr);
        xo << BOOST_SERIALIZATION_NVP(res);
    }

    conn->write(ofs.str());
}

/** @brief handle incoming commands
    @param[in,out] usec_timeout wait until new data is found, max timeout usecs.
            -1: wait endless, 0: instant return
    @param[out] usec_timeout_remaining microseconds from the timeout that were not used
*/
void command_server::handle(long long usec_timeout, long long* usec_timeout_remaining)
{
    guard_handle++;

    try
    {
        if (s.fill_buffer(usec_timeout,usec_timeout_remaining))
        {
            std::string packet;
            unsigned int conn_id = 0;

            while (s.get_packet(packet,conn_id))
            {
                server_connection* conn=s.get_connection(conn_id);
                if (!conn)
                    EXCEPTIONSTREAM(error,logic_error,"illegal connection id " << conn_id << " received");
                try
                    { handle_packet(packet,conn); }
                catch (t2n_transfer_error &e)
                {
                    // shut down a connection with transfer errors (usually write errors)
                    conn->close();
                }
            }
        }
    }
    catch(...)
    {
        guard_handle--;
        throw;
    }
    guard_handle--;

    // don't call cleanup on re-entered handle-calls
    if (guard_handle == 0)
        s.cleanup();
}

}
