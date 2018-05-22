# Project
project(libt2n-example1-client-wrapper)
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

# pkg_check_modules(T2NEXAMPLE REQUIRED t2nexample=0.1)
# include_directories(${T2NEXAMPLE_INCLUDE_DIRS})
# link_directories(${T2NEXAMPLE_LIBRARY_DIRS})

# include(CPack)

# Sources
set(libt2n_example1_client_wrapper_SOURCES
    client.cpp
)

# Executables
add_executable(libt2n-example1-client-wrapper ${libt2n_example1_client_wrapper_SOURCES})

# Dependencies
# target_link_libraries(libt2n-example1-client-wrapper ${T2NEXAMPLE_LIBRARIES})
target_link_libraries(libt2n-example1-client-wrapper t2nexample)

# Installation
# install(TARGETS libt2n-example1-client-wrapper DESTINATION bin)
