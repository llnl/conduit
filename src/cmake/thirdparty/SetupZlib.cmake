# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################
#

###############################################################################
#
# Setup ZLIB
#
###############################################################################


if(NOT ZLIB_DIR)
  message(FATAL_ERROR "Zlib support needs explicit ZLIB_DIR")
endif()

message(STATUS "Looking for ZLib in: ${ZLIB_DIR}")

if(EXISTS "${ZLIB_DIR}/include/zlib.h")
  message(STATUS "Found ${ZLIB_DIR}/include/zlib.h")
else()
  message(STATUS "Did not find ${ZLIB_DIR}/include/zlib.h")
endif()

if(EXISTS "${ZLIB_DIR}/lib")
  message(STATUS "Found ${ZLIB_DIR}/lib")
else()
  message(STATUS "Did not find ${ZLIB_DIR}/lib")
endif()

if(ZLIB_DIR)
    set(ZLIB_ROOT ${ZLIB_DIR})
    find_package(ZLIB REQUIRED)
endif()

message(STATUS "ZLIB_LIBRARIES: ${ZLIB_LIBRARIES}")

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${ZLIB_DIR}/bin/)
endif()
