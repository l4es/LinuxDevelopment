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

#include "t2n_exception.hxx"

#include <boost/serialization/export.hpp>

BOOST_CLASS_EXPORT(libt2n::t2n_exception)
BOOST_CLASS_EXPORT(libt2n::t2n_communication_error)
BOOST_CLASS_EXPORT(libt2n::t2n_connect_error)
BOOST_CLASS_EXPORT(libt2n::t2n_server_error)
BOOST_CLASS_EXPORT(libt2n::t2n_transfer_error)
BOOST_CLASS_EXPORT(libt2n::t2n_version_mismatch)
BOOST_CLASS_EXPORT(libt2n::t2n_command_error)
BOOST_CLASS_EXPORT(libt2n::t2n_serialization_error)
BOOST_CLASS_EXPORT(libt2n::t2n_runtime_error)

namespace libt2n
{


} // namespace libt2n
