// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_mesh_utils.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mpi_mesh.hpp"
#include "conduit_blueprint_mpi_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"
#include "conduit_log.hpp"
#if defined(CONDUIT_BLUEPRINT_MPI_PARMETIS_ENABLED)
  #include "conduit_blueprint_mpi_mesh_partition.hpp"
  #include "conduit_blueprint_mpi_mesh_parmetis.hpp"
  #pragma message "Building parmetis support into testing."
#endif

#include "blueprint_test_helpers.hpp"
#include "blueprint_mpi_test_helpers.hpp"

#include <algorithm>
#include <vector>
#include <sstream>
#include <string>
#include <mpi.h>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
using namespace generate;

//---------------------------------------------------------------------------
void printNode(const conduit::Node &n)
{
    conduit::Node opts;
    opts["num_children_threshold"] = 1000000;
    opts["num_elements_threshold"] = 1000000;
    std::cout << n.to_summary_string(opts) << std::endl;
    std::cout.flush();
}

//---------------------------------------------------------------------------
std::string
adjset_centering(const conduit::Node &n_mesh, const std::string &adjsetName)
{
    const auto domains = conduit::blueprint::mesh::domains(n_mesh);
    const conduit::Node &n_adjset = domains[0]->fetch_existing("adjsets/" + adjsetName);
    return n_adjset["association"].as_string();
}

//---------------------------------------------------------------------------
/**
 @brief Save the node to an HDF5 compatible with VisIt or the
        conduit_adjset_validate tool.
 */
void save_mesh(const conduit::Node &root, const std::string &filebase)
{
    // NOTE: Enable this to write files for debugging.
#if 0
    const std::string protocol("hdf5");
    conduit::relay::mpi::io::blueprint::save_mesh(root, filebase, protocol, MPI_COMM_WORLD);
#else
    std::cout << "Skip writing " << filebase << std::endl;
#endif
}

//---------------------------------------------------------------------------
bool validate(const conduit::Node &root,
              const std::string &adjsetName,
              conduit::Node &info)
{
    // Use default opts and comm (by not passing them)
    return conduit::blueprint::mpi::mesh::utils::adjset::validate(root,
               adjsetName, info);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_validate_element_0d)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node root, opts, info;
    create_2_domain_0d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_0d");
    bool res = validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_0d_bad");
    res = validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //printNode(info);

    if(rank == 0 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
        const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
        EXPECT_EQ(n0.number_of_children(), 1);
        const conduit::Node &c0 = n0[0];
        EXPECT_TRUE(c0.has_path("element"));
        EXPECT_TRUE(c0.has_path("neighbor"));
        EXPECT_EQ(c0["element"].to_int(), 0);
        EXPECT_EQ(c0["neighbor"].to_int(), 1);
    }
    if(rank == 1 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
        const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
        EXPECT_EQ(n1.number_of_children(), 1);
        const conduit::Node &c1 = n1[0];
        EXPECT_TRUE(c1.has_path("element"));
        EXPECT_TRUE(c1.has_path("neighbor"));
        EXPECT_EQ(c1["element"].to_int(), 2);
        EXPECT_EQ(c1["neighbor"].to_int(), 0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_validate_element_1d)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node root, info;
    create_2_domain_1d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_1d");
    bool res = validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{1});
    info.reset();
    save_mesh(root, "adjset_validate_element_1d_bad");
    res = validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //printNode(info);

    if(rank == 0 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
        const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
        EXPECT_EQ(n0.number_of_children(), 1);
        const conduit::Node &c0 = n0[0];
        EXPECT_TRUE(c0.has_path("element"));
        EXPECT_TRUE(c0.has_path("neighbor"));
        EXPECT_EQ(c0["element"].to_int(), 0);
        EXPECT_EQ(c0["neighbor"].to_int(), 1);
    }
    if(rank == 1 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
        const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
        EXPECT_EQ(n1.number_of_children(), 1);
        const conduit::Node &c1 = n1[0];
        EXPECT_TRUE(c1.has_path("element"));
        EXPECT_TRUE(c1.has_path("neighbor"));
        EXPECT_EQ(c1["element"].to_int(), 1);
        EXPECT_EQ(c1["neighbor"].to_int(), 0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_validate_element_2d)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node root, info;
    create_2_domain_2d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_2d");
    bool res = validate(root, "main_adjset", info);
    EXPECT_TRUE(res);
    printNode(info);

    // Now, adjust the adjset for domain1 so it includes an element not present in domain 0
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0,2,4});
    info.reset();
    save_mesh(root, "adjset_validate_element_2d_bad");
    res = validate(root, "main_adjset", info);
    EXPECT_FALSE(res);

    if(rank == 1 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
        const conduit::Node &n = info["domain1/main_adjset/domain0_1"];
        EXPECT_EQ(n.number_of_children(), 1);
        const conduit::Node &c = n[0];
        EXPECT_TRUE(c.has_path("element"));
        EXPECT_TRUE(c.has_path("neighbor"));
        EXPECT_EQ(c["element"].to_int(), 2);
        EXPECT_EQ(c["neighbor"].to_int(), 0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_validate_element_3d)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node root, info;
    create_2_domain_3d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_3d");
    bool res = validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_3d_bad");
    res = validate(root, "main_adjset", info);
    EXPECT_FALSE(res);

    if(rank == 0 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
        const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
        EXPECT_EQ(n0.number_of_children(), 1);
        const conduit::Node &c0 = n0[0];
        EXPECT_TRUE(c0.has_path("element"));
        EXPECT_TRUE(c0.has_path("neighbor"));
        EXPECT_EQ(c0["element"].to_int(), 0);
        EXPECT_EQ(c0["neighbor"].to_int(), 1);
    }
    if(rank == 1 || size == 1)
    {
        EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
        const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
        EXPECT_EQ(n1.number_of_children(), 1);
        const conduit::Node &c1 = n1[0];
        EXPECT_TRUE(c1.has_path("element"));
        EXPECT_TRUE(c1.has_path("neighbor"));
        EXPECT_EQ(c1["element"].to_int(), 2);
        EXPECT_EQ(c1["neighbor"].to_int(), 0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_compare_pointwise_2d)
{
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conduit::Node root, info;
    create_2_domain_2d_mesh(root, rank, size);
    save_mesh(root, "adjset_compare_pointwise_2d");
    auto domains = conduit::blueprint::mesh::domains(root);

    for(const auto &domPtr : domains)
    {
        // It's not in canonical form.
        const conduit::Node &adjset = domPtr->fetch_existing("adjsets/pt_adjset");
        bool canonical = conduit::blueprint::mesh::utils::adjset::is_canonical(adjset);
        EXPECT_FALSE(canonical);

        // The fails_pointwise adjset is in canonical form.
        const conduit::Node &adjset2 = domPtr->fetch_existing("adjsets/fails_pointwise");
        canonical = conduit::blueprint::mesh::utils::adjset::is_canonical(adjset2);
        EXPECT_TRUE(canonical);
    }

    // Check that we can still run compare_pointwise - it will convert internally.
    bool eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", info);
    EXPECT_TRUE(eq);

    // Make sure the extra adjset was removed.
    for(const auto &domPtr : domains)
    {
        // It's not in canonical form.
        bool tmpExists = domPtr->has_path("adjsets/__pt_adjset__");
        EXPECT_FALSE(tmpExists);
    }

    // Force it to be canonical
    for(const auto &domPtr : domains)
    {
        // It's not in canonical form.
        conduit::Node &adjset = domPtr->fetch_existing("adjsets/pt_adjset");
        conduit::blueprint::mesh::utils::adjset::canonicalize(adjset);
    }
    info.reset();
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", info);
    in_rank_order(MPI_COMM_WORLD, [&](int /*r*/)
    {
        if(!eq)
        {
            std::cout << rank << ": pt_adjset eq=" << eq << std::endl;
            printNode(info);
        }
    });
    EXPECT_TRUE(eq);

    // Test that the fails_pointwise adjset actually fails.
    info.reset();
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "fails_pointwise", info);
    in_rank_order(MPI_COMM_WORLD, [&](int /*r*/)
    {
        if(eq)
        {
            std::cout << rank << ": fails_pointwise eq=" << eq << std::endl;
            printNode(info);
        }
    });
    EXPECT_FALSE(eq);

    // Test that the notevenclose adjset actually fails.
    info.reset();
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "notevenclose", info);
    in_rank_order(MPI_COMM_WORLD, [&](int /*r*/)
    {
        if(eq)
        {
            std::cout << rank << ": notevenclose eq=" << eq << std::endl;
            printNode(info);
        }
    });
    EXPECT_FALSE(eq);
}

//-----------------------------------------------------------------------------
class SortTester
{
    MPI_Comm comm;
    int rank{0}, size{1};
public:
    static const int NUM_DOMAINS = 4;

    SortTester(MPI_Comm c) : comm(c)
    {
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
    }

    void test(int res, int a, double angle, int permute)
    {
        conduit::Node n_mesh;
        build(res, angle, permute, n_mesh);

        std::vector<std::string> adjsetNames;
        adjsetNames.push_back("mesh_adjset");
        adjsetNames.push_back("corners_adjset");
        adjsetNames.push_back("lines_adjset");

#if defined(CONDUIT_BLUEPRINT_MPI_PARMETIS_ENABLED)
        buildPartitions(n_mesh);
        adjsetNames.push_back("part_mesh_adjset");
        adjsetNames.push_back("part_mesh_corners_adjset");
        adjsetNames.push_back("part_lines_adjset");
#endif
        // NOTE: Do this step after partitioning because there is a bug in
        //       adjset-creation in the partitioner where it seems to try
        //       and operate on ALL adjsets in the mesh instead of just the
        //       adjsets relevant to the topology. To replicate, move the
        //       call to buildDerived before buildPartitions.
        buildDerived(n_mesh);

        save(res, a, permute, n_mesh);

        // Validate adjsets.
        validate(res, a, angle, n_mesh, adjsetNames);
    }

    void build(int res, double angle, int permute, conduit::Node &n_mesh)
    {
        MeshBuilder B;
        // Assign domains to ranks.
        for(int d = 0; d < NUM_DOMAINS; d++)
        {
            if(d % size == rank)
            {
                B.m_selectedDomains.push_back(d);
            }
        }

        // Build the mesh.
        B.m_angle = angle;
        B.m_resolution = res;
        B.m_permute = permute > 0; // Permute node/zone order
        B.build(n_mesh);
    }

    void buildDerived(conduit::Node &n_mesh)
    {
        // Build the lines mesh.
        conduit::Node s2dmap, d2smap;
        conduit::blueprint::mpi::mesh::generate_lines(n_mesh,
                                                      "mesh_adjset",
                                                      "lines_adjset",
                                                      "lines",
                                                       s2dmap,
                                                       d2smap,
                                                       comm);

        // Build the corner mesh.
        s2dmap.reset();
        d2smap.reset();
        conduit::blueprint::mpi::mesh::generate_corners(n_mesh,
                                                        "mesh_adjset",
                                                        "corners_adjset",
                                                        "corners",
                                                        "corners_coords",
                                                        s2dmap,
                                                        d2smap,
                                                        comm);
    }

#if defined(CONDUIT_BLUEPRINT_MPI_PARMETIS_ENABLED)
    // Build a partitioned mesh and the corner mesh for that mesh.
    void buildPartitions(conduit::Node &n_mesh)
    {
        // Make a partition field on the mesh topology.
        conduit::Node opts;
        opts["topology"] = "mesh";
        opts["partitions"] = NUM_DOMAINS;
        opts["adjset"] = "mesh_adjset";
        conduit::blueprint::mpi::mesh::generate_partition_field(n_mesh, opts, comm);

        // Rename the field.
        std::string partitionField("partition");
        auto domains = conduit::blueprint::mesh::domains(n_mesh);
        for(auto &dom_ptr : domains)
        {
            conduit::Node &n_fields = dom_ptr->fetch_existing("fields");
            n_fields.rename_child("parmetis_result", partitionField);
        }

        // Repartition the mesh.
        conduit::Node n_part, partopts;
        partopts["mapping"] = 0;
        conduit::Node &sel1 = partopts["selections"].append();
        sel1["type"] = "field";
        sel1["domain_id"] = "any";
        sel1["field"] = partitionField;
        sel1["topology"] = "mesh";
        sel1["destination_ranks"].set(conduit::DataType::int32(size));
        auto ranks = sel1["destination_ranks"].as_int32_ptr();
        for (int i = 0; i < NUM_DOMAINS; i++)
            ranks[i] = i % size;
        conduit::blueprint::mpi::mesh::partition(n_mesh, partopts, n_part, comm);

        // Move the partitiond mesh into the n_mesh node, renaming as needed.
        auto part_domains = conduit::blueprint::mesh::domains(n_part);
        for(size_t i = 0; i < domains.size(); i++)
        {
            conduit::Node &n_dest = *domains[i];
            conduit::Node &n_src = *part_domains[i];

            n_dest["coordsets/part_coords"].move(n_src["coordsets/coords"]);
            n_dest["topologies/part_mesh"].move(n_src["topologies/mesh"]);
            n_dest["topologies/part_mesh/coordset"] = "part_coords";
            n_dest["adjsets/part_mesh_adjset"].move(n_src["adjsets/mesh_adjset"]);
            n_dest["adjsets/part_mesh_adjset/topology"] = "part_mesh";
        }

        // Generate corners for the part_mesh
        conduit::Node s2dmap, d2smap;
        conduit::blueprint::mpi::mesh::generate_corners(n_mesh,
                                                        "part_mesh_adjset",
                                                        "part_mesh_corners_adjset",
                                                        "part_mesh_corners",
                                                        "part_mesh_corners_coords",
                                                        s2dmap,
                                                        d2smap,
                                                        comm);

        // Build the lines for the part_mesh.
        s2dmap.reset(); d2smap.reset();
        conduit::blueprint::mpi::mesh::generate_lines(n_mesh,
                                                      "part_mesh_adjset",
                                                      "part_lines_adjset",
                                                      "part_lines",
                                                      s2dmap,
                                                      d2smap,
                                                      comm);
    }
#endif

    void save(int res, int a, int permute, const conduit::Node &n_mesh)
    {
#if 0
  #if defined(CONDUIT_RELAY_IO_HDF5_ENABLED)
        // Save the mesh
        std::stringstream ss;
        ss << (permute ? "test_p_" : "test_") << res << "_" << a;
        std::string filename(ss.str());
        conduit::relay::mpi::io::blueprint::save_mesh(n_mesh, filename, "hdf5", comm);
        std::cout << "Saved mesh to " << filename << std::endl;
  #endif
#endif
    }

    void validate(int res, int a, double angle, conduit::Node &n_mesh, const std::vector<std::string> &adjsetNames)
    {
        // Test adjsets.
        conduit::Node results;
        for(const auto &adjsetName : adjsetNames)
        {
            // Check that the adjset is valid.
            conduit::Node info;
            conduit::Node opts;
            opts["tolerance"] = 1.e-8;

            bool validate_result =
                conduit::blueprint::mpi::mesh::utils::adjset::validate(
                    n_mesh, adjsetName, opts, info, comm);

            results[adjsetName]["validate"] = validate_result;
            EXPECT_TRUE(validate_result);

            if(validate_result)
            {
                // If the adjset is vertex-associated then compare_pointwise.
                std::string association = adjset_centering(n_mesh, adjsetName);
                if(association == "vertex" ||
                   (association == "element" && adjsetName.find("line") != std::string::npos))
                {
                   info.reset();
                   bool cpw_result =
                       conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(
                           n_mesh, adjsetName, info, opts, comm);

                   results[adjsetName]["compare_pointwise"] = cpw_result;
                   EXPECT_TRUE(cpw_result);

                   in_rank_order(comm, [&](int /*r*/)
                   {
                       if(!cpw_result && info.has_path("valid") && info["valid"].as_string() == "false")
                       {
                           std::cout << rank << ": compare_pointwise: " << adjsetName
                                     << ", res=" << res
                                     << ", a=" << a
                                     << ", angle=" << angle
                                     << ", cpw_result=" << cpw_result << std::endl;
                           printNode(info);
                       }
                   });
                }
            }
            else
            {
                in_rank_order(comm, [&](int /*r*/)
                {
                    if(!validate_result && info.has_path("valid") && info["valid"].as_string() == "false")
                    {
                        std::cout << rank << ": validate: " << adjsetName
                                  << ", res=" << res
                                  << ", a=" << a
                                  << ", angle=" << angle
                                  << ", validate_result=" << validate_result
                                  << std::endl;
                        printNode(info);
                    }
                });
            }
        }

        if(rank == 0)
        {
            printNode(results);
        }
    }
};


//---------------------------------------------------------------------------
TEST(conduit_blueprint_mpi_mesh_utils, adjset_sorting_2d)
{
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    EXPECT_TRUE(size <= 4);

    const int NUM_ANGLES = 17;
    const double pi = 3.141592653589793;
    const double a0 = 0.;
    const double a1 = 2. * pi;
    const std::vector<int> resolutions{{1,2,3}};

    // Build the mesh for various resolutions and angles to make sure the adjset
    // sorting inside line/corner construction works.
    for(const auto &res : resolutions)
    {
        for(int a = 0; a < NUM_ANGLES; a++)
        {
            double ta = static_cast<double>(a) / static_cast<double>(NUM_ANGLES - 1);
            double angle = a0 + ta * (a1 - a0);

            for(int permute = 0; permute < 2; permute++)
            {
                if(rank == 0)
                {
                    std::cout << "Resolution: " << res
                              << ", angle: " << angle
                              << ", permute: " << permute
                              << std::endl;
                }

                SortTester st(MPI_COMM_WORLD);
                st.test(res, a, angle, permute);
            }
        }
    }
}

//---------------------------------------------------------------------------
static void conduit_debug_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    std::cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << std::endl;
    // This is on purpose.
    while(1)
      ;
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    // Handle command line args.
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-handler") == 0)
        {
            conduit::utils::set_error_handler(conduit_debug_err_handler);
        }
    }

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}
