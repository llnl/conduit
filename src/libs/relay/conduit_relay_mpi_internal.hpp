// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi_internal.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_MPI_INTERNAL_HPP
#define CONDUIT_RELAY_MPI_INTERNAL_HPP

#include "conduit_relay_mpi.hpp"

namespace conduit
{
namespace relay
{
namespace mpi
{
namespace detail
{

// Internal helper for in-tree tests. This header is not installed.
int CONDUIT_RELAY_API tag_upper_bound_probe(MPI_Comm comm);

}
}
}
}

#endif
