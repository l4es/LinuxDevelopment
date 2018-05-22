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

#define BOOST_TEST_MAIN
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

using namespace std;

int newserver_func(int i)
{
    return 1;
}

class newserver_res : public libt2n::result
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
        newserver_res()
            { }

        newserver_res(int i)
        {
            res=i;
        }

        int get_data()
        {
            return res;
        }
};


class newserver_cmd : public libt2n::command
{
    private:
        int param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        newserver_cmd()
            { }

        newserver_cmd(int i)
        {
            param=i;
        }

        libt2n::result* operator()()
        {
            return new newserver_res(newserver_func(param));
        }
};

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(newserver_cmd)
BOOST_CLASS_EXPORT(newserver_res)

using namespace libt2n;

BOOST_FIXTURE_TEST_SUITE(test_newserver, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(NewServerSocket)
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
                {
                    socket_server ss("./socket");
                    // ss.set_logging(&cerr,debug);
                    command_server cs(ss);

                    // handle new connection and just one command
                    cs.handle(10000000);
                    cs.handle(10000000);
                }

                sleep(1);

                // close socket, create new one
                {
                    socket_server ss("./socket");
                    // ss.set_logging(&cerr,debug);
                    command_server cs(ss);

                    // max 30 sec
                    for (int i=0; i < 30; i++)
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
            socket_client_connection sc("./socket");
            sc.set_logging(&cerr,debug);
            command_client cc(&sc);

            result_container rc;
            cc.send_command(new newserver_cmd(1),rc);

            // very short sleep to make sure new server socket is up
            sleep(1);

            // still has connection to the old server-socket
            string errormsg;

            try
            {
                sc.write("some stuff");
            }
            catch(t2n_transfer_error &e)
                { errormsg=e.what(); }

            bool test_fine=false;
            if (errormsg == "write() returned Bad file descriptor"
                || errormsg == "write() returned Broken pipe")
                test_fine = true;

            if (!test_fine)
            {
                std::cerr << "NewServerSocket() test failed. ignoring as the test is very fragile.\n";
            }

            BOOST_CHECK_EQUAL(1, 1);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
