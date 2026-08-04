// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi_io_silo.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_MPI_IO_SILO_HPP
#define CONDUIT_RELAY_MPI_IO_SILO_HPP

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include <silo.h>

//-----------------------------------------------------------------------------
// conduit lib include 
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_relay_exports.h"
#include "conduit_relay_config.h"

#include <mpi.h>

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

// Functions are provided by this include file.
#include "conduit_relay_io_silo_api.hpp"

//-----------------------------------------------------------------------------
// -- begin <>::silo --
//-----------------------------------------------------------------------------
namespace silo
{

//-----------------------------------------------------------------------------
// Write a blueprint mesh to silo
//-----------------------------------------------------------------------------
/// These methods assume `mesh` is a valid blueprint mesh.
///
/// Note: These methods use "write" semantics, they will append to existing
///       files.
///
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  MPI_Comm comm);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      TODO
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  const conduit::Node &opts,
                                  MPI_Comm comm);

//-----------------------------------------------------------------------------
// Save a blueprint mesh to silo
//-----------------------------------------------------------------------------
/// These methods assume `mesh` is a valid blueprint mesh.
///
/// Note: These methods use "save" semantics, they will overwrite existing
///       files.
///
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      TODO
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 const conduit::Node &opts,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
// The load semantics, the mesh node is reset before reading.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);


//-----------------------------------------------------------------------------
///
/// opts:
///      TODO
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
// Load a blueprint mesh from root + file set
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API read_mesh(const std::string &root_file_path,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);


//-----------------------------------------------------------------------------
///
/// opts:
///      TODO
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API read_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
///
/// Added to header for testing purposes; not meant to be used publicly
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API
honor_material_dependent_fields(const int domain_start,
                                const int domain_end,
                                Node &mesh,
                                MPI_Comm mpi_comm);


}
//-----------------------------------------------------------------------------
// -- end <>::silo --
//-----------------------------------------------------------------------------

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

