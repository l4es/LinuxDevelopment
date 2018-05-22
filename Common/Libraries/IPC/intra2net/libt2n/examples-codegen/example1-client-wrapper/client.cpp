// for a wrapped socket connection
#include <socket_wrapper.hxx>

// include generated library header
#include "t2nexample_client.hxx"

// define a type for more conveniant access
typedef libt2n::T2nSingletonWrapper<cmd_group_t2nexample_client> wraptype;

// static object which keeps the wrapper-singleton
template<>
std::auto_ptr<wraptype> wraptype::SingletonObject = std::auto_ptr<wraptype>();

// static object which keeps the connection
template<>
std::auto_ptr<libt2n::ConnectionWrapper> wraptype::WrappedConnection =
    std::auto_ptr<libt2n::ConnectionWrapper>();

int main(int argc, char** argv)
{
    // tell the client-wrapper how to contact the server if a connection is requested
    wraptype::set_connection(std::auto_ptr<libt2n::ConnectionWrapper>
        (new libt2n::ReconnectSocketWrapper("./socket")));

    // execute a function via t2n. The wrapper will open a connection to the server if none
    // exists or try to reconnect if the existing one doesn't answer
    std::cout << t2n_exec(&cmd_group_t2nexample_client::testfunc)("hello") << std::endl;

    return EXIT_SUCCESS;
}
