// for socket_client_connection
#include <socket_client.hxx>

// include generated library header
#include "t2nexample_client.hxx"

int main(int argc, char** argv)
{
    // use a local socket (a.k.a "unix domain socket")
    // if you want to connect to a tcp/ip server you pass the port and server name to the constructor
    libt2n::socket_client_connection sc("./socket");
    // this generated class has a method for each of our exported procedures
    cmd_group_t2nexample_client cc(&sc);

    bool throwok=false;

    // exceptions are passed back to the client transparently
    try
    {
        // call the remote procedure (we pass "throw" to cause a exception to be thrown)
        cc.testfunc("throw");
    }
    catch(libt2n::t2n_runtime_error &e)
    {
        throwok=(std::string(e.what())=="throw me around");
    }

    // call remote procedure and check the return value is correct
    return ( throwok && ( cc.testfunc("hello") == "hello, testfunc() was here" ) )
        ? EXIT_SUCCESS : EXIT_FAILURE;
}
