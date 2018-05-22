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

#include "test_fixtures.hxx"

using namespace std;
using namespace libt2n;

string testfunc4(const string& str)
{
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    string ret(str);
    ret+=", testfunc() was here";
    return ret;
}

class testfunc4_res : public libt2n::result
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
        testfunc4_res()
            { }

        testfunc4_res(const string& str)
        {
            res=str;
        }

        string get_data()
        {
            return res;
        }
};

class cmd_group_a : public command
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
        }
};

class cmd_group_b : public command
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
        }
};

class testfunc4a_cmd : public cmd_group_a
{
    private:
        string param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(cmd_group_a);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        testfunc4a_cmd()
            { }

        testfunc4a_cmd(const string& str)
        {
            param=str;
        }

        libt2n::result* operator()()
        {
            return new testfunc4_res(testfunc4(param));
        }
};

class testfunc4b_cmd : public cmd_group_b
{
    private:
        string param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(cmd_group_b);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        testfunc4b_cmd()
            { }

        testfunc4b_cmd(const string& str)
        {
            param=str;
        }

        libt2n::result* operator()()
        {
            return new testfunc4_res(testfunc4(param));
        }
};


BOOST_CLASS_EXPORT(testfunc4_res)
BOOST_CLASS_EXPORT(cmd_group_a)
BOOST_CLASS_EXPORT(cmd_group_b)
BOOST_CLASS_EXPORT(testfunc4a_cmd)
BOOST_CLASS_EXPORT(testfunc4b_cmd)


BOOST_FIXTURE_TEST_SUITE(test_cmdgroup, KillChildOnShutdownFixture)

BOOST_AUTO_TEST_CASE(GroupOk)
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
                group_command_server<cmd_group_a> cs(ss);

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

            result_container rc;
            cc.send_command(new testfunc4a_cmd("hello"),rc);

            string ret=dynamic_cast<testfunc4_res*>(rc.get_result())->get_data();

            BOOST_CHECK_EQUAL(string("hello, testfunc() was here"),ret);
        }
    }
}

BOOST_AUTO_TEST_CASE(WrongGroup)
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
                group_command_server<cmd_group_b> cs(ss);

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

            result_container rc;
            cc.send_command(new testfunc4a_cmd("hello"),rc);

            string ret;

            try
            {
                ret=dynamic_cast<testfunc4_res*>(rc.get_result())->get_data();
            }
            catch(t2n_command_error &e)
                { ret=e.what(); }

            string expected_what="illegal command of type ";

            BOOST_CHECK_EQUAL(expected_what,ret.substr(0,expected_what.size()));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
