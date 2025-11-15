# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Add our library
################################

ENABLE_WARNINGS()

include("cmake/BitwidthMapping.cmake")


convert_to_native_escaped_file_path(${CMAKE_INSTALL_PREFIX}
                                    CONDUIT_INSTALL_PREFIX)

set(CONDUIT_INSTALL_PREFIX ${CONDUIT_INSTALL_PREFIX} CACHE STRING "" FORCE)
# CONDUIT_INSTALL_PREFIX is basically used as an output var so
# we mark it as advanced b/c otherwise folks may see it in ccmake or
# the cmake gui  and think they should be setting it
mark_as_advanced(CONDUIT_INSTALL_PREFIX)


if(FORTRAN_FOUND)
    set(CONDUIT_FORTRAN_COMPILER ${CMAKE_Fortran_COMPILER})
endif()

if(OPENMP_FOUND)
    set(CONDUIT_OPENMP_ENABLED TRUE)
endif()

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_config.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_config.h")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_bitwidth_style_types.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_bitwidth_style_types.h")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_exports.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_exports.h")

#
# Specify conduit cpp headers
#
set(conduit_headers
    conduit.hpp
    conduit_endianness_types.h
    conduit_core.hpp
    conduit_data_array.hpp
    conduit_data_accessor.hpp
    conduit_data_type.hpp
    conduit_endianness.hpp
    conduit_execution.hpp
    conduit_execution_omp.hpp
    conduit_execution_serial.hpp
    conduit_fixed_size_map.hpp
    conduit_fixed_size_vector.hpp
    conduit_geometry_vector.hpp
    conduit_node.hpp
    conduit_generator.hpp
    conduit_error.hpp
    conduit_node_iterator.hpp
    conduit_range_vector.hpp
    conduit_schema.hpp
    conduit_log.hpp
    conduit_utils.hpp
    conduit_vector_view.hpp
    conduit_annotations.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_exports.h
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_config.h
    conduit_config.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_bitwidth_style_types.h
    )

#
# Specify conduit c interface headers
#
set(conduit_c_headers
    c/conduit.h
    c/conduit_datatype.h
    c/conduit_node.h
    c/conduit_cpp_to_c.hpp
    c/conduit_utils.h
    )

#
# Specify conduit cpp sources
#
set(conduit_sources
    conduit_core.cpp
    conduit_error.cpp
    conduit_endianness.cpp
    conduit_data_type.cpp
    conduit_data_array.cpp
    conduit_data_accessor.cpp
    conduit_generator.cpp
    conduit_node.cpp
    conduit_node_iterator.cpp
    conduit_schema.cpp
    conduit_log.cpp
    conduit_utils.cpp
    conduit_annotations.cpp
    )

if(TOTALVIEW_FOUND)
  list(APPEND conduit_sources debug/conduit_node_totalview.cpp)
  if(NOT CONDUIT_EXCLUDE_TV_DATA_DISPLAY)
    list(APPEND conduit_sources ${TOTALVIEW_SOURCE_DIRECTORY}/tv_data_display.c)
  endif()
endif()

if (CMAKE_GENERATOR MATCHES "Visual Studio")
  list(APPEND conduit_sources ../../debug/msvs/ConduitNode.natvis)
endif()

#
# Specify conduit c interface sources
#
set(conduit_c_sources
    c/conduit_c.cpp
    c/conduit_datatype_c.cpp
    c/conduit_node_c.cpp
    c/conduit_cpp_to_c.cpp
    c/conduit_utils_c.cpp
    )



#################################
# Add fortran interface lib
# if fortran support was selected
#################################
if(FORTRAN_FOUND)
    # Create include file with bitwidth style mappings for fortran
    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/fortran/conduit_fortran_bitwidth_style_types.inc.in"
                    "conduit_fortran_bitwidth_style_types.inc")

    # Allow fortran compiler to include the Bitwidth_Style_Types.inc file
    # Note: Unlike .h files, the .inc file will not be needed by client code
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    #
    # Specify fortran sources
    #
    set(conduit_fortran_sources
        fortran/conduit_fortran.F90
        fortran/conduit_fortran_endianess_types.f90)

    #  add oo interface if the fortran compiler supports it
    if(ENABLE_FORTRAN_OBJ_INTERFACE)
        list(APPEND conduit_fortran_sources fortran/conduit_fortran_obj.f90)
    endif()

endif()

set(conduit_thirdparty_libs "")

if(CALIPER_FOUND)
    list(APPEND conduit_thirdparty_libs caliper)
endif()

if(OPENMP_FOUND)
    list(APPEND conduit_thirdparty_libs ${conduit_blt_openmp_deps})
endif()

if(ENABLE_YYJSON)
    list(APPEND conduit_sources $<TARGET_OBJECTS:conduit_yyjson>)
endif()

#
# Setup the conduit lib
#
add_compiled_library(NAME   conduit
                     EXPORT conduit
                     HEADERS ${conduit_headers} ${conduit_c_headers}
                     SOURCES ${conduit_sources} ${conduit_c_sources} ${conduit_fortran_sources}
                             $<TARGET_OBJECTS:conduit_b64>
                             $<TARGET_OBJECTS:conduit_libyaml>
                     HEADERS_DEST_DIR include/conduit
                     FOLDER libs
                     DEPENDS_ON ${conduit_thirdparty_libs})

target_include_directories(conduit
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                           $<$<BOOL:TOTALVIEW_FOUND>:${TOTALVIEW_INCLUDE_DIRECTORIES}>)


#################################
# Fortran related target options
#################################
if(FORTRAN_FOUND)
    if("${CMAKE_Fortran_COMPILER_ID}" STREQUAL "GNU")
         # disable c <-> fortran binding warnings for gfortran
         message(STATUS "Disabling c <-> fortran binding warnings for "
                         "conduit "
                         "(Adding Fortran compiler flag: -Wno-c-binding-type)")

        set_source_files_properties(${conduit_fortran_sources}
                                    PROPERTIES COMPILE_FLAGS "-Wno-c-binding-type")
    endif()
endif()

################################
# Add python wrappers if python
# support was selected
################################
if(PYTHON_FOUND)
    add_subdirectory(python)
endif()


####################################################################
# Install target for conduit fortran modules
####################################################################
if(FORTRAN_FOUND)
   install(DIRECTORY
   ${CMAKE_Fortran_MODULE_DIRECTORY}/
   DESTINATION include/conduit)
endif()

