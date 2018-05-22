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
#include <vector>

#include <boost/bind.hpp>

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <socket_client.hxx>
#include <socket_server.hxx>

#include "test_fixtures.hxx"

using namespace std;
using namespace libt2n;

class test_callbackFixture : public KillChildOnShutdownFixture
{
protected:
    std::vector<bool> callback_done;

    void callback_func(callback_event_type ev, int conn_id)
    {
        // we don't care for the conn_id, we just mark the callback as done
        if (callback_done[ev])
            throw runtime_error("callback already done for this event");

        callback_done[ev]=true;
    }

public:
    test_callbackFixture()
    {
        callback_done.resize(__events_end);
    }

    ~test_callbackFixture()
    {
        callback_done.clear();
    }
};

BOOST_FIXTURE_TEST_SUITE(test_callback, test_callbackFixture)

BOOST_AUTO_TEST_CASE(ServerNewConnCallback)
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
                string data;
                // wait till server is up
                sleep(1);

                {
                    socket_client_connection sc("./socket");

                    sc.write("ABC");

                    // wait half a sec
                    sc.fill_buffer(500000);
                    sc.get_packet(data);

                    // close the connection
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
            socket_server ss("./socket");

            ss.add_callback(new_connection,bind(&test_callback::ServerNewConnCallback::callback_func, boost::ref(*this), new_connection, _1));

            // max 3 sec
            for (int i=0; i < 3; i++)
            {
                ss.fill_buffer(1000000);

                string data;
                unsigned int cid;
                if(ss.get_packet(data,cid))
                {
                    server_connection* con=ss.get_connection(cid);
                    con->write("XYZ");
                }
            }
            BOOST_CHECK_EQUAL(true,static_cast<bool>(callback_done[new_connection]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_closed]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_deleted]));
        }
    }
}

BOOST_AUTO_TEST_CASE(ServerConnClosedCallback)
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
                string data;
                // wait till server is up
                sleep(1);

                {
                    socket_client_connection sc("./socket");

                    sc.write("ABC");

                    // wait half a sec
                    sc.fill_buffer(500000);
                    sc.get_packet(data);

                    // close the connection
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
            socket_server ss("./socket");

            ss.add_callback(connection_closed,bind(&test_callback::ServerConnClosedCallback::callback_func, boost::ref(*this), connection_closed, _1));

            // max 3 sec
            for (int i=0; i < 3; i++)
            {
                ss.fill_buffer(1000000);

                string data;
                unsigned int cid;
                if(ss.get_packet(data,cid))
                {
                    server_connection* con=ss.get_connection(cid);
                    con->write("XYZ");
                }
            }
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[new_connection]));
            BOOST_CHECK_EQUAL(true,static_cast<bool>(callback_done[connection_closed]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_deleted]));
        }
    }
}

BOOST_AUTO_TEST_CASE(ServerConnDeletedCallback)
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
                string data;
                // wait till server is up
                sleep(1);

                {
                    socket_client_connection sc("./socket");

                    sc.write("ABC");

                    // wait half a sec
                    sc.fill_buffer(500000);
                    sc.get_packet(data);

                    // close the connection
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
            socket_server ss("./socket");

            ss.add_callback(connection_deleted,bind(&test_callback::ServerConnDeletedCallback::callback_func, boost::ref(*this), connection_deleted, _1));

            // max 3 sec
            for (int i=0; i < 3; i++)
            {
                ss.fill_buffer(1000000);

                string data;
                unsigned int cid;
                if(ss.get_packet(data,cid))
                {
                    server_connection* con=ss.get_connection(cid);
                    con->write("XYZ");
                }
                ss.cleanup();
            }
            ss.cleanup();

            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[new_connection]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_closed]));
            BOOST_CHECK_EQUAL(true,static_cast<bool>(callback_done[connection_deleted]));
        }
    }
}

BOOST_AUTO_TEST_CASE(ServerCallbackOrder)
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
                string data;
                // wait till server is up
                sleep(1);

                {
                    socket_client_connection sc("./socket");

                    sc.write("1");

                    // wait half a sec
                    sc.fill_buffer(500000);
                    sc.get_packet(data);

                    sc.write("2");

                    // close the connection
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
            socket_server ss("./socket");

            ss.add_callback(connection_closed,bind(&test_callback::ServerCallbackOrder::callback_func, boost::ref(*this), connection_closed, _1));
            ss.add_callback(connection_deleted,bind(&test_callback::ServerCallbackOrder::callback_func, boost::ref(*this), connection_deleted, _1));

            bool got_1=false;

            for (int i=0; i < 5; i++)
            {
                ss.fill_buffer(500000);

                string data;
                unsigned int cid;
                if(!got_1 && ss.get_packet(data,cid))
                {
                    server_connection* con=ss.get_connection(cid);
                    con->write("XYZ");
                    got_1=true;
                    // don't call get_packet anymore
                }
                ss.cleanup();
            }
            ss.cleanup();

            BOOST_CHECK_MESSAGE(static_cast<bool>(callback_done[connection_closed]) == true, "closed done");
            BOOST_CHECK_MESSAGE(static_cast<bool>(callback_done[connection_deleted]) == false, "not deleted yet");

            for (int i=0; i < 4; i++)
            {
                ss.fill_buffer(500000);

                string data;
                unsigned int cid;
                ss.get_packet(data,cid);
                ss.cleanup();
            }
            ss.cleanup();

            BOOST_CHECK_MESSAGE(static_cast<bool>(callback_done[connection_closed]) == true, "closed done (2)");
            BOOST_CHECK_MESSAGE(static_cast<bool>(callback_done[connection_deleted]) == true, "not deleted yet(2)");
        }
    }
}

BOOST_AUTO_TEST_CASE(ClientConnClosedCallback)
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

                // max 3 sec
                for (int i=0; i < 3; i++)
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;
                    if(ss.get_packet(data,cid))
                        break;
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

            sc.add_callback(connection_closed,bind(&test_callback::ClientConnClosedCallback::callback_func, boost::ref(*this), connection_closed, 0));

            sc.write("ABC");

            // wait half a sec
            sc.fill_buffer(500000);
            sc.get_packet(data);

            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[new_connection]));
            BOOST_CHECK_EQUAL(true,static_cast<bool>(callback_done[connection_closed]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_deleted]));
        }
    }
}

BOOST_AUTO_TEST_CASE(ClientConnDeletedCallback)
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

                // max 3 sec
                for (int i=0; i < 3; i++)
                {
                    ss.fill_buffer(1000000);

                    string data;
                    unsigned int cid;
                    if(ss.get_packet(data,cid))
                        break;
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

            {
                socket_client_connection sc("./socket");

                sc.add_callback(connection_deleted,bind(&test_callback::ClientConnDeletedCallback::callback_func, boost::ref(*this), connection_deleted, 0));

                sc.write("ABC");

                // wait half a sec
                sc.fill_buffer(500000);
                sc.get_packet(data);
            }

            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[new_connection]));
            BOOST_CHECK_EQUAL(false,static_cast<bool>(callback_done[connection_closed]));
            BOOST_CHECK_EQUAL(true,static_cast<bool>(callback_done[connection_deleted]));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
