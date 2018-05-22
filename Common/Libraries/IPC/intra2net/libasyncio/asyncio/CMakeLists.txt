# Sources
set(libasyncio_SOURCES
    async_callout.cpp
    async_io.cpp
    async_pipe.cpp
    async_process.cpp
    async_socket.cpp
    async_timer.cpp
)
set(libasyncio_HEADERS 
    async_callout.hpp
    async_io.hpp
    async_pipe.hpp
    async_process.hpp
    async_socket.hpp
    async_timer.hpp
    asyncio_config.hpp
)
include_directories(BEFORE
		    ${CMAKE_CURRENT_SOURCE_DIR}
		    ${CMAKE_SOURCE_DIR}/utils
		    ${CMAKE_SOURCE_DIR}/utils/i2ncommon
		    )

# Libraries
add_library(libasyncio SHARED ${libasyncio_SOURCES} ${libasyncio_HEADERS})
set_target_properties(libasyncio PROPERTIES VERSION ${VERSION} SOVERSION 1)
set_target_properties(libasyncio PROPERTIES OUTPUT_NAME asyncio)
set_target_properties(libasyncio PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_library(libasyncio-static STATIC ${libasyncio_SOURCES} ${libasyncio_HEADERS})
set_target_properties(libasyncio-static PROPERTIES OUTPUT_NAME asyncio)
set_target_properties(libasyncio-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(libasyncio ${LIBI2NCOMMON_LIBRARIES} ${Boost_SIGNALS_LIBRARIES})
target_link_libraries(libasyncio-static ${LIBI2NCOMMON_LIBRARIES} ${Boost_SIGNALS_LIBRARIES})

# Headerlist
foreach(header_file_in ${libasyncio_HEADERS})
    set(header_files_out "${header_files_out}${includedir}/${header_file_in}\n")
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/headerlist.asyncio ${header_files_out})

# Installation
install(TARGETS libasyncio LIBRARY DESTINATION lib)
install(TARGETS libasyncio-static ARCHIVE DESTINATION lib)
install(FILES ${libasyncio_HEADERS} DESTINATION include)
