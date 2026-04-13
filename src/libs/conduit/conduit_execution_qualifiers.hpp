// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_qualifiers.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_QUALIFIERS_HPP
#define CONDUIT_EXECUTION_QUALIFIERS_HPP

#include "conduit_config.hpp"

//-----------------------------------------------------------------------------
// -- host + device annotation helper --
//-----------------------------------------------------------------------------
#if (defined(CONDUIT_USE_CUDA) && defined(__CUDACC__)) || \
    (defined(CONDUIT_USE_HIP) && defined(__HIPCC__))
#define CONDUIT_EXEC_HOST_DEVICE __host__ __device__
#else
#define CONDUIT_EXEC_HOST_DEVICE
#endif

#endif
