# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.
###############################################################################
#
# Example that shows how to use Conduit across C++, Fortran, and an
# embedded Python interpreter.
#
#
# Building:
#
# Note: The python instance must have the conduit python module installed
# or it must be in your PYTHONPATH.
#
# > mkdir build
# > cd build
# 
# # if conduit python module is not installed in your python instance
# # > export PYTHONPATH=/path/to/conduit-install/python-modules
#
# > cmake  \
#    -DCONDUIT_DIR=/path/to/conduit/install
#    -DPYTHON_EXECUTABLE=/path/to/python/bin/python
#    ../
# > make
#
#
# Running:
# > ./conduit_cpp_and_py_ex
# > ./conduit_fort_and_py_ex
#
# # if conduit python module is not installed in your python instance
# > env PYTHONPATH=/path/to/conduit-install/python-modules  ./conduit_cpp_and_py_ex
# > env PYTHONPATH=/path/to/conduit-install/python-modules  ./conduit_fort_and_py_ex
###############################################################################

cmake_minimum_required(VERSION 3.21)

project(conduit_cpp_fort_and_py C CXX Fortran)


######
# Setup Conduit
######
find_package(Conduit REQUIRED
             NO_DEFAULT_PATH 
             PATHS ${CONDUIT_DIR}/lib/cmake/conduit)

######
# Setup Python
######
include(SetupPython.cmake)

######
# Conduit requires c++14 support
######
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##############################################################################
# cpp to and from fortran example
##############################################################################
add_executable(conduit_cpp_and_py_ex
               conduit_cpp_and_py_ex.cpp
               python_interpreter.hpp
               python_interpreter.cpp)

target_link_libraries(conduit_cpp_and_py_ex
                      conduit::conduit
                      conduit::conduit_python)


# extra includes and libs to support the embedded python interpreter
target_include_directories(conduit_cpp_and_py_ex
                           PUBLIC ${PYTHON_INCLUDE_DIR})

target_link_libraries(conduit_cpp_and_py_ex
                      ${PYTHON_LIBRARY})

##############################################################################
# fortran to and from python example
##############################################################################

add_executable(conduit_fort_and_py_ex
               conduit_fort_and_py_ex.F90
               conduit_fort_and_py_mod.cpp
               conduit_fort_and_py_mod.F90
               python_interpreter.hpp
               python_interpreter.cpp)


target_link_libraries(conduit_fort_and_py_ex
                      conduit::conduit
                      conduit::conduit_python)

# extra includes and libs to support the embedded python interpreter
target_include_directories(conduit_fort_and_py_ex
                           PUBLIC ${PYTHON_INCLUDE_DIR})

target_link_libraries(conduit_fort_and_py_ex
                      ${PYTHON_LIBRARY})

