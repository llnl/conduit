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
              const conduit::Node &opts,
              conduit::Node &info)
{
    return conduit::blueprint::mpi::mesh::utils::adjset::validate(root,
               adjsetName, opts, info, MPI_COMM_WORLD);
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
    bool res = validate(root, "main_adjset", opts, info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_0d_bad");
    res = validate(root, "main_adjset", opts, info);
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

    conduit::Node root, opts, info;
    create_2_domain_1d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_1d");
    bool res = validate(root, "main_adjset", opts, info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{1});
    info.reset();
    save_mesh(root, "adjset_validate_element_1d_bad");
    res = validate(root, "main_adjset", opts, info);
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

    conduit::Node root, opts, info;
    create_2_domain_2d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_2d");
    bool res = validate(root, "main_adjset", opts, info);
    EXPECT_TRUE(res);
    printNode(info);

    // Now, adjust the adjset for domain1 so it includes an element not present in domain 0
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0,2,4});
    info.reset();
    save_mesh(root, "adjset_validate_element_2d_bad");
    res = validate(root, "main_adjset", opts, info);
    EXPECT_FALSE(res);
    //printNode(info);

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

    conduit::Node root, opts, info;
    create_2_domain_3d_mesh(root, rank, size);
    save_mesh(root, "adjset_validate_element_3d");
    bool res = validate(root, "main_adjset", opts, info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    if(root.has_child("domain0"))
        root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    if(root.has_child("domain1"))
        root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_3d_bad");
    res = validate(root, "main_adjset", opts, info);
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
    conduit::Node opts;
    bool eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", opts, info, MPI_COMM_WORLD);
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
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", opts, info, MPI_COMM_WORLD);
    in_rank_order(MPI_COMM_WORLD, [&](int r)
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
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "fails_pointwise", opts, info, MPI_COMM_WORLD);
    in_rank_order(MPI_COMM_WORLD, [&](int r)
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
    eq = conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(root, "notevenclose", opts, info, MPI_COMM_WORLD);
    in_rank_order(MPI_COMM_WORLD, [&](int r)
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
TEST(conduit_blueprint_mpi_mesh_utils, adjset_sorting_2d)
{
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    EXPECT_TRUE(size <= 4);

    const int NUM_DOMAINS = 4;
    const int NUM_ANGLES = 17;
    const double a0 = 0.;
    const double a1 = 2. * M_PI;
    const std::vector<int> resolutions{{1,2,3}};

    // Build the mesh for various resolutions and angles to make sure the adjset
    // sorting inside line/corner construction works.
    for(const auto &res : resolutions)
    {
        for(int a = 0; a < NUM_ANGLES; a++)
        {
            double ta = static_cast<double>(a) / static_cast<double>(NUM_ANGLES - 1);
            double angle = a0 + ta * (a1 - a0);

            if(rank == 0)
            {
                std::cout << "Resolution: " << res << ", angle: " << angle << std::endl;
            }

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
            conduit::Node n_mesh;
            B.m_angle = angle;
            B.m_resolution = res;
            B.build(n_mesh);
#if 0
            in_rank_order(MPI_COMM_WORLD, [&](int r)
            {
                printNode(n_mesh);
            });
#endif

            // Build the lines mesh.
            conduit::Node s2dmap, d2smap;
            conduit::blueprint::mpi::mesh::generate_lines(n_mesh,
                                                          "mesh_adjset",
                                                          "lines_adjset",
                                                          "lines",
                                                           s2dmap,
                                                           d2smap,
                                                           MPI_COMM_WORLD);

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
                                                            MPI_COMM_WORLD);
#if 0
            // Save the mesh
            std::stringstream ss;
            ss << "test_" << res << "_" << a;
            std::string filename(ss.str());
            conduit::relay::mpi::io::blueprint::save_mesh(n_mesh, filename, "hdf5", MPI_COMM_WORLD);
            std::cout << "Saved mesh to " << filename << std::endl;
#endif

            // Test adjsets.
            const char *adjsetNames[] = {"mesh_adjset", "corners_adjset", "lines_adjset"};
            for(int ni = 0; ni < 3; ni++)
            {
                // Check that the adjset is valid.
                conduit::Node info;
                conduit::Node opts;
                opts["tolerance"] = 1.e-9;

                bool validate_result =
                    conduit::blueprint::mpi::mesh::utils::adjset::validate(
                        n_mesh, adjsetNames[ni], opts, info, MPI_COMM_WORLD);

                EXPECT_TRUE(validate_result);
                if(validate_result)
                {
                    // If the adjset is vertex-associated then compare_pointwise.
                    std::string association = adjset_centering(n_mesh, adjsetNames[ni]);
                    if(association == "vertex")
                    {
                       info.reset();
                       bool cpw_result =
                           conduit::blueprint::mpi::mesh::utils::adjset::compare_pointwise(
                               n_mesh, adjsetNames[ni], opts, info, MPI_COMM_WORLD);
                       EXPECT_TRUE(cpw_result);
                       in_rank_order(MPI_COMM_WORLD, [&](int r)
                       {
                           if(!cpw_result && info.has_path("valid") && info["valid"].as_string() == "false")
                           {
                               std::cout << rank << ": compare_pointwise: " << adjsetNames[ni]
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
                    in_rank_order(MPI_COMM_WORLD, [&](int r)
                    {
                        if(!validate_result && info.has_path("valid") && info["valid"].as_string() == "false")
                        {
                            std::cout << rank << ": validate: " << adjsetNames[ni]
                                      << ", res=" << res
                                      << ", a=" << a
                                      << ", angle=" << angle
                                      << ", validate_result=" << validate_result
                                      << std::endl;
                            printNode(info);
                        }
                    });
                }
            } // end for
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
