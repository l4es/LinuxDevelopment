/*
    Copyright (C) 2006                                                    
    intra2net.com                                                         

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <signal.h>

#include <socket_server.hxx>
#include <command_server.hxx>

#include "default_server.hxx"
#include "other_server.hxx"

using namespace libt2n;


int main(int argc, char** argv)
{
    // don't kill the server on broken pipe
    signal(SIGPIPE, SIG_IGN);

    socket_server ss("./socket");
    socket_server ss_other("./socket_other");
    group_command_server<cmd_group_default> cs(ss);
    group_command_server<cmd_group_other> cs_other(ss_other);

    // handle requests
    while(true)
    {
        cs.handle(1000);
        cs_other.handle(1000);
    }

    return 0;
}
