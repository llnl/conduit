# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Add the conduit relay library
################################

ENABLE_WARNINGS()

####
# enable optional features if deps exists
####

if(HDF5_FOUND)
  SET(CONDUIT_RELAY_IO_HDF5_ENABLED TRUE)
endif()

if(H5ZZFP_FOUND)
  SET(CONDUIT_RELAY_IO_H5ZZFP_ENABLED TRUE)
endif()

if(SILO_FOUND)
  SET(CONDUIT_RELAY_IO_SILO_ENABLED TRUE)
endif()

if(ADIOS_FOUND AND NOT MPI_FOUND)
  SET(CONDUIT_RELAY_IO_ADIOS_ENABLED TRUE)
endif()

if(ADIOS_FOUND AND MPI_FOUND)
  SET(CONDUIT_RELAY_IO_MPI_ADIOS_ENABLED TRUE)
endif()

if(ZFP_FOUND)
  SET(CONDUIT_RELAY_ZFP_ENABLED TRUE)
endif()

if(MPI_FOUND)
  SET(CONDUIT_RELAY_MPI_ENABLED TRUE)
endif()

if(ENABLE_RELAY_WEBSERVER)
    SET(CONDUIT_RELAY_WEBSERVER_ENABLED TRUE)
endif()


convert_to_native_escaped_file_path(${CMAKE_CURRENT_SOURCE_DIR}
                                    CONDUIT_RELAY_SOURCE_DIR)


configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_relay_config.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_config.h")

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/conduit_relay_exports.h.in"
                "${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_exports.h")


#
# Specify cpp headers
#
set(conduit_relay_headers
    conduit_relay.hpp
    conduit_relay_io.hpp
    conduit_relay_io_handle.hpp
    conduit_relay_io_handle_api.hpp
    conduit_relay_io_handle_sidre.hpp
    conduit_relay_io_handle_sidre_api.hpp
    conduit_relay_io_identify_protocol.hpp
    conduit_relay_io_identify_protocol_api.hpp
    conduit_relay_io_blueprint.hpp
    conduit_relay_io_csv.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_exports.h
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_config.h)

#
# Specify c headers
#
set(conduit_relay_c_headers
    c/conduit_relay.h
    c/conduit_relay_io.h)

#
# Specify cpp sources
#
set(conduit_relay_sources
    conduit_relay.cpp
    conduit_relay_io.cpp
    conduit_relay_io_handle.cpp
    conduit_relay_io_handle_sidre.cpp
    conduit_relay_io_identify_protocol.cpp
    conduit_relay_io_blueprint.cpp
    conduit_relay_io_csv.cpp
)

#
# Specify c sources
#
set(conduit_relay_c_sources
    c/conduit_relay_c.cpp
    c/conduit_relay_io_c.cpp)

iF(ENABLE_RELAY_WEBSERVER)
    list(APPEND conduit_relay_headers
                conduit_relay_web.hpp
                conduit_relay_web_node_viewer_server.hpp
        )
    list(APPEND conduit_relay_sources
                conduit_relay_web.cpp
                conduit_relay_web_node_viewer_server.cpp)
endif()

if(HDF5_FOUND)
    list(APPEND conduit_relay_headers
         conduit_relay_hdf5.hpp 
         conduit_relay_io_hdf5.hpp
         conduit_relay_io_hdf5_api.hpp
        )
    list(APPEND conduit_relay_sources conduit_relay_io_hdf5.cpp)
endif()

if(SILO_FOUND)
    list(APPEND conduit_relay_headers 
         conduit_relay_silo.hpp 
         conduit_relay_io_silo.hpp
         conduit_relay_io_silo_api.hpp
        )
    list(APPEND conduit_relay_sources conduit_relay_io_silo.cpp)
endif()

if(ADIOS_FOUND AND NOT MPI_FOUND)
    list(APPEND conduit_relay_headers
         conduit_relay_io_adios.hpp
        )
    list(APPEND conduit_relay_sources conduit_relay_io_adios.cpp)
endif()

if(ZFP_FOUND)
    list(APPEND conduit_relay_headers conduit_relay_zfp.hpp)
    list(APPEND conduit_relay_sources conduit_relay_zfp.cpp)
endif()

################################
# Add fortran interface if 
# fortran support was selected
################################
#
# Specify fortran sources
#
set(conduit_relay_fortran_sources
    fortran/conduit_relay_fortran.f90)

#
# setup conduit relay lib
#


##################################
# system dependencies for civetweb
##################################


#
# Link with conduit
#
set(conduit_relay_deps conduit conduit_blueprint)

if(UNIX AND NOT APPLE)
    if(ENABLE_RELAY_WEBSERVER)
        # we need these for civetweb on linux, we may need similar libs
        # on windows (OSX appears ok without them)
        list(APPEND conduit_relay_deps dl rt Threads::Threads)
        set(CONDUIT_MAKE_EXTRA_LIBS "${CONDUIT_MAKE_EXTRA_LIBS} -ldl -lrt ${CMAKE_THREAD_LIBS_INIT}" CACHE STRING "" FORCE)
    endif()
endif()

# CONDUIT_MAKE_EXTRA_LIBS is basically used as an output var so
# we mark it as advanced b/c otherwise folks may see it in ccmake or
# the cmake gui  and think they should be setting it
mark_as_advanced(CONDUIT_MAKE_EXTRA_LIBS)

#
# Link with silo and hdf5 and adios if these are enabled
#
if(SILO_FOUND)
    list(APPEND conduit_relay_deps silo)
endif()

if(HDF5_FOUND)
    list(APPEND conduit_relay_deps hdf5)
    if(HDF5_IS_PARALLEL)
        list(APPEND conduit_relay_deps ${conduit_blt_mpi_deps})
    endif()
endif()

if(H5ZZFP_FOUND)
    list(APPEND conduit_relay_deps h5zzfp)
endif()

if(ADIOS_FOUND AND NOT MPI_FOUND)
    # Link with the serial ADIOS libraries.
    list(APPEND conduit_relay_deps adios_nompi)
endif()

if(ZFP_FOUND)
    list(APPEND conduit_relay_deps zfp)
endif()

if (MINGW)
  list(APPEND conduit_relay_deps ws2_32)
endif ()

add_compiled_library(NAME conduit_relay
                     EXPORT conduit
                     HEADERS ${conduit_relay_headers} ${conduit_relay_c_headers}
                     SOURCES ${conduit_relay_sources} ${conduit_relay_c_sources}
                             ${conduit_relay_fortran_sources}
                     DEPENDS_ON ${conduit_relay_deps}
                     HEADERS_DEST_DIR include/conduit
                     FOLDER libs)

target_include_directories(conduit_relay
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

# add civetweb if web server support is enabled
if(ENABLE_RELAY_WEBSERVER)
    target_sources(conduit_relay PRIVATE $<TARGET_OBJECTS:conduit_civetweb>)
endif()


if(ENABLE_UTILS)

    if(ENABLE_RELAY_WEBSERVER)
    ###################################
        # add conduit_relay_node_viewer exe
        ###################################
        blt_add_executable(
            NAME        conduit_relay_node_viewer
            SOURCES     conduit_relay_web_node_viewer_exe.cpp
            DEPENDS_ON  conduit_relay
            OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
            FOLDER utils)

        # add install target for conduit_relay_node_viewer
        install(TARGETS conduit_relay_node_viewer
                RUNTIME DESTINATION bin)
    endif()

    ###################################
    # add conduit_relay_io_ls exe
    ###################################
    blt_add_executable(
        NAME        conduit_relay_io_ls
        SOURCES     conduit_relay_io_ls_exe.cpp
        DEPENDS_ON  conduit_relay
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
        FOLDER utils)


    # add install target
    install(TARGETS conduit_relay_io_ls
            RUNTIME DESTINATION bin)

    ###################################
    # add conduit_relay_io_convert exe
    ###################################

    blt_add_executable(
        NAME        conduit_relay_io_convert
        SOURCES     conduit_relay_io_convert_exe.cpp
        DEPENDS_ON  conduit_relay
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
        FOLDER utils)

    # add install target for conduit_relay_io_convert
    install(TARGETS conduit_relay_io_convert
            RUNTIME DESTINATION bin)
endif()

if(ENABLE_RELAY_WEBSERVER)
    ##############################################################
    # install files needed for the relay web node viewer client
    #############################################################
    install(DIRECTORY web_clients/node_viewer
            DESTINATION share/conduit/web_clients/)

    #############################################################
    # add install of entangle helper script
    #############################################################
    install(FILES scripts/conduit_relay_entangle.py
            DESTINATION bin)
endif()

################################
# Add python wrappers if python
# support was selected
################################
if(PYTHON_FOUND)
    add_subdirectory(python)
endif()

################################################################
# If we have mpi, add the conduit relay mpi library
################################################################

if(MPI_FOUND)

#
# Specify relay mpi headers
#
set(conduit_relay_mpi_headers
    conduit_relay_mpi.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/conduit_relay_config.h)

#
# Specify relay mpi sources
#
set(conduit_relay_mpi_sources conduit_relay_mpi.cpp)

#
# Specify the relay mpi deps
#
set(conduit_relay_mpi_deps conduit
                           ${conduit_blt_mpi_deps})

#
# setup conduit relay mpi lib
#
add_compiled_library(NAME conduit_relay_mpi
                     EXPORT conduit
                     HEADERS ${conduit_relay_mpi_headers}
                     SOURCES ${conduit_relay_mpi_sources}
                     DEPENDS_ON ${conduit_relay_mpi_deps}
                     HEADERS_DEST_DIR include/conduit
                     FOLDER libs)

# avoid mpi C++ apis + link dep of libmpi_cxx
target_compile_definitions(conduit_relay_mpi PUBLIC ${CONDUIT_MPI_SKIP_MPICXX_DEFINES} )


################################################################
# If we have mpi, add the conduit relay mpi io library
#
# We put the io functions into a separate library so we do not
# disturb applications that may already be linking with
# relay+relay_mpi.
################################################################

#
# Specify relay mpi io sources
#
set(conduit_relay_mpi_io_sources conduit_relay_mpi_io.cpp
                                 conduit_relay_io_identify_protocol.cpp)

#
# Specify relay mpi io headers
#
set(conduit_relay_mpi_io_headers conduit_relay_mpi_io.hpp)

#
# Specify the relay mpi io deps
#
set(conduit_relay_mpi_io_deps conduit
                              ${conduit_blt_mpi_deps}
                              conduit_relay_mpi
                              conduit_relay
                              conduit_blueprint
                              conduit_blueprint_mpi)

#
# Specify relay mpi c headers
#
set(conduit_relay_mpi_io_c_headers
    c/conduit_relay_mpi_io.h)
#
# Specify relay mpi c sources
#
set(conduit_relay_mpi_io_c_sources
    c/conduit_relay_mpi_io_c.cpp)

list(APPEND conduit_relay_mpi_io_headers conduit_relay_mpi_io_blueprint.hpp)
list(APPEND conduit_relay_mpi_io_sources conduit_relay_io_blueprint.cpp)

if(SILO_FOUND)
    list(APPEND conduit_relay_mpi_io_headers conduit_relay_mpi_io_silo.hpp)
    list(APPEND conduit_relay_mpi_io_sources conduit_relay_io_silo.cpp)
    list(APPEND conduit_relay_mpi_io_deps silo)
endif()

if(HDF5_FOUND)
    list(APPEND conduit_relay_mpi_io_headers conduit_relay_mpi_io_hdf5.hpp)
    list(APPEND conduit_relay_mpi_io_sources conduit_relay_io_hdf5.cpp)
    list(APPEND conduit_relay_mpi_io_deps hdf5)
endif()

if(ADIOS_FOUND)
    list(APPEND conduit_relay_mpi_io_headers conduit_relay_mpi_io_adios.hpp)
    list(APPEND conduit_relay_mpi_io_sources conduit_relay_io_adios.cpp)
    # Link with parallel versions of ADIOS
    list(APPEND conduit_relay_mpi_io_deps adios_mpi)
endif()
 

#
# setup conduit relay mpi io lib
#
add_compiled_library(NAME conduit_relay_mpi_io
                     EXPORT conduit
                     HEADERS ${conduit_relay_mpi_io_headers} ${conduit_relay_mpi_io_c_headers}
                     SOURCES ${conduit_relay_mpi_io_sources} ${conduit_relay_mpi_io_c_sources}
                     DEPENDS_ON ${conduit_relay_mpi_io_deps}
                     HEADERS_DEST_DIR include/conduit
                     FOLDER libs)

blt_add_target_compile_flags(TO conduit_relay_mpi_io FLAGS "-DCONDUIT_RELAY_IO_MPI_ENABLED")

# avoid mpi C++ apis + link dep of libmpi_cxx
target_compile_definitions(conduit_relay_mpi_io PUBLIC ${CONDUIT_MPI_SKIP_MPICXX_DEFINES} )


endif() # end if MPI_FOUND
