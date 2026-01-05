# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Conduit 3rd Party Dependencies
################################


################################
# BLT provides support for:
#  gtest, fruit, and mpi
################################

# clear DLL runtime path
set(CONDUIT_TPL_DLL_PATHS)

################################
# Threads support
################################
if(UNIX AND NOT APPLE)
    if(ENABLE_RELAY_WEBSERVER)
        # on some linux platforms we need to explicitly link threading
        # options.
        find_package( Threads REQUIRED )
    endif()
endif()

################################
# Setup and build json parsing
#   yyjson or rapidjson
################################
if(ENABLE_YYJSON)
    message(STATUS "Using yyjson for JSON parsing")
    add_subdirectory(thirdparty_builtin/yyjson)
    include_directories(thirdparty_builtin/yyjson/)
    set(CONDUIT_USE_YYJSON TRUE)
else() # rapidjson
    include(cmake/thirdparty/SetupRapidJSON.cmake)
    message(STATUS "Using RapidJSON Include: ${RAPIDJSON_INCLUDE_DIR}")
    include_directories(${RAPIDJSON_INCLUDE_DIR})
endif()

################################
# Setup and build libb64
################################
add_subdirectory(thirdparty_builtin/libb64-1.2.1/)
include_directories(thirdparty_builtin/libb64-1.2.1/include/)

################################
# Setup and build libyaml
################################
add_subdirectory(thirdparty_builtin/libyaml-690a781/)
include_directories(thirdparty_builtin/libyaml-690a781/include)


################################
# Setup includes for fmt
################################
# this cmake var used to signal fmt support for downstream users
set(CONDUIT_USE_FMT TRUE)

# setup include dirs used during conduit build
include_directories(thirdparty_builtin/fmt-7.1.0/)

# setup install of fmt headers for downstream users
install(DIRECTORY 
        thirdparty_builtin/fmt-7.1.0/conduit_fmt
        DESTINATION include/conduit/)

################################
# Optional Features
################################

################################
# Setup and build civetweb
################################
if(ENABLE_RELAY_WEBSERVER)
    add_subdirectory(thirdparty_builtin/civetweb-0a95342/)
    include_directories(thirdparty_builtin/civetweb-0a95342/include)
endif()

################################
# Setup ZLib if available
################################
if(ZLIB_DIR)
    include(cmake/thirdparty/SetupZlib.cmake)
endif()

if(ENABLE_PYTHON)
    ################################
    # Setup includes for Python & Numpy
    ################################
    include(cmake/thirdparty/SetupPython.cmake)
    message(STATUS "Using Python Include: ${PYTHON_INCLUDE_DIRS}")
    include_directories(${PYTHON_INCLUDE_DIRS})
    # if we don't find python, throw a fatal error
    if(NOT PYTHON_FOUND)
        message(FATAL_ERROR "ENABLE_PYTHON is true, but Python wasn't found.")
    endif()


    include(cmake/thirdparty/FindNumPy.cmake)
    message(STATUS "Using NumPy Include: ${NUMPY_INCLUDE_DIRS}")
    include_directories(${NUMPY_INCLUDE_DIRS})
    # if we don't find numpy, throw a fatal error
    if(NOT NUMPY_FOUND)
        message(FATAL_ERROR "ENABLE_PYTHON is true, but NumPy wasn't found.")
    endif()
endif()


################################
# Setup HDF5 if available
################################
# Search for HDF5.
if(HDF5_DIR)
    include(cmake/thirdparty/SetupHDF5.cmake)
    include_directories(${HDF5_INCLUDE_DIRS})
    # if we don't find HDF5, throw a fatal error
    if(NOT HDF5_FOUND)
        message(FATAL_ERROR "HDF5_DIR is set, but HDF5 wasn't found.")
    endif()
endif()

################################
# Setup Silo if available
################################
# Search for Silo.
if(SILO_DIR)
    include(cmake/thirdparty/SetupSilo.cmake)
    include_directories(${SILO_INCLUDE_DIRS})
    # if we don't find silo, throw a fatal error
    if(NOT SILO_FOUND)
        message(FATAL_ERROR "SILO_DIR is set, but Silo wasn't found.")
    endif()
endif()

################################
# Setup ADIOS if available
################################
# Search for ADIOS.
if(ADIOS_DIR)
    include(cmake/thirdparty/SetupADIOS.cmake)
    include_directories(${ADIOS_INCLUDE_DIRS})
    # if we don't find ADIOS, throw a fatal error
    if(NOT ADIOS_FOUND)
        message(FATAL_ERROR "ADIOS_DIR is set, but ADIOS wasn't found.")
    endif()
endif()

################################
# Setup Zfp if available
################################
# Search for Zfp.
if(ZFP_DIR)
    include(cmake/thirdparty/SetupZfp.cmake)
    include_directories(${ZFP_INCLUDE_DIR})
    # if we don't find Zfp, throw a fatal error
    if(NOT ZFP_FOUND)
        message(FATAL_ERROR "ZFP_DIR is set, but Zfp wasn't found.")
    endif()
endif()


################################
# Setup h5z-zfp if available
################################
if(H5ZZFP_DIR)
    include(cmake/thirdparty/SetupH5zZfp.cmake)
    include_directories(${H5ZZFP_INCLUDE_DIR})
    # if we don't find h5z-zfp, throw a fatal error
    if(NOT H5ZZFP_DIR)
        message(FATAL_ERROR "H5ZZFP_DIR is set, but h5z-zfp wasn't found.")
    endif()
endif()

################################
# Setup Parmetis if available
################################
if(PARMETIS_DIR)
    if(NOT ENABLE_MPI)
        message(FATAL_ERROR "PARMETIS_DIR is set while ENABLE_MPI is OFF. Parmetis support requires MPI.")
    endif()
    include(cmake/thirdparty/SetupParmetis.cmake)
    include_directories(${PARMETIS_INCLUDE_DIR})
    # if we don't find it, throw a fatal error
    if(NOT PARMETIS_FOUND)
        message(FATAL_ERROR "PARMETIS_DIR is set, but parmetis wasn't found.")
    endif()
endif()

################################
# Setup Caliper if available
################################
# Search for Caliper.
if(CALIPER_DIR)
    include(cmake/thirdparty/SetupCaliper.cmake)
    if(NOT CALIPER_FOUND)
        message(FATAL_ERROR "CALIPER_DIR is set, but Caliper wasn't found.")
    endif()
endif()


################################
# Camp
################################
if(CAMP_DIR) # optional for now
    include(cmake/thirdparty/SetupCamp.cmake)
    if(NOT CAMP_FOUND)
        message(FATAL_ERROR "CAMP_DIR is set, but CAMP wasn't found.")
    endif()
endif()

################################
# RAJA
################################
if(RAJA_DIR)  # optional for now
    include(cmake/thirdparty/SetupRAJA.cmake)
    if(NOT RAJA_FOUND)
        message(FATAL_ERROR "RAJA_DIR is set, but RAJA wasn't found.")
    endif()
endif()

################################
# Umpire
################################
if(UMPIRE_DIR) # optional for now
    include(cmake/thirdparty/SetupUmpire.cmake)
    if(NOT UMPIRE_FOUND)
        message(FATAL_ERROR "UMPIRE_DIR is set, but Umpire wasn't found.")
    endif()
endif()

################################
# Setup Totalview if available
################################
# Search for Totalview.
if(TOTALVIEW_DIR)
    include(cmake/thirdparty/SetupTotalview.cmake)
    if(NOT TOTALVIEW_FOUND)
        message(WARNING "TOTALVIEW_DIR is set, but Totalview wasn't found.")
    endif()
endif()
