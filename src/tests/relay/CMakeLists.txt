# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Conduit Unit Tests
################################

################################
# conduit relay Unit Tests
################################
set(RELAY_TESTS t_relay_smoke
                t_relay_io_smoke
                t_relay_io_basic
                t_relay_io_file_sizes
                t_relay_io_handle
                t_relay_io_handle_sidre
                t_conduit_intro_bp_and_relay)

################################
# Optional Unit Tests
################################
set(RELAY_WEBSERVER_TESTS  t_relay_node_viewer
                           t_relay_websocket)

set(RELAY_MPI_TESTS      t_relay_mpi_smoke
                         t_relay_mpi_test)
set(RELAY_SILO_TESTS     t_relay_io_silo)
set(RELAY_MPI_SILO_TESTS t_relay_mpi_io_silo)

set(RELAY_HDF5_TESTS t_relay_io_hdf5
                     t_relay_io_hdf5_read_and_print
                     t_relay_io_hdf5_slab
                     t_relay_io_hdf5_opts
                     t_relay_io_hdf5_diag_verbose
                     t_relay_io_hdf5_ident_report)


set(RELAY_MPI_IO_TESTS t_relay_io_mpi_smoke)
set(RELAY_ADIOS_TESTS t_relay_io_adios)
set(RELAY_MPI_ADIOS_TESTS t_relay_mpi_io_adios)
set(RELAY_ZFP_TESTS t_relay_zfp)

################################
# Add our main tests
################################

message(STATUS "Adding conduit_relay unit tests")
foreach(TEST ${RELAY_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit conduit_relay
                 FOLDER tests/relay)
endforeach()


################################
# Add c interface tests
################################
add_subdirectory("c")

################################
# Add our Optional Unit Tests
################################
if(ENABLE_RELAY_WEBSERVER)
    foreach(TEST ${RELAY_WEBSERVER_TESTS})
        add_cpp_test(TEST ${TEST}
                     DEPENDS_ON conduit conduit_relay
                     FOLDER tests/relay)
    endforeach()
else()
    message(STATUS "Relay Web Server disabled: Skipping conduit_relay web related tests")
endif()


if(HDF5_FOUND)
    foreach(TEST ${RELAY_HDF5_TESTS})
        add_cpp_test(TEST ${TEST}
                     DEPENDS_ON conduit conduit_relay
                     FOLDER tests/relay)
    endforeach()
else()
    message(STATUS "HDF5 disabled: Skipping conduit_relay hdf5 tests")
endif()

if(SILO_FOUND)
    message(STATUS "Silo enabled: Adding conduit_relay Silo unit tests")
    foreach(TEST ${RELAY_SILO_TESTS})
        add_cpp_test(TEST ${TEST}
                     DEPENDS_ON conduit conduit_relay
                     FOLDER tests/relay)
    endforeach()
    if(MPI_FOUND)
        message(STATUS "SILO + MPI enabled: Adding conduit_relay_mpi_silo unit tests")
        foreach(TEST ${RELAY_MPI_SILO_TESTS})
            add_cpp_mpi_test(TEST ${TEST}
                             NUM_MPI_TASKS 2
                             DEPENDS_ON conduit conduit_relay_mpi_io
                             FOLDER tests/relay)
        endforeach()
    endif()
else()
    message(STATUS "Silo disabled: Skipping conduit_relay silo tests")
endif()

if(ADIOS_FOUND)
    if(NOT MPI_FOUND)
        message(STATUS "ADIOS enabled: Adding conduit_relay ADIOS unit tests")
        foreach(TEST ${RELAY_ADIOS_TESTS})
            add_cpp_test(TEST ${TEST}
                         DEPENDS_ON conduit conduit_relay
                         FOLDER tests/relay)
        endforeach()
    else()
        message(STATUS "ADIOS + MPI enabled: Adding conduit_relay_mpi_adios unit tests")
        foreach(TEST ${RELAY_MPI_ADIOS_TESTS})
            add_cpp_mpi_test(TEST ${TEST}
                             NUM_MPI_TASKS 2
                             DEPENDS_ON conduit conduit_relay_mpi_io
                             FOLDER tests/relay)
        endforeach()
    endif()
else()
    message(STATUS "ADIOS disabled: Skipping conduit_relay ADIOS tests")
endif()


if(MPI_FOUND)
    message(STATUS "MPI enabled: Adding conduit_relay_mpi unit tests")
    foreach(TEST ${RELAY_MPI_TESTS})
        add_cpp_mpi_test(TEST ${TEST}
                         NUM_MPI_TASKS 2
                         DEPENDS_ON conduit conduit_relay_mpi
                         FOLDER tests/relay)
    endforeach()
    message(STATUS "MPI enabled: Adding conduit_relay_mpi_io unit tests")
    foreach(TEST ${RELAY_MPI_IO_TESTS})
        add_cpp_mpi_test(TEST ${TEST}
                         NUM_MPI_TASKS 2
                         DEPENDS_ON conduit conduit_relay_mpi_io
                         FOLDER tests/relay)
    endforeach()
    
else()
    message(STATUS "MPI disabled: Skipping conduit_relay_mpi tests")
endif()


if(ZFP_FOUND)
    message(STATUS "ZFP enabled: Adding conduit_relay_zfp unit tests")
    foreach(TEST ${RELAY_ZFP_TESTS})
        add_cpp_test(TEST ${TEST}
                     DEPENDS_ON conduit conduit_relay
                     FOLDER tests/relay)
    endforeach()
else()
    message(STATUS "ZFP disabled: Skipping conduit_relay_zfp tests")
endif()


################################
# Add optional tests
################################
if(PYTHON_FOUND)
    add_subdirectory("python")
else()
    message(STATUS "Python disabled: Skipping conduit relay python module tests")
endif()


if(FORTRAN_FOUND)
    add_subdirectory("fortran")
else()
    message(STATUS "Fortran disabled: Skipping conduit relay fortran interface tests")
endif()



