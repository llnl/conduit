// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mpi_mesh_parmetis.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_blueprint_mpi_mesh_parmetis.hpp"
#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_o2mrelation_iterator.hpp"
#include "conduit_blueprint_mesh_utils_iterate_elements.hpp"

#include "conduit_relay_mpi.hpp"

#include <parmetis.h>

#include <algorithm>
#include <unordered_map>

using namespace conduit::blueprint::mesh::utils;

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
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------

namespace mesh 
{
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-- Map Parmetis Types (idx_t and real_t) to conduit dtype ids 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// check our assumptions
static_assert(IDXTYPEWIDTH != 32 || IDXTYPEWIDTH != 64,
              "Metis idx_t is not 32 or 64 bits");

static_assert(REALTYPEWIDTH != 32 || REALTYPEWIDTH != 64,
              "Metis real_t is not 32 or 64 bits");

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// IDXTYPEWIDTH and REALTYPEWIDTH are metis type defs
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
index_t
metis_idx_t_to_conduit_dtype_id()
{
#if IDXTYPEWIDTH == 64
// 64 bits
// int64
    return conduit::DataType::INT64_ID;
#else
// 32 bits
// int32
    return conduit::DataType::INT32_ID;
#endif
}

//-----------------------------------------------------------------------------
index_t
metis_real_t_t_to_conduit_dtype_id()
{
#if REALTYPEWIDTH == 64
// 64 bits
// float64
    return conduit::DataType::FLOAT64_ID;
#else
// 32 bits
// float32
    return conduit::DataType::FLOAT32_ID;
#endif
}


//-----------------------------------------------------------------------------
// NOTE: this is generally useful, it should be added to mpi::mesh
//
// supported options:
//   topology: {string}
//   field_prefix: {string}
void generate_global_element_and_vertex_ids(conduit::Node &mesh,
                                            const Node &options,
                                            MPI_Comm comm)
{
    // TODO: Check of dest fields already exist, if they do error
    
    
    int par_rank = conduit::relay::mpi::rank(comm);
    int par_size = conduit::relay::mpi::size(comm);

    index_t local_num_doms = ::conduit::blueprint::mesh::number_of_domains(mesh);
    index_t global_num_doms = number_of_domains(mesh,comm);

    if(global_num_doms == 0)
    {
        return;
    }

    std::vector<Node*> domains;
    ::conduit::blueprint::mesh::domains(mesh,domains);
    
    // parse options
    std::string topo_name = "";
    std::string field_prefix = "";
    std::string adjset_name = "";
    if( options.has_child("topology") )
    {
        topo_name = options["topology"].as_string();
    }
    else if(local_num_doms > 0)
    {
        // TOOD: IMP find the first topo name on a rank with data
        // for now, just grab first topo
        const Node &dom_topo = domains[0]->fetch("topologies")[0];
        topo_name = dom_topo.name();
    }

    if( options.has_child("field_prefix") )
    {
        field_prefix = options["field_prefix"].as_string();
    }

    if( options.has_child("adjset") )
    {
        adjset_name = options["adjset"].as_string();
    }

    // count all local elements + verts and create offsets
    uint64 local_total_num_eles=0;
    uint64 local_total_num_verts=0;

    Node local_info;
    // per domain local info books
    local_info["num_verts"].set(DataType::uint64(local_num_doms));
    local_info["num_eles"].set(DataType::uint64(local_num_doms));
    local_info["verts_offsets"].set(DataType::uint64(local_num_doms));
    local_info["eles_offsets"].set(DataType::uint64(local_num_doms));

    uint64_array local_num_verts = local_info["num_verts"].value();
    uint64_array local_num_eles  = local_info["num_eles"].value();

    uint64_array local_vert_offsets = local_info["verts_offsets"].value();
    uint64_array local_ele_offsets  = local_info["eles_offsets"].value();

    local_info["num_verts_primary"].set(DataType::uint64(local_num_doms));
    uint64_array local_num_verts_pri = local_info["num_verts_primary"].value();
    std::vector<std::unordered_map<uint64, int64>> dom_shared_nodes(domains.size());

    int64 local_ndomains = static_cast<int64>(domains.size());
    int64 global_ndomains;
    MPI_Allreduce(&local_ndomains, &global_ndomains, 1,
                  MPI_INT64_T, MPI_SUM, comm);

    // A map of global domain IDs to their rank.
    std::vector<int64> dom_locs(global_ndomains, INT64_MAX);
    // A map of local domain IDs to global domain IDs.
    std::vector<int64> global_domids(domains.size(), -1);

    for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
    {
        Node &dom = *domains[local_dom_idx];
        // we do need to make sure we have the requested topo
        if(dom["topologies"].has_child(topo_name))
        {
            // get the topo node
            const Node &dom_topo = dom["topologies"][topo_name];
            // get the number of elements in the topo
            local_num_eles[local_dom_idx] = blueprint::mesh::utils::topology::length(dom_topo);
            local_ele_offsets[local_dom_idx] = local_total_num_eles;
            local_total_num_eles += local_num_eles[local_dom_idx];

            // get the coordset that the topo refs
            const Node &dom_cset = dom["coordsets"][dom_topo["coordset"].as_string()];
            // get the number of points in the coordset
            // THIS RETURNS ZERO:
            //local_num_verts[local_dom_idx] = blueprint::mesh::utils::coordset::length(dom_cset);
            // so we are using this: 
            local_num_verts[local_dom_idx] = dom_cset["values/x"].dtype().number_of_elements();
            local_num_verts_pri[local_dom_idx] = local_num_verts[local_dom_idx];
        }
        if (adjset_name != "" && dom.has_child("adjsets"))
        {
            if (!dom["adjsets"].has_child(adjset_name))
            {
                CONDUIT_ERROR("Specified adjset = \"" << adjset_name
                                << "\" was not found in adjsets node");
            }
            // Set up global domain ID maps
            uint64 global_domid = dom["state/domain_id"].to_uint64();
            dom_locs[global_domid] = par_rank;
            global_domids[local_dom_idx] = global_domid;

            std::unordered_map<uint64, int64>& shared_nodes = dom_shared_nodes[local_dom_idx];
            const Node& dom_aset = dom["adjsets"][adjset_name];
            std::string assoc_type = dom_aset["association"].as_string();
            std::string assoc_topo = dom_aset["topology"].as_string();
            if (assoc_type != "vertex")
            {
                CONDUIT_ERROR("Specified adjset \"" << adjset_name << "\" is "
                              << "not a vertex-associated adjset. Element-"
                              << "associated adjsets are not supported at this time.");

            }
            if (assoc_topo != topo_name)
            {
                CONDUIT_ERROR("Specified adjset \"" << adjset_name
                                << "\" associated with unexpected topology \"" << assoc_topo
                                << "\" (per generate_partition_field options: topology = \""
                                << topo_name << "\")");
            }

            for (const Node& group : dom_aset["groups"].children())
            {
                uint64_accessor nbr_doms = group["neighbors"].as_uint64_accessor();
                uint64 min_domain = global_domid;
                for (index_t inbr = 0; inbr < nbr_doms.number_of_elements(); inbr++)
                {
                    min_domain = std::min(min_domain, nbr_doms[inbr]);
                }
                // Use the lower-indexed domain as the primary domain for these vertices
                uint64_accessor group_verts = group["values"].as_uint64_accessor();
                for (index_t ivert = 0; ivert < group_verts.number_of_elements(); ivert++)
                {
                    shared_nodes[group_verts[ivert]] = (min_domain == global_domid
                                                        ? -1
                                                        : min_domain);
                }
            }
            // Count the number of nodes that are shared and non-primary
            uint64 n_shared_nodes = 0;
            for (const auto& shared_node_ent : shared_nodes)
            {
                if (shared_node_ent.second != -1) { n_shared_nodes++; }
            }
            local_num_verts_pri[local_dom_idx] -= n_shared_nodes;
        }
        // Calculate offsets based on primary vertices in each domain
        local_vert_offsets[local_dom_idx] = local_total_num_verts;
        local_total_num_verts += local_num_verts_pri[local_dom_idx];
    }

    // Reduce to get locations of all domains.
    MPI_Allreduce(MPI_IN_PLACE, dom_locs.data(), dom_locs.size(),
                  MPI_INT64_T, MPI_MIN, comm);

    // calc per MPI task offsets using 
    // local_total_num_verts
    // local_total_num_eles

    // first count verts
    Node max_local, max_global;
    max_local.set(DataType::uint64(par_size));
    max_global.set(DataType::uint64(par_size));

    uint64_array max_local_vals = max_local.value();
    uint64_array max_global_vals = max_global.value();

    max_local_vals[par_rank] = local_total_num_verts;

    relay::mpi::max_all_reduce(max_local, max_global, comm);


    index_t global_verts_offset = 0;
    for(index_t i=0; i< par_rank; i++ )
    {
        global_verts_offset += max_global_vals[i];
    }
    
    // reset our buffers 
    for(index_t i=0; i< par_size; i++ )
    {
        max_local_vals[i]  = 0;
        max_global_vals[i] = 0;
    }

    // now count eles
    max_local_vals[par_rank] = local_total_num_eles;

    relay::mpi::max_all_reduce(max_local, max_global, comm);

    index_t global_eles_offset = 0;
    for(index_t i=0; i< par_rank; i++ )
    {
        global_eles_offset += max_global_vals[i];
    }

    // we now have our offsets, we can create output fields on each local domain
    for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
    {
        Node &dom = *domains[local_dom_idx];
        // we do need to make sure we have the requested topo
        if(dom["topologies"].has_child(topo_name))
        {
            Node &verts_field = dom["fields"][field_prefix + "global_vertex_ids"];
            verts_field["association"] = "vertex";
            verts_field["topology"] = topo_name;
            verts_field["values"].set(DataType::int64(local_num_verts[local_dom_idx]));

            int64 vert_base_idx = global_verts_offset + local_vert_offsets[local_dom_idx];

            int64_array vert_ids_vals = verts_field["values"].value();
            int64 curr_vert_id = 0;
            for(uint64 i=0; i < local_num_verts[local_dom_idx]; i++)
            {
                bool is_primary_domain = true;
                int primary_domid = 0;
                if (dom_shared_nodes[local_dom_idx].count(i) > 0)
                {
                    primary_domid = dom_shared_nodes[local_dom_idx][i];
                    is_primary_domain = (primary_domid == -1);
                }

                if (!is_primary_domain)
                {
                    // mark the node with the domain we need to fetch vids from
                    vert_ids_vals[i] = ~primary_domid;
                }
                else
                {
                    // number a node for which we are the primary domain
                    vert_ids_vals[i] = curr_vert_id + vert_base_idx;
                    curr_vert_id++;
                }
            }

            // NOTE: VISIT BP DOESNT SUPPORT UINT64!!!!
            Node &eles_field = dom["fields"][field_prefix + "global_element_ids"];
            eles_field["association"] = "element";
            eles_field["topology"] = topo_name;
            eles_field["values"].set(DataType::int64(local_num_eles[local_dom_idx]));

            int64 ele_base_idx = global_eles_offset + local_ele_offsets[local_dom_idx];

            int64_array ele_ids_vals = eles_field["values"].value();
            for(uint64 i=0; i < local_num_eles[local_dom_idx]; i++)
            {
               ele_ids_vals[i] = i + ele_base_idx;
            }
        }
    }

    if (adjset_name != "")
    {
        const int TAG_SHARED_NODE_SYNC = 175;
        // map of groups -> global vtx ids
        std::map<std::set<uint64>, std::vector<uint64>> groups_2_vids;
        // map of rank -> sends to/recvs from that rank of global vtx ids for
        // an adjset group
        std::unordered_map<uint64, std::vector<std::set<uint64>>> pending_sends, pending_recvs;

        // 1. First iterate through our local domains to prepare global vtx id
        //    lists that we control
        for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
        {
            Node &dom = *domains[local_dom_idx];
            int64 global_domid = global_domids[local_dom_idx];
            Node &verts_field = dom["fields"][field_prefix + "global_vertex_ids"];
            int64_array vert_ids_vals = verts_field["values"].value();
            const Node& dom_aset = dom["adjsets"][adjset_name];

            for (const Node& group : dom_aset["groups"].children())
            {
                uint64_accessor nbr_doms = group["neighbors"].as_uint64_accessor();
                uint64 min_domain = global_domid;
                std::set<uint64> sorted_nbrs;
                sorted_nbrs.insert(global_domid);
                for (index_t inbr = 0; inbr < nbr_doms.number_of_elements(); inbr++)
                {
                    min_domain = std::min(min_domain, nbr_doms[inbr]);
                    sorted_nbrs.insert(nbr_doms[inbr]);
                }

                uint64_accessor group_verts = group["values"].as_uint64_accessor();
                if (static_cast<int64>(min_domain) == global_domid)
                {
                    // This domain provides the actual vids
                    std::vector<uint64> actual_vids(group_verts.number_of_elements());
                    for (index_t ivert = 0; ivert < group_verts.number_of_elements(); ivert++)
                    {
                        actual_vids[ivert] = vert_ids_vals[group_verts[ivert]];
                    }
                    if (groups_2_vids.count(sorted_nbrs) > 0)
                    {
                        CONDUIT_ERROR("Multiple primary domains?");
                    }
                    groups_2_vids[sorted_nbrs] = std::move(actual_vids);

                    // If any neighbor domains are off-rank, we need to send
                    // our global vids to those ranks.
                    for (uint64 nbr_dom : sorted_nbrs)
                    {
                        if (static_cast<int64>(nbr_dom) == global_domid)
                        {
                            // skip source domain
                            continue;
                        }
                        const uint64 dst_rank = dom_locs[nbr_dom];
                        if (static_cast<uint64>(par_rank) != dst_rank)
                        {
                            pending_sends[dst_rank].push_back(sorted_nbrs);
                        }
                    }
                }
                else
                {
                    // We need primary domain data, which might not yet exist
                    // on this rank. Prepare irecv if necessary.
                    const uint64 src_rank = dom_locs[min_domain];

                    if (static_cast<uint64>(par_rank) != src_rank)
                    {
                        pending_recvs[src_rank].push_back(sorted_nbrs);
                        groups_2_vids[sorted_nbrs].resize(group_verts.number_of_elements());
                    }
                }
            }
        }

        // 2. Do required communication asynchronously.
        std::vector<MPI_Request> async_sends, async_recvs;
        for (auto& it : pending_recvs)
        {
            const uint64 rank_from = it.first;
            std::vector<std::set<uint64>>& recv_groups = it.second;
            // Sort the groups to receive first. This gives us a consistent
            // ordering of isends/irecvs
            std::sort(recv_groups.begin(), recv_groups.end());

            int group_idx = 0;
            for (const std::set<uint64>& group : recv_groups)
            {
                index_t domid = *(group.begin());
                const int tag = conduit::relay::mpi::safe_tag(TAG_SHARED_NODE_SYNC + domid * 100 + group_idx, comm);
                async_recvs.push_back(MPI_Request{});
                group_idx++;
                std::vector<uint64>& recvbuf = groups_2_vids[group];
                MPI_Irecv(recvbuf.data(), recvbuf.size(), MPI_UINT64_T,
                          rank_from, tag, comm, &(*async_recvs.rbegin()));
            }
        }
        for (auto& it : pending_sends)
        {
            const uint64 rank_to = it.first;
            std::vector<std::set<uint64>>& send_groups = it.second;
            // Sort the groups to send first. This gives us a consistent
            // ordering of isends/irecvs
            std::sort(send_groups.begin(), send_groups.end());

            int group_idx = 0;
            for (const std::set<uint64>& group : send_groups)
            {
                index_t domid = *(group.begin());
                const int tag = conduit::relay::mpi::safe_tag(TAG_SHARED_NODE_SYNC + domid * 100 + group_idx, comm);
                async_sends.push_back(MPI_Request{});
                group_idx++;
                std::vector<uint64>& sendbuf = groups_2_vids[group];
                MPI_Isend(sendbuf.data(), sendbuf.size(), MPI_UINT64_T,
                          rank_to, tag, comm, &(*async_sends.rbegin()));
            }
        }
        std::vector<MPI_Status> async_recv_statuses(async_recvs.size());
        // Make sure all our irecvs have completed
        MPI_Waitall(async_recvs.size(), async_recvs.data(), async_recv_statuses.data());

        // 3. Finally, iterate through our local domains to remap any vertices
        //    that have been numbered by another domain.
        for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
        {
            Node &dom = *domains[local_dom_idx];
            int64 global_domid = global_domids[local_dom_idx];
            Node &verts_field = dom["fields"][field_prefix + "global_vertex_ids"];
            int64_array vert_ids_vals = verts_field["values"].value();
            const Node& dom_aset = dom["adjsets"][adjset_name];

            for (const Node& group : dom_aset["groups"].children())
            {
                uint64_accessor nbr_doms = group["neighbors"].as_uint64_accessor();
                uint64 min_domain = global_domid;
                std::set<uint64> sorted_nbrs;
                sorted_nbrs.insert(global_domid);
                for (index_t inbr = 0; inbr < nbr_doms.number_of_elements(); inbr++)
                {
                    min_domain = std::min(min_domain, nbr_doms[inbr]);
                    sorted_nbrs.insert(nbr_doms[inbr]);
                }

                uint64_accessor group_verts = group["values"].as_uint64_accessor();
                if ( static_cast<int64>(min_domain) != global_domid)
                {
                    // Remap higher-numbered domains with primary domain's
                    // assigned global vtx ids
                    const std::vector<uint64>& actual_vids = groups_2_vids[sorted_nbrs];
                    if (static_cast<index_t>(actual_vids.size()) != group_verts.number_of_elements())
                    {
                        CONDUIT_ERROR("mismatch in shared verts");
                    }
                    for (index_t ivert = 0; ivert < static_cast<index_t>(actual_vids.size()); ivert++)
                    {
                        vert_ids_vals[group_verts[ivert]] = actual_vids[ivert];
                    }
                }
            }
        }
        std::vector<MPI_Status> async_send_statuses(async_sends.size());
        // Make sure all our isends have completed
        MPI_Waitall(async_sends.size(), async_sends.data(), async_send_statuses.data());
    }
}

//-----------------------------------------------------------------------------
void generate_partition_field(conduit::Node &mesh,
                              MPI_Comm comm)
{
    Node opts;
    generate_partition_field(mesh,opts,comm);
}

//-----------------------------------------------------------------------------
void generate_partition_field(conduit::Node &mesh,
                              const conduit::Node &options,
                              MPI_Comm comm)
{
    generate_global_element_and_vertex_ids(mesh,
                                           options,
                                           comm);

    int par_rank = conduit::relay::mpi::rank(comm);
    int par_size = conduit::relay::mpi::size(comm);

    index_t global_num_doms = number_of_domains(mesh,comm);


    if(global_num_doms == 0)
    {
        return;
    }

    std::vector<Node*> domains;
    ::conduit::blueprint::mesh::domains(mesh,domains);

    // parse options
    std::string topo_name = "";
    std::string field_prefix = "";
    idx_t nparts = (idx_t)global_num_doms;

    if( options.has_child("topology") )
    {
        topo_name = options["topology"].as_string();
    }
    else if(domains.size() > 0 )
    {
        // TOOD: IMP find the first topo name on a rank with data
        // for now, just grab first topo
        const Node &dom_topo = domains[0]->fetch("topologies")[0];
        topo_name = dom_topo.name();
    }
    idx_t ncommonnodes;
    if ( options.has_child("parmetis_ncommonnodes") )
    {
        ncommonnodes = options["parmetis_ncommonnodes"].as_int();
    }
    else if(domains.size() > 0 )
    {
        // in 2D, zones adjacent if they share 2 nodes (edge)
        // in 3D, zones adjacent if they share 3 nodes (plane)
        std::string coordset_name
            = domains[0]->fetch(std::string("topologies/") + topo_name + "/coordset").as_string();
        const Node& coordset = domains[0]->fetch(std::string("coordsets/") + coordset_name);
        ncommonnodes = conduit::blueprint::mesh::coordset::dims(coordset);
    }

    if( options.has_child("field_prefix") )
    {
        field_prefix = options["field_prefix"].as_string();
    }

    if( options.has_child("partitions") )
    {
        nparts = (idx_t) options["partitions"].to_int64();
    }
    // TODO: Should this be an error or use default (discuss more)?
    // else
    // {
    //     CONDUIT_ERROR("Missing required option in generate_partition_field(): "
    //                   << "expected \"partitions\" field with number of partitions.");
    // }

    // we now have global element and vertex ids
    // we just need to do some counting and then 
    //  traverse our topo to convert this info to parmetis input

    // we need the total number of local eles
    // the total number of element to vers entries


    index_t local_total_num_eles =0;
    index_t local_total_ele_to_verts_size = 0;
    
    for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
    {
        Node &dom = *domains[local_dom_idx];
        // we do need to make sure we have the requested topo
        if(dom["topologies"].has_child(topo_name))
        {
            // get the topo node
            const Node &dom_topo = dom["topologies"][topo_name];
            // get the number of elements in the topo
            local_total_num_eles += blueprint::mesh::utils::topology::length(dom_topo);

            topology::iterate_elements(dom_topo, [&](const topology::entity &e)
            {
                if(e.shape.is_polyhedral())
                {
                    for(const auto &faceIds : e.subelement_ids)
                        local_total_ele_to_verts_size += faceIds.size();
                }
                else
                {
                    local_total_ele_to_verts_size += e.element_ids.size();
                }
            });

        }
    }

    // reminder:
    // idx_t eldist[] = {0, 3, 4};
    //
    // idx_t eptr[] = {0,4,8,12};
    //
    // idx_t eind[] = {0,1,3,4,
    //                        1,2,4,5,
    //                        3,4,6,7};

    Node parmetis_params;
    // eldist tells how many elements there are per mpi task,
    // it will be size par_size + 1
    parmetis_params["eldist"].set(DataType(metis_idx_t_to_conduit_dtype_id(),
                                           par_size+1));
    // eptr holds the offsets to the start of each element's
    // vertex list
    // size == total number of local elements (we counted this above)
    parmetis_params["eptr"].set(DataType(metis_idx_t_to_conduit_dtype_id(),
                                         local_total_num_eles+1));
    // eind holds, for each element, a list of vertex ids
    // (we also counted this above)
    parmetis_params["eind"].set(DataType(metis_idx_t_to_conduit_dtype_id(),
                                         local_total_ele_to_verts_size));

    // output array, size of local num elements
    parmetis_params["part"].set(DataType(metis_idx_t_to_conduit_dtype_id(),
                                         local_total_num_eles));


    // first lets get eldist setup:
    // eldist[0] = 0
    // eldist[1] == # of elements on rank 0-
    // eldist[2] == # of elemens on rank 0 + rank 1
    //    ...
    // eldist[n] == # of total elements
    //
    Node el_counts;
    el_counts["local"]  = DataType(metis_idx_t_to_conduit_dtype_id(),
                                   par_size);
    el_counts["global"] = DataType(metis_idx_t_to_conduit_dtype_id(),
                                   par_size);

    idx_t *el_counts_local_vals  = el_counts["local"].value();
    idx_t *el_counts_global_vals = el_counts["global"].value();
    el_counts_local_vals[par_rank] = local_total_num_eles;
    relay::mpi::max_all_reduce(el_counts["local"], el_counts["global"], comm);

    // prefix sum to set eldist
    idx_t *eldist_vals = parmetis_params["eldist"].value();
    eldist_vals[0] = 0;
    for(size_t i=0;i<(size_t)par_size;i++)
    {
        eldist_vals[i+1] =  eldist_vals[i] + el_counts_global_vals[i];
    }

    idx_t *eptr_vals = parmetis_params["eptr"].value();
    idx_t *eind_vals = parmetis_params["eind"].value();

    // now elptr  == prefix sum of the sizes
    // (note: the offsets don't matter for elptr b/c we are creating a compact
    //  rep for parmetis)
    //
    // and eind == look up of global vertex id
    size_t eptr_idx=0;
    size_t eind_idx=0;
    idx_t  curr_offset = 0;
    for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
    {
        Node &dom = *domains[local_dom_idx];
        // we do need to make sure we have the requested topo
        if(dom["topologies"].has_child(topo_name))
        {
            // get the topo node
            Node &dom_topo = dom["topologies"][topo_name];

            topology::iterate_elements(dom_topo, [&](const topology::entity &e)
            {
                eptr_vals[eptr_idx] = curr_offset;
                if(e.shape.is_polyhedral())
                {
                    for(const auto &faceIds : e.subelement_ids)
                        curr_offset += faceIds.size();
                }
                else
                {
                    curr_offset += e.element_ids.size();
                }
                eptr_idx++;
            });

            // add last offset
            eptr_vals[eptr_idx] = curr_offset;

            const Node &dom_g_vert_ids = dom["fields"][field_prefix + "global_vertex_ids"]["values"];
            int64_accessor global_vert_ids = dom_g_vert_ids.as_int64_accessor();

            blueprint::mesh::utils::topology::iterate_elements(dom_topo, [&](const blueprint::mesh::utils::topology::entity &e)
            {
                if(e.shape.is_polyhedral())
                {
                    for(const auto &faceIds : e.subelement_ids)
                    {
                        for(size_t i = 0; i < faceIds.size(); i++)
                        {
                            eind_vals[eind_idx] = (idx_t) global_vert_ids[faceIds[i]];
                            eind_idx++;
                        }
                    }
                }
                else
                {
                    for(size_t i=0;i< e.element_ids.size();i++)
                    {
                        eind_vals[eind_idx] = (idx_t) global_vert_ids[e.element_ids[i]];
                        eind_idx++;
                    }
                }
            });
        }
    }

    idx_t wgtflag = 0; // weights are NULL
    idx_t numflag = 0; // C-style numbering
    idx_t ncon = 1; // the number of weights per vertex
    // equal weights for each proc
    std::vector<real_t> tpwgts(nparts, 1.0/nparts);
    real_t ubvec = 1.050000;

    // options == extra output
    idx_t parmetis_opts[] = {1,
                       PARMETIS_DBGLVL_TIME |
                       PARMETIS_DBGLVL_INFO |
                       PARMETIS_DBGLVL_PROGRESS |
                       PARMETIS_DBGLVL_REFINEINFO |
                       PARMETIS_DBGLVL_MATCHINFO |
                       PARMETIS_DBGLVL_RMOVEINFO |
                       PARMETIS_DBGLVL_REMAP,
                       0};
    // outputs
    idx_t edgecut = 0; // will hold # of cut edges

    // output array, size of local num elements
    parmetis_params["part"].set(DataType(metis_idx_t_to_conduit_dtype_id(),
                                         local_total_num_eles));
    idx_t *part_vals = parmetis_params["part"].value();

    int parmetis_res = ParMETIS_V3_PartMeshKway(eldist_vals,
                                                eptr_vals,
                                                eind_vals,
                                                NULL,
                                                &wgtflag,
                                                &numflag,
                                                &ncon,
                                                &ncommonnodes,
                                                &nparts,
                                                tpwgts.data(),
                                                &ubvec,
                                                parmetis_opts,
                                                &edgecut,
                                                part_vals,
                                                &comm);

    if( parmetis_res == METIS_ERROR )
    {
        // TODO: Should this be a full Error?
        CONDUIT_INFO("ParMETIS_V3_PartMeshKway call failed!");
    }

    index_t part_vals_idx=0;
    // create output field with part result
    for(size_t local_dom_idx=0; local_dom_idx < domains.size(); local_dom_idx++)
    {
        Node &dom = *domains[local_dom_idx];
        // we do need to make sure we have the requested topo
        if(dom["topologies"].has_child(topo_name))
        {
            // get the topo node
            const Node &dom_topo = dom["topologies"][topo_name];
            // get the number of elements in the topo
            index_t dom_num_eles = blueprint::mesh::utils::topology::length(dom_topo);
            // for unstrcut we need to do shape math, for unif/rect/struct
            //  we need to do implicit math

            // NOTE: VISIT BP DOESNT SUPPORT UINT64!!!!
            Node &part_field = dom["fields"][field_prefix + "parmetis_result"];
            part_field["association"] = "element";
            part_field["topology"] = topo_name;
            part_field["values"].set(DataType::int64(dom_num_eles));

            int64_array part_field_vals = part_field["values"].value();
            for(index_t i=0; i < dom_num_eles; i++)
            {
               part_field_vals[i] = part_vals[part_vals_idx];
               part_vals_idx++;
            }
        }
    }

}


//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh --
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

