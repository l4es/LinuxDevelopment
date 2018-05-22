# Project
project(libt2n-example2)
set(VERSION 0.1)
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

pkg_check_modules(LIBT2N REQUIRED libt2n>=0.2)
include_directories(${LIBT2N_INCLUDE_DIRS})
link_directories(${LIBT2N_LIBRARY_DIRS})

include(CPack)

# Setup libt2n
# for each command group list the files to parse for LIBT2N_EXPORT
set(default_GROUP
    ${CMAKE_CURRENT_SOURCE_DIR}/default.cpp
)
# example of a command group using multiple cpp files
set(other_GROUP
    ${CMAKE_CURRENT_SOURCE_DIR}/other-1.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/other-2.cpp
)
#set(CPP_HEADERS
#    ${CMAKE_CURRENT_SOURCE_DIR}/foo.hxx
#)
# include CMake snippet doing all the magic
include(${CMAKE_SOURCE_DIR}/codegen/Libt2n.cmake)
# command groups are listed here (seperated by spaces)
setup_libt2n(default other)

# Sources
# for each group build a client library from generated source file(s)
# Note: the library name must match the group name
set(libdefault_SOURCES
    default_client.cpp
)
set(libother_SOURCES
    other_client.cpp
)
# build an example server and client
set(client_SOURCES
    client.cpp
)
set(libt2n_example2_server_SOURCES
    other_server.cpp
    default_server.cpp
    server.cpp
    ${other_GROUP}
    ${default_GROUP}
)

# Libraries
add_library(libdefault STATIC ${libdefault_SOURCES})
set_target_properties(libdefault PROPERTIES OUTPUT_NAME default)

add_library(libother STATIC ${libother_SOURCES})
set_target_properties(libother PROPERTIES OUTPUT_NAME other)

# Executables
# build server program
add_executable(libt2n-example2-server ${libt2n_example2_server_SOURCES})

# Dependencies
target_link_libraries(libdefault ${LIBT2N_LIBRARIES})
add_dependencies(libdefault default_codegen_done)

target_link_libraries(libother ${LIBT2N_LIBRARIES})
add_dependencies(libother other_codegen_done)

target_link_libraries(libt2n-example2-server ${LIBT2N_LIBRARIES} libdefault libother)
add_dependencies(libt2n-example2-server default_codegen_done other_codegen_done)

# Installation
# install(TARGETS default_client ARCHIVE DESTINATION lib)
# install(TARGETS other_client ARCHIVE DESTINATION lib)
# install(TARGETS libt2n-example2-server DESTINATION bin)
# install(FILES ${CPP_HEADERS} DESTINATION include)
# install(PROGRAMS test DESTINATION ./)
