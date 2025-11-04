# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
#
# Setup Silo
# This file defines:
#  SILO_FOUND - If Silo was found
#  SILO_INCLUDE_DIRS - The Silo include directories
#  SILO_LIBRARIES - The libraries needed to use Silo


# first Check for SILO_DIR, HDF5_DIR

if(NOT SILO_DIR)
    MESSAGE(FATAL_ERROR "Silo support needs explicit SILO_DIR")
endif()

if(NOT HDF5_DIR)
    MESSAGE(FATAL_ERROR "Silo support needs explicit HDF5_DIR")
endif()


find_path(SILO_INCLUDE_DIR silo.h
          PATHS ${SILO_DIR}/include
          NO_DEFAULT_PATH
          NO_CMAKE_ENVIRONMENT_PATH
          NO_CMAKE_PATH
          NO_SYSTEM_ENVIRONMENT_PATH
          NO_CMAKE_SYSTEM_PATH)
        

find_library(SILO_LIBS NAMES siloh5 silohdf5
             PATHS ${SILO_DIR}/lib
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_CMAKE_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

#
# Silo Depends on libjson, hdf5 and szip:
#             

#
# if json support is enabled, libjson is part of the silo build
#
find_library(SILO_JSON_LIBS NAMES json
             PATHS ${SILO_DIR}/json/lib/
             NO_DEFAULT_PATH
             NO_CMAKE_ENVIRONMENT_PATH
             NO_CMAKE_PATH
             NO_SYSTEM_ENVIRONMENT_PATH
             NO_CMAKE_SYSTEM_PATH)

set(SILO_LIBRARIES  ${SILO_LIBS} ${HDF5_LIBRARIES})

if(${SILO_JSON_LIBS})
    list(APPEND SILO_LIBRARIES ${SILO_JSON_LIBS})
endif()
set(SILO_INCLUDE_DIRS ${SILO_INCLUDE_DIR} )

#
# Add ZLIB to the SILO_LIBRARIES
#
if(ZLIB_FOUND)
    list(APPEND SILO_LIBRARIES ${ZLIB_LIBRARIES})
endif()


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SILO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Silo  DEFAULT_MSG
                                  SILO_LIBRARIES SILO_INCLUDE_DIRS)

mark_as_advanced(SILO_INCLUDE_DIR 
                 SILO_LIBS 
                 SILO_JSON_LIBS)


blt_register_library(NAME silo
                     INCLUDES ${SILO_INCLUDE_DIRS}
                     LIBRARIES ${SILO_LIBRARIES} )

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${SILO_DIR}/bin/)
endif()
