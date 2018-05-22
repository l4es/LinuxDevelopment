/*
    Copyright (C) 2006-2008 by Intra2net AG
    intra2net.com

    The software in this package is distributed under the GNU General
    Public License version 2 (with a special exception described below).

    A copy of GNU General Public License (GPL) is included in this distribution,
    in the file COPYING.GPL.

    As a special exception, if other files instantiate templates or use macros
    or inline functions from this file, or you compile this file and link it
    with other works to produce a work based on this file, this file
    does not by itself cause the resulting work to be covered
    by the GNU General Public License.

    However the source code for this file must still be made available
    in accordance with section (3) of the GNU General Public License.

    This exception does not invalidate any other reasons why a work based
    on this file might be covered by the GNU General Public License.
*/

#include <libxml++/libxml++.h>
#include <cassert>
#include <iostream>
#include <set>
#include <fstream>
#include <list>
#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/extended_type_info.hpp>

#include "config.h"



//! map group to class name
std::string groupClass(const std::string &group)
{
    return std::string("cmd_group_")+group;
}

//! convert string to upper case
std::string toupper(std::string s)
{
    for (unsigned i=0; i<s.length(); ++i) s[i]=toupper(s[i]);
    return s;
}

//! replace all characters f by r in string s
std::string replace(std::string s, char f, char r)
{
    for (unsigned i=0; i<s.length(); ++i) if (s[i]==f) s[i]=r;
    return s;
}

//! strip prefix from string s
/*!
  \return string s without prefix or an empty string on error
 */
std::string strip(std::string s, std::string prefix)
{
    std::string error;
    if ( (prefix.length()>s.length() ) || ( std::string(s,0,prefix.length())!=prefix ) ) return error;
    return std::string(s, prefix.length(), s.length()-prefix.length());
}

//! get child element by id
/*!
  \return pointer to element having id or null on error
  \todo find libxmlpp pendant
*/
const xmlpp::Element* get_element_by_id(const xmlpp::Element* element, const std::string &id)
{
    const xmlpp::Attribute* cid = element->get_attribute("id");
    if ( cid && ( cid->get_value() == id)) return element;

    //Recurse through child nodes:
    xmlpp::Node::NodeList list = element->get_children();
    for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
    {
        const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(*iter);
        if (element)
        {
            const xmlpp::Element* match = get_element_by_id(element, id);
            if (match) return match;
        }
    }
    return NULL;
}

std::string get_file(const xmlpp::Element* root, const std::string &file_id)
{
    std::string error;
    const xmlpp::Element* e=get_element_by_id(root, file_id);
    if ((!e)||(!e->get_attribute("name"))) return error;
    return e->get_attribute("name")->get_value();
}

//! get namespace by id
/*!
  \return namespace name or empty string on error
*/
std::string get_namespace(const xmlpp::Element* root, const std::string &id)
{
    std::string error;
    const xmlpp::Element* element(get_element_by_id(root, id));
    // [RP:20071024]: use "demangled" attribute instead of "name" to cover nested namespaces:
    if ((!element)||(!element->get_attribute("demangled"))) return error;
    return element->get_attribute("demangled")->get_value();
}

//! procedure marked for export?
bool is_marked(const std::string &attrs)
{
    // todo: improve this
    std::string to_match("gccxml(libt2n-");
    std::string::size_type p(attrs.find(to_match));
    return (p!=std::string::npos);
}

struct type_info
{
    std::string name;
    std::string noref_name;
    bool operator==(const type_info& o) {return (name==o.name) && (noref_name == o.noref_name);}
    std::string noref() const {return noref_name.empty() ? name : noref_name;}
    bool isVoid() const {return name=="void";}
};

std::ostream &operator<<(std::ostream &o, const type_info &t)
{
    o << t.name;
    return o;
}

struct parse_error : public std::runtime_error
{
    parse_error(const std::string &file, unsigned line, const std::string &msg)
            : std::runtime_error(file+":"+boost::lexical_cast<std::string>(line)+": error: "+msg)
    {}
};

struct error_name_too_long : public std::runtime_error
{
    error_name_too_long(const std::string &name)
            : std::runtime_error("symbol name '" + name + "' too long for serialization ("
                                 + boost::lexical_cast<std::string>(name.length()) + ">"
                                 + boost::lexical_cast<std::string>(BOOST_SERIALIZATION_MAX_KEY_SIZE-1) + ")")
    {}
};

//! get type by id
/*!
  \return type name or empty string on error
*/
type_info get_type(const xmlpp::Element* root, const std::string &id)
{
    type_info error;
    const xmlpp::Element* element(get_element_by_id(root, id));
    if (!element) return error;

    // TODO: not yet complete
    // if we recurse - when do we stop?
    // if it is a typedef? yes? (hmm if the typedef is in the file parsed this will not work)

    // TODO: const and reference types handling is a ugly hack

    std::string tag(element->get_name());
    if (tag=="ReferenceType")
    {
        assert(element->get_attribute("type"));
        type_info ret(get_type(root, element->get_attribute("type")->get_value()));
        if (ret==error) return error;
        // at the moment we only support const &
        // todo: nice error message!
        if ((ret.noref_name=strip(ret.name,"const ")).empty()) return error;
        ret.name=ret.name+"&";
        return ret;
    }
    else if (tag=="CvQualifiedType")
    {
        assert(element->get_attribute("type"));
        type_info ret(get_type(root, element->get_attribute("type")->get_value()));
        if (ret==error) return error;
        ret.name=std::string("const ")+ret.name;
        return ret;
    }
    else if (tag=="PointerType")
    {
        // todo: nearly the same as reference type handling
        assert(element->get_attribute("type"));
        type_info ret(get_type(root, element->get_attribute("type")->get_value()));
        if (ret==error) return error;
        // at the moment we only support const &
        // todo: nice error message!
        if ((ret.noref_name=strip(ret.name,"const ")).empty()) return error;
        ret.name=ret.name+"*";
        return ret;
    }

    assert(element->get_attribute("name"));
    type_info ret;
    if (element->get_attribute("context"))
    {
        ret.name=get_namespace(root, element->get_attribute("context")->get_value());
        if (ret.name!="::")
            ret.name+="::";
        else
            // do not explicitely add ::
            ret.name="";
    }
    ret.name+=element->get_attribute("name")->get_value();
    return ret;
}

struct Arg
{
    std::string name;
    type_info type;
    std::string defaultArg;
};

struct t2n_procedure
{
    typedef std::list<Arg> Args;

    type_info ret_type;
    std::string name;
    std::string mangled;
    Args  args;

    std::string ret_classname() const
    {
        std::string result = name+mangled+"_res";
        if (result.length() >= BOOST_SERIALIZATION_MAX_KEY_SIZE)
            throw error_name_too_long(result);
        return result;
    }
    std::string cmd_classname() const
    {
        std::string result = name+mangled+"_cmd";
        if (result.length() >= BOOST_SERIALIZATION_MAX_KEY_SIZE)
            throw error_name_too_long(result);
        return result;
    }
    bool hasReturn() const {return !ret_type.isVoid();}
};

std::ostream &operator<<(std::ostream &o, const t2n_procedure::Args &args)
{
    for (t2n_procedure::Args::const_iterator it=args.begin();it!=args.end();++it)
    {
        if (it!=args.begin()) o << ", ";
        o << it->type << " " << it->name;

        if (!it->defaultArg.empty())
            o << "=" << it->defaultArg;
    }
    return o;
}

std::ostream &operator<<(std::ostream &o, const t2n_procedure &f)
{
    o << f.ret_type << " " << f.name << "(" << f.args << ")";
    return o;
}

std::pair<std::string, unsigned>
get_file_and_line(const xmlpp::Element* root, const xmlpp::Element* element)
{
    return std::pair<std::string, unsigned>(get_file(root, element->get_attribute("file")->get_value()),
                                            boost::lexical_cast<unsigned>(element->get_attribute("line")->get_value())-1);
}

std::string get_file_and_line_as_string(const xmlpp::Element* root, const xmlpp::Element* element)
{
    std::pair<std::string, unsigned> fl(get_file_and_line(root,element));
    return std::string(fl.first)+":"+boost::lexical_cast<std::string>(fl.second);
}

class Parser
{
public:
    Parser(const std::string &fname) : m_fname(fname) {}

    std::list<t2n_procedure> get_procedures()
    {
        xmlpp::DomParser parser;
        //    parser.set_validate();
        parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
        parser.parse_file(m_fname);
        if (parser)
        {
            //Walk the tree:
            const xmlpp::Node* pNode = parser.get_document()->get_root_node(); //deleted by DomParser.
            const xmlpp::Element* root = dynamic_cast<const xmlpp::Element*>(pNode);
            assert(root);
            visit_node(root);
        }
        return m_procedures;
    }
protected:
    std::string m_fname;
    std::list<t2n_procedure> m_procedures;

    void parse_function(const xmlpp::Element* root, const xmlpp::Node* node)
    {
        const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
        if (!element) return;

        const xmlpp::Attribute* attributes = element->get_attribute("attributes");
        const xmlpp::Attribute* name = element->get_attribute("name");
        const xmlpp::Attribute* mangled = element->get_attribute("mangled");
        const xmlpp::Attribute* returns = element->get_attribute("returns");
        if ((!attributes)||(!name)||(!mangled)||(!returns)) return;

        // check wether the procedure is marked (TODO: improve)
        // attributes are speparated by spaces?

        t2n_procedure f;
        if (!is_marked(attributes->get_value())) return;

        // we need the return type
        f.ret_type=get_type(root, returns->get_value());
        f.name=name->get_value();
        f.mangled=mangled->get_value();

        xmlpp::Node::NodeList list = node->get_children("Argument");
        for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
        {
            const xmlpp::Element* arg = dynamic_cast<const xmlpp::Element*>(*iter);
            if ( arg )
            {
                struct Arg a;

                assert(arg->get_name() == "Argument");

                assert(arg->get_attribute("name"));
                a.name=arg->get_attribute("name")->get_value();

                assert(arg->get_attribute("type"));
                a.type=get_type(root, arg->get_attribute("type")->get_value());

                // this would be the nice way of solving the problem of default arguments
                // but currently the output of gccxml is unreliable (e.g. namespace only
                // sometimes available)
                // if(arg->get_attribute("default"))
                //    a.defaultArg=arg->get_attribute("default")->get_value();

                // so we need to get the def. arg. via attribute
                // which is previously set by the macro LIBT2N_DEFAULT_ARG(type,value)
                if(arg->get_attribute("attributes"))
                {
                    std::string attr=arg->get_attribute("attributes")->get_value();
                    const std::string look_for = "gccxml(libt2n-default-arg,";

                    if (attr.compare(0,look_for.size(),look_for) == 0)
                        a.defaultArg=attr.substr(look_for.size(),attr.size()-look_for.size()-1);
                }

                // todo: ugly - could be any other error
                if (a.type.name.empty())
                {
                    assert(element->get_attribute("file"));
                    assert(element->get_attribute("line"));
                    std::pair<std::string, unsigned> file_and_line(get_file_and_line(root, element));
                    throw parse_error(file_and_line.first,
                                      file_and_line.second,
                                      std::string("type of parameter '")+a.name+"' not (yet?) supported");
                }

                f.args.push_back(a);
            }
        }
        std::cerr << get_file_and_line_as_string(root, element) << ":\texport procedure: " << f << std::endl;
        m_procedures.push_back(f);
    }

    void visit_node(const xmlpp::Element* root, const xmlpp::Node* node = NULL, unsigned int indentation = 0)
    {
        if (!node) node=root;

        const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
        const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
        const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

        if (nodeText && nodeText->is_white_space()) //Let's ignore the indenting - you don't always want to do this.
            return;

        std::string nodename = node->get_name();

        if (!nodeText && !nodeComment && !nodename.empty()) //Let's not say "name: text".
        {
            if (node->get_name() == "Function") parse_function(root, node);
        }
        if (!nodeContent)
        {
            //Recurse through child nodes:
            xmlpp::Node::NodeList list = node->get_children();
            for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
            {
                visit_node(root, *iter, indentation + 2); //recursive
            }
        }
    }
};

void output_common_hpp(std::ostream &o, const std::string &group, const std::list<t2n_procedure> &procs)
{
    o << "class " << groupClass(group) << " : public libt2n::command\n"
    << "{\n"
    << "private:\n"
    << " friend class boost::serialization::access;\n"
    << " template<class Archive>\n"
    << " void serialize(Archive & ar, const unsigned int /* version */)\n"
    << " {ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::command);}\n"
    << "};\n";

    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
    {
        o << "class " << it->ret_classname() << " : public libt2n::result\n"
        << "{\n"
        << "private:\n";
        if (it->hasReturn())
            o << " " << it->ret_type << " res;\n";
        o << " friend class boost::serialization::access;\n"
        << " template<class Archive>\n"
        << " void serialize(Archive & ar, const unsigned int /* version */)\n"
        << " {\n"
        << "  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(libt2n::result);\n";
        if (it->hasReturn())
            o << "  ar & BOOST_SERIALIZATION_NVP(res);\n";
        o << " }\n"
        << "public:\n"
        << " " << it->ret_classname() << "() {}\n";
        if (it->hasReturn())
        {
            o << " " << it->ret_classname() << "(const " << it->ret_type << " &_res) : res(_res) {}\n"
            << " " << it->ret_type << " get_data() { return res; }\n";
        }
        o << "};\n";
    }
    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
    {
        o << "class " << it->cmd_classname() << " : public " << groupClass(group) << "\n"
        << "{\n"
        << "private:\n";
        for (t2n_procedure::Args::const_iterator ait=it->args.begin();ait!=it->args.end();++ait)
        {
            o << " " << ait->type.noref() << " " << ait->name << ";\n";
        }
        o << " friend class boost::serialization::access;\n"
        << " template<class Archive>\n"
        << " void serialize(Archive & ar, const unsigned int /* version */)\n"
        << " {\n"
        << "  ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(" << groupClass(group) << ");\n";
        for (t2n_procedure::Args::const_iterator ait=it->args.begin();ait!=it->args.end();++ait)
        {
            o << "  ar & BOOST_SERIALIZATION_NVP(" << ait->name << ");\n";
        }

        // default constructor
        o << " }\n"
        << "\n"
        << "public:\n"
        << " " << it->cmd_classname() << "() {}\n";

        // constructor taking all arguments
        if (!it->args.empty())
        {
            o << " " << it->cmd_classname() << "(";
            for (t2n_procedure::Args::const_iterator ait=it->args.begin();ait!=it->args.end();++ait)
            {
                if (ait!=it->args.begin()) o << ", ";
                o << ait->type << " _" << ait->name;
            }
            o << ") : ";
            for (t2n_procedure::Args::const_iterator ait=it->args.begin();ait!=it->args.end();++ait)
            {
                if (ait!=it->args.begin()) o << ", ";
                // pointers are const pointers and must be dereferenced
                o << ait->name << "(" << ((ait->type.name.find_first_of('*')!=std::string::npos) ? "*" : "" ) << "_" << ait->name << ")";
            }
            o << " {}\n";
        }
        o << " libt2n::result* operator()();\n"
        << "};\n";
    }
}

void output_common_cpp(std::ostream &o, const std::string &group, const std::list<t2n_procedure> &procs, const std::string &common_hpp)
{
    o << "#include \"" << common_hpp << "\"\n"
    << "#include <boost/serialization/export.hpp>\n"
    << "\n"
    << "/* register types with boost serialization */\n";
    o << "BOOST_CLASS_EXPORT(" << groupClass(group) << ")\n";
    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
    {
        o << "BOOST_CLASS_EXPORT("<<it->ret_classname()<<")\n"
        << "BOOST_CLASS_EXPORT("<<it->cmd_classname()<<")\n";
    }
}

void output_client_hpp(std::ostream &o, const std::string &group, const std::list<t2n_procedure> &procs)
{
    o << "#include <command_client.hxx>\n";

    o << "class " << groupClass(group) << "_client : public libt2n::command_client\n"
    << "{\n"
    << "public:\n"
    << groupClass(group) << "_client(libt2n::client_connection *_c,\n"
    << " long long _command_timeout_usec=command_timeout_usec_default,\n"
    << " long long _hello_timeout_usec=hello_timeout_usec_default)\n"
    << " : libt2n::command_client(_c,_command_timeout_usec,_hello_timeout_usec)\n"
    << " {}\n";
    for (std::list<t2n_procedure>::const_iterator pit=procs.begin();pit!=procs.end();++pit)
    {
        o << " " << *pit << ";\n";
    }
    o << "};\n";
}

void output_client_cpp(std::ostream &o, const std::string &group, const std::list<t2n_procedure> &procs, const std::string &common_hpp, const std::string &common_cpp, const std::string &client_hpp)
{
    o << "#include \"" << client_hpp << "\"\n"
    << "#include \"" << common_hpp << "\"\n"
    << "// fake\n";
    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
    {
        o << "libt2n::result* " << it->cmd_classname() << "::operator()() { return NULL; }\n";
    }

    for (std::list<t2n_procedure>::const_iterator pit=procs.begin();pit!=procs.end();++pit)
    {
        o << pit->ret_type << " " << groupClass(group) << "_client::" << pit->name << "(";

        // we need to do this by hand here because we don't want default arguments within the cpp
        for (t2n_procedure::Args::const_iterator xit=pit->args.begin();xit!=pit->args.end();++xit)
        {
            if (xit!=pit->args.begin())
                o << ", ";
            o << xit->type << " " << xit->name;
        }

        o << ")\n"
        << "{\n"
        << " libt2n::result_container rc;\n"
        << " send_command(new " << pit->cmd_classname() << "(";
        for (t2n_procedure::Args::const_iterator ait=pit->args.begin();ait!=pit->args.end();++ait)
        {
            if (ait!=pit->args.begin()) o << ", ";
            o << ait->name;
        }
        o << "), rc);\n"
        << " " << pit->ret_classname() << "* res=dynamic_cast<" << pit->ret_classname() << "*>(rc.get_result());\n"
        << " if (!res) throw libt2n::t2n_communication_error(\"result object of wrong type\");\n";
        if (pit->hasReturn())
            o << " return res->get_data();\n";
        o << "}\n";
    }

    // include in this compilation unit to ensure the compilation unit is used
    // see also:
    // http://www.google.de/search?q=g%2B%2B+static+initializer+in+static+library
    o << "#include \"" << common_cpp << "\"\n";
}

void output_server_hpp(std::ostream &o, const std::string & /* group */, const std::list<t2n_procedure> &procs, const std::string &common_hpp)
{
    o << "#include \"" << common_hpp << "\"\n";

    // output function declarations
    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
        o << *it << ";\n";
}

void output_server_cpp(std::ostream &o, const std::string &group, const std::list<t2n_procedure> &procs, const std::string &common_hpp, const std::string &common_cpp)
{
    o << "#include \"" << common_hpp << "\"\n";

    for (std::list<t2n_procedure>::const_iterator it=procs.begin();it!=procs.end();++it)
    {
        o << *it << ";\n";
        o << "libt2n::result* " << it->cmd_classname() << "::operator()() { ";

        if (it->hasReturn())
            o << "return new " << it->ret_classname() << "(";

        // output function name and args
        o << it->name << "(";
        for (t2n_procedure::Args::const_iterator ait=it->args.begin();ait!=it->args.end();++ait)
        {
            if (ait!=it->args.begin()) o << ", ";
            // get pointer
            if (ait->type.name.find_first_of('*')!=std::string::npos)
                o << '&';
            o << ait->name;
        }

        if (it->hasReturn())
            o << "));";
        else
            o << "); return new " << it->ret_classname() << "();";

        o << " }\n";
    }
    o << "#include \"" << common_cpp << "\"\n";
}

struct header_file : public std::ofstream
{
    header_file(const char* fname) : std::ofstream(fname)
    {
        std::cerr << "create header: '" << fname << "'" << std::endl;
        std::string macro(replace(toupper(fname),'.','_'));
        *this << "// automatically generated code (generated by libt2n-codegen " << VERSION << ") - do not edit\n" << std::endl;
        *this << "#ifndef " << macro << "\n"
        << "#define " << macro << "\n";
    }
    ~header_file()
    {
        *this << "#endif" << std::endl;
    }
};

struct cpp_file : public std::ofstream
{
    cpp_file(const char* fname) : std::ofstream(fname)
    {
        std::cerr << "create cpp: '" << fname << "'" << std::endl;
        *this << "// automatically generated code - do not edit\n" << std::endl;
    }
};

int
main(int argc, char* argv[])
{
    // todo: maybe use getopt
    if ((argc>1)&&(std::string(argv[1])=="--version"))
    {
        std::cerr << VERSION << std::endl;
        return 0;
    }
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << "default-group gccxml-file1 gccxml-file2 ... " << std::endl;
        return 1;
    }

    try
    {
        std::string group(argv[1]);
        std::list<std::string> xmlfiles;
        for (int i=2;i<argc;++i)
            xmlfiles.push_back(argv[i]);

        std::string prefix=group+"_";
        std::list<t2n_procedure> procedures;
        for (std::list<std::string>::iterator it=xmlfiles.begin();it!=xmlfiles.end();++it)
        {
            std::cerr << "Parse " << *it << std::endl;
            Parser parser(*it);
            const std::list<t2n_procedure> &p(parser.get_procedures());
            std::copy(p.begin(), p.end(), std::back_inserter(procedures));
        }

        std::cerr << "All procedures:" << std::endl;
        for (std::list<t2n_procedure>::const_iterator it=procedures.begin();it!=procedures.end();++it)
            std::cerr << *it << ";" << std::endl;

        std::string common_hpp_fname(prefix+"common.hxx");
        std::string common_cpp_fname(prefix+"common.cpp");
        std::string client_hpp_fname(prefix+"client.hxx");
        std::string client_cpp_fname(prefix+"client.cpp");
        std::string server_hpp_fname(prefix+"server.hxx");
        std::string server_cpp_fname(prefix+"server.cpp");

        header_file common_hpp(common_hpp_fname.c_str());
        common_hpp << "// boost serialization is picky about order of include files => we have to include this one first\n"
        << "#include \"codegen-stubhead.hxx\"\n"
        << "#include \"" << group << ".hxx\"\n";

        output_common_hpp(common_hpp, group, procedures);

        cpp_file common_cpp(common_cpp_fname.c_str());
        output_common_cpp(common_cpp, group, procedures, common_hpp_fname);

        header_file client_hpp(client_hpp_fname.c_str());
        client_hpp << "// boost serialization is picky about order of include files => we have to include this one first\n"
        << "#include \"codegen-stubhead.hxx\"\n"
        << "#include \"" << group << ".hxx\"\n";
        output_client_hpp(client_hpp, group, procedures);

        cpp_file client_cpp(client_cpp_fname.c_str());
        output_client_cpp(client_cpp, group, procedures, common_hpp_fname, common_cpp_fname, client_hpp_fname);

        header_file server_hpp(server_hpp_fname.c_str());
        output_server_hpp(server_hpp, group, procedures, common_hpp_fname);

        cpp_file server_cpp(server_cpp_fname.c_str());
        output_server_cpp(server_cpp, group, procedures, common_hpp_fname, common_cpp_fname);
    }
    catch (const parse_error &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
