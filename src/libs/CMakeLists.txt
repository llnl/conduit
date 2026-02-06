# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

# Create an interface target to hold the common flags for python targets
add_library(conduit_python_build INTERFACE)
install(TARGETS conduit_python_build EXPORT "conduit")

if(Python3_FOUND)
  if(PYTHON_CONFIG_VERSION VERSION_GREATER_EQUAL "3.11")
    option(CONDUIT_PYTHON_USE_LIMITED_API "Build Python extensions using Limited API" OFF)
    if(CONDUIT_PYTHON_USE_LIMITED_API)
      set(CONDUIT_PYTHON_LIMITED_API 0x030B0000)
      target_compile_definitions(conduit_python_build INTERFACE Py_LIMITED_API=${CONDUIT_PYTHON_LIMITED_API})
      message(STATUS "Using Python's Stable ABI Version: ${CONDUIT_PYTHON_LIMITED_API}")
    endif()
  endif()
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

