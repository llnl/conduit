// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi_io_adios2.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_MPI_IO_ADIOS2_HPP
#define CONDUIT_RELAY_MPI_IO_ADIOS2_HPP

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <adios2_mpi.h>

//-----------------------------------------------------------------------------
// conduit lib include 
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_relay_exports.h"
#include "conduit_relay_config.h"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi::io --
//-----------------------------------------------------------------------------
namespace io
{

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_initialize_library(MPI_Comm comm);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_finalize_library(MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Write node data to a given path
///
/// This methods supports a file system and adios2 path, joined using a ":"
///  ex: "/path/on/file/system.bp:/path/inside/adios2/file"
/// 
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_save(const Node &node,
                                   const std::string &path,
                                   MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Write node data to a given path in an existing file.
///
/// This methods supports a file system and adios2 path, joined using a ":"
///  ex: "/path/on/file/system.bp:/path/inside/adios2/file"
/// 
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_save_merged(const Node &node,
                                          const std::string &path,
                                          MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Add a step of node data to an existing file.
///
/// This methods supports a file system and adios2 path, joined using a ":"
///  ex: "/path/on/file/system.adios2:/path/inside/adios2/file"
/// 
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_add_step(const Node &node,
                                       const std::string &path,
                                       MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Read adios2 data from given path into the output node 
/// 
/// This methods supports a file system and adios2 path, joined using a ":"
///  ex: "/path/on/file/system.bp:/path/inside/adios2/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_load(const std::string &path,
                                   Node &node,
                                   MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Read a given step and domain of adios2 data from given path into the
//  output node.
/// 
/// This methods supports a file system and adios2 path, joined using a ":"
///  ex: "/path/on/file/system.bp:/path/inside/adios2/file"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_load(const std::string &path,
                                   int step,
                                   int domain,
                                   Node &node,
                                   MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Pass a Node to set adios2 i/o options.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_set_options(const Node &opts,
                                          MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Get a Node that contains adios2 i/o options.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API adios2_options(Node &opts,
                                      MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Get a number of steps.
//-----------------------------------------------------------------------------
int  CONDUIT_RELAY_API adios2_query_number_of_steps(const std::string &path,
                                                    MPI_Comm comm);

//-----------------------------------------------------------------------------
/// Get a number of domains.
//-----------------------------------------------------------------------------
int  CONDUIT_RELAY_API adios2_query_number_of_domains(const std::string &path,
                                                      MPI_Comm comm);

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi::io --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
