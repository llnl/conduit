# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Blueprint Unit Tests
################################

################################
# blueprint lib Unit Tests
################################
set(BLUEPRINT_TESTS t_blueprint_smoke
                    t_blueprint_mcarray_verify
                    t_blueprint_mcarray_examples
                    t_blueprint_o2mrelation_verify
                    t_blueprint_o2mrelation_examples
                    t_blueprint_ndarray_index
                    t_blueprint_mesh_verify
                    t_blueprint_mesh_convert
                    t_blueprint_mesh_transform
                    t_blueprint_mesh_partition
                    t_blueprint_mesh_query
                    t_blueprint_mesh_relay
                    t_blueprint_mesh_generate
                    t_blueprint_mesh_generate_map_fields
                    t_blueprint_mesh_examples
                    t_blueprint_mesh_examples_generate
                    t_blueprint_mesh_flatten
                    t_blueprint_mesh_matset_xforms
                    t_blueprint_mesh_topology_metadata
                    t_blueprint_mesh_utils
                    t_blueprint_table_verify
                    t_blueprint_table_examples
                    t_blueprint_table_relay
                    )


################################
# Add our Main Unit Tests
################################

message(STATUS "Adding blueprint lib unit tests")
foreach(TEST ${BLUEPRINT_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit conduit_blueprint conduit_relay
                 FOLDER tests/blueprint)
endforeach()

################################
# Add c interface tests
################################
add_subdirectory("c")

################################
# Add optional tests
################################
if(PYTHON_FOUND)
    add_subdirectory("python")
else()
    message(STATUS "Python disabled: Skipping conduit blueprint python module tests")
endif()


if(FORTRAN_FOUND)
    add_subdirectory("fortran")
else()
    message(STATUS "Fortran disabled: Skipping conduit blueprint fortran interface tests")
endif()

set(BLUEPRINT_MPI_TESTS_RANKS_2
    t_blueprint_mpi_smoke
    t_blueprint_mpi_mesh_query
    t_blueprint_mpi_mesh_transform
    t_blueprint_mpi_mesh_verify)


set(BLUEPRINT_RELAY_MPI_TESTS
    t_blueprint_mpi_mesh_relay
    t_blueprint_mpi_mesh_polytopal
    t_blueprint_mpi_mesh_distribute_2_ranks
    t_blueprint_mpi_mesh_utils)

set(BLUEPRINT_RELAY_PARMETIS_MPI_TESTS
    t_blueprint_mpi_mesh_parmetis
    t_blueprint_mpi_mesh_load_balance_related_boundary)

set(BLUEPRINT_RELAY_MPI_TESTS_RANKS_4
    t_blueprint_mpi_mesh_flatten
    t_blueprint_mpi_mesh_partition
    t_blueprint_mpi_mesh_distribute_4_ranks
    t_blueprint_mpi_mesh_tiled)

if(MPI_FOUND)
    message(STATUS "MPI enabled: Adding conduit_blueprint_mpi  and conduit_relay_mpi unit tests")
    foreach(TEST ${BLUEPRINT_MPI_TESTS_RANKS_2})
        add_cpp_mpi_test(TEST ${TEST} NUM_MPI_TASKS 2 DEPENDS_ON conduit
                                                                 conduit_blueprint_mpi)
    endforeach()

    message(STATUS "MPI enabled: Adding conduit_blueprint_mpi  and conduit_relay_mpi unit tests")
    foreach(TEST ${BLUEPRINT_RELAY_MPI_TESTS})
        add_cpp_mpi_test(TEST ${TEST} NUM_MPI_TASKS 2 DEPENDS_ON conduit
                                                                 conduit_blueprint
                                                                 conduit_blueprint_mpi
                                                                 conduit_relay
                                                                 conduit_relay_mpi
                                                                 conduit_relay_mpi_io)
    endforeach()

    if(PARMETIS_FOUND)
        message(STATUS "Parmetis enabled: Adding parmetis dependent unit tests")
        foreach(TEST ${BLUEPRINT_RELAY_PARMETIS_MPI_TESTS})
            add_cpp_mpi_test(TEST ${TEST}
                             NUM_MPI_TASKS 2
                             DEPENDS_ON conduit
                                        conduit_blueprint_mpi
                                        conduit_relay
                                        conduit_relay_mpi
                                        conduit_relay_mpi_io)
        endforeach()
    else()
        message(STATUS "Parmetis disabled: Skipping parmetis dependent unit tests")
    endif()

    foreach(TEST ${BLUEPRINT_RELAY_MPI_TESTS_RANKS_4})
        add_cpp_mpi_test(TEST ${TEST} NUM_MPI_TASKS 4 DEPENDS_ON conduit
                                                                 conduit_blueprint
                                                                 conduit_blueprint_mpi
                                                                 conduit_relay
                                                                 conduit_relay_mpi
                                                                 conduit_relay_mpi_io)
    endforeach()

else()
    message(STATUS "MPI disabled: Skipping conduit_blueprint_mpi tests")
endif()

message(STATUS "Adding blueprint lib Zfp unit tests")
add_cpp_test(TEST t_blueprint_zfp
             DEPENDS_ON conduit conduit_blueprint
             FOLDER tests/blueprint)

