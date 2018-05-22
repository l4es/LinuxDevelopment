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
#ifndef __LIBT2N_CLIENT
#define __LIBT2N_CLIENT

#include <string>
#include <iostream>

#include "connection.hxx"
#include "types.hxx"

namespace libt2n
{

/** @brief a (generic) connection from client to server. Abstract.
 */
class client_connection : public connection
{
    private:
        log_level_values log_level;
        std::ostream *logstream;

    public:
        client_connection();

        void set_logging(std::ostream *_logstream, log_level_values _log_level);

        std::ostream* get_logstream(log_level_values level);
};

}

#endif
