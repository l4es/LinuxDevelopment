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

using namespace std;
using namespace libt2n;

BOOST_FIXTURE_TEST_SUITE(test_comm, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(UnixCommToServer)
{
    string data;

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
                sc.write("hello");
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
            socket_server ss("./socket");

            time_t t0 = time(NULL);

            // max 10 sec
            while (time(NULL) < t0 + 10 )
            {
                ss.fill_buffer(1000000);

                if(ss.get_packet(data))
                    break;
            }
        }
    }
    BOOST_CHECK_EQUAL(string("hello"),data);
}

BOOST_AUTO_TEST_CASE(UnixCommToServerAndBack)
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
                ss.set_logging(&cerr,debug);

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

                        if (data=="ABC")
                            con->write("DEF");
                        else
                            con->write("xyz");
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

            sc.write("ABC");

            sc.fill_buffer(1000000);
            sc.get_packet(data);

            BOOST_CHECK_EQUAL(string("DEF"),data);

            sc.write("HAHA");

            sc.fill_buffer(1000000);
            sc.get_packet(data);

            BOOST_CHECK_EQUAL(string("xyz"),data);

            sc.write("QUIT");
        }
    }
}

BOOST_AUTO_TEST_CASE(UnixCommToServerAndBackBig)
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
                ss.set_logging(&cerr,debug);

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

                        con->write(string().insert(0,100*1024,'y'));
                    }
                }
                std::cerr << "child: OVER" << std::endl;
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

            sc.write(string().insert(0,100*1024,'x'));

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            BOOST_CHECK_EQUAL(string().insert(0,100*1024,'y'),data);

            sc.write("QUIT");
        }
    }
}

BOOST_AUTO_TEST_CASE(IPCommToServer)
{
    string data;

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
                socket_client_connection sc(6666);
                sc.write("hello");
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
            socket_server ss(6666);

            time_t t0 = time(NULL);

            // max 10 sec
            while (time(NULL) < t0 + 10 )
            {
                ss.fill_buffer(1000000);

                if(ss.get_packet(data))
                    break;
            }
        }
    }
    BOOST_CHECK_EQUAL(string("hello"),data);
}

BOOST_AUTO_TEST_CASE(IPCommToServerAndBack)
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
                socket_server ss(6666);
                ss.set_logging(&cerr,debug);

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

                        if (data=="ABC")
                            con->write("DEF");
                        else
                            con->write("xyz");
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
            socket_client_connection sc(6666);
            sc.write("ABC");

            sc.fill_buffer(1000000);
            sc.get_packet(data);

            BOOST_CHECK_EQUAL(string("DEF"),data);

            sc.write("HAHA");

            sc.fill_buffer(1000000);
            sc.get_packet(data);

            BOOST_CHECK_EQUAL(string("xyz"),data);

            sc.write("QUIT");
        }
    }
}


BOOST_AUTO_TEST_CASE(IPCommToServerAndBackBig)
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
                socket_server ss(6666);
                ss.set_logging(&cerr,debug);

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

                        socket_handler* alias= dynamic_cast< socket_handler* >(con);

                        if (data=="QUIT")
                            break;

                        alias->set_write_block_size( 4093 );
                        con->write(string().insert(0,2048*1024,'y'));
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
            socket_client_connection sc(6666);

            sc.write(string().insert(0,100*1024,'x'));

            while (!sc.get_packet(data))
                sc.fill_buffer(1000000);

            BOOST_CHECK_EQUAL(string().insert(0,2048*1024,'y'),data);

            sc.write("QUIT");
        }
    }
} // eo IPCommToServerAndBackBig()

BOOST_AUTO_TEST_SUITE_END()
