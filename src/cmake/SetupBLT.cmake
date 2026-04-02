# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

################################################################
# if BLT_SOURCE_DIR is not set - use "blt" as default
################################################################
if(NOT BLT_SOURCE_DIR)
    set(BLT_SOURCE_DIR "blt")
endif()

################################################################
# if not set, prefer c++17 lang standard
################################################################
if(NOT BLT_CXX_STD)
    set(BLT_CXX_STD "c++17" CACHE STRING "")
endif()

################################################################
# if not set, prefer folder grouped targets
################################################################
if(NOT ENABLE_FOLDERS)
    set(ENABLE_FOLDERS TRUE CACHE STRING "")
endif()

################################################################
# init blt using BLT_SOURCE_DIR
################################################################
include(${BLT_SOURCE_DIR}/SetupBLT.cmake)

if(ENABLE_MPI)
    set(CONDUIT_MPI_USES_CMAKE_TARGETS OFF CACHE BOOL "")

    # on some platforms (mostly cray systems) folks skip mpi
    # detection in BLT by setting ENABLE_FIND_MPI = OFF
    # in these cases, we need to set MPI_FOUND = TRUE,
    # since the rest of our cmake logic to include MPI uses MPI_FOUND
    if(NOT ENABLE_FIND_MPI)
        set(MPI_FOUND ON CACHE BOOL "")
    endif()

    #################
    # defines to prevent mpi from using C++ apis and introducing link
    # dep of libmpi_cxx  (encountred when using openmpi as mpi)
    #
    # In theory, calling set(MPI_CXX_SKIP_MPICXX TRUE BOOL) before FindMPI
    # would also add these flags, but that did not work w/ cmake 3.22
    #################
    set(CONDUIT_MPI_SKIP_MPICXX_DEFINES MPICH_SKIP_MPICXX OMPI_SKIP_MPICXX _MPICC_H)

    # if we are using BLT's enable mpi, then we must
    # make sure the MPI targets exist
    if(ENABLE_FIND_MPI)
        if(TARGET MPI::MPI_C)
            set(CONDUIT_USE_CMAKE_MPI_TARGETS TRUE CACHE BOOL "")
            message(STATUS "Using MPI CMake imported target: MPI::MPI_C")
            # use cmake mpi targets directly
            set(conduit_blt_mpi_deps MPI::MPI_C CACHE STRING "")
        else()
            message(FATAL_ERROR "Cannot use CMake imported targets for MPI."
                                "(ENABLE_MPI == ON, but "
                                "MPI::MPI_C CMake target is missing.)")
        endif()
    else()
        set(CONDUIT_USE_CMAKE_MPI_TARGETS FALSE CACHE BOOL "")
        # compiler will handle them implicitly
        set(conduit_blt_mpi_deps "" CACHE STRING "")
    endif()

    # In some cases (mpich?) -fallow-argument-mismatch will be
    # reported as a needed MPI flag for fortran.
    # BLT fuses all MPI compiler flags into one big bunch.
    # (It does not differentiate between C and fortran flags)
    #
    # -fallow-argument-mismatch is a fortran compiler flag that makes clang
    # very unhappy, and this will cause blt's mpi smoke test to fail to build
    # with clang.
    #
    # Conduit does not use mpi fortran, so we strip this flag if it exists.
    #
    # blt's mpi target is called "mpi"
    if(TARGET mpi)
    # check and strip interface compile opts
        get_target_property(_mpi_iface_compile_opts mpi INTERFACE_COMPILE_OPTIONS)
        if(_mpi_iface_compile_opts)
            list(REMOVE_ITEM _mpi_iface_compile_opts "-fallow-argument-mismatch")
            list(REMOVE_ITEM _mpi_iface_compile_opts "-fallow-invalid-boz")
            set_target_properties(mpi PROPERTIES INTERFACE_COMPILE_OPTIONS "${_mpi_iface_compile_opts}")
        endif()
    endif()

endif()

if(ENABLE_OPENMP)
    if(TARGET OpenMP::OpenMP_CXX)
        set(CONDUIT_USE_CMAKE_OPENMP_TARGETS TRUE CACHE BOOL "")
        message(STATUS "Using OpenMP CMake imported target: OpenMP::OpenMP_CXX")
        # use cmake openmp targets directly
        set(conduit_blt_openmp_deps OpenMP::OpenMP_CXX CACHE STRING "")
    else()
        message(FATAL_ERROR "Cannot use CMake imported targets for OpenMP."
                            "(ENABLE_OPENMP == ON, but "
                            "OpenMP::OpenMP_CXX CMake target is missing.)")
    endif()
endif()


################################
# Invoke CMake HIP lang setup
# if ENABLE_HIP == ON
################################
if(ENABLE_HIP)
    enable_language(HIP)
endif()

#
# case sensitive awesomness
#
if(ENABLE_HIP AND hip_FOUND)
    set(HIP_FOUND TRUE)
endif()

################################################################
# apply folders to a few ungrouped blt targets
################################################################

###############################################
# group main blt docs targets into docs folder
###############################################
blt_set_target_folder( TARGET docs FOLDER docs)

if(TARGET sphinx_docs)
    blt_set_target_folder( TARGET sphinx_docs FOLDER docs)
endif()

if(TARGET doxygen_docs)
    blt_set_target_folder( TARGET doxygen_docs FOLDER docs)
endif()

####################################################
# group top level blt health checks into blt folder
####################################################
if(TARGET check)
    blt_set_target_folder( TARGET check FOLDER blt)
endif()

if(TARGET style)
    blt_set_target_folder( TARGET style FOLDER blt)
endif()


