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

# # ADIOS2 provides a modern cmake interface.
# # However, conduit does everything quite explicitly, so we have to extract all the information manually.
# set(ADIOS2_FOUND TRUE)
# set(ADIOS2_INC 
# set(ADIOS2_LIB ${adios2::cxx11_mpi} ${MPI::MPI_CXX})
# 
# # Print out some results.
# MESSAGE(STATUS "  ADIOS2_INC=${ADIOS2_INC}")
# MESSAGE(STATUS "  ADIOS2_LIB=${ADIOS2_LIB}")

# if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
#     # if we are running tests with dlls, we need path to dlls
#     list(APPEND CONDUIT_TPL_DLL_PATHS ${ADIOS2_DIR}/bin)
# endif()



# make_minimum_required(VERSION 3.12)
# project(MySimulation C CXX)
# 
# find_package(MPI REQUIRED)
# find_package(ADIOS2 REQUIRED)
# #...
# add_library(my_library src1.cxx src2.cxx)
# target_link_libraries(my_library PRIVATE adios2::cxx11_mpi MPI::MPI_CXX)
