#include "minimalistic-stub.hxx"
#include <boost/serialization/export.hpp>

/* register types with boost serialization */

BOOST_CLASS_EXPORT(cmd_group_example)
BOOST_CLASS_EXPORT(testfunc_cmd)
BOOST_CLASS_EXPORT(testfunc_res)

void
extended_type_info_test() {
  cmd_group_example* t=new testfunc_cmd();
  const boost::serialization::extended_type_info * true_type
    = boost::serialization::type_info_implementation<libt2n::command>::type::get_const_instance().get_derived_extended_type_info(*t);
  // note:if this exception is thrown, be sure that derived pointer
  // is either regsitered or exported.
  assert(NULL != true_type);
  assert(std::string("testfunc_cmd")==true_type->get_key());
}
