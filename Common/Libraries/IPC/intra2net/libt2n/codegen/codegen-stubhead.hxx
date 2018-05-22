/*
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
#ifndef MINIMALISTIC_STUB_HXX
#define MINIMALISTIC_STUB_HXX

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/serialization.hpp>

#include <string>
#include <t2n_exception.hxx>
#include <command.hxx>

#ifdef __GCCXML__
// used during parsing run with gccxml

/** @brief mark a function to export it via libt2n
    @par Example
    @code
    LIBT2N_EXPORT std::string testfunc(const std::string &str)
    {
        // your code here
    }
    @endcode
*/
#define LIBT2N_EXPORT __attribute((gccxml("libt2n-default")))


/** @brief tell libt2n about a default argument
    @param type type of the default argument
    @param value default value
    @par Example
    @code
    LIBT2N_EXPORT std::string testfunc(LIBT2N_DEFAULT_ARG(const std::string str&,"hello world"))
    {
        // your code here
    }
    @endcode
    @remarks Codegen gets default arguments via this define (gccxml-attribute) because gccxml
             does not correctly output them (e.g. the namespace is ambiguous)
*/
#define LIBT2N_DEFAULT_ARG(type,value) __attribute((gccxml("libt2n-default-arg",#value))) type = value

#else
// used during regular compile

#define LIBT2N_EXPORT
#define LIBT2N_DEFAULT_ARG(type,value) type = value

#endif

#endif
