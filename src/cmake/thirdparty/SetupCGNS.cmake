
# first Check for CGNS_DIR
if(NOT CGNS_DIR)
    MESSAGE(FATAL_ERROR "CGNS support needs explicit CGNS_DIR")
endif()

set(CGNS_ROOT ${CGNS_DIR})

find_package(CGNS REQUIRED)

get_target_property(CGNS_INCLUDE_DIRS CGNS::cgns_shared INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "CGNS Include Directory: ${CGNS_INCLUDE_DIRS}")