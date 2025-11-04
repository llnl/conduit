# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
#
# Setup HDF5
#


# first Check for HDF5_DIR
if(NOT HDF5_DIR)
    MESSAGE(FATAL_ERROR "HDF5 support needs explicit HDF5_DIR")
endif()

# find the absolute path w/ symlinks resolved of the passed HDF5_DIR, 
# since sanity checks later need to compare against the real path
get_filename_component(HDF5_DIR_REAL "${HDF5_DIR}" REALPATH)
message(STATUS "Looking for HDF5 at: " ${HDF5_DIR_REAL})

# CMake's FindHDF5 module uses the HDF5_ROOT env var
set(HDF5_ROOT ${HDF5_DIR_REAL})

if(NOT WIN32)
    # use HDF5_ROOT env var for FindHDF5 with older versions of cmake
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
        set(ENV{HDF5_ROOT} ${HDF5_ROOT}/bin)
    endif()
endif()

# Use CMake's FindHDF5 module to locate hdf5 and setup hdf5
find_package(HDF5 REQUIRED)

# FindHDF5/find_package sets HDF5_DIR to its installed CMake info if it exists
# we want to keep HDF5_DIR as the root dir of the install to be 
# consistent with other packages

# find the absolute path w/ symlinks resolved of the passed HDF5_DIR, 
# since sanity checks later need to compare against the real path
get_filename_component(HDF5_DIR_REAL "${HDF5_ROOT}" REALPATH)

set(HDF5_DIR ${HDF5_DIR_REAL} CACHE PATH "" FORCE)
message(STATUS "HDF5_DIR_REAL=${HDF5_DIR_REAL}")
#
# Sanity check to alert us if some how we found an hdf5 instance
# in an unexpected location.  
#
message(STATUS "Checking that found HDF5_INCLUDE_DIRS are in HDF5_DIR")

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
        message(WARNING "FindHDF5 did not provide HDF5_INCLUDE_DIRS or HDF5_INCLUDE_DIR.")
    endif()
endif()

if(NOT HDF5_LIBRARIES)
    message(WARNING "FindHDF5 did not provide HDF5_LIBRARIES.")
endif()

message(STATUS "HDF5_INCLUDE_DIRS=${HDF5_INCLUDE_DIRS}")
set(check_hdf5_inc_dir_ok 0)
foreach(IDIR ${HDF5_INCLUDE_DIRS})

    # get real path of the include dir
    # w/ abs and symlinks resolved
    get_filename_component(IDIR_REAL "${IDIR}" REALPATH)
    # check if idir_real is a substring of hdf5_dir

    if("${IDIR_REAL}" MATCHES "${HDF5_DIR}")
        message(STATUS " ${IDIR_REAL} includes HDF5_DIR (${HDF5_DIR})")
        set(check_hdf5_inc_dir_ok 1)
    endif()

    if("${IDIR_REAL}" MATCHES "${HDF5_REAL_DIR}")
        message(STATUS " ${IDIR_REAL} includes HDF5_REAL_DIR (${HDF5_REAL_DIR})")
        set(check_hdf5_inc_dir_ok 1)
    endif()

endforeach()

if(NOT check_hdf5_inc_dir_ok)
    message(WARNING "HDF5_INCLUDE_DIRS (${HDF5_INCLUDE_DIRS}) does not include HDF5_DIR")
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
    message(STATUS "Removing hdf5_hl from HDF5_LIBRARIES")
    list(REMOVE_ITEM HDF5_LIBRARIES ${HDF5_HL_LIB})
endif()

##########################################################
# Use libhdf5.settings or h5cc to capture transitive hdf5
# deps (mainly zlib) for downstream static builds using 
# config.mk
##########################################################
message(STATUS "Attempting to find libhdf5.settings in HDF5_REAL_DIR...")
find_file(HDF5_SETTINGS_FILE
          NAMES libhdf5.settings
          PATHS ${HDF5_DIR}
          PATH_SUFFIXES lib share/cmake/hdf5
          NO_DEFAULT_PATH
          NO_CMAKE_ENVIRONMENT_PATH
          NO_CMAKE_PATH
          NO_SYSTEM_ENVIRONMENT_PATH
          NO_CMAKE_SYSTEM_PATH)

if(EXISTS ${HDF5_SETTINGS_FILE})
    message(STATUS "Found HDF5 settings file: ${HDF5_SETTINGS_FILE}")
    #Output by HDF5 CMake build system
    message(STATUS "Reading 'HDF5_SETTINGS_FILE' to determine hdf5 config settings")
    file(READ ${HDF5_SETTINGS_FILE} _HDF5_CC_CONFIG_VALUE)
else()
    message(STATUS "Unable to find libhdf5.settings, defaulting to h5cc output")
    #Run HDF5_C_COMPILER_EXECUTABLE -showconfig
    message(STATUS "Using 'h5cc -showconfig' to determine hdf5 config settings")
    execute_process(COMMAND "${HDF5_C_COMPILER_EXECUTABLE}" "-showconfig"
        RESULT_VARIABLE _HDF5_CC_CONFIG_SUCCESS
        OUTPUT_VARIABLE _HDF5_CC_CONFIG_VALUE
        ERROR_VARIABLE  _HDF5_CC_CONFIG_ERROR_VALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(_HDF5_CC_CONFIG_SUCCESS MATCHES 0)
        #h5cc ran ok
        message(STATUS "SUCCESS: h5cc -showconfig")
    else()
        # use warning b/c fatal error is to heavy handed
        message(WARNING "h5cc -showconfig failed, config.mk may not correclty report HDF5 details.")
    endif()
endif()


#######
# parse include flags (key = AM_CPPFLAGS)
#######
string(REGEX MATCHALL "AM_CPPFLAGS: .+\n" hdf5_tpl_inc_flags "${_HDF5_CC_CONFIG_VALUE}")
# strip prefix 
string(REGEX REPLACE  "AM_CPPFLAGS: " "" hdf5_tpl_inc_flags "${hdf5_tpl_inc_flags}")
# strip after
string(FIND  "${hdf5_tpl_inc_flags}" "\n" hdf5_tpl_inc_flags_end_pos)
string(SUBSTRING "${hdf5_tpl_inc_flags}" 0 "${hdf5_tpl_inc_flags_end_pos}" hdf5_tpl_inc_flags)
# only do final strip if not empty
if("${hdf5_tpl_inc_flags}")
    string(STRIP "${hdf5_tpl_inc_flags}" hdf5_tpl_inc_flags)
endif()
#######
# parse -L flags (key = AM_LDFLAGS)
#######
string(REGEX MATCHALL "AM_LDFLAGS: .+\n" hdf5_tpl_lnk_flags "${_HDF5_CC_CONFIG_VALUE}")
# strip prefix 
string(REGEX REPLACE  "AM_LDFLAGS: " "" hdf5_tpl_lnk_flags "${hdf5_tpl_lnk_flags}")
# strip after
string(FIND  "${hdf5_tpl_lnk_flags}" "\n" hdf5_tpl_lnk_flags_end_pos)
string(SUBSTRING "${hdf5_tpl_lnk_flags}" 0 "${hdf5_tpl_lnk_flags_end_pos}" hdf5_tpl_lnk_flags)
# only do final strip if not empty
if("${hdf5_tpl_lnk_flags}")
    string(STRIP "${hdf5_tpl_lnk_flags}" hdf5_tpl_lnk_flags)
endif()
#######
# parse -l flags (key = Extra libraries)
#######
string(REGEX MATCHALL "Extra libraries: .+\n" hdf5_tpl_lnk_libs "${_HDF5_CC_CONFIG_VALUE}")
# strip prefix 
string(REGEX REPLACE  "Extra libraries: " "" hdf5_tpl_lnk_libs "${hdf5_tpl_lnk_libs}")
# strip after
string(FIND  "${hdf5_tpl_lnk_libs}" "\n" hdf5_tpl_lnk_libs_end_pos)
string(SUBSTRING "${hdf5_tpl_lnk_libs}" 0 "${hdf5_tpl_lnk_libs_end_pos}" hdf5_tpl_lnk_libs)

# When hdf5 is built with cmake, the h5settings file may list
# these libs as a cmake list (separated by semi colons)
#
# when hdf5 is not build with cmake, these entries will be separated 
# by spaces and we want to remove leading whitespace,
# then split into a cmake list
#
# if hdf5_tpl_lnk_libs_len greater than 1 -- we have a semi colon case
list(LENGTH hdf5_tpl_lnk_libs hdf5_tpl_lnk_libs_len)

# only do final strip if not empty
if("${hdf5_tpl_lnk_libs}")
    # and not already a list
    if(hdf5_tpl_lnk_libs_len EQUAL 1)
        string(STRIP "${hdf5_tpl_lnk_libs}" hdf5_tpl_lnk_libs)
    endif()
endif()

set(hdf5_tpl_lnk_libs_list)
# get a cmake list of these libs if don't already have one by
# splitting the link libs string
if(hdf5_tpl_lnk_libs_len EQUAL 1)
    # ** Note **
    # The NATIVE_COMMAND arg to separate_arguments() was added in CMake 3.9
    # instead use strategy that allows older versions of CMake:
    #    an if to select WINDOWS_COMMAND or UNIX_COMMAND arg
    if(WIN32)
        separate_arguments(hdf5_tpl_lnk_libs_list WINDOWS_COMMAND "${hdf5_tpl_lnk_libs}")
    else()
        separate_arguments(hdf5_tpl_lnk_libs_list UNIX_COMMAND "${hdf5_tpl_lnk_libs}")
    endif()
else()
    set(hdf5_tpl_lnk_libs_list "${hdf5_tpl_lnk_libs}")
endif()

#
# Add ZLIB to the hdf5_tpl_lnk_libs_list, so it will appear in config.mk
#
if(ZLIB_FOUND)
    list(APPEND hdf5_tpl_lnk_libs_list ${ZLIB_LIBRARIES})
endif()


# add -l to any libraries that are just their names (like "m" instead of "-lm")
# this will get them into proper shape for the config.mk entry
set(_fixed_link_libs)
foreach(lib ${hdf5_tpl_lnk_libs_list})
    if(NOT "${lib}" MATCHES ^[-/])
        # lib doesn't start with '-' (-l) or '/' ()
        set(_fixed_link_libs "${_fixed_link_libs} -l${lib}")
    else()
        set(_fixed_link_libs "${_fixed_link_libs} ${lib}")
    endif()
endforeach()
set(hdf5_tpl_lnk_libs "${_fixed_link_libs}")


# append hdf5_tpl_lnk_libs to hdf5_tpl_lnk_flags
set(hdf5_tpl_lnk_flags "${hdf5_tpl_lnk_flags} ${hdf5_tpl_lnk_libs}")

#
# these will be used in Conduit's config.mk
#
set(CONDUIT_HDF5_TPL_INC_FLAGS ${hdf5_tpl_inc_flags})
set(CONDUIT_HDF5_TPL_LIB_FLAGS ${hdf5_tpl_lnk_flags})

######################################################
# Add any libs reported via hdf5_tpl_lnk_libs_list
# as deps we need to link to
######################################################
foreach(lib ${hdf5_tpl_lnk_libs_list})
    list(APPEND HDF5_LIBRARIES ${lib})
endforeach()


#
# Display main hdf5 cmake vars
#
message(STATUS "HDF5 Include Dirs: ${HDF5_INCLUDE_DIRS}")
message(STATUS "HDF5 Libraries:    ${HDF5_LIBRARIES}")
message(STATUS "HDF5 Definitions:  ${HDF5_DEFINITIONS}")
message(STATUS "HDF5 is parallel:  ${HDF5_IS_PARALLEL}")


# if hdf5 was built with mpi support, we have to propgate
# that regardless of conduit's use of MPI

if(HDF5_IS_PARALLEL)
    if(NOT MPI_FOUND)
         MESSAGE(FATAL_ERROR "Cannot link non-mpi aware conduit (MPI_FOUND == FALSE) "
                             "against HDF5 built with MPI support (HDF5_IS_PARALLEL == TRUE). "
                             "Please enable conduit's MPI support (ENABLE_MPI == ON, etc) or "
                             "build against a non-mpi HDF5 library.")
    endif()
endif()

#############
# Note: MPI libraries and include dirs are not propagated for the config.mk case
#############

message(STATUS "HDF5 Thirdparty Include Flags: ${hdf5_tpl_inc_flags}")
message(STATUS "HDF5 Thirdparty Link Flags: ${hdf5_tpl_lnk_flags}")

if(ZLIB_FOUND)
    set(hdf5_zlib_target ZLIB::ZLIB)
endif()

# if newer style hdf5 imported targets exist, use these
if(TARGET hdf5::hdf5-shared AND BUILD_SHARED_LIBS)
    # reg shared ver of imported lib target
    message(STATUS "HDF5 using hdf5::hdf5-shared target")
    blt_register_library(NAME hdf5
                         LIBRARIES hdf5::hdf5-shared ${hdf5_zlib_target})
elseif(TARGET hdf5::hdf5-static)
    # reg static ver of imported lib target
    message(STATUS "HDF5 using hdf5::hdf5-static target")
    blt_register_library(NAME hdf5
                         LIBRARIES hdf5::hdf5-static ${hdf5_zlib_target})
elseif(TARGET hdf5-shared AND BUILD_SHARED_LIBS)
    # reg shared ver of imported lib target
    message(STATUS "HDF5 using hdf5-shared target")
    blt_register_library(NAME hdf5
                         LIBRARIES hdf5-shared ${hdf5_zlib_target})
elseif(TARGET hdf5-static)
    # reg static ver of imported lib target
     message(STATUS "HDF5 using hdf5-static target")
     blt_register_library(NAME hdf5
                          LIBRARIES hdf5-static ${hdf5_zlib_target})
elseif(TARGET hdf5)
    # legacy hdf5 CMake build system support creates an hdf5 target we use directly
    message(STATUS "HDF5 using hdf5 target")
else()
    # reg includes and libs with blt
    message(STATUS "HDF5 using HDF5_DEFINITIONS + HDF5_INCLUDE_DIRS + HDF5_LIBRARIES")
    message(STATUS "HDF5_DEFINITIONS:  ${HDF5_DEFINITIONS}")
    message(STATUS "HDF5_INCLUDE_DIRS: ${HDF5_INCLUDE_DIRS}")
    message(STATUS "HDF5_LIBRARIES:    ${HDF5_LIBRARIES}")
    message(STATUS "HDF5_LINK_FLAGS:   ${hdf5_tpl_lnk_flags}")

    blt_register_library(NAME hdf5
                         DEFINES   ${HDF5_DEFINITIONS}
                         INCLUDES  ${HDF5_INCLUDE_DIRS}
                         LIBRARIES ${HDF5_LIBRARIES}
                         LINK_FLAGS ${hdf5_tpl_lnk_flags})
endif()

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    list(APPEND CONDUIT_TPL_DLL_PATHS ${HDF5_DIR}/bin/)
endif()
