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
/** @file
 * @copyright Copyright &copy; 2007-2008 by Intra2net AG
 */

//#define NOISEDEBUG

#include "async_io.hpp"
#include <asyncio_config.hpp>

#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include <sys/poll.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <boost/bind.hpp>

#include <asyncio_signalfunc.hpp>

#include <iostream>

#ifdef NOISEDEBUG
#include <iostream>
#include <iomanip>
#define DOUT(msg) std::cout << msg << std::endl
#define FODOUT(obj,msg) std::cout << typeid(*obj).name() << "[" << obj << "]:" << msg << std::endl
//#define ODOUT(msg) std::cout << typeid(*this).name() << "[" << this << "]:" << msg << std::endl
#define ODOUT(msg) std::cout << __PRETTY_FUNCTION__ << "[" << this << "]:" << msg << std::endl
#else
#define DOUT(msg) do {} while (0)
#define FODOUT(obj,msg) do {} while (0)
#define ODOUT(msg) do {} while (0)
#endif


namespace
{

using namespace AsyncIo::Utils;

/*
 * configuration:
 */


const int c_max_poll_wait= 10*60*1000; // maximal poll wait (while in backend loop): 10 min


/**
 * contains internal helper structs and functions for io handling.
 */
namespace internal_io
{

/**
 * extends struct pollfd with some convenience functions
 */
struct PollFd : public ::pollfd
{
    PollFd()
    {
        fd= 0;
        events= revents= 0;
    } // eo PollFd


    /**
     * initializes the struct with a given file descriptor and clears the event mask(s).
     * @param _fd
     */
    PollFd(int _fd)
    {
        fd= _fd;
        events= revents= 0;
    } // eo PollFd


    /**
     * set that we want to be notified about new incoming data
     */
    void setPOLLIN()  { events |= POLLIN; }

    /**
     * set that we want to be notified if we can send (more) data.
     */
    void setPOLLOUT() { events |= POLLOUT; }

}; // eo struct PollFd


typedef std::vector<PollFd> PollVector;
typedef std::map<int,PollVector::size_type> FdPollMap;
typedef std::map<int,AsyncIo::IOImplementation*> FdIOMap;


/**
 * struct for interfacing our local structures with poll()
 */
struct PollDataCluster
{
    PollVector m_poll_vector;
    FdPollMap  m_fd_poll_map;
    FdIOMap    m_read_fd_io_map;
    FdIOMap    m_write_fd_io_map;

    void add_read_fd( int fd, AsyncIo::IOImplementation* io);
    void add_write_fd( int fd, AsyncIo::IOImplementation* io);

    pollfd* get_pollfd_ptr();
    unsigned int get_num_pollfds() const;

}; // eo struct PollDataCluster


typedef PtrList< AsyncIo::IOImplementation, true > IOList;
typedef PtrList< AsyncIo::TimerBase, true >        TimerList;

template<> int IOList::GlobalCountType::InstanceCount= 0;
template<> int TimerList::GlobalCountType::InstanceCount= 0;


/**
 * the (internal) global list of io objects (object pointers)
 */
IOList& g_io_list()
{
    static IOList _the_io_list;
    return _the_io_list;
};


/**
 * the (internal) global list of timer objects (object pointers)
 */
TimerList& g_timer_list()
{
    static TimerList _the_timer_list;
    return _the_timer_list;
}

/*
 * implementation of PollDataCluster
 */


/**
 * add a new file descriptor to the read list.
 *
 * @param fd the file descriptor.
 * @param io the io object which uses the fd for reading.
 */
void PollDataCluster::add_read_fd( int fd, AsyncIo::IOImplementation* io)
{
    FdPollMap::iterator itPollMap = m_fd_poll_map.find(fd);
    if (itPollMap != m_fd_poll_map.end())
    {
        m_poll_vector[ itPollMap->second ].setPOLLIN();
    }
    else
    {
        PollFd item(fd);
        item.setPOLLIN();
        m_fd_poll_map[fd] = m_poll_vector.size();
        m_poll_vector.push_back( item );
    }
    m_read_fd_io_map[fd]= io;
} // eo PollDataCluster::add_read_fd


/**
 * add a new file descriptor to the write list.
 *
 * @param fd the file descriptor.
 * @param io the io object which uses the fd for writing.
 */
void PollDataCluster::add_write_fd( int fd, AsyncIo::IOImplementation* io)
{
    FdPollMap::iterator itPollMap = m_fd_poll_map.find(fd);
    if (itPollMap != m_fd_poll_map.end())
    {
        m_poll_vector[ itPollMap->second ].setPOLLOUT();
    }
    else
    {
        PollFd item(fd);
        item.setPOLLOUT();
        m_fd_poll_map[fd] = m_poll_vector.size();
        m_poll_vector.push_back( item );
    }
    m_write_fd_io_map[fd]= io;
} // eo PollDataCluster::add_write_fd


/**
 * returns a pointer to a pollfd array; suitable for passing to poll()
 *
 * @return pointer to pollfd array
 */
pollfd* PollDataCluster::get_pollfd_ptr()
{
    return m_poll_vector.empty() ? NULL : &m_poll_vector.front();
} // eo get_pollfd_ptr


/**
 * returns the number of entries in the pollfd array; suitable for passing to poll()
 *
 * @return the number of entries in the pollfd array
 */
unsigned int PollDataCluster::get_num_pollfds() const
{
    return m_poll_vector.size();
} // eo get_num_pollfds



} // eo namespace internal_io


/*
 * some internal tool functions and structures
 */

struct FilterMatch {
    AsyncIo::FilterBasePtr m_filter;

    FilterMatch(AsyncIo::FilterBasePtr filter)
    : m_filter(filter)
    {}

    bool operator () (const AsyncIo::FilterBasePtr& item)
    {
        return item && item == m_filter;
    }

}; // eo struct FilterMatch



} // eo anonymous namespace




namespace AsyncIo
{


/*
 * implementation of TimerBase
 */

/**
 * constructor. Adds the object to the internal timer list.
 */
TimerBase::TimerBase()
: m_active(false)
, m_marked(false)
{
    internal_io::g_timer_list().add_item(this);
} // eo TimerBase::TimerBase


/**
 * destructor. Removes the object from the internal timer list.
 */
TimerBase::~TimerBase()
{
    ODOUT("enter");
    if (internal_io::TimerList::Instances())
    {
        ODOUT("remove from list");
        internal_io::g_timer_list().remove_item(this);
    }
} // eo TimerBase::~TimerBase


/**
 * @brief returns the point in time when the time is executed in real time.
 * @return the point in time when the timer is to be executed.
 */
MilliTime TimerBase::getRealWhenTime() const
{
    MilliTime mono_time;
    MilliTime real_time;
    get_current_monotonic_time(mono_time);
    get_current_real_time(real_time);
    MilliTime result= m_when - mono_time + real_time;
    return result;
} // eo TimerBase::getRealWhenTime() const


/**
 * sets the time when the event should be executed.
 * @param sec the seconds part of the point in time.
 * @param msec  the milliseconds part of the point in time.
 */
void TimerBase::setWhenTime(long sec, long msec)
{
    m_when.set(sec,msec);
    m_marked= false;
} // eo TimerBase::setWhenTime


/**
 * sets the time when the event should be executed.
 * @param mt the point in time.
 */
void TimerBase::setWhenTime(const MilliTime& mt)
{
    m_when= mt;
    m_marked= false;
} // eo TimerBase::setWhenTime


/**
 * sets the time delta measured from current time when the event should be executed.
 * @param sec the seconds of the time delta
 * @param msec the milli seconds of the time delta
 */
void TimerBase::setDeltaWhenTime(long sec, long msec)
{
    setDeltaWhenTime( MilliTime(sec,msec) );
} // eo TimerBase::setWhenTime



/**
 * sets the time delta measured from current time when the event should be executed.
 * @param mt the time delta
 */
void TimerBase::setDeltaWhenTime(const MilliTime& mt)
{
    get_current_monotonic_time(m_when);
    m_when+= mt;
    m_marked= false;
} // eo TimerBase::setWhenTime


/**
 * set the active state of the timer event.
 * @param active determines if the object should be active (default: yes).
 */
void TimerBase::activate(bool active)
{
    m_active = active;
    if (!active)
    {
        // clear the mark if we are not active.
        m_marked= false;
    }
} // eo TimerBase::activate


/** @fn void TimerBase::deactivate()
 *  deactivates the event by clearing the active state.
 */


/**
 * called when the timer event occured.
 */
void TimerBase::execute()
{
} // eo TimerBase::execute


/*
 * implementation of FilterBase class
 */


FilterBase::FilterBase()
: m_io(NULL)
{
} // eo FilterBase::FilterBase()


/**
 * injects incoming data.
 * @param data  the new data
 */
void FilterBase::injectIncomingData(const std::string& data)
{
    if (m_io)
    {
        FilterBasePtr ptr= get_ptr_as< FilterBase >();
        if (ptr)
        {
            m_io->injectIncomingData(ptr,data);
        }
    }
} // FilterBase::injectIncomingData(const std::string&)


/**
 * injects outgoing data.
 * @param data the new data
 */
void FilterBase::injectOutgoingData(const std::string& data)
{
    if (m_io)
    {
        FilterBasePtr ptr= get_ptr_as< FilterBase >();
        if (ptr)
        {
            m_io->injectOutgoingData(ptr,data);
        }
    }
} // eo FilterBase::injectOutgoingData(const std::string&)



/**
 * called when EOF detected on incoming channel (or incoming channel closed)
 */
void FilterBase::endOfIncomingData()
{
} // eo FilterBase::endOfIncomingData()

/**
 * called when the filter should reset.
 * This is used when a new channel is opened or when the filter is taken out of a filter chain.
 */
void FilterBase::reset()
{
} // eo FilterBase::reset()


/*
 * implementation of IOImplementation class
 */


/**
 * constructor for the base io class.
 *
 * Also adds the object to internal list of io objects (which is used by the backend).
 *
 * @param read_fd the file descriptor which should be used for reading (default -1 for no value)
 * @param write_fd  the file descriptor which should be used for writing (default -1 for no value)
 */
IOImplementation::IOImplementation(int read_fd, int write_fd)
: m_read_fd(-1)
, m_write_fd(-1)
, m_eof(false)
, m_not_writable(false)
, m_input_buffer()
, m_output_buffer()
, m_marked_for_reading(false)
, m_marked_for_writing(false)
{
    internal_io::g_io_list().add_item(this);
    if (read_fd >= 0)
    {
        setReadFd( read_fd );
    }
    if (write_fd >= 0)
    {
        setWriteFd( write_fd );
    }
} // eo IOImplementation::IOImplementation


/**
 * destructor of the base io class.
 *
 * Removes the object from the interal list of io objects.
 */
IOImplementation::~IOImplementation()
{
    close();
    if (internal_io::IOList::Instances())
    {
        internal_io::g_io_list().remove_item(this);
    }
    // now clear the filters:
    while (! m_filter_chain.empty() )
    {
        FilterChain::iterator it = m_filter_chain.begin();
        (*it)->reset();
        (*it)->m_io= NULL;
        //TODO: signal the filter that it is removed ?!
        m_filter_chain.erase(it);
    }
} // eo IOImplementation::~IOImplementation


/**
 * adds another filter to the filter chain.
 * @param filter pointer to the new filter.
 */
void IOImplementation::addFilter
(
    FilterBasePtr filter
)
{
    if (!filter)
    {
        return; // nothing to do
    }
    if (filter->m_io)
    {
        filter->m_io->removeFilter(filter);
    }
    m_filter_chain.push_back( filter );
} // eo IOImplementation::addFilter


/**
 * removes a filter from the filter chain.
 * @param filter the pointer to the filter which is removed.
 * @note if the filter is removed the class gives away the ownership; i.,e. the caller is responsible for
 * deleting the filter if it was dynamically allocated.
 */
void IOImplementation::removeFilter
(
    FilterBasePtr filter
)
{
    FilterChain::iterator it =
        std::find_if( m_filter_chain.begin(), m_filter_chain.end(), FilterMatch(filter) );
    if (it != m_filter_chain.end())
    {
        filter->reset();
        filter->m_io= NULL;
        //TODO: signal the filter that it is removed ?!
        m_filter_chain.erase(it);
    }
} // eo IOImplementation::removeFilter


/**
 * closes the file descriptors (/ the connection).
 *
 * @param direction the direction which should be closed (default: @a Direction::both for all).
 */
void IOImplementation::close(Direction direction)
{
    bool had_read_fd= (m_read_fd >= 0);
    m_errno= 0;
    if (direction == Direction::unspecified) direction= Direction::both;
    if (direction != Direction::both && m_read_fd==m_write_fd && m_read_fd>=0)
    { // special case: half closed (socket) connections...
        // NOTE: for file descriptors m_errno will set to ENOTSOCK, but since we "forget" the desired part
        // (read_fd or write_fd) this class works as desired.
        switch(direction)
        {
            case Direction::in:
                {
                    int res= ::shutdown(m_read_fd, SHUT_RD);
                    if (res<0)
                    {
                        m_errno= errno;
                    }
                    m_read_fd= -1;
                    if (!m_eof)
                    {
                        for(FilterChain::iterator it= m_filter_chain.begin();
                            it != m_filter_chain.end();
                            ++it)
                        {
                            (*it)->endOfIncomingData();
                        }
                    }
                }
                return;

            case Direction::out:
                {
                    int res= ::shutdown(m_write_fd, SHUT_WR);
                    if (res<0)
                    {
                        m_errno= errno;
                    }
                    m_write_fd= -1;
                    m_output_buffer.clear();
                }
                return;
        }
    }
    if (m_write_fd >= 0 && (direction & Direction::out) )
    {
        int res1 = ::close(m_write_fd);
        if (m_write_fd == m_read_fd)
        {
            m_read_fd= -1;
        }
        m_write_fd= -1;
        m_output_buffer.clear();
        if (res1<0) m_errno= errno;
    }
    if (m_read_fd >=0 && (direction & Direction::in) )
    {
        int res1 = ::close(m_read_fd);
        m_read_fd= -1;
        if (res1<0) m_errno= errno;
    }
    if (had_read_fd && !m_eof && (m_read_fd<0))
    {
        for(FilterChain::iterator it= m_filter_chain.begin();
            it != m_filter_chain.end();
            ++it)
        {
            (*it)->endOfIncomingData();
        }
    }
} // eo IOImplementation::close


/**
 * determines if the io class wants to read data.
 * Default implementation checks only for a valid file descriptor value.
 *
 * @return @a true if the objects wants to read data
 */
bool IOImplementation::wantRead()
{
    return (m_read_fd >= 0) && ! m_eof;
} // eo IOImplementation::wantRead


/**
 * determines if the io class wants to write data.
 * Default implementation checks for a valid file descriptor value and if the object
 * cannot write data immediately.
 *
 * @return @a true if the objects wants to write data
 */
bool IOImplementation::wantWrite()
{
    return (m_write_fd >= 0) && ! m_marked_for_writing && ! m_not_writable;
} // eo IOImplementation::wantWrite


/**
 * delivers if opened.
 * The default returns @a true if at least one file descriptor (read or write) is valid.
 * @return @a true if opened.
 */
bool IOImplementation::opened() const
{
    return (m_read_fd>=0) || (m_write_fd>=0);
} // eo IOImplementation::opened() const


/**
 * returns if the read side detected an end of file (EOF).
 * @return @a true if end of file was detected on read file descriptor (or read file descriptor isn't valid).
 */
bool IOImplementation::eof() const
{
    return (m_read_fd < 0) || m_eof;
} // eo IOImplementatio::eof() const


/**
 * @brief returns of the write side didn't detect that it cannot write.
 * @return @a true if we can write.
 */
bool IOImplementation::writable() const
{
    return (m_write_fd >=0 ) and not m_not_writable;
} // eo IOImplementation::writable() const


/**
 * returns if the output buffer is empty.
 * @return
 */
bool IOImplementation::empty() const
{
    return m_output_buffer.empty();
} // eo IOImplementation::empty

/**
 * puts data into the output buffer and sends it immediately if possible,
 *
 * The data is passed through the filter chain before it's stored in the output buffer
 * (i.e. the output buffer contains data as it should be send directly to the descriptor).
 * @param _data the data which should be send.
 */
void IOImplementation::lowSend(const std::string& _data)
{
    std::string data(_data);

    for(FilterChain::reverse_iterator it_filter= m_filter_chain.rbegin();
        it_filter!= m_filter_chain.rend();
        ++it_filter)
    {
        data= (*it_filter)->filterOutgoingData(data);
    }
    m_output_buffer+= data;

    // if we can send immediately, do it:
    if (! m_output_buffer.empty() && m_marked_for_writing)
    {
        doWrite();
    }
} // eo IOImplementation::lowSend


/**
 * called by the backend when there is data to read for this object.
 *
 * Reads the data from the connection (read file descriptor) and passes the data through the filter chain.
 * The final data is appended to the input buffer and the signal @a m_signal_read() is called.
 *
 * If EOF is detected (i,e, no data was received) then the signal @a m_signal_eof() is called.
 *
 * @note overload this method only when You know what You are doing!
 * (overloading is necessary when handling server sockets.)
 */
void IOImplementation::doRead()
{
    // static read buffer; should be ok as long as we don't use threads
    static char buffer[8*1024]; // 8 KiB

    m_errno = 0;
    if (m_read_fd<0 || !m_marked_for_reading)
    {
        ODOUT("exit0; read_fd="<<m_read_fd << " mark=" << m_marked_for_reading);
        return;
    }

    // reset the mark:
    m_marked_for_reading = false;

    // now read the data:
    ssize_t count;
    count = ::read(m_read_fd, buffer, sizeof(buffer));

    ODOUT("::read -> " << count);

    // interpret what we got:
    if (count < 0) // error
    {
        m_errno = errno;
        int fd= m_read_fd;

        switch(m_errno)
        {
            case EINVAL:
            case EBADF:
            case ECONNRESET:
            case ENETRESET:
                if (fd == m_read_fd)
                {
                    close( (m_read_fd == m_write_fd) ? Direction::both : Direction::in );
                }
                break;
        }
    }
    else if (count==0) // EOF
    {
        // remember the read fd:
        int fd = m_read_fd;
        // remember the EOF:
        m_eof= true;
        // signal EOF
        m_signal_eof();
        // if the fd is still the same: close it.
        if (fd == m_read_fd)
        {
            close( Direction::in );
        }
    }
    else // we have valid data
    {
        std::string data(buffer,count);
        ODOUT(" got \"" << data << "\"");
        for(FilterChain::iterator it_filter= m_filter_chain.begin();
            it_filter != m_filter_chain.end();
            ++it_filter)
        {
            data= (*it_filter)->filterIncomingData(data);
        }
        m_input_buffer+= data;
        m_signal_read();
    }
} // eo IOImplementation::doRead


/**
 * interface for filter classes to inject data into the filter chain (emulating new incoming data).
 * @param from_filter the filter which injects the new data.
 * @param _data  the new data.
 */
void IOImplementation::injectIncomingData(FilterBasePtr from_filter, const std::string& _data)
{
    FilterChain::iterator it_filter =
        std::find_if( m_filter_chain.begin(), m_filter_chain.end(), FilterMatch(from_filter) );
    if (it_filter == m_filter_chain.end())
    {
        // dont accept data inject from a unknown filter
        return;
    }
    // well: pass the data through the remaining filters:
    // NOTE: processing is (nearly) the same as in IOImplementation::doRead()
    std::string data(_data);
    for(++it_filter;
        it_filter != m_filter_chain.end();
        ++it_filter)
    {
        data= (*it_filter)->filterIncomingData(data);
    }
    m_input_buffer+= data;
    m_signal_read();
} // eo IOImplementation::injectIncomingData(FilterBase*,const std::string&)


/**
 * interface for filter classes to inject data into the filter chain (emulating new outgoing data).
 * @param from_filter the filter which injects the new data.
 * @param _data  the new data.
 */
void IOImplementation::injectOutgoingData(FilterBasePtr from_filter, const std::string& _data)
{
    FilterChain::reverse_iterator it_filter =
        std::find_if( m_filter_chain.rbegin(), m_filter_chain.rend(), FilterMatch(from_filter) );
    if (it_filter == m_filter_chain.rend())
    {
        // dont accept data inject from a unknown filter
        return;
    }
    // well: pass the data through the remaining filters:
    // NOTE: processing is (nearly) the same as in IOImplementation::lowSend()
    std::string data(_data);
    for(++it_filter;
        it_filter!= m_filter_chain.rend();
        ++it_filter)
    {
        data= (*it_filter)->filterOutgoingData(data);
    }
    m_output_buffer+= data;

    // if we can send immediately, do it:
    if (! m_output_buffer.empty() && m_marked_for_writing)
    {
        doWrite();
    }
} // eo IOImplementation::injectOutgoingData(FilterBase*,const std::string&)


/**
 * set the read file descriptor.
 * Although a derived class can also set the read fd directly; this method should be used
 * for this task since it updates some flags on the fd for async operation.
 * @param fd the new read file descriptor.
 */
void IOImplementation::setReadFd(int fd)
{
    // test if we already have a valid descriptor (and may have to close it):
    if (m_read_fd >=0 )
    {
        if (m_read_fd == fd)
        {
            // fd was already right; consider it to be ok.
            return;
        }
        close(Direction::in);
    }
    // reset our errno:
    m_errno= 0;
    // if the new descriptor looks valid, set some flags:
    if (fd >= 0)
    {
        long flags= ::fcntl(fd, F_GETFL);
        if (flags != -1)
        {
            // set the flags for non blocking, async operation
            flags |= O_NONBLOCK|O_ASYNC;
            ::fcntl(fd,F_SETFL, flags);
        }
        else if ( errno == EBADF )
        {
            // well, we seemed to be fed with an invalid descriptor...:
            m_errno = errno;
            fd= -1;
        }
    }
    if (fd >= 0) // if still valid:
    {
        // set the close-on-exec flag
        ::fcntl(fd,F_SETFD, FD_CLOEXEC);
    }
    m_read_fd= fd;
    m_marked_for_reading= false;
    m_eof= false;
} // eo IOImplementation::setReadFd(int)



/**
 * set the write file descriptor.
 * Although a derived class can also set the write fd directly; this method should be used
 * for this task since it updates some flags on the fd for async operation.
 * @param fd the new write file descriptor.
 */
void IOImplementation::setWriteFd(int fd)
{
    if (m_write_fd >=0 )
    {
        if (m_write_fd == fd)
        {
            // fd was already right; consider it to be ok.
            return;
        }
        close(Direction::out);
    }
    // reset our errno:
    m_errno= 0;
    // if the new descriptor looks valid, set some flags:
    if (fd >= 0)
    {
        long flags= ::fcntl(fd, F_GETFL);
        if (flags != -1)
        {
            // set the flags for non blocking, async operation
            flags |= O_NONBLOCK|O_ASYNC;
            ::fcntl(fd,F_SETFL, flags);
        }
        else if (errno == EBADF)
        {
            // well, we seemed to be fed with an invalid descriptor...:
            m_errno = errno;
            fd= -1;
        }
    }
    if (fd >= 0) // if still valid:
    {
        // set the close-on-exec flag
        ::fcntl(fd,F_SETFD, FD_CLOEXEC);
    }
    m_write_fd = fd;
    m_marked_for_writing= false;
    m_not_writable= false;
} // eo IOImplementation::setWriteFd(int)



/**
 * called by the backend when this object can write data.
 *
 * If some data was sended, the signal @a m_signal_write is called.
 *
 * @internal tries to write all buffered data to output; if this succeeds,
 * the connection is assumed to be still able to accept more data.
 * (i.e. the internal write mark is kept!)
 *
  * @note overload this method only when You know what You are doing!
*/
void IOImplementation::doWrite()
{
    m_errno = 0;
    if ( m_write_fd<0 || !m_marked_for_writing || m_output_buffer.empty())
    {
        return;
    }

    ODOUT("doWrite, d=\"" << m_output_buffer << "\"");

    //reset mark:
    m_marked_for_writing= false;

    // now write the data
    ssize_t count= ::write( m_write_fd, m_output_buffer.data(), m_output_buffer.size());

    ODOUT("::write -> " << count);

    if (count < 0) // error
    {
        m_errno= errno;
        int fd= m_write_fd;

        switch(m_errno)
        {
            case EPIPE:
                m_not_writable= true;
                // emit a signal
                m_signal_not_writable();
                // fall through
            case EINVAL:
            case EBADF:
            case ECONNRESET:
            case ENETRESET:
                if (fd == m_write_fd)
                {
                    close( (m_write_fd == m_read_fd) ? Direction::both : Direction::out );
                }
                break;
        }
    }
    else
    {
        m_output_buffer.erase(0, count);
        if (m_output_buffer.empty())
        {
            // special case: if we were able to send all the data, we keep the write mark:
            m_marked_for_writing= true;
        }
    }
    if (count > 0)
    {
        m_signal_write();
    }
} // eo IOImplementation::doWrite

/**
 * Return a copy of the input buffer.
 */
std::string IOImplementation::getInput() const
{
    return m_input_buffer;
}

/**
 * Return the input buffer and clear it.
 */
std::string IOImplementation::getInputClear()
{
    std::string retbuf;

    retbuf.swap(m_input_buffer);

    return retbuf;
}

/**
 * Return true if there are bytes available in the input buffer.
 */
bool IOImplementation::inputAvailable() const
{
    return !m_input_buffer.empty();
}

/**
 * Cut off the first len bytes from the input buffer.
 * @returns The number of bytes actually shortened
 */
std::string::size_type IOImplementation::shortenInput(std::string::size_type len)
{
    std::string::size_type real_len=len;
    if (real_len > m_input_buffer.size())
        real_len=m_input_buffer.size();

    if (real_len > 0)
        m_input_buffer.erase(0,real_len);

    return real_len;
}

/**
 * Read and remove a line from the input buffer.
 * @param delim the line ending character, \\n by default.
 * @returns A full line including the line ending if available, an empty string otherwise.
 */
std::string IOImplementation::getline(char delim)
{
    std::string line;
    std::string::size_type pos=m_input_buffer.find(delim);

    // no line ending in the buffer?
    if (pos==std::string::npos)
        return line;

    // copy the line including the delimiter
    line=m_input_buffer.substr(0,pos+1);

    m_input_buffer.erase(0,pos+1);

    return line;
}


/*
 * implementation of SimpleIO
 */


SimpleIO::SimpleIO(int read_fd, int write_fd)
: inherited(read_fd, write_fd)
{
    m_signal_read.connect(boost::bind(&SimpleIO::slotReceived,this));
} // eo SimpleIO::SimpleIO()


SimpleIO::~SimpleIO()
{
} // eo SimpleIO::~SimpleIO()


/**
 * sends a string.
 * @param data the string.
 */
void SimpleIO::sendString(const std::string& data)
{
    lowSend(data);
} // eo SimpleIO::sendString(const std::string&)


/**
 * emits the signal signalReceived with the received data.
 * This slot is connected to IOImplementation::m_signal_read.
 */
void SimpleIO::slotReceived()
{
    std::string data;
    data.swap(m_input_buffer);
    signal_received_string(data);
} // eo SimpleIO::slotReceived()



/*
 * implementation of SimpleIO2
 */


SimpleIO2::SimpleIO2(int read_fd, int write_fd)
: inherited(read_fd, write_fd)
{
    m_signal_read.connect(boost::bind(&SimpleIO2::slotReceived,this));
} // eo SimpleIO2::SimpleIO2()


SimpleIO2::~SimpleIO2()
{
} // eo SimpleIO2::~SimpleIO2()


/**
 * sends a string.
 * @param data the string.
 */
void SimpleIO2::sendString(const std::string& data)
{
    lowSend(data);
} // eo SimpleIO2::sendString(const std::string&)


/**
 * emits the signal signalReceived with the received data.
 * This slot is connected to IOImplementation::m_signal_read.
 */
void SimpleIO2::slotReceived()
{
    std::string data;
    data.swap(m_input_buffer);
    signal_received_string(data);
} // eo SimpleIO2::slotReceived()



/*
 * implementation of class Backend (singleton)
 */

Backend* Backend::g_backend= NULL;

int Backend::m_count_active_steps=0;


Backend::Backend()
: m_count_active_loops(0)
, m_count_stop_requests(0)
{
	SystemTools::ignore_signal( SystemTools::Signal::PIPE );
} // eo Backend::Backend


Backend::~Backend()
{
    SystemTools::restore_signal_handler( SystemTools::Signal::PIPE );
} // eo Backend::~Backend()

/**
 * delivers pointer to the current backend, instantiating a new backend if there was no current one.
 *
 * This should be the only way to access the backend which should be a singleton.
 *
 * @return the pointer to the current backend.
 */
Backend* Backend::getBackend()
{
    if (!g_backend)
    {
        g_backend = new Backend();
    }
    return g_backend;
} // eo Backend::getBackend




/**
 * performs one backend cycle.
 *
 * Collects all file descriptors from the active io objects which should be selected for reading and/or writing.
 * Also determines the timer events which become due and adjusts the timeout.
 * Constructs the necessary structures and calls poll().
 * Finally interprets the results from poll() (i.e. performs the reading/writing/timer events)
 *
 * @param timeout maximal wait value in milliseconds; negative value waits until at least one event occured.
 * @return @a true if there was at least one active object; otherwise @a false
 *
 * @note this method is a little beast.
 *
 * @internal
 * The cycle is divided into four steps: collecting; poll; mark and execute.
 * The "mark" step is necessary to avoid some bad side effects when method calls in the execution stage
 * are calling @a Backup::doOneStep or open their own local backend loop.
 *
 * @todo handle some more error cases.
 * @todo provide a plugin interface for external handler.
 * (currently inclusion of external handler is possible by (ab)using timer classes)
 */
bool Backend::doOneStep(int timeout)
{
    ODOUT( "timeout=" << timeout );
    internal_io::PollDataCluster poll_data;
    bool had_active_object = false;

    ++m_count_active_steps;

    try {

        FdSetType local_read_fds;
        FdSetType local_write_fds;

        // step 1 ; collect

        { // step 1.1: collect fds for read/write operations
            for(internal_io::IOList::iterator itIOList = internal_io::g_io_list().begin();
                itIOList != internal_io::g_io_list().end();
                ++itIOList)
            {
                if (! *itIOList) continue; // skip NULL entries
                int  read_fd  = (*itIOList)->m_read_fd;
                int  write_fd = (*itIOList)->m_write_fd;
                bool want_read  = (read_fd >= 0) and (*itIOList)->wantRead();
                bool want_write = (write_fd >= 0) and (*itIOList)->wantWrite();
                if (read_fd >= 0 )
                {
                    local_read_fds.insert( read_fd );
                }
                if (write_fd >= 0)
                {
                    local_write_fds.insert( write_fd );
                }
                if (!want_read && !want_write) continue;
                if (want_read)
                {
                    FODOUT( (*itIOList), "wants to read  (fd=" << read_fd << ")");
                    poll_data.add_read_fd(read_fd, *itIOList);
                }
                if (want_write)
                {
                    FODOUT( (*itIOList), "wants to write  (fd=" << write_fd << ")");
                    poll_data.add_write_fd(write_fd, *itIOList);
                }
                had_active_object= true;
            }
        }

        { // step 1.2: collect timer events
            MilliTime current_time;
            MilliTime min_event_time;

            get_current_monotonic_time(current_time);
            bool min_event_time_set;

            if (timeout >= 0)
            {
                min_event_time = current_time + MilliTime(0,timeout);
                min_event_time_set= true;
            }
            else
            {
                min_event_time = current_time + MilliTime(86400,0);
                min_event_time_set= false;
            }
            // TODO

            for(internal_io::TimerList::iterator it_timer= internal_io::g_timer_list().begin();
                it_timer != internal_io::g_timer_list().end()
                && (!had_active_object || !min_event_time_set || current_time < min_event_time);
                ++ it_timer)
            {
                if (! *it_timer) continue; // skip NULL entries
                if (! (*it_timer)->m_active) continue; // skip if not enabled
                if ( !min_event_time_set || (*it_timer)->m_when < min_event_time)
                {
                    min_event_time = (*it_timer)->m_when;
                    min_event_time_set= true;
                }
                had_active_object= true;
            }

            if (min_event_time_set)
            { // we have at a minimal event time, so (re)compute the timeout value:
                MilliTime delta= (min_event_time - current_time);
                long long delta_ms = std::min( delta.get_milliseconds(), 21600000LL); // max 6h
                if (delta_ms <= 0L)
                {
                    timeout= 0L;
                }
                else
                {
                    timeout= delta_ms + (delta_ms<5 ? 1 : 3);
                }
            }
        }

        // step 2 : poll
        ODOUT(" poll timeout is " << timeout);
        {
            MilliTime current_time;
            get_current_monotonic_time(current_time);
            ODOUT("  current time is sec="<<current_time.mt_sec << ", msec=" << current_time.mt_msec);
        }
        int poll_result= ::poll(poll_data.get_pollfd_ptr(), poll_data.get_num_pollfds(), timeout);

        ODOUT("poll -> " << poll_result);
        {
            MilliTime current_time;
            get_current_monotonic_time(current_time);
            ODOUT("  current time is sec="<<current_time.mt_sec << ", msec=" << current_time.mt_msec);
        }

        if (poll_result < 0)
        {
            //TODO poll error handling (signals ?!)
        }

        // step 3 : mark

        // step 3.1: mark io objects (if necessary)
        if (poll_result > 0)
        {
            for(internal_io::PollVector::iterator itPollItem = poll_data.m_poll_vector.begin();
                itPollItem != poll_data.m_poll_vector.end();
                ++itPollItem)
            {
                ODOUT("  fd=" << itPollItem->fd << ", events=" << itPollItem->events << ", revents=" << itPollItem->revents);
                if ( 0 == (itPollItem->revents))
                { // preliminary continuation if nothing is to handle for this item(/fd)...
                    continue;
                }
                if ( 0!= (itPollItem->revents & (POLLIN|POLLHUP)))
                {
                    IOImplementation *io= poll_data.m_read_fd_io_map[ itPollItem->fd ];
                    if (io && io->m_read_fd==itPollItem->fd)
                    {
                        FODOUT(io,"marked for reading");
                        io->m_marked_for_reading= true;
                    }
                }
                if ( 0!= (itPollItem->revents & POLLOUT))
                {
                    IOImplementation *io= poll_data.m_write_fd_io_map[ itPollItem->fd ];
                    if (io && io->m_write_fd==itPollItem->fd)
                    {
                        io->m_marked_for_writing= true;
                    }
                }
                if ( 0!= (itPollItem->revents & POLLERR))
                {
                    IOImplementation *io= poll_data.m_write_fd_io_map[ itPollItem->fd ];
                    if (0!= (itPollItem->events & POLLOUT))
                    {
                        if (io && io->m_write_fd==itPollItem->fd)
                        {
                            io->m_marked_for_writing= false;
                            //io->close( Direction::out );
                        }
                    }
                }
                // TODO error handling (POLLERR, POLLHUP, POLLNVAL)
            }
        }

        //Step 3.2: mark timer objects
        {
            MilliTime current_time;

            get_current_monotonic_time(current_time);
            ODOUT("  current time is sec="<<current_time.mt_sec << ", msec=" << current_time.mt_msec);

            for(internal_io::TimerList::iterator it_timer= internal_io::g_timer_list().begin();
                it_timer != internal_io::g_timer_list().end();
                ++ it_timer)
            {
                ODOUT("  check timer " << *it_timer);
                if (! *it_timer) continue; // skip NULL entries
                if (! (*it_timer)->m_active) continue; // skip if not enabled
                if ( (*it_timer)->m_when <= current_time)
                {
                    ODOUT("   ==> MARK");
                    (*it_timer)->m_marked = true;
                }
            }
        }


        // step 4 : execute

        // step 4.1: execute io
        ODOUT("execute stage");
        for(internal_io::IOList::iterator it_io = internal_io::g_io_list().begin();
            it_io != internal_io::g_io_list().end();
            ++ it_io)
        {
            ODOUT("  check obj " << *it_io);
            if (NULL == *it_io) continue;
            if ((*it_io)->m_marked_for_writing)
            {
                FODOUT((*it_io),"exec doWrite");
                (*it_io)->doWrite();
                if ((*it_io) == NULL) continue; // skip remaining if we lost the object
                if ((*it_io)->m_errno)
                {
                    continue;
                }
            }
            if ((*it_io)->m_marked_for_reading)
            {
                FODOUT((*it_io),"exec doRead");
                (*it_io)->doRead();
                if ((*it_io) == NULL) continue; // skip remaining if we lost the object
                if ((*it_io)->m_errno)
                {
                    continue;
                }
            }
        }

        // step 4.2: execute timer events
        {
            for(internal_io::TimerList::iterator it_timer= internal_io::g_timer_list().begin();
                it_timer != internal_io::g_timer_list().end();
                ++ it_timer)
            {
                if (! *it_timer) continue; // skip NULL entries
                if (! (*it_timer)->m_active) continue; // skip if not enabled
                if (! (*it_timer)->m_marked) continue; // skip if not marked

                // reset the mark and deactivate object now since the execute() method might activate it again
                (*it_timer)->m_marked= false;
                (*it_timer)->m_active= false;

                // now execute the event:
                (*it_timer)->execute();
            }
        }

    } // eo try
    catch(...)
    {
        // clean up our counter
        --m_count_active_steps;
        // and forward the exception
        throw;
    }

    if ( 0 == --m_count_active_steps)
    {
        internal_io::g_io_list().clean_list();
        internal_io::g_timer_list().clean_list();
    }

    return had_active_object;
} // eo Backend::doOneStep


/**
 * enters a backend loop.
 *
 * Calls @a Backend::doOneStep within a loop until @a Backend::stop was called or there are no more
 * active objects (io objects or timer objects).
 */
void Backend::run()
{
    ++m_count_active_loops;
    do
    {
        try
        {
            if (!doOneStep(c_max_poll_wait))
            {
                // stop if there are no more active objects.
                stop();
            }
        }
        catch(...)
        {
            // clean up our counter
            --m_count_active_loops;
            // and forward the exception
            throw;
        }
    }
    while (0 == m_count_stop_requests);
    --m_count_active_loops;
    --m_count_stop_requests;
} // eo Backend::run


/**
 * @brief stops the latest loop currently run by Backend::run().
 * @see Backend::run()
 */
void Backend::stop()
{
    if (m_count_active_loops)
    {
        ++m_count_stop_requests;
    }
} // eo Backend::stop()



} // eo namespace AsyncIo
