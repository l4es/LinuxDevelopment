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

#include "test_fixtures.hxx"

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

using namespace std;

string testfunc3(const string& str)
{
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    string ret(str);
    ret+=", testfunc() was here";
    return ret;
}

class testfunc3_res : public libt2n::result
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
        testfunc3_res()
            { }

        testfunc3_res(const string& str)
        {
            res=str;
        }

        string get_data()
        {
            return res;
        }
};


class testfunc3_cmd : public libt2n::command
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
        testfunc3_cmd()
            { }

        testfunc3_cmd(const string& str)
        {
            param=str;
        }

        libt2n::result* operator()()
        {
            return new testfunc3_res(testfunc3(param));
        }
};

#include <boost/serialization/export.hpp>

using namespace libt2n;

BOOST_FIXTURE_TEST_SUITE(test_serialize, KillChildOnShutdownFixture)
// TODO: Server Deserialization Error
// TODO: Server Serialization Error
// TODO: Client Deserialization Error
// but those probably need separate client/server binaries

BOOST_AUTO_TEST_CASE(ClientSerializeErr)
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
                    cs.handle(1000000);
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
            command_client cc(&sc);

            string errormsg;

            result_container rc;
            try
            {
                cc.send_command(new testfunc3_cmd("xyz"),rc);
            }
            catch(t2n_serialization_error &e)
                { errormsg=e.what(); }

            BOOST_CHECK_EQUAL(string("archive_exception while serializing on client-side, code 2 (unregistered class - derived class not registered or exported)"),errormsg);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
