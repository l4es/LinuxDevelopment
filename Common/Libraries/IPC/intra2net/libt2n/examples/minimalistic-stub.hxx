#ifndef MINIMALISTIC_STUB_HXX
#define MINIMALISTIC_STUB_HXX


/*
  stubs to be generated
  header is used by the client library and the server
*/

// todo: why do we have to include the boost headers first?
// do we have to know which archive format we will use?

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>

#include <string>
#include <t2n_exception.hxx>
#include <command.hxx>

#ifdef __GCCXML__
#define LIBT2N_SET_DEFAULTGROUP(x) namespace { typedef __attribute((gccxml(#x))) int libt2n_default; }
#define LIBT2N_EXPORT __attribute((gccxml("libt2n-default")))
#define LIBT2N_EXPORT_GROUP(group) __attribute((gccxml("libt2n-"#group)))
#else
#define LIBT2N_SET_DEFAULTGROUP(x)
#define LIBT2N_EXPORT
#define LIBT2N_EXPORT_GROUP(group)
#endif

class cmd_group_example : public libt2n::command
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);
        }
};


class testfunc_res : public libt2n::result
{
    private:
        std::string res;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
	    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);
            ar & BOOST_SERIALIZATION_NVP(res);
        }

    public:
        testfunc_res()
            { }

        testfunc_res(const std::string& str)
        {
            res=str;
        }

        std::string get_data()
        {
            return res;
        }
};


class testfunc_cmd : public cmd_group_example
{
    private:
        std::string param;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(cmd_group_example);
            ar & BOOST_SERIALIZATION_NVP(param);
        }

    public:
        testfunc_cmd()
            { }

        testfunc_cmd(const std::string& str)
        {
            param=str;
        }

        libt2n::result* operator()();
};

void
extended_type_info_test();

#endif
