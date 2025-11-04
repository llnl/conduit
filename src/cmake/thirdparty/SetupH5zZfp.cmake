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

find_path(H5ZZFP_INCLUDE_DIR H5Zzfp.h
          PATHS ${H5ZZFP_DIR}/include
          NO_DEFAULT_PATH
          NO_CMAKE_ENVIRONMENT_PATH
          NO_CMAKE_PATH
          NO_SYSTEM_ENVIRONMENT_PATH
          NO_CMAKE_SYSTEM_PATH)
        

find_library(H5ZZFP_LIBRARIES NAMES libh5zzfp.a h5zzfp
             PATHS ${H5ZZFP_DIR}/lib
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_CMAKE_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SILO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(H5zZfp  DEFAULT_MSG
                                  H5ZZFP_LIBRARIES H5ZZFP_INCLUDE_DIR)


blt_register_library(NAME h5zzfp
                     INCLUDES ${H5ZZFP_INCLUDE_DIR}
                     LIBRARIES ${H5ZZFP_LIBRARIES})

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${H5ZZFP_DIR}/bin/)
endif()
