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

#include <string>
#include "minimalistic-stub.hxx"

#include <socket_server.hxx>
#include <command_server.hxx>

using namespace std;

LIBT2N_SET_DEFAULTGROUP(example);

LIBT2N_EXPORT std::string testfunc(const string& str) 
{
    string ret;
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    if (str=="big")
        ret.insert(0,100*1024,'x');
    else
        ret=str+", testfunc() was here";
    return ret;
}

using namespace libt2n;

int main(int argc, char** argv)
{
    extended_type_info_test();

    // don't kill us on broken pipe
    signal(SIGPIPE, SIG_IGN);

    socket_server ss("./socket");
    group_command_server<cmd_group_example> cs(ss);

    // handle requests (without timeout)
    while(true)
        cs.handle();

    return 0;
}
