# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

# Create an interface target to hold the common flags for python targets
add_library(conduit_python_build INTERFACE)
install(TARGETS conduit_python_build EXPORT "conduit")

if(Python3_FOUND)
  include(CMakeDependentOption)
  cmake_dependent_option(CONDUIT_PYTHON_USE_LIMITED_API "Build Python extensions using Limited API" OFF
    "Python3_VERSION VERSION_GREATER_EQUAL \"3.11\"" OFF)
endif()

################################
# Add the conduit lib
################################
add_subdirectory(conduit)

################################
# Add the conduit relay libs
################################
add_subdirectory(relay)

################################
# Add the blueprint lib 
################################
add_subdirectory(blueprint)

