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
#ifndef __LIBT2N_LOG
#define __LIBT2N_LOG

#include <iostream>
#include <sstream>

#define LOGGING

#ifdef LOGGING

#define LOGSTREAM(level,pipe) \
            do { \
                std::ostream* streamptr; \
                if ((streamptr=get_logstream(level))!=NULL) \
                    (*streamptr) << pipe << std::endl; \
            } while (0)

#define OBJLOGSTREAM(obj,level,pipe) \
            do { \
                std::ostream* streamptr; \
                if ((streamptr=obj.get_logstream(level))!=NULL) \
                    (*streamptr) << pipe << std::endl; \
            } while (0)

#define EXCEPTIONSTREAM(loglevel,exception,pipe) \
            do { \
                std::ostringstream ostr; \
                ostr << pipe; \
                std::ostream* streamptr; \
                if ((streamptr=get_logstream(loglevel))!=NULL) \
                    (*streamptr) << ostr.str() << std::endl; \
                throw exception(ostr.str()); \
            } while (0)

#else

#define LOGSTREAM(level,pipe)
#define OBJLOGSTREAM(obj,level,pipe)
#define EXCEPTIONSTREAM(loglevel,exception,pipe)

#endif

#endif
