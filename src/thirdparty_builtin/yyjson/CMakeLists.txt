# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Simple CMake setup for yyjson
################################

#
# yyjson sources
#

set(yyjson_sources
    conduit_yyjson.cpp
    )

#
# yyjson headers
#

set(yyjson_headers
    conduit_yyjson_interface.h
    conduit_yyjson.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_compiled_library(NAME conduit_yyjson
                     OBJECT
                     SOURCES ${yyjson_sources}
                     HEADERS ${yyjson_headers}
                     FOLDER thirdparty_builtin)

