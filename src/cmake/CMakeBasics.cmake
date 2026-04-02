# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Standard CMake Options
################################


# Fail if someone tries to config an in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "In-source builds are not supported. Please remove "
                       "CMakeCache.txt from the 'src' dir and configure an "
                       "out-of-source build in another directory.")
endif()

# enable creation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# always use position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "CMake build tool name: ${CMAKE_BUILD_TOOL}")

macro(ENABLE_WARNINGS)
    # set the warning levels we want to abide by
    if("${CMAKE_BUILD_TOOL}" MATCHES "(msdev|devenv|nmake|MSBuild)")
        add_definitions(/W4)
    else()
        if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR
            "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"   OR
            "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
            # use these flags for clang, gcc, or icc
            add_definitions(-Wall -Wextra)
        endif()
    endif()
endmacro()


################################
# Shared vs Static Libs
################################
if(BUILD_SHARED_LIBS)
    message(STATUS "Building shared libraries (BUILD_SHARED_LIBS == ON)")
else()
    message(STATUS "Building static libraries (BUILD_SHARED_LIBS == OFF)")
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
    set(CONDUIT_WINDOWS_DLL_EXPORTS TRUE)
endif()


################################
# Extra RPath Settings
################################
# BLT sets this when BUILD_SHARED_LIBS == TRUE
# We always want to apply rpath settings b/c even if we are building our own
# static libs, tpls may be shared libs
if(NOT BUILD_SHARED_LIBS)
    # use, i.e. don't skip the full RPATH for the build tree
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # the RPATH to be used when installing, but only if it's not a system directory
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

if( BLT_CXX_STD STREQUAL "c++98" )
    message(FATAL_ERROR "Conduit requires C++14 support."
                        "\nPlease set BLT_CXX_STD to c++14 or newer.")
elseif( BLT_CXX_STD STREQUAL "c++11" )
    message(FATAL_ERROR "Conduit requires C++14 support."
                        "\nPlease set BLT_CXX_STD to c++14 or newer.")
else()
    # keep CONDUIT_USE_CXX11 to support old install logic
    set(CONDUIT_USE_CXX11 TRUE)
    set(CONDUIT_USE_CXX14 TRUE)
    message(STATUS "C++14 support enabled (CONDUIT_USE_CXX14 == TRUE)")
endif()

if(ENABLE_OPENMP)
    set(CONDUIT_USE_OPENMP TRUE)
    message(STATUS "OpenMP support enabled (CONDUIT_USE_OPENMP == TRUE)")
endif()

################################
# Device Support
################################
set(CONDUIT_USE_CUDA FALSE)
set(CONDUIT_USE_CUDA FALSE)

if(ENABLE_CUDA)
    set(CONDUIT_USE_CUDA TRUE)
elseif( ENABLE_HIP)
    set(CONDUIT_USE_HIP TRUE)
endif()

################################
# Examples and Utils Flags
################################
if(ENABLE_EXAMPLES)
    message(STATUS "Building examples (ENABLE_EXAMPLES == ON)")
else()
    message(STATUS "Skipping examples (ENABLE_EXAMPLES == OFF)")
endif()

if(ENABLE_UTILS)
    message(STATUS "Building utilities (ENABLE_UTILS == ON)")
else()
    message(STATUS "Skipping utilities (ENABLE_UTILS == OFF)")
endif()

#######################################
# Relay Web Server Support
#######################################
if(ENABLE_RELAY_WEBSERVER)
    message(STATUS "Building Relay Web Server support (ENABLE_RELAY_WEBSERVER == ON)")
else()
    message(STATUS "Skipping Relay Web Server support (ENABLE_RELAY_WEBSERVER == OFF)")
endif()

#######################################
# Global Helpers (clear every config)
#######################################
set(CONDUIT_INSTALL_PREFIX CACHE STRING "" FORCE)
set(CONDUIT_MAKE_EXTRA_LIBS CACHE STRING "" FORCE)

################################
# Coverage Flags
################################
if(ENABLE_COVERAGE)
    message(STATUS "Building using coverage flags (ENABLE_COVERAGE == ON)")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} --coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
else()
    message(STATUS "Building without coverage flags (ENABLE_COVERAGE == OFF)")
endif()


################################
# Standard CTest Options
################################
if(ENABLE_TESTS)
    set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full --gen-suppressions=all")
    file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/cmake/valgrind.supp" MEMORYCHECK_SUPPRESSIONS_FILE)
    include(CTest)
    message(STATUS "Memcheck suppressions file: ${MEMORYCHECK_SUPPRESSIONS_FILE}")
endif()

##############################################################################
# Try to extract the current git sha
#
# This solution is derived from:
#  http://stackoverflow.com/a/21028226/203071
#
# This does not have full dependency tracking - it wont auto update when the
# git HEAD changes or when a branch is checked out, unless a change causes
# cmake to reconfigure.
#
# However, this limited approach will still be useful in many cases, 
# including building and for installing  conduit as a tpl
#
##############################################################################
find_package(Git)
if(GIT_FOUND)
    message(STATUS "git executable: ${GIT_EXECUTABLE}")
    # try to get sha1
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE CONDUIT_GIT_SHA1
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    if("${CONDUIT_GIT_SHA1}" STREQUAL "")
       set(CONDUIT_GIT_SHA1 "unknown")
    endif()
    execute_process(COMMAND
        "${GIT_EXECUTABLE}" diff --quiet HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_QUIET ERROR_QUIET)
    if (res)
       string(APPEND CONDUIT_GIT_SHA1 "-dirty")
    endif ()
    message(STATUS "git SHA1: " ${CONDUIT_GIT_SHA1})

    execute_process(COMMAND
        "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE CONDUIT_GIT_SHA1_ABBREV
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    if("${CONDUIT_GIT_SHA1_ABBREV}" STREQUAL "")
       set(CONDUIT_GIT_SHA1_ABBREV "unknown")
    endif()
    message(STATUS "git SHA1-abbrev: " ${CONDUIT_GIT_SHA1_ABBREV})

    # try to get tag
    execute_process(COMMAND
            "${GIT_EXECUTABLE}" describe --exact-match --tags
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE CONDUIT_GIT_TAG
            ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    if("${CONDUIT_GIT_TAG}" STREQUAL "")
       set(CONDUIT_GIT_TAG "unknown")
    endif()
    message(STATUS "git tag: " ${CONDUIT_GIT_TAG})
  
endif()


###############################################################################
# Provide macros to simplify creating libs
###############################################################################
macro(add_compiled_library)
    set(options OBJECT)
    set(singleValuedArgs NAME EXPORT HEADERS_DEST_DIR LIB_DEST_DIR FOLDER)
    set(multiValuedArgs  HEADERS SOURCES DEPENDS_ON)

    ## parse the arguments to the macro
    cmake_parse_arguments(args
         "${options}" "${singleValuedArgs}" "${multiValuedArgs}" ${ARGN} )

    #############################
    # add lib target
    #############################

    #
    # Note: headers are added here so they show up in project file generators
    # (such as xcode, eclipse, etc)
    #

    # if OBJECT is present, create an object library instead of a
    # standard library.

    if(${args_OBJECT})
        add_library(${args_NAME} OBJECT
                    ${args_SOURCES}
                    ${args_HEADERS})

    else()
        
        # general case build libs into lib dir
        set(lib_output_dir ${CMAKE_BINARY_DIR}/lib)

        # on windows we need the libs to live next to the tests
        if(WIN32)
            set(lib_output_dir ${CMAKE_BINARY_DIR}/bin)
        endif()
        
        blt_add_library( NAME        ${args_NAME}
                         SOURCES     ${args_SOURCES} 
                         HEADERS     ${args_HEADERS}
                         OUTPUT_DIR  ${lib_output_dir}
                         DEPENDS_ON  ${args_DEPENDS_ON})
    endif()

    #############################
    # install and export setup
    #############################
    # if export is set, install our lib

    # object libraries don't need to be installed
    if(NOT ${args_OBJECT})
        # if we have headers, install them
        if(NOT "${args_HEADERS}" STREQUAL "")
            if(NOT "${args_HEADERS_DEST_DIR}" STREQUAL "")
                install(FILES ${args_HEADERS}
                        DESTINATION ${args_HEADERS_DEST_DIR})
            else()
                install(FILES ${args_HEADERS}
                        DESTINATION include)
            endif()
        endif()
        # install our lib
        if(NOT "${args_LIB_DEST}" STREQUAL "")
            install(TARGETS ${args_NAME}
                    EXPORT ${args_EXPORT}
                    LIBRARY DESTINATION ${args_LIB_DEST_DIR}
                    ARCHIVE DESTINATION ${args_LIB_DEST_DIR}
                    RUNTIME DESTINATION ${args_LIB_DEST_DIR})
        else()
            install(TARGETS ${args_NAME}
                    EXPORT ${args_EXPORT}
                    LIBRARY DESTINATION lib
                    ARCHIVE DESTINATION lib
                    RUNTIME DESTINATION bin)
        endif()
    endif()

    # set folder if passed
    if(DEFINED args_FOLDER)
        blt_set_target_folder(TARGET  ${args_NAME} FOLDER ${args_FOLDER})
    endif()

endmacro()


###############################################################################
# This macro converts a cmake path to a platform specific string literal
# usable in C++. (For example, on windows C:/Path will be come C:\\Path)
###############################################################################

macro(convert_to_native_escaped_file_path path output)
    file(TO_NATIVE_PATH ${path} ${output})
    string(REPLACE "\\" "\\\\"  ${output} "${${output}}")
endmacro()


