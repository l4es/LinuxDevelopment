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
 * @brief simple basic IO handling.
 *
 * @copyright &copy; Copyright 2007-2008 by Intra2net AG
 *
 * Deals with POSIX file descriptors; provides an additional abstraction
 * level above select() or poll() interface.
 * Also provides basic functionality for dealing with timer events.
 */

#ifndef __ASYNC_IO_HPP__
#define __ASYNC_IO_HPP__

#include <string>
#include <list>
#include <set>

#include <asyncio_config.hpp>

#include <asyncio_utils.hpp>

#include <boost/signal.hpp>
#include <boost/shared_ptr.hpp>


namespace AsyncIo
{

using namespace I2n;

using Utils::MilliTime;

/*
 * forward declarations
 */
class Backend;
class IOImplementation;

/*
 * end of forward declarations
 */


/**
 * direction for io operations.
 */
struct Direction
{
    enum _Direction
    {
        unspecified= 0,
        in = 1,
        out = 2,
        both= 3
    } m_direction;

    Direction( _Direction direction = unspecified) : m_direction(direction) {}

    operator _Direction () const { return m_direction; }
}; // eo struct IODirection;



/**
 * base class for time based events (timer events).
 *
 * consists basically of a point in time (when the event should be executed) and a method
 * which will be called when the given time is reached (or passed).
 */
class TimerBase
{
        friend class Backend;
    public:
        TimerBase();
        virtual ~TimerBase();

        bool active() const { return m_active; }

        MilliTime getWhenTime() const {return m_when;}
        MilliTime getRealWhenTime() const;

    protected:

        void setWhenTime(long sec, long msec=0);
        void setWhenTime(const MilliTime& mt);

        void setDeltaWhenTime(long sec, long msec=0);
        void setDeltaWhenTime(const MilliTime& mt);

        void activate(bool _active= true);
        void deactivate() { activate(false); }

        virtual void execute();

    private:
        /// @a true when the event is active.
        bool m_active;
        /// point in time when the event should be executed
        MilliTime m_when;
        /// mark from backend cycle that the event has to be executed.
        bool m_marked;
}; // eo class TimerBase



/**
 * base class for filter classes.
 *
 * filter objects can be "plugged" into IO objects for manipulating the data streams.
 * (for example: one could make a filter which handles the telnet protocol and plug it into a
 * socket io object.)
 *
 * @note filter object can be used only by one io object.
 */
class FilterBase
: virtual public Utils::SharedBase
{
        friend class IOImplementation;
    public:
        typedef boost::shared_ptr< FilterBase > PtrType;
    public:
        FilterBase();
        virtual ~FilterBase() { m_io = NULL; };

    protected:

        virtual std::string filterIncomingData(const std::string& data)= 0;
        virtual std::string filterOutgoingData(const std::string& data)= 0;

        virtual void endOfIncomingData();
        virtual void reset();

    protected:

        void injectIncomingData(const std::string& data);
        void injectOutgoingData(const std::string& data);

    private:
        /// pointer to the io object which uses this filter:
        IOImplementation *m_io;
}; // eo class FilterBase

typedef FilterBase::PtrType FilterBasePtr;


/**
 * identity filter; does nothing with the data (in other words: it's useless ;-) ).
 */
class FilterIdentity : public FilterBase
{
    protected:
        virtual std::string filterIncomingData(const std::string& data) { return data; }
        virtual std::string filterOutgoingData(const std::string& data) { return data; }
}; // eo class FilterIdentity


/**
 * @brief null filter; deletes everything it receives.
 *
 * usefull when output from a subprocess (like stderr) should be ignored...
 */
class FilterNull : public FilterBase
{
    protected:
        virtual std::string filterIncomingData(const std::string& data) { return std::string(); }
        virtual std::string filterOutgoingData(const std::string& data) { return std::string(); }
}; // eo FilterNull


/**
 * the base class of the IO classes.
 *
 * provides the functionality to read from a file descriptor and write to a file descriptor (which can be
 * identical (like for socket io), but also can be different (like pipes from/to a process)).
 * The data stream can be filtered through plugged filter objects which are building a filter chain.
 * Incoming data is filtered forward through the chain; outgoing data is filtered backward through the chain.
 * (So the first filter is the one which modifies the data closest to the connections).
 *
 * @note the functionality is provided in conjunction with the @a Backend class which handles parts of
 * the low level IO.
 *
 * @note this is a base class; it provides most "interesting" functionality in the protected section only.
 * This way, derived classes can decide which of that functionality they want to export in their own public
 * interfaces and which to keep hidden.
 */
class IOImplementation
: public boost::signals::trackable
, virtual public Utils::SharedBase
{
        friend class Backend;
        friend class FilterBase;

    public:

        typedef std::list< FilterBasePtr > FilterChain;

        typedef boost::signal< void() > SignalType;

        typedef boost::shared_ptr< IOImplementation > PtrType;

    public:
        IOImplementation(int read_fd=-1, int write_fd=-1);
        virtual ~IOImplementation();                //lint !e1509  // boost::signals::trackable is not virtual

        virtual void close(Direction direction = Direction::both);

        virtual bool wantRead();
        virtual bool wantWrite();

        virtual bool opened() const;
        virtual bool eof() const;
        virtual bool writable() const;
        virtual bool empty() const;

        virtual std::string getInput() const;
        virtual std::string getInputClear();
        virtual bool inputAvailable() const;
        virtual std::string::size_type shortenInput(std::string::size_type len);
        virtual std::string getline(char delim='\n');

    protected:

        void addFilter(FilterBasePtr filter);
        void removeFilter(FilterBasePtr);


        void lowSend(const std::string& data);

        std::string::size_type getOutputBufferSize() const { return m_output_buffer.size(); }

        void setWriteFd(int fd);
        void setReadFd(int fd);

        inline int readFd() const
        {
            return m_read_fd;
        }

        inline int writeFd() const
        {
            return m_write_fd;
        }

        inline bool isMarkedForReading() const { return m_marked_for_reading; }
        inline bool isMarkedForWriting() const { return m_marked_for_writing; }


    protected:
        virtual void doRead();
        virtual void doWrite();

        void resetReadMark()  { m_marked_for_reading= false; }
        void resetWriteMark() { m_marked_for_writing= false; }

        void injectIncomingData(FilterBasePtr from_filter, const std::string& _data);
        void injectOutgoingData(FilterBasePtr from_filter, const std::string& _data);

    protected:
        /// last error number
        int m_errno;
        /// the input buffer (i.e. the data read from @a m_read_fd)
        std::string m_input_buffer;

        /// the chain of filter which are applied to the data received are the data which should be send.
        FilterChain m_filter_chain;

        /// signal which is fired when end of file is detected
        SignalType m_signal_eof;
        /// signal which is fired when write is no longer possible
        SignalType m_signal_not_writable;
        /// signal which is fired when new data was read
        SignalType m_signal_read;
        /// signal which is fired when data was written
        SignalType m_signal_write;

        /// end of file (on @a m_read_fd) detected (used additionally when m_read_fd is valid)
        bool m_eof;

        /// unable-to-write (on @a m_write_fd) detected (used additionally when m_write_fd is valid)
        bool m_not_writable;

    private:
        /// the file descriptor to read from (-1 if none is given)
        int m_read_fd;
        /// the file descriptor to write to (-1 if none is given)
        int m_write_fd;
        /// output buffer; contains the data which needs to be written.
        std::string m_output_buffer;

    private:
        /// @a true when data is available to be read
        bool m_marked_for_reading;
        /// @a true when data can be written
        bool m_marked_for_writing;

}; // eo class IOImplementation



/**
 * same as IOImplementation, but makes fd access functions public.
 */
class IOImplementation2 : public IOImplementation
{
        typedef IOImplementation inherited;
    public:
        IOImplementation2(int read_fd=-1, int write_fd=-1) : inherited(read_fd, write_fd) {}

        void setWriteFd(int fd) { inherited::setWriteFd(fd); }
        void setReadFd(int fd)  { inherited::setReadFd(fd); }

        int readFd() const { return inherited::readFd(); }
        int writeFd() const { return inherited::writeFd(); }

}; // eo class IOImplementation2


/**
 * provides sending data and receiving data via a signal.
 *
 * @note the received data is passed as parameter to the signal and no longer stored in the received buffer.
 */
class SimpleIO : public IOImplementation
{
        typedef IOImplementation inherited;
    public:
        SimpleIO(int read_fd=-1, int write_fd=-1);
        virtual ~SimpleIO();

        void sendString(const std::string& data);

        boost::signal<void(const std::string&)> signal_received_string;

    private:

        void slotReceived();
}; // eo class SimpleIO


/**
 * provides sending data and receiving data via a signal.
 *
 * @note the received data is passed as parameter to the signal and no longer stored in the received buffer.
 */
class SimpleIO2 : public IOImplementation2
{
        typedef IOImplementation2 inherited;
    public:
        SimpleIO2(int read_fd=-1, int write_fd=-1);
        virtual ~SimpleIO2();

        void sendString(const std::string& data);

        boost::signal<void(const std::string&)> signal_received_string;

    private:

        void slotReceived();
}; // eo class SimpleIO2


/**
 * provides the backend for io handling.
 *
 * This (singleton) object provides the management of io events. It collects all wishes for reading/writing
 * from the io objects and information from the timer events.
 * It poll()s for the events and distributes them to the objects,
 *
 * The class provides the possibility for executing one io cycle (/step) or to run a backend loop.
 *
 * @note the Backend class needs to be a friend of IOImplementation since it accesses private members
 * of IOImplementation while performing the io cycles.
 */
class Backend
{
    public:
        typedef std::set< int > FdSetType;

    public:

        bool doOneStep(int ms_timeout= -1);
        void run();
        void stop();

    protected:
        Backend();
        Backend(const Backend& other);
        ~Backend();

    protected:

        /// the number of currently active backend loops
        int m_count_active_loops;
        /// the number of pending stop requests (where each one should exit one active backend loop)
        int m_count_stop_requests;

        /// the number of currently active backend cycles(/ steps)
        static int m_count_active_steps;

    public:
        static Backend* getBackend();

    protected:
        /// pointer to the active backend (which is delivered by Backend::getBackend)
        static Backend* g_backend;
}; // eo class Backend



} // eo namespace AsyncIo

#endif
