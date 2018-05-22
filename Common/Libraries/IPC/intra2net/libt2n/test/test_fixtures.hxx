/*
Copyright (C) 2010 by Intra2net AG - Thomas Jarosch

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
#ifndef __LIBT2N_TEST_FIXTURES
#define __LIBT2N_TEST_FIXTURES

#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#include <socket_server.hxx>
#include <string>

class KillChildOnShutdownFixture
{
protected:
    pid_t child_pid;

    // this is an evil hack to get access to real_write, don't ever do this in an app!!!
    class real_write_connection: public libt2n::socket_server_connection
    {
        public:
            void real_write(const std::string& data)
                { socket_write(data); }
    };

    //
    // common functions used by some tests
    //
    void send_hello(std::string hello_string, libt2n::socket_server* ss, int conn_id)
    {
        libt2n::server_connection *sc=ss->get_connection(conn_id);
        sc->write(hello_string);
    }

    void send_raw_socket(std::string hello_string, libt2n::socket_server* ss, int conn_id)
    {
        libt2n::socket_server_connection *ssc=dynamic_cast<libt2n::socket_server_connection*>(ss->get_connection(conn_id));

        // this is an evil hack to get access to real_write, don't ever do this in an app!!!
        real_write_connection *rwc=(real_write_connection*)ssc;
        rwc->real_write(hello_string);
    }

public:
    KillChildOnShutdownFixture()
        : child_pid(0)
    {
        // Work around Boost.test reporting killed childs as "fatal error".
        // Can be ignored with boost 1.40.0+, need to check after upgrade
        signal(SIGCHLD, SIG_IGN);
    }

    ~KillChildOnShutdownFixture()
    {
        // make sure the server-child is dead before the next test runs
        if (child_pid)
        {
            // std::cout << "Killing child with pid: " << child_pid << std::endl;
            kill(child_pid, SIGKILL);

            int status = 0;
            waitpid(child_pid, &status, 0);
        }
    }
};

#endif
