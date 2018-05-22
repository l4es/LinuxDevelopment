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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "container.hxx"

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(libt2n::result_container)
BOOST_CLASS_EXPORT(libt2n::command_container)

namespace libt2n
{


/// deletes the carried result or exception objects
result_container::~result_container()
{
    if (res)
        delete res;
    if (ex)
        delete ex;

    res = NULL;
    ex = NULL;
}

/** @brief returns the result or throw the carried exception.
            ATTENTION: the result object is deleted in the destructor
*/
result* result_container::get_result(void)
{
    if (result_type==exception)
        ex->do_throw();

    return res;
}

/// deletes the carried command
command_container::~command_container()
{
    if (cmd)
        delete cmd;

    cmd = NULL;
}

} // namespace libt2n
