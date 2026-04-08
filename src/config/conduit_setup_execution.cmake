# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

function(_conduit_set_execution_language target language)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "conduit_enable_execution_target() target '${target}' does not exist")
    endif()

    get_target_property(_conduit_target_type ${target} TYPE)
    if(_conduit_target_type STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "conduit_enable_execution_target() requires a non-interface target")
    endif()

    if(language STREQUAL "CUDA")
        if(NOT CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
        endif()
        set(_conduit_exec_exts .c .cc .cp .cpp .cxx .c++)
    elseif(language STREQUAL "HIP")
        if(NOT CMAKE_HIP_COMPILER)
            enable_language(HIP)
        endif()
        set(_conduit_exec_exts .cc .cp .cpp .cxx .c++)
    else()
        message(FATAL_ERROR "Unsupported execution language '${language}'")
    endif()

    get_target_property(_conduit_target_sources ${target} SOURCES)
    if(_conduit_target_sources)
        foreach(_conduit_source IN LISTS _conduit_target_sources)
            if(_conduit_source MATCHES "^\\$<")
                continue()
            endif()

            get_filename_component(_conduit_ext "${_conduit_source}" LAST_EXT)
            string(TOLOWER "${_conduit_ext}" _conduit_ext)

            if(_conduit_ext IN_LIST _conduit_exec_exts)
                set_source_files_properties("${_conduit_source}"
                                            PROPERTIES LANGUAGE ${language})
            endif()
        endforeach()
    endif()

    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE ${language})
endfunction()


function(conduit_enable_execution_target)
    set(options)
    set(singleValueArgs TARGET SCOPE)
    cmake_parse_arguments(args "${options}" "${singleValueArgs}" "" ${ARGN})

    if(NOT args_TARGET)
        if(ARGC EQUAL 1)
            set(args_TARGET "${ARGV0}")
        else()
            message(FATAL_ERROR "conduit_enable_execution_target() requires TARGET <target> or a single target argument")
        endif()
    endif()

    if(NOT args_SCOPE)
        set(args_SCOPE PRIVATE)
    endif()

    string(TOUPPER "${args_SCOPE}" args_SCOPE)
    if(NOT args_SCOPE MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
        message(FATAL_ERROR "conduit_enable_execution_target() SCOPE must be PRIVATE, PUBLIC, or INTERFACE")
    endif()

    get_property(_conduit_exec_enabled TARGET ${args_TARGET}
                 PROPERTY CONDUIT_EXECUTION_TARGET_ENABLED SET)
    if(_conduit_exec_enabled)
        return()
    endif()

    if(TARGET conduit::conduit_execution)
        target_link_libraries(${args_TARGET} ${args_SCOPE} conduit::conduit_execution)
    elseif(TARGET conduit_execution)
        target_link_libraries(${args_TARGET} ${args_SCOPE} conduit_execution)
    elseif(TARGET conduit::conduit)
        target_link_libraries(${args_TARGET} ${args_SCOPE} conduit::conduit)
    elseif(TARGET conduit)
        target_link_libraries(${args_TARGET} ${args_SCOPE} conduit)
    else()
        message(FATAL_ERROR "conduit_enable_execution_target() could not find a Conduit target to link against")
    endif()

    if(CONDUIT_USE_CUDA)
        _conduit_set_execution_language(${args_TARGET} CUDA)
    elseif(CONDUIT_USE_HIP)
        _conduit_set_execution_language(${args_TARGET} HIP)
    endif()

    set_property(TARGET ${args_TARGET}
                 PROPERTY CONDUIT_EXECUTION_TARGET_ENABLED TRUE)
endfunction()
