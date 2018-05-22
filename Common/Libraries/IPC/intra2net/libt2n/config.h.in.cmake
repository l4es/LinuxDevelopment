/* asyncio_config.hpp.in.cmake: configure input file for cmake */

/* define if the Boost library is available */
#cmakedefine HAVE_BOOST

/* define if the Boost::Serialization library is available */
#cmakedefine HAVE_BOOST_SERIALIZATION

/* define if the Boost::Unit_Test_Framework library is available */
#cmakedefine HAVE_BOOST_UNIT_TEST_FRAMEWORK

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine01 HAVE_DLFCN_H

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine01 HAVE_INTTYPES_H

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine01 HAVE_MEMORY_H

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine01 HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine01 HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine01 HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine01 HAVE_STRING_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine01 HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine01 HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine01 HAVE_UNISTD_H

/* Name of package */
#define PACKAGE "${PROJECT_NAME}"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "${PROJECT_NAME}"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "${PROJECT_NAME} ${VERSION}"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "${PROJECT_NAME}"

/* Define to the version of this package. */
#define PACKAGE_VERSION "${VERSION}"

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine01 STDC_HEADERS

/* Version number of package */
#define VERSION "${VERSION}"

/* Version number of used protocol (integer, increase if incompatible) */
#define PROTOCOL_VERSION ${PROTOCOL_VERSION}
