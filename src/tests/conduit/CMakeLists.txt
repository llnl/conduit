# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Conduit Unit Tests
################################

################################
# Core (conduit lib) Unit Tests
################################
set(BASIC_TESTS t_conduit_smoke
                t_conduit_version
                t_conduit_endianness
                t_conduit_char8_str
                t_conduit_datatype_tests
                t_conduit_node
                t_conduit_node_parent
                t_conduit_node_compare
                t_conduit_node_static_init
                t_conduit_node_move_and_swap
                t_conduit_serialize
                t_conduit_array
                t_conduit_list_of
                t_conduit_node_binary_io
                t_conduit_node_save_load
                t_conduit_node_paths
                t_conduit_node_set
                t_conduit_node_to_value
                t_conduit_node_to_array
                t_conduit_to_string
                t_conduit_json
                t_conduit_json_sanitize
                t_conduit_yaml
                t_conduit_generator
                t_conduit_data_accessor
                t_conduit_node_update
                t_conduit_node_compact
                t_conduit_node_info
                t_conduit_node_iterator
                t_conduit_node_cpp_iterator
                t_conduit_node_obj_names_with_slashes
                t_conduit_node_type_dispatch
                t_conduit_schema
                t_conduit_error
                t_conduit_log
                t_conduit_utils
                t_conduit_annotations
                t_conduit_mem_allocator
                t_conduit_device
                t_conduit_intro_cpp_example
                t_conduit_fixed_size_map
                t_conduit_fixed_size_vector
                t_conduit_geometry_vector)


################################
# Add our tests
################################
message(STATUS "Adding conduit lib unit tests")
foreach(TEST ${BASIC_TESTS})
    add_cpp_test(TEST ${TEST}
                 DEPENDS_ON conduit
                 FOLDER tests/conduit)
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
    message(STATUS "Python disabled: Skipping conduit python module tests")
endif()

if(FORTRAN_FOUND)
    add_subdirectory("fortran")
else()
    message(STATUS "Fortran disabled: Skipping conduit fortran interface tests")
endif()


