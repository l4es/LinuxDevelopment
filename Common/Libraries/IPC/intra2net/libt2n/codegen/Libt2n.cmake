# Version of this file: 1.3.1

# Detect libt2n
pkg_check_modules(LIBT2N REQUIRED libt2n)
include_directories(${LIBT2N_INCLUDE_DIRS})
link_directories(${LIBT2N_LIBRARY_DIRS})

# Detect the boost serialization library
find_package(Boost 1.34 COMPONENTS serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Get locations from pkgconfig
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable libt2n_codegen libt2n
                                OUTPUT_VARIABLE LIBT2N_CODEGEN
                                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable libt2n_datadir libt2n
                                OUTPUT_VARIABLE LIBT2N_DATADIR
                                OUTPUT_STRIP_TRAILING_WHITESPACE)
set(LIBT2N_CLIENT_PCTEMPLATE ${LIBT2N_DATADIR}/clientlib.pc.in)
set(LIBT2N_GCCXML /usr/bin/libt2n-gccxml.sh)

# Basic pkgconfig settings needed for client.pc generation
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})

# Setup libt2n. Arguments are the CMDGROUP names
function(setup_libt2n)
    string(LENGTH "${ARGV}" CMDGROUP_LEN)
    if (${CMDGROUP_LEN} EQUAL 0)
        message(FATAL_ERROR "libt2n CMDGROUP is empty")
    endif(${CMDGROUP_LEN} EQUAL 0)

    include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    # Compute include directories
    # TODO: http://www.cmake.org/Bug/view.php?id=11889 contains a better solution
    get_property(gcc_include_dirs
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY INCLUDE_DIRECTORIES
    )
    set(gccxml_include_dirs "")
    foreach(gcc_include_dir ${gcc_include_dirs})
        set(gccxml_include_dirs "${gccxml_include_dirs} -I${gcc_include_dir}")
    endforeach(gcc_include_dir ${gcc_include_dirs})

    foreach(CMDGROUP ${ARGV})
        message(STATUS "Processing libt2n command group ${CMDGROUP}")

        # We are going to run each .cpp file through gccxml for the current CMDGROUP
        set(T2N_GCCXML_FILES "")
        set(T2N_GCCXML_COMMANDS "")
        foreach(T2NFILE ${${CMDGROUP}_GROUP})
            get_filename_component(FILE_NAME ${T2NFILE} NAME)
            # get_filename_component(FILE_EXT ${T2NFILE} EXT)   <-- Doesn't work for filenames with multiple dots
            string(REGEX MATCH "(\\.[^\\.]*)$" FILE_EXT ${T2NFILE})
            if(NOT FILE_EXT STREQUAL ".cpp")
                message(FATAL_ERROR "   Error: ${FILE_NAME} is not a .cpp file based on its filename extension!")
            endif()
            message(STATUS "   Processing file ${FILE_NAME}")

            # We build the commands in advance which execute gccxml on each file in the CMDGROUP
            set(T2N_GCCXML_COMMANDS ${T2N_GCCXML_COMMANDS}
                COMMAND ${LIBT2N_GCCXML} ${gccxml_include_dirs} ${T2NFILE} -fxml=${T2NFILE}.xml
                )
            # The filenames of the created intermediate gccxml files for the current CMDGROUP
            set(T2N_GCCXML_FILES ${T2N_GCCXML_FILES} ${T2NFILE}.xml)
        endforeach(T2NFILE ${${CMDGROUP}_GROUP})

        add_custom_command(
            OUTPUT ${CMDGROUP}_common.cpp ${CMDGROUP}_common.hxx ${CMDGROUP}_client.cpp ${CMDGROUP}_client.hxx ${CMDGROUP}_server.cpp  ${CMDGROUP}_server.hxx

            # Create dummy _common.hxx file
            COMMAND echo "\\#include \\\"codegen-stubhead.hxx\\\"" >${CMDGROUP}_common.hxx
            COMMAND echo "\\#include \\\"${CMDGROUP}.hxx\\\"" >>${CMDGROUP}_common.hxx

            # Invoke gccxml on each source file in the current CMDGROUP and delete the dummy file
            ${T2N_GCCXML_COMMANDS}
            COMMAND rm -f ${CMDGROUP}_common.hxx

            # Run the code generator on all the generated gccxml files and remove those intermediate gccxml files
            COMMAND ${LIBT2N_CODEGEN} ${CMDGROUP} ${T2N_GCCXML_FILES}
            COMMAND rm -f ${T2N_GCCXML_FILES}

            DEPENDS ${${CMDGROUP}_GROUP}
            )

        # Write out pkgconfig file
        configure_file(${LIBT2N_CLIENT_PCTEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/${CMDGROUP}.pc @ONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMDGROUP}.pc
            DESTINATION lib/pkgconfig)

        # Create shared client library
        add_library(${CMDGROUP} SHARED ${CMDGROUP}_client.cpp ${CMDGROUP}_client.hxx)
        set_target_properties(${CMDGROUP} PROPERTIES VERSION ${VERSION} SOVERSION 1)
        target_link_libraries(${CMDGROUP} ${Boost_SERIALIZATION_LIBRARIES})
        install(TARGETS ${CMDGROUP} LIBRARY DESTINATION lib COMPONENT sharedlibs)

        # Create static client library
        add_library(${CMDGROUP}-static STATIC ${CMDGROUP}_client.cpp ${CMDGROUP}_client.hxx)
        set_target_properties(${CMDGROUP}-static PROPERTIES OUTPUT_NAME "${CMDGROUP}")
        target_link_libraries(${CMDGROUP}-static ${Boost_SERIALIZATION_LIBRARIES})
        install(TARGETS ${CMDGROUP}-static ARCHIVE DESTINATION lib COMPONENT staticlibs)

        # Prevent clobbering each other during the build
        set_target_properties(${CMDGROUP} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
        set_target_properties(${CMDGROUP}-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMDGROUP}_client.hxx ${CMDGROUP}.hxx
                    DESTINATION include/${PROJECT_NAME}
                    COMPONENT headers
        )

        # Wait till generated code is available
        add_custom_target(${CMDGROUP}_codegen_done ALL DEPENDS ${CMDGROUP}_common.cpp ${CMDGROUP}_common.hxx ${CMDGROUP}_client.cpp ${CMDGROUP}_client.hxx ${CMDGROUP}_server.cpp  ${CMDGROUP}_server.hxx)
        add_dependencies(${CMDGROUP} ${CMDGROUP}_codegen_done)
        add_dependencies(${CMDGROUP}-static ${CMDGROUP}_codegen_done)

    endforeach(CMDGROUP ${ARGV})
endfunction(setup_libt2n)
