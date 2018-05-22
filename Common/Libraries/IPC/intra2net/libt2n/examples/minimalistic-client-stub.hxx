/* header file for the library that should be automatically generated */

#ifndef MINIMALISTIC_CLIENT_STUB_HXX
#define MINIMALISTIC_CLIENT_STUB_HXX

#include <command_client.hxx>

class cmd_group_example_client : public libt2n::command_client
{
    public:
        cmd_group_example_client(libt2n::client_connection *_c, 
            long long _command_timeout_usec=command_timeout_usec_default,
            long long _hello_timeout_usec=hello_timeout_usec_default)
            : libt2n::command_client(_c,_command_timeout_usec,_hello_timeout_usec)
            { }

        std::string testfunc(const std::string& str);
};

void
extended_type_info_test();

#endif
