/*
Copyright (C) 2006 by Intra2net AG - Gerd v. Egidy

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
#ifndef __LIBT2N_T2N_EXCEPTION
#define __LIBT2N_T2N_EXCEPTION

#include <stdexcept>
#include <string>

#include <boost/serialization/serialization.hpp>

namespace boost {
namespace serialization {

// make std::exception serializable
template<class Archive>
void serialize(Archive & ar, std::exception & g, const unsigned int version)
{
}

} // namespace serialization
} // namespace boost

namespace libt2n
{
/** @brief a generic exception that can be handeled with libt2n
    @note don't derive the exceptions your application generates directly from this one
          but use libt2n::t2n_runtime_error for this
*/
class t2n_exception : public std::exception
{
    private:
        std::string message;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_exception(const std::string& _message)
            { message=_message; }

        t2n_exception()
            { }

        virtual const char* what() const throw()
            { return message.c_str(); }

        virtual t2n_exception* clone() const
            { return new t2n_exception(*this); }

        virtual ~t2n_exception() throw()
            {}

        virtual void do_throw()
            { throw *this; }
};

/// a (unspecified) problem with libt2n communication
class t2n_communication_error : public t2n_exception
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_communication_error(const std::string& _message)
            : t2n_exception(_message)
            { }

        t2n_communication_error()
            { }

        t2n_exception* clone() const
            { return new t2n_communication_error(*this); }

        void do_throw()
            { throw *this; }
};

/// can't connect to libt2n server
class t2n_connect_error : public t2n_communication_error
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_connect_error(const std::string& _message)
            : t2n_communication_error(_message)
            { }

        t2n_connect_error()
            { }

        t2n_exception* clone() const
            { return new t2n_connect_error(*this); }

        void do_throw()
            { throw *this; }
};

/// error establishing a socket on the server (only thrown on the server-side)
class t2n_server_error : public t2n_communication_error
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_server_error(const std::string& _message)
            : t2n_communication_error(_message)
            { }

        t2n_server_error()
            { }

        t2n_exception* clone() const
            { return new t2n_server_error(*this); }

        void do_throw()
            { throw *this; }
};

/// error transmitting or receiving libt2n messages
class t2n_transfer_error : public t2n_communication_error
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_transfer_error(const std::string& _message)
            : t2n_communication_error(_message)
            { }

        t2n_transfer_error()
            { }

        t2n_exception* clone() const
            { return new t2n_transfer_error(*this); }

        void do_throw()
            { throw *this; }
};

/// tried to talk to an incompatible libt2n version
class t2n_version_mismatch : public t2n_communication_error
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_version_mismatch(const std::string& _message)
            : t2n_communication_error(_message)
            { }

        t2n_version_mismatch()
            { }

        t2n_exception* clone() const
            { return new t2n_version_mismatch(*this); }

        void do_throw()
            { throw *this; }
};

/// illegal libt2n command received
class t2n_command_error : public t2n_exception
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_command_error(const std::string& _message)
            : t2n_exception(_message)
            { }

        t2n_command_error()
            { }

        t2n_exception* clone() const
            { return new t2n_command_error(*this); }

        void do_throw()
            { throw *this; }
};

/// error serializing or deserializing a libt2n command packet
class t2n_serialization_error : public t2n_exception
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_serialization_error(const std::string& _message)
            : t2n_exception(_message)
            { }

        t2n_serialization_error()
            { }

        t2n_exception* clone() const
            { return new t2n_serialization_error(*this); }

        void do_throw()
            { throw *this; }
};

/** @brief a runtime error within the remote function.
 derive your own custom exceptions from this one

 @note you must override the virtual clone method if you do so (used by libt2n::command_server::handle())
*/
class t2n_runtime_error : public t2n_exception
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        t2n_runtime_error(const std::string& _message)
            : t2n_exception(_message)
            { }

        t2n_runtime_error()
            { }

        t2n_exception* clone() const
            { return new t2n_runtime_error(*this); }

        void do_throw()
            { throw *this; }
};

}  // namespace libt2n

#include "t2n_exception.tcc"

#endif
