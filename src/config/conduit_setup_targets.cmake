# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.


# calc the proper relative install root
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)

if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")
endif()

# we want the import root, which is right above the "lib" prefix
get_filename_component(_IMPORT_ROOT "${_IMPORT_PREFIX}" PATH)


set(CONDUIT_INCLUDE_DIRS "${_IMPORT_ROOT}/include/conduit")

#
# Probe Conduit Features
#

# check for conduit fortran support
if(EXISTS ${_IMPORT_ROOT}/include/conduit/conduit.mod)
    set(CONDUIT_FORTRAN_ENABLED TRUE)
else()
    set(CONDUIT_FORTRAN_ENABLED FALSE)
endif()

if(EXISTS  ${_IMPORT_ROOT}/include/conduit/conduit_relay_mpi.hpp)
    set(CONDUIT_RELAY_MPI_ENABLED TRUE)
else()
    set(CONDUIT_RELAY_MPI_ENABLED FALSE)
endif()

if(EXISTS  ${_IMPORT_ROOT}/include/conduit/conduit_relay_io_hdf5_api.hpp)
    set(CONDUIT_RELAY_HDF5_ENABLED TRUE)
else()
    set(CONDUIT_RELAY_HDF5_ENABLED FALSE)
endif()

if(EXISTS  ${_IMPORT_ROOT}/include/conduit/conduit_relay_io_adios_api.hpp)
    set(CONDUIT_RELAY_ADIOS_ENABLED TRUE)
else()
    set(CONDUIT_RELAY_ADIOS_ENABLED FALSE)
endif()

if(EXISTS  ${_IMPORT_ROOT}/include/conduit/conduit_relay_io_silo_api.hpp)
    set(CONDUIT_RELAY_SILO_ENABLED TRUE)
else()
    set(CONDUIT_RELAY_SILO_ENABLED FALSE)
endif()

if(EXISTS  ${_IMPORT_ROOT}/include/conduit/conduit_relay_web.hpp)
    set(CONDUIT_RELAY_WEBSERVER_ENABLED TRUE)
else()
    set(CONDUIT_RELAY_WEBSERVER_ENABLED FALSE)
endif()


# create convenience target that bundles all reg conduit deps (conduit::conduit)

add_library(conduit::conduit INTERFACE IMPORTED)

set_property(TARGET conduit::conduit
             APPEND PROPERTY
             INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_ROOT}/include/")

set_property(TARGET conduit::conduit
             APPEND PROPERTY
             INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_ROOT}/include/conduit/")

set_property(TARGET conduit::conduit
             PROPERTY INTERFACE_LINK_LIBRARIES
             conduit conduit_relay conduit_blueprint)


if(CONDUIT_PYTHON_ENABLED)
    # create convenience target that exposes the header file for the
    # Python Capsule API for conduit
    add_library(conduit::conduit_python INTERFACE IMPORTED)

    # custom prefix allows the module to be installed outside of the
    # conduit install, when given we use that exact path
    if(CONDUIT_PYTHON_MODULE_CUSTOM_PREFIX)
        set_property(TARGET conduit::conduit_python
                     APPEND PROPERTY
                     INTERFACE_INCLUDE_DIRECTORIES "${CONDUIT_PYTHON_MODULE_DIR}/conduit/")
     else()
         # default case, find the python headers relative to the install root
         set_property(TARGET conduit::conduit_python
                      APPEND PROPERTY
                      INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_ROOT}/${CONDUIT_PYTHON_MODULE_DIR}/conduit/")
     endif()
endif()

# and if mpi enabled, a convenience target for remaining mpi deps (conduit::conduit_mpi)
if(CONDUIT_RELAY_MPI_ENABLED)
    add_library(conduit::conduit_mpi INTERFACE IMPORTED)
    set_property(TARGET conduit::conduit_mpi
                 PROPERTY INTERFACE_LINK_LIBRARIES
                 conduit::conduit
                 conduit_relay_mpi
                 conduit_relay_mpi_io
                 conduit_blueprint
                 conduit_blueprint_mpi)
endif()

message(STATUS "Found Conduit: ${_IMPORT_ROOT} (found version ${CONDUIT_VERSION})")

if(NOT Conduit_FIND_QUIETLY)
    message(STATUS "CONDUIT_VERSION             = ${CONDUIT_VERSION}")
    message(STATUS "CONDUIT_INSTALL_PREFIX      = ${CONDUIT_INSTALL_PREFIX}")
    message(STATUS "CONDUIT_IMPORT_ROOT         = ${_IMPORT_ROOT}")
    message(STATUS "CONDUIT_USE_CXX14           = ${CONDUIT_USE_CXX14}")
    message(STATUS "CONDUIT_USE_FMT             = ${CONDUIT_USE_FMT}")
    message(STATUS "CONDUIT_USE_CALIPER         = ${CONDUIT_USE_CALIPER}")
    message(STATUS "CONDUIT_USE_OPENMP          = ${CONDUIT_USE_OPENMP}")
    message(STATUS "CONDUIT_INCLUDE_DIRS        = ${CONDUIT_INCLUDE_DIRS}")
    message(STATUS "CONDUIT_FORTRAN_ENABLED     = ${CONDUIT_FORTRAN_ENABLED}")
    message(STATUS "CONDUIT_PYTHON_ENABLED      = ${CONDUIT_PYTHON_ENABLED}")
    message(STATUS "CONDUIT_PYTHON_EXECUTABLE   = ${CONDUIT_PYTHON_EXECUTABLE}")
    message(STATUS "CONDUIT_PYTHON_MODULE_DIR   = ${CONDUIT_PYTHON_MODULE_DIR}")
    message(STATUS "CONDUIT_PYTHON_MODULE_CUSTOM_PREFIX = ${CONDUIT_PYTHON_MODULE_CUSTOM_PREFIX}")
    message(STATUS "Conduit Relay features:")
    message(STATUS " CONDUIT_RELAY_WEBSERVER_ENABLED = ${CONDUIT_RELAY_WEBSERVER_ENABLED}")
    message(STATUS " CONDUIT_RELAY_HDF5_ENABLED      = ${CONDUIT_RELAY_HDF5_ENABLED}")
    message(STATUS " CONDUIT_HDF5_DIR                = ${CONDUIT_HDF5_DIR}")
    message(STATUS " CONDUIT_RELAY_ADIOS_ENABLED     = ${CONDUIT_RELAY_ADIOS_ENABLED}")
    message(STATUS " CONDUIT_ADIOS_DIR               = ${CONDUIT_ADIOS_DIR}")
    message(STATUS " CONDUIT_RELAY_SILO_ENABLED      = ${CONDUIT_RELAY_SILO_ENABLED}")
    message(STATUS " CONDUIT_SILO_DIR                = ${CONDUIT_SILO_DIR}")
    message(STATUS " CONDUIT_RELAY_MPI_ENABLED       = ${CONDUIT_RELAY_MPI_ENABLED}")
    message(STATUS " CONDUIT_USE_CMAKE_MPI_TARGETS   = ${CONDUIT_USE_CMAKE_MPI_TARGETS}")

    set(_print_targets "conduit::conduit")

    if(CONDUIT_PYTHON_ENABLED)
        set(_print_targets "${_print_targets} conduit::conduit_python")
    endif()

    if(CONDUIT_RELAY_MPI_ENABLED)
        set(_print_targets "${_print_targets} conduit::conduit_mpi")
    endif()

    message(STATUS "Conduit imported targets: ${_print_targets}")
    unset(_print_targets)
endif()


