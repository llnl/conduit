# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

if(NOT UMPIRE_DIR)
  message(FATAL_ERROR "Umpire support needs explicit UMPIRE_DIR")
endif()

message(STATUS "Looking for Umpire in: ${UMPIRE_DIR}")

set(_UMPIRE_SEARCH_PATH)
if(EXISTS ${UMPIRE_DIR}/share/umpire/cmake)
  # old install layout
  set(_UMPIRE_SEARCH_PATH ${UMPIRE_DIR}/share/umpire/cmake)
elseif(EXISTS ${UMPIRE_DIR}/lib/cmake/umpire)
  # new install layout
  set(_UMPIRE_SEARCH_PATH ${UMPIRE_DIR}/lib/cmake/umpire)
elseif(EXISTS ${UMPIRE_DIR}/lib64/cmake/umpire)
  # new install layout
  set(_UMPIRE_SEARCH_PATH ${UMPIRE_DIR}/lib64/cmake/umpire)
endif()

set(UMPIRE_DIR_ORIG ${UMPIRE_DIR})
find_package(umpire REQUIRED
             NO_DEFAULT_PATH
             PATHS ${_UMPIRE_SEARCH_PATH})

message(STATUS "Found Umpire in: ${UMPIRE_DIR}")
# reset UMPIRE_DIR just in case the find process mangled it
set(UMPIRE_DIR ${UMPIRE_DIR_ORIG})
set(UMPIRE_FOUND TRUE)
set(CONDUIT_USE_UMPIRE TRUE)

if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # if we are running tests with dlls, we need path to dlls
    # hey, now we have to look at bin for the dlls :-(

    # we want the root of the umpire install so we can
    # locate the dlls
    set(_UMPIRE_DLL_DIR)
    if(EXISTS ${UMPIRE_DIR}/bin/)
        set(_UMPIRE_DLL_DIR ${UMPIRE_DIR}/bin/)
    elseif(EXISTS ${UMPIRE_DIR}/lib)
        set(_UMPIRE_DLL_DIR ${UMPIRE_DIR}/lib/)
    elseif(EXISTS ${UMPIRE_DIR}/lib64)
        # lib64 shouldn't happen on windows, but someone might
        # be clever and surprise us
        set(_UMPIRE_DLL_DIR ${UMPIRE_DIR}/lib64/)
    else()
      message(FATAL_ERROR "Failed to locate umpire dll dir ujnder intsall at ${UMPIRE_DIR}")
    endif()
    list(APPEND CONDUIT_TPL_DLL_PATHS ${_UMPIRE_DLL_DIR})
endif()
