# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################
#
# Setup Zfp
# This file defines:
#  ZFP_FOUND - If Zfp was found
#  ZFP_INCLUDE_DIR - The Zfp include directories
#  ZFP_LIB - Zfp library


# first Check for ZFP_DIR

if(NOT ZFP_DIR)
    MESSAGE(FATAL_ERROR "Zfp support needs explicit ZFP_DIR")
endif()

find_path(ZFP_INCLUDE_DIR zfp.h
          PATHS ${ZFP_DIR}/include
          NO_DEFAULT_PATH
          NO_CMAKE_ENVIRONMENT_PATH
          NO_CMAKE_PATH
          NO_SYSTEM_ENVIRONMENT_PATH
          NO_CMAKE_SYSTEM_PATH)

find_library(ZFP_LIB NAMES zfp
             PATHS ${ZFP_DIR}/lib64 ${ZFP_DIR}/lib
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_CMAKE_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZFP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Zfp  DEFAULT_MSG
                                  ZFP_LIB ZFP_INCLUDE_DIR)

mark_as_advanced(ZFP_INCLUDE_DIR
                 ZFP_LIB)


blt_register_library(NAME zfp
                     INCLUDES ${ZFP_INCLUDE_DIR}
                     LIBRARIES ${ZFP_LIB} )

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${ZFP_DIR}/bin/)
endif()
