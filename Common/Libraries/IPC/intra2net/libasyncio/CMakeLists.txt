# Project
project(libasyncio)
set(VERSION 0.3)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

# CMake
set(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# Source package generation
set(CPACK_SOURCE_GENERATOR             TGZ)
set(CPACK_SOURCE_IGNORE_FILES          "~$;\\\\.git;build;build-rpm")
set(CPACK_SOURCE_PACKAGE_FILE_NAME     "${PROJECT_NAME}-${VERSION}")

# Find external packages
include(FindPkgConfig)

option(WITH_LIBI2NCOMMON "Build with libi2ncommon support" OFF)
if(WITH_LIBI2NCOMMON)
    message(STATUS "[!] Building *with* libi2ncommon support. [!]")
    pkg_check_modules(LIBI2NCOMMON REQUIRED libi2ncommon)
    include_directories(${LIBI2NCOMMON_INCLUDE_DIRS})
    link_directories(${LIBI2NCOMMON_LIBRARY_DIRS})
else(WITH_LIBI2NCOMMON)
    message(STATUS "[!] Building *without* libi2ncommon support. [!]")
endif(WITH_LIBI2NCOMMON)

find_package(Boost 1.34 REQUIRED COMPONENTS signals unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

option(WITH_LIBT2N "Build with libt2n support" OFF)
if(WITH_LIBT2N)
    message(STATUS "[!] Building *with* libt2n support. [!]")
    pkg_check_modules(LIBT2N REQUIRED libt2n)
    include_directories(${LIBT2N_INCLUDE_DIRS})
    link_directories(${LIBT2N_LIBRARY_DIRS})
    add_subdirectory(glue_t2n)
else(WITH_LIBT2N)
    message(STATUS "[!] Building *without* libt2n support. [!]")
endif(WITH_LIBT2N)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
   # Find doxy config
   message(STATUS "Doxygen found.")
   set(DOXY_DIR "${CMAKE_SOURCE_DIR}/doc")
   set(DOXY_CONFIG "${DOXY_DIR}/Doxyfile.in")
   set(top_srcdir ${CMAKE_SOURCE_DIR})

   # Copy doxy.config.in
   configure_file("${DOXY_CONFIG}" "${CMAKE_BINARY_DIR}/doxy.config")

   # Create doc directory
   add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/doc
   COMMAND rm -rf ${CMAKE_BINARY_DIR}/doc/{html,man}
   COMMAND mkdir -p ${CMAKE_BINARY_DIR}/doc
   DEPENDS utils asyncio ${T2NDIRS} unittest
   )

   # Run doxygen
   add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/doc/html/index.html
   COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_BINARY_DIR}/doxy.config"
   DEPENDS "${CMAKE_BINARY_DIR}/doxy.config" "${CMAKE_BINARY_DIR}/doc"
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
   )

   add_custom_target(docs ALL DEPENDS ${CMAKE_BINARY_DIR}/doc/html/index.html)

   message(STATUS "Generating API documentation with Doxygen.")
else(DOXYGEN_FOUND)
   message(STATUS "Not generating API documentation.")
endif(DOXYGEN_FOUND)

# Spec file
configure_file(${CMAKE_SOURCE_DIR}/libasyncio.spec.in ${CMAKE_SOURCE_DIR}/libasyncio.spec @ONLY)

# Check include files and availability of libraries for config file
include(CheckIncludeFiles)

if(Boost_FOUND)
    set(HAVE_BOOST 1)
endif()
if(Boost_SIGNALS_FOUND)
    set(HAVE_BOOST_SIGNALS 1)
endif()
if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
    set(HAVE_BOOST_UNIT_TEST_FRAMEWORK 1)
endif()
if(LIBI2NCOMMON_FOUND)
    set(HAVE_LIBI2NCOMMON 1)
endif()
if(LIBT2N_FOUND)
    set(HAVE_LIBT2N 1)
endif()
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)
set(STDC_HEADER_NAMES
    assert.h
    ctype.h
    errno.h
    float.h
    limits.h
    locale.h
    math.h
    setjmp.h
    signal.h
    stdarg.h
    stddef.h
    stdio.h
    stdlib.h
    string.h
    time.h
)
check_include_files("${STDC_HEADER_NAMES}" STDC_HEADERS)

# Config file
# configure_file(${CMAKE_SOURCE_DIR}/asyncio_config.hpp.in.cmake ${CMAKE_BINARY_DIR}/asyncio_config.hpp)
# include_directories(BEFORE ${CMAKE_BINARY_DIR})
configure_file(${CMAKE_SOURCE_DIR}/asyncio_config.hpp.in.cmake ${CMAKE_SOURCE_DIR}/asyncio/asyncio_config.hpp)
# include_directories(BEFORE ${CMAKE_SOURCE_DIR}/asyncio)

# PkgConfig output
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib)

# PkgConfig files
configure_file(${CMAKE_SOURCE_DIR}/asyncio/libasyncio.pc.in ${CMAKE_BINARY_DIR}/asyncio/libasyncio.pc @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/utils/libasyncio_utils.pc.in ${CMAKE_BINARY_DIR}/utils/libasyncio_utils.pc @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/glue_t2n/libasyncio_t2n.pc.in ${CMAKE_BINARY_DIR}/glue_t2n/libasyncio_t2n.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/asyncio/libasyncio.pc DESTINATION lib/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/utils/libasyncio_utils.pc DESTINATION lib/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/glue_t2n/libasyncio_t2n.pc DESTINATION lib/pkgconfig)

# Subdirectories
set(CPACK_SET_DESTDIR "ON")
add_subdirectory(utils)
add_subdirectory(asyncio)
add_subdirectory(unittest)

include(CPack)
