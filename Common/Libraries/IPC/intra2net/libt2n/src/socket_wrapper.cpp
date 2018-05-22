/*
Copyright (C) 2008 by Intra2net AG - Gerd v. Egidy

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

#include <functional>
#include <string>

#include <socket_wrapper.hxx>

namespace libt2n
{

/// set logging for coming and current connections
void BasicSocketWrapper::set_logging(std::ostream *_logstream, log_level_values _log_level)
{
    ConnectionWrapper::set_logging(_logstream,_log_level);

    if (connection_established())
        get_connection()->set_logging(_logstream,_log_level);
}


/// return active connection, create new tcp or unix connection if not existing
client_connection* BasicSocketWrapper::get_connection(void)
{
    if (c.get() == NULL)
    {
        if (socket_type == tcp_s)
            c=std::auto_ptr<socket_client_connection>
                (new socket_client_connection(port,server,connect_timeout_usec,max_retries,logstream,log_level));
        else if (socket_type == unix_s)
            c=std::auto_ptr<socket_client_connection>
                (new socket_client_connection(path,connect_timeout_usec,max_retries,logstream,log_level));
    }

    return c.get();
}

/// try to reconnect max_retries time if we encounter a t2n_communication_error 
/// during execution of the command
bool ReconnectSocketWrapper::handle(command_client* stubBase, boost::function< void() > f)
{
    int tries=0;

    while(true)
    {
        try
        {
            // we always reconnect if something went wrong before:
            // makes sure the buffers are clean of half-sent packets etc.
            if (tries > 0)
                c->reconnect();

            f();
            // we were successful
            return true;
        }
        catch(t2n_connect_error &e)
        {
            // reconnect already tries max_tries times to reconnect: we are done if that failed
            throw(e);
        }
        catch(t2n_communication_error &e)
        {
            // aborts the loop with an exception after max_retries iterations
            // retries means that the first try doesn't count: use >
            if (tries > max_retries)
                throw(e);

            // otherwise ignore the exception and reconnect in the next iteration
        }

        tries++;
    }

    return false;
}

/// return active connection, return a dummy-connection if we can't establish one
client_connection* ReconnectIgnoreFailureSocketWrapper::get_connection(void)
{
    client_connection* tmp=BasicSocketWrapper::get_connection();

    if (tmp->is_closed())
    {
        // throw away the closed connection...
        c.reset();
        // ...return the dummy one instead
        tmp=&dc;
    }

    return tmp;
}

/// try to execute the command, may ignore the command if server not available
bool ReconnectIgnoreFailureSocketWrapper::handle(command_client* stubBase, boost::function< void() > f)
{
    if (!connection_established())
    {
        // dummy connection is in place: try to establish a real one
        client_connection* tmp=get_connection();

        if (tmp != &dc)
        {
            // success! we've got a real connection
            stubBase->replace_connection(tmp);
        }
    }

    // only try to handle the call if we've got a real connection
    if (connection_established())
    {
        try
        {
            ReconnectSocketWrapper::handle(stubBase,f);
            return true;
        }
        catch(t2n_communication_error &e)
        {
            // ignore
        }
    }

    return false;
}

}
