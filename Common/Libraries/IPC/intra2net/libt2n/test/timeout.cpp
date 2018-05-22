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

#include <boost/bind.hpp>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include <container.hxx>
#include <socket_client.hxx>
#include <socket_server.hxx>
#include <command_client.hxx>
#include <command_server.hxx>

#include "test_fixtures.hxx"

#include <config.h>

using namespace std;
using namespace libt2n;

string testfunc2(const string& str)
{
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    string ret(str);
    ret+=", testfunc() was here";
    return ret;
}

class testfunc2_res : public libt2n::result
{
    private:
        string res;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);
            ar & BOOST_SERIALIZATION_NVP(res);
        }

    public:
        testfunc2_res()
            { }

        testfunc2_res(const string& str)
        {
            res=str;
        }

        string get_data()
        {
            return res;
        }
};


class testfunc2_cmd : public libt2n::command
{
    private:
        string param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        testfunc2_cmd()
            { }

        testfunc2_cmd(const string& str)
        {
            param=str;
        }

        libt2n::result* operator()()
        {
            return new testfunc2_res(testfunc2(param));
        }
};

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(testfunc2_cmd)
BOOST_CLASS_EXPORT(testfunc2_res)

class test_timeoutFixture : public KillChildOnShutdownFixture
{
protected:
    typedef uint32_t packet_size_indicator;

    // this is an evil hack to get access to real_write, don't ever do this in an app!!!
    class real_write_client_connection: public socket_client_connection
    {
        public:
            void real_write(const std::string& data)
                { socket_write(data); }
    };

    void send_slow_raw_socket(string data, socket_server* ss, unsigned int conn_id)
    {
        socket_server_connection *ssc=dynamic_cast<socket_server_connection*>(ss->get_connection(conn_id));

        // this is an evil hack to get access to real_write, don't ever do this in an app!!!
        real_write_connection *rwc=(real_write_connection*)ssc;

        // we write one char each 0.2 sec
        for (int pos=0; pos < data.size(); pos++)
        {
            string onebyte;
            onebyte+=data[pos];
            rwc->real_write(onebyte);
            usleep(200000);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(test_timeout, test_timeoutFixture)

BOOST_AUTO_TEST_CASE(ConnectTimeout)
{
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
            string data;

            // wait till server is up
            sleep(1);

            string errormsg;

            socket_client_connection sc("./socket");

            BOOST_CHECK_MESSAGE(sc.connection::is_closed() == true, "connection not closed");

            BOOST_CHECK_MESSAGE(sc.get_last_error_msg() == string("no more retries left after connect error"), "wrong errormessage");
        }
    }
}

BOOST_AUTO_TEST_CASE(HelloTimeoutNothing)
{
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
            string data;

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");
            command_client cc(&sc,1000000,1000000);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("timeout exceeded"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(HelloTimeoutSlowData)
{
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

                // create a valid packet
                ostringstream hello;
                hello << "T2Nv" << PROTOCOL_VERSION << ';';
                int byteordercheck=1;
                hello.write((char*)&byteordercheck,sizeof(byteordercheck));
                hello << ';';

                packet_size_indicator psize=htonl(hello.str().size());
                std::string send_data(hello.str());
                send_data.insert(0,(char*)&psize,sizeof(packet_size_indicator));

                ss.add_callback(new_connection,bind(&test_timeout::HelloTimeoutSlowData::send_slow_raw_socket, boost::ref(*this), send_data,&ss, _1));

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
            string data;

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");
            command_client cc(&sc,1000000,1000000);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("timeout exceeded"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(CommandTimeout)
{
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

                ostringstream hello;
                hello << "T2Nv" << PROTOCOL_VERSION << ';';
                int byteordercheck=1;
                hello.write((char*)&byteordercheck,sizeof(byteordercheck));
                hello << ';';

                ss.add_callback(new_connection,bind(&test_timeout::CommandTimeout::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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
            string data;

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            command_client cc(&sc,1000000,1000000);
            result_container rc;

            string errormsg;

            try
            {
                cc.send_command(new testfunc2_cmd("hello"),rc);
            }
            catch(t2n_transfer_error &e)
            { errormsg=e.what(); }

            BOOST_CHECK_EQUAL(string("timeout exceeded"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(CommandSlowResponse)
{
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

                ostringstream hello;
                hello << "T2Nv" << PROTOCOL_VERSION << ';';
                int byteordercheck=1;
                hello.write((char*)&byteordercheck,sizeof(byteordercheck));
                hello << ';';

                ss.add_callback(new_connection,bind(&test_timeout::CommandSlowResponse::send_hello, boost::ref(*this), hello.str(),&ss, _1));

                // max 10 sec
                for (int i=0; i < 10; i++)
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        // create a valid packet & send
                        string response="abcdefghijklmnopqrstuvwxyz";
                        packet_size_indicator psize=htonl(response.size());
                        std::string send_data(response);
                        send_data.insert(0,(char*)&psize,sizeof(packet_size_indicator));
                        send_slow_raw_socket(send_data,&ss,cid);
                    }
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
            string data;

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            command_client cc(&sc,1000000,1000000);
            result_container rc;

            string errormsg;

            try
            {
                cc.send_command(new testfunc2_cmd("hello"),rc);
            }
            catch(t2n_transfer_error &e)
            { errormsg=e.what(); }

            BOOST_CHECK_EQUAL(string("timeout exceeded"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(DisconnectOnWrite)
{
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

                // bail out as soon as we get something
                ss.fill_buffer(-1);
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
            string data;

            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            string errormsg;

            string huge(5000000,'x');

            try
            {
                sc.write(huge);
            }
            catch(t2n_transfer_error &e)
            { errormsg=e.what(); }

            BOOST_CHECK_EQUAL(string("write() returned Broken pipe"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(WriteTwice)
{
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

                // bail out as soon as we get something
                ss.fill_buffer(-1);
                ss.fill_buffer(-1);
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
            string data;

            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            string errormsg;

            sc.write("somedata");

            sleep(1);

            // server should disconnect now
            try
            {
                sc.write("other data(2)");
                sleep(1);
                sc.write("other data(3)");
            }
            catch(t2n_transfer_error &e)
            { errormsg=e.what(); }

            BOOST_CHECK_EQUAL(string("write() returned Broken pipe"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(DisconnectOnRead)
{
    pid_t pid2;

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
                // wait till server is up
                sleep(1);

                socket_client_connection sc("./socket");

                // this is an evil hack to get access to real_write, don't ever do this in an app!!!
                real_write_client_connection *rwc=(real_write_client_connection*)&sc;
                rwc->real_write(string(10000,'x'));
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

            socket_server ss("./socket");

            time_t t0 = time(NULL);

            // max 5 sec
            while (time(NULL) < t0 + 5 )
            {
                ss.fill_buffer(1000000);

                string data;
                ss.get_packet(data);
            }

            // are we still alive and able to process data?

            switch(pid2=fork())
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
                        socket_client_connection *sc=new socket_client_connection("./socket");
                        sc->write(string(10000,'x'));
                        // socket is closed regularly
                        delete sc;
                    } catch(...)
                    {
                        std::cerr << "exception in child. ignoring\n";
                    }

                    // don't run regular cleanup, otherwise cppunit stuff gets called
                    _exit(0);
                }

                default:
                // parent
                {
                    string received;

                    t0 = time(NULL);

                    // max 10 sec
                    while (time(NULL) < t0 + 10 )
                    {
                        ss.fill_buffer(1000000);

                        if (ss.get_packet(received))
                            break;
                    }

                    BOOST_CHECK_EQUAL(string(10000,'x'),received);
                }
            }
        }
    }
    kill(pid2,SIGKILL);
}

BOOST_AUTO_TEST_CASE(BreakAccept)
{
    pid_t pid2;

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
                // wait till server is really up and waiting
                sleep(2);

                // connect with very tight timeout and only 1 retry
                socket_client_connection sc("./socket",50,1);
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

            socket_server ss("./socket");

            // server is "working" while client wants to connect
            sleep(5);

            time_t t0 = time(NULL);

            // max 5 sec
            while (time(NULL) < t0 + 5 )
            {
                ss.fill_buffer(1000000);

                string data;
                ss.get_packet(data);
            }

            // are we still alive and able to process data?

            switch(pid2=fork())
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
                        socket_client_connection *sc=new socket_client_connection("./socket");
                        sc->write(string(10000,'x'));
                        delete sc;
                        // socket is closed regularly
                    } catch(...)
                    {
                        std::cerr << "exception in child. ignoring\n";
                    }

                    // don't run regular cleanup, otherwise cppunit stuff gets called
                    _exit(0);
                }

                default:
                // parent
                {
                    string received;

                    t0 = time(NULL);

                    // max 10 sec
                    while (time(NULL) < t0 + 10 )
                    {
                        ss.fill_buffer(1000000);

                        if (ss.get_packet(received))
                            break;
                    }

                    BOOST_CHECK_EQUAL(string(10000,'x'),received);
                }
            }
        }
    }
    kill(pid2,SIGKILL);
}

BOOST_AUTO_TEST_SUITE_END()
