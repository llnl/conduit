// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.
//-----------------------------------------------------------------------------
///
/// file: conduit_relay_mpi_io_cgns.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_MPI_IO_CGNS_HPP
#define CONDUIT_RELAY_MPI_IO_CGNS_HPP

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
// #include <cgns.h>

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
#include "conduit_relay_io_cgns_api.hpp"

//-----------------------------------------------------------------------------
// -- begin <>::cgns --
//-----------------------------------------------------------------------------
namespace cgns
{

//-----------------------------------------------------------------------------
// Write a blueprint mesh to cgns
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
// Save a blueprint mesh to cgns
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


}
//-----------------------------------------------------------------------------
// -- end <>::cgns --
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

