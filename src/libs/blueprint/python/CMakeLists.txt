# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_blueprint_python_exports.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h")


# add main conduit python source dir to allow us to use conduit_python_common.h,
# which has our standard python dev headers and macros
include_directories(${CMAKE_CURRENT_SOURCE_DIR}../../conduit/python)

# add blueprint submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint
                           SOURCES       conduit_blueprint_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_python PRIVATE conduit conduit_blueprint)

#############################################################
# blueprint.mcarray
#############################################################

# add mcarray submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mcarray_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/mcarray
                           SOURCES       conduit_blueprint_mcarray_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_mcarray_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_mcarray_python PRIVATE conduit conduit_blueprint)

# add mcarray examples submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mcarray_examples_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/mcarray/examples
                           SOURCES       conduit_blueprint_mcarray_examples_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_mcarray_examples_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_mcarray_examples_python PRIVATE conduit conduit_blueprint)

#############################################################
# blueprint.mesh
#############################################################

# add mesh submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mesh_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/mesh
                           SOURCES       conduit_blueprint_mesh_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_mesh_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_mesh_python PRIVATE conduit conduit_blueprint)


# add mesh examples submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mesh_examples_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/mesh/examples
                           SOURCES       conduit_blueprint_mesh_examples_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_mesh_examples_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_mesh_examples_python PRIVATE conduit conduit_blueprint)

#############################################################
# blueprint.table
#############################################################

# add table submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_table_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/table
                           SOURCES       conduit_blueprint_table_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_table_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_table_python PRIVATE conduit conduit_blueprint)


# add table examples submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_table_examples_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/blueprint/table/examples
                           SOURCES       conduit_blueprint_table_examples_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_blueprint_table_examples_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_blueprint_table_examples_python PRIVATE conduit conduit_blueprint)

################################################################
# If we have mpi, add the conduit blueprint mpi module
################################################################

if(MPI_FOUND)

    # add blueprint mpi submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mpi_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/blueprint/mpi
                               SOURCES       conduit_blueprint_mpi_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_blueprint_mpi_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_blueprint_mpi_python PRIVATE conduit conduit_blueprint_mpi)

endif()

#############################################################
# blueprint.mpi.mesh
#############################################################

if(MPI_FOUND)

    # add mesh submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mpi_mesh_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/blueprint/mpi/mesh
                               SOURCES       conduit_blueprint_mpi_mesh_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_blueprint_mpi_mesh_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_blueprint_mpi_mesh_python PRIVATE conduit conduit_blueprint_mpi)


    # add mesh examples submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_blueprint_mpi_mesh_examples_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/blueprint/mpi/mesh/examples
                               SOURCES       conduit_blueprint_mpi_mesh_examples_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_blueprint_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_blueprint_mpi_mesh_examples_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_blueprint_mpi_mesh_examples_python PRIVATE conduit conduit_blueprint_mpi)

endif()
