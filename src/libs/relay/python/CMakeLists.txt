# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_relay_python_exports.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h")

# add relay submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/relay
                           SOURCES       conduit_relay_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_relay_python PRIVATE conduit_python_py_setup)

# link with the proper libs
target_link_libraries(conduit_relay_python PRIVATE conduit conduit_relay conduit_python_build)

# add relay io submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_io_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/relay/io
                           SOURCES       conduit_relay_io_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_relay_io_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_relay_io_python PRIVATE conduit conduit_relay conduit_python_build)

# add relay io blueprint submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_io_blueprint_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/relay/io/blueprint
                           SOURCES       conduit_relay_io_blueprint_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_relay_io_blueprint_python PRIVATE conduit_python_py_setup)

# link with the proper libs (beyond python)
target_link_libraries(conduit_relay_io_blueprint_python PRIVATE conduit conduit_relay conduit_blueprint conduit_python_build)

if(SILO_FOUND)

    # add relay io silo submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_io_silo_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/relay/io/silo
                               SOURCES       conduit_relay_io_silo_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_relay_io_silo_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_relay_io_silo_python PRIVATE conduit conduit_relay conduit_blueprint conduit_python_build)

endif()

if(ENABLE_RELAY_WEBSERVER)
    # add relay web submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_web_python
                               DEST_DIR      python-modules 
                               PY_MODULE_DIR conduit/relay/web
                               SOURCES       conduit_relay_web_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_relay_web_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_relay_web_python PRIVATE conduit conduit_relay conduit_python_build)
endif()

################################################################
# If we have mpi, add the conduit relay mpi module
################################################################

if(MPI_FOUND)

    # add relay mpi submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_mpi_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/relay/mpi
                               SOURCES       conduit_relay_mpi_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_relay_mpi_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_relay_mpi_python PRIVATE conduit conduit_relay_mpi conduit_python_build)

endif()

#############################################################
# relay.mpi.io
#############################################################

if(MPI_FOUND)

    # add io submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_mpi_io_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/relay/mpi/io
                               SOURCES       conduit_relay_mpi_io_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_relay_mpi_io_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_relay_mpi_io_python PRIVATE conduit conduit_relay_mpi conduit_relay_mpi_io conduit_python_build)


    # add io blueprint submodule
    PYTHON_ADD_COMPILED_MODULE(NAME          conduit_relay_mpi_io_blueprint_python
                               DEST_DIR      python-modules
                               PY_MODULE_DIR conduit/relay/mpi/io/blueprint
                               SOURCES       conduit_relay_mpi_io_blueprint_python.cpp
                                             ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_python_exports.h
                               FOLDER        libs/python)

    # compiled modules depend on output dir structure created by main module setup
    target_link_libraries(conduit_relay_mpi_io_blueprint_python PRIVATE conduit_python_py_setup)

    # link with the proper libs (beyond python)
    target_link_libraries(conduit_relay_mpi_io_blueprint_python PRIVATE conduit conduit_relay_mpi conduit_relay_mpi_io conduit_python_build)

endif()

