/*
Copyright (C) 2013 by Intra2net AG

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
#include <set>

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

set<int> SocketSet;

bool check_sockets(const int& test_socket)
{
    bool ret = false;
    set<int>::iterator it;
    for (it = SocketSet.begin(); it != SocketSet.end(); it++)
    {
        if (test_socket == *it)
        {
            ret = true;
            break;
        }
    }
    return ret;
}

class checkfunc_res : public libt2n::result
{
    private:
        bool res;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);
            ar & BOOST_SERIALIZATION_NVP(res);
        }

    public:
        checkfunc_res()
            { }

        checkfunc_res(const bool& value)
        {
            res=value;
        }

        bool get_data()
        {
            return res;
        }
};


class checkfunc_cmd : public libt2n::command
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
        checkfunc_cmd()
            {}

        checkfunc_cmd(const int& value)
        {
            param=value;
        }

        libt2n::result* operator()()
        {
            return new checkfunc_res(check_sockets(param));
        }
};

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(checkfunc_cmd)
BOOST_CLASS_EXPORT(checkfunc_res)

using namespace libt2n;

BOOST_FIXTURE_TEST_SUITE(test_getsocket, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(SocketCheck)
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
                command_server cs(ss);

                // max 10 sec
                for (int i=0; i < 10; i++)
                {
                    SocketSet = ss.get_sockets_set();
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
            socket_client_connection sc("./socket");
            sc.set_logging(&cerr,debug);
            command_client cc(&sc);

            result_container rc;
            int socket_fd = sc.get_socket();
            // Next line causes problem
            cc.send_command(new checkfunc_cmd(socket_fd),rc);

            bool ret=dynamic_cast<checkfunc_res*>(rc.get_result())->get_data();

            BOOST_CHECK(ret);
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
