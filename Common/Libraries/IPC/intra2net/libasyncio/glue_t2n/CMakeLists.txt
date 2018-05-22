# Sources
set(libasyncio_t2n_SOURCES asyncio_t2n.cpp)
set(libasyncio_t2n_HEADERS asyncio_t2n.hpp)
include_directories(BEFORE
		    ${CMAKE_CURRENT_SOURCE_DIR}
		    ${CMAKE_SOURCE_DIR}/asyncio
		    ${CMAKE_SOURCE_DIR}/utils
		    )

# Libraries
add_library(libasyncio_t2n SHARED ${libasyncio_t2n_SOURCES} ${libasyncio_t2n_HEADERS})
set_target_properties(libasyncio_t2n PROPERTIES VERSION ${VERSION} SOVERSION 0)
set_target_properties(libasyncio_t2n PROPERTIES OUTPUT_NAME asyncio_t2n)
set_target_properties(libasyncio_t2n PROPERTIES CLEAN_DIRECT_OUTPUT 1)

add_library(libasyncio_t2n-static STATIC ${libasyncio_t2n_SOURCES} ${libasyncio_t2n_HEADERS})
set_target_properties(libasyncio_t2n-static PROPERTIES OUTPUT_NAME asyncio_t2n)
set_target_properties(libasyncio_t2n-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# Dependencies
target_link_libraries(libasyncio_t2n libasyncio ${LIBT2N_LIBRARIES} ${LIBI2NCOMMON_LIBRARIES} ${Boost_SIGNALS_LIBRARIES})
target_link_libraries(libasyncio_t2n-static libasyncio ${LIBT2N_LIBRARIES} ${LIBI2NCOMMON_LIBRARIES} ${Boost_SIGNALS_LIBRARIES})

# Installation
install(TARGETS libasyncio_t2n LIBRARY DESTINATION lib)
install(TARGETS libasyncio_t2n-static ARCHIVE DESTINATION lib)
install(FILES ${libasyncio_t2n_HEADERS} DESTINATION include)
