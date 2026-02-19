# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
#
# Setup h5z-zfp
# This file defines:
#  H5ZZFP_FOUND - If Silo was found
#  H5ZZFP_INCLUDE_DIRS - The h5z-zfp include directories
#  H5ZZFP_LIBRARIES - The libraries needed to use h5z-zfp


# first Check for H5ZZFP_DIR, HDF5_DIR, and ZFP_DIR

if(NOT H5ZZFP_DIR)
    MESSAGE(FATAL_ERROR "h5z-zfp support needs explicit H5ZZFP_DIR")
endif()

if(NOT HDF5_DIR)
    MESSAGE(FATAL_ERROR "h5z-zfp support needs explicit HDF5_DIR")
endif()

if(NOT ZFP_DIR)
    MESSAGE(FATAL_ERROR "h5z-zfp support needs explicit ZFP_DIR")
endif()

message(STATUS "Looking for h5z-zfp in: ${H5ZZFP_DIR}")

set(_H5ZZFP_SEARCH_PATH ${H5ZZFP_DIR}/lib/cmake/h5z_zfp/)
find_package(h5z_zfp REQUIRED
             NO_DEFAULT_PATH
             PATHS ${_H5ZZFP_SEARCH_PATH})

blt_register_library(NAME h5zzfp
                     LIBRARIES h5z_zfp::h5z_zfp)

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${H5ZZFP_DIR}/lib/)
endif()
