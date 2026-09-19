# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

cmake_minimum_required(VERSION 3.26)
include(CMakePackageConfigHelpers)

# Isolate the package configuration's policy behavior from optional dependencies.
set(_package_dir "${TEST_BINARY_DIR}/lib/cmake/conduit")
file(MAKE_DIRECTORY "${_package_dir}")
configure_package_config_file(
    "${CONDUIT_SOURCE_DIR}/config/ConduitConfig.cmake.in"
    "${_package_dir}/ConduitConfig.cmake"
    INSTALL_DESTINATION lib/cmake/conduit)
foreach(_dependency_file conduit_setup_deps.cmake conduit.cmake conduit_setup_targets.cmake)
    file(WRITE "${_package_dir}/${_dependency_file}" "")
endforeach()

set(CMAKE_WARN_DEPRECATED TRUE)
set(CMAKE_ERROR_DEPRECATED TRUE)
set(_minimum_before "${CMAKE_MINIMUM_REQUIRED_VERSION}")
set(_policies CMP0074 CMP0126 CMP0144 CMP0177)
foreach(_policy IN LISTS _policies)
    if(POLICY ${_policy})
        cmake_policy(SET ${_policy} NEW)
    endif()
endforeach()

foreach(_mode DEFAULT NO_POLICY_SCOPE)
    set(_scope_args)
    if(_mode STREQUAL "NO_POLICY_SCOPE")
        set(_scope_args NO_POLICY_SCOPE)
    endif()
    unset(Conduit_FOUND)
    unset(CONDUIT_FOUND)

    # Check both the first import and the already-found guard.
    foreach(_iteration RANGE 1 2)
        find_package(Conduit REQUIRED CONFIG ${_scope_args}
                     PATHS "${_package_dir}" NO_DEFAULT_PATH)
        if(NOT Conduit_FOUND OR NOT CONDUIT_FOUND)
            message(FATAL_ERROR "Conduit was not found in ${_mode} mode")
        endif()
        if(NOT CMAKE_MINIMUM_REQUIRED_VERSION STREQUAL _minimum_before)
            message(FATAL_ERROR "Conduit changed the caller's minimum CMake version")
        endif()
        foreach(_policy IN LISTS _policies)
            if(POLICY ${_policy})
                cmake_policy(GET ${_policy} _actual)
                if(NOT _actual STREQUAL "NEW")
                    message(FATAL_ERROR "Conduit changed caller policy ${_policy} in ${_mode} mode")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()
