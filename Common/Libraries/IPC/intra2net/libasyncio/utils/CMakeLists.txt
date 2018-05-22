# Sources
add_subdirectory(i2ncommon)

set(libasyncio_utils_SOURCES
    asyncio_time_tools.cpp
    asyncio_utils.cpp
    asyncio_system_tools.cpp
    ${compat_SOURCES}
)
set(libasyncio_utils_HEADERS
    asyncio_ptr_list.hpp
    asyncio_utils.hpp
    asyncio_time_tools.hpp
    asyncio_pointer_func.hpp
    asyncio_system_tools.hpp
    asyncio_containerfunc.hpp
    asyncio_signalfunc.hpp
    ${compat_HEADERS}
)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/i2ncommon)

# Libraries
add_library(libasyncio_utils SHARED ${libasyncio_utils_SOURCES} ${libasyncio_utils_HEADERS})
set_target_properties(libasyncio_utils PROPERTIES VERSION ${VERSION} SOVERSION 0)
set_target_properties(libasyncio_utils PROPERTIES OUTPUT_NAME asyncio_utils)

# Headerlist
foreach(header_file_in ${libasyncio_utils_HEADERS})
    set(header_files_out "${header_files_out}${includedir}/${header_file_in}\n")
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/headerlist.utils ${header_files_out})

# Installation
install(TARGETS libasyncio_utils LIBRARY DESTINATION lib)
install(FILES ${libasyncio_utils_HEADERS} DESTINATION include)
