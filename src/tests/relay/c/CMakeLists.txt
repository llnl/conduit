# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

###############################################################################
# Conduit Relay C Interface Unit Tests
###############################################################################

set(RELAY_C_TESTS
     t_c_relay_smoke)

set(RELAY_C_IO_ADIOS_TESTS t_c_relay_io_adios)
set(RELAY_C_MPI_IO_ADIOS_TESTS t_c_relay_mpi_io_adios)

################################
# Add our tests
################################
message(STATUS "Adding conduit relay lib c interface unit tests")
foreach(TEST ${RELAY_C_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit conduit_relay
                 FOLDER tests/relay/c)
endforeach()


if(ADIOS_FOUND)
    if(NOT MPI_FOUND)
        message(STATUS "ADIOS enabled: Adding conduit_relay c interface ADIOS unit tests")
        foreach(TEST ${RELAY_C_IO_ADIOS_TESTS})
            add_cpp_test(TEST ${TEST}
                         DEPENDS_ON conduit conduit_relay
                         FOLDER tests/relay/c)
        endforeach()
    else()
        message(STATUS "ADIOS + MPI enabled: Adding conduit_relay_mpi_adios unit tests")
        foreach(TEST ${RELAY_C_MPI_IO_ADIOS_TESTS})
            add_cpp_mpi_test(TEST ${TEST}
                             NUM_MPI_TASKS 2
                             DEPENDS_ON conduit conduit_relay_mpi_io
                             FOLDER tests/relay/c)
        endforeach()
    endif()
else()
    message(STATUS "ADIOS disabled: Skipping conduit_relay c interface ADIOS tests")
endif()


if(ADIOS2_FOUND)
    if(NOT MPI_FOUND)
        message(STATUS "ADIOS2 enabled: Adding conduit_relay c interface ADIOS2 unit tests")
        foreach(TEST ${RELAY_C_IO_ADIOS2_TESTS})
            add_cpp_test(TEST ${TEST}
                         DEPENDS_ON conduit conduit_relay
                         FOLDER tests/relay/c)
        endforeach()
    else()
        message(STATUS "ADIOS2 + MPI enabled: Adding conduit_relay_mpi_adios2 unit tests")
        foreach(TEST ${RELAY_C_MPI_IO_ADIOS2_TESTS})
            add_cpp_mpi_test(TEST ${TEST}
                             NUM_MPI_TASKS 2
                             DEPENDS_ON conduit conduit_relay_mpi_io
                             FOLDER tests/relay/c)
        endforeach()
    endif()
else()
    message(STATUS "ADIOS2 disabled: Skipping conduit_relay c interface ADIOS2 tests")
endif()
