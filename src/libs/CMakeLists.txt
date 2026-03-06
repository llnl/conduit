# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

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

