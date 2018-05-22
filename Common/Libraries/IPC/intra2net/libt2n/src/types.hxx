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
#ifndef __LIBT2N_TYPES
#define __LIBT2N_TYPES

namespace libt2n
{

/// possible levels for logging
enum log_level_values { none=0, error=1, debug=2, fulldebug=3 };

/// possible types of a socket (tcp and unix)
enum socket_type_value { tcp_s, unix_s };

/// possible events for callback, __events_end must be the last event and must not be used 
/// for anything else than marking the end
enum callback_event_type { new_connection=0, connection_closed, connection_deleted, __events_end };

}

#endif
