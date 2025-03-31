# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_python_exports.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_python_exports.h")

# Specify the sources of the pure python and compiled portions of our module.
SET(conduit_py_python_sources  py_src/__init__.py
                               py_src/utils/__init__.py
                               # relay 
                               py_src/relay/__init__.py
                               py_src/relay/io/__init__.py
                               py_src/relay/io/blueprint/__init__.py
                               py_src/relay/mpi/__init__.py
                               py_src/relay/mpi/io/__init__.py
                               py_src/relay/mpi/io/blueprint/__init__.py
                               py_src/relay/web/__init__.py
                               # blueprint
                               py_src/blueprint/__init__.py
                               py_src/blueprint/mcarray/__init__.py
                               py_src/blueprint/mcarray/examples/__init__.py
                               py_src/blueprint/mesh/__init__.py
                               py_src/blueprint/mesh/examples/__init__.py
                               py_src/blueprint/table/__init__.py
                               py_src/blueprint/table/examples/__init__.py
                               py_src/blueprint/mpi/__init__.py
                               py_src/blueprint/mpi/mesh/__init__.py
                               py_src/blueprint/mpi/mesh/examples/__init__.py)



SET(conduit_py_headers         conduit_python.hpp
                               ${CMAKE_CURRENT_BINARY_DIR}/conduit_python_exports.h)

SET(conduit_py_cpp_sources     conduit_python.cpp)

# Make sure we have access to numpy
include_directories(${NUMPY_INCLUDE_DIRS})

# add conduit + conduit_python modules
PYTHON_ADD_HYBRID_MODULE(NAME          conduit_python
                         DEST_DIR      python-modules
                         PY_MODULE_DIR conduit
                         PY_SETUP_FILE setup.py
                         PY_SOURCES    ${conduit_py_python_sources}
                         SOURCES       ${conduit_py_headers}
                                       ${conduit_py_cpp_sources}
                         FOLDER        libs/python)

# link with the proper libs (beyond python)
target_link_libraries(conduit_python PRIVATE conduit conduit_python_build)


#############################################################
# conduit.utils
#############################################################

# add utils submodule
PYTHON_ADD_COMPILED_MODULE(NAME          conduit_utils_python
                           DEST_DIR      python-modules
                           PY_MODULE_DIR conduit/utils
                           SOURCES       conduit_utils_python.cpp
                                         ${CMAKE_CURRENT_BINARY_DIR}/conduit_python_exports.h
                           FOLDER        libs/python)

# compiled modules depend on output dir structure created by main module setup
target_link_libraries(conduit_utils_python PRIVATE conduit_python_py_setup)
                           
# link with the proper libs (beyond python)
target_link_libraries(conduit_utils_python PRIVATE conduit conduit_python_build)

# install the capi header so other python modules can use it
# support alt install dir for python module via PYTHON_MODULE_INSTALL_PREFIX
if(PYTHON_MODULE_INSTALL_PREFIX)
    install(FILES ${conduit_py_headers} DESTINATION ${PYTHON_MODULE_INSTALL_PREFIX}/conduit/)
else()
    install(FILES ${conduit_py_headers} DESTINATION python-modules/conduit/)
endif()


# create scripts that make it easy to launch the proper 
# python interp with the conduit module already in PYTHONPATH

set(_PYMOD_DIR ${PROJECT_BINARY_DIR}/python-modules)

# gen python helper to ${PROJECT_BINARY_DIR}/bin
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/run_python_with_conduit.sh.in"
                "${PROJECT_BINARY_DIR}/bin/run_python_with_conduit_build.sh" @ONLY)

if(PYTHON_MODULE_INSTALL_PREFIX)
    set(_PYMOD_DIR ${PYTHON_MODULE_INSTALL_PREFIX})
else()
    set(_PYMOD_DIR ${CMAKE_INSTALL_PREFIX}/python-modules/)
endif()


configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/run_python_with_conduit.sh.in"
                "${CMAKE_CURRENT_BINARY_DIR}/run_python_with_conduit_install.sh" @ONLY)

unset(_PYMOD_DIR)

# install the script that works for install paths
# 'PROGRAMS' makes sure exec perms are set
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/run_python_with_conduit_install.sh"
        DESTINATION bin
        RENAME run_python_with_conduit.sh)






