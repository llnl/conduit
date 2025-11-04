// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_io_silo.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_IO_SILO_HPP
#define CONDUIT_RELAY_IO_SILO_HPP

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
// -- begin conduit::relay::io --
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
                                  const std::string &path);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      silo_type: "default", "pdb", "hdf5", "unknown"
///            when the file we are writing to exists, "default" ==> "unknown"
///            else,                                   "default" ==> "hdf5"
///         note: these are additional silo_type options that we could add
///         support for in the future:
///           "hdf5_sec2", "hdf5_stdio", "hdf5_mpio", "hdf5_mpiposix", "taurus"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      ovl_topo_name: (used if present, default ==> "")
///
///      nameschemes: "default", "yes", "no"
///            "default" ==> "no"
///
///      unified_types: "default", "yes", "no"
///            "default" ==> "yes"
///            prefer single mesh/var types versus writing an entire array
///            of types. "yes" will prefer this if possible, "no" will 
///            always write the entire array.
///
///      number_of_files:  {# of files}
///            when "multi_file" or "overlink":
///                 <= 0, use # of files == # of domains
///                  > 0, # of files == number_of_files
///
/// Note:
///  In the non-overlink case...
///   1) We have made the choice to output ALL topologies as multimeshes.
///   2) We prepend the provided mesh_name to each of these topo names. We do
///      this to avoid a name collision in the root only + single domain case.
///      We do this across all cases for the sake of consistency. We also use
///      the mesh_name as the name of the silo directory within each silo file
///      where data is stored.
///   3) ovl_topo_name is ignored if provided.
///  In the overlink case...
///   1) We have made the choice to output only ONE topology as a multimesh.
///   2) mesh_name is ignored if provided and changed to "MMESH"
///   3) ovl_topo_name is the name of the topo we are outputting. If it is not
///      provided, we choose the first topology in the blueprint.
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API write_mesh(const conduit::Node &mesh,
                                  const std::string &path,
                                  const conduit::Node &opts);

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
                                 const std::string &path);

//-----------------------------------------------------------------------------
/// The following options can be passed via the opts Node:
//-----------------------------------------------------------------------------
/// opts:
///
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      silo_type: "default", "pdb", "hdf5", "unknown"
///            when the file we are writing to exists, "default" ==> "unknown"
///            else,                                   "default" ==> "hdf5"
///         note: these are additional silo_type options that we could add
///         support for in the future:
///           "hdf5_sec2", "hdf5_stdio", "hdf5_mpio", "hdf5_mpiposix", "taurus"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      ovl_topo_name: (used if present, default ==> "")
///
///      nameschemes: "default", "yes", "no"
///            "default" ==> "no"
///
///      unified_types: "default", "yes", "no"
///            "default" ==> "yes"
///            prefer single mesh/var types versus writing an entire array
///            of types. "yes" will prefer this if possible, "no" will 
///            always write the entire array.
///
///      number_of_files:  {# of files}
///            when "multi_file" or "overlink":
///                 <= 0, use # of files ==> # of domains
///                  > 0, # of files ==> number_of_files
///
/// Note:
///  In the non-overlink case...
///   1) We have made the choice to output ALL topologies as multimeshes.
///   2) We prepend the provided mesh_name to each of these topo names. We do
///      this to avoid a name collision in the root only + single domain case.
///      We do this across all cases for the sake of consistency. We also use
///      the mesh_name as the name of the silo directory within each silo file
///      where data is stored.
///   3) ovl_topo_name is ignored if provided.
///  In the overlink case...
///   1) We have made the choice to output only one topology as a multimesh.
///   2) mesh_name is ignored if provided and changed to "MMESH"
///   3) ovl_topo_name is the name of the topo we are outputting. If it is not
///      provided, we choose the first topology in the blueprint.
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API save_mesh(const conduit::Node &mesh,
                                 const std::string &path,
                                 const conduit::Node &opts);

//-----------------------------------------------------------------------------
// The load semantics, the mesh node is reset before reading.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 conduit::Node &mesh);


//-----------------------------------------------------------------------------
///
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where silo data includes
///          more than one mesh.
///          We only allow reading of a single mesh to keep these options on
///          par with the relay io blueprint options.
///
///      matset_style: "default", "multi_buffer_full", "sparse_by_element",
///            "multi_buffer_by_material"
///            "default"   ==> "sparse_by_element"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API load_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh);

//-----------------------------------------------------------------------------
// Load a blueprint mesh from root + file set
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API read_mesh(const std::string &root_file_path,
                                 conduit::Node &mesh);


//-----------------------------------------------------------------------------
///
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where silo data includes
///          more than one mesh.
///          We only allow reading of a single mesh to keep these options on
///          par with the relay io blueprint options.
///
///      matset_style: "default", "multi_buffer_full", "sparse_by_element",
///            "multi_buffer_by_material"
///            "default"   ==> "sparse_by_element"
///
//-----------------------------------------------------------------------------
void CONDUIT_RELAY_API read_mesh(const std::string &root_file_path,
                                 const conduit::Node &opts,
                                 conduit::Node &mesh);


}
//-----------------------------------------------------------------------------
// -- end <>::silo --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
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

