// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mpi_mesh.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MPI_MESH_HPP
#define CONDUIT_BLUEPRINT_MPI_MESH_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_exports.h"
#include "conduit_blueprint_mesh.hpp"

#include <mpi.h>

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi --
//-----------------------------------------------------------------------------
namespace mpi
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi::mesh --
//-----------------------------------------------------------------------------

namespace mesh
{

//-----------------------------------------------------------------------------
/// blueprint protocol verify interface
//-----------------------------------------------------------------------------

// mesh verify
//-----------------------------------------------------------------------------
bool CONDUIT_BLUEPRINT_API verify(const conduit::Node &n,
                                  conduit::Node &info,
                                  MPI_Comm comm);

//-----------------------------------------------------------------------------
/// blueprint mesh property methods
///
/// These methods can be called on any verified blueprint mesh.
//-----------------------------------------------------------------------------

void CONDUIT_BLUEPRINT_API generate_index(const conduit::Node &mesh,
                                          const std::string &ref_path,
                                          Node &index_out,
                                          MPI_Comm comm);

//
//  note: the to_poly methods require a structured grid 
//        with an adjset
//

void CONDUIT_BLUEPRINT_API to_polygonal(const conduit::Node &n,
                                        conduit::Node &dest,
                                        const std::string& name,
                                        MPI_Comm comm);

void CONDUIT_BLUEPRINT_API to_polyhedral(const conduit::Node &n,
                                         conduit::Node &dest,
                                         const std::string& name,
                                         MPI_Comm comm);

void CONDUIT_BLUEPRINT_API to_polytopal(const conduit::Node &n,
                                        conduit::Node &dest,
                                        const std::string& name,
                                        MPI_Comm comm);


//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_domain_to_rank_map(
                                              const conduit::Node &mesh,
                                              Node &domain_to_rank_map,
                                              MPI_Comm comm);

//-------------------------------------------------------------------------
index_t CONDUIT_BLUEPRINT_API number_of_domains(const conduit::Node &mesh,
                                                MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API state(const conduit::Node &mesh,
                                 Node &state,
                                 MPI_Comm comm);

//-------------------------------------------------------------------------
index_t CONDUIT_BLUEPRINT_API cycle(const conduit::Node &mesh,
                                    MPI_Comm comm);

//-------------------------------------------------------------------------
float64 CONDUIT_BLUEPRINT_API time(const conduit::Node &mesh,
                                   MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API partition(const conduit::Node &mesh,
                                     const conduit::Node &options,
                                     conduit::Node &output,
                                     MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API partition_map_back(const conduit::Node& repart_mesh,
                                              const conduit::Node& options,
                                              conduit::Node& orig_mesh,
                                              MPI_Comm comm);




///@name blueprint::mpi::mesh::distribute(...)
//-----------------------------------------------------------------------------
/// description:
///   distribute(...) Allows you to send input domains to arbitrary MPI ranks.
///   Domain Overloading is one key use case.
///
///  --------------
///  Example:
///  --------------
///
///   Input -- 4 domains, one per mpi task
///    task 0: [domain 0]
///    task 1: [domain 1]
///    task 2: [domain 2]
///    task 3: [domain 3]
///
///   Goal: Duplicate each domain on two mpi tasks to obtain the following:
///   Output:
///    task 0: [domain 0, domain 3]
///    task 1: [domain 0, domain 1]
///    task 2: [domain 1, domain 2]
///    task 3: [domain 2, domain 3]
///
///
///  Node input; /// setup input mesh ...
///  Node opts, output;
///  opts["domain_map/values"] = { 0,1,  1,2,  2,3,  3,0};
///  // we have two dest ranks for for each domain
///  opts["domain_map/sizes"] = { 2, 2, 2, 2};
///  opts["domain_map/offsets"] = { 0, 2, 4, 6};
///  blueprint::mpi::mesh::distribute(input,opts,output,comm);
//-----------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API distribute(const conduit::Node &mesh,
                                      const conduit::Node &options,
                                      conduit::Node &output,
                                      MPI_Comm comm);


///@name blueprint::mpi::mesh::find_delegate_domain(...)
///@{
//-----------------------------------------------------------------------------
/// description:
///   find_delegate_domain(...) uses cross-rank MPI communication to find a
//    "delegate" domain that can be used to represent the mesh's schema across
//    all ranks. This function is most useful in cases where all ranks need some
//    mesh information to bootstrap local data structures, but one or more ranks
//    have empty mesh definitions.
//-----------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API find_delegate_domain(const conduit::Node &mesh,
                                                conduit::Node &domain,
                                                MPI_Comm comm);

//-----------------------------------------------------------------------------
/// blueprint mesh transform methods
///
/// These methods can be called on specific verified blueprint mesh.
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_points(conduit::Node &mesh,
                                           const std::string& src_adjset_name,
                                           const std::string& dst_adjset_name,
                                           const std::string& dst_topo_name,
                                           conduit::Node& s2dmap,
                                           conduit::Node& d2smap,
                                           MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_lines(conduit::Node &mesh,
                                          const std::string& src_adjset_name,
                                          const std::string& dst_adjset_name,
                                          const std::string& dst_topo_name,
                                          conduit::Node& s2dmap,
                                          conduit::Node& d2smap,
                                          MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_faces(conduit::Node &mesh,
                                          const std::string& src_adjset_name,
                                          const std::string& dst_adjset_name,
                                          const std::string& dst_topo_name,
                                          conduit::Node& s2dmap,
                                          conduit::Node& d2smap,
                                          MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_centroids(conduit::Node& mesh,
                                              const std::string& src_adjset_name,
                                              const std::string& dst_adjset_name,
                                              const std::string& dst_topo_name,
                                              const std::string& dst_cset_name,
                                              conduit::Node& s2dmap,
                                              conduit::Node& d2smap,
                                              MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_sides(conduit::Node& mesh,
                                          const std::string& src_adjset_name,
                                          const std::string& dst_adjset_name,
                                          const std::string& dst_topo_name,
                                          const std::string& dst_cset_name,
                                          conduit::Node& s2dmap,
                                          conduit::Node& d2smap,
                                          MPI_Comm comm);

//-------------------------------------------------------------------------
void CONDUIT_BLUEPRINT_API generate_corners(conduit::Node& mesh,
                                            const std::string& src_adjset_name,
                                            const std::string& dst_adjset_name,
                                            const std::string& dst_topo_name,
                                            const std::string& dst_cset_name,
                                            conduit::Node& s2dmap,
                                            conduit::Node& d2smap,
                                            MPI_Comm comm);

//-------------------------------------------------------------------------
/**
 @brief Performs the mesh::flatten() operation across all ranks in comm.
    The resulting table will be gathered to rank 0.
 @param mesh    A Conduit node containing a blueprint mesh or set of mesh domains.
 @param options A Conduit node containing options for the flatten operation.
 @param[out] output A Conduit node that will contain the blueprint table output.
 @param comm The MPI communicator to be used.

Supports the same options as serial flatten() plus:
    "add_rank": Includes the rank number as a column in the output table.
        (Default = 0 (false))
    "root": The rank that will contain the output table for all ranks in comm.
        (Default = 0)
*/
void CONDUIT_BLUEPRINT_API flatten(const conduit::Node &mesh,
                                   const conduit::Node &options,
                                   conduit::Node &output,
                                   MPI_Comm comm);

void CONDUIT_BLUEPRINT_API generate_domain_ids(conduit::Node &domains,
                                               MPI_Comm mpi_comm);

//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif 



