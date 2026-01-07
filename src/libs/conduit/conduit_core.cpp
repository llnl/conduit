// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_core.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"


//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_node.hpp"

// Note: This header is only needed a compile time.
#include "conduit_license.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{


//---------------------------------------------------------------------------//
std::string
about()
{
    Node n;
    about(n);
    return n.to_yaml();
}

//---------------------------------------------------------------------------//
void
about(Node &n)
{
    n.reset();
    n["version"] = CONDUIT_VERSION;
    n["version_major"].set_int64(CONDUIT_VERSION_MAJOR);
    n["version_minor"].set_int64(CONDUIT_VERSION_MINOR);
    n["version_patch"].set_int64(CONDUIT_VERSION_PATCH);

#ifdef CONDUIT_GIT_SHA1
    n["git_sha1"] = CONDUIT_GIT_SHA1;
#else
    n["git_sha1"] = "unknown";
#endif

#ifdef CONDUIT_GIT_SHA1_ABBREV
    n["git_sha1_abbrev"] = CONDUIT_GIT_SHA1_ABBREV;
#else
    n["git_sha1_abbrev"] = "unknown";
#endif

#ifdef CONDUIT_GIT_TAG
    n["git_tag"] = CONDUIT_GIT_TAG;
#else
    n["git_tag"] = "unknown";
#endif

    if(n["git_tag"].as_string() == "unknown" && 
       n["git_sha1_abbrev"].as_string() != "unknown")
    {
        n["version"] = n["version"].as_string()
                       + "-" + n["git_sha1_abbrev"].as_string();
    }

    n["compilers/cpp"] = CONDUIT_CPP_COMPILER;
#ifdef CONDUIT_FORTRAN_COMPILER
    n["compilers/fortran"] = CONDUIT_FORTRAN_COMPILER;
#endif

#if   defined(CONDUIT_PLATFORM_WINDOWS)
    n["platform"] = "windows";
#elif defined(CONDUIT_PLATFORM_APPLE)
    n["platform"] = "apple";
#else 
    n["platform"] = "linux";
#endif
    
    n["system"] = CONDUIT_SYSTEM_TYPE;
    n["install_prefix"] = CONDUIT_INSTALL_PREFIX;
    n["license"] = CONDUIT_LICENSE_TEXT;

    Node &nt = n["index_t_typemap"];

    // index_t
#ifdef CONDUIT_INDEX_32
    nt["index_t"] = "int32";
    nt["sizeof_index_t"] = 4;
#else
    nt["index_t"] = "int64";
    nt["sizeof_index_t"] = 8;
#endif

    // Type Info Map
    Node &nn = n["native_typemap"];

// caliper annotations support
#if defined(CONDUIT_USE_CALIPER)
    n["annotations"] = "enabled";
#else
    n["annotations"] = "disabled";
#endif

// device support
#if defined(CONDUIT_USE_CUDA)
    n["device_support"] = "cuda";
#elif defined(CONDUIT_USE_HIP)
    n["device_support"] = "hip";
#else
    n["device_support"] = "disabled";
#endif

    // ints
#ifdef CONDUIT_INT8_NATIVE_NAME
    nn["int8"] = CONDUIT_INT8_NATIVE_NAME;
#else
    nn["int8"] = "<unmapped>";
#endif
#ifdef CONDUIT_INT16_NATIVE_NAME
    nn["int16"] = CONDUIT_INT16_NATIVE_NAME;
#else
    nn["int16"] = "<unmapped>";
#endif
#ifdef CONDUIT_INT32_NATIVE_NAME
    nn["int32"] = CONDUIT_INT32_NATIVE_NAME;
#else
    nn["int32"] = "<unmapped>";
#endif
#ifdef CONDUIT_INT64_NATIVE_NAME
    nn["int64"] = CONDUIT_INT64_NATIVE_NAME;
#else
    nn["int64"] = "<unmapped>";
#endif

    // unsigned ints
#ifdef CONDUIT_UINT8_NATIVE_NAME
    nn["uint8"] = CONDUIT_UINT8_NATIVE_NAME;
#else
    nn["uint8"] = "<unmapped>";
#endif
#ifdef CONDUIT_UINT16_NATIVE_NAME
    nn["uint16"] = CONDUIT_UINT16_NATIVE_NAME;
#else
    nn["uint16"] = "<unmapped>";
#endif
#ifdef CONDUIT_UINT32_NATIVE_NAME
    nn["uint32"] = CONDUIT_UINT32_NATIVE_NAME;
#else
    nn["uint32"] = "<unmapped>";
#endif
#ifdef CONDUIT_UINT64_NATIVE_NAME
    nn["uint64"] = CONDUIT_UINT64_NATIVE_NAME;
#else
    nn["uint64"] = "<unmapped>";
#endif

    // floating points numbers
#ifdef CONDUIT_FLOAT32_NATIVE_NAME
    nn["float32"] = CONDUIT_FLOAT32_NATIVE_NAME;
#else
    nn["float32"] = "<unmapped>";
#endif
#ifdef CONDUIT_FLOAT64_NATIVE_NAME
    nn["float64"] = CONDUIT_FLOAT64_NATIVE_NAME;
#else
    nn["float64"] = "<unmapped>";
#endif

    // index_t
#ifdef CONDUIT_INDEX_32
    nn["index_t"] = CONDUIT_INT32_NATIVE_NAME;
#else
    nn["index_t"] = CONDUIT_INT64_NATIVE_NAME;
#endif


}


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

