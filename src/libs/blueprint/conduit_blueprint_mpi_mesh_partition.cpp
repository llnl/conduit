// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mpi_mesh_partition.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_blueprint_mpi_mesh_partition.hpp"

#include "conduit_relay_mpi.hpp"
#include <mpi.h>

#include "conduit_fmt/conduit_fmt.h"

#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <unordered_map>

using Partitioner = conduit::blueprint::mesh::Partitioner;
using Selection   = conduit::blueprint::mesh::Selection;

using std::cout;
using std::endl;

// Uncomment these macros to enable debugging output.
//#define CONDUIT_DEBUG_MAP_CHUNKS
//#define CONDUIT_DEBUG_COMMUNICATE_CHUNKS

// Renumber domains in parallel
#define RENUMBER_DOMAINS

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
//---------------------------------------------------------------------------
ParallelPartitioner::ParallelPartitioner(MPI_Comm c)
: Partitioner()
{
    comm = c;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    create_chunk_info_dt();
}

//---------------------------------------------------------------------------
ParallelPartitioner::~ParallelPartitioner()
{
    free_chunk_info_dt();
}

//---------------------------------------------------------------------------
std::vector<index_t>
ParallelPartitioner::get_global_domids(const conduit::Node& n_mesh)
{
    const auto doms = conduit::blueprint::mesh::domains(n_mesh);
    std::vector<index_t> domids(doms.size(), -1);
    bool have_domids = true;
    for(size_t i = 0; i < doms.size(); i++)
    {
        domids[i] = i;
        // If the mesh has a domain_id then use that as the domain number.
        if(doms[i]->has_path("state/domain_id"))
        {
            domids[i] = doms[i]->operator[]("state/domain_id").to_index_t();
        }
        else
        {
            have_domids = false;
        }
    }

    int64 prob_ndoms = 0;
    if (!have_domids)
    {
        // We had to generate local domain IDs in the range [0, ndoms)
        // Offset these for global IDs
        int64_t ndoms_local = doms.size();
        std::vector<int64_t> ndoms_all(size);
        ndoms_all[rank] = ndoms_local;
        // Gather all domain counts across ranks
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                      ndoms_all.data(), 1, MPI_INT64_T, comm);
        index_t offset = 0;
        for (index_t irnk = 0; irnk < rank; irnk++)
        {
            offset += ndoms_all[irnk];
        }
        for (size_t idom = 0; idom < doms.size(); idom++)
        {
            domids[idom] += offset;
        }
        prob_ndoms = std::accumulate(ndoms_all.begin(), ndoms_all.end(), 0);
    }
    else
    {
        if (domids.size() > 0)
        {
            prob_ndoms = *std::max_element(domids.begin(), domids.end());
        }
        MPI_Allreduce(MPI_IN_PLACE, &prob_ndoms, 1, MPI_INT64_T,
                      MPI_MAX, comm);
        // increment max domid to get number of domains in problem
        prob_ndoms++;
    }
    // Generate domain-to-rank map here
    domain_to_rank_map.resize(prob_ndoms, -1);
    for (index_t domid : domids)
    {
        domain_to_rank_map[domid] = rank;
    }
    MPI_Allreduce(MPI_IN_PLACE,
                  domain_to_rank_map.data(),
                  domain_to_rank_map.size(),
                  MPI_INT64_T, MPI_MAX, comm);
    return domids;
}

//---------------------------------------------------------------------------
bool
ParallelPartitioner::options_get_target(const conduit::Node &options,
                                        unsigned int &value) const
{
    // Get the target value using the base class method.
    unsigned int val = 0;
    Partitioner::options_get_target(options, val);

    // Take the max target across ranks. Ranks that did not provide it use 0.
    MPI_Allreduce(&val, &value, 1, MPI_UNSIGNED, MPI_MAX, comm);

    // A target was set by at least one rank if we have a value greater than 0.
    return value > 0;
}

//---------------------------------------------------------------------------
unsigned int
ParallelPartitioner::count_targets() const
{
    // Get the number of selections from all ranks. Count them.
    auto nlocal_sel = static_cast<int>(selections.size());
    std::vector<int> nglobal_sel(size);
    MPI_Allgather(&nlocal_sel, 1, MPI_INT,
                  &nglobal_sel[0], 1, MPI_INT, comm);

    // Count the total number of selections.
    int ntotal_sel = 0;
    for(size_t i = 0; i < nglobal_sel.size(); i++)
        ntotal_sel += nglobal_sel[i];

    // Compute offsets. We use int because of MPI_Gatherv
    std::vector<int> offsets(size, 0);
    for(size_t i = 1; i < nglobal_sel.size(); i++)
        offsets[i] = offsets[i-1] + nglobal_sel[i-1];

    // Populate the destination domains that we'll send.
    std::vector<int> local_dd(selections.size());
    for(size_t i = 0; i < selections.size(); i++)
        local_dd[i] = selections[i]->get_destination_domain();

    // Allgather the destination domains so each rank knows all the
    // destination domains.
    std::vector<int> global_dd(ntotal_sel);
    MPI_Allgatherv(&local_dd[0],
                   static_cast<int>(local_dd.size()),
                   MPI_INT,
                   &global_dd[0],
                   &nglobal_sel[0],
                   &offsets[0],
                   MPI_INT,
                   comm);

    // Now we know where each domain wants to go, determine the target count.
    unsigned int free_domains = 0;
    std::set<int> named_domains;
    for(size_t i = 0; i < global_dd.size(); i++)
    {
        if(global_dd[i] == Selection::FREE_DOMAIN_ID)
            free_domains++;
        else
            named_domains.insert(global_dd[i]);
    }

    unsigned int n = free_domains + static_cast<unsigned int>(named_domains.size());
    return n;
}

//---------------------------------------------------------------------------
long
ParallelPartitioner::get_total_selections() const
{
    // Gather the number of selections on each rank.
    long nselections = static_cast<long>(selections.size());
    long ntotal_selections = nselections;
    MPI_Allreduce(&nselections, &ntotal_selections, 1, MPI_LONG, MPI_SUM, comm);

    return ntotal_selections;
}

//---------------------------------------------------------------------------
/**
 @note This method is called iteratively until we have the number of target
       selections that we want to make. We could do better by identifying
       more selections to split in each pass. We use MPI_LONG_INT because it
       can be used with MPI_MAXLOC.
 */
void
ParallelPartitioner::get_largest_selection(int &sel_rank, int &sel_index) const
{
    // Find largest selection locally.
    long_int largest_selection;
    largest_selection.value = 0;
    largest_selection.rank = rank;
    std::vector<uint64> local_sizes(selections.size());
    for(size_t i = 0; i < selections.size(); i++)
    {
        local_sizes[i] = static_cast<uint64>(selections[i]->length(*meshes[i]));
        if(local_sizes[i] > static_cast<uint64>(largest_selection.value))
        {
            largest_selection.value = static_cast<long>(local_sizes[i]);
        }
    }

    // What's the largest selection across ranks? We use MPI_MAXLOC to
    // get the max size and the rank where it occurs.
    long_int global_largest_selection;
    MPI_Allreduce(&largest_selection, &global_largest_selection,
                  1, MPI_LONG_INT, MPI_MAXLOC, comm);

    // MPI found us the rank that has the largest selection.
    sel_rank = global_largest_selection.rank;
    sel_index = -1;

    // If we're on that rank, determine the local selection index.
    if(sel_rank == rank)
    {
        uint64 ssize = static_cast<uint64>(global_largest_selection.value);
        for(size_t i = 0; i < selections.size(); i++)
        {
            if(ssize == local_sizes[i])
            {
                sel_index = static_cast<int>(i);
                break;
            }
        }
    }
}

//-------------------------------------------------------------------------
void
ParallelPartitioner::create_chunk_info_dt()
{
    chunk_info obj;
    int slen = 3;
    int lengths[3] = {1,1,1};
    MPI_Datatype types[3];
    MPI_Aint offsets[3];

    types[0] = MPI_UNSIGNED_LONG_LONG;
    types[1] = MPI_INT;
    types[2] = MPI_INT;

    size_t base = ((size_t)(&obj));
    offsets[0] = ((size_t)(&obj.num_elements)) - base;
    offsets[1] = ((size_t)(&obj.destination_rank)) - base;
    offsets[2] = ((size_t)(&obj.destination_domain)) - base;

    MPI_Type_create_struct(slen, lengths, offsets, types, &chunk_info_dt);
    MPI_Type_commit(&chunk_info_dt);
}

//-------------------------------------------------------------------------
void
ParallelPartitioner::free_chunk_info_dt()
{
    MPI_Type_free(&chunk_info_dt);
}

//-------------------------------------------------------------------------
/**
The purpose of this function is to decide for a set of chunks on a rank, how
they will be assigned to final domains and where in the set of MPI ranks those
domains will live.

Some chunks will not care which domain they belong to nor where they
might end up. Such chunks will indicate a -1 for their domain number so we
have some freedom in how we assemble chunks into domains, according to the
target number of domains.

Some chunks may be the result of a field-based selection that
says explicitly where the cells will end up in a domain/rank. We can only have
a domain going to a single rank though.

@note We pass out the global dest_rank, dest_domain, offsets in this method
      since we consume it immediately in communicate_chunks where we need the
      global information to do matching sends/recvs.
*/
void
ParallelPartitioner::map_chunks(const std::vector<Partitioner::Chunk> &chunks,
                                std::vector<int> &dest_rank,
                                std::vector<int> &dest_domain,
                                std::vector<int> &_offsets)
{
    // Gather number of chunks on each rank.
    auto nlocal_chunks = static_cast<int>(chunks.size());
    std::vector<int> nglobal_chunks(size, 0);
    MPI_Allgather(&nlocal_chunks, 1, MPI_INT,
                  &nglobal_chunks[0], 1, MPI_INT,
                  comm);
    // Compute total chunks
    int ntotal_chunks = 0;
    for(size_t i = 0; i < nglobal_chunks.size(); i++)
        ntotal_chunks += nglobal_chunks[i];

#ifdef CONDUIT_DEBUG_MAP_CHUNKS
    MPI_Barrier(comm);
    if(rank == 0)
    {
        cout << "------------------------ map_chunks ------------------------" << endl;
        cout << "ntotal_chunks = " << ntotal_chunks << endl;
    }
    MPI_Barrier(comm);
#endif

    // Compute offsets. We use int because of MPI_Allgatherv
    std::vector<int> offsets(size, 0);
    for(size_t i = 1; i < nglobal_chunks.size(); i++)
        offsets[i] = offsets[i-1] + nglobal_chunks[i-1];

    // What we have at this point is a list of chunk sizes for all chunks
    // across all ranks. Let's get a global list of chunk domains (where
    // they want to go). A chunk may already know where it wants to go.
    // If it doesn't then we can assign it to move around. A chunk is
    // free to move around if its destination domain is -1.
    std::vector<chunk_info> local_chunk_info(chunks.size());
    for(size_t i = 0; i < chunks.size(); i++)
    {
        const conduit::Node &n_topos = chunks[i].mesh->operator[]("topologies");
        uint64 len = 0;
        for(index_t j = 0; j < n_topos.number_of_children(); j++)
            len += conduit::blueprint::mesh::topology::length(n_topos[j]);
        local_chunk_info[i].num_elements = len;
        local_chunk_info[i].destination_rank   = chunks[i].destination_rank;
        // If the destination rank is not Selection::FREE_RANK_ID then we
        // should make sure that the rank is not out of bounds (in case the
        // user set it to an out of bounds value).
        if(local_chunk_info[i].destination_rank != Selection::FREE_RANK_ID)
        {
            local_chunk_info[i].destination_rank = local_chunk_info[i].destination_rank % size;
        }
        local_chunk_info[i].destination_domain = chunks[i].destination_domain;
    }
    std::vector<chunk_info> global_chunk_info(ntotal_chunks);
    MPI_Allgatherv(&local_chunk_info[0],
                   static_cast<int>(local_chunk_info.size()),
                   chunk_info_dt,
                   &global_chunk_info[0],
                   &nglobal_chunks[0],
                   &offsets[0],
                   chunk_info_dt,
                   comm);
#ifdef CONDUIT_DEBUG_MAP_CHUNKS
    if(rank == 0)
    {
        for(int i = 0; i < ntotal_chunks; i++)
        {
            cout << "global_chunk_info[" << i << "]={"
                 << "num_elements=" << global_chunk_info[i].num_elements
                 << ", dest_rank=" << global_chunk_info[i].destination_rank
                 << ", dest_domain=" << global_chunk_info[i].destination_domain
                 << "}" << endl;
        }
    }
#endif

    // Determine how many ranks are free to move to various domains.
    // Also determine the domain ids that are reserved.
    std::set<int> reserved_dd;
#ifdef CONDUIT_DEBUG_MAP_CHUNKS
    int free_to_move = 0;
#endif
    for(int i = 0; i < ntotal_chunks; i++)
    {
        int domid = global_chunk_info[i].destination_domain;
        if(domid != Selection::FREE_DOMAIN_ID)
        {
            reserved_dd.insert(domid);

        }
#ifdef CONDUIT_DEBUG_MAP_CHUNKS
        else
        {
            free_to_move++;
        }
#endif
    }

#ifdef CONDUIT_DEBUG_MAP_CHUNKS
    if(rank == 0)
    {
        cout << "reserved_dd = {";
        for(int dd : reserved_dd)
        {
            cout << dd << ", ";
        }
        cout << "}" << endl;
        cout << "free_to_move = " << free_to_move << endl;
        cout << "target = " << target << endl;
    }
#endif

    // Pass out global information
    dest_rank.reserve(ntotal_chunks);
    dest_domain.reserve(ntotal_chunks);
    for(int i = 0; i < ntotal_chunks; i++)
    {
        dest_rank.push_back(global_chunk_info[i].destination_rank);
        dest_domain.push_back(global_chunk_info[i].destination_domain);
    }
    _offsets.swap(offsets);

    //-------------------------------------------------------------------
    // Assign domain numbers for any chunks that are not numbered.
    //-------------------------------------------------------------------
    // Figure out the size of the named domains. We'll add unassigned
    // domains to them.
    std::map<int,int> domain_elem_counts;
    for(int i = 0; i < ntotal_chunks; i++)
    {
        if(dest_domain[i] != Selection::FREE_DOMAIN_ID)
        {
            std::map<int,int>::iterator it = domain_elem_counts.find(dest_domain[i]);
            if(it == domain_elem_counts.end())
                domain_elem_counts[dest_domain[i]] = global_chunk_info[i].num_elements;
            else
                it->second += global_chunk_info[i].num_elements;
        }
    }
    if(reserved_dd.size() > target)
    {
        // We're not going to produce the target number of domains because
        // some chunks have told us which domains they want to be part of.
        // We will add any unassigned domains into these existing domains.
        CONDUIT_WARN("The unique number of domain ids "
            << reserved_dd.size()
            << " is greater than the desired target number of domains: "
            << target  << ".");
    }
    else
    {
        // We have some named domains and some unassigned domains that we
        // need to group together.
        int domains_to_create = static_cast<int>(target) -
                                static_cast<int>(reserved_dd.size());

        // Generate some additional domain numbers.
        int domid = 0;
        for(int i = 0; i < domains_to_create; i++)
        {
            while(reserved_dd.find(domid) != reserved_dd.end())
                domid++;
            reserved_dd.insert(domid);
            domain_elem_counts[domid] = 0;
        }
    }
    // Assign any unassigned domains to the domains in domain_elem_counts
    for(int i = 0; i < ntotal_chunks; i++)
    {
        if(dest_domain[i] == Selection::FREE_DOMAIN_ID)
        {
            // Find the domain that has the least cells.
            std::map<int,int>::iterator it, dit;
            dit = domain_elem_counts.begin();
            for(it = domain_elem_counts.begin(); it != domain_elem_counts.end(); it++)
            {
                if(it->second < dit->second)
                    dit = it;
            }

            dest_domain[i] = dit->first;
            dit->second += global_chunk_info[i].num_elements;
        }
    }
    // All domains should be assigned in dest_domain at this point.

    //-------------------------------------------------------------------
    // Assign domains to a rank if they are not already assigned.
    //-------------------------------------------------------------------
    // Take a look at the dest_rank values to see if there are domains
    // for which we need to assign to ranks.
    std::set<int> domains_to_assign;
    std::map<int,int> rank_elem_counts;
    for(int i = 0; i < size; i++)
        rank_elem_counts[i] = 0;
    for(int i = 0; i < ntotal_chunks; i++)
    {
        if(dest_rank[i] == Selection::FREE_RANK_ID)
        {
            // This domain is not assigned to a rank.
            domains_to_assign.insert(dest_domain[i]);
        }
        else
        {
            // Add the cells to the known rank.
            rank_elem_counts[dest_rank[i]] += global_chunk_info[i].num_elements;
        }
    }
#if 1
    // NOTE: This could be better. We could try to minimize communication
    //       by keeping domains where they are if possible while also
    //       trying to keep things balanced.

    // Add domains to ranks largest to smallest. This should
    // make smaller domains group together on a rank to some extent.
    std::multimap<int,int> size_to_domain;
    for(auto domid : domains_to_assign)
        size_to_domain.emplace(std::make_pair(domain_elem_counts[domid], domid));

    // Assign domains that do not have a dest_rank.
    for(std::multimap<int,int>::reverse_iterator sit = size_to_domain.rbegin();
        sit != size_to_domain.rend(); sit++)
    {
        int domid = sit->second;
#else
    // Assign domains that do not have a dest_rank.
    for(auto domid : domains_to_assign)
    {
#endif
        // Find the rank that has the least elements.
        std::map<int,int>::iterator it, rit;
        rit = rank_elem_counts.begin();
        for(it = rank_elem_counts.begin(); it != rank_elem_counts.end(); it++)
        {
            if(it->second < rit->second)
                rit = it;
        }
        // Now we know which rank will get the domain. Record it
        // into dest_rank.
        rit->second += domain_elem_counts[domid];
        for(int i = 0; i < ntotal_chunks; i++)
        {
            if(dest_domain[i] == domid)
                dest_rank[i] = rit->first;
        }
    }

#ifdef CONDUIT_DEBUG_MAP_CHUNKS
    // We're passing out global info now so all ranks should be the same.
    if(rank == 0)
    {
        std::cout << rank << ": dest_ranks={";
        for(size_t i = 0; i < dest_rank.size(); i++)
            std::cout << dest_rank[i] << ", ";
        std::cout << "}" << std::endl;
        std::cout << rank << ": dest_domain={";
        for(size_t i = 0; i < dest_domain.size(); i++)
            std::cout << dest_domain[i] << ", ";
        std::cout << "}" << std::endl;
        std::cout.flush();
    }
#endif
}

//-------------------------------------------------------------------------
/**
 @note In the parallel version of this function, we pass in global information
       for dest_rank, dest_domain, offsets. This helps us know not only the
       domains to which we have to send but also those who are sending to
       this rank.
 */
void
ParallelPartitioner::communicate_chunks(const std::vector<Partitioner::Chunk> &chunks,
    const std::vector<int> &dest_rank,
    const std::vector<int> &dest_domain,
    const std::vector<int> &offsets,
    std::vector<Partitioner::Chunk> &chunks_to_assemble,
    std::vector<int> &chunks_to_assemble_domains,
    std::vector<int> &chunks_to_assemble_gids)
{
    const int PARTITION_TAG_BASE = 12000;

    // Use the offsets to determine the sender for each chunk.
    std::vector<int> src_rank(dest_rank.size(),size-1);
    size_t idx = 0;
    for(size_t r = 1; r < offsets.size(); r++)
    {
        int n = offsets[r] - offsets[r-1];
        for(int i = 0; i < n; i++)
            src_rank[idx++] = r-1;
    }

#ifdef CONDUIT_DEBUG_COMMUNICATE_CHUNKS
    MPI_Barrier(comm);
    if(rank == 0)
    {
        cout << "offsets = {";
        for(size_t i = 0; i < offsets.size(); i++)
            cout << offsets[i] << ", ";
        cout << "}" << endl;
        cout << "src_rank = {";
        for(size_t i = 0; i < src_rank.size(); i++)
            cout << src_rank[i] << ", ";
        cout << "}" << endl;
    }
    MPI_Barrier(comm);
#endif

    // Create the object that will help us send/recv nodes. This object uses
    // non-blocking MPI communication so we do not have to worry about
    // send/recv order across ranks since communication may encompass a
    // complicated graph.
    conduit::relay::mpi::communicate_using_schema C(comm);
    //C.set_logging(true);

    // Do sends for the chunks we own on this processor that must migrate.
    for(size_t i = 0; i < chunks.size(); i++)
    {
        int gidx = offsets[rank] + i;
        int tag = PARTITION_TAG_BASE + gidx;
        int dest = dest_rank[gidx];
        // If we're not sending to self, send the chunk.
        if(dest != rank)
        {
#ifdef CONDUIT_DEBUG_COMMUNICATE_CHUNKS
            cout << rank << ": add_isend(dest="
                 << dest << ", tag=" << tag << ")" << endl;
#endif
            C.add_isend(*chunks[i].mesh, dest, tag);
        }
    }

    // Do recvs.
#ifdef RENUMBER_DOMAINS
    std::map<conduit::Node*,int> node_domains;
#endif
    for(size_t i = 0; i < dest_rank.size(); i++)
    {
        if(dest_rank[i] == rank)
        {
            int gidx = i;
            int tag = PARTITION_TAG_BASE + gidx;
            int start = offsets[rank];
            int end = start + chunks.size();
            bool this_rank_already_owns_it = (gidx >= start && gidx < end);
            if(this_rank_already_owns_it)
            {
                int local_i = i - offsets[rank];

#ifdef RENUMBER_DOMAINS
                // The chunk we have here needs its state/domain_id updated but
                // we really should not modify it directly. Do we have to make
                // a new node and set_external everything in it? Then make it
                // have its own state/domain_id?
                conduit::Node *n_recv = new conduit::Node;
                for(index_t ci = 0; ci < chunks[local_i].mesh->number_of_children(); ci++)
                {
                    const conduit::Node &n = chunks[local_i].mesh->operator[](ci);
                    if(n.name() != "state")
                        (*n_recv)[n.name()].set_external_node(n);
                }
                if(chunks[local_i].mesh->has_path("state/cycle"))
                    (*n_recv)["state/cycle"] = (*chunks[local_i].mesh)["state/cycle"];
                if(chunks[local_i].mesh->has_path("state/time"))
                    (*n_recv)["state/time"] = (*chunks[local_i].mesh)["state/time"];
                (*n_recv)["state/domain_id"] = dest_domain[i];

                // Save the chunk "wrapper" that has its own state.
                chunks_to_assemble.push_back(Chunk(n_recv, true));
                chunks_to_assemble_domains.push_back(dest_domain[i]);
                chunks_to_assemble_gids.push_back(gidx);
#else
                // Pass the chunk through since we already own it on this rank.
                chunks_to_assemble.push_back(Chunk(chunks[local_i].mesh, false));
                chunks_to_assemble_domains.push_back(dest_domain[i]);
                chunks_to_assemble_gids.push_back(gidx);
#endif
            }
            else
            {
#ifdef CONDUIT_DEBUG_COMMUNICATE_CHUNKS
                cout << rank << ": add_irecv(src=" << src_rank[i]
                     << ", tag=" << tag << ")" << endl;
#endif
                // Make a new node that we'll recv into.
                conduit::Node *n_recv = new conduit::Node;
                C.add_irecv(*n_recv, src_rank[i], tag);

#ifdef RENUMBER_DOMAINS
                node_domains[n_recv] = dest_domain[i];
#endif
                // Save the received chunk and indicate we own it for later.
                chunks_to_assemble.push_back(Chunk(n_recv, true));
                chunks_to_assemble_domains.push_back(dest_domain[i]);
                chunks_to_assemble_gids.push_back(gidx);
            }
        }
    }

    // Execute all of the isends/irecvs
    C.execute();

#ifdef RENUMBER_DOMAINS
    // Make another pass through the received domains and renumber them.
    for(auto it : node_domains)
    {
        conduit::Node &n = *it.first;
        n["state/domain_id"] = it.second;
    }
#endif
}

//-----------------------------------------------------------------------------
void
ParallelPartitioner::get_prelb_adjset_maps(const std::vector<int>& chunk_offsets,
                                           const DomainToChunkMap& chunks,
                                           const std::map<index_t, const Node*>& domain_map,
                                           std::vector<Node>& adjset_chunk_maps)
{
    const int PARTITION_TAG_BASE = 14000;

    adjset_chunk_maps.resize(domain_to_rank_map.size());

    Partitioner::get_prelb_adjset_maps(chunk_offsets, chunks, domain_map, adjset_chunk_maps);

    // 1. Compute the ranks to send/receive data from for each domain
    std::unordered_map<index_t, std::unordered_set<int>> send_rank;
    std::unordered_map<index_t, int> recv_rank;

    for (const auto& dom_it : domain_map)
    {
        const Node& domain = *(dom_it.second);
        const index_t dom_idx = dom_it.first;
        if (!domain.has_child("adjsets"))
        {
            continue;
        }

        // loop over all adjsets in the current domain
        for (const Node& adjset : domain["adjsets"].children())
        {
            for (const Node& group : adjset["groups"].children())
            {
                index_t_accessor neighbor_vals = group["neighbors"].as_index_t_accessor();
                index_t dst_dom = neighbor_vals[0];
                int nbr_rank = domain_to_rank_map[dst_dom];

                // For each corresponding pair of domains linked by adjsets, we
                // need a matched pair of send/recvs
                send_rank[dom_idx].insert(nbr_rank);
                recv_rank[dst_dom] = nbr_rank;
            }
        }
    }

    conduit::relay::mpi::communicate_using_schema C(comm);
    //C.set_logging(true);

    // 2. Setup nonblocking sends.
    for (const auto& send_dom : send_rank)
    {
        index_t dom_to_send = send_dom.first;
        int tag = PARTITION_TAG_BASE + dom_to_send;

        for (int dst_rank : send_dom.second)
        {
#ifdef CONDUIT_DEBUG_COMMUNICATE_CHUNKS
            cout << rank << ": add_isend(dest="
                 << dst_rank << ", dom=" << dom_to_send << ")" << endl;
#endif
            C.add_isend(adjset_chunk_maps[dom_to_send], dst_rank, tag);
        }
    }

    // 3. Setup nonblocking recvs.
    for (const auto& recv_dom : recv_rank)
    {
        index_t dom_to_recv = recv_dom.first;
        int src_rank = recv_dom.second;
        int tag = PARTITION_TAG_BASE + dom_to_recv;
#ifdef CONDUIT_DEBUG_COMMUNICATE_CHUNKS
            cout << rank << ": add_irecv(src="
                 << src_rank << ", dom=" << dom_to_recv << ")" << endl;
#endif
        C.add_irecv(adjset_chunk_maps[dom_to_recv], src_rank, tag);
    }

    C.execute();
}

//-----------------------------------------------------------------------------
void
ParallelPartitioner::communicate_mapback(std::unordered_map<index_t, Node>& packed_fields)
{
    // Get source ranks for domains homed on this processor
    std::unordered_map<index_t, std::vector<index_t>> dom_to_recv_rnk;
    {
        // Get number of outgoing chunks
        int64 nchunks = packed_fields.size();
        std::vector<int> cnks_per_proc(size, 0);
        cnks_per_proc[rank] = nchunks;
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                      cnks_per_proc.data(), 1, MPI_INT, comm);

        int64 ncnks = std::accumulate(cnks_per_proc.begin(),
                                      cnks_per_proc.end(), 0);
        std::vector<int> displs(size, 0);
        for (int irnk = 1; irnk < size; irnk++)
        {
            displs[irnk] = displs[irnk - 1] + cnks_per_proc[irnk - 1];
        }

        std::vector<int64> all_doms(ncnks, -1), all_src_rnks(ncnks, -1);
        // We'll allgather corresponding domain ids and source ranks from each
        // processor
        size_t offset = displs[rank];
        for (const auto& dom_field : packed_fields)
        {
            index_t tgt_domain = dom_field.first;
            all_doms[offset] = tgt_domain;
            all_src_rnks[offset] = rank;
            offset++;
        }
        size_t expected_max = (rank + 1 == size) ? ncnks : displs[rank + 1];
        if (offset != expected_max)
        {
            CONDUIT_ERROR(
              conduit_fmt::format("Displacement error: (rank = {} offset = {} "
                                  "bound = {}", rank, offset, expected_max));
        }

        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                       all_doms.data(), cnks_per_proc.data(), displs.data(),
                       MPI_INT64_T, comm);

        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                       all_src_rnks.data(), cnks_per_proc.data(), displs.data(),
                       MPI_INT64_T, comm);

        // Put everything we own in dom_to_recv_rnk
        for (int64 icnk = 0; icnk < ncnks; icnk++)
        {
            int64 dom_id = all_doms[icnk];
            int64 src_rnk = all_src_rnks[icnk];
            if (domain_to_rank_map[dom_id] == rank && src_rnk != rank)
            {
                dom_to_recv_rnk[dom_id].push_back(src_rnk);
            }
        }
    }

    conduit::relay::mpi::communicate_using_schema C(comm);
    //C.set_logging(true);

    constexpr int MAPBACK_TAG_BASE = 16000;

    // Setup nonblocking sends.
    for (const auto& dom_field : packed_fields)
    {
        index_t tgt_domain = dom_field.first;
        index_t tgt_rank = domain_to_rank_map[tgt_domain];
        if (tgt_rank != rank)
        {
            const int tag = MAPBACK_TAG_BASE + tgt_domain;
            C.add_isend(dom_field.second, tgt_rank, tag);
        }
    }

    // Compute the total number of receive nodes to allocate.
    // This avoids vector resizes invalidating any stored references to nodes
    // in the communication object.
    index_t ntotal_recvs = 0;
    for (const auto& dom_recv_pair : dom_to_recv_rnk)
    {
        ntotal_recvs += dom_recv_pair.second.size();
    }

    // Setup nonblocking receives.
    std::vector<std::pair<index_t, Node>> pending_recvs(ntotal_recvs);
    for (const auto& dom_recv_pair : dom_to_recv_rnk)
    {
        index_t tgt_domain = dom_recv_pair.first;
        const int tag = MAPBACK_TAG_BASE + tgt_domain;
        for (index_t src_rank : dom_recv_pair.second)
        {
            pending_recvs.emplace_back(tgt_domain, Node{});
            Node& recv_node = pending_recvs.rbegin()->second;
            C.add_irecv(recv_node, src_rank, tag);
        }
    }

    C.execute();

    // Append received chunks to original domain chunk map
    for (const auto& recv_result : pending_recvs)
    {
        index_t tgt_domain = recv_result.first;
        for (const Node& cnk : recv_result.second.children())
        {
            packed_fields[tgt_domain].append().set(cnk);
        }
    }

    // Remove domains not homed on this rank
    std::vector<index_t> to_remove;
    for (const auto& dom_nodes : packed_fields)
    {
        index_t tgt_domain = dom_nodes.first;
        if (domain_to_rank_map[tgt_domain] != rank)
        {
            to_remove.push_back(tgt_domain);
        }
    }
    for (index_t domid : to_remove)
    {
        packed_fields.erase(domid);
    }
}

void
ParallelPartitioner::synchronize_gvids(
    const std::vector<std::vector<index_t>>& remap_to_local_doms,
    std::map<index_t, std::vector<index_t>>& orig_dom_gvids)
{
    std::vector<int64> orig_domain_nverts(domain_to_rank_map.size(), 0);
    for (const auto& orig_dom_verts : orig_dom_gvids)
    {
        index_t domid = orig_dom_verts.first;
        orig_domain_nverts[domid] = orig_dom_verts.second.size();
    }
    // Get the number of verts in each domain
    MPI_Allreduce(MPI_IN_PLACE,
                  orig_domain_nverts.data(),
                  orig_domain_nverts.size(),
                  MPI_INT64_T, MPI_MAX, comm);

    // First, figure out the unique domains that we need to receive on this rank
    std::unordered_set<index_t> orig_doms_required;
    for (const auto& orig_domset : remap_to_local_doms)
    {
        for (index_t domid : orig_domset)
        {
            if (domain_to_rank_map[domid] != rank)
            {
                orig_doms_required.insert(domid);
            }
        }
    }

    MPI_Datatype index_mpi_dtype
        = relay::mpi::conduit_dtype_to_mpi_dtype(conduit::DataType::index_t());
    std::vector<std::pair<int, index_t>> domain_sends;
    // Determine ranks that we need to send domains to
    {
        std::vector<int> counts(size, 0);
        counts[rank] = orig_doms_required.size();
        MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                      counts.data(), 1, MPI_INT,
                      comm);

        std::vector<int> offsets(size, 0);
        for (int irnk = 1; irnk < size; irnk++)
        {
            offsets[irnk] = offsets[irnk-1] + counts[irnk-1];
        }
        int64 total_counts = offsets[size-1] + counts[size-1];
        std::vector<int64> all_doms(total_counts);
        std::copy(orig_doms_required.begin(), orig_doms_required.end(),
                  all_doms.begin() + offsets[rank]);
        MPI_Allgatherv(MPI_IN_PLACE,
                       0,
                       MPI_DATATYPE_NULL,
                       all_doms.data(),
                       counts.data(),
                       offsets.data(),
                       index_mpi_dtype, comm);
        // Add domains which other ranks will need to receive from us
        for (int irnk = 0; irnk < size; irnk++)
        {
            for (int ireq = 0; ireq < counts[irnk]; ireq++)
            {
                size_t realIdx = offsets[irnk] + ireq;
                index_t domid = all_doms[realIdx];
                if (domain_to_rank_map[domid] == rank)
                {
                    domain_sends.emplace_back(irnk, domid);
                }
            }
        }
    }

    std::vector<MPI_Request> pending_reqs(orig_doms_required.size()
                                          + domain_sends.size());
    const int GVID_TAG = 227000;
    int req_id = 0;
    // Start any receives of required domains
    for (index_t domid : orig_doms_required)
    {
        int src_rnk = domain_to_rank_map[domid];
        CONDUIT_ASSERT((src_rnk != rank),
            conduit_fmt::format("Rank {}: Unnecessary recv of original domain {}",
                                rank, domid));
        orig_dom_gvids[domid].resize(orig_domain_nverts[domid]);
        MPI_Irecv(orig_dom_gvids[domid].data(),
                  orig_dom_gvids[domid].size(),
                  index_mpi_dtype,
                  src_rnk,
                  GVID_TAG + domid,
                  comm,
                  &pending_reqs[req_id]);
        req_id++;
    }
    for (const auto& incoming_req : domain_sends)
    {
        int dst_rank = incoming_req.first;
        int64 domid = incoming_req.second;
        CONDUIT_ASSERT((domain_to_rank_map[domid] == rank),
            conduit_fmt::format("Rank {}: domain id {} doesn't exist on this rank",
                                rank, domid));
        MPI_Isend(orig_dom_gvids[domid].data(),
                  orig_dom_gvids[domid].size(),
                  index_mpi_dtype,
                  dst_rank,
                  GVID_TAG + domid,
                  comm,
                  &pending_reqs[req_id]);
        req_id++;
    }

    MPI_Waitall(pending_reqs.size(), pending_reqs.data(), MPI_STATUSES_IGNORE);
}

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

