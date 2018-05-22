# Project
project(libt2n-example1)
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
set(t2nexample_GROUP ${CMAKE_CURRENT_SOURCE_DIR}/t2nexample.cpp)
# include CMake snippet doing all the magic
include(${CMAKE_SOURCE_DIR}/codegen/Libt2n.cmake)
# command groups are listed here (seperated by spaces)
setup_libt2n(t2nexample)

# Sources
# for each group build a client library from generated source file(s)
# Note: the library name must match the group name
set(libt2nexample_SOURCES
    t2nexample_client.cpp
)
set(libt2n_example1_server_SOURCES
    server.cpp
    ${t2nexample_GROUP}
    t2nexample_server.cpp
)

# Executables
# build server program
add_executable(libt2n-example1-server ${libt2n_example1_server_SOURCES})

# Libraries
add_library(libt2nexample STATIC ${libt2nexample_SOURCES})
set_target_properties(libt2nexample PROPERTIES OUTPUT_NAME t2nexample)

# Dependencies
target_link_libraries(libt2n-example1-server ${LIBT2N_LIBRARIES})
target_link_libraries(libt2nexample t2nexample ${LIBT2N_LIBRARIES})

add_dependencies(libt2n-example1-server t2n_arnielizer_codegen_done)
add_dependencies(libt2nexample t2n_arnielizer_codegen_done)

# Installation
# install(TARGETS libt2n-example1-server DESTINATION bin)
# install(TARGETS libt2nexample ARCHIVE DESTINATION lib)
