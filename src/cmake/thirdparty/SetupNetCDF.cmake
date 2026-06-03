# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
#
# Setup NetCDF

# first Check for NETCDF_DIR
if(NOT NETCDF_DIR)
  message(FATAL_ERROR "NetCDF support needs explicit NETCDF_DIR")
endif()

message(STATUS "Looking for NetCDF in: ${NETCDF_DIR}")

# set(_RAJA_SEARCH_PATH)
# if(EXISTS ${RAJA_DIR}/share/raja/cmake)
#   # old install layout
#   set(_RAJA_SEARCH_PATH ${RAJA_DIR}/share/raja/cmake)
# elseif(EXISTS ${RAJA_DIR}/lib/cmake/raja)
#   # new install layout
#   set(_RAJA_SEARCH_PATH ${RAJA_DIR}/lib/cmake/raja)
# else ()
#   # try RAJA_DIR itself
#   set(_RAJA_SEARCH_PATH ${RAJA_DIR})
# endif()

find_package(netCDF REQUIRED
             NO_DEFAULT_PATH
             PATHS ${NETCDF_DIR})
message(STATUS "Found NetCDF in: ${NETCDF_DIR}")

set(NETCDF_FOUND TRUE)

# reset RAJA_DIR just in case the find process mangled it
# set(RAJA_DIR ${RAJA_DIR_ORIG})
if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${NETCDF_DIR}/lib/)
endif()
