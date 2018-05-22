#include <signal.h> 

#include <socket_server.hxx>
#include <command_server.hxx> // for group_command_server

// the automatically generated server side code (cmd_group_t2nexample class)
#include "t2nexample_server.hxx"

int main(int argc, char** argv)
{
    // don't kill the server on broken pipe
    signal(SIGPIPE, SIG_IGN);

    // create local socket server (a.k.a "unix domain socket")
    // if you want to to create a tcp/ip server you pass the port to the constructor
    // (for details take a look at the socket_server class documentation)
    libt2n::socket_server ss("./socket");
    libt2n::group_command_server<cmd_group_t2nexample> cs(ss);

    // handle requests
    while(true)
        cs.handle();

    return 0;
}
