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
#ifndef __LIBT2N_COMMAND
#define __LIBT2N_COMMAND

#include <iostream>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>

namespace libt2n
{

/** @brief base class for the results that are returned from commands.
*/
class result
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & /* ar */, const unsigned int /* version */)
        { }

    public:
        result() {}
        virtual ~result() {}
};
}
//BOOST_IS_ABSTRACT(libt2n::result)
BOOST_CLASS_TRACKING(libt2n::result, boost::serialization::track_never)

namespace libt2n
{
/** @brief a command that can be serialized. abstract.
*/
class command
{
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & /* ar */, const unsigned int /* version */)
        { }

    public:
        /// this calls the wanted target function on the server
        virtual result* operator()()=0;
        virtual ~command() {}
};
} // namespace libt2n
//BOOST_IS_ABSTRACT(libt2n::command)
BOOST_CLASS_TRACKING(libt2n::command, boost::serialization::track_never)


#endif

