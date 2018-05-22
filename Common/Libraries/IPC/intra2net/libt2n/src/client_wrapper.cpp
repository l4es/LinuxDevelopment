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

#include <client_wrapper.hxx>

namespace libt2n
{

const char* T2nSingletonWrapperMessages::NotInitializedMessage = "T2nSingletonWrapper used before setting initializing connection";

/// get pointer to logging stream, returns NULL if no logging needed
std::ostream* ConnectionWrapper::get_logstream(log_level_values level)
{
    if (logstream && log_level >= level)
        return logstream;
    else
        return NULL;
}

/// activate logging to the given stream. everything above the given level is logged.
void ConnectionWrapper::set_logging(std::ostream *_logstream, log_level_values _log_level)
{
    log_level=_log_level;
    logstream=_logstream;
}

/// always call this when you got a new connection to transfer logging settings
void ConnectionWrapper::set_logging_on_connection(client_connection& c)
{
    if (logstream != NULL && log_level > none)
        c.set_logging(logstream,log_level);
}

}
