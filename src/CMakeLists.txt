# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

# BLT >= 0.6.0  requires cmake >= 3.21
cmake_minimum_required(VERSION 3.21)
################################
# cmake policy selections
################################
# allow find_packages to use ZZZ_ROOT vars
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()
# use old cmake python find logic
if(POLICY CMP0148)
  cmake_policy(SET CMP0148 OLD)
endif()

################################
# Conduit
################################

project(conduit VERSION "0.9.5")

################################
# Build Options
################################
option(BUILD_SHARED_LIBS       "Build shared libraries"         ON)
option(ENABLE_TESTS            "Build conduit tests"            ON)
option(ENABLE_EXAMPLES         "Build Examples"                 ON)
option(ENABLE_UTILS            "Build Utilities"                ON)
option(ENABLE_DOCS             "Build conduit documentation"    ON)
option(ENABLE_RELAY_WEBSERVER  "Build Relay Web Server Support" ON)

option(ENABLE_COVERAGE    "Build with coverage flags"   OFF)

option(ENABLE_PYTHON      "Build Python Support"        OFF)
option(ENABLE_FORTRAN     "Build Fortran Support"       OFF)

option(ENABLE_MPI         "Build MPI Support"           OFF)
option(ENABLE_OPENMP      "Build OpenMP Support"        OFF)
option(ENABLE_CUDA        "Build CUDA Support"          OFF)
option(ENABLE_HIP         "Build HIP Support"           OFF)

option(ENABLE_YYJSON      "Use yyjson for JSON Parsing" ON)

# Add another option that provides extra
# control over conduit tests for cases where
# conduit is brought in as a submodule
option(CONDUIT_ENABLE_TESTS "Build conduit tests" ON)

######################
# CMake Policy Selections
######################

# allow FindPythonInterp and FindPythonLibs
# https://cmake.org/cmake/help/latest/policy/CMP0148.html
if(POLICY CMP0148)
    cmake_policy(SET CMP0148 OLD)
endif()

################################
# Invoke CMake Fortran setup
# if ENABLE_FORTRAN == ON
################################
if(ENABLE_FORTRAN)
    enable_language(Fortran)
endif()

# don't use BLT's all warnings feature
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "")

################################
# Init BLT
################################
# This also includes
# Conduit's BLT defaults
include(cmake/SetupBLT.cmake)

################################
# Set some standard cmake opts
################################
include(cmake/CMakeBasics.cmake)

################################
# Fortran Support
################################
include(cmake/SetupFortran.cmake)

################################
# Checks for type sizes, etc
################################
include(cmake/BasicTypeChecks.cmake)

################################
# Setup project wide includes
################################
include(cmake/SetupIncludes.cmake)

################################
# Setup 3rd Party Libs
################################
include(cmake/Setup3rdParty.cmake)

################################
# Setup tests helpers
################################
include(cmake/SetupTests.cmake)

################################
# Add our libs
################################
add_subdirectory(libs)

################################
# Add our examples
################################
add_subdirectory(examples)

################################
# Add our executables
################################
add_subdirectory(executables)

################################
# Add docs
################################
if(ENABLE_DOCS)
    add_subdirectory(docs)
else()
    message(STATUS "Skipping documentation targets (ENABLE_DOCS = OFF)")
endif()

################################
# Add our tests
################################
if(ENABLE_TESTS)
    if(CONDUIT_ENABLE_TESTS)
        add_subdirectory(tests)
    else()
        message(STATUS "Skipping test targets (CONDUIT_ENABLE_TESTS = OFF)")
    endif()
else()
    message(STATUS "Skipping test targets (ENABLE_TESTS = OFF)")
endif()

################################
# Add our config helpers
################################
add_subdirectory(config)

################################
# Create CMake importable
# exports for all of our targets
################################
# install exports to comply with standard find_package search path expectations
install(EXPORT conduit DESTINATION lib/cmake/conduit)


