// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_macros.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_MACROS_HPP
#define CONDUIT_EXECUTION_MACROS_HPP

//
// Macro disambiguation:
//

// 1. CONDUIT_USE_CUDA/CONDUIT_USE_HIP: our build enabled these
// backends.

// 2. CONDUIT_USE_DEVICE: we are using CUDA or HIP and UMPIRE is
// enabled.
#if (defined(CONDUIT_USE_CUDA) || defined(CONDUIT_USE_HIP)) && defined(CONDUIT_USE_UMPIRE)
#define CONDUIT_USE_DEVICE
#endif

// 3. CONDUIT_TU_IS_CUDA/CONDUIT_TU_IS_HIP: our current translation
// unit is a CUDA/HIP target. We cannot get away with just using
// CONDUIT_USE_*** because execution is included broadly, and
// normal host-only TUs will not compile symbols like
// RAJA::cuda_exec/RAJA::hip_exec.
#if defined(CONDUIT_USE_CUDA) && defined(__CUDACC__)
#define CONDUIT_TU_IS_CUDA
#endif

#if defined(CONDUIT_USE_HIP) && defined(__HIPCC__)
#define CONDUIT_TU_IS_HIP
#endif

// 4. CONDUIT_DEVICE_COMPILE: means the compiler is compiling
// for the device right now (typically there is a host compilation
// pass and a device pass). This is useful for having different
// behavior for host and device (like for error-handling).
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define CONDUIT_DEVICE_COMPILE
#endif

// 5. CONDUIT_EXEC: host/device function decorator. For a
// CUDA/HIP TU, any function marked with this is compiled for both
// host and device, while for a normal C++ TU it means nothing.
#if defined(CONDUIT_TU_IS_CUDA) || defined(CONDUIT_TU_IS_HIP)
#define CONDUIT_EXEC __host__ __device__
#else
#define CONDUIT_EXEC
#endif

// 6. CONDUIT_DEVICE_ERROR_CHECK: error checking macro
#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

#endif
