# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################
# Unit Tests for 3rd Party Libs
################################

message(STATUS "Adding thirdparty lib unit tests")

set(smoke_device_deps)

if(CUDA_FOUND)
    list(APPEND smoke_device_deps cuda)
endif()

if(HIP_FOUND)
    list(APPEND smoke_device_deps blt::hip)
endif()

if(ENABLE_YYJSON)
    add_cpp_test(TEST t_yyjson_smoke
                 SOURCES $<TARGET_OBJECTS:conduit_yyjson>
                 FOLDER tests/thirdparty)
else()
    add_cpp_test(TEST t_rapidjson_smoke
                 FOLDER tests/thirdparty)
endif()


add_cpp_test(TEST t_libb64_smoke
                 SOURCES $<TARGET_OBJECTS:conduit_b64>
                 FOLDER tests/thirdparty)


add_cpp_test(TEST t_libyaml_smoke
            SOURCES $<TARGET_OBJECTS:conduit_libyaml>
            FOLDER tests/thirdparty)

add_cpp_test(TEST t_fmt_smoke
             FOLDER tests/thirdparty)

################################
# Optional TPL Unit Tests
################################

if(ZLIB_FOUND)
    message(STATUS "Adding zlib smoke test")
    add_cpp_test(TEST t_zlib_smoke
                 DEPENDS_ON ZLIB::ZLIB
                 FOLDER tests/thirdparty)
endif()

if(ENABLE_RELAY_WEBSERVER)
    message(STATUS "Relay Web Server enabled: Added related third party unit tests")
    set(civet_test_deps ${CIVETWEB_LIB_DEPENDS})
    if(UNIX AND NOT APPLE)
        # we need these for civetweb on linux, we may need similar libs
        # on windows (OSX appears ok without them)
        list(APPEND civet_test_deps dl rt ${CMAKE_THREAD_LIBS_INIT})
    endif()


    if (MINGW)
      list(APPEND civet_test_deps ws2_32)
    endif ()

    add_cpp_test(TEST t_civetweb_smoke
                 SOURCES $<TARGET_OBJECTS:conduit_civetweb>
                 DEPENDS_ON ${civet_test_deps}
                 FOLDER tests/thirdparty)
else()
    message(STATUS "Relay Web Server disabled: Skipping related third party tests")
endif()


if(PYTHON_FOUND)
    message(STATUS "Python enabled: Adding related unit tests")
    foreach(TEST t_numpy_smoke)
        add_cpp_test(TEST t_numpy_smoke
                     FOLDER tests/thirdparty)
        target_link_libraries(${TEST} PUBLIC ${PYTHON_LIBRARIES})
    endforeach()
else()
    message(STATUS "Python disabled: Skipping related tests")
endif()


if(HDF5_FOUND)
    message(STATUS "HDF5 enabled: Adding related unit tests")
    if(HDF5_IS_PARALLEL)
       set(HDF5_MPI_DEP ${conduit_blt_mpi_deps})
    endif()
    add_cpp_test(TEST t_hdf5_smoke
                 DEPENDS_ON hdf5 ${HDF5_MPI_DEP}
                 FOLDER tests/thirdparty)
else()
    message(STATUS "HDF5 disabled: Skipping related tests")
endif()


if(SILO_FOUND)
    if(HDF5_FOUND AND HDF5_IS_PARALLEL)
       set(SILO_HDF5_MPI_DEP ${conduit_blt_mpi_deps})
    endif()
    message(STATUS "Silo enabled: Adding related unit tests")
        add_cpp_test(TEST t_silo_smoke
                     DEPENDS_ON silo ${SILO_HDF5_MPI_DEP}
                     FOLDER tests/thirdparty)
else()
    message(STATUS "Silo disabled: Skipping related tests")
endif()

if(ADIOS_FOUND)
    set(serial_adios_deps adios_nompi)
    if(UNIX AND NOT APPLE)
        list(APPEND serial_adios_deps dl rt ${CMAKE_THREAD_LIBS_INIT})
    endif()
    message(STATUS "ADIOS enabled: Adding related unit tests")
        add_cpp_test(TEST t_adios_smoke
                     DEPENDS_ON ${serial_adios_deps}
                     FOLDER tests/thirdparty)
else()
    message(STATUS "ADIOS disabled: Skipping related tests")
endif()

if(FORTRAN_FOUND)
    message(STATUS "Fortran enabled: Adding related unit tests")
    add_fortran_test(TEST t_fortran_smoke
                     FOLDER tests/thirdparty)
else()
    message(STATUS "Fortran disabled: Skipping related tests")
endif()

if(RAJA_FOUND)
    message(STATUS "Adding raja smoke test")
    set(raja_smoke_deps RAJA ${smoke_device_deps})
    add_cpp_test(TEST t_raja_smoke DEPENDS_ON ${raja_smoke_deps})
endif()

if(UMPIRE_FOUND)
    message(STATUS "Adding umpire smoke test")
    set(umpire_smoke_deps umpire ${smoke_device_deps})
    add_cpp_test(TEST t_umpire_smoke DEPENDS_ON ${umpire_smoke_deps})
endif()

if(ZFP_FOUND)
    message(STATUS "Zfp enabled: Adding related unit tests")
    add_cpp_test(TEST t_zfp_smoke
                 DEPENDS_ON zfp
                 FOLDER tests/thirdparty)
else()
    message(STATUS "Zfp disabled: Skipping related tests")
endif()

if(H5ZZFP_FOUND)
    message(STATUS "h5z-zfp enabled: Adding related unit tests")
    if(HDF5_IS_PARALLEL)
        set(H5ZZFP_MPI_DEP ${conduit_blt_mpi_deps})
    endif()
    add_cpp_test(TEST t_h5z_zfp_smoke
                 DEPENDS_ON h5zzfp zfp hdf5 ${H5ZZFP_MPI_DEP}
                 FOLDER tests/thirdparty)
else()
    message(STATUS "h5z-zfp disabled: Skipping related tests")
endif()


if(PARMETIS_FOUND)
    message(STATUS "Parmetis enabled: Adding related unit tests")
    add_cpp_mpi_test(TEST t_parmetis_smoke
                     NUM_MPI_TASKS 2
                     DEPENDS_ON parmetis
                     FOLDER tests/thirdparty)
else()
    message(STATUS "Parmetis disabled: Skipping related tests")
endif()


if(CALIPER_FOUND)
    message(STATUS "Caliper enabled: Adding related unit tests")
    add_cpp_test(TEST t_caliper_smoke
                 DEPENDS_ON caliper
                 FOLDER tests/thirdparty)
else()
    message(STATUS "Caliper disabled: Skipping related tests")
endif()


