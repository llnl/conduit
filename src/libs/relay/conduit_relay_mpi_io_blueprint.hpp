// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_blueprint.hpp
///
//-----------------------------------------------------------------------------

// NOTE: This functionality is a placeholder for more general functionality in
// future versions of Blueprint. That said, the functions in this header are
// subject to change and could be moved with any future iteration of Conduit,
// so use this header with caution!

#ifndef CONDUIT_RELAY_IO_MPI_BLUEPRINT_HPP
#define CONDUIT_RELAY_IO_MPI_BLUEPRINT_HPP

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

//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi::io::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

bool CONDUIT_RELAY_API clean_mesh(const conduit::Node &data,
                                  conduit::Node &output,
                                  MPI_Comm comm);

void CONDUIT_RELAY_API gen_domain_to_file_map(index_t num_domains,
                                              index_t num_files,
                                              Node &out);

//-----------------------------------------------------------------------------
// Save a blueprint mesh to root + file set
//-----------------------------------------------------------------------------
/// Note: These methods use "save" semantics, they will overwrite existing
///       files. 
///
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 const std::string &protocol,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none" 
///            when # of domains == 1,  "default"   ==> "none"
///            else,                    "default"   ==> "cycle"
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      number_of_files:  {# of files}
///            when "multi_file":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 const std::string &protocol,
                                 const conduit::Node &opts,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
// Save a blueprint mesh to root + file set
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// Note: These methods use "write" semantics, they will append to existing
///       files. 
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  MPI_Comm comm);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  const std::string &protocol,
                                  MPI_Comm comm);


//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none" 
///            when # of domains == 1,  "default"   ==> "none"
///            else,                    "default"   ==> "cycle"
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      number_of_files:  {# of files}
///            when "multi_file":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
///      truncate: "false", "true" (used if present, default ==> "false")
///           when "true" overwrites existing files (relay 'save' semantics)
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  const std::string &protocol,
                                  const conduit::Node &opts,
                                  MPI_Comm comm);

// -----------------------------------------------------------
// get the cycle info
// -----------------------------------------------------------
void CONDUIT_RELAY_API determine_cycle_and_resolve_suffix(const Node &multi_dom,
                                                          const std::string &path,
                                                          index_t &cycle,
                                                          std::string &opts_suffix,
                                                          bool gen_name,
                                                          MPI_Comm comm);

//-----------------------------------------------------------------------------
// Generate root file name given parameters
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::string CONDUIT_RELAY_API generate_root_filename(const conduit::Node &mesh,
                                                     const std::string &path,
                                                     MPI_Comm comm);

//-----------------------------------------------------------------------------
std::string CONDUIT_RELAY_API generate_root_filename(const conduit::Node &mesh,
                                                     const std::string &path,
                                                     const std::string &protocol,
                                                     MPI_Comm comm);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      suffix: "default", "cycle", "none" 
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
//-----------------------------------------------------------------------------
std::string CONDUIT_RELAY_API generate_root_filename(const conduit::Node &mesh,
                                                     const std::string &path,
                                                     const std::string &protocol,
                                                     const conduit::Node &opts,
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
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where bp data includes
///           more than one mesh.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API read_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);


//-----------------------------------------------------------------------------
///
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where bp data includes
///           more than one mesh.
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh,
                                 MPI_Comm comm);

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi::io::blueprint --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi::io --
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// -- begin conduit::relay::mpi::io_blueprint -- (DEPRECATED!)
//-----------------------------------------------------------------------------
namespace io_blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
// DEPRECATED FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save(const conduit::Node &mesh,
                            const std::string &path,
                            MPI_Comm comm);

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save(const conduit::Node &mesh,
                            const std::string &path,
                            const std::string &protocol,
                            MPI_Comm comm);

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::mpi::io_blueprint --
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
