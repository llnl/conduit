// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mpi_mesh_util.hpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_mpi_mesh_utils.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_annotations.hpp"
#include "conduit_relay_mpi.hpp"

// #define DEBUG_PRINT
#ifdef DEBUG_PRINT
// NOTE: if DEBUG_PRINT is defined then the library must also depend on conduit_relay
#include "conduit_relay_io.hpp"
#endif

#include <cstring>

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
// -- begin conduit::blueprint::mpi::mesh::utils --
//-----------------------------------------------------------------------------
namespace utils
{

topology::MeshInfoCollection::MeshInfoCollection(MPI_Comm comm) :
    conduit::blueprint::mesh::utils::topology::MeshInfoCollection(), m_comm(comm)
{
}

void topology::MeshInfoCollection::end()
{
    CONDUIT_ANNOTATE_MARK_SCOPE("MeshInfoCollection::end()");
    using MeshInfo = conduit::blueprint::mesh::utils::topology::MeshInfo;
    const auto numLocalMeshInfo = static_cast<index_t>(meshInfos.size());
    const index_t numValuesPerMeshInfo = 1 + static_cast<index_t>(sizeof(MeshInfo) / sizeof(float64));

    // Encode local mesh info into an array.
    Node n_localMeshInfo;
    n_localMeshInfo.set(conduit::DataType::float64(numLocalMeshInfo * numValuesPerMeshInfo));
    float64 *ptr = n_localMeshInfo.value();
    for(auto it = meshInfos.begin(); it != meshInfos.end(); it++)
    {
        *ptr++ = static_cast<float64>(it->first);
        *ptr++ = it->second.minExtents[0];
        *ptr++ = it->second.minExtents[1];
        *ptr++ = it->second.minExtents[2];
        *ptr++ = it->second.maxExtents[0];
        *ptr++ = it->second.maxExtents[1];
        *ptr++ = it->second.maxExtents[2];
        *ptr++ = it->second.minEdgeLength;
        *ptr++ = it->second.maxEdgeLength;
        *ptr++ = it->second.minDiagonalLength;
        *ptr++ = it->second.maxDiagonalLength;
    }

    // Make sure all ranks get the info.
    Node n_globalMeshInfo;
    conduit::relay::mpi::all_gather(n_localMeshInfo, n_globalMeshInfo, m_comm);

    // Decode the infos and store them.
    bool first = true;
    mergedMeshInfo = MeshInfo();
    for(index_t i = 0; i < n_globalMeshInfo.number_of_children(); i++)
    {
        const Node &n_meshInfo = n_globalMeshInfo[i];
        const index_t numMeshInfo = n_meshInfo.dtype().number_of_elements() / numValuesPerMeshInfo;
        const float64 *ptr2 = n_meshInfo.value();
        for(index_t j = 0; j < numMeshInfo; j++)
        {
           MeshInfo info;
           index_t domainId = static_cast<index_t>(ptr2[0]);
           info.minExtents[0] = ptr2[1];
           info.minExtents[1] = ptr2[2];
           info.minExtents[2] = ptr2[3];
           info.maxExtents[0] = ptr2[4];
           info.maxExtents[1] = ptr2[5];
           info.maxExtents[2] = ptr2[6];
           info.minEdgeLength = ptr2[7];
           info.maxEdgeLength = ptr2[8];
           info.minDiagonalLength = ptr2[9];
           info.maxDiagonalLength = ptr2[10];

           add(domainId, info);
           if(first)
           {
               first = false;
               mergedMeshInfo = info;
           }
           else
           {
               mergedMeshInfo = merge(mergedMeshInfo, info);
           }

           ptr2 += numValuesPerMeshInfo;
        }
    }
}

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi::mesh::utils::query --
//-----------------------------------------------------------------------------
namespace query
{

const double PointQuery::DEFAULT_POINT_TOLERANCE = 1.e-8;

//---------------------------------------------------------------------------
PointQuery::PointQuery(const conduit::Node &mesh, MPI_Comm comm) :
    conduit::blueprint::mesh::utils::query::PointQuery(mesh), m_comm(comm)
{
}

//---------------------------------------------------------------------------
void
PointQuery::execute(const std::string &coordsetName)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    int rank = 0, size = 1;
    MPI_Comm_rank(m_comm, &rank);
    MPI_Comm_size(m_comm, &size);

    // Figure out which ranks own the domains. Get the domain ids on this rank
    // and sum the number of domains on all ranks.
    std::vector<int> localDomains = domainIds();
    int ndoms = static_cast<int>(localDomains.size());
    int ntotal_doms = 0;
    MPI_Allreduce(&ndoms, &ntotal_doms, 1, MPI_INT, MPI_SUM, m_comm);

    // Make sure each rank knows who owns which domains. This assumes that
    // the domains were set properly and only occur on one rank.
    std::vector<int> local_domain_to_rank(ntotal_doms, 0);
    std::vector<int> domain_to_rank(ntotal_doms, 0);
    for(auto dom : localDomains)
        local_domain_to_rank[dom] = rank;
    MPI_Allreduce(&local_domain_to_rank[0], &domain_to_rank[0], ntotal_doms, MPI_INT, MPI_SUM, m_comm);

    // Put together the queries that we need to run.
    std::vector<int> queries;
    for(auto it = m_domInputs.begin(); it != m_domInputs.end(); it++)
    {
        queries.push_back(rank);              // the rank asking the question
        queries.push_back(it->first);         // the domain
        queries.push_back(static_cast<int>(it->second.size())); // the number of points.
    }

    // Let all ranks know the sizes of the queries vectors.
    auto nq = static_cast<int>(queries.size());
    std::vector<int> qsize(size, 0);
    MPI_Allgather(&nq, 1, MPI_INT, &qsize[0], 1, MPI_INT, m_comm);

    // Make an offset array and total the sizes.
    std::vector<int> qoffset(size, 0);
    for(int i = 1; i < size; i++)
        qoffset[i] = qoffset[i-1] + qsize[i-1];
    int total_qsize = 0;
    for(int i = 0; i < size; i++)
        total_qsize += qsize[i];

    // Gather the queries to all ranks.
    std::vector<int> allqueries(total_qsize, 0);
    MPI_Allgatherv(&queries[0], nq, MPI_INT,
                   &allqueries[0], &qsize[0], &qoffset[0], MPI_INT,
                   m_comm);

#ifdef DEBUG_PRINT
    // Print out some debugging information to files.
    conduit::Node debug;
    debug["rank"] = rank;
    debug["size"] = size;
    debug["localDomains"].set(localDomains);
    debug["ntotal_doms"] = ntotal_doms;
    debug["domain_to_rank"].set(domain_to_rank);
    debug["queries"].set(queries);
    debug["nq"] = nq;
    debug["qsize"].set(qsize);
    debug["qoffset"].set(qoffset);
    debug["total_qsize"] = total_qsize;
    debug["allqueries"].set(allqueries);

    // Store the points that we'll ask of each domain.
    conduit::Node &qi = debug["inputs"];
    for(auto it = m_domInputs.begin(); it != m_domInputs.end(); it++)
    {
        std::vector<double> qvals(it->second.begin(), it->second.end());
        conduit::Node &qid = qi.append();
        qid["domain"] = it->first;
        qid["input"].set(qvals);
    }

    // Save the input mesh
    debug["mesh"].set_external(m_mesh);

    std::stringstream ss;
    ss << "pointquery." << rank << ".yaml";
    std::string filename(ss.str());
    conduit::relay::io::save(debug, filename, "yaml");
#endif

    // Do a pass upfront where we look for domain ids from the query that
    // do not map to ranks. This would be possible if the adjset references
    // domains that were not actually added to the mesh. This should not
    // happen but it came up in a test case.
    for(size_t i = 0; i < allqueries.size(); i += 3)
    {
        int domain = allqueries[i+1];
        if(domain < 0 || domain >= static_cast<int>(domain_to_rank.size()))
        {
            CONDUIT_ERROR("An adjacency set referenced domain " << domain
                << ", which is not a valid domain in the input mesh.");
        }
    }

    // Now we can start to issue some queries. We do a first pass that sends/recvs
    // all of the query points. Then a second pass does the queries and sends/recvs
    // the results. The results get stored in m_domResults.
    std::map<std::pair<int,int>, conduit::Node *> input_sends, result_sends,
                                                  input_recvs, result_recvs;
    const int inputs_tag = 550;
    const int results_tag = 660;
    for(int pass = 0; pass < 2; pass++)
    {
        conduit::relay::mpi::communicate_using_schema C(m_comm);
#ifdef DEBUG_PRINT
        // Turn on logging.
        std::stringstream ss;
        ss << "mpi_pointquery_pass" << pass;
        C.set_logging_root(ss.str());
        C.set_logging(true);
#endif
        for(size_t i = 0; i < allqueries.size(); i += 3)
        {
            const int asker  = allqueries[i];
            const int domain = allqueries[i+1];
            const int npts   = allqueries[i+2];
            const int owner = domain_to_rank[domain];

            if(asker == rank)
            {
                if(owner == rank)
                {
                    // This rank already owns the data. We can do the search.
                    // There is no need to communicate.
                    if(pass == 0)
                    {
                        // Query the domain and store the results in r.
                        std::vector<int> &r = m_domResults[domain];
                        r.resize(npts);
                        const conduit::Node *dom = getDomain(domain);
                        findPointsInDomain(*dom, coordsetName, inputs(domain), r);
                    }
                }
                else
                {
                    auto id = std::make_pair(owner, domain);

                    // A different rank owns the data. We need to send the coordinates
                    // we're querying and then post a receive for the result.
                    if(pass == 0)
                    {
                        // Wrap the inputs as a Conduit node and post an isend.
                        const std::vector<double> &inp = inputs(domain);
                        input_sends[id] = new conduit::Node;
                        conduit::Node &q = *input_sends[id];
                        q["inputs"].set_external(const_cast<double *>(&inp[0]), inp.size());
                        C.add_isend(q, owner, inputs_tag + domain);
                    }

                    if(pass == 1)
                    {
                        // Make a node to receive the results.
                        result_recvs[id] = new conduit::Node;
                        conduit::Node &r = *result_recvs[id];
                        C.add_irecv(r, owner, results_tag + domain);
                    }
                }
            }
            else if(owner == rank)
            {
                // This rank is not asker but it owns the domain and so it has to
                // do the query.
                auto id = std::make_pair(asker, domain);

                if(pass == 0)
                {
                    // Make a node to store the query parameters
                    input_recvs[id] = new conduit::Node;
                    conduit::Node &q = *input_recvs[id];
                    C.add_irecv(q, asker, inputs_tag + domain);
                }

                if(pass == 1)
                {
                    // We have the query parameters in input_recv. Turn it into a vector.
                    conduit::Node &q = *input_recvs[id];
                    auto acc = q["inputs"].as_double_accessor();
                    std::vector<double> input;
                    input.reserve(acc.number_of_elements());
                    for(conduit::index_t j = 0; j < acc.number_of_elements(); j++)
                        input.push_back(acc[j]);

                    // Do the query.
                    std::vector<int> result;
                    const conduit::Node *dom = getDomain(domain);
                    findPointsInDomain(*dom, coordsetName, input, result);

                    // Make a node to send the results back to the asker.
                    result_sends[id] = new conduit::Node;
                    conduit::Node &r = *result_sends[id];
                    r["results"].set(result);
                    C.add_isend(r, asker, results_tag + domain);
                }
            }
        }

        // Do the exchanges.
        C.execute();
    }

    // We have query results to convert/store in m_domResults.
    for(auto it = result_recvs.begin(); it != result_recvs.end(); it++)
    {
        int domain = it->first.second;

        const conduit::Node &r = it->second->fetch_existing("results");
        auto acc = r.as_int_accessor();
        std::vector<int> &result = m_domResults[domain];
        result.reserve(acc.number_of_elements());
        for(conduit::index_t i = 0; i < acc.number_of_elements(); i++)
            result.push_back(acc[i]);

#ifdef DEBUG_PRINT
        // Add the results into the debug node. Any points that could not be
        // located will contain -1 (NotFound) for their entry.
        conduit::Node &rn = debug["results"].append();
        rn["domain"] = domain;
        rn["result"].set(result);
#endif
    }

#ifdef DEBUG_PRINT
    // Overwrite the log file with new information.
    conduit::relay::io::save(debug, filename, "yaml");
#endif

    // Clean up maps.
    for(auto it = input_recvs.begin(); it != input_recvs.end(); it++)
        delete it->second;
    for(auto it = input_sends.begin(); it != input_sends.end(); it++)
        delete it->second;
    for(auto it = result_recvs.begin(); it != result_recvs.end(); it++)
        delete it->second;
    for(auto it = result_sends.begin(); it != result_sends.end(); it++)
        delete it->second;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
MatchQuery::MatchQuery(const conduit::Node &mesh, MPI_Comm comm) :
    conduit::blueprint::mesh::utils::query::MatchQuery(mesh), m_comm(comm)
{
}

//---------------------------------------------------------------------------
void
MatchQuery::execute()
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    int rank = 0, size = 1;
    MPI_Comm_rank(m_comm, &rank);
    MPI_Comm_size(m_comm, &size);

    // Build the query geometries. Store them in the query_topo node.
    CONDUIT_ANNOTATE_MARK_BEGIN("build");
    std::string shape;
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        int dom = it->first.first;

        if(shape.empty())
        {
            // We have not determined the shape yet. Do that now so the subset
            // topologies can be built.
            const auto dtopo = getDomainTopology(dom);
            conduit::blueprint::mesh::utils::ShapeCascade c(*dtopo);
            const auto &s = c.get_shape((c.dim == 0) ? c.dim : (c.dim - 1));
            shape = s.type();
        }

        it->second.builder->execute(it->second.query_mesh, shape);
    }
    CONDUIT_ANNOTATE_MARK_END("build");

    // If we have a bunch of requests A,B  C,D then we need to get the entity
    // arrays for B,A and D,C. The second int indicates the rank that owns the
    // entity data.

    CONDUIT_ANNOTATE_MARK_BEGIN("setup");
    // Make a set of ints <dom, query_dom, nents> for each map entry. Each
    // rank owns the domains indicated by dom.
    const int ntuple_values = 3;
    std::vector<int> queries;
    queries.reserve(m_query.size() * ntuple_values);
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        queries.push_back(rank);
        queries.push_back(it->first.first);
        queries.push_back(it->first.second);
    }

    // Let all ranks know the sizes of the queries vectors.
    int nq = static_cast<int>(queries.size());
    std::vector<int> qsize(size, 0);
    MPI_Allgather(&nq, 1, MPI_INT, &qsize[0], 1, MPI_INT, m_comm);

    // Make an offset array and total the sizes.
    std::vector<int> qoffset(size, 0);
    for(int i = 1; i < size; i++)
        qoffset[i] = qoffset[i-1] + qsize[i-1];
    int total_qsize = 0;
    for(int i = 0; i < size; i++)
        total_qsize += qsize[i];

    // Gather the queries to all ranks.
    std::vector<int> allqueries(total_qsize, 0);
    MPI_Allgatherv(&queries[0], nq, MPI_INT,
                   &allqueries[0], &qsize[0], &qoffset[0], MPI_INT,
                   m_comm);

    // Look up a rank that owns a domain.
    auto domain_to_rank = [=](const std::vector<int> &allq, int d) -> int
    {
        for(size_t i = 0; i < allq.size(); i += ntuple_values)
        {
            int owner = allq[i];
            int domain = allq[i + 1];
            //int query_domain = allqueries[i + 2];
            if(domain == d)
                return owner;
        }
        return -1;
    };

#ifdef DEBUG_PRINT
    // Print out some debugging information to files.
    conduit::Node debug;
    debug["rank"] = rank;
    debug["size"] = size;
    debug["queries"].set(queries);
    debug["nq"] = nq;
    debug["qsize"].set(qsize);
    debug["qoffset"].set(qoffset);
    debug["total_qsize"] = total_qsize;
    debug["allqueries"].set(allqueries);

    // Store the points that we'll ask of each domain.
    conduit::Node &qi = debug["inputs"];
    for(auto it = m_query.begin(); it != m_query.end(); it++)
    {
        conduit::Node &qid = qi.append();
        qid["domain"] = it->first.first;
        qid["query_domain"] = it->first.second;
        qid["query_topo"] = it->second.query_mesh;
    }

    std::stringstream ss;
    ss << "matchquery." << rank << ".yaml";
    std::string filename(ss.str());
    conduit::relay::io::save(debug, filename, "yaml");
#endif
    CONDUIT_ANNOTATE_MARK_END("setup");

    // Send/recv entity data.
    CONDUIT_ANNOTATE_MARK_BEGIN("communication");
    conduit::relay::mpi::communicate_using_schema C(m_comm);
#ifdef DEBUG_PRINT
    // Turn on logging.
    C.set_logging_root("mpi_matchquery");
    C.set_logging(true);
#endif
    const int query_tag = 770;
    for(size_t i = 0; i < allqueries.size(); i += ntuple_values)
    {
        const int owner = allqueries[i];
        const int domain = allqueries[i + 1];
        const int query_domain = allqueries[i + 2];

        auto oppositeKey = std::make_pair(query_domain, domain);

        if(owner == rank)
        {
            // The query was asked by this rank. We need to prepare to
            // receive the opposite query, which is the answer.

            if(m_query.find(oppositeKey) == m_query.end())
            {
                int remote = domain_to_rank(allqueries, query_domain);

                // The domain whose entities we're querying is on a remote rank.
                // We need to receive entities from it.
                auto &q = m_query[oppositeKey];
                conduit::Node &r = q.query_mesh;
                C.add_irecv(r, remote, query_tag + query_domain);
            }
        }
        else
        {
            // We're not the rank that requested the data.
            // See if we own the answer. If so, send. Note the key swap.

            auto it = m_query.find(oppositeKey);
            if(it != m_query.end())
            {
                int remote = domain_to_rank(allqueries, domain);

                // Send the query_topo to the remote rank.
                conduit::Node &s = it->second.query_mesh;
                C.add_isend(s, remote, query_tag + query_domain);
            }
        }
    }

    // Do the communication.
    C.execute();
    CONDUIT_ANNOTATE_MARK_END("communication");

    // Now, m_query should contain query topos for A,B and B,A. For all the
    // domains owned by this rank, make the results.
    CONDUIT_ANNOTATE_MARK_BEGIN("results");
    for(size_t i = 0; i < allqueries.size(); i += ntuple_values)
    {
        int owner = allqueries[i];
        int domain = allqueries[i + 1];
        int query_domain = allqueries[i + 2];

        if(owner == rank)
        {
            auto key = std::make_pair(domain, query_domain);
            auto oppositeKey = std::make_pair(query_domain, domain);

            // Try and get the geometries.
            auto it = m_query.find(key);
            auto oppit = m_query.find(oppositeKey);
            if(it == m_query.end() || oppit == m_query.end())
            {
                CONDUIT_ERROR("MatchQuery is missing the topologies for "
                    << domain << ":" << query_domain);
            }

            // Get both of the topologies.
            conduit::Node &mesh1 = it->second.query_mesh;
            conduit::Node &mesh2 = oppit->second.query_mesh;
            std::string topoKey("topologies/" + m_topoName);
            conduit::Node &topo1 = mesh1[topoKey];
            conduit::Node &topo2 = mesh2[topoKey];

            // Perform the search and store the results.
            it->second.results = conduit::blueprint::mesh::utils::topology::search(topo2, topo1);

#ifdef DEBUG_PRINT
            conduit::Node &n = debug["queries"].append();
            n["domain_id"] = it->first.first;
            n["query_domain"] = it->first.second;
            n["mesh"].set_external(mesh1);
            n["query_mesh"].set_external(mesh2);
            n["results"].set_external(it->second.results);
#endif
        }
    }
    CONDUIT_ANNOTATE_MARK_END("results");

#ifdef DEBUG_PRINT
    // Overwrite the old file with the recv results.
    conduit::relay::io::save(debug, filename, "yaml");
#endif
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh::utils::query --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mpi::mesh::utils::adjset --
//-----------------------------------------------------------------------------

bool
adjset::validate(const conduit::Node &doms,
                 const std::string &adjsetName,
                 conduit::Node &info,
                 const conduit::Node &options,
                 MPI_Comm comm)
{
    auto to_string = [](const conduit::Node &n) -> std::string
    {
        std::string s(n.to_string());
        if(s.find("\"") == 0)
            s = s.substr(1, s.size() - 2);
        return s;
    };

    auto agree = [](bool res, MPI_Comm acomm) -> bool
    {
        int size = 1;
        MPI_Comm_size(acomm, &size);
        int val = res ? 1 : 0;
        int globalval = 0;
        MPI_Allreduce(&val, &globalval, 1, MPI_INT, MPI_SUM, acomm);
        return globalval == size;
    };

    bool retval = false;

    // Set values from options.
    double tolerance = conduit::blueprint::mpi::mesh::utils::query::PointQuery::DEFAULT_POINT_TOLERANCE;
    if(options.has_path("tolerance"))
    {
        tolerance = options["tolerance"].to_float64();
    }

    // Get the domains.
    auto domains = conduit::blueprint::mesh::domains(doms);

    // We need to figure out the association, topologyName, and coordsetName.
    // Get them from the domains, if available.
    std::string association, topologyName, coordsetName;
    if(!domains.empty())
    {
        const auto dom = domains[0];

        std::string adjsetPath("adjsets/" + adjsetName);
        const conduit::Node &adjset = dom->fetch_existing(adjsetPath);

        association = to_string(adjset.fetch_existing("association"));
        topologyName = to_string(adjset.fetch_existing("topology"));

        const conduit::Node &topo = dom->fetch_existing("topologies/"+topologyName);
        coordsetName = to_string(topo["coordset"]);
    }

    // Some ranks might not have domains from which to figure out association,
    // etc. Make a YAML string with the answers if we have association. Then
    // allreduce on the characters so ranks with no data will get their buffers
    // updated. We use a fixed size buffer.
    constexpr int MAX_BUFFER_SIZE = 1024;
    char *local = new char[MAX_BUFFER_SIZE];
    memset(local, 0, sizeof(char) * MAX_BUFFER_SIZE);
    if(!association.empty())
    {
        std::stringstream ss;
        ss << "association: " << association << std::endl
           << "topology: " << topologyName << std::endl
           << "coordset: " << coordsetName << std::endl;
        std::string msg(ss.str());
        if(msg.size()+1 >= MAX_BUFFER_SIZE)
        {
            delete [] local;
            CONDUIT_ERROR("Message size exceeded max " << MAX_BUFFER_SIZE);
        }
        strncpy(local, msg.c_str(), msg.size());
    }
    char *global = new char[MAX_BUFFER_SIZE];
    memset(global, 0, sizeof(char) * MAX_BUFFER_SIZE);
    MPI_Allreduce(local, global, MAX_BUFFER_SIZE, MPI_CHAR, MPI_MAX, comm);
    global[MAX_BUFFER_SIZE - 1] = '\0';
    conduit::Node s;
    s.parse(global, "yaml");
    delete [] local;
    delete [] global;
    association = s["association"].as_string();
    topologyName = s["topology"].as_string();
    coordsetName = s["coordset"].as_string();

    // Make parallel queries and do the validation.
    conduit::blueprint::mpi::mesh::utils::query::PointQuery PQ(doms, comm);
    PQ.setPointTolerance(tolerance);
    conduit::blueprint::mpi::mesh::utils::query::MatchQuery MQ(doms, comm);

    // Do the validation.
    const bool checkMultiDomain = false;
    retval = conduit::blueprint::mesh::utils::adjset::validate(doms,
                 adjsetName, association, topologyName, coordsetName,
                 info, PQ, MQ, checkMultiDomain);

    // Make sure all ranks agree on the answer.
    retval = agree(retval, comm);
    return retval;
}

//-----------------------------------------------------------------------------
static void
adjset_extract_pointmesh(const conduit::Node &n_topo, const conduit::Node &n_ids, conduit::Node &n_output)
{
    namespace bputils = conduit::blueprint::mesh::utils;

    if(n_topo.fetch_existing("type").as_string() == "unstructured" &&
       n_topo.fetch_existing("elements/shape").as_string() == "line")
    {
        bputils::topology::extract_pointmesh(n_topo, n_ids, n_output);
    }
    else
    {
        bputils::topology::TopologyBuilder B(n_topo);
        const auto ids = n_ids.as_index_t_accessor();
        for(index_t i = 0; i < ids.number_of_elements(); i++)
        {
            index_t ptid = ids[i];
            B.add(&ptid, 1);
        }

        // Make the local point mesh.
        B.execute(n_output, "point");
    }
}

//-----------------------------------------------------------------------------
static bool
compare_pointwise_impl(conduit::Node &mesh, const std::string &adjsetName,
    conduit::Node &info, const conduit::Node &options, MPI_Comm comm)
{
    namespace bputils = conduit::blueprint::mesh::utils;
    std::vector<Node *> domains = conduit::blueprint::mesh::domains(mesh);

    // Set values from options.
    double tolerance = conduit::blueprint::mpi::mesh::utils::query::PointQuery::DEFAULT_POINT_TOLERANCE;
    if(options.has_path("tolerance"))
    {
        tolerance = options["tolerance"].to_float64();
    }

    // Determine total number of domains.
    conduit::Node nd_local, nd_total;
    nd_local = static_cast<int>(domains.size());
    relay::mpi::sum_all_reduce(nd_local, nd_total, comm);
    int maxDomains = nd_total.to_int();
    const int par_rank = relay::mpi::rank(comm);

    // Figure out which MPI ranks own each domain.
    conduit::Node n_domain2rank;
    {
        conduit::Node n_local(DataType::int32(maxDomains));
        int *iptr = n_local.as_int_ptr();
        memset(iptr, 0, sizeof(int) * maxDomains);
        for(size_t i = 0; i < domains.size(); i++)
        {
            auto domainId = bputils::find_domain_id(*domains[i]);
            iptr[domainId] = par_rank;
        }
        relay::mpi::sum_all_reduce(n_local, n_domain2rank, comm);
    }
    const int *domain2rank = n_domain2rank.as_int_ptr();

    // Iterate over each of the possible adjset relationships. Not all of these
    // will have adjset groups.
    const int tag = 122;
    for(int d0 = 0; d0 < maxDomains; d0++)
    {
        for(int d1 = d0 + 1; d1 < maxDomains; d1++)
        {
            // make the adjset group name.
            std::stringstream ss;
            ss << conduit::blueprint::mesh::adjset::group_prefix() << "_" << d0 << "_" << d1;
            std::string groupName(ss.str());

            // There are up to 2 local meshes and their corresponding remote mesh.
            relay::mpi::communicate_using_schema C(comm);
            conduit::Node localMesh[2], remoteMesh[2];
            int mi = 0;            
            for(auto dom_ptr : domains)
            {
                Node &domain = *dom_ptr;

                // If the domain has the adjset, make a point mesh of its points
                // that we can send to the neighbor.
                std::string key("adjsets/" + adjsetName + "/groups/" + groupName + "/values");
                if(domain.has_path(key))
                {
                    // Get the topology that the adjset wants.
                    std::string tkey("adjsets/" + adjsetName + "/topology");
                    std::string topoName = domain.fetch_existing(tkey).as_string();
                    const Node &topo = domain.fetch_existing("topologies/" + topoName);

                    // Get the group values and add them as points to the topo builder
                    // so we pull out a point mesh.
                    const Node &n_values = domain.fetch_existing(key);
                    adjset_extract_pointmesh(topo, n_values, localMesh[mi]);

                    // Get the neighbor for this group
                    std::string nkey("adjsets/" + adjsetName + "/groups/" + groupName + "/neighbors");
                    int neighbor = domain.fetch_existing(nkey).to_int();

                    // Send this local mesh to the neighbor.
                    C.add_isend(localMesh[mi], domain2rank[neighbor], tag + mi);
                    // That neighbor will have to send us a mesh too.
                    C.add_irecv(remoteMesh[mi], domain2rank[neighbor], tag + mi);

                    mi++;
                }
            }

            // Perform the exchange.
            C.execute();

            // Make sure the nodes are not different.
            Node different, reducedDiff;
            different = 0;
            for(int i = 0; i < mi; i++)
            {
                bool d = localMesh[i].diff(remoteMesh[i], info, tolerance);
                different = different.to_int() + (d ? 1 : 0);

                // Add some diagnostic info.
                if(d)
                {
                    info["adjset"] = adjsetName;
                    info["group"] = groupName;
                    break;
                }
            }
            relay::mpi::sum_all_reduce(different, reducedDiff, comm);
            if(reducedDiff.to_int() > 0)
                return false;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
bool
adjset::compare_pointwise(conduit::Node &mesh, const std::string &adjsetName,
    conduit::Node &info, const conduit::Node &options, MPI_Comm comm)
{
    bool retval = true;
    const std::string tempAdjsetName("__" + adjsetName + "__");

    try
    {
        // Make sure we have a suitable adjset.
        conduit::blueprint::mesh::utils::adjset::to_pairwise_canonical(mesh, adjsetName, tempAdjsetName);

        // Call the real implementation on the temporary adjset.
        retval = compare_pointwise_impl(mesh, tempAdjsetName, info, options, comm);

        // Remove the adjset that was added.
        conduit::blueprint::mesh::utils::adjset::remove(mesh, tempAdjsetName);
    }
    catch(...)
    {
        // Remove the adjset that was added.
        conduit::blueprint::mesh::utils::adjset::remove(mesh, tempAdjsetName);
        // Rethrow the exception.
        throw;
    }

    return retval;
}

//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh::utils::adjset --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh::utils --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mpi::mesh::utils --
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
