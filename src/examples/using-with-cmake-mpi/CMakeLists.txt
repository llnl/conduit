# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################
#
# Example that shows how to use an installed instance of Conduit in another
# CMake-based build system.
#
# To build:
#  mkdir build
#  cd build
#  cmake -DCONDUIT_DIR={conduit install path} ../
#  make
#  ./conduit_example
#
#
# If run in sub directory of a conduit install, 
# CONDUIT_DIR will default to ../../..
# 
#  mkdir build
#  cd build
#  cmake ..
#  make
#  ./conduit_example
#
###############################################################################

cmake_minimum_required(VERSION 3.21)

project(using_with_cmake)


################################################################
# Option 1: import conduit using an explicit path (recommended)
################################################################

#
# Provide default CONDUIT_DIR that works in a conduit install
#
if(NOT CONDUIT_DIR)
    set(CONDUIT_DIR "../../..")
endif()

#
# Check for valid conduit install
#

# if given relative path, resolve
get_filename_component(CONDUIT_DIR ${CONDUIT_DIR} ABSOLUTE)
# check for expected cmake exported target files
if(NOT EXISTS ${CONDUIT_DIR}/lib/cmake/conduit/ConduitConfig.cmake)
    message(FATAL_ERROR "Could not find Conduit CMake include file (${CONDUIT_DIR}/lib/cmake/conduit/ConduitConfig.cmake)")
endif()

#
# Use CMake's find_package to import conduit's targets
# using explicit path
#
find_package(Conduit REQUIRED
             NO_DEFAULT_PATH 
             PATHS ${CONDUIT_DIR}/lib/cmake/conduit)

# make sure we have mpi, since we are going to use
# conduits mpi features
if(NOT MPI_FOUND)
    find_package(MPI COMPONENTS C)
endif()


################################################################
# Option 2: import conduit using find_package search
################################################################
##
## Add Conduit's install path to CMAKE_PREFIX_PATH 
## and use following find_package call to import conduit's targets
##
#
# find_package(Conduit REQUIRED)
#

######
# Conduit requires c++14 support
######
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#######################################################
# create our example 
#######################################################
add_executable(conduit_mpi_example conduit_mpi_example.cpp)

# link to conduit targets
target_link_libraries(conduit_mpi_example conduit::conduit
                                          conduit::conduit_mpi
                                          MPI::MPI_C)

# if you are using conduit's python CAPI capsule interface
# target_link_libraries(conduit_mpi_example conduit::conduit_python)


