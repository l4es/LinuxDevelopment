# Sources
set(test_SOURCES
    newserver.cpp
    callback.cpp
    cmdgroup.cpp
    comm.cpp
    hello.cpp
    reconnect.cpp
    reentrant.cpp
    serialize.cpp
    simplecmd.cpp
    timeout.cpp
    wrapper.cpp
    getsocket.cpp
    test_fixtures.hxx
)
include_directories(BEFORE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Executables
add_executable(test ${test_SOURCES})

# Dependencies
target_link_libraries(test libt2n ${Boost_SERIALIZATION_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})

# Tests
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test)
enable_testing()
add_test(test test)
