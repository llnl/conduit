# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

###############################################################################
# Conduit C Interface Unit Tests
###############################################################################

set(C_TESTS
    t_c_conduit_smoke
    t_c_conduit_node
    t_c_conduit_node_set
    t_c_conduit_datatype
    t_c_conduit_schema)

################################
# Add our tests
################################
message(STATUS "Adding conduit lib c interface unit tests")
foreach(TEST ${C_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit
                 FOLDER tests/conduit/c)
endforeach()

