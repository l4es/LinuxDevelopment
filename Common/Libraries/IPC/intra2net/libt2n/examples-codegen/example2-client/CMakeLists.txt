# Project
project(libt2n-example2-client)
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
# include(FindPkgConfig)

# pkg_check_modules(DEFAULT REQUIRED default=0.1)
# include_directories(${DEFAULT_INCLUDE_DIRS})
# link_directories(${DEFAULT_LIBRARY_DIRS})

include(CPack)

# Sources
set(libt2n_example2_client_SOURCES
    client.cpp
    )

# Executables
add_executable(libt2n-example2-client ${libt2n_example2_client_SOURCES})

# Dependencies
# target_link_libraries(libt2n-example2-client ${DEFAULT_LIBRARIES})
target_link_libraries(libt2n-example2-client default)

# Installation
# install(TARGETS libt2n-example2-client DESTINATION bin)
