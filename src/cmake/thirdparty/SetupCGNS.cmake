
# first Check for CGNS_DIR
if(NOT CGNS_DIR)
    MESSAGE(FATAL_ERROR "CGNS support needs explicit CGNS_DIR")
endif()

set(CGNS_ROOT ${CGNS_DIR})


find_package(CGNS REQUIRED)

if(TARGET CGNS::cgns_shared)
    get_target_property(_inc CGNS::cgns_shared INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(_lib CGNS::cgns_shared IMPORTED_LOCATION_RELEASE)
    get_target_property(_link CGNS::cgns_shared INTERFACE_LINK_LIBRARIES)
    message(STATUS "CGNS imported target INTERFACE_INCLUDE_DIRECTORIES: ${_inc}")
    message(STATUS "  INTERFACE_LINK_LIBRARIES: ${_link}")
    if(_lib)
        get_filename_component(_libdir "${_lib}" DIRECTORY)
        message(STATUS "CGNS imported library location: ${_lib}")
        message(STATUS "Guessed CGNS lib dir: ${_libdir}")
    endif()
    blt_register_library(NAME cgns
                         DEFINES "-DOMPI_SKIP_MPICXX"
                         INCLUDES  ${_inc}
                         LIBRARIES ${_link} ${_lib}
                         )
endif()

# Abort CMake here for debugging
# message(FATAL_ERROR "Aborting configuration: reached intentional stop in SetupCGNS.cmake")


    # if(TARGET CGNS::cgns_shared)



    #   message(STATUS "Found imported target CGNS::cgns_shared")
    #   get_target_property(_inc CGNS::cgns_shared INTERFACE_INCLUDE_DIRECTORIES)
    #   message(STATUS "  INTERFACE_INCLUDE_DIRECTORIES: ${_inc}")
    #   get_target_property(_link CGNS::cgns_shared INTERFACE_LINK_LIBRARIES)
    #   message(STATUS "  INTERFACE_LINK_LIBRARIES: ${_link}")

    #   message(STATUS "CGNS_ROOT/include: ${CGNS_ROOT}/include")
    #   message(STATUS "CGNS_ROOT/lib: ${CGNS_ROOT}/lib")

    # blt_register_library(NAME cgns
    #                     INCLUDES  ${_inc}
    #                     LIBRARIES ${_link}
    #                     )

    # endif()

# message(STATUS "CGNS_INCLUDE_DIRS: ${_inc}")
# message(STATUS "CGNS_LIBRARIES:    ${CGNS_LIBRARIES}")

