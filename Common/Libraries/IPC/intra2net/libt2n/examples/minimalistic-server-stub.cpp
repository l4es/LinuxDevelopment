#include "minimalistic-stub.hxx"

std::string testfunc(const std::string& str);

libt2n::result* testfunc_cmd::operator()()
{
  return new testfunc_res(testfunc(param));
}

#include "minimalistic-stub-once.hxx"

