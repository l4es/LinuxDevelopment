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
 *
 * (c) Copyright 2007-2010 by Intra2net AG
 */

//#define NOISEDEBUG

#include <string>
#include <iostream>
#include <iomanip>
#include <vector>

#include <async_io.hpp>
#include <async_pipe.hpp>
#include <async_process.hpp>
#include <async_timer.hpp>
#include <async_callout.hpp>
#include <async_socket.hpp>
#include <asyncio_system_tools.hpp>
#include <asyncio_containerfunc.hpp>

#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <boost/random.hpp>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#ifdef NOISEDEBUG
#define DOUT(msg) std::cout << msg << std::endl
#else
#define DOUT(msg) do {} while (0)
#endif


using namespace I2n;
using namespace AsyncIo;

namespace {


struct Counter
{
    int value;


    Counter() : value(0) { DOUT("Counter construct");}

    void reset() { value=0;}

    void advance()
    {
        DOUT(" advance called");
        ++value;
    }

    int operator()()
    {
        DOUT(" () called");
        return ++value;
    }
}; // eo struct Counter


class TestTimer : public TimerBase
{
    public:

        TestTimer()
        : m_counter(0u)
        {
        } // eo TestTimer()


        void setDelta(long msec)
        {
            setDeltaWhenTime(0,msec);
            activate();
        } // eo setDelta(long)


        unsigned m_counter;

    protected:

        virtual void execute()
        {
            ++m_counter;
        } // eo execute()

}; // eo class TestTimer



struct ReceivedData
{
    std::vector<std::string> m_received_vector;
    std::string m_received_string;

    unsigned long m_count_lines;
    unsigned long m_data_size;


    void resetReceivedData()
    {
        m_received_vector.clear();
        m_received_string.clear();
        m_count_lines= 0uL;
        m_data_size= 0uL;
    } // eo reset


    void receiveData(const std::string& data)
    {
        m_received_vector.push_back(data);
        m_received_string.append(data);
        ++m_count_lines;
        m_data_size += data.size();
    } // eo receiveData(const std::string&)

}; // eo struct ReceivedData



class TestPipe : public SimplePipe, public ReceivedData
{
    public:
        TestPipe()
        : SimplePipe()
        {
            signal_received_string.connect( boost::bind(&TestPipe::receiveData, this, _1) );
        } // eo TestPipe()



    protected:

}; // eo class TestPipe


class TestIO : public SimpleIO2, public ReceivedData
{
    public:
        TestIO()
        {
            signal_received_string.connect( boost::bind(&TestIO::receiveData, this, _1) );
        }

}; // eo TestIO


class TestProcess : public ProcessImplementation, public ReceivedData
{
    public:
        TestProcess(
            const std::string& path,
            const std::vector<std::string>& args= std::vector<std::string>() )
        : ProcessImplementation(path,args)
        {
            m_signal_read.connect( boost::bind(&TestProcess::slotReceivedData, this) );
        } // eo Testprocess(const std::string&, const std::vector<std::string>&)

        TestProcess(
            const std::string& path,
            const std::string& arg1 )
        : ProcessImplementation(
            path,
            TransientPushBackFiller< std::string,std::vector >()(arg1)
          )
        {
            m_signal_read.connect( boost::bind(&TestProcess::slotReceivedData, this) );
        } // eo Testprocess(const std::string&, const std::vector<std::string>&)

        TestProcess(
            const std::string& path,
            const std::string& arg1, const std::string& arg2 )
        : ProcessImplementation(
            path,
            TransientPushBackFiller< std::string, std::vector >()(arg1)(arg2)
          )
        {
            m_signal_read.connect( boost::bind(&TestProcess::slotReceivedData, this) );
        } // eo Testprocess(const std::string&, const std::vector<std::string>&)

    protected:

        void slotReceivedData()
        {
            receiveData(m_input_buffer);
            m_input_buffer.clear();
        } // eo slotReceivedData()

}; // eo class TestProcess



class TestUnixIOSocket
: public UnixIOSocket
, public ReceivedData
{
    public:

        TestUnixIOSocket()
        : UnixIOSocket()
        {
            m_signal_read.connect( boost::bind(&TestUnixIOSocket::slotReceivedData, this) );
        } // eo TestUnixIOSocket()


        TestUnixIOSocket(
            int fd, const std::string& path
        )
        : UnixIOSocket(fd, path)
        {
            m_signal_read.connect( boost::bind(&TestUnixIOSocket::slotReceivedData, this) );
        } // eo TestUnixIOSocket()


        void sendData(const std::string& data)
        {
            lowSend(data);
        } // eo sendData(const std::string&)

    protected:

        void slotReceivedData()
        {
            receiveData(m_input_buffer);
            m_input_buffer.clear();
        } // eo slotReceivedData()

}; // eo class TestUnixIOSocket

typedef boost::shared_ptr< TestUnixIOSocket > TestUnixIOSocketPtr;


class UnixIOSocketHolder
: public std::vector< UnixIOSocketPtr >
{
    public:

        void operator()(UnixIOSocketPtr ptr)
        {
            push_back(ptr);
        }

        void storeBase (IOImplementationPtr ptr)
        {
            push_back(boost::dynamic_pointer_cast< UnixIOSocket >(ptr) );
        }

        void store (UnixIOSocketPtr ptr)
        {
            push_back(ptr);
        }

        TestUnixIOSocketPtr get(int idx)
        {
            return boost::dynamic_pointer_cast<
                TestUnixIOSocket
            >( (*this)[idx] );
        }
}; // eo class UnixIOSocketHolder


/// global random generator (from boost lib).
boost::mt19937 g_random_gen;


/**
 * generates a string with random characters from a given ASCII subset.
 * @param len the desired length of the output string
 * @return a random string of length @a len
 */
std::string makeRandomAsciiString(std::string::size_type len)
{
    static std::string chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz=+-*/%(){}<>.,:;\\");
    std::string s;

    boost::uniform_int<> discreter(0, chars.size()-1);
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > idxgen(g_random_gen, discreter);

    for(; len-->0;) s+= chars.at( idxgen() );

    return s;
} // eo makeRandomAsciiString


} // eo namespace <anonymous>


class SimpleioBasicsFixture
{
protected:
    Backend *backend;
    std::set<std::string>  used_check_files;

    template<class Callable>
    bool backendLoopUntil( Callable condition, int maxLoops=100 )
    {
        for (int i=std::max(maxLoops,1); i-->0 && ! condition();)
        {
            backend->doOneStep(10);
        }
        return condition();
    } // eo backendLoopUntil


    bool backendStep(int msTimeout= 10, int count=1)
    {
        bool res= true;
        for(;count-->0 && res;)
        {
            res= backend->doOneStep(msTimeout);
        }
        return res;
    } // eo backendStep


    std::string getCheckFilepath(std::string tag)
    {
        std::string result;
        result= "__unittest__" + tag + ".dat";
        used_check_files.insert(result);
        return result;
    } // eo get_check_file_path


    void removeCheckFiles()
    {
        for(std::set<std::string>::iterator it= used_check_files.begin();
            it != used_check_files.end();
            ++it)
        {
            std::string filepath(*it);
            if (Utils::FileStat(filepath))
            {
                Utils::unlink(filepath);
            }
        }
        used_check_files.clear();
    } // eo removeCheckFiles

public:
    SimpleioBasicsFixture()
    {
        backend = Backend::getBackend();
        installChildHandler();
        used_check_files.clear();
    }

    ~SimpleioBasicsFixture()
    {
        restoreChildHandler();
        removeCheckFiles();
    }
};

BOOST_FIXTURE_TEST_SUITE(TestSimpleioBasics, SimpleioBasicsFixture)

BOOST_AUTO_TEST_CASE(EmptyBackendStepCall)
{
    BOOST_REQUIRE( backend );

    // a backend call without active objects should return false:
    bool result = backend->doOneStep(0);

    BOOST_CHECK_EQUAL( false, result );
} // eo EmptyBackendStepCall



BOOST_AUTO_TEST_CASE(NonEmptyBackendStepCall)
{
    BOOST_REQUIRE(backend);

    {
        TestTimer timer;
        timer.setDelta(10);
        // with an active object, a step should return true:
        bool result = backend->doOneStep(0);
        BOOST_CHECK_EQUAL( true, result );
        // the timer should not be executed:
        BOOST_CHECK_EQUAL( 0u, timer.m_counter );
    }
    // now it should return false:
    bool result = backend->doOneStep(0);
    BOOST_CHECK_EQUAL( false, result );
} // eo NonEmptyBackendStepCall



/**
    * check for timer to execute immediatly.
    */
BOOST_AUTO_TEST_CASE(SingleTimerShot)
{
    BOOST_REQUIRE(backend);

    TestTimer timer;
    timer.setDelta(0); // shot now!

    bool result = backend->doOneStep(10);

    BOOST_CHECK_EQUAL( true, result );
    // the timer should be executed once:
    BOOST_CHECK_EQUAL( 1u, timer.m_counter );

    result = backend->doOneStep(0);

    BOOST_CHECK_EQUAL( false, result );
    // the timer should not be executed again:
    BOOST_CHECK_EQUAL( 1u, timer.m_counter );

} // eo SingleTimerShot()



/**
    * tests a simple timer class to be executed with a timeout.
    */
BOOST_AUTO_TEST_CASE(SimpleTimerShot)
{
    bool res;
    BOOST_REQUIRE(backend);

    SimpleTimer timer1;
    Counter counter1;
    timer1.addAction( boost::bind(&Counter::advance,&counter1) );
    BOOST_CHECK_EQUAL(false, timer1.active());

    timer1.startTimerMS( 100 );
    BOOST_CHECK_EQUAL(true, timer1.active());

    res=backend->doOneStep( 1000 );
    BOOST_CHECK_EQUAL( true, res );

    BOOST_CHECK_EQUAL( 1, counter1.value );
} // eo SimpleTimerShot



/**
    * tests 3 timers; after the first was active, disable another and check if the remaining one fires.
    */
BOOST_AUTO_TEST_CASE(SimpleTimerShot2)
{
    bool res;
    BOOST_REQUIRE(backend);

    SimpleTimer timer1, timer2, timer3;
    Counter counter1, counter2, counter3;
    timer1.addAction( boost::bind(&Counter::advance,&counter1) );
    timer2.addAction( boost::bind(&Counter::advance,&counter2) );
    timer3.addAction( boost::bind(&Counter::advance,&counter3) );
    BOOST_CHECK_EQUAL(false, timer1.active());
    BOOST_CHECK_EQUAL(false, timer2.active());
    BOOST_CHECK_EQUAL(false, timer3.active());

    timer1.startTimerMS( 100 );
    timer2.startTimerMS( 500 );
    timer3.startTimerMS( 400 );
    BOOST_CHECK_EQUAL(true, timer1.active());
    BOOST_CHECK_EQUAL(true, timer2.active());
    BOOST_CHECK_EQUAL(true, timer3.active());

    res=backend->doOneStep( 1000 );
    BOOST_CHECK_EQUAL( true, res );

    BOOST_CHECK_EQUAL(false, timer1.active());
    BOOST_CHECK_EQUAL(true, timer2.active());
    BOOST_CHECK_EQUAL(true, timer3.active());

    BOOST_CHECK_EQUAL( 1, counter1.value );
    BOOST_CHECK_EQUAL( 0, counter2.value );
    BOOST_CHECK_EQUAL( 0, counter3.value );

    // now stop the next timer:
    timer3.stopTimer();
    BOOST_CHECK_EQUAL(false, timer3.active());

    res=backend->doOneStep( 1000 );
    BOOST_CHECK_EQUAL( true, res );

    BOOST_CHECK_EQUAL( 1, counter1.value );
    BOOST_CHECK_EQUAL( 1, counter2.value );
    BOOST_CHECK_EQUAL( 0, counter3.value );
} // eo SimpleTimerShot2




/*
** I/O tests:
*/

BOOST_AUTO_TEST_CASE(EmptyWantTest)
{
    IOImplementation io;

    BOOST_CHECK_EQUAL(false, io.wantRead() );
    BOOST_CHECK_EQUAL(false, io.wantWrite() );
} // eo EmptyWantTest


/**
    * a simple pipe (and io) test.
    *
    * This test basically tests the io framework.
    * It opens two connected pipes and sends a test string in each direction.
    * It tests the following functionalities of the base classes:
    *   - set write marks in backend step (enabling direct send of data)
    *   - low send data
    *   - construct and interpret poll() data structures
    *   - receive data
    *   - signal chains for received data
    *   - eof detection
    *   .
    *
    */
BOOST_AUTO_TEST_CASE(SimplePipeTest)
{
    static const std::string test_string("a test string");
    static const std::string test_string2("only another short test string");

    BOOST_REQUIRE(backend);

    TestPipe pipe1, pipe2;

    bool res= pipe1.makePipe(pipe2);

    BOOST_CHECK_EQUAL(true, res);
    BOOST_CHECK_EQUAL(true, pipe1.opened());
    BOOST_CHECK_EQUAL(true, pipe2.opened());

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    pipe1.sendString(test_string);

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    BOOST_CHECK_EQUAL( test_string, pipe2.m_received_string );

    pipe2.sendString(test_string2);

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    BOOST_CHECK_EQUAL( test_string2, pipe1.m_received_string );

    pipe1.close();
    BOOST_CHECK_EQUAL(false, pipe1.opened());

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    BOOST_CHECK_EQUAL(true, pipe2.eof());
} // eo SimplePipeTest



/**
    * sends a larger data chunk through a pipe.
    * This tests if sending and receiving data in (smaller internal) chunks works.
    */
BOOST_AUTO_TEST_CASE(SimplePipePump)
{
    BOOST_REQUIRE(backend);

    TestPipe pipe1, pipe2;

    bool res= pipe1.makePipe(pipe2);

    BOOST_CHECK_EQUAL(true, res);
    BOOST_CHECK_EQUAL(true, pipe1.opened());
    BOOST_CHECK_EQUAL(true, pipe2.opened());

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    std::string test_string= makeRandomAsciiString(256*1024);

    pipe1.sendString(test_string);

    res= backend->doOneStep(0);
    BOOST_CHECK_EQUAL(true, res);

    // do some backend cycles to empty the pipe:
    for (int i=64; i-->0 && res && !pipe1.empty(); )
    {
        res= backend->doOneStep(100);
    };

    pipe1.close();
    BOOST_CHECK_EQUAL(false, pipe1.opened());

    // now read the remaining data until we recognize EOF:
    for (int i=64; i-->0 && res && !pipe2.eof();)
    {
        res= backend->doOneStep(100);
    }

    BOOST_CHECK_EQUAL( test_string.size(), pipe2.m_received_string.size() );
    BOOST_CHECK_EQUAL( test_string, pipe2.m_received_string );

    BOOST_CHECK_EQUAL(true, pipe2.eof());
} // eo SimplePipePump



/**
    * fork a subprocess (/bin/true) and test exit code.
    */
BOOST_AUTO_TEST_CASE(SimpleProcessTestBinTrue)
{
    bool res;
    BOOST_REQUIRE(backend);

    TestProcess proc("/bin/true");

    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(200);
    BOOST_CHECK_EQUAL(true, res);

    for(int i=20; i-->0 && proc.processState() != ProcessState::stopped;)
    {
        backend->doOneStep(15);
    }

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( 0, proc.exitCode() );
} // eo SimpleProcessTestBinTrue


/**
    * fork a subprocess (/bin/false) and test exit code.
    */
BOOST_AUTO_TEST_CASE(SimpleProcessTestBinFalse)
{
    bool res;
    BOOST_REQUIRE(backend);

    TestProcess proc("/bin/false");

    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(200);
    BOOST_CHECK_EQUAL(true, res);
    for(int i=20; i-->0 && proc.processState() != ProcessState::stopped;)
    {
        backend->doOneStep(15);
    }

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( 1, proc.exitCode() );
    DOUT("leave SimpleProcessTestBinFalse");
} // eo SimpleProcessTestBinFalse


/**
    * fork an echo subprocess and read back the output.
    */
BOOST_AUTO_TEST_CASE(SimpleProcessTestEcho)
{
    DOUT("enter SimpleProcessTestEcho");
    bool res;
    BOOST_REQUIRE(backend);

    TestProcess proc(
        "/bin/echo",
        TransientPushBackFiller<std::string, std::vector >()("Eine")("Zeichenkette")
    );

    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(200);
    BOOST_CHECK_EQUAL(true, res);
    for(int i=20; i-->0 && (proc.processState()!= ProcessState::stopped || !proc.eof());)
    {
        backend->doOneStep(10);
    }

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( 0, proc.exitCode() );
    BOOST_CHECK_EQUAL( std::string("Eine Zeichenkette\n"), proc.m_received_string);
} // eo SimpleProcessTestEcho



/**
    * fork a bash subprocess, echo something on stderr and read back the output.
    */
BOOST_AUTO_TEST_CASE(SimpleProcessTestStderr)
{
    bool res;
    BOOST_REQUIRE(backend);

    TestIO my_stderr;

    TestProcess proc(
        "/bin/bash",
        TransientPushBackFiller<std::string, std::vector >()
            ("-c")
            ("echo Eine Zeichenkette >&2")
    );

    // start with a seperate io object for stderr.
    DOUT("## start process");
    res= proc.startProcess( &my_stderr );
    BOOST_CHECK_EQUAL(true, res);
    BOOST_CHECK_EQUAL(true, my_stderr.opened());

    DOUT("## do a backend step");
    res= backend->doOneStep(200);
    BOOST_CHECK_EQUAL(true, res);
    // wait until process stopped and both io's signal EOF (or until the loop ends ;-) )
    DOUT("## enter loop");
    for(int i=17; i-->0 && (proc.processState()!= ProcessState::stopped || !proc.eof() || !my_stderr.eof());)
    {
        DOUT("## round i=" << i);
        backend->doOneStep(10);
    }
    DOUT("## loop left");

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( true, my_stderr.eof() );
    BOOST_CHECK_EQUAL( 0, proc.exitCode() );
    BOOST_CHECK_EQUAL( std::string("Eine Zeichenkette\n"), my_stderr.m_received_string);
    DOUT("leave Test SimpleProcessTestStderr");
} // eo SimpleProcessTestStderr



/**
    * checks termination of process by signal and if the signal is returned.
    */
BOOST_AUTO_TEST_CASE(SignaledProcessTermination)
{
    bool res;
    BOOST_REQUIRE(backend);

    TestProcess proc("/bin/sleep","2");
    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(10);
    BOOST_CHECK_EQUAL(true, res);
    BOOST_CHECK_EQUAL( ProcessState(ProcessState::running), proc.processState() );

    res= backend->doOneStep(50);

    // now send the process an USR1 (which terminates the process)
    res=proc.kill( Signal::USR1 );
    BOOST_CHECK_EQUAL(true, res);

    // give the backend a chance to process the termination event:
    for(int i=30; i-->0 && proc.processState()!=ProcessState::stopped;) backend->doOneStep(10);

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( Signal::USR1 , proc.exitCode()>>8 );
} // eo SignaledProcessTermination

/**
    * fork an echo subprocess and read back the output. Directly use ProcessImplementation without helper class.
    */
BOOST_AUTO_TEST_CASE(DirectProcessImplementationTestEcho)
{
    DOUT("enter SimpleProcessTestEcho");
    bool res;
    BOOST_REQUIRE(backend);

    ProcessImplementation proc(
        "/bin/echo",
        TransientPushBackFiller<std::string, std::vector >()("Eine")("Zeichenkette")
    );

    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(200);
    BOOST_CHECK_EQUAL(true, res);
    for(int i=20; i-->0 && (proc.processState()!= ProcessState::stopped || !proc.eof());)
    {
        backend->doOneStep(10);
    }

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( 0, proc.exitCode() );
    BOOST_CHECK_EQUAL( std::string("Eine Zeichenkette\n"), proc.getInput());
    BOOST_CHECK_EQUAL( 5, proc.shortenInput(5));
    BOOST_CHECK_EQUAL( std::string("Zeichenkette\n"), proc.getInput());
    BOOST_CHECK_EQUAL( true, proc.inputAvailable());
    BOOST_CHECK_EQUAL( std::string("Zeichenkette\n"), proc.getInputClear());
    BOOST_CHECK_EQUAL( false, proc.inputAvailable());
} // eo SimpleProcessTestEcho

BOOST_AUTO_TEST_CASE(DirectProcessImplementationGetline)
{
    DOUT("enter SimpleProcessTestEcho");
    bool res;
    BOOST_REQUIRE(backend);

    ProcessImplementation proc(
        "/bin/sh",
        TransientPushBackFiller<std::string, std::vector >()("-c")("echo -n 1; sleep 1; echo 2; echo")
    );

    res= proc.startProcess();
    BOOST_CHECK_EQUAL(true, res);

    res= backend->doOneStep(800);
    BOOST_CHECK_EQUAL(true, res);

    // echo -n 1 should be out by now

    BOOST_CHECK_EQUAL( std::string(""), proc.getline());

    for(int i=20; i-->0 && (proc.processState()!= ProcessState::stopped || !proc.eof());)
    {
        backend->doOneStep(100);
    }

    BOOST_CHECK_EQUAL( ProcessState(ProcessState::stopped), proc.processState() );
    BOOST_CHECK_EQUAL( true, proc.eof() );
    BOOST_CHECK_EQUAL( 0, proc.exitCode() );
    BOOST_CHECK_EQUAL( std::string("12\n"), proc.getline());
    BOOST_CHECK_EQUAL( std::string("\n"), proc.getline());
    BOOST_CHECK_EQUAL( false, proc.inputAvailable());
} // eo SimpleProcessTestEcho

BOOST_AUTO_TEST_CASE(CallOut1)
{
    Counter count;

    callOut( boost::bind(&Counter::advance, &count), 1 );
    backend->doOneStep( 10 );

    BOOST_CHECK_EQUAL( 0, count.value );
    backend->doOneStep( 1100 );

    BOOST_CHECK_EQUAL( 1, count.value );
} // eo CallOut1()



BOOST_AUTO_TEST_CASE(CallOut2)
{
    Counter count;

    callOut( boost::bind(&Counter::advance, &count), 0.5 );
    backend->doOneStep( 10 );

    BOOST_CHECK_EQUAL( 0, count.value );
    backend->doOneStep( 800 );

    BOOST_CHECK_EQUAL( 1, count.value );
} // eo CallOut2()



BOOST_AUTO_TEST_CASE(RemoveCallOut1)
{
    Counter count;

    CallOutId id= callOut( boost::bind(&Counter::advance, &count), 1 );
    backend->doOneStep( 10 );

    BOOST_CHECK_EQUAL( 0, count.value );
    bool res1 = removeCallOut(id);
    bool res2 = removeCallOut(id);

    BOOST_CHECK_EQUAL( true, res1 );
    BOOST_CHECK_EQUAL( false, res2 );

    backend->doOneStep( 1100 );

    BOOST_CHECK_EQUAL( 0, count.value );
} // eo RemoveCallOut1()



BOOST_AUTO_TEST_CASE(FrozenCall_Thaw)
{
    Counter count;

    CallOutId id= frozenCall( boost::bind(&Counter::advance, &count), 1 );
    backend->doOneStep( 10 );

    BOOST_CHECK_EQUAL( 0, count.value );
    id.thaw();

    backend->doOneStep( 1100 );

    BOOST_CHECK_EQUAL( 1, count.value );
} // eo FrozenCall_Thaw()



BOOST_AUTO_TEST_CASE(FrozenCall_Decay)
{
    Counter count;

    CallOutId id= frozenCall( boost::bind(&Counter::advance, &count), 1 );
    backend->doOneStep( 10 );

    BOOST_CHECK_EQUAL( 0, count.value );
    BOOST_CHECK_EQUAL( true, id.active() );
    backend->doOneStep( 1100 );

    BOOST_CHECK_EQUAL( 0, count.value );
    BOOST_CHECK_EQUAL( false, id.active() );
} // eo FrozenCall_Decay()



BOOST_AUTO_TEST_CASE(UnixSockets_ClientServer)
{
    std::string path= getCheckFilepath("UDS_CS");

    UnixIOSocketHolder server_holder;
    UnixServerSocket< TestUnixIOSocket > server_port;
    UnixIOSocketPtr server;
    TestUnixIOSocket client0;
    TestUnixIOSocket client1;

    bool res1 = server_port.open(path, 0600);
    BOOST_CHECK_EQUAL( true, res1 );

    {
        AsyncIo::Utils::FileStat stat(path,false);
        BOOST_REQUIRE( stat.is_socket() );
        BOOST_CHECK_EQUAL( 0600u, (stat.mode() & 0777));
    }

    server_port.setNewConnectionCallback(
        boost::bind( &UnixIOSocketHolder::store, &server_holder, _1)
    );

    // open a first client
    bool res2= client0.open(path);
    BOOST_CHECK_EQUAL( true, res2 );

    BOOST_CHECK_EQUAL(0u, server_holder.size() );
    backendStep(5,1);
    BOOST_CHECK_EQUAL(1u, server_holder.size() );
    BOOST_REQUIRE( server_holder.get(0).get() );

    client0.sendData("a simple test string.");
    backendStep(3,2);

    BOOST_CHECK_EQUAL(
        std::string("a simple test string."),
        server_holder.get(0)->m_received_string
    );
    server_holder.get(0)->sendData("reply 1");
    backendStep(3,2);
    BOOST_CHECK_EQUAL( std::string("reply 1"), client0.m_received_string );

    // open a second client
    res2= client1.open(path);
    BOOST_CHECK_EQUAL( true, res2 );
    backendStep(5,1);
    BOOST_CHECK_EQUAL(2u, server_holder.size() );
    BOOST_REQUIRE( server_holder.get(1).get() );

    server_holder.get(1)->sendData("::reply 2");
    backendStep(3,2);
    BOOST_CHECK_EQUAL( std::string("::reply 2"), client1.m_received_string );

    client1.sendData("another simple test string. 124");
    backendStep(3,2);

    BOOST_CHECK_EQUAL(
        std::string("another simple test string. 124"),
        server_holder.get(1)->m_received_string
    );

    // close first client
    client0.close();
    BOOST_CHECK_EQUAL( false, server_holder.get(0)->eof() );
    backendStep(3,2);
    BOOST_CHECK_EQUAL( true, server_holder.get(0)->eof() );
    server_holder.get(0)->close();

    // close second connection from server side
    BOOST_CHECK_EQUAL( false, client1.eof() );
    server_holder.get(1)->close();
    backendStep(3,2);
    BOOST_CHECK_EQUAL( true, client1.eof() );
    client1.close();
} // eo UnixSockets_ClientServer()


/*
BOOST_AUTO_TEST_CASE(Dummy)
{
    using namespace std;
    cout << endl << "Random strings:" << endl;
    for (int i=10; i-->0;)
    {
        cout << "  " << makeRandomAsciiString(70)<< endl;
    }
} // eo Dummy
*/

BOOST_AUTO_TEST_SUITE_END()
