/*
Copyright (C) 2004 by Intra2net AG

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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include <container.hxx>
#include <socket_client.hxx>
#include <socket_server.hxx>
#include <command_client.hxx>
#include <command_server.hxx>
#include <client_wrapper.hxx>
#include <socket_wrapper.hxx>

#include "test_fixtures.hxx"

#include <config.h>

using namespace std;
using namespace libt2n;

// the server part

stringstream logstream;
bool close_server=false;
bool kill_server=false;

int serverfunc(int i)
{
    // magic commands
    if (i==42)
        close_server=true;
    if (i==666)
        kill_server=true;

    return i+1;
}

std::string getserverlog(void)
{
    return logstream.str();
}

class serverfunc_res : public libt2n::result
{
    private:
        int res;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);
            ar & BOOST_SERIALIZATION_NVP(res);
        }

    public:
        serverfunc_res()
            { }

        serverfunc_res(int i)
        {
            res=i;
        }

        int get_data()
        {
            return res;
        }
};

class getserverlog_res : public libt2n::result
{
    private:
        std::string res;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);
            ar & BOOST_SERIALIZATION_NVP(res);
        }

    public:
        getserverlog_res()
            { }

        getserverlog_res(std::string s)
        {
            res=s;
        }

        std::string get_data()
        {
            return res;
        }
};

class cmd_group_x : public command
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
        }
};

class serverfunc_cmd : public cmd_group_x
{
    private:
        int param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(cmd_group_x);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        serverfunc_cmd()
            { }

        serverfunc_cmd(int i)
        {
            param=i;
        }

        libt2n::result* operator()()
        {
            return new serverfunc_res(serverfunc(param));
        }
};

class getserverlog_cmd : public cmd_group_x
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(cmd_group_x);
        }

    public:
        getserverlog_cmd()
            { }

        libt2n::result* operator()()
        {
            return new getserverlog_res(getserverlog());
        }
};

BOOST_CLASS_EXPORT(serverfunc_res)
BOOST_CLASS_EXPORT(getserverlog_res)
BOOST_CLASS_EXPORT(cmd_group_x)
BOOST_CLASS_EXPORT(serverfunc_cmd)
BOOST_CLASS_EXPORT(getserverlog_cmd)

class cmd_group_x_client : public command_client
{
    public:
        cmd_group_x_client(libt2n::client_connection *_c,
         long long _command_timeout_usec=command_timeout_usec_default,
         long long _hello_timeout_usec=hello_timeout_usec_default)
         : libt2n::command_client(_c,_command_timeout_usec,_hello_timeout_usec)
        {}

    int serverfunc(int i)
    {
        libt2n::result_container rc;

        send_command(new serverfunc_cmd(i), rc);
        serverfunc_res* res=dynamic_cast<serverfunc_res*>(rc.get_result());
        if (!res) throw libt2n::t2n_communication_error("result object of wrong type");
        return res->get_data();
    }

    std::string getserverlog(void)
    {
        libt2n::result_container rc;

        send_command(new getserverlog_cmd(), rc);
        getserverlog_res* res=dynamic_cast<getserverlog_res*>(rc.get_result());
        if (!res) throw libt2n::t2n_communication_error("result object of wrong type");
        return res->get_data();
    }
};

typedef T2nSingletonWrapper<cmd_group_x_client> wraptype;

template<>
std::auto_ptr<wraptype> wraptype::SingletonObject = std::auto_ptr<wraptype>();

template<>
std::auto_ptr<ConnectionWrapper> wraptype::WrappedConnection = std::auto_ptr<ConnectionWrapper>();


class test_wrapperFixture : public KillChildOnShutdownFixture
{
public:
    test_wrapperFixture()
    {
        close_server=false;
        kill_server=false;

        switch(child_pid=fork())
        {
            case -1:
            {
                BOOST_FAIL("fork error");
                break;
            }
            case 0:
            // child
            {
                try
                {
                    int i=0;
                    while(i < 10 && !kill_server)
                    {
                        close_server=false;

                        socket_server ss("./socket");
                        group_command_server<cmd_group_x> cs(ss);
                        ss.set_logging(&logstream,debug);

                        // max 10 sec
                        for (; !close_server && !kill_server && i < 10; i++)
                            cs.handle(1000000);
                    }
                } catch(...)
                {
                    std::cerr << "exception in child. ignoring\n";
                }

                // don't call atexit and stuff
                _exit(0);
            }

            default:
            // parent
            {
                // don't kill us on broken pipe
                signal(SIGPIPE, SIG_IGN);

                // wait till server is up
                sleep(1);
            }
        }
    }

    ~test_wrapperFixture()
    {
    }
};

BOOST_FIXTURE_TEST_SUITE(test_wrapper, test_wrapperFixture)

BOOST_AUTO_TEST_CASE(no_init_exception) // must be called first!
{
    BOOST_REQUIRE_THROW(t2n_exec(&cmd_group_x_client::serverfunc)(1),std::logic_error);
}

BOOST_AUTO_TEST_CASE(simple_wrap)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new BasicSocketWrapper("./socket")));

    int i=t2n_exec(&cmd_group_x_client::serverfunc)(1);

    BOOST_CHECK_EQUAL(2,i);
}

BOOST_AUTO_TEST_CASE(double_use)
{
    // only one connection used?
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new BasicSocketWrapper("./socket")));

    t2n_exec(&cmd_group_x_client::serverfunc)(17);
    string out=t2n_exec(&cmd_group_x_client::getserverlog)();

    // count the number of times that "new connection accepted" appears in the server log
    string::size_type p=0;
    int cnt=0;
    while ((p=out.find("new connection accepted",p))++ != string::npos)
        cnt++;

    BOOST_CHECK_EQUAL(1,cnt);
}

BOOST_AUTO_TEST_CASE(double_use_with_close)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new BasicSocketWrapper("./socket")));

    t2n_exec(&cmd_group_x_client::serverfunc)(17);

    // closes the connection from the client side
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new BasicSocketWrapper("./socket")));

    string out=t2n_exec(&cmd_group_x_client::getserverlog)();

    // count the number of times that "new connection accepted" appears in the server log
    string::size_type p=0;
    int cnt=0;
    while ((p=out.find("new connection accepted",p))++ != string::npos)
        cnt++;

    BOOST_CHECK_EQUAL(2,cnt);
}

BOOST_AUTO_TEST_CASE(reconnect_after_close)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectSocketWrapper("./socket")));

    wraptype::get_connection_wrapper()->set_command_timeout_usec(3000000);
    wraptype::get_connection_wrapper()->set_hello_timeout_usec(3000000);

    // 42 closes connection on the server side
    t2n_exec(&cmd_group_x_client::serverfunc)(42);

    string out=t2n_exec(&cmd_group_x_client::getserverlog)();

    // count the number of times that "new connection accepted" appears in the server log
    string::size_type p=0;
    int cnt=0;
    while ((p=out.find("new connection accepted",p))++ != string::npos)
        cnt++;

    BOOST_CHECK_EQUAL(2,cnt);
}

BOOST_AUTO_TEST_CASE(reconnect_not_possible)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectSocketWrapper("./socket")));

    // the server doens't like the beast
    t2n_exec(&cmd_group_x_client::serverfunc)(666);

    BOOST_REQUIRE_THROW(t2n_exec(&cmd_group_x_client::serverfunc)(1),t2n_communication_error);
}

BOOST_AUTO_TEST_CASE(ignore_server_disconnect)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectIgnoreFailureSocketWrapper("./socket")));

    // the server doens't like the beast
    t2n_exec(&cmd_group_x_client::serverfunc)(666);

    int i=t2n_exec(&cmd_group_x_client::serverfunc)(1);

    // result is constructed with default constructor on error-and-ignore  -> i=0

    BOOST_CHECK_EQUAL(0,i);
}

BOOST_AUTO_TEST_CASE(ignore_handler_reconnects)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectIgnoreFailureSocketWrapper("./socket")));

    wraptype::get_connection_wrapper()->set_command_timeout_usec(3000000);
    wraptype::get_connection_wrapper()->set_hello_timeout_usec(3000000);

    // 42 closes connection on the server side
    t2n_exec(&cmd_group_x_client::serverfunc)(42);

    string out=t2n_exec(&cmd_group_x_client::getserverlog)();

    // count the number of times that "new connection accepted" appears in the server log
    string::size_type p=0;
    int cnt=0;
    while ((p=out.find("new connection accepted",p))++ != string::npos)
        cnt++;

    BOOST_CHECK_EQUAL(2,cnt);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(test_wrapper_noserver, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(ignore_noserver)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectIgnoreFailureSocketWrapper("./socket")));

    // wraptype::get_connection_wrapper()->set_logging(&cerr,debug);

    // there is no server

    int i=t2n_exec(&cmd_group_x_client::serverfunc)(1);

    // result is constructed with default constructor on error-and-ignore  -> i=0

    BOOST_CHECK_EQUAL(0,i);
}

BOOST_AUTO_TEST_CASE(ignore_finds_lateserver)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectIgnoreFailureSocketWrapper("./socket")));

    // there is no server
    t2n_exec(&cmd_group_x_client::serverfunc)(1);

    // launch a server
    close_server=false;
    kill_server=false;

    switch(child_pid=fork())
    {
        case -1:
        {
            BOOST_FAIL("fork error");
            break;
        }
        case 0:
        // child
        {
            try
            {
                int i=0;
                while(i < 10 && !kill_server)
                {
                    close_server=false;

                    socket_server ss("./socket");
                    group_command_server<cmd_group_x> cs(ss);
                    ss.set_logging(&logstream,debug);

                    // max 10 sec
                    for (; !close_server && !kill_server && i < 10; i++)
                        cs.handle(1000000);
                }
            } catch(...)
            {
                std::cerr << "exception in child. ignoring\n";
            }

            // don't call atexit and stuff
            _exit(0);
        }

        default:
        // parent
        {
            // wait till server is up
            sleep(1);
        }
    }

    // server should be active
    int i=t2n_exec(&cmd_group_x_client::serverfunc)(1);

    BOOST_CHECK_EQUAL(2,i);
}

BOOST_AUTO_TEST_CASE(ignore_wrongserver)
{
    wraptype::set_connection(auto_ptr<ConnectionWrapper>
        (new ReconnectIgnoreFailureSocketWrapper("./socket")));

    // launch a server

    switch(child_pid=fork())
    {
        case -1:
        {
            BOOST_FAIL("fork error");
            break;
        }
        case 0:
        // child
        {
            try
            {
                socket_server ss("./socket");

                // server sends garbage

                ostringstream hello;
                hello << "XYZ 123";

                ss.add_callback(new_connection,bind(&test_wrapper_noserver::ignore_wrongserver::send_hello, boost::ref(*this), hello.str(),&ss, _1));

                // max 10 sec
                for (int i=0; i < 10; i++)
                    ss.fill_buffer(1000000);
            } catch(...)
            {
                std::cerr << "exception in child. ignoring\n";
            }

            // don't call atexit and stuff
            _exit(0);
        }

        default:
        // parent
        {
            // wait till server is up
            sleep(1);
        }
    }

    // there is no valid server

    int i=t2n_exec(&cmd_group_x_client::serverfunc)(1);

    // result is constructed with default constructor on error-and-ignore  -> i=0

    BOOST_CHECK_EQUAL(0,i);
}

BOOST_AUTO_TEST_SUITE_END()
