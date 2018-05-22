/*
Copyright (C) 2006 by Intra2net AG - Gerd v. Egidy

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
#ifndef __LIBT2N_COMMAND_CLIENT
#define __LIBT2N_COMMAND_CLIENT

#include <functional>
#include <string>

#include "client.hxx"
#include "container.hxx"

namespace libt2n
{

/// a client sending out commands to a server
class command_client
{
    public:
        static const long long command_timeout_usec_default=90000000;
        static const long long hello_timeout_usec_default=30000000;

    private:
        client_connection *c;

        long long hello_timeout_usec;
        long long command_timeout_usec;

        void read_hello();
        std::string read_packet(const long long &usec_timeout);
        bool check_hello(const std::string& hellostr);

        t2n_exception *constructorException;

    public:
        command_client(client_connection* _c,
            long long _command_timeout_usec=command_timeout_usec_default,
            long long _hello_timeout_usec=hello_timeout_usec_default);
        virtual ~command_client();

        void replace_connection(client_connection* _c);

        void send_command(command* cmd, result_container &res);

        void set_command_timeout_usec(long long _command_timeout_usec=command_timeout_usec_default)
            { command_timeout_usec=_command_timeout_usec; }
        void set_hello_timeout_usec(long long _hello_timeout_usec=hello_timeout_usec_default)
            { hello_timeout_usec=_hello_timeout_usec; }
        long long get_command_timeout_usec(void)
            { return command_timeout_usec; }
        long long get_hello_timeout_usec(void)
            { return hello_timeout_usec; }
        bool is_connection_closed(void)
            { return c->is_closed(); }
        t2n_exception* get_constuctor_exception(void)
            { return constructorException; }
};

}

#endif

