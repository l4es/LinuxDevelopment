#include "minimalistic-client-stub.hxx"
#include "minimalistic-stub.hxx"

// fake
libt2n::result* testfunc_cmd::operator()()
{
  return NULL;
}

std::string cmd_group_example_client::testfunc(const std::string& str)
{
    libt2n::result_container rc;
    send_command(new testfunc_cmd(str),rc);

    testfunc_res* res=dynamic_cast<testfunc_res*>(rc.get_result());
    if (!res)
      // TODO: another exception and details
      throw libt2n::t2n_communication_error("result object of wrong type");
    return res->get_data();
}

// include in this compilation unit to ensure the compilation unit is used
// see also:
// http://www.google.de/search?q=g%2B%2B+static+initializer+in+static+library
#include "minimalistic-stub-once.hxx"
