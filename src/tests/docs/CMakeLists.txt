# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Conduit Unit Tests
################################

################################
# Docs Related Unit Tests
################################
set(DOCS_TESTS
    t_conduit_docs_tutorial_basics
    t_conduit_docs_tutorial_numeric
    t_conduit_docs_tutorial_parse
    t_conduit_docs_tutorial_ownership
    t_conduit_docs_tutorial_utils
    t_conduit_docs_tutorial_move_and_swap
    t_conduit_docs_tutorial_errors
    t_conduit_docs_relay_io_generic_examples
    t_conduit_docs_blueprint_examples
    t_conduit_docs_blueprint_demos)

set(DOCS_HDF5_TESTS
    t_conduit_docs_relay_io_handle_examples
    t_conduit_docs_relay_io_hdf5_examples)

set(DOCS_PYTHON_TESTS
    t_conduit_docs_tutorial_python_basics
    t_conduit_docs_tutorial_python_numeric
    t_conduit_docs_tutorial_python_parse
    t_conduit_docs_tutorial_python_ownership
    t_conduit_docs_tutorial_python_utils
    t_conduit_docs_tutorial_python_move_and_swap
    t_conduit_docs_tutorial_python_relay_io_handle_examples)

set(DOCS_PYTHON_MPI_TESTS  
    t_conduit_docs_tutorial_python_relay_mpi_examples)


################################
# Add our tests
################################
message(STATUS "Adding docs related unit tests")
foreach(TEST ${DOCS_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit conduit_blueprint conduit_relay
                 FOLDER tests/docs)
endforeach()

################################
# Add HDF5 Tests
################################
if(HDF5_FOUND)
    foreach(TEST ${DOCS_HDF5_TESTS})
        add_cpp_test(TEST ${TEST}
                     DEPENDS_ON conduit conduit_blueprint conduit_relay
                     FOLDER tests/docs)
    endforeach()
endif()

################################
# Add Python Tests
################################
if(PYTHON_FOUND)
    foreach(TEST ${DOCS_PYTHON_TESTS})
        add_python_test(TEST ${TEST}
                        FOLDER tests/docs)
    endforeach()

    if(MPI_FOUND)
        foreach(TEST ${DOCS_PYTHON_MPI_TESTS})
            add_python_mpi_test(TEST ${TEST}
                                NUM_MPI_TASKS 2)
                                # FOLDER tests/docs)
        endforeach()
    endif()

else()
    message(STATUS "Python disabled: Skipping conduit python tutorial examples")
endif()




