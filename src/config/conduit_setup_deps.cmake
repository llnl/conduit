# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

include(CMakeFindDependencyMacro)

# calc the proper relative install root
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)

if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

# we want the import root, which is right above the "lib" prefix
get_filename_component(_IMPORT_ROOT "${_IMPORT_PREFIX}" PATH)

if(POLICY CMP0074)
    #policy for <PackageName>_ROOT variables
    cmake_policy(PUSH)
    cmake_policy(SET CMP0074 NEW)
endif()

###############################################################################
# Setup Threads
###############################################################################
if(UNIX AND NOT APPLE)
    # if built web server support, we depend on Threads::Threads
    # in our exported targets so we need to bootstrap that here
    if(EXISTS ${_IMPORT_ROOT}/include/conduit/conduit_relay_web.hpp)

        if(NOT TARGET Threads::Threads)
            find_package( Threads REQUIRED )
        endif()
    endif()
endif()

###############################################################################
# Setup OpenMP
###############################################################################
if(CONDUIT_USE_OPENMP)
    # config openmp if not already found
    if(NOT TARGET OpenMP::OpenMP_CXX)
        find_dependency(OpenMP REQUIRED)
    endif()
endif()

###############################################################################
# Setup Caliper
###############################################################################
if(NOT CALIPER_DIR)
    set(CALIPER_DIR ${CONDUIT_CALIPER_DIR})
endif()

if(CALIPER_DIR)
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Conduit was built with Caliper Support")
    endif()

    # use caliper config header to detect necessary deps
    find_file(CALI_CONFIG_HEADER
              NAMES caliper-config.h
              PATHS ${CALIPER_DIR}
              PATH_SUFFIXES include/caliper
              NO_DEFAULT_PATH
              NO_CMAKE_ENVIRONMENT_PATH
              NO_CMAKE_PATH
              NO_SYSTEM_ENVIRONMENT_PATH
              NO_CMAKE_SYSTEM_PATH)

    if(EXISTS ${CALI_CONFIG_HEADER})
        if(NOT Conduit_FIND_QUIETLY)
            message(STATUS "Found Caliper Config Header: ${CALI_CONFIG_HEADER}")
        endif()
    else()
        message(FATAL_ERROR "Could not find caliper-config.h in caliper ${CALIPER_DIR}/include/caliper")
    endif()

    file(READ ${CALI_CONFIG_HEADER} _CALI_CONFIG_HEADER_CONTENTS)

    # check if we need ADIAK
    string(FIND  ${_CALI_CONFIG_HEADER_CONTENTS} "#define CALIPER_HAVE_ADIAK" _caliper_have_adiak)

    if(${_caliper_have_adiak} GREATER_EQUAL 0 )
        # caliper is built with adiak support and caliper needs us to find adiak.
        if(NOT ADIAK_DIR)
            set(ADIAK_DIR ${CONDUIT_ADIAK_DIR})
        endif()

        if(ADIAK_DIR)
            if(NOT Conduit_FIND_QUIETLY)
                message(STATUS "Looking for Adiak at: ${ADIAK_DIR}/lib/cmake/adiak")
            endif()
            # find adiak first
            find_dependency(adiak REQUIRED
                            NO_DEFAULT_PATH
                            PATHS ${ADIAK_DIR}/lib/cmake/adiak)
        endif()
    endif()

    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Looking for Caliper at: ${CALIPER_DIR}/share/cmake/caliper")
    endif()
    # find caliper
    find_dependency(caliper REQUIRED
                    NO_DEFAULT_PATH
                    PATHS ${CALIPER_DIR}/share/cmake/caliper)
endif()

###############################################################################
# Setup Zlib
###############################################################################
if(CONDUIT_ZLIB_DIR)
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Conduit was built with Zlib Support")
    endif()

    if(NOT ZLIB_DIR)
        # if conduit was configured with ZLib, we need to include it
        set(ZLIB_DIR ${CONDUIT_ZLIB_DIR})
    endif()
    set(ZLIB_ROOT ${ZLIB_DIR})

    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Looking for Zlib at: " ${ZLIB_DIR})
    endif()

    find_package(ZLIB REQUIRED)
endif()

###############################################################################
# Setup HDF5
###############################################################################
if(CONDUIT_HDF5_DIR)
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Conduit was built with HDF5 Support")
    endif()
    # we depend on hdf5 in our exported targets
    # If ZZZ_DIR not set, use known install path for HDF5
    if(NOT HDF5_DIR)
        # if conduit was configured with hdf, we need to include it
        set(HDF5_DIR ${CONDUIT_HDF5_DIR})
    endif()

    # this logic mirrors Conduit's SetupHDF5.cmake

    # find the absolute path w/ symlinks resolved of the passed HDF5_DIR, 
    # since sanity checks later need to compare against the real path
    get_filename_component(HDF5_DIR_REAL "${HDF5_DIR}" REALPATH)
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Looking for HDF5 at: " ${HDF5_DIR_REAL})
    endif()

    # CMake's FindHDF5 module uses the HDF5_ROOT env var
    set(HDF5_ROOT ${HDF5_DIR_REAL})

    if(NOT WIN32)
        # use HDF5_ROOT env var for FindHDF5 with older versions of cmake
        if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
            set(ENV{HDF5_ROOT} ${HDF5_ROOT}/bin)
        endif()

        # Use CMake's FindHDF5 module to locate hdf5 and setup hdf5
        find_package(HDF5 REQUIRED)
    else()
        # CMake's FindHDF5 module is buggy on windows and will put the dll
        # in HDF5_LIBRARY.  Instead, use the 'CONFIG' signature of find_package
        # with appropriate hints for where cmake can find hdf5-config.cmake.
        find_package(HDF5 CONFIG
                     REQUIRED
                     HINTS ${HDF5_DIR}/cmake/hdf5 
                           ${HDF5_DIR}/lib/cmake/hdf5
                           ${HDF5_DIR}/share/cmake/hdf5
                           ${HDF5_DIR}/cmake)
    endif()

    # FindHDF5/find_package sets HDF5_DIR to it's installed CMake info if it exists
    # we want to keep HDF5_DIR as the root dir of the install to be 
    # consistent with other packages

    # find the absolute path w/ symlinks resolved of the passed HDF5_DIR, 
    # since sanity checks later need to compare against the real path
    get_filename_component(HDF5_DIR_REAL "${HDF5_ROOT}" REALPATH)

    set(HDF5_DIR ${HDF5_DIR_REAL} CACHE PATH "" FORCE)
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "HDF5_DIR_REAL=${HDF5_DIR_REAL}")
        #
        # Sanity check to alert us if some how we found an hdf5 instance
        # in an unexpected location.
        #
        message(STATUS "Checking that found HDF5_INCLUDE_DIRS are in HDF5_DIR")
    endif()

    #
    # HDF5_INCLUDE_DIRS may also include paths to external lib headers 
    # (such as szip), so we check that *at least one* of the includes
    # listed in HDF5_INCLUDE_DIRS exists in the HDF5_DIR specified.
    #

    # HDF5_INCLUDE_DIR is deprecated, but there are still some cases
    # where HDF5_INCLUDE_DIR is set, but HDF5_INCLUDE_DIRS is not
    if(NOT HDF5_INCLUDE_DIRS)
        if(HDF5_INCLUDE_DIR)
            set(HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
        else()
            if(NOT Conduit_FIND_QUIETLY)
                message(WARNING "FindHDF5 did not provide HDF5_INCLUDE_DIRS or HDF5_INCLUDE_DIR.")
            endif()
        endif()
    endif()

    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "HDF5_INCLUDE_DIRS=${HDF5_INCLUDE_DIRS}")
    endif()

    set(check_hdf5_inc_dir_ok 0)
    foreach(IDIR ${HDF5_INCLUDE_DIRS})

        # get real path of the include dir
        # w/ abs and symlinks resolved
        get_filename_component(IDIR_REAL "${IDIR}" REALPATH)
        # check if idir_real is a substring of hdf5_dir

        if("${IDIR_REAL}" MATCHES "${HDF5_DIR}")
            if(NOT Conduit_FIND_QUIETLY)
                message(STATUS " ${IDIR_REAL} includes HDF5_DIR (${HDF5_DIR})")
            endif()
            set(check_hdf5_inc_dir_ok 1)
        endif()

        if("${IDIR_REAL}" MATCHES "${HDF5_REAL_DIR}")
            if(NOT Conduit_FIND_QUIETLY)
                message(STATUS " ${IDIR_REAL} includes HDF5_REAL_DIR (${HDF5_REAL_DIR})")
            endif()
            set(check_hdf5_inc_dir_ok 1)
        endif()

    endforeach()

    if(NOT check_hdf5_inc_dir_ok)
        if(NOT Conduit_FIND_QUIETLY)
            message(WARNING "HDF5_INCLUDE_DIRS (${HDF5_INCLUDE_DIRS}) does not include HDF5_DIR")
        endif()
    endif()

    #
    # filter HDF5_LIBRARIES to remove hdf5_hl if it exists
    # we don't use hdf5_hl, but if we link with it will become
    # a transitive dependency
    #
    set(HDF5_HL_LIB FALSE)
    foreach(LIB ${HDF5_LIBRARIES})
        if("${LIB}" MATCHES "hdf5_hl")
            set(HDF5_HL_LIB ${LIB})
        endif()
    endforeach()

    if(HDF5_HL_LIB)
        if(NOT Conduit_FIND_QUIETLY)
            message(STATUS "Removing hdf5_hl from HDF5_LIBRARIES")
        endif()
        list(REMOVE_ITEM HDF5_LIBRARIES ${HDF5_HL_LIB})
    endif()

    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "HDF5 is parallel:  ${HDF5_IS_PARALLEL}")
    endif()
    # if HDF5 was built with parallel support, we need to find MPI
    # to make sure the targets propgate correctly.
    # in other cases, folks will 
    if(HDF5_IS_PARALLEL AND NOT MPI_FOUND)
        if(CONDUIT_USE_CMAKE_MPI_TARGETS)
            find_package(MPI COMPONENTS C)
        endif()
    endif()

    if(HDF5_IS_PARALLEL)
        if(NOT MPI_FOUND)
             MESSAGE(FATAL_ERROR "Cannot find MPI, but the HDF5 Conduit is linked to requires MPI support."
                                 " (HDF5_IS_PARALLEL == TRUE). ")
        endif()
    endif()

else()
    if(NOT Conduit_FIND_QUIETLY)
        message(STATUS "Conduit was NOT built with HDF5 Support")
    endif()
endif()

if(POLICY CMP0074)
    # clear CMP0074
    cmake_policy(POP)
endif()

