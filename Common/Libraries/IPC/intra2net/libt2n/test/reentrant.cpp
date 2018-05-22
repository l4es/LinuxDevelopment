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

#include <container.hxx>
#include <socket_client.hxx>
#include <socket_server.hxx>
#include <command_client.hxx>
#include <command_server.hxx>

using namespace std;
using namespace libt2n;

namespace reentrant
{

command_server *global_server = NULL;

int fork_count = 3;
int requests_per_child = 100;
int all_requests = (2 << (fork_count-1)) * requests_per_child;

int seen_client_requests = 0;

string testfunc(const string& str)
{
    string ret;
    ret=str+", testfunc() was here";

    // call handle, eventually reentrant
    if (global_server)
        global_server->handle(10000);

    ++seen_client_requests;

    return ret;
}

class testfunc_res : public libt2n::result
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
        testfunc_res()
            {
            }

        testfunc_res(const string& str)
        {
            res=str;
        }

        string get_data()
        {
            return res;
        }
};


class testfunc_cmd : public libt2n::command
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
        testfunc_cmd()
            {
            }

        testfunc_cmd(const string& str)
        {
            param=str;
        }

        libt2n::result* operator()()
        {
            return new testfunc_res(testfunc(param));
        }
};

}


#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(reentrant::testfunc_cmd)
BOOST_CLASS_EXPORT(reentrant::testfunc_res)

using namespace reentrant;

BOOST_AUTO_TEST_SUITE(test_reentrant)

BOOST_AUTO_TEST_CASE(ReentrantServer)
{
    switch(fork())
    {
        case -1:
        {
            BOOST_FAIL("fork error");
            break;
        }
        case 0:
        // child
        {
            // wait till server is up
            sleep(2);

            // hammer the server
            for (int i = 0; i < fork_count; i++)
                fork();

            try
            {
                for (int i=0; i < requests_per_child; i++)
                {
                    socket_client_connection sc("./socket");
                    // sc.set_logging(&cerr,debug);
                    command_client cc(&sc);

                    result_container rc;
                    cc.send_command(new testfunc_cmd("hello"),rc);

                    testfunc_res *res = dynamic_cast<testfunc_res*>(rc.get_result());
                    if (res)
                    {
                        string ret = res->get_data();
                        if (ret != "hello, testfunc() was here")
                            std::cout << "ERROR reentrant server testfunc_res failed, res: \"" << ret << "\"\n";
                    }
                    else
                    {
                        std::cout << "ERROR result from reentrant server empty (" << rc.get_result() << ")\n";
                    }
                }
            } catch (exception &e)
            {
                cerr << "caught exception: " << e.what() << endl;
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
            command_server cs(ss);

            global_server=&cs;

            // Wait until all requests have successed
            int safety_check = 0;
            while (seen_client_requests < all_requests)
            {
                ++safety_check;
                if (safety_check > 10) {
                    std::cerr << "reached safety check, aborting.\n";
                    break;
                }

                long long maxtime=1000000;
                while(maxtime > 0)
                    cs.handle(maxtime,&maxtime);
            }

            global_server = NULL;
        }

        // we are still alive, everything is ok
        BOOST_CHECK_EQUAL(all_requests, seen_client_requests);
    }
}

BOOST_AUTO_TEST_SUITE_END()
