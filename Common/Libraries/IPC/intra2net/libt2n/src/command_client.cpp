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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include <boost/bind.hpp>

#include "command_client.hxx"

#include <config.h>

using namespace std;

namespace libt2n
{

/**
 * Constructor
 * @param _c connection for this command. Ownership of the pointer is outside.
 * @param _command_timeout_usec timeout until the command has to be completed
 * @param _hello_timeout_usec timeout until hello has to be received
 */
command_client::command_client(client_connection* _c, long long _command_timeout_usec, long long _hello_timeout_usec)
    : c(_c)
    , constructorException(NULL)
{
    command_timeout_usec=_command_timeout_usec;
    hello_timeout_usec=_hello_timeout_usec;

    // for reconnects
    c->add_callback(new_connection,bind(&command_client::read_hello, boost::ref(*this)));

    // don't expect hello from an always closed connection (like dummy_client_connection)
    if (!is_connection_closed())
    {
        try
        {
            read_hello();
        }
        catch (t2n_communication_error &e)
        {
            c->close();

            // store a copy of the exception that you can find out details about the error later
            constructorException = e.clone();
        }
    }
}

/**
 * Destructor
 */
command_client::~command_client()
{
    if (constructorException)
    {
        delete constructorException;
        constructorException = NULL;
    }
}

/** @brief replace the connection currently in use with a new one
    @param _c pointer to the new connection

    @note the old connection must still be valid when this method is called,
          it can safely be deleted after this method returned

    @note all callbacks registered on the old connection will be copied over
          to the new one
*/
void command_client::replace_connection(client_connection* _c)
{
    // copy all callbacks registered on the old connection
    for(callback_event_type e=static_cast<callback_event_type>(0);
        e < __events_end;
        e=static_cast<callback_event_type>(static_cast<int>(e)+1))
    {
        list<boost::function<void ()> > evcb=c->get_callback_list(e);

        for (list<boost::function<void ()> >::iterator i=evcb.begin(); i != evcb.end(); i++)
            _c->add_callback(e,*i);
    }

    // replace the connection
    c=_c;

    read_hello();
}

/** @brief return a complete packet
    @param usec_timeout maximum microseconds to wait until the packet is complete
    @retval packet data as std::string

    @note throws a t2n_transfer_error if the timeout is exceeded
*/
std::string command_client::read_packet(const long long &usec_timeout)
{
    string resultpacket;
    bool got_packet=false;
    long long my_timeout=usec_timeout;
    while(!(got_packet=c->get_packet(resultpacket)) && my_timeout > 0  && !c->is_closed())
        c->fill_buffer(my_timeout,&my_timeout);

    if (!got_packet)
        throw t2n_transfer_error("timeout exceeded");

    return resultpacket;
}

/** @brief read and check the hello message at the beginning of a connection

    @note throws exceptions if something went wrong
*/
void command_client::read_hello()
{
    string resultpacket;
    bool got_packet=false;
    long long my_timeout=hello_timeout_usec;
    while(!(got_packet=c->get_packet(resultpacket)) && my_timeout > 0  && !c->is_closed())
    {
        c->fill_buffer(my_timeout,&my_timeout);

        c->peek_packet(resultpacket);
        check_hello(resultpacket);           // will throw before timeout if wrong data received
    }

    if (!got_packet)
        throw t2n_transfer_error("timeout exceeded");

    if (!check_hello(resultpacket))
        throw t2n_version_mismatch("illegal hello received (incomplete): "+resultpacket);
}

/** @brief check if a hello message is valid
    @param hellostr std::string with the message to check
    @retval true if the hello is good and complete

    @note you can check incomplete hellos. you will get a false return value
          but no exception. throws exceptions as soon as something is wrong.
*/
bool command_client::check_hello(const std::string& hellostr)
{
    istringstream hello(hellostr);

    char chk;

    if (hello.read(&chk,1))
    {
        if (chk != 'T')
            throw t2n_version_mismatch("illegal hello received (T2N)");
    }
    else
        return false;

    if (hello.read(&chk,1))
    {
        if (chk != '2')
            throw t2n_version_mismatch("illegal hello received (T2N)");
    }
    else
        return false;

    if (hello.read(&chk,1))
    {
        if (chk != 'N')
            throw t2n_version_mismatch("illegal hello received (T2N)");
    }
    else
        return false;

    if (hello.read(&chk,1))
    {
        if (chk != 'v')
            throw t2n_version_mismatch("illegal hello received (T2N)");
    }
    else
        return false;

    int prot_version;
    if (hello >> prot_version)
    {
        if (prot_version != PROTOCOL_VERSION)
            throw t2n_version_mismatch("not compatible with the server protocol version");
    }
    else
        return false;

    if (hello.read(&chk,1))
    {
        if (chk != ';')
            throw t2n_version_mismatch("illegal hello received (1. ;)");
    }
    else
        return false;

    unsigned int hbo;
    if (hello.read((char*)&hbo,sizeof(hbo)))
    {
        if (hbo != 1)
            throw t2n_version_mismatch("host byte order not matching");
    }
    else
        return false;

    if (hello.read(&chk,1))
    {
        if (chk != ';')
            throw t2n_version_mismatch("illegal hello received (2. ;)");
    }
    else
        return false;

    return true;
}

/** @brief send a command to the server and store the result
    @param cmd pointer to a command-object
    @param[out] res result container to store the result in

    @note you can check incomplete hellos. you will get a false return value
          but no exception. throws exceptions as soon as something is wrong.
*/
void command_client::send_command(command* cmd, result_container &res)
{
    ostringstream ofs;
    command_container cc(cmd);
    boost::archive::binary_oarchive oa(ofs);

    if (is_connection_closed())
        throw t2n_transfer_error("connection to server is closed");

    try
    {
        oa << cc;
    }
    catch(boost::archive::archive_exception &e)
    {
        ostringstream msg;
        msg << "archive_exception while serializing on client-side, code " << e.code << " (" << e.what() << ")";
        throw t2n_serialization_error(msg.str());
    }

    std::ostream* ostr;
    if ((ostr=c->get_logstream(fulldebug))!=NULL)
    {
        (*ostr) << "sending command, decoded data: " << std::endl;
        boost::archive::xml_oarchive xo(*ostr);
        xo << BOOST_SERIALIZATION_NVP(cc);
   }

    c->write(ofs.str());

    istringstream ifs(read_packet(command_timeout_usec));
    boost::archive::binary_iarchive ia(ifs);

    try
    {
        ia >> res;
    }
    catch(boost::archive::archive_exception &e)
    {
        ostringstream msg;
        msg << "archive_exception while deserializing on client-side, code " << e.code << " (" << e.what() << ")";
        throw t2n_serialization_error(msg.str());
    }

    if ((ostr=c->get_logstream(fulldebug))!=NULL)
    {
        (*ostr) << "received result, decoded data: " << std::endl;
        boost::archive::xml_oarchive xo(*ostr);
        xo << BOOST_SERIALIZATION_NVP(res);
    }
}

}
