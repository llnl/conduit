// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_verify.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mpi.hpp"

#include "conduit_fmt/conduit_fmt.h"

#include <iostream>
#include <string>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
bool has_empty_warning(const conduit::Node &info)
{
    bool res = false;

    if((info.dtype().is_object() && info.has_child("info")) &&
       (info["info"].dtype().is_object() || info["info"].dtype().is_list()))
    {
        conduit::NodeConstIterator iitr = info["info"].children();
        while(iitr.has_next())
        {
            const conduit::Node &ichld = iitr.next();
            res |= ichld.dtype().is_char8_str() &&
                   ichld.to_string().find("is an empty mesh", 0) != std::string::npos;
        }
    }

    return res;
}

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_smoke, basic_verify)
{
    conduit::Node mesh, info;

    int par_size;
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // empty on all domains should return true, with a warning
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
    EXPECT_TRUE(has_empty_warning(info));

    conduit::blueprint::mesh::examples::braid("uniform",
                                              10,
                                              10,
                                              10,
                                              mesh);

    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
    EXPECT_FALSE(has_empty_warning(info));
    EXPECT_EQ(conduit::blueprint::mpi::mesh::number_of_domains(mesh,MPI_COMM_WORLD),par_size);

    conduit::Node domain_to_rank_map;
    conduit::blueprint::mpi::mesh::generate_domain_to_rank_map(mesh,domain_to_rank_map,MPI_COMM_WORLD);

    EXPECT_TRUE(domain_to_rank_map.dtype().is_int64());
    EXPECT_EQ(domain_to_rank_map.dtype().number_of_elements(),par_size);

    conduit::int64_array map_array = domain_to_rank_map.as_int64_array();
    for(conduit::int64 rank=0; rank < par_size; ++rank)
    {
        EXPECT_EQ(map_array[rank], rank);
    }

}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_smoke, ranks_with_no_mesh)
{
    conduit::Node mesh, info;

    // empty on all domains should return true, with a warning
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
    EXPECT_TRUE(has_empty_warning(info));

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    for(conduit::index_t active_rank=0; active_rank < par_size; active_rank++)
    {
        mesh.reset();
        // even with a single domain on one rank, we should still verify true
        if(par_rank == active_rank)
        {
            conduit::blueprint::mesh::examples::braid("uniform",
                                                      10,
                                                      10,
                                                      10,
                                                      mesh);
        }

        EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
        EXPECT_EQ(has_empty_warning(info), par_rank != active_rank);
        EXPECT_EQ(conduit::blueprint::mpi::mesh::number_of_domains(mesh,MPI_COMM_WORLD), 1);

        // check hypothetical index gen
        conduit::Node bp_index;
        conduit::blueprint::mpi::mesh::generate_index(mesh,
                                                      "",
                                                      bp_index["mesh"],
                                                      MPI_COMM_WORLD);

        // all ranks should have index data.
        EXPECT_TRUE(bp_index["mesh"].dtype().is_object());

        conduit::Node domain_to_rank_map;
        conduit::blueprint::mpi::mesh::generate_domain_to_rank_map(mesh,domain_to_rank_map,MPI_COMM_WORLD);

        EXPECT_TRUE(domain_to_rank_map.dtype().is_int64());
        EXPECT_EQ(domain_to_rank_map.dtype().number_of_elements(), active_rank + 1);

        conduit::int64_array map_array = domain_to_rank_map.as_int64_array();
        for(conduit::int64 rank=0; rank <= active_rank; ++rank)
        {
            EXPECT_EQ(map_array[rank], (rank == active_rank) ? rank : -1);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_smoke, multi_domain)
{
    conduit::Node mesh, info;

    // empty on all domains should return true, with a warning
    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
    EXPECT_TRUE(has_empty_warning(info));

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // This tests a parallel multi-domain mesh with varying number of domains
    // on each rank.
    //
    // Each rank will have a number of domains equal to (rank%3)+1.

    conduit::index_t start_dom = 0;
    for(conduit::index_t rank=0; rank < par_rank; ++rank)
    {
        start_dom += (rank%3) + 1;
    }

    int num_doms_on_rank = (par_rank%3) + 1;
    int stop_dom = start_dom + num_doms_on_rank;
    for(conduit::index_t dom=start_dom; dom < stop_dom; ++dom)
    {
        std::string domain_name = conduit_fmt::format("domain_{:d}", dom);

        conduit::Node& domain = mesh[domain_name];

        conduit::blueprint::mesh::examples::braid("uniform",
                                                  10,
                                                  10,
                                                  10,
                                                  domain);

        domain["state/domain_id"] = dom;
    }

    EXPECT_TRUE(conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));
    EXPECT_FALSE(has_empty_warning(info));

    // check the total number of domains.
    conduit::index_t num_domains = 0;
    for(conduit::index_t rank=0; rank < par_size; ++rank)
    {
        num_domains += (rank%3) + 1;
    }

    EXPECT_EQ(conduit::blueprint::mpi::mesh::number_of_domains(mesh,MPI_COMM_WORLD), num_domains);

    // check hypothetical index gen
    conduit::Node bp_index;
    conduit::blueprint::mpi::mesh::generate_index(mesh,
                                                  "",
                                                  bp_index["mesh"],
                                                  MPI_COMM_WORLD);

    // all ranks should have index data.
    EXPECT_TRUE(bp_index["mesh"].dtype().is_object());

    conduit::Node domain_to_rank_map;
    conduit::blueprint::mpi::mesh::generate_domain_to_rank_map(mesh,domain_to_rank_map,MPI_COMM_WORLD);

    EXPECT_TRUE(domain_to_rank_map.dtype().is_int64());
    EXPECT_EQ(domain_to_rank_map.dtype().number_of_elements(), num_domains);

    // Check the values in domain_to_rank_map.
    conduit::int64_array map_array = domain_to_rank_map.as_int64_array();
    conduit::int64 start_dom_for_rank = 0;
    for(conduit::int64 cur_rank=0; cur_rank < par_size; ++cur_rank)
    {
        conduit::int64 stop_dom_for_rank = start_dom_for_rank + (cur_rank%3)+1;
        for(conduit::int64 dom=start_dom_for_rank; dom < stop_dom_for_rank;
            ++dom)
        {
            EXPECT_EQ(map_array[dom], cur_rank);
        }

        start_dom_for_rank += (cur_rank%3) + 1;
    }

}


//-----------------------------------------------------------------------------
TEST(blueprint_mpi_smoke, cross_domain_index_matmap_gen_2_domain)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    std::cout<<"Rank "<<par_rank<<" of "<<par_size<<"\n";

    conduit::Node mesh;
    int nx = 4;
    int ny = 4;
    double radius = 0.15;

    std::string matset_type = "sparse_by_material";
    conduit::blueprint::mesh::examples::venn(matset_type, nx, ny, radius, mesh);
    
    conduit::Node verify_info;
    if (! conduit::blueprint::mesh::verify(mesh, verify_info))
    {
        std::cout << "verify failed" << std::endl;
        std::cout << verify_info.to_yaml() << std::endl;
    }

    mesh["fields"].remove_child("radius_a");
    mesh["fields"].remove_child("radius_b");
    mesh["fields"].remove_child("radius_c");
    mesh["fields"].remove_child("overlap");
    mesh["fields"].remove_child("circle_a");
    mesh["fields"].remove_child("circle_b");
    mesh["fields"].remove_child("circle_c");
    mesh["fields"].remove_child("background");
    // mesh["fields"].remove_child("importance");
    // mesh["fields"].remove_child("mat_check");
    // mesh["fields"].remove_child("area");

    conduit::Node mesh_part;
    conduit::Node opts;
    opts["target"] = 2;
    conduit::blueprint::mesh::partition(mesh, opts, mesh_part);

    if(par_rank == 0)
    {
        mesh_part.remove(1);
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_b");
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_c");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_b");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_c");
    }
    else
    {
        mesh_part.remove(0);
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_a");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_a");
    }

    conduit::Node bp_index;
    conduit::blueprint::mpi::mesh::generate_index(mesh_part, "", bp_index, comm);

    const conduit::Node &material_map = bp_index["matsets"]["matset"]["material_map"];

    EXPECT_TRUE(material_map.has_child("background"));
    EXPECT_EQ(material_map["background"].to_int(), 0);
    EXPECT_TRUE(material_map.has_child("circle_a"));
    EXPECT_EQ(material_map["circle_a"].to_int(), 1);
    EXPECT_TRUE(material_map.has_child("circle_b"));
    EXPECT_EQ(material_map["circle_b"].to_int(), 2);
    EXPECT_TRUE(material_map.has_child("circle_c"));
    EXPECT_EQ(material_map["circle_c"].to_int(), 3);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mpi_smoke, cross_domain_index_matmap_gen_3_domain)
{
    MPI_Comm comm = MPI_COMM_WORLD;
    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    std::cout<<"Rank "<<par_rank<<" of "<<par_size<<"\n";

    conduit::Node mesh;
    int nx = 4;
    int ny = 4;
    double radius = 0.15;

    std::string matset_type = "sparse_by_material";
    conduit::blueprint::mesh::examples::venn(matset_type, nx, ny, radius, mesh);
    
    conduit::Node verify_info;
    if (! conduit::blueprint::mesh::verify(mesh, verify_info))
    {
        std::cout << "verify failed" << std::endl;
        std::cout << verify_info.to_yaml() << std::endl;
    }

    mesh["fields"].remove_child("radius_a");
    mesh["fields"].remove_child("radius_b");
    mesh["fields"].remove_child("radius_c");
    mesh["fields"].remove_child("overlap");
    mesh["fields"].remove_child("circle_a");
    mesh["fields"].remove_child("circle_b");
    mesh["fields"].remove_child("circle_c");
    mesh["fields"].remove_child("background");
    // mesh["fields"].remove_child("importance");
    // mesh["fields"].remove_child("mat_check");
    // mesh["fields"].remove_child("area");

    conduit::Node mesh_part;
    conduit::Node opts;
    opts["target"] = 3;
    conduit::blueprint::mesh::partition(mesh, opts, mesh_part);

    if(par_rank == 0)
    {
        mesh_part.remove(2);
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_a");
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_b");
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_c");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_a");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_b");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_c");

        mesh_part[1]["matsets/matset/volume_fractions"].remove_child("circle_b");
        mesh_part[1]["matsets/matset/volume_fractions"].remove_child("circle_c");
        mesh_part[1]["matsets/matset/element_ids"].remove_child("circle_b");
        mesh_part[1]["matsets/matset/element_ids"].remove_child("circle_c");
    }
    else
    {
        mesh_part.remove(0);
        mesh_part.remove(1);
        mesh_part[0]["matsets/matset/volume_fractions"].remove_child("circle_a");
        mesh_part[0]["matsets/matset/element_ids"].remove_child("circle_a");
    }

    conduit::Node bp_index;
    conduit::blueprint::mpi::mesh::generate_index(mesh_part, "", bp_index, comm);

    const conduit::Node &material_map = bp_index["matsets"]["matset"]["material_map"];

    EXPECT_TRUE(material_map.has_child("background"));
    EXPECT_EQ(material_map["background"].to_int(), 0);
    EXPECT_TRUE(material_map.has_child("circle_a"));
    EXPECT_EQ(material_map["circle_a"].to_int(), 1);
    EXPECT_TRUE(material_map.has_child("circle_b"));
    EXPECT_EQ(material_map["circle_b"].to_int(), 2);
    EXPECT_TRUE(material_map.has_child("circle_c"));
    EXPECT_EQ(material_map["circle_c"].to_int(), 3);
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
