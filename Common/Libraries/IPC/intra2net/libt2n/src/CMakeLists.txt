# Sources
set(libt2n_SOURCES
    client.cpp
    client_wrapper.cpp
    command.cpp
    command_client.cpp
    command_server.cpp
    connection.cpp
    container.cpp
    server.cpp
    socket_client.cpp
    socket_handler.cpp
    socket_server.cpp
    socket_wrapper.cpp
    t2n_exception.cpp
)
set(libt2n_HEADERS 
    client.hxx
    client_wrapper.hxx
    command.hxx
    command_client.hxx
    command_server.hxx
    connection.hxx
    container.hxx
    log.hxx
    server.hxx
    socket_client.hxx
    socket_handler.hxx
    socket_server.hxx
    socket_wrapper.hxx
    t2n_exception.hxx
    types.hxx
    container.tcc
    t2n_exception.tcc
)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# Libraries
add_library(libt2n SHARED ${libt2n_SOURCES} ${libt2n_HEADERS})
set_target_properties(libt2n PROPERTIES VERSION ${VERSION} SOVERSION 1)
set_target_properties(libt2n PROPERTIES OUTPUT_NAME t2n)
set_target_properties(libt2n PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_library(libt2n-static STATIC ${libt2n_SOURCES} ${libt2n_HEADERS})
set_target_properties(libt2n-static PROPERTIES OUTPUT_NAME t2n)
set_target_properties(libt2n-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Installation
install(FILES ${libt2n_HEADERS} DESTINATION include)
install(TARGETS libt2n LIBRARY DESTINATION lib)
install(TARGETS libt2n-static ARCHIVE DESTINATION lib)
