// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_decorators.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_DECORATORS_HPP
#define CONDUIT_EXECUTION_DECORATORS_HPP

#include "conduit_config.hpp"

//-----------------------------------------------------------------------------
// Why this header exists:
//
// These macros are needed by low-level Conduit headers such as
// conduit_core.hpp, conduit_data_array.hpp, and conduit_data_accessor.hpp.
// Those headers need lightweight access to host/device decoration without
// depending on the full execution API in conduit_execution.hpp.
//
// Keeping the macros here preserves a clean dependency boundary:
// - low-level data structure headers can opt into host/device decoration
//   without pulling in execution policy or dispatch machinery
// - conduit_execution.hpp remains a higher-level API entry point instead of
//   becoming a prerequisite for basic core/data headers
// - we avoid dragging RAJA-facing execution headers into translation units
//   that only need annotations like __host__ __device__
//
// In short, this header exists so decoration macros stay lightweight and can
// be used below the execution layer in Conduit's include graph.
//-----------------------------------------------------------------------------
// -- host + device annotation helper --
//-----------------------------------------------------------------------------
#if (defined(CONDUIT_USE_CUDA) && defined(__CUDACC__)) || \
    (defined(CONDUIT_USE_HIP) && defined(__HIPCC__))
#define CONDUIT_EXEC_HOST_DEVICE __host__ __device__
#else
#define CONDUIT_EXEC_HOST_DEVICE
#endif

//-----------------------------------------------------------------------------
// -- device compilation helper --
//-----------------------------------------------------------------------------
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define CONDUIT_EXEC_DEVICE_COMPILE
#endif

#endif
