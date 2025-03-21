# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
#
# Setup ADIOS2
#

MESSAGE(STATUS "Looking for ADIOS2 using ADIOS2_DIR=${ADIOS2_DIR}...")

IF(ENABLE_MPI)
    find_package(ADIOS2 REQUIRED COMPONENTS CXX MPI)
ELSE()
    find_package(ADIOS2 REQUIRED COMPONENTS CXX)
ENDIF()
