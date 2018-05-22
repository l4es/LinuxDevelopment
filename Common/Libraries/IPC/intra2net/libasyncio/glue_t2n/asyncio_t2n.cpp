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
 *
 * @author Reinhard Pfau \<reinhard.pfau@intra2net.com\>
 *
 * @copyright &copy; Copyright 2008 by Intra2net AG
 */
#include "asyncio_t2n.hpp"

#include <iostream>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>
#include <boost/signal.hpp>
#include <climits>
#include <logfunc.hpp>
#include <tracefunc.hpp>
#include <log_macros.hpp>


namespace AsyncIo
{

using namespace Utils;

namespace
{


Logger::PartLogger& module_logger()
{
    static Logger::PartLogger _module_logger(HERE);
    return _module_logger;
} // eo module_logger();



/**
 * @brief a class with some methods we like to have on our io classes.
 *
 * This class is to be used as second base in a wrapper class and needs it's methods to
 * be redefined for "the real thing".
 */
class IOExportWrapperBase
{
    public:

        IOExportWrapperBase()
        {
        }

        virtual ~IOExportWrapperBase()
        {
        }

        virtual void sendData(const std::string& data)
        {
        }


        virtual std::string receiveData()
        {
            return std::string();
        }

        virtual boost::signals::connection connectEof( const boost::function< void() >& func )
        {
            return boost::signals::connection();
        }

        virtual boost::signals::connection connectRead( const boost::function< void() >& func )
        {
            return boost::signals::connection();
        }

}; // eo class IOExportWrapperBase


typedef boost::shared_ptr< IOExportWrapperBase > IOExportWrapperBasePtr;


/**
 * @brief IO wrapper template.
 * @tparam IOClass a type based on AsyncIo::IOImplementation
 *
 * The type is used as a public base for the resulting class; the second public base is our
 * helper with the additional methods we need internally and which we (finally) define here.
 */
template<
    class IOClass
>
class IOExportWrapper
: public IOClass
, public IOExportWrapperBase
{
        BOOST_STATIC_ASSERT(( boost::is_base_of< IOImplementation,IOClass >::value ));

    public:
        IOExportWrapper()
        {
        }

        template<
            typename Arg1
        >
        IOExportWrapper(Arg1 arg1)
        : IOClass(arg1)
        {}


        template<
            typename Arg1, typename Arg2
        >
        IOExportWrapper(Arg1 arg1, Arg2 arg2)
        : IOClass(arg1,arg2)
        {}


        template<
            typename Arg1, typename Arg2, typename Arg3
        >
        IOExportWrapper(Arg1 arg1, Arg2 arg2, Arg3 arg3)
        : IOClass(arg1,arg2,arg3)
        {}


        template<
            typename Arg1, typename Arg2, typename Arg3, typename Arg4
        >
        IOExportWrapper(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
        : IOClass(arg1,arg2,arg3,arg4)
        {}


        template<
            typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5
        >
        IOExportWrapper(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
        : IOClass(arg1,arg2,arg3,arg4,arg5)
        {}


        /**
         * @brief exposed funtion for sending data.
         * @param data the chunk to be send.
         */
        virtual void sendData(const std::string& data)
        {
            IOClass::lowSend(data);
        }

        /**
         * @brief returns the new received data.
         * @return the receievd data.
         *
         * Clears the receive buffer.
         */
        virtual std::string receiveData()
        {
            std::string result;
            result.swap(IOClass::m_input_buffer);
            return result;
        }

        /**
         * @brief exposed connect to EOF signal.
         * @param func the function which should be connected to the eof signal.
         * @return signal connection handle.
         */
        virtual boost::signals::connection connectEof( const boost::function< void() >& func )
        {
            return IOClass::m_signal_eof.connect(func);
        }

        /**
         * @brief exposed connect to "read" signal.
         * @param func the function which should be connected to the "read" signal.
         * @return signal connection handle.
         */
        virtual boost::signals::connection connectRead( const boost::function< void() >& func )
        {
            return IOClass::m_signal_read.connect(func);
        }

    protected:

}; // eo class IOExportWrapper


/*
** specialized versions of io classes:
*/

/**
 * @brief enhanced unix domain socket class with reconnect feature.
 *
 * Used for t2n client connections.
 */
class T2nUnixIOSocket
: public AsyncIo::UnixIOSocket
{
        typedef AsyncIo::UnixIOSocket inherited;
    public:
        T2nUnixIOSocket( const std::string& path );

        virtual void close(AsyncIo::Direction direction = AsyncIo::Direction::both);

        bool reopen(bool force= false);

    protected:

        virtual void doRead();


    protected:

        bool m_in_do_read;
        bool m_may_reconnect;

}; // T2nUnixIOSocket


T2nUnixIOSocket::T2nUnixIOSocket(const std::string& path)
: inherited(path)
, m_in_do_read(false)
, m_may_reconnect(false)
{
} // eo T2nUnixIOSocket::T2nUnixIOSocket(const std::string&)


void T2nUnixIOSocket::close(AsyncIo::Direction direction)
{
    bool was_open= opened();
    inherited::close(direction);
    if (m_in_do_read and not opened())
    {
        m_may_reconnect= was_open;
    }
} // eo T2nUnixIOSocket::close(AsyncIo::Direction)


bool T2nUnixIOSocket::reopen(bool force)
{
    if (m_path.empty())
    {
        return false;
    }
    if (m_may_reconnect || force)
    {
        return inherited::open( m_path );
    }
    return false;
} // eo T2nUnixIOSocket::reopen()


void T2nUnixIOSocket::doRead()
{
    m_in_do_read= true;
    try
    {
        inherited::doRead();
    }
    catch (...)
    {
        m_in_do_read= false;
        throw;
    }
    m_in_do_read= false;
} // eo T2nUnixIOSocket::doRead()


/**
 * @brief server class for libt2n using unix domain sockets.
 *
 * well, it's enough to provide an appropriate constructor.
 * (did i mention that templates are really cool stuff? :-) )
 */
class T2NUnixServer
: public T2NServerBase
{
    public:

        T2NUnixServer(const std::string& path, int mode=0600)
        : T2NServerBase( ServerSocketBaseImplementationPtr(
            new UnixServerSocket<
                IOExportWrapper< UnixIOSocket >
            >(path, mode)
        ) )
        {
        } // eo T2NServerBase

}; // eo T2NUnixServer



class RealT2NClientConnection
: public T2NClientConnection
{
    public:
        RealT2NClientConnection( AsyncIo::IOImplementationPtr connection )
        : T2NClientConnection(connection)
        {
        }
}; // eo class T2NClient

} // eo namespace <anonymous>



/*
** implementation of T2NClientConnection
*/


T2NClientConnection::T2NClientConnection(
    IOImplementationPtr connection
)
: libt2n::client_connection()
, m_real_connection(connection)
, m_got_new_data(false)
{
    SCOPETRACKER();
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(connection);
    if (!ptr)
    {
        module_logger().error(HERE) << "illegal pointer passed";
        close();
        return;
    }
    if (!connection->opened())
    {
        module_logger().warning(HERE) << "connection not open, either failed or already closed";
        close();
        return;
    }

    ptr->connectRead( boost::bind(&T2NClientConnection::newDataSlot, this) );
    ptr->connectEof( boost::bind(&T2NClientConnection::eofSlot, this) );

} // eo T2NClientConnection::T2NClientConnection(IOImplementationPtr)


T2NClientConnection::~T2NClientConnection()
{
    SCOPETRACKER();
} // eo T2NClientConnection::~T2NClientConnection


/**
 * @brief returns if the connection is open.
 * @return @a true if the connection is open.
 */
bool T2NClientConnection::isOpen()
{
    return m_real_connection and m_real_connection->opened();
} // eo T2NClientConnection::isOpen()


/**
 * @brief try to reopen a connection.
 * @return @a true if the connection was reopened.
 */
bool T2NClientConnection::reopen(bool force)
{
    if (not m_real_connection)
    {
        return false;
    }
    boost::shared_ptr< T2nUnixIOSocket > t2n_socket=
        boost::dynamic_pointer_cast< T2nUnixIOSocket >(m_real_connection);
    if (t2n_socket)
    {
        return t2n_socket->reopen(force);
    }
    return false;
} // eo T2NClientConnection::reopen()


/**
 * @brief closes the connection.
 *
 * This closes the underlying IO connection and calls libt2n::server_connection::close() to
 * mark the connection as closed for libt2n.
 */
void T2NClientConnection::close()
{
    SCOPETRACKER();
    if (m_real_connection)
    {
        m_real_connection->close();
        m_real_connection.reset();
    }
    libt2n::client_connection::close();
} // eo T2NClientConnection::close()


/**
 * @brief sends a raw data chunk on the connection.
 *
 * @param data the (raw) data chunk which should be sended.
 */
void T2NClientConnection::real_write(const std::string& data)
{
    SCOPETRACKER();
    if (is_closed())
    {
        module_logger().warning(HERE) << "attempt to write data on closed connection";
        return;
    }
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
    if (!ptr)
    {
        // should never happen...
        module_logger().error(HERE)<< "illegal io pointer";
        close();
        //TODO: throw an error?!
        NOT_REACHED();
        return;
    }
    ptr->sendData(data);
} // eo T2NClientConnection::real_write(const std::string)


/**
 * @brief called to fill the connection buffer.
 *
 * Since this class uses the asnychronous AsyncIo framework, new data may already be read when
 * this method is called.
 *
 * @param usec_timeout 
 * @param usec_timeout_remaining 
 * @return @a true if new data is available.
 */
bool T2NClientConnection::fill_buffer(long long usec_timeout,long long* usec_timeout_remaining)
{
    SCOPETRACKER();
    if (is_closed())
    {
        module_logger().debug(HERE) << "fill_buffer() called on closed connection";
        return false;
    }
    AsyncIo::MilliTime t0,t1;
    AsyncIo::get_current_monotonic_time(t0);
    if (!m_got_new_data)
    {
        IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
        if (!ptr)
        {
            module_logger().error(HERE) << "illegal io pointer";
            close();
            return false;
        }
        // try to fetch data (call the backend)
        int timeout= 0;

        if (usec_timeout<0)
        {
            timeout= -1;
        }
        else if (usec_timeout > 0)
        {
            long long msec_timeout= (usec_timeout + 500)/1000;

            if (msec_timeout >= INT_MAX)
            {
                timeout= INT_MAX;
            }
            else
            {
                timeout= (int)msec_timeout;
            }
        }
        Backend::getBackend()->doOneStep( timeout );
    }
    AsyncIo::get_current_monotonic_time(t1);
    if (usec_timeout_remaining)
    {
        long long delta= ((long long)(t1 - t0).get_milliseconds())* 1000L;
        *usec_timeout_remaining= (usec_timeout > delta ) ? (usec_timeout - delta) : 0L;
        module_logger().debug() << "timeout: " << usec_timeout << " -> " << *usec_timeout_remaining;
    }
    if (m_got_new_data)
    {
        m_got_new_data= false;
        return true;
    }
    return false;
} // eo T2NClientConnection::fill_buffer(long long,long long*)


/**
 * @brief called when new data arrived on this connection.
 *
 * reads the new data from the underlying IO object and stores it in the connection buffer.
 * Also remembers (in the bool member var @a m_got_new_data) that new data was received.
 */
void T2NClientConnection::newDataSlot()
{
    SCOPETRACKER();
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
    if (!ptr)
    {
        //TODO: throw an error?!
        NOT_REACHED();
        return;
    }
    std::string new_data= ptr->receiveData();
    module_logger().debug() << "got " << new_data.size() << " bytes of new data";
    buffer+= new_data;
    m_got_new_data= true;
} // eo T2NClientConnection::newDataSlot()


/**
 * @brief called when an EOF was detected by the underlying IO object (i.e. the connection
 * was closed by the peer side).
 *
 * Calls close().
 */
void T2NClientConnection::eofSlot()
{
    SCOPETRACKER();
    close();
} // eo T2NClientConnection::eofSlot()


/*
** implementation of T2NServerConnection
*/


T2NServerConnection::T2NServerConnection(
    T2NServerBasePtr server,
    IOImplementationPtr connection,
    int timeout
)
: libt2n::server_connection(timeout)
, m_real_connection(connection)
, m_server_weak_ptr(server)
, m_got_new_data(false)
{
    SCOPETRACKER();
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(connection);
    if (!ptr)
    {
        module_logger().error(HERE) << "illegal pointer passed";
        close();
        return;
    }
    if (!connection->opened())
    {
        module_logger().warning(HERE) << "connection not open, either failed or already closed";
        close();
        return;
    }

    ptr->connectRead( boost::bind(&T2NServerConnection::newDataSlot, this) );
    ptr->connectEof( boost::bind(&T2NServerConnection::eofSlot, this) );

} // eo T2NServerConnection::T2NServerConnection(IOImplementationPtr)


T2NServerConnection::~T2NServerConnection()
{
    SCOPETRACKER();
} // eo T2NServerConnection::~T2NServerConnection


/**
 * @brief closes the connection.
 *
 * This closes the underlying IO connection and calls libt2n::server_connection::close() to
 * mark the connection as closed for libt2n.
 */
void T2NServerConnection::close()
{
    SCOPETRACKER();
    if (m_real_connection)
    {
        m_real_connection->close();
        m_real_connection.reset();
    }
    libt2n::server_connection::close();
} // eo T2NServerConnection::close()


/**
 * @brief sends a raw data chunk on the connection.
 *
 * @param data the (raw) data chunk which should be sended.
 */
void T2NServerConnection::real_write(const std::string& data)
{
    SCOPETRACKER();
    if (is_closed())
    {
        module_logger().warning(HERE) << "attempt to write data on closed connection";
        return;
    }
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
    if (!ptr)
    {
        // should never happen...
        module_logger().error(HERE)<< "illegal io pointer";
        close();
        //TODO: throw an error?!
        NOT_REACHED();
        return;
    }
    module_logger().debug() << "send " << data.size() << " bytes of data";
    ptr->sendData(data);
} // eo T2NServerConnection::real_write(const std::string)


/**
 * @brief called to fill the connection buffer.
 *
 * Since this class uses the asnychronous AsyncIo framework, new data may already be read when
 * this method is called.
 *
 * @param wait determines if we need to wait; if @a false it is just checked if new data
 *  was received, but no backend cycle is executed.
 * @param usec_timeout 
 * @param usec_timeout_remaining 
 * @return @a true if new data is available.
 */
bool T2NServerConnection::low_fill_buffer(bool wait, long long usec_timeout, long long* usec_timeout_remaining)
{
    SCOPETRACKER();
    if (is_closed())
    {
        module_logger().debug(HERE) << "fill_buffer() called on closed connection";
        return false;
    }
    if (not m_got_new_data and wait)
    {
        AsyncIo::MilliTime t0,t1;
        AsyncIo::get_current_monotonic_time(t0);
        IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
        if (!ptr)
        {
            module_logger().error(HERE) << "illegal io pointer";
            close();
            return false;
        }
        // try to fetch data (call the backend)
        int timeout= 0;

        if (usec_timeout<0)
        {
            timeout= -1;
        }
        else if (usec_timeout > 0)
        {
            long long msec_timeout= (usec_timeout + 500)/1000;

            if (msec_timeout >= INT_MAX)
            {
                timeout= INT_MAX;
            }
            else
            {
                timeout= (int)msec_timeout;
            }
        }
        Backend::getBackend()->doOneStep( timeout );
        AsyncIo::get_current_monotonic_time(t1);
        if (usec_timeout_remaining)
        {
            long long delta= ((long long)(t1 - t0).get_milliseconds())* 1000L;
            *usec_timeout_remaining= (usec_timeout > delta ) ? (usec_timeout - delta) : 0L;
        }
    }
    else
    {
        if (usec_timeout_remaining)
        {
            *usec_timeout_remaining= usec_timeout;
        }
    }
    if (m_got_new_data)
    {
        m_got_new_data= false;
        return true;
    }
    return false;
} // eo T2NServerConnection::low_fill_buffer(bool,long long,long long*)


/**
 * @brief called to fill the connection buffer.
 *
 * Since this class uses the asnychronous AsyncIo framework, new data may already be read when
 * this method is called.
 *
 * @param usec_timeout 
 * @param usec_timeout_remaining 
 * @return @a true if new data is available.
 */
bool T2NServerConnection::fill_buffer(long long usec_timeout,long long* usec_timeout_remaining)
{
    return low_fill_buffer(true, usec_timeout, usec_timeout_remaining);
} // eo T2NServerConnection::fill_buffer(long long,long long*)


/**
 * @brief called when new data arrived on this connection.
 *
 * reads the new data from the underlying IO object and stores it in the connection buffer.
 * Also remembers (in the bool member var @a m_got_new_data that new data was received.
 */
void T2NServerConnection::newDataSlot()
{
    SCOPETRACKER();
    IOExportWrapperBasePtr ptr = boost::dynamic_pointer_cast< IOExportWrapperBase >(m_real_connection);
    if (!ptr)
    {
        //TODO:throw an error?!
        NOT_REACHED();
        return;
    }
    std::string new_data= ptr->receiveData();
    buffer+= new_data;
    module_logger().debug() << "got " << new_data.size() << " bytes of new data";
    m_got_new_data= true;
    reset_timeout();

    T2NServerBasePtr server =m_server_weak_ptr.lock();
    if (server)
    {
        server->m_signal_client_got_new_data();
    }
} // eo T2NServerConnection::newDataSlot()


/**
 * @brief called when an EOF was detected by the underlying IO object (i.e. the connection
 * was closed by the peer side).
 *
 * Calls close().
 */
void T2NServerConnection::eofSlot()
{
    SCOPETRACKER();
    close();
} // eo T2NServerConnection::eofSlot()



/*
** implementation of T2NServerBase
*/


/**
 * @brief constructs a libt2n server object.
 *
 * @param server_port shared pointer to a (AsyncIo) port server object which
 * is used as underlying port handler.
 */
T2NServerBase::T2NServerBase( ServerSocketBaseImplementationPtr server_port)
: m_server_port(server_port)
, m_new_data_available(false)
{
    SCOPETRACKER();
    // register our callback for new incoming conncetions.
    server_port->setNewConnectionBaseCallback(
        boost::bind(&T2NServerBase::newConnectionSlot, this, _1)
    );
    m_signal_client_got_new_data.connect
    (
        boost::bind(&T2NServerBase::clientGotNewDataSlot, this)
    );
} // eo T2NServerBase::T2NServerBase(ServerSocketBaseImplementationPtr)


/**
 * @brief destructor.
 *
 */
T2NServerBase::~T2NServerBase()
{
    SCOPETRACKER();
} // eo T2NServerBase::~T2NServerBase()


/**
 * @brief returns wether the server port is opened.
 *
 * @return @a true iff the server port is open.
 */
bool T2NServerBase::isOpen()
{
    return (m_server_port && m_server_port->opened());
} // eo T2NServerBase



/**
 * @brief callback for the port server object when a new connection is established.
 *
 * @param io_ptr the (shared) pointer to the new connection.
 */
void T2NServerBase::newConnectionSlot(IOImplementationPtr io_ptr)
{
    SCOPETRACKER();
    add_connection( new T2NServerConnection( get_ptr_as< T2NServerBase >(), io_ptr, get_default_timeout() ) );
} // eo T2NServerBase::newConnectionSlot(IOImplementationPtr)


/**
 * @brief callback for "new data available" signal
 */
void T2NServerBase::clientGotNewDataSlot()
{
    m_new_data_available= true;
} // eo T2NServerBase::clientGotNewDataSlot()


/**
 * @brief try to fill the buffers of the managed connections.
 *
 * will be called by T2NServerBase::fill_buffer().
 *
 * @return @a true if at least one connection buffer has new data.
 */
bool T2NServerBase::fill_connection_buffers()
{
    SCOPETRACKER();
    Backend::getBackend()->doOneStep(0);
    bool result= false;
    for(std::map<unsigned int, libt2n::server_connection*>::iterator it=connections.begin();
        it != connections.end();
        ++it)
    {
        T2NServerConnection *conn = dynamic_cast<T2NServerConnection*>(it->second);
        if (!conn)
        {
            if (it->second)
            {
                // react somehow if (it->second) is not NULL...
                module_logger().error(HERE) << "illegal connection pointer";
                it->second->close();
            }
            continue;
        }
        if ( conn->low_fill_buffer(false, 0) )
        {
            result= true;
        }
    }
    return result;
} // eo T2NServerBase::fill_connection_buffers()


/**
 * @brief fills the connection buffers.
 *
 * Uses the AsyncIo Backend to wait for new data.
 *
 * @param usec_timeout the maximum time period to wait for new data (in microseconds). 
 *  0 returns immediately, -1 waits until some event occurred.
 * @param timeout_remaining ignored!
 * @return @a true if new data for at least one connection arrived.
 *
 * @note since this method uses the AsyncIo backend, the timeout will have only milli second
 * resolution.
 */
bool T2NServerBase::fill_buffer(long long usec_timeout, long long* timeout_remaining)
{
    SCOPETRACKER();

    if (m_new_data_available)
    {
        // short cut if we already know that we have new data:
        m_new_data_available= false;
        return true;
    }

    int timeout= 0;

    if (usec_timeout<0)
    {
        timeout= -1;
    }
    else if (usec_timeout > 0)
    {
        long long msec_timeout= (usec_timeout + 500)/1000;

        if (msec_timeout >= INT_MAX)
        {
            timeout= INT_MAX;
        }
        else
        {
            timeout= (int)msec_timeout;
        }
    }
    // not really.. but it shouldn't be used either...
    if (timeout_remaining) *timeout_remaining= 0L;

    if (! fill_connection_buffers() && timeout>0)
    {
        bool had_activity= Backend::getBackend()->doOneStep( timeout );
        return fill_connection_buffers();
    }
    return true;
} // to T2NServerBase::fill_buffer(long long,long long*)



/*
** creator functions:
*/


/**
 * @brief creates a server object with unix domain server socket.
 * @param path path of the unix domain socket.
 * @param mode mode for the socket.
 * @return shared pointer with the new server object; empty if none could be created..
 */
T2NServerBasePtr createT2NUnixServerPort(const std::string& path, int mode)
{
    SCOPETRACKER();
    boost::shared_ptr< T2NUnixServer > result( new T2NUnixServer(path,mode) );
    if (!result->isOpen())
    {
        module_logger().error(HERE)
            << "failed to open unix domain server socket on \"" << path << "\"";
    }
    return result;
} // eo createT2NUnixServerPort(const std::string&,int)


/**
 * @brief creates a client object connected to a server via unix daomain socket.
 * @param path path of cthe unix domain socket.
 * @return shared pointer with the new client object; empty if none could be created..
 */
T2NClientConnectionPtr createT2NUnixClientConnection(const std::string& path)
{
    typedef IOExportWrapper< AsyncIo::UnixIOSocket > MyIo;
    typedef boost::shared_ptr< MyIo > MyIoPtr;
    SCOPETRACKER();
    MyIoPtr connection( new MyIo(path) );
    boost::shared_ptr< RealT2NClientConnection > result( new RealT2NClientConnection( connection ) );
    if (not result->isOpen())
    {
        module_logger().error(HERE)
            << "failed to open unix domain client socket on \"" << path << "\"";
        return T2NClientConnectionPtr();
    }
    return result;
} // eo createT2NUnixClientConnection(const std::string&)

} // eo namespace AsyncIo
