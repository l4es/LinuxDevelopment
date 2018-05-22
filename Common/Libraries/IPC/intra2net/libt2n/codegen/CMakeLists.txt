# Sources
set(CPP_SOURCES main.cpp)
set(CPP_HEADERS codegen-stubhead.hxx)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

# Executables
add_executable(libt2n-codegen ${CPP_SOURCES})

# Dependencies
target_link_libraries(libt2n-codegen ${XMLPP_LIBRARIES})

# Installation
install(TARGETS libt2n-codegen DESTINATION bin)
install(FILES ${CPP_HEADERS} DESTINATION include)
install(FILES clientlib.pc.in DESTINATION share/libt2n)
install(FILES Libt2n.cmake DESTINATION ${CMAKE_ROOT}/Modules)
