
//-----------------------------------------------------------------------------
///
/// file: t_relay_io_cgns.cpp
///
//-----------------------------------------------------------------------------

#include "silo_test_utils.hpp"

#include "conduit_relay.hpp"
#include "conduit_relay_io_cgns.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_cgns, round_trip_basic)
{
    // const std::vector<std::pair<std::string, std::string>> mesh_types = {
    //     std::make_pair("uniform", "2"), std::make_pair("uniform", "3"),
    //     std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
    //     std::make_pair("structured", "2"), std::make_pair("structured", "3"),
    //     std::make_pair("tris", "2"),
    //     std::make_pair("quads", "2"),
    //     std::make_pair("polygons", "2"),
    //     std::make_pair("tets", "3"),
    //     std::make_pair("hexs", "3"),
    //     std::make_pair("wedges", "3"),
    //     std::make_pair("pyramids", "3"),
    //     // TODO
    //     // std::make_pair("polyhedra", "3")
    // };
        const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("tets", "3"),
        std::make_pair("hexs", "3"),
    };
    for (int i = 0; i < mesh_types.size(); ++i)
    {
        const std::string dim = mesh_types[i].second;
        const index_t nx = 3;
        const index_t ny = 4;
        const index_t nz = (dim == "2" ? 0 : 2);

        const std::string mesh_type = mesh_types[i].first;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::basic(mesh_type, nx, ny, nz, save_mesh);

        const std::string basename = "cgns_basic_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename;

        remove_path_if_exists(filename);
        io::cgns::save_mesh(save_mesh, basename);
        io::cgns::load_mesh(filename, load_mesh);
        // EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        std::cout << "saved mesh:\n";
        save_mesh.print();
        std::cout << "\nloaded mesh:\n";
        load_mesh.print();
        std::cout << "\n";

        // // make changes to save mesh so the diff will pass
        // if (mesh_type == "uniform")
        // {
        //     silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);
        // }
        // silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}