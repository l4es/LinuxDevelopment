# Project
project(libt2n)
set(VERSION 0.8)
set(PROTOCOL_VERSION 1) # protocol version used (integers, increase version if incompatible)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

# CMake
set(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# Source package generation
set(CPACK_SOURCE_GENERATOR             TGZ)
set(CPACK_SOURCE_IGNORE_FILES          "~$;\\\\.git;build;build-rpm")
set(CPACK_SOURCE_PACKAGE_FILE_NAME     "${PROJECT_NAME}-${VERSION}")

# "make dist" target
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${VERSION})
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
            | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Find external packages
include(FindPkgConfig)

find_package(Boost 1.33 REQUIRED COMPONENTS serialization unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

pkg_check_modules(XMLPP REQUIRED libxml++-2.6>=2.8.1)
include_directories(${XMLPP_INCLUDE_DIRS})
link_directories(${XMLPP_LIBRARY_DIRS})

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
   # Find doxy config
   message(STATUS "Doxygen found.")
   set(DOXY_DIR "${CMAKE_SOURCE_DIR}/doc")
   set(DOXY_CONFIG "${DOXY_DIR}/Doxyfile.in")

   # Copy doxy.config.in
   set(top_srcdir ${CMAKE_SOURCE_DIR})
   configure_file("${DOXY_CONFIG}" "${CMAKE_BINARY_DIR}/doxy.config")

   # Create doc directory
   add_custom_command(
   OUTPUT ${CMAKE_BINARY_DIR}/doc
   COMMAND rm -rf ${CMAKE_BINARY_DIR}/doc/{html,man}
   COMMAND mkdir -p ${CMAKE_BINARY_DIR}/doc
   DEPENDS codegen examples examples-codegen src test
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
configure_file(${CMAKE_SOURCE_DIR}/libt2n.spec.in ${CMAKE_SOURCE_DIR}/libt2n.spec @ONLY)

# Check include files and availability of libraries for config file
include(CheckIncludeFiles)

if(Boost_FOUND)
    set(HAVE_BOOST 1)
endif()
if(Boost_SERIALIZATION_FOUND)
    set(HAVE_BOOST_SERIALIZATION 1)
endif()
if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
    set(HAVE_BOOST_UNIT_TEST_FRAMEWORK 1)
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
configure_file(${CMAKE_SOURCE_DIR}/config.h.in.cmake ${CMAKE_BINARY_DIR}/config.h)
include_directories(BEFORE ${CMAKE_BINARY_DIR})

# PkgConfig output
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib)
set(bindir      ${exec_prefix})
set(datadir     ${CMAKE_INSTALL_PREFIX}/share)

# PkgConfig files
configure_file(${CMAKE_SOURCE_DIR}/libt2n.pc.in ${CMAKE_BINARY_DIR}/libt2n.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libt2n.pc DESTINATION lib/pkgconfig)

# Installation of scripts
install(PROGRAMS libt2n-gccxml.sh DESTINATION bin)

# Subdirectories
set(CPACK_SET_DESTDIR "ON")
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(codegen)
add_subdirectory(examples EXCLUDE_FROM_ALL)
add_subdirectory(examples-codegen EXCLUDE_FROM_ALL)

include(CPack)
