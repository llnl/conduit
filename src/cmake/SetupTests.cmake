# Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
# Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
# other details. No copyright assignment is required to contribute to Conduit.

set(UNIT_TEST_BASE_LIBS gtest_main gtest)


#####################################################################
if(CONDUIT_ENABLE_TESTS AND WIN32 AND BUILD_SHARED_LIBS)
    # Copy DLLs into our bin dir so we can satisfy
    # deps to run tests.
    #
    # Note: There are per target ways to do this, however
    #       all of our many, many tests share these dlls so
    #       I opted for a single copy step, instead of
    #       trying to track and copy each test.
    #
    # Show TPL DLL Paths
    message(STATUS "CONDUIT_TPL_DLL_PATHS: ${CONDUIT_TPL_DLL_PATHS}")
    # glob and gather dlls from all TPL dirs
    set(tpl_all_dlls)
    foreach( tpl_dll_path in ${CONDUIT_TPL_DLL_PATHS})
        file(GLOB tpl_glob_dlls ${tpl_dll_path}/*.dll)
        foreach( tpl_dll ${tpl_glob_dlls})
            list(APPEND tpl_all_dlls ${tpl_dll})
        endforeach()
    endforeach()
    add_custom_target(tpl_dlls_dir ALL
                      COMMAND ${CMAKE_COMMAND} -E make_directory
                      ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
    if(tpl_all_dlls)
        add_custom_target(tpl_dlls ALL
                          COMMAND ${CMAKE_COMMAND} -E copy
                          ${tpl_all_dlls}
                          ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
        add_dependencies(tpl_dlls tpl_dlls_dir)
    endif()
endif()


##------------------------------------------------------------------------------
## - Builds and adds a test that uses gtest
##
## add_cpp_test( TEST test SOURCES extra.cpp ... DEPENDS_ON dep1 dep2 ... )
##------------------------------------------------------------------------------
function(add_cpp_test)

    set(options)
    set(singleValueArgs TEST FOLDER)
    set(multiValueArgs DEPENDS_ON SOURCES)

    # parse our arguments
    cmake_parse_arguments(args
                         "${options}"
                         "${singleValueArgs}"
                         "${multiValueArgs}" ${ARGN} )
   
    message(STATUS " [*] Adding Unit Test: ${args_TEST} ")

    # note: OUTPUT_DIR is ignored on windows
    blt_add_executable( NAME ${args_TEST}
                        SOURCES ${args_TEST}.cpp ${args_SOURCES}
                        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
                        DEPENDS_ON "${args_DEPENDS_ON}" gtest)


    blt_add_test( NAME ${args_TEST}
                  COMMAND ${args_TEST})

    # set folder if passed
    if( DEFINED args_FOLDER )
        blt_set_target_folder(TARGET ${args_TEST} FOLDER ${args_FOLDER})
    endif()

endfunction()


##------------------------------------------------------------------------------
## - Builds and adds a test that uses gtest and mpi
##
## add_cpp_mpi_test( TEST test NUM_MPI_TASKS 2 DEPENDS_ON dep1 dep2... )
##------------------------------------------------------------------------------
function(add_cpp_mpi_test)

    set(options)
    set(singleValueArgs TEST NUM_MPI_TASKS FOLDER)
    set(multiValueArgs DEPENDS_ON SOURCES)

    # parse our arguments
    cmake_parse_arguments(args
                         "${options}"
                         "${singleValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

    message(STATUS " [*] Adding Unit Test: ${args_TEST}")
    
    # note: OUTPUT_DIR is ignored on windows
    blt_add_executable( NAME ${args_TEST}
                        SOURCES ${args_TEST}.cpp ${args_SOURCES}
                        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
                        DEPENDS_ON "${args_DEPENDS_ON}" gtest ${conduit_blt_mpi_deps})

    blt_add_test( NAME ${args_TEST}
                  COMMAND ${args_TEST}
                  NUM_MPI_TASKS ${args_NUM_MPI_TASKS})

    # set folder if passed
    if( DEFINED args_FOLDER )
        blt_set_target_folder(TARGET ${args_TEST} FOLDER ${args_FOLDER})
    endif()

endfunction()


##------------------------------------------------------------------------------
## - Adds a python based unit test
##
## add_python_test( TEST test)
##------------------------------------------------------------------------------
function(add_python_test )
    set(options)
    set(singleValueArgs TEST )
    set(multiValueArgs )

    # parse our arguments
    cmake_parse_arguments(args
                         "${options}"
                         "${singleValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

    message(STATUS " [*] Adding Python-based Unit Test: ${args_TEST}")
    add_test(NAME ${args_TEST} COMMAND
             ${PYTHON_EXECUTABLE} -B -m unittest -v ${args_TEST})

    # use proper env var path sep for current platform
    if(WIN32)
        set(ENV_PATH_SEP "\\;")
    else()
        set(ENV_PATH_SEP ":")
    endif()
    # make sure python can pick up the modules we built
    # if python path is already set -- we need to append to it
    # this is important for running in spack's build-env
    set(py_path "")
    if(DEFINED ENV{PYTHONPATH})
        set(py_path "$ENV{PYTHONPATH}${ENV_PATH_SEP}")
    endif()
    set_property(TEST ${args_TEST}
                 PROPERTY
                 ENVIRONMENT "PYTHONPATH=${py_path}${CMAKE_BINARY_DIR}/python-modules/${ENV_PATH_SEP}${CMAKE_CURRENT_SOURCE_DIR}")
    if(WIN32)
        # proper path to dlls for vstudio and proper path for non config based gen (nmake, etc)
        set_property(TEST ${args_TEST}
                     APPEND
                     PROPERTY
                     ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/${ENV_PATH_SEP}${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${ENV_PATH_SEP}$ENV{PATH}")
        # NOTE: windows python 3.8 and newer ignore PATH when loading DLLs
        #   https://docs.python.org/3/whatsnew/3.8.html#bpo-36085-whatsnew
        #   https://docs.python.org/3/library/os.html#os.add_dll_directory
        # we set CONDUIT_DLL_DIR so that the python module can find the our DLLs
        set_property(TEST ${args_TEST}
                     APPEND
                     PROPERTY
                     ENVIRONMENT "CONDUIT_DLL_DIR=${CMAKE_BINARY_DIR}/bin/${ENV_PATH_SEP}${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${ENV_PATH_SEP}$ENV{PATH}")
    endif()

endfunction(add_python_test)

##------------------------------------------------------------------------------
## - Builds and adds a test that uses python and mpi
##
## add_python_mpi_test( TEST test NUM_MPI_TASKS 2 )
##------------------------------------------------------------------------------
function(add_python_mpi_test)

    set(options)
    set(singleValueArgs TEST NUM_MPI_TASKS FOLDER)

    # parse our arguments
    cmake_parse_arguments(args
                         "${options}"
                         "${singleValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

    message(STATUS " [*] Adding Python-based MPI Unit Test: ${args_TEST}")
    set(test_command ${PYTHON_EXECUTABLE} -B -m unittest -v ${args_TEST})

    # Handle mpi
    if ( ${args_NUM_MPI_TASKS} )
          set(test_command ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${args_NUM_MPI_TASKS} ${test_command} )
    endif()

    add_test(NAME ${args_TEST}
             COMMAND ${test_command} )

    # use proper env var path sep for current platform
    if(WIN32)
        set(ENV_PATH_SEP "\\;")
    else()
        set(ENV_PATH_SEP ":")
    endif()
    # make sure python can pick up the modules we built
    # if python path is already set -- we need to append to it
    # this is important for running in spack's build-env
    set(py_path "")
    if(DEFINED ENV{PYTHONPATH})
        set(py_path "$ENV{PYTHONPATH}${ENV_PATH_SEP}")
    endif()

    set_property(TEST ${args_TEST}
                 PROPERTY
                 ENVIRONMENT "PYTHONPATH=${py_path}${CMAKE_BINARY_DIR}/python-modules/${ENV_PATH_SEP}${CMAKE_CURRENT_SOURCE_DIR}")
    if(WIN32)
        # proper path to dlls for vstudio and proper path for non config based gen (nmake, etc)
        set_property(TEST ${args_TEST}
                     APPEND
                     PROPERTY
                     ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin/${ENV_PATH_SEP}${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${ENV_PATH_SEP}$ENV{PATH}")
        # NOTE: windows python 3.8 and newer ignore PATH when loading DLLs
        #   https://docs.python.org/3/whatsnew/3.8.html#bpo-36085-whatsnew
        #   https://docs.python.org/3/library/os.html#os.add_dll_directory
        # we set CONDUIT_DLL_DIR so that the python module can find the our DLLs
        set_property(TEST ${args_TEST}
                     APPEND
                     PROPERTY
                     ENVIRONMENT "CONDUIT_DLL_DIR=${CMAKE_BINARY_DIR}/bin/${ENV_PATH_SEP}${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${ENV_PATH_SEP}$ENV{PATH}")
    endif()

    ###########################################################################
    # Newer versions of OpenMPI require OMPI_MCA_rmaps_base_oversubscribe=1
    # to run with more tasks than actual cores
    # Since this is an OpenMPI specific env var, it shouldn't interfere
    # with other mpi implementations.
    ###########################################################################
    set_property(TEST ${args_TEST}
                 APPEND
                 PROPERTY ENVIRONMENT  "OMPI_MCA_rmaps_base_oversubscribe=1")

    # set folder if passed
    if( DEFINED args_FOLDER )
        blt_set_target_folder(TARGET ${args_TEST} FOLDER ${args_FOLDER})
    endif()

endfunction()

##------------------------------------------------------------------------------
## - Adds a fortran based unit test
##
## add_fortran_test( TEST test DEPENDS_ON dep1 dep2... )
##------------------------------------------------------------------------------
macro(add_fortran_test)
    set(options)
    set(singleValueArgs TEST FOLDER )
    set(multiValueArgs DEPENDS_ON SOURCES )

    # parse our arguments
    cmake_parse_arguments(arg
                         "${options}"
                         "${singleValueArgs}"
                         "${multiValueArgs}" ${ARGN} )

    message(STATUS " [*] Adding Fortran Unit Test: ${arg_TEST}")

    # note: OUTPUT_DIR is ignored on windows

    blt_add_executable( NAME ${arg_TEST}
                        SOURCES "${arg_TEST}.f90" ${arg_SOURCES}
                        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
                        DEPENDS_ON fruit "${arg_DEPENDS_ON}")

    blt_add_test( NAME ${arg_TEST}
                  COMMAND  ${arg_TEST})

    # set folder if passed
    if( DEFINED args_FOLDER )
        blt_set_target_folder(TARGET ${args_TEST} FOLDER ${args_FOLDER})
    endif()

endmacro(add_fortran_test)
