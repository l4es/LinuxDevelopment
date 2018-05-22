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
#include <time.h>

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <socket_client.hxx>
#include <socket_server.hxx>

#include "test_fixtures.hxx"

#include <config.h>

using namespace std;
using namespace libt2n;

BOOST_FIXTURE_TEST_SUITE(test_reconnect, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(simple_reconnect)
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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                            con->write(string().insert(0,100,'X'));
                        else
                            con->write(string().insert(0,100,'Y'));
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("abc");

            string data;

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            sc.reconnect();

            sc.write("x");

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            BOOST_CHECK_EQUAL(string().insert(0,100,'X'),data);
        }
    }
}

BOOST_AUTO_TEST_CASE(reconnect_with_close)
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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                            con->write(string().insert(0,100,'X'));
                        else
                            con->write(string().insert(0,100,'Y'));
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("abc");

            string data;

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            sc.close();

            // empty buffer
            sc.get_packet(data);

            sc.reconnect();

            sc.write("x");

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            BOOST_CHECK_EQUAL(string().insert(0,100,'X'),data);
        }
    }
}

BOOST_AUTO_TEST_CASE(reconnect_buffer_complete)
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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                            con->write(string().insert(0,100,'X'));
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("x");

            string data;

            while (!sc.packet_available())
                sc.fill_buffer(1000000);

            sc.reconnect();

            BOOST_CHECK_MESSAGE(sc.get_packet(data) == true, "packet not there");

            BOOST_CHECK_MESSAGE(data == string().insert(0,100,'X'), "data incorrect");
        }
    }
}

BOOST_AUTO_TEST_CASE(reconnect_buffer_several_complete)
{
    const int packets=3;

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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                        {
                            for (int i=0; i<packets; i++)
                                con->write(string().insert(0,100,'X'));
                        }
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("x");

            // retrieve packets for 3 seconds
            time_t t0 = time(NULL);

            // max 3 sec
            while (time(NULL) < t0 + 3 )
                sc.fill_buffer(1000000);

            // we now should have packets complete packets in the buffer

            sc.reconnect();

            // are these packets still there?

            for (int i=0; i < packets; i++)
            {
                string data;

                ostringstream os;
                os << "packet " << i << " not there";

                BOOST_CHECK_MESSAGE(sc.get_packet(data) == true, os.str());

                os.str("");
                os << "packet " << i << " incorrect";

                BOOST_CHECK_MESSAGE(string().insert(0,100,'X') == data, os.str());
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(reconnect_buffer_no_incomplete1)
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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                        {
                            con->write(string().insert(0,100,'X'));
                            send_raw_socket("aaaab",&ss,cid);
                        }
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("x");

            // retrieve packets for 3 seconds
            time_t t0 = time(NULL);

            // max 3 sec
            while (time(NULL) < t0 + 3 )
                sc.fill_buffer(1000000);

            // we now should have one complete packet and some stuff in the buffer

            string data;
            sc.get_packet(data);

            BOOST_CHECK_MESSAGE((sc.peek_packet(data))>0, "no incomplete packet");

            sc.reconnect();

            BOOST_CHECK_MESSAGE((int)sc.peek_packet(data) == 0, "incomplete packet not removed");
        }
    }
}

BOOST_AUTO_TEST_CASE(reconnect_buffer_no_incomplete2)
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

                time_t t0 = time(NULL);

                // max 10 sec
                while (time(NULL) < t0 + 10 )
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;

                    if(ss.get_packet(data,cid))
                    {
                        server_connection* con=ss.get_connection(cid);

                        if (data=="QUIT")
                            break;

                        if (data=="x")
                        {
                            con->write(string().insert(0,100,'X'));

                            string blob=string().insert(0,100,'Y');

                            // one byte will be missing...
                            int size=blob.size()+1;
                            char sizetransfer[sizeof(int)+1];
                            memcpy(sizetransfer,(void*)&size,sizeof(int));
                            sizetransfer[sizeof(int)]=0;

                            string packet=string(sizetransfer)+blob;

                            send_raw_socket(packet,&ss,cid);
                        }
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
            // don't kill us on broken pipe
            signal(SIGPIPE, SIG_IGN);

            // wait till server is up
            sleep(1);
            socket_client_connection sc("./socket");

            sc.write("x");

            // retrieve packets for 3 seconds
            time_t t0 = time(NULL);

            // max 3 sec
            while (time(NULL) < t0 + 3 )
                sc.fill_buffer(1000000);

            // we now should have one complete packet and some stuff in the buffer

            sc.reconnect();

            string data;

            BOOST_CHECK_MESSAGE(sc.get_packet(data) == true, "packet not there");
            BOOST_CHECK_MESSAGE(string().insert(0,100,'X') == data, "data incorrect");

            BOOST_CHECK_MESSAGE((int)sc.peek_packet(data) == 0, "incomplete packet not removed");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
