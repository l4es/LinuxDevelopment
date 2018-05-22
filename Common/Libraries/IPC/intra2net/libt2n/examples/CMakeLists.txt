# Sources
set(libminimalistic_client_SOURCES
    minimalistic-stub.hxx
    minimalistic-stub-once.hxx
    minimalistic-client-stub.hxx
    minimalistic-client-stub.cpp
)
set(minimalistic_client_SOURCES
    minimalistic-client.cpp
)
set(minimalistic_server_SOURCES
    minimalistic-server.cpp
    minimalistic-server-stub.cpp
)
include_directories(
    BEFORE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Libraries
add_library(libminimalistic_client STATIC ${libminimalistic_client_SOURCES})
set_target_properties(libminimalistic_client PROPERTIES OUTPUT_NAME minimalistic_client)

# Executables
add_executable(minimalistic_client ${minimalistic_client_SOURCES})
add_executable(minimalistic_server ${minimalistic_server_SOURCES})

# Dependencies
target_link_libraries(minimalistic_client libt2n ${Boost_SERIALIZATION_LIBRARIES} libminimalistic_client)
target_link_libraries(minimalistic_server libt2n ${Boost_SERIALIZATION_LIBRARIES})

# Generation of minimalistic-server.xml
get_property(gcc_include_dirs
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY INCLUDE_DIRECTORIES
)
foreach(gcc_include_dir ${gcc_include_dirs})
    set(gccxml_include_dirs "${gccxml_include_dirs} -I${gcc_include_dir}")
endforeach()
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/minimalistic-server.xml
    COMMAND ${CMAKE_SOURCE_DIR}/libt2n-gccxml.sh ${gccxml_include_dirs} ${CMAKE_CURRENT_SOURCE_DIR}/minimalistic-server.cpp -fxml=${CMAKE_CURRENT_SOURCE_DIR}/minimalistic-server.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/minimalistic-server.cpp
)
