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

namespace libt2n
{

template<class Archive>
void result_container::serialize(Archive & ar, const unsigned int /* version */)
{
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.

    ar & BOOST_SERIALIZATION_NVP(result_type);
    ar & BOOST_SERIALIZATION_NVP(res);
    ar & BOOST_SERIALIZATION_NVP(ex);
}

template<class Archive>
void command_container::serialize(Archive & ar, const unsigned int /* version */)
{
    ar & BOOST_SERIALIZATION_NVP(cmd);
}



}
