# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Conduit Unit Tests
################################

# this header allows us to easily use the cmake source and binary paths in
# our unit tests

convert_to_native_escaped_file_path(${CMAKE_CURRENT_SOURCE_DIR}
                                    CONDUIT_T_SRC_DIR)

convert_to_native_escaped_file_path(${CMAKE_CURRENT_BINARY_DIR}
                                    CONDUIT_T_BIN_DIR)


configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/t_config.hpp.in"
                "${CMAKE_CURRENT_BINARY_DIR}/t_config.hpp")

include_directories(${CMAKE_CURRENT_BINARY_DIR})


if(MSVC AND MSVC_VERSION LESS 1800)
   add_definitions("/D_VARIADIC_MAX=10")
endif()

add_subdirectory("thirdparty")

add_subdirectory("conduit")
add_subdirectory("relay")
# allow blueprint tests to share test headers in relay tests
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/relay")
add_subdirectory("blueprint")
add_subdirectory("docs")


