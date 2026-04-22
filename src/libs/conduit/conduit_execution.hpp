// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_HPP
#define CONDUIT_EXECUTION_HPP

// Conduit's host/device decorators live here so headers that need execution
// annotations can include the execution facade directly instead of depending
// on a separate decorators-only header.
#include "conduit_config.hpp"

#if (defined(CONDUIT_USE_CUDA) && defined(__CUDACC__)) || \
    (defined(CONDUIT_USE_HIP) && defined(__HIPCC__))
#define CONDUIT_EXEC_HOST_DEVICE __host__ __device__
#else
#define CONDUIT_EXEC_HOST_DEVICE
#endif

#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define CONDUIT_EXEC_DEVICE_COMPILE
#endif

#include "conduit_execution_policy.hpp"
#include "conduit_execution_core.hpp"

#endif
