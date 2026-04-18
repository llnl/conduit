// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_execution.hpp"

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------

#if defined(CONDUIT_USE_CUDA)
#include <cuda_runtime.h>
#endif

#if defined(CONDUIT_USE_HIP)
#include <hip/hip_runtime.h>
#endif


//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::execution --
//-----------------------------------------------------------------------------
namespace execution
{

//-----------------------------------------------------------------------------
/// policy constructor helpers
//-----------------------------------------------------------------------------
ExecutionPolicy
ExecutionPolicy::empty()
{
    return ExecutionPolicy(PolicyID::EMPTY_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::host()
{
#if defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    return ExecutionPolicy(PolicyID::SERIAL_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::serial()
{
    return ExecutionPolicy(PolicyID::SERIAL_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::device()
{
#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA)
    return ExecutionPolicy(PolicyID::CUDA_ID);
#elif defined(CONDUIT_EXEC_BUILD_HAS_HIP)
    return ExecutionPolicy(PolicyID::HIP_ID);
#else
    CONDUIT_ERROR("Conduit was built with neither CUDA nor HIP.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::cuda()
{
    return ExecutionPolicy(PolicyID::CUDA_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::hip()
{
    return ExecutionPolicy(PolicyID::HIP_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::openmp()
{
#if defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    CONDUIT_ERROR("Conduit was built without openMP.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::parallel()
{
#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA)
    return ExecutionPolicy(PolicyID::CUDA_ID);
#elif defined(CONDUIT_EXEC_BUILD_HAS_HIP)
    return ExecutionPolicy(PolicyID::HIP_ID);
#elif defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    return ExecutionPolicy(PolicyID::SERIAL_ID);
#endif
}

//=============================================================================
//-----------------------------------------------------------------------------
//
//
// -- begin conduit::execution::ExecutionPolicy public methods --
//
//
//-----------------------------------------------------------------------------
//=============================================================================

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------
//---------------------------------------------------------------------------//
ExecutionPolicy::ExecutionPolicy(const std::string &policy_name)
: m_policy_id(name_to_policy_id(policy_name))
{}

//-----------------------------------------------------------------------------
// Getters and info methods.
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_empty() const
{
    return m_policy_id == PolicyID::EMPTY_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_serial() const
{
    return m_policy_id == PolicyID::SERIAL_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_cuda() const
{
    return m_policy_id == PolicyID::CUDA_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_hip() const
{
    return m_policy_id == PolicyID::HIP_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_openmp() const
{
    return m_policy_id == PolicyID::OPENMP_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_host_policy() const
{
    return is_serial() || is_openmp();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_device_policy() const
{
    return is_cuda() || is_hip();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_parallel_policy() const
{
    return is_cuda() || is_hip() || is_openmp();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_serial_enabled()
{
    return true;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_cuda_enabled()
{
#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_hip_enabled()
{
#if defined(CONDUIT_EXEC_BUILD_HAS_HIP)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_openmp_enabled()
{
#if defined(CONDUIT_USE_OPENMP)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_host_enabled()
{
    return is_serial_enabled() || is_openmp_enabled();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_device_enabled()
{
    return is_cuda_enabled() || is_hip_enabled();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_parallel_enabled()
{
    return is_device_enabled() || is_openmp_enabled();
}

//-----------------------------------------------------------------------------
// PolicyID to string and string to PolicyID
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
ExecutionPolicy::PolicyID
ExecutionPolicy::name_to_policy_id(const std::string &policy_name)
{
    if      (policy_name == "empty")    return PolicyID::EMPTY_ID;
    else if (policy_name == "serial")   return PolicyID::SERIAL_ID;
    else if (policy_name == "cuda")     return PolicyID::CUDA_ID;
    else if (policy_name == "hip")      return PolicyID::HIP_ID;
    else if (policy_name == "openmp")   return PolicyID::OPENMP_ID;
    else if (policy_name == "host")     return host().policy_id();
    else if (policy_name == "device")   return device().policy_id();
    else if (policy_name == "parallel") return parallel().policy_id();
    return PolicyID::EMPTY_ID;
}

//---------------------------------------------------------------------------//
std::string 
ExecutionPolicy::policy_id_to_name(const PolicyID policy_id)
{
    if      (policy_id == PolicyID::EMPTY_ID)   return "empty";
    else if (policy_id == PolicyID::SERIAL_ID)  return "serial";
    else if (policy_id == PolicyID::CUDA_ID)    return "cuda";
    else if (policy_id == PolicyID::HIP_ID)     return "hip";
    else if (policy_id == PolicyID::OPENMP_ID)  return "openmp";
    return "empty";
}

//---------------------------------------------------------------------------//
void
init_device_memory_handlers()
{
#if defined(CONDUIT_EXEC_BUILD_HAS_DEVICE)
    // we only need to override the mem handlers in the
    // presence of cuda or hip
    conduit::utils::set_memcpy_handler(MagicMemory::copy);
    conduit::utils::set_memset_handler(MagicMemory::set);
#endif
}

//---------------------------------------------------------------------------//
void
device_error_check(ExecutionPolicy policy, const char *file, const int line)
{
    if (policy.is_hip())
    {
#if defined(CONDUIT_EXEC_BUILD_HAS_HIP)
        hipError_t err = hipGetLastError();
        if ( hipSuccess != err )
        {
            std::cerr<<"HIP error reported at: "<<file<<":"<<line;
            std::cerr<<" : "<<hipGetErrorName(err)<<"\n";
            //exit( -1 );
        }
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA)
        cudaError err = cudaGetLastError();
        if ( cudaSuccess != err )
        {
            std::cerr<<"CUDA error reported at: "<<file<<":"<<line;
            std::cerr<<" : "<<cudaGetErrorString(err)<<"\n";
            //exit( -1 );
        }
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }

    (void)file; // suppress unused variable warnings
    (void)line; // suppress unused variable warnings
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
