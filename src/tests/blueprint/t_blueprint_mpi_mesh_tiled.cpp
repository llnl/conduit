// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_mesh_tiled.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_examples.hpp"
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_blueprint_mpi_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"
#include "conduit_log.hpp"

#include "conduit_fmt/conduit_fmt.h"

#include "blueprint_test_helpers.hpp"
#include "blueprint_mpi_test_helpers.hpp"

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

#include <mpi.h>

#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
using namespace generate;

// Uncomment if we want to write the data files.
//#define CONDUIT_WRITE_TEST_DATA

// Uncomment for more verbose test output during debugging.
//#define CONDUIT_DEBUG_TEST

//---------------------------------------------------------------------------
#ifdef CONDUIT_WRITE_TEST_DATA
/**
 @brief Save the node to an HDF5 compatible with VisIt or the
        conduit_adjset_validate tool.
 */
void save_mesh(const conduit::Node &root, const std::string &filebase)
{
    // NOTE: Enable this to write files for debugging.
    const std::string protocol("hdf5");
    conduit::relay::mpi::io::blueprint::save_mesh(root, filebase, protocol, MPI_COMM_WORLD);
}
#endif

//-----------------------------------------------------------------------------
template <typename T>
std::ostream &operator << (std::ostream &os, const std::vector<T> &vec)
{
    os << "{";
    for(const T &value : vec)
        os << value << ", ";
    os << "}";
    return os;
}

//-----------------------------------------------------------------------------
/**
 @brief Make domains for a tiled mesh. The way this is being done, it will use
        the original tiler suitable for weak scaling.

 @param mesh The node that will contain the domains.
 @param dims The number of zones in each domain. dims[2] == 0 for 2D meshes.
 @param domains The number of domains to make. All values > 0.
 @param reorder The reordering method, if any.
 @param domainNumbering An optional vector that can reorder the domains.
 */
void
make_tiled(conduit::Node &mesh, const int dims[3], const int domains[3],
           const std::string &reorder, const std::vector<int> &domainNumbering)
{
    const int ndoms = domains[0] * domains[1] * domains[2];
    const int par_rank = relay::mpi::rank(MPI_COMM_WORLD);
    const int par_size = relay::mpi::size(MPI_COMM_WORLD);
    std::vector<int> domains_per_rank(par_size, 0);
    for(int di = 0; di < ndoms; di++)
        domains_per_rank[di % par_size]++;
    int offset = 0;
    for(int i = 0; i < par_rank; i++)
        offset += domains_per_rank[i];

    // Make domains.
#ifdef CONDUIT_DEBUG_TEST
    std::stringstream ss;
#endif
    const double extents[] = {0., 1., 0., 1., 0., 1.};
    int domainIndex = 0;
    for(int k = 0; k < domains[2]; k++)
    for(int j = 0; j < domains[1]; j++)
    for(int i = 0; i < domains[0]; i++, domainIndex++)
    {
        int domain[] = {i,j,k};

        int domainId = domainIndex;
        if(!domainNumbering.empty())
            domainId = domainNumbering[domainIndex];

        // See if we need to make the domain on this rank.
        if(domainId >= offset && domainId < offset + domains_per_rank[par_rank])
        {
            // Determine the size and location of this domain in the whole.
            double sideX = (extents[1] - extents[0]) / static_cast<double>(domains[0]);
            double sideY = (extents[3] - extents[2]) / static_cast<double>(domains[1]);
            double sideZ = (extents[5] - extents[4]) / static_cast<double>(domains[2]);
            double domainExt[] = {extents[0] + domain[0]     * sideX,
                                  extents[0] + (domain[0]+1) * sideX,
                                  extents[2] + domain[1]     * sideY,
                                  extents[2] + (domain[1]+1) * sideY,
                                  extents[4] + domain[2]     * sideZ,
                                  extents[4] + (domain[2]+1) * sideZ};
            conduit::Node opts;
            opts["extents"].set(domainExt, 6);
            opts["domain"].set(domain, 3);
            opts["domains"].set(domains, 3);
            opts["reorder"] = reorder;

#ifdef CONDUIT_DEBUG_TEST
            ss << "Rank " << par_rank << " makes domain " << domainId
               << ", ijk={" << domain[0] << ", " << domain[1] << ", " << domain[2] << "}"
               << ", extents={" << domainExt[0] << ", " << domainExt[1]
               << ", " << domainExt[2] << ", " << domainExt[3]
               << ", " << domainExt[4] << ", " << domainExt[5] << "}"
               << ", reorder=\"" << reorder << "\"\n";
#endif
            if(ndoms > 1)
            {
                std::string domainName = conduit_fmt::format("domain_{:07}",domainId);
                conduit::Node &dom = mesh[domainName];
                conduit::blueprint::mesh::examples::tiled(dims[0], dims[1], dims[2], dom, opts);
            }
            else
            {
                conduit::blueprint::mesh::examples::tiled(dims[0], dims[1], dims[2], mesh, opts);
            }
        }
    }

#ifdef CONDUIT_DEBUG_TEST
    // Print some build messages.
    for(int rank = 0; rank < par_size; rank++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == par_rank)
        {
           std::cout << ss.str();
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
#endif
}

//-----------------------------------------------------------------------------
template <typename Func>
void
foreach_permutation_impl(int level, std::vector<int> &values, Func &&func)
{
    int n = static_cast<int>(values.size());
    if(level < n)
    {
        int n = static_cast<int>(values.size());
        for(int i = 0; i < n; i++)
        {
            const int *start = &values[0];
            const int *end = start + level;
            if(std::find(start, end, i) == end)
            {
                values[level] = i;
                foreach_permutation_impl(level + 1, values, func);
            }
        }
    }
    else
    {
        func(values);
    }
}

template <typename Func>
void
foreach_permutation(int maxval, Func &&func)
{
    std::vector<int> values(maxval, -1);
    foreach_permutation_impl(0, values, func);
}

//-----------------------------------------------------------------------------
void
test_tiled_adjsets(const int dims[3], const std::string &testName)
{
    // Make 4 domains but alter their locations.
    foreach_permutation(4, [&](const std::vector<int> &domainNumbering)
    {
        const int domains[] = {2,2,1};
        const std::vector<std::string> reorder{"normal", "kdtree"};
        for(const auto &r : reorder)
        {
            // Make the mesh.
            conduit::Node mesh;
            make_tiled(mesh, dims, domains, r, domainNumbering);

            // Now, make a corner mesh
            conduit::Node s2dmap, d2smap;
            conduit::blueprint::mpi::mesh::generate_corners(mesh,
                                                            "mesh_adjset",
                                                            "corner_adjset",
                                                            "corner_mesh",
                                                            "corner_coords",
                                                            s2dmap,
                                                            d2smap,
                                                            MPI_COMM_WORLD);
            // Convert to pairwise adjset.
            std::vector<conduit::Node *> doms = conduit::blueprint::mesh::domains(mesh);
            for(auto dom_ptr : doms)
            {
               conduit::Node &domain = *dom_ptr;
               conduit::blueprint::mesh::adjset::to_pairwise(domain["adjsets/corner_adjset"],
                                                             domain["adjsets/corner_pairwise_adjset"]);
            }

#ifdef CONDUIT_WRITE_TEST_DATA
            // Save the mesh.
            std::stringstream ss;
            ss << "_r" << r;
            for(const auto &value : domainNumbering)
                ss << "_" << value;
            std::string filebase(testName + ss.str());
            save_mesh(mesh, filebase);
#endif

            // Check that its adjset points are the same along the edges.
            conduit::Node info;
            bool same = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(mesh, "mesh_adjset", info);
            in_rank_order(MPI_COMM_WORLD, [&](int rank) {
                if(!same)
                {
                    if(info.number_of_children() > 0 && info["valid"].as_string() == "false")
                    {
                        std::cout << "Rank " << rank << ": mesh_adjset was different."
                                  << " domainNumbering=" << domainNumbering
                                  << ", reorder=" << r
                                  << std::endl;
                        info.print();
                    }
                }
            });
            EXPECT_TRUE(same);

            // Check that its adjset points are the same along the edges.
            info.reset();
            same = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(mesh, "corner_pairwise_adjset", info);
            in_rank_order(MPI_COMM_WORLD, [&](int rank) {
                if(!same)
                {
                    if(info.number_of_children() > 0 && info["valid"].as_string() == "false")
                    {
                        std::cout << "Rank " << rank << ": corner_pairwise_adjset was different."
                                  << " domainNumbering=" << domainNumbering
                                  << ", reorder=" << r
                                  << std::endl;
                        info.print();
                    }
                }
            });
            EXPECT_TRUE(same);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_tiled, two_dimensional)
{
    const int dims[]= {3,3,0};
    test_tiled_adjsets(dims, "two_dimensional");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_tiled, three_dimensional)
{
    const int dims[]= {2,2,2};
    test_tiled_adjsets(dims, "three_dimensional");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_tiled, three_dimensional_12)
{
    // This 12 domain case was found to cause adjset problems.
    const int dims[] = {2,2,2}, domains[] = {3,2,2};
    const std::vector<std::string> reorder{"normal", "kdtree"};

    for(const auto &r : reorder)
    {
        // Make the mesh.
        conduit::Node mesh;
        make_tiled(mesh, dims, domains, r, std::vector<int>{});

        // Now, make a corner mesh
        conduit::Node s2dmap, d2smap;
        conduit::blueprint::mpi::mesh::generate_corners(mesh,
                                                        "mesh_adjset",
                                                        "corner_adjset",
                                                        "corner_mesh",
                                                        "corner_coords",
                                                        s2dmap,
                                                        d2smap,
                                                        MPI_COMM_WORLD);
        // Convert to pairwise adjset.
        std::vector<conduit::Node *> doms = conduit::blueprint::mesh::domains(mesh);
        for(auto dom_ptr : doms)
        {
            conduit::Node &domain = *dom_ptr;
            conduit::blueprint::mesh::adjset::to_pairwise(domain["adjsets/corner_adjset"],
                                                          domain["adjsets/corner_pairwise_adjset"]);
        }

#ifdef CONDUIT_WRITE_TEST_DATA
        // Save the mesh.
        const std::string testName("three_dimensional_12");
        std::stringstream ss;
        ss << "_r_" << r;
        std::string filebase(testName + ss.str());
        save_mesh(mesh, filebase);
#endif

        // Check that its adjset points are the same along the edges.
        conduit::Node info;
        bool same = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(mesh, "corner_pairwise_adjset", info);
        in_rank_order(MPI_COMM_WORLD, [&](int rank) {
            if(!same)
            {
                if(info.number_of_children() > 0)
                {
                    std::cout << "Rank " << rank << ": corner_pairwise_adjset was different."
                              << " reorder=" << r
                              << std::endl;
                    info.print();
                }
            }
        });
        EXPECT_TRUE(same);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, topdown_17_domain)
{
    // This test case tries the topdown tiler on 17 domains in a configuration
    // that once caused there to be too few groups in the adjsets. That is fixed
    // but test for it.
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    EXPECT_EQ(size, 4);

    // Spread out the 17 domains over the 4 MPI ranks.
    const std::vector<int> selectedDomains[] = {
        {0,1,2,3,4},
        {5,6,7,8},
        {9,10,11,12},
        {13,14,15,16}
    };

    // Make a 1 quad tile so we get hex zones.
    // This is a simpler tile to speed up testing.
    const char *tile = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0., 1., 1., 0.]
      y: [0., 0., 1., 1.]
topologies:
  tile:
    type: unstructured
    coordset: coords
    elements:
      shape: quad
      connectivity: [0,1,2,3]
left: [0,3]
right: [1,2]
bottom: [0,1]
top: [3,2]
translate:
   x: 1.
   y: 1.
)";

    // This uses the tiler to make a mesh that is divided top-down.
    conduit::index_t dims[] = {52, 59, 61};
    conduit::Node mesh, opts;
    opts["tile"].parse(tile);
    opts["numDomains"] = 17; // This selects the topdown tiler.
    opts["curveSplitting"] = 0;
    opts["meshname"] = "main";
    opts["selectedDomains"].set(selectedDomains[rank]);
    conduit::blueprint::mesh::examples::tiled(dims[0], dims[1], dims[2], mesh, opts);

    // Make sure each rank made the right number of domains.
    EXPECT_EQ(selectedDomains[rank].size(), conduit::blueprint::mesh::domains(mesh).size());

    // Make sure that the adjset is compares pointwise.
    conduit::Node info;
    bool same = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(mesh, "main_adjset", info);
    in_rank_order(MPI_COMM_WORLD, [&](int rank)
    {
        if(!same)
        {
            if(info.number_of_children() > 0)
            {
                std::cout << "Rank " << rank << ": main_adjset was different." << std::endl;
                info.print();
            }
        }
    });
    EXPECT_TRUE(same);

    // Generate faces for the mesh.
    conduit::Node s2dmap, d2smap;
    conduit::blueprint::mpi::mesh::generate_faces(mesh,
                                                  "main_adjset",
                                                  "face_adjset",
                                                  "main_faces",
                                                  s2dmap,
                                                  d2smap,
                                                  MPI_COMM_WORLD);

    // Read through the face_adjset groups and make a map of neighbors per domain.
    auto make_dom_neighbors = [](const conduit::Node &mesh)
    {
        std::map<conduit::index_t, std::set<conduit::index_t>> dom_neighbors;
        const auto doms = conduit::blueprint::mesh::domains(mesh);
        for(const auto &domPtr : doms)
        {
            const conduit::Node &dom = *domPtr;
            auto domain_id = dom["state/domain_id"].to_int();
            auto it = dom_neighbors.find(domain_id);
            if(it == dom_neighbors.end())
            {
                dom_neighbors[domain_id] = std::set<conduit::index_t>();
                it = dom_neighbors.find(domain_id);
            }
            const auto &groups = dom["adjsets/face_adjset/groups"];
            for(conduit::index_t i = 0; i < groups.number_of_children(); i++)
            {
                const auto &group = groups[i];
                const auto neighbors = group["neighbors"].as_int_accessor();
                for(conduit::index_t j = 0; j < neighbors.number_of_elements(); j++)
                {
                    it->second.insert(neighbors[j]);
                }
            }
        }
        return dom_neighbors;
    };

    // Make sure the face adjset has the right neighbors on each domain.
    std::map<conduit::index_t, std::set<conduit::index_t>> dom_neighbors, baseline;
#if 0
    // Make this section to compile to rebaseline.
    in_rank_order(MPI_COMM_WORLD, [&](int rank)
    {
        // Print out the adjsets for each domain.
        baseline = make_dom_neighbors(mesh);
        for(auto it = baseline.begin(); it != baseline.end(); it++)
        {
            std::cout << "    baseline[" << it->first << "] = std::set<conduit::index_t>{";
            for(const auto &value : it->second)
                std::cout << value << ", ";
            std::cout << "};" << std::endl;
        }
    });
#else
    // NOTE: These values are pasted in from the above code.
    baseline[0] = std::set<conduit::index_t>{1, 3, 9};
    baseline[1] = std::set<conduit::index_t>{0, 2, 4, 9, 10};
    baseline[2] = std::set<conduit::index_t>{1, 5, 10};
    baseline[3] = std::set<conduit::index_t>{0, 4, 6, 9, 11};
    baseline[4] = std::set<conduit::index_t>{1, 3, 5, 7, 9, 10, 11, 12};
    baseline[5] = std::set<conduit::index_t>{2, 4, 8, 10, 12};
    baseline[6] = std::set<conduit::index_t>{3, 7, 11};
    baseline[7] = std::set<conduit::index_t>{4, 6, 8, 11, 12};
    baseline[8] = std::set<conduit::index_t>{5, 7, 12};
    baseline[9] = std::set<conduit::index_t>{0, 1, 3, 4, 10, 11, 13};
    baseline[10] = std::set<conduit::index_t>{1, 2, 4, 5, 9, 12, 14};
    baseline[11] = std::set<conduit::index_t>{3, 4, 6, 7, 9, 12, 15};
    baseline[12] = std::set<conduit::index_t>{4, 5, 7, 8, 10, 11, 16};
    baseline[13] = std::set<conduit::index_t>{9, 14, 15};
    baseline[14] = std::set<conduit::index_t>{10, 13, 16};
    baseline[15] = std::set<conduit::index_t>{11, 13, 16};
    baseline[16] = std::set<conduit::index_t>{12, 14, 15};
#endif
    dom_neighbors = make_dom_neighbors(mesh);
    for(const auto &domainId : selectedDomains[rank])
    {
        EXPECT_EQ(dom_neighbors[domainId], baseline[domainId]);
    }
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}
