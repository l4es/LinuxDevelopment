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

#include <t2n_exception.hxx>
#include <socket_client.hxx>
#include <socket_server.hxx>
#include <command_client.hxx>

#include "test_fixtures.hxx"

#include <config.h>

using namespace std;
using namespace libt2n;


BOOST_FIXTURE_TEST_SUITE(test_hello, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(HelloOk)
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

                ss.add_callback(new_connection,bind(&test_hello::HelloOk::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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
            command_client cc(&sc);

            // All fine we reached this point
            BOOST_CHECK(true);
        }
    }
}

BOOST_AUTO_TEST_CASE(BadTag)
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
                hello << "XYZ 123";

                ss.add_callback(new_connection,bind(&test_hello::BadTag::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("illegal hello received (T2N)"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(BadVersion)
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
                // lets hope we don't ever get near such a version number...
                hello << "T2Nv" << 4982271 << ';';
                int byteordercheck=1;
                hello.write((char*)&byteordercheck,sizeof(byteordercheck));
                hello << ';';

                ss.add_callback(new_connection,bind(&test_hello::BadVersion::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("not compatible with the server protocol version"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(SeparatorMissing)
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
                hello << "T2Nv" << PROTOCOL_VERSION;
                int byteordercheck=1;
                hello.write((char*)&byteordercheck,sizeof(byteordercheck));
                hello << ';';

                ss.add_callback(new_connection,bind(&test_hello::SeparatorMissing::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("illegal hello received (1. ;)"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(WrongByteOrder)
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
                int dst;
                char* si=(char*)&byteordercheck;
                char* di=(char*)&dst;

                di[0]=si[3];
                di[1]=si[2];
                di[2]=si[1];
                di[3]=si[0];

                hello.write((char*)&dst,sizeof(dst));
                hello << ';';

                ss.add_callback(new_connection,bind(&test_hello::WrongByteOrder::send_hello, boost::ref(*this), hello.str(),&ss, _1));

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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("host byte order not matching"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(OtherServerBig)
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
                // hmm, we got the wrong socket
                hello << "* OK intradev.net.lan Cyrus IMAP4 v2.2.13 server ready";

                ss.add_callback(new_connection,bind(&test_hello::OtherServerBig::send_raw_socket, boost::ref(*this), hello.str(),&ss, _1));

                // max 3 sec
                for (int i=0; i < 3; i++)
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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("illegal hello received (T2N)"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_CASE(OtherServerSmall)
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
                // hmm, we got the wrong socket
                hello << "READY";

                ss.add_callback(new_connection,bind(&test_hello::OtherServerSmall::send_raw_socket, boost::ref(*this), hello.str(),&ss, _1));

                // max 3 sec
                for (int i=0; i < 3; i++)
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

            command_client cc(&sc);

            t2n_exception* ep=cc.get_constuctor_exception();

            string errormsg;
            if (ep)
                errormsg=ep->what();

            BOOST_CHECK_EQUAL(string("illegal hello received (T2N)"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
