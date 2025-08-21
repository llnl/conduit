// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_mesh_transform.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mpi.hpp"
#include "conduit_relay_mpi.hpp"
#include "conduit_blueprint_mpi_mesh_utils.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;

/// Testing Constants ///

typedef void (*GenDerivedFun)(Node&, const std::string&, const std::string&, const std::string&, Node&, Node&, MPI_Comm);
typedef void (*GenDecomposedFun)(Node&, const std::string&, const std::string&, const std::string&, const std::string&, Node&, Node&, MPI_Comm);

static const index_t TYPE_POINT_ID = 0;
static const index_t TYPE_LINE_ID = 1;
static const index_t TYPE_FACE_ID = 2;
static const index_t TYPE_CENTROID_ID = 3;
static const index_t TYPE_SIDE_ID = 4;
static const index_t TYPE_CORNER_ID = 5;

static const auto IS_DERIVED_TYPE = [] (const index_t type_id)
{
    return type_id < TYPE_CENTROID_ID;
};
static const auto IS_DECOMPOSED_TYPE = [] (const index_t type_id)
{
    return type_id >= TYPE_CENTROID_ID;
};

static const std::vector<std::string> TYPE_NAMES = {
    "point",    // 0
    "line",     // 1
    "face",     // 2
    "cent",     // 3
    "side",     // 4
    "corner"    // 5
};
static const std::vector<std::string> TYPE_SHAPES = {
    "point",    // 0
    "line",     // 1
    "quad",     // 2
    "point",    // 3
    "tet",      // 4
    "polygonal" // 5
};
static const std::vector<GenDerivedFun> DERIVED_TYPE_FUNS = {
    conduit::blueprint::mpi::mesh::generate_points,   // 0
    conduit::blueprint::mpi::mesh::generate_lines,    // 1
    conduit::blueprint::mpi::mesh::generate_faces,    // 2
    nullptr,                                          // 3
    nullptr,                                          // 4
    nullptr                                           // 5
};
static const std::vector<GenDecomposedFun> DECOMPOSED_TYPE_FUNS = {
    nullptr,                                          // 0
    nullptr,                                          // 1
    nullptr,                                          // 2
    nullptr,                                          // 3
    conduit::blueprint::mpi::mesh::generate_sides,    // 4
    conduit::blueprint::mpi::mesh::generate_corners   // 5
};

/// Testing Helpers ///

// Utility Functions //

std::vector<index_t> to_index_vector(const Node &n)
{
    std::vector<index_t> index_vector;

    Node data;
    for(index_t ni = 0; ni < n.dtype().number_of_elements(); ni++)
    {
        data.set_external(DataType(n.dtype().id(), 1), (void*)n.element_ptr(ni));
        index_vector.push_back(data.to_index_t());
    }

    return std::vector<index_t>(std::move(index_vector));
}

std::set<index_t> to_index_set(const Node &n)
{
    const std::vector<index_t> index_vector = to_index_vector(n);
    return std::set<index_t>(index_vector.begin(), index_vector.end());
}

std::set<std::set<index_t>> to_neighbor_set(const Node &groups)
{
    // <<group 0 domain ids>, <group 1 domain ids>, ...}
    std::set<std::set<index_t>> neighbor_set;

    NodeConstIterator group_itr = groups.children();
    while(group_itr.has_next())
    {
        const Node &group = group_itr.next();
        neighbor_set.insert(to_index_set(group["neighbors"]));
    }

    return std::set<std::set<index_t>>(std::move(neighbor_set));
}

std::map<std::set<index_t>, std::set<index_t>> to_group_map(const Node &groups)
{
    // {<domain ids>: <entity index list>, ...}
    std::map<std::set<index_t>, std::set<index_t>> group_map;

    NodeConstIterator group_itr = groups.children();
    while(group_itr.has_next())
    {
        const Node &group = group_itr.next();
        group_map[to_index_set(group["neighbors"])] = to_index_set(group["values"]);
    }

    return std::map<std::set<index_t>, std::set<index_t>>(std::move(group_map));
}

// Setup Functions //

void setup_test_mesh_paths(const index_t type,
                           const Node &mesh,
                           Node &info)
{
    info.reset();

    info["src/cset"].set(mesh.child(0).fetch("coordsets").child_names()[0]);
    info["src/topo"].set(mesh.child(0).fetch("topologies").child_names()[0]);
    info["src/aset"].set(mesh.child(0).fetch("adjsets").child_names()[0]);
    info["dst/cset"].set(IS_DERIVED_TYPE(type) ? info["src/cset"].as_string() : "centroids");
    info["dst/topo"].set(TYPE_NAMES[type]);
    info["dst/aset"].set(TYPE_NAMES[type] + "_adj");
}

void setup_test_mesh(const index_t type,
                     const size_t ndims,
                     const size_t dims,
                     Node &rank_mesh,
                     Node &rank_paths,
                     Node &rank_s2dmap,
                     Node &rank_d2smap,
                     Node &full_mesh)
{
    rank_mesh.reset();
    full_mesh.reset();

    conduit::blueprint::mesh::examples::grid(
        (ndims == 3) ? "hexs" : "quads",
        dims, dims, (ndims == 3) ? dims : 0,
        2, 2, 1, // nx,ny,nz domains
        full_mesh);
    setup_test_mesh_paths(type, full_mesh, rank_paths);

    const int par_rank = relay::mpi::rank(MPI_COMM_WORLD);
    const int par_size = relay::mpi::size(MPI_COMM_WORLD);

    // full_mesh contains 4 domains that we need to assign to ranks.
    // Make sure all domains get assigned.
    const int domain_to_rank[4][4] = {
        {0, 0, 0, 0},
        {0, 0, 1, 1},
        {0, 0, 1, 2},
        {0, 1, 2, 3}
    };
    int pIdx = std::min(par_size, 4) - 1;
    for(int dom = 0; dom < 4; dom++)
    {
        if(domain_to_rank[pIdx][dom] == par_rank)
        {
            std::ostringstream oss;
            oss << "domain" << dom;
            const std::string domain_name = oss.str();
            rank_mesh.append().set_external(full_mesh[domain_name]);
        }
    }

    if(DERIVED_TYPE_FUNS[type] != nullptr)
    {
        DERIVED_TYPE_FUNS[type](rank_mesh,
                                rank_paths["src/aset"].as_string(),
                                rank_paths["dst/aset"].as_string(),
                                rank_paths["dst/topo"].as_string(),
                                rank_s2dmap,
                                rank_d2smap,
                                MPI_COMM_WORLD);
    }
    else if(DECOMPOSED_TYPE_FUNS[type] != nullptr)
    {
        DECOMPOSED_TYPE_FUNS[type](rank_mesh,
                                   rank_paths["src/aset"].as_string(),
                                   rank_paths["dst/aset"].as_string(),
                                   rank_paths["dst/topo"].as_string(),
                                   rank_paths["dst/cset"].as_string(),
                                   rank_s2dmap,
                                   rank_d2smap,
                                   MPI_COMM_WORLD);
    }
}

// Test Functions //

void test_mesh_paths(const index_t type,
                     const Node &rank_mesh,
                     const Node &rank_paths)
{
    const std::vector<const Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    for(const Node *domain_ptr : domains)
    {
        const Node &domain = *domain_ptr;

        // Verify Paths //
        EXPECT_TRUE(domain["adjsets"].has_path(rank_paths["dst/aset"].as_string()));
        EXPECT_TRUE(domain["topologies"].has_path(rank_paths["dst/topo"].as_string()));
        EXPECT_TRUE(domain["coordsets"].has_path(rank_paths["dst/cset"].as_string()));

        // Verify Path Pointers //
        EXPECT_EQ(domain["adjsets"][rank_paths["dst/aset"].as_string()]["topology"].as_string(),
                  rank_paths["dst/topo"].as_string());
        EXPECT_EQ(domain["topologies"][rank_paths["dst/topo"].as_string()]["coordset"].as_string(),
                  rank_paths["dst/cset"].as_string());

        // Verify Enum Values //
        EXPECT_EQ(domain["adjsets"][rank_paths["dst/aset"].as_string()]["association"].as_string(),
                  IS_DERIVED_TYPE(type) ? "element" : "vertex");
        EXPECT_EQ(domain["topologies"][rank_paths["dst/topo"].as_string()]["elements/shape"].as_string(),
                  TYPE_SHAPES[type]);
    }
}

// TODO(JRC): Validate that points/entities match up across processor boundaries
// by performing a communication step and doing a list comparison.
// TODO(JRC): Validate 's2d' and 'd2s' maps with another test function, and then
// put this function alongside 'test_mesh_paths' in all the test cases.
// TODO(JRC): Add a test case for input meshes that have an adjsets entry
// with no groups (e.g. 1-rank case).
// TODO(JRC): Add a test case to ensure that per-dimension fuzz factor comparisons
// are working as expected in parallel 'generate_*' functions.

/// Transform Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_points)
{
    const index_t TEST_TYPE_ID = TYPE_POINT_ID;
    const index_t TEST_MESH_NDIM = 2;
    const index_t TEST_MESH_RES = 3;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Data //
            const std::map<std::set<index_t>, std::set<index_t>> src_group_map = to_group_map(src_aset_groups);
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            ASSERT_EQ(dst_group_map.size(), src_group_map.size());
            ASSERT_EQ(dst_group_map, src_group_map);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_lines)
{
    const index_t TEST_TYPE_ID = TYPE_LINE_ID;
    const index_t TEST_MESH_NDIM = 2;
    const index_t TEST_MESH_RES = 3;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            // const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Count //
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            ASSERT_EQ(dst_group_map.size(), 2); // NOTE(JRC): 2 interfaces per domain in the grid

            // Verify Group Data //
            for(const auto &group_pair : dst_group_map)
            {
                const std::set<index_t> &group_neighbors = group_pair.first;
                const std::set<index_t> &group_values = group_pair.second;
                ASSERT_EQ(group_neighbors.size(), 1);
                ASSERT_EQ(group_values.size(), TEST_MESH_RES - 1);
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_faces)
{
    const index_t TEST_TYPE_ID = TYPE_FACE_ID;
    const index_t TEST_MESH_NDIM = 3;
    const index_t TEST_MESH_RES = 3;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            // const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Count //
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            ASSERT_EQ(dst_group_map.size(), 2); // NOTE(JRC): 2 interfaces per domain in the grid

            // Verify Group Data //
            for(const auto &group_pair : dst_group_map)
            {
                const std::set<index_t> &group_neighbors = group_pair.first;
                const std::set<index_t> &group_values = group_pair.second;
                ASSERT_EQ(group_neighbors.size(), 1);
                ASSERT_EQ(group_values.size(), (TEST_MESH_RES - 1) * (TEST_MESH_RES - 1));
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_centroids)
{
    const index_t TEST_TYPE_ID = TYPE_CENTROID_ID;
    const index_t TEST_MESH_NDIM = 2;
    const index_t TEST_MESH_RES = 3;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    { // Initialize Centroid Topology //
        const std::string mid_topo_name = "line";
        const std::string mid_aset_name = "line_adj";

        conduit::blueprint::mpi::mesh::generate_lines(rank_mesh,
                                                      rank_paths["src/aset"].as_string(),
                                                      mid_aset_name,
                                                      mid_topo_name,
                                                      rank_s2dmap,
                                                      rank_d2smap,
                                                      MPI_COMM_WORLD);

        // TODO(JRC): Remove this hack once the 'generate_*' functions support adjsets
        // for which "association" is "element".
        const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);
        for(Node *domain_ptr : domains)
        {
            Node &domain = *domain_ptr;
            domain["adjsets"][mid_aset_name].set(domain["adjsets"][rank_paths["src/aset"].as_string()]);
            domain["adjsets"][mid_aset_name]["topology"].set(mid_topo_name);
        }

        conduit::blueprint::mpi::mesh::generate_centroids(rank_mesh,
                                                          mid_aset_name,
                                                          rank_paths["dst/aset"].as_string(),
                                                          rank_paths["dst/topo"].as_string(),
                                                          rank_paths["dst/cset"].as_string(),
                                                          rank_s2dmap,
                                                          rank_d2smap,
                                                          MPI_COMM_WORLD);
    }
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    // TODO(JRC): Add more thorough testing to ensure that centroids are properly generated
    // and used, etc.

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            // const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Count //
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            ASSERT_EQ(dst_group_map.size(), 2); // NOTE(JRC): 2 interfaces per domain in the grid

            // Verify Group Data //
            for(const auto &group_pair : dst_group_map)
            {
                const std::set<index_t> &group_neighbors = group_pair.first;
                const std::set<index_t> &group_values = group_pair.second;
                ASSERT_EQ(group_neighbors.size(), 1);
                ASSERT_EQ(group_values.size(), TEST_MESH_RES - 1);
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_sides)
{
    const index_t TEST_TYPE_ID = TYPE_SIDE_ID;
    const index_t TEST_MESH_NDIM = 3;
    const index_t TEST_MESH_RES = 2;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Counts/Participants //
            const std::set<std::set<index_t>> src_neighbor_set = to_neighbor_set(src_aset_groups);
            const std::set<std::set<index_t>> dst_neighbor_set = to_neighbor_set(dst_aset_groups);
            ASSERT_EQ(dst_neighbor_set.size(), src_neighbor_set.size());
            ASSERT_EQ(dst_neighbor_set, src_neighbor_set);

            // Verify Group Data //
            const std::map<std::set<index_t>, std::set<index_t>> src_group_map = to_group_map(src_aset_groups);
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            for(const auto &group_pair : dst_group_map)
            {
                const std::set<index_t> &group_neighbors = group_pair.first;
                const std::set<index_t> &dst_group_values = group_pair.second;
                const std::set<index_t> &src_group_values = src_group_map.at(group_neighbors);

                if(group_neighbors.size() == 1)
                {
                    // NOTE(JRC): The number of shared points along an individual boundary is
                    // the number of existing shared points plus the number of shared faces
                    ASSERT_EQ(dst_group_values.size(),
                              src_group_values.size() + (TEST_MESH_RES - 1) * (TEST_MESH_RES - 1));
                }
                else // if(group_neighbors.size() == 3)
                {
                    // NOTE(JRC): The number of points shared by all domains is just the number
                    // of points along the z-axis.
                    ASSERT_EQ(dst_group_values.size(), TEST_MESH_RES);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_transform, generate_corners)
{
    const index_t TEST_TYPE_ID = TYPE_CORNER_ID;
    const index_t TEST_MESH_NDIM = 2;
    const index_t TEST_MESH_RES = 2;

    Node rank_mesh, full_mesh, info;
    Node rank_paths, rank_s2dmap, rank_d2smap;

    setup_test_mesh(TEST_TYPE_ID, TEST_MESH_NDIM, TEST_MESH_RES,
        rank_mesh, rank_paths, rank_s2dmap, rank_d2smap, full_mesh);
    rank_mesh.print();
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh", rank_mesh, info, MPI_COMM_WORLD));

    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(rank_mesh);

    { // Sanity Tests //
        test_mesh_paths(TEST_TYPE_ID, rank_mesh, rank_paths);
    }

    { // Adjacency Tests //
        for(Node *domain_ptr : domains)
        {
            const Node &domain = *domain_ptr;

            const Node &src_aset_groups = domain["adjsets"][rank_paths["src/aset"].as_string()]["groups"];
            const Node &dst_aset_groups = domain["adjsets"][rank_paths["dst/aset"].as_string()]["groups"];

            // Verify Group Counts/Participants //
            const std::set<std::set<index_t>> src_neighbor_set = to_neighbor_set(src_aset_groups);
            const std::set<std::set<index_t>> dst_neighbor_set = to_neighbor_set(dst_aset_groups);
            ASSERT_EQ(dst_neighbor_set.size(), src_neighbor_set.size());
            ASSERT_EQ(dst_neighbor_set, src_neighbor_set);

            // Verify Group Data //
            const std::map<std::set<index_t>, std::set<index_t>> src_group_map = to_group_map(src_aset_groups);
            const std::map<std::set<index_t>, std::set<index_t>> dst_group_map = to_group_map(dst_aset_groups);
            for(const auto &group_pair : dst_group_map)
            {
                const std::set<index_t> &group_neighbors = group_pair.first;
                const std::set<index_t> &dst_group_values = group_pair.second;
                const std::set<index_t> &src_group_values = src_group_map.at(group_neighbors);

                if(group_neighbors.size() == 1)
                {
                    ASSERT_EQ(dst_group_values.size(), src_group_values.size() * 2);
                }
                else // if(group_neighbors.size() == 3)
                {
                    ASSERT_EQ(dst_group_values.size(), 1);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
generate_wonky_mesh(conduit::Node &mesh)
{
    // There is a 3x3x3 zone mesh that was giving generate_corners a problem
    // due to adjacency sets.

    // Domain 0
    const char *dom0_str = R"(
coordsets:
  coords:
    type: "explicit"
    values:
      x: [0.3333, 0.3333, 0.3333, 0.3333, 0.3333, 0.0, 0.3333, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.3333, 0.0, 0.6666, 0.3333, 0.3333, 0.6666, 0.6666, 0.3333, 0.0, 0.6666, 0.3333, 0.0, 0.6666, 1.0, 1.0, 1.0, 1.0]
      y: [0.3333, 0.0, 0.3333, 0.0, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.6666, 0.3333, 0.0, 0.0, 0.3333, 0.3333, 0.6666, 0.6666, 0.3333, 0.3333, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.3333, 0.0, 0.3333, 0.0]
      z: [0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.3333, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.0, 0.6666, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.3333, 0.3333, 0.3333, 0.3333, 0.0, 0.0]
topologies:
  main:
    type: "unstructured"
    coordset: "coords"
    elements:
      shape: "hex"
      connectivity: [7, 8, 0, 4, 9, 10, 2, 6, 8, 11, 1, 0, 10, 12, 3, 2, 20, 17, 18, 19, 21, 13, 14, 15, 22, 15, 16, 23, 25, 4, 5, 26, 24, 21, 15, 22, 27, 7, 4, 25, 21, 13, 14, 15, 7, 8, 0, 4, 28, 29, 11, 8, 30, 31, 12, 10]
      offsets: [0, 8, 16, 24, 32, 40, 48]
adjsets:
  main_adjset:
    association: "vertex"
    topology: "main"
    groups:
      group_0_1:
        neighbors: 1
        values: [2, 3, 5, 26]
      group_0_3:
        neighbors: 3
        values: [7, 8, 9, 10, 11, 27, 28, 29, 30]
      group_0_2:
        neighbors: 2
        values: [17, 20, 22, 23]
      group_0_1_3:
        neighbors: [1, 3]
        values: [0, 1, 4, 6, 25]
      group_0_2_3:
        neighbors: [2, 3]
        values: [13, 21, 24]
      group_0_1_2:
        neighbors: [1, 2]
        values: [15, 16, 18, 19]
      group_0_1_2_3:
        neighbors: [1, 2, 3]
        values: 14
state:
  domain_id: 0
)";

    // Domain 1
    const char *dom1_str = R"(
coordsets:
  coords:
    type: "explicit"
    values:
      x: [0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.0, 0.3333, 0.0, 0.3333, 0.3333, 0.3333, 0.0, 0.0, 0.0, 0.3333, 0.0, 0.3333, 0.0, 0.3333, 0.0, 0.3333, 0.0]
      y: [0.3333, 0.0, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.6666, 0.6666, 0.6666, 0.6666, 0.0, 0.3333, 0.6666, 0.3333, 0.6666, 0.0, 0.3333, 0.3333, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0]
      z: [0.3333, 0.3333, 0.3333, 0.3333, 0.0, 0.0, 0.0, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 0.3333, 0.3333, 0.0, 0.0]
topologies:
  main:
    type: "unstructured"
    coordset: "coords"
    elements:
      shape: "hex"
      connectivity: [0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 3, 9, 10, 4, 7, 11, 14, 13, 15, 16, 8, 0, 3, 9, 13, 12, 17, 15, 0, 1, 2, 3, 20, 18, 19, 21, 14, 13, 15, 16, 22, 8, 9, 23, 24, 10, 11, 25]
      offsets: [0, 8, 16, 24, 32, 40]
adjsets:
  main_adjset:
    association: "vertex"
    topology: "main"
    groups:
      group_0_1:
        neighbors: 0
        values: [4, 5, 9, 23]
      group_1_3:
        neighbors: 3
        values: 24
      group_1_2:
        neighbors: 2
        values: [15, 17, 19, 21]
      group_0_1_3:
        neighbors: [0, 3]
        values: [0, 1, 8, 10, 22]
      group_1_2_3:
        neighbors: [2, 3]
        values: 12
      group_0_1_2:
        neighbors: [0, 2]
        values: [14, 16, 18, 20]
      group_0_1_2_3:
        neighbors: [0, 2, 3]
        values: 13
state:
  domain_id: 1
)";

    // Domain 2
    const char *dom2_str = R"(
coordsets:
  coords:
    type: "explicit"
    values:
      x: [0.6666, 0.6666, 0.3333, 0.3333, 0.3333, 0.0, 0.0, 0.0, 0.6666, 0.6666, 0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.0, 0.6666, 0.6666, 0.3333, 0.0, 0.3333, 0.0, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
      y: [0.3333, 0.0, 0.0, 0.3333, 0.6666, 0.3333, 0.6666, 0.0, 0.3333, 0.0, 0.0, 0.3333, 0.0, 0.3333, 0.6666, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.6666, 1.0, 0.6666, 0.3333, 0.3333, 0.0, 0.0]
      z: [0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.6666, 1.0, 1.0, 0.6666, 0.6666, 1.0, 0.6666, 0.6666, 0.6666, 1.0, 1.0, 1.0, 0.6666, 1.0, 0.6666]
topologies:
  main:
    type: "unstructured"
    coordset: "coords"
    elements:
      shape: "hex"
      connectivity: [8, 9, 10, 11, 0, 1, 2, 3, 11, 10, 12, 13, 3, 2, 7, 5, 18, 14, 15, 19, 20, 4, 6, 21, 22, 16, 14, 18, 23, 17, 4, 20, 26, 27, 16, 22, 24, 25, 17, 23, 27, 28, 8, 16, 25, 29, 0, 17, 28, 30, 9, 8, 29, 31, 1, 0]
      offsets: [0, 8, 16, 24, 32, 40, 48]
adjsets:
  main_adjset:
    association: "vertex"
    topology: "main"
    groups:
      group_2_3:
        neighbors: 3
        values: [1, 24, 25, 29, 31]
      group_1_2:
        neighbors: 1
        values: [5, 7, 13, 15]
      group_0_2:
        neighbors: 0
        values: [8, 16, 20, 21]
      group_1_2_3:
        neighbors: [1, 3]
        values: 2
      group_0_2_3:
        neighbors: [0, 3]
        values: [0, 17, 23]
      group_0_1_2:
        neighbors: [0, 1]
        values: [4, 6, 11, 14]
      group_0_1_2_3:
        neighbors: [0, 1, 3]
        values: 3
state:
  domain_id: 2
)";

    // Domain 3
    const char *dom3_str = R"(
coordsets:
  coords:
    type: "explicit"
    values:
      x: [0.3333, 0.3333, 0.3333, 0.3333, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.3333, 0.6666, 0.6666, 0.3333, 0.3333, 0.6666, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
      y: [0.3333, 0.0, 0.6666, 0.6666, 0.6666, 0.3333, 0.6666, 0.3333, 0.0, 0.3333, 0.0, 0.0, 0.3333, 0.6666, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.6666, 1.0, 0.6666, 1.0, 0.6666, 0.3333, 0.0, 0.3333, 0.0, 0.3333]
      z: [0.3333, 0.3333, 0.3333, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.3333, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.0, 0.3333, 0.0, 0.3333, 0.3333, 0.0, 0.0, 0.6666, 0.6666, 0.6666, 0.6666, 0.3333, 0.3333, 0.0]
topologies:
  main:
    type: "unstructured"
    coordset: "coords"
    elements:
      shape: "hex"
      connectivity: [9, 10, 11, 12, 5, 8, 1, 0, 17, 4, 2, 15, 18, 6, 3, 16, 19, 20, 4, 17, 21, 22, 6, 18, 23, 24, 13, 14, 19, 20, 4, 17, 25, 26, 10, 9, 27, 28, 8, 5, 24, 25, 9, 13, 20, 27, 5, 4, 20, 27, 5, 4, 22, 29, 7, 6]
      offsets: [0, 8, 16, 24, 32, 40, 48]
adjsets:
  main_adjset:
    association: "vertex"
    topology: "main"
    groups:
      group_0_3:
        neighbors: 0
        values: [4, 5, 6, 7, 8, 17, 27, 28, 29]
      group_2_3:
        neighbors: 2
        values: [10, 23, 24, 25, 26]
      group_1_3:
        neighbors: 1
        values: 16
      group_0_1_3:
        neighbors: [0, 1]
        values: [0, 1, 2, 3, 15]
      group_1_2_3:
        neighbors: [1, 2]
        values: 11
      group_0_2_3:
        neighbors: [0, 2]
        values: [9, 13, 14]
      group_0_1_2_3:
        neighbors: [0, 1, 2]
        values: 12
state:
  domain_id: 3
)";

    // This test is supposed to run on 2 processors
    const int par_rank = relay::mpi::rank(MPI_COMM_WORLD);
    const int par_size = relay::mpi::size(MPI_COMM_WORLD);
    const int domain_to_rank[4][4] = {
        {0, 0, 0, 0},
        {0, 0, 1, 1},
        {0, 0, 1, 2},
        {0, 1, 2, 3}
    };
    int pIdx = std::min(par_size, 4) - 1;

    // Add domains to the mesh.
    if(domain_to_rank[pIdx][0] == par_rank)
        mesh["domain0"].parse(dom0_str, "yaml");
    if(domain_to_rank[pIdx][1] == par_rank)
        mesh["domain1"].parse(dom1_str, "yaml");
    if(domain_to_rank[pIdx][2] == par_rank)
        mesh["domain2"].parse(dom2_str, "yaml");
    if(domain_to_rank[pIdx][3] == par_rank)
        mesh["domain3"].parse(dom3_str, "yaml");
}

//-----------------------------------------------------------------------------
template <typename Func>
inline void
iterate_adjset(conduit::Node &mesh, const std::string &adjsetName, Func &&func)
{
    // Get the domains in the mesh for this rank.
    const std::vector<Node *> domains = conduit::blueprint::mesh::domains(mesh);

    // Iterate over the corner mesh's adjacency sets for each domain and make
    // sure all points are found using a PointQuery.
    for(auto dom_ptr : domains)
    {
        const conduit::Node &dom = *dom_ptr;
        const conduit::Node &adjset = dom["adjsets/" + adjsetName];
        const conduit::Node &groups = adjset["groups"];
        int domain_id = static_cast<int>(conduit::blueprint::mesh::utils::find_domain_id(dom));

        const conduit::Node *topo = conduit::blueprint::mesh::utils::find_reference_node(adjset, "topology");
        const conduit::Node *cset = conduit::blueprint::mesh::utils::find_reference_node(*topo, "coordset");

        for(conduit::index_t gi = 0; gi < groups.number_of_children(); gi++)
        {
            int_accessor neighbors = groups[gi]["neighbors"].as_int_accessor();
            int_accessor values = groups[gi]["values"].as_int_accessor();

            for(conduit::index_t ni = 0; ni < neighbors.number_of_elements(); ni++)
            {
                int nbr = neighbors[ni];
                for(conduit::index_t vi = 0; vi < values.number_of_elements(); vi++)
                {
                    int val = values[vi];

                    func(domain_id, nbr, val, cset, topo);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
test_adjset_points(conduit::Node &mesh, const std::string &adjsetName)
{
    conduit::blueprint::mpi::mesh::utils::query::PointQuery Q(mesh, MPI_COMM_WORLD);

    // Iterate over the points in the adjset and add them to the 
    iterate_adjset(mesh, adjsetName,
        [&](int /*dom*/, int nbr, int val, const conduit::Node *cset, const conduit::Node * /*topo*/)
        {
            // Get the point (it might not be 3D)
            auto pt = conduit::blueprint::mesh::utils::coordset::_explicit::coords(*cset, val);
            double pt3[3];
            pt3[0] = pt[0];
            pt3[1] = (pt.size() > 1) ? pt[1] : 0.;
            pt3[2] = (pt.size() > 2) ? pt[2] : 0.;

            Q.add(nbr, pt3);
        });

    // Execute the query.
    Q.execute("corner_coords");

    // If this rank had domains, check the query results. There should be no
    // occurrances of NotFound.
    for(auto domainId : Q.queryDomainIds())
    {
        const auto &r = Q.results(domainId);
        auto it = std::find(r.begin(), r.end(), Q.NotFound);
        bool found = it != r.end();

        // If NotFound was in the results, print the occurrances for debugging.
        if(found)
        {
            const int par_rank = relay::mpi::rank(MPI_COMM_WORLD);
            const auto &p = Q.inputs(domainId);
            std::stringstream ss;
            ss << "rank" << par_rank << ":\n";
            ss << "  domain: " << domainId << "\n";
            ss << "  search:\n";
            for(size_t i = 0; i < r.size(); i++)
            {
                if(r[i] == Q.NotFound)
                {
                    ss << "    -\n";
                    ss << "      point: [" << p[3*i+0] << ", " << p[3*i+1] << ", " << p[3*i+2] << "]\n";
                    ss << "      found: " << r[i] << "\n";
                }
            }
            std::cout << ss.str() << std::endl;
        }

        EXPECT_FALSE(found);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, generate_corners_wonky)
{
    // There is a 3x3x3 zone mesh that was giving generate_corners a problem
    // due to adjacency sets. Adjacency sets are produced from the original one
    // and when making corners. It was giving rise to adjacency sets that
    // contained points that do not exist in neighbor domains. We can use the
    // PointQuery to test this.

    conduit::Node mesh, s2dmap, d2smap;
    generate_wonky_mesh(mesh);

    // Make the corner mesh.
    conduit::blueprint::mpi::mesh::generate_corners(mesh,
                                                    "main_adjset",
                                                    "corner_adjset",
                                                    "corner_mesh",
                                                    "corner_coords",
                                                    s2dmap,
                                                    d2smap,
                                                    MPI_COMM_WORLD);

    std::vector<Node *> domains = conduit::blueprint::mesh::domains(mesh);

    // Test the adjset points.
    test_adjset_points(mesh, "corner_adjset");

    // Convert the adjset in all domains to pairwise and test the points again.
    for(auto dom_ptr : domains)
    {
        Node &domain = *dom_ptr;
        conduit::blueprint::mesh::adjset::to_pairwise(domain["adjsets/corner_adjset"],
                                                      domain["adjsets/corner_pairwise_adjset"]);
    }
    test_adjset_points(mesh, "corner_pairwise_adjset");

    conduit::Node info;
    bool same_pointwise = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(
                              mesh, "corner_pairwise_adjset", info);
    EXPECT_TRUE(same_pointwise);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, generate_faces_wonky)
{
    conduit::Node mesh, s2dmap, d2smap;
    generate_wonky_mesh(mesh);

    // Make the face mesh from the wonky mesh. Internally, it will use a MatchQuery
    // to avoid putting a few faces into the faces. This means that the face adjset
    // should be produced properly so all match query results should have 1's.
    conduit::blueprint::mpi::mesh::generate_faces(mesh,
                                                  "main_adjset",
                                                  "face_adjset",
                                                  "face_mesh",
                                                  s2dmap,
                                                  d2smap,
                                                  MPI_COMM_WORLD);

    conduit::blueprint::mpi::mesh::utils::query::MatchQuery Q(mesh, MPI_COMM_WORLD);
    Q.selectTopology("main");

    // Iterate over the faces in the face adjset and add them to the match query.
    iterate_adjset(mesh, "face_adjset",
        [&](int dom, int nbr, int ei, const conduit::Node */*cset*/, const conduit::Node *topo)
        {
            // Get the points for the face and add them to the query.
            std::vector<index_t> facepts = conduit::blueprint::mesh::utils::topology::unstructured::points(*topo, ei);
            Q.add(dom, nbr, facepts);
        });

    // Execute the query.
    Q.execute();

    // If this rank had domains, check the query results. They should all be 1.
    for(const auto &qid : Q.queryDomainIds())
    {
        const auto &r = Q.results(qid.first, qid.second);
        auto it = std::find_if_not(r.begin(), r.end(), [](int value){ return value == 1;} );
        bool found = it != r.end();
        EXPECT_FALSE(found);
    }  
}

/// Test Driver ///

int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}
