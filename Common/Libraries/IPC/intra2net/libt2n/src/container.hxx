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
#ifndef __LIBT2N_CONTAINER
#define __LIBT2N_CONTAINER

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include "command.hxx"
#include "t2n_exception.hxx"

#include <iostream>

namespace libt2n
{

/** @brief contains the result (return value as libt2n::result or an libt2n::t2n_exception) of a executed command
*/
class result_container
{
    private:
        result *res;
        t2n_exception *ex;

        enum result_type_t { regular, exception } result_type;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        result_container()
            : res(NULL)
            , ex(NULL)
            , result_type(regular)
        {
        }

        result_container(result *_res)
            : res(_res)
            , ex(NULL)
            , result_type(regular)
        {
        }

        result_container(t2n_exception *_ex)
            : res(NULL)
            , ex(_ex)
            , result_type(exception)
        {
        }

        ~result_container();

        void set_result(result *_res)
            { res=_res; ex=0; result_type=regular; }

        void set_exception(t2n_exception *_ex)
            { res=0; ex=_ex; result_type=exception; }

        result* get_result(void);

        bool has_exception()
            { return (result_type==exception && ex != NULL); }
        bool has_result()
            { return (result_type==regular && res != NULL); }
};

/** @brief contains a libt2n::command
*/
class command_container
{
    private:
        command *cmd;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version);

    public:
        command_container()
            : cmd(NULL)
        {}

        command_container(command *_cmd)
            : cmd(_cmd)
        {}

        ~command_container();

        /// return the contained command
        command* get_command()
            { return cmd; }
};

} // namespace libt2n

BOOST_CLASS_TRACKING(libt2n::result_container, boost::serialization::track_never)
BOOST_CLASS_TRACKING(libt2n::command_container, boost::serialization::track_never)

#include "container.tcc"

#endif

