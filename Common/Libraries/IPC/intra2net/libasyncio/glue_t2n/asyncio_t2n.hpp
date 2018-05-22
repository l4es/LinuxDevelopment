/*
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
/**
 * @file
 * @brief the "glue" between libt2n and AsyncIo framework.
 *
 * contains our own server and client class which should fit into the asnychronous way of "AsyncIo".
 * We use our own classes since the libt2n socket classes are made for synchronous operation
 * which can lead to problems even if we import the connection fd's into "AsyncIo"...
 *
 * @author Reinhard Pfau \<reinhard.pfau@intra2net.com\>
 *
 * @copyright &copy; Copyright 2008 by Intra2net AG
 *
 * @todo support for TCP/IP connections.
 */

#ifndef __CONND_GLUE_T2N_HPP__
#define __CONND_GLUE_T2N_HPP__

#include <string>
#include <server.hxx>
#include <client.hxx>
#include <async_io.hpp>
#include <async_socket.hpp>
#include <pointer_func.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/signal.hpp>


namespace AsyncIo
{

using namespace I2n;

/**
 * @brief specialized version of the libt2n client connection which fits into the AsyncIo framework.
 *
 */
class T2NClientConnection
: public libt2n::client_connection
{
    public:

        T2NClientConnection();
        virtual ~T2NClientConnection();

        bool isOpen();

        bool reopen(bool force= false);


        /*
        ** overloaded methods from libt2n classes:
        */

        virtual void close();

    protected:

        T2NClientConnection( AsyncIo::IOImplementationPtr connection );

        void newDataSlot();

        void eofSlot();

        /*
        ** overloaded methods from t2n classes:
        */

        virtual void real_write(const std::string& data);

        virtual bool fill_buffer(long long usec_timeout=-1,long long* usec_timeout_remaining=NULL);

    protected:

        AsyncIo::IOImplementationPtr m_real_connection;

        bool m_got_new_data;

}; // eo class T2NClientConnection


typedef boost::shared_ptr< T2NClientConnection > T2NClientConnectionPtr;


class T2NServerBase;

typedef boost::shared_ptr< T2NServerBase > T2NServerBasePtr;
typedef boost::weak_ptr< T2NServerBase > T2NServerBaseWeakPtr;

/**
 * @brief specialized version of the libt2n server connection which fits into the AsyncIo framework.
 *
 */
class T2NServerConnection
: public libt2n::server_connection
{
        friend class T2NServerBase;
    public:

        T2NServerConnection();
        virtual ~T2NServerConnection();


        /*
        ** overloaded methods from libt2n classes:
        */

        virtual void close();

    protected:

        T2NServerConnection(
            T2NServerBasePtr server,
            AsyncIo::IOImplementationPtr connection,
            int timeout);

        void newDataSlot();

        void eofSlot();

        bool low_fill_buffer(bool wait, long long usec_timeout=-1, long long* usec_timeout_remaining=NULL);

        /*
        ** overloaded methods from t2n classes:
        */

        virtual void real_write(const std::string& data);

        virtual bool fill_buffer(long long usec_timeout=-1,long long* usec_timeout_remaining=NULL);


    protected:

        AsyncIo::IOImplementationPtr m_real_connection;
        T2NServerBaseWeakPtr m_server_weak_ptr;

        bool m_got_new_data;

}; // eo class T2NServerConnection



/**
 * @brief base server class for handling server ports for libt2n.
 *
 * Needs to be derived for the real type of connection (unix, IPv4, etc.).
 *
 * Does all necessary connection handling and realizes the abstract methods from
 * the libt2n::server class.
 */
class T2NServerBase
: public libt2n::server
, virtual public SharedBase
{
    public:
        virtual ~T2NServerBase();

        bool isOpen();

        /*
        ** overloaded methods from t2n classes:
        */

        virtual bool fill_buffer(long long usec_timeout=-1, long long* timeout_remaining=NULL);

    public:

        boost::signal< void() > m_signal_client_got_new_data;

    protected:

        T2NServerBase( AsyncIo::ServerSocketBaseImplementationPtr server_port);

        void newConnectionSlot(AsyncIo::IOImplementationPtr io_ptr);

        void clientGotNewDataSlot();

        /*
        ** overloaded methods from t2n classes:
        */

        virtual bool fill_connection_buffers(void);


    protected:

        AsyncIo::ServerSocketBaseImplementationPtr m_server_port;
        bool m_new_data_available;

}; // eo T2NServerBase


typedef boost::shared_ptr< T2NServerBase > T2NServerBasePtr;


T2NServerBasePtr createT2NUnixServerPort(const std::string& path, int mode= 0600);

T2NClientConnectionPtr createT2NUnixClientConnection(const std::string& path);


} // eo namespace AsyncIo

#endif
