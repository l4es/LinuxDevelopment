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
#ifndef __LIBT2N_COMMAND_SERVER
#define __LIBT2N_COMMAND_SERVER

#include "command.hxx"
#include "server.hxx"

namespace libt2n
{

/// a server handling incoming commands
class command_server
{
    private:
        server& s;

        void handle_packet(const std::string& packet, server_connection* conn);

        int guard_handle;

    protected:
        virtual command* cast_command(command* input)
            { return input; }

    public:
        command_server(server& _s);
        ~command_server();

        void handle(long long usec_timeout=-1, long long* usec_timeout_remaining=NULL);

        void send_hello(unsigned int conn_id);

        std::ostream* get_logstream(log_level_values level)
            { return s.get_logstream(level); }
};

template<class T, class B> struct Derived_from {
        static void constraints(T* p) { B* pb = p; }
        Derived_from() { void(*/*p*/)(T*) = constraints; }
};

/** @brief server handling group of incoming commands

    the template must be derived from libt2n::command.
*/
template<class COMMAND_GROUP>
class group_command_server : public command_server
{
    private:
        virtual command* cast_command(command* input)
            { return dynamic_cast<COMMAND_GROUP*>(input); }

    public:
        group_command_server(server& _s)
            : command_server(_s)
        { Derived_from<COMMAND_GROUP,command>(); }
};

}
#endif

