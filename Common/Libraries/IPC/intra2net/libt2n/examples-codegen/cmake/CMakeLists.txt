# Show basic usage of libt2n and cmake
# Requires Libt2n.cmake

# Setup libt2n
set(t2n_mydaemon_GROUP
    ${CMAKE_SOURCE_DIR}/src/t2n_mydaemon.cpp
)
include(${CMAKE_SOURCE_DIR}/codegen/Libt2n.cmake)
# Or "include(Libt2n)" if Libt2n.cmake is already installed in /${CMAKE_ROOT}/Modules

# Call setup_libt2n with the CMDGROUP name
setup_libt2n(t2n_mydaemon)

# --------------------------------------------------------------------------------

set(keymaker_cpp_sources
    something.cpp
    ${t2n_mydaemon_GROUP}
)

add_executable(mydaemon main.cpp ${keymaker_cpp_sources} t2n_mydaemon_server.cpp)
target_link_libraries(mydaemon ${LIBT2N_LIBRARIES})
# Wait till generated code is available
add_dependencies(mydaemon t2n_mydaemon_codegen_done)
