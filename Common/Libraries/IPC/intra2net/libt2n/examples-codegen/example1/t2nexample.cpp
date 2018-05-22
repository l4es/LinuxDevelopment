// include automatically generated code
#include "t2nexample_common.hxx"

//! example procedure to export (LIBT2N_EXPORT tells the code generator that we want to export this procedure)
LIBT2N_EXPORT std::string testfunc(std::string str) 
{
    // exceptions derived from libt2n::t2n_exception are passed to the client transparently
    if (str=="throw")
        throw libt2n::t2n_runtime_error("throw me around");
    return str+", testfunc() was here";
}
