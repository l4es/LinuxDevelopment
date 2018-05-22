# Sources
set(testsimpleio_SOURCES test_simpleio_basics.cpp)
include_directories(BEFORE
    ${CMAKE_SOURCE_DIR}/asyncio
    ${CMAKE_SOURCE_DIR}/utils
)

# Executables
add_executable(testsimpleio ${testsimpleio_SOURCES})

# Dependencies
target_link_libraries(testsimpleio libasyncio_utils libasyncio ${Boost_LIBRARIES} ${LIBI2NCOMMON_LIBRARIES})

# Tests
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS testsimpleio)
enable_testing()
add_test(testsimpleio testsimpleio)
