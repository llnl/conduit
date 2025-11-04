// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_silo.cpp
///
//-----------------------------------------------------------------------------

#include "silo_test_utils.hpp"
#include "conduit_fmt/conduit_fmt.h"

#include "conduit_relay.hpp"
#include "conduit_relay_io_silo.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, conduit_silo_cold_storage)
{
    uint32 a_val = 20;
    uint32 b_val = 8;
    uint32 c_val = 13;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;

    EXPECT_EQ(n["a"].as_uint32(), a_val);
    EXPECT_EQ(n["b"].as_uint32(), b_val);
    EXPECT_EQ(n["c"].as_uint32(), c_val);

    io::silo_write(n,"tout_cold_storage_test.silo:myobj");

    Node n_load;
    io::silo_read("tout_cold_storage_test.silo:myobj",n_load);

    EXPECT_EQ(n_load["a"].as_uint32(), a_val);
    EXPECT_EQ(n_load["b"].as_uint32(), b_val);
    EXPECT_EQ(n_load["c"].as_uint32(), c_val);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, conduit_silo_cold_storage_generic_iface)
{
    uint32 a_val = 20;
    uint32 b_val = 8;
    uint32 c_val = 13;

    Node n;
    n["a"] = a_val;
    n["b"] = b_val;
    n["c"] = c_val;

    EXPECT_EQ(n["a"].as_uint32(), a_val);
    EXPECT_EQ(n["b"].as_uint32(), b_val);
    EXPECT_EQ(n["c"].as_uint32(), c_val);

    io::save(n, "tout_cold_storage_test_generic_iface.silo:myobj");

    Node n_load;
    io::load("tout_cold_storage_test_generic_iface.silo:myobj",n_load);

    EXPECT_EQ(n_load["a"].as_uint32(), a_val);
    EXPECT_EQ(n_load["b"].as_uint32(), b_val);
    EXPECT_EQ(n_load["c"].as_uint32(), c_val);
}

//-----------------------------------------------------------------------------
// test silo file format detection
TEST(conduit_relay_io_silo, test_silo_detect)
{
    // make sure bogus file doesn't return true
    EXPECT_FALSE(io::is_silo_file("BoGUS.txt"));
    
    // make sure pure hdf5 file doesn't return true
    Node n_test;
    n_test["a"] = 42.0;
    io::save(n_test,"tout_hdf5_plain_vs_silo_open.hdf5");
    EXPECT_FALSE(io::is_silo_file("tout_hdf5_plain_vs_silo_open.hdf5"));

    // make sure blueprint hdf5 file doesn't return true
    n_test.reset();
    blueprint::mesh::examples::braid("uniform", 5, 5, 0, n_test);
    io::blueprint::save_mesh(n_test,"tout_bp_hdf5_mesh_vs_silo_open","hdf5");
    EXPECT_FALSE(io::is_silo_file("tout_bp_hdf5_mesh_vs_silo_open.root"));

    // make sure blueprint yaml file doesn't return true
    io::blueprint::save_mesh(n_test,"tout_bp_yaml_mesh_vs_silo_open","yaml");
    EXPECT_FALSE(io::is_silo_file("tout_bp_yaml_mesh_vs_silo_open.root"));

    // make sure conduit creaeted silo pdb file *does* return true
    Node n_save_opts;
    n_save_opts["silo_type"] = "pdb";
    n_save_opts["suffix"] = "none";
    io::silo::save_mesh(n_test, "tout_bp_silo_pdb_open", n_save_opts);
    EXPECT_TRUE(io::is_silo_file("tout_bp_silo_pdb_open.root"));

    // make sure conduit created silo hdf5 file *does* return true
    n_save_opts["silo_type"] = "hdf5";
    n_save_opts["suffix"] = "none";
    io::silo::save_mesh(n_test, "tout_bp_silo_hdf5_open", n_save_opts);
    EXPECT_TRUE(io::is_silo_file("tout_bp_silo_hdf5_open.root"));

    // make sure known silo file *does* return true
    std::string silo_input_file = utils::join_file_path("overlink", "box2d.silo");
    silo_input_file = relay_test_silo_data_path(silo_input_file);
    EXPECT_TRUE(io::is_silo_file(silo_input_file));

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_basic)
{
    const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("uniform", "2"), std::make_pair("uniform", "3"),
        std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
        std::make_pair("structured", "2"), std::make_pair("structured", "3"),
        std::make_pair("tris", "2"),
        std::make_pair("quads", "2"),
        std::make_pair("polygons", "2"),
        std::make_pair("tets", "3"),
        std::make_pair("hexs", "3"),
        std::make_pair("wedges", "3"),
        std::make_pair("pyramids", "3"),
        // TODO
        // std::make_pair("polyhedra", "3")
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


        const std::string basename = "silo_basic_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename + ".root";

        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass
        if (mesh_type == "uniform")
        {
            silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);
        }
        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_avoid_name_collisions)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::misc("specsets", 4, 4, 1, save_mesh);
    save_mesh["fields"]["mesh"].set(save_mesh["fields"]["braid"]);
    save_mesh["matsets"]["matset"].set(save_mesh["matsets"]["mesh"]);
    save_mesh["specsets"]["specset"].set(save_mesh["specsets"]["mesh"]);
    save_mesh["specsets"]["specset"]["matset"].set("matset");

    const std::string basename = "silo_round_trip_avoid_name_collisions";
    const std::string filename = basename + ".cycle_000100.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    Node read_opts;
    read_opts["matset_style"] = "multi_buffer_full";

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, read_opts, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    save_mesh["fields"].remove_child("mesh");
    save_mesh["matsets"].remove_child("mesh");
    save_mesh["specsets"].remove_child("mesh");

    // make changes to save mesh so the diff will pass
    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
// we are testing vector fields in this test
TEST(conduit_relay_io_silo, round_trip_braid)
{
    const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("uniform", "2"), std::make_pair("uniform", "3"),
        std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
        std::make_pair("structured", "2"), std::make_pair("structured", "3"),
        std::make_pair("points", "2"), std::make_pair("points", "3"),
        std::make_pair("points_implicit", "2"), std::make_pair("points_implicit", "3"),
        std::make_pair("lines", "2"), std::make_pair("lines", "3"),
        std::make_pair("tris", "2"),
        std::make_pair("quads", "2"),
        std::make_pair("tets", "3"),
        std::make_pair("hexs", "3"),
        std::make_pair("wedges", "3"),
        std::make_pair("pyramids", "3"),
        // TODO
        // std::make_pair("mixed_2d", "2"),
        // std::make_pair("mixed", "3"),
    };
    for (int i = 0; i < mesh_types.size(); i ++)
    {
        std::string dim = mesh_types[i].second;
        index_t nx = 3;
        index_t ny = 4;
        index_t nz = (dim == "2" ? 0 : 2);

        const std::string mesh_type = mesh_types[i].first;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::braid(mesh_type, nx, ny, nz, save_mesh);

        const std::string basename = "silo_braid_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename + ".cycle_000100.root";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

        // remove existing root file, directory and any output files
        remove_path_if_exists(filename);

        io::silo::save_mesh(save_mesh, basename);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass
        if (mesh_type == "uniform")
        {
            silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);
        }
        if (mesh_type == "points")
        {
            // this is custom code for braid
            // We know it is correct because the unstructured points version of braid
            // uses every point in the coordset
            save_mesh["topologies"].remove_child("mesh");
            save_mesh["topologies"]["mesh"]["type"] = "points";
            save_mesh["topologies"]["mesh"]["coordset"] = "coords";
        }
        if (mesh_type == "points_implicit" || mesh_type == "points")
        {
            // the association doesn't matter for point meshes
            // we choose vertex by convention
            save_mesh["fields"]["radial"]["association"].reset();
            save_mesh["fields"]["radial"]["association"] = "vertex";
        }
        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// multidomain test
TEST(conduit_relay_io_silo, round_trip_spiral)
{
    for (int ndomains = 2; ndomains < 6; ndomains ++)
    {
        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);

        const std::string basename = "silo_spiral_" + std::to_string(ndomains) + "_domains";
        const std::string filename = basename + ".cycle_000000.root";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            silo_name_changer("mesh", save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_julia)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::julia(5,  // nx
                                     5,  // ny
                                     0,  // x_min
                                     10, // x_max
                                     2,  // y_min
                                     7,  // y_max
                                     3,  // c_re
                                     4,  // c_im
                                     save_mesh);

    const std::string basename = "silo_julia";
    const std::string filename = basename + ".root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // make changes to save mesh so the diff will pass
    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
// test material write and read
TEST(conduit_relay_io_silo, round_trip_venn)
{
    std::string matset_type = "sparse_by_element";
    for (int j = 0; j < 2; j ++)
    {
        Node save_mesh, sbe, load_mesh, info;
        std::string size;
        int nx, ny;
        const double radius = 0.25;
        if (j == 0)
        {
            size = "small";
            nx = ny = 4;
        }
        else
        {
            size = "large";
            nx = ny = 100;
        }
        blueprint::mesh::examples::venn(matset_type, nx, ny, radius, save_mesh);

        const std::string basename = "silo_venn_" + matset_type + "_" + size;
        const std::string filename = basename + ".root";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass

        // The field mat_check has values that are one type and matset_values
        // that are another type. The silo writer converts both to double arrays
        // in this case, so we follow suit.
        Node mat_check_new_values, mat_check_new_matset_values;
        save_mesh["fields"]["mat_check"]["values"].to_double_array(mat_check_new_values);
        save_mesh["fields"]["mat_check"]["matset_values"].to_double_array(mat_check_new_matset_values);
        save_mesh["fields"]["mat_check"]["values"].set_external(mat_check_new_values);
        save_mesh["fields"]["mat_check"]["matset_values"].set_external(mat_check_new_matset_values);

        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_venn_modded_matnos)
{
    const std::string matset_type = "sparse_by_element";
    Node save_mesh, load_mesh, info;
    const int nx = 4;
    const int ny = 4;
    const double radius = 0.25;
    blueprint::mesh::examples::venn(matset_type, nx, ny, radius, save_mesh);

    auto replace_matno = [](int matno)
    {
        return (matno == 1 ? 15 : 
               (matno == 2 ? 37 : 
               (matno == 3 ? 4  : 
               (matno == 0 ? 22 : 
               -1))));
    };

    auto matmap_itr = save_mesh["matsets"]["matset"]["material_map"].children();
    while (matmap_itr.has_next())
    {
        Node &mat = matmap_itr.next();
        mat.set(replace_matno(mat.as_int()));
    }

    int_array matids = save_mesh["matsets"]["matset"]["material_ids"].value();
    for (int i = 0; i < save_mesh["matsets"]["matset"]["material_ids"].dtype().number_of_elements(); i ++)
    {
        matids[i] = replace_matno(matids[i]);
    }

    const std::string silo_basename = "silo_venn_" + matset_type + "_modded_matnos";
    const std::string silo_filename = silo_basename + ".root";
    EXPECT_EQ(silo_filename, io::blueprint::generate_root_filename(save_mesh, silo_basename, "silo"));
    remove_path_if_exists(silo_filename);
    io::silo::save_mesh(save_mesh, silo_basename);

    const std::string bp_basename = "bp_venn_" + matset_type + "_modded_matnos";
    const std::string bp_filename = bp_basename + ".root";
    EXPECT_EQ(bp_filename, io::blueprint::generate_root_filename(save_mesh, bp_basename, "hdf5"));
    remove_path_if_exists(bp_filename);
    io::blueprint::save_mesh(save_mesh, bp_basename, "hdf5");
    
    io::silo::load_mesh(silo_filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // make changes to save mesh so the diff will pass

    // The field mat_check has values that are one type and matset_values
    // that are another type. The silo writer converts both to double arrays
    // in this case, so we follow suit.
    Node mat_check_new_values, mat_check_new_matset_values;
    save_mesh["fields"]["mat_check"]["values"].to_double_array(mat_check_new_values);
    save_mesh["fields"]["mat_check"]["matset_values"].to_double_array(mat_check_new_matset_values);
    save_mesh["fields"]["mat_check"]["values"].set_external(mat_check_new_values);
    save_mesh["fields"]["mat_check"]["matset_values"].set_external(mat_check_new_matset_values);

    // to_silo is going to reorder mixed materials least to greatest
    // so we must do the same
    int_array mat_ids = save_mesh["matsets"]["matset"]["material_ids"].value();
    const auto mat_id10 = mat_ids[10];
    const auto mat_id11 = mat_ids[11];
    const auto mat_id12 = mat_ids[12];
    mat_ids[10] = mat_id12;
    mat_ids[11] = mat_id10;
    mat_ids[12] = mat_id11;
    auto field_itr = save_mesh["fields"].children();
    while (field_itr.has_next())
    {
        const Node &n_field = field_itr.next();
        if (n_field.has_child("matset"))
        {
            double_array matset_vals = n_field["matset_values"].value();
            const auto matset_val10 = matset_vals[10];
            const auto matset_val11 = matset_vals[11];
            const auto matset_val12 = matset_vals[12];
            matset_vals[10] = matset_val12;
            matset_vals[11] = matset_val10;
            matset_vals[12] = matset_val11;
        }
    }

    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_spiral_multi_dom_materials)
{
    Node save_mesh, load_mesh, info;
    const int ndomains = 4;
    blueprint::mesh::examples::spiral(ndomains, save_mesh);
    add_matset_to_spiral(save_mesh, ndomains);
    EXPECT_TRUE(blueprint::mesh::verify(save_mesh, info));
    
    const std::string basename = "silo_multidom_materials_spiral";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        // get the matset for this domain
        Node &n_matset = save_mesh[child]["matsets"]["matset"];

        // clean up volume fractions
        Node vf_arr;
        n_matset["volume_fractions"].to_float64_array(vf_arr);
        n_matset["volume_fractions"].reset();
        n_matset["volume_fractions"].set(vf_arr);
        
        // cheat a little bit - we don't have these to start
        n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
        n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);

        silo_name_changer("mesh", save_mesh[child]);
    }

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_grid_adjset)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::grid("structured", 3, 3, 1, 2, 2, 1, save_mesh);

    // we need a material in order for this to be valid overlink
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        add_multi_buffer_full_matset(save_mesh[child], 4, "mesh");
    }

    Node write_opts;
    write_opts["file_style"] = "overlink";
    write_opts["ovl_topo_name"] = "mesh";

    Node read_opts;
    read_opts["matset_style"] = "multi_buffer_full";

    const std::string basename = "silo_grid_adjset";
    const std::string filename = basename + "/OvlTop.silo";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename, write_opts);
    io::blueprint::save_mesh(save_mesh, basename, "hdf5");
    io::silo::load_mesh(filename, read_opts, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        // separate out vector fields
        Node &field_vel = save_mesh[child]["fields"]["vel"];
        Node &field_vel_u = save_mesh[child]["fields"]["vel_u"];
        Node &field_vel_v = save_mesh[child]["fields"]["vel_v"];
        
        field_vel_u["topology"].set(field_vel["topology"]);
        field_vel_u["association"].set(field_vel["association"]);
        field_vel_u["values"].set(field_vel["values/u"]);
        field_vel_v["topology"].set(field_vel["topology"]);
        field_vel_v["association"].set(field_vel["association"]);
        field_vel_v["values"].set(field_vel["values/v"]);

        save_mesh[child]["fields"].remove_child("vel");

        // make adjset pairwise
        Node &pairwise_adjset = save_mesh[child]["adjsets"]["adjset"];
        conduit::blueprint::mesh::adjset::to_pairwise(save_mesh[child]["adjsets"]["mesh_adj"], pairwise_adjset);
        save_mesh[child]["adjsets"].remove_child("mesh_adj");

        // make changes to save mesh so the diff will pass
        overlink_name_changer(save_mesh[child]);
    }

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_specsets)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::misc("specsets", 10, 10, 1, save_mesh);
    save_mesh["matsets"].rename_child("mesh", "matset");
    save_mesh["specsets"].rename_child("mesh", "specset");
    save_mesh["specsets"]["specset"]["matset"].set("matset");

    const std::string basename = "silo_round_trip_specsets";
    const std::string filename = basename + ".cycle_000100.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    Node read_opts;
    read_opts["matset_style"] = "multi_buffer_full";

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, read_opts, load_mesh);

    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // make changes to save mesh so the diff will pass
    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_units_and_labels)
{
    const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
        std::make_pair("points", "2"), std::make_pair("points", "3"),
        std::make_pair("points_implicit", "2"), std::make_pair("points_implicit", "3"),
        std::make_pair("quads", "2"),
        std::make_pair("hexs", "3"),
    };
    for (int i = 0; i < mesh_types.size(); ++i)
    {
        const std::string dim = mesh_types[i].second;
        index_t nx = 3;
        index_t ny = 4;
        index_t nz = (dim == "2" ? 0 : 2);

        const std::string mesh_type = mesh_types[i].first;

        const bool points_cases = mesh_type == "points" || mesh_type == "points_implicit";
        const bool do_overlink_too = ! points_cases;

        Node save_mesh, save_mesh_overlink, load_mesh, load_mesh_overlink, info;
        blueprint::mesh::examples::braid(mesh_type, nx, ny, nz, save_mesh);
        
        // add units and labels to coordset
        save_mesh["coordsets"]["coords"]["units"]["x"] = "these are my x units";
        save_mesh["coordsets"]["coords"]["units"]["y"] = "these are my y units";
        save_mesh["coordsets"]["coords"]["labels"]["x"] = "these are my x labels";
        save_mesh["coordsets"]["coords"]["labels"]["y"] = "these are my y labels";
        if (dim == "3")
        {
            save_mesh["coordsets"]["coords"]["units"]["z"] = "these are my z units";
            save_mesh["coordsets"]["coords"]["labels"]["z"] = "these are my z labels";
        }

        // add units and labels to fields
        save_mesh["fields"]["braid"]["units"] = "these are my braid units";
        save_mesh["fields"]["radial"]["units"] = "these are my radial units";
        save_mesh["fields"]["vel"]["units"] = "these are my vel units";
        save_mesh["fields"]["braid"]["label"] = "this is my braid label";
        save_mesh["fields"]["radial"]["label"] = "this is my radial label";
        save_mesh["fields"]["vel"]["label"] = "this is my vel label";

        // provide a matset for braid
        if (points_cases)
        {
            braid_init_example_matset(nx, ny, nz, save_mesh["matsets"]["matset"]);
        }
        else
        {
            const index_t nele_x = nx - 1;
            const index_t nele_y = ny - 1;
            const index_t nele_z = (dim == "2" ? 0 : nz - 1);
            braid_init_example_matset(nele_x, nele_y, nele_z, save_mesh["matsets"]["matset"]);
        }

        Node write_opts, read_opts;
        write_opts["file_style"] = "overlink";
        read_opts["matset_style"] = "multi_buffer_full";

        const std::string basename = "silo_braid_units_and_labels_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename + ".cycle_000100.root";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));
        const std::string basename_ovl = "overlink_braid_units_and_labels_" + mesh_type + "_" + dim + "D";
        const std::string filename_ovl = basename_ovl + "/OvlTop.silo";
        EXPECT_EQ(filename_ovl, io::blueprint::generate_root_filename(save_mesh, basename_ovl, "silo", write_opts));

        // remove existing root file, directory and any output files
        remove_path_if_exists(filename);

        io::silo::save_mesh(save_mesh, basename);
        io::silo::load_mesh(filename, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));
        if (do_overlink_too)
        {
            io::silo::save_mesh(save_mesh, basename_ovl, write_opts);
            io::silo::load_mesh(filename_ovl, read_opts, load_mesh_overlink);
            EXPECT_TRUE(blueprint::mesh::verify(load_mesh_overlink, info));
        }

        // make changes to save mesh so the diff will pass
        if (mesh_type == "points")
        {
            // this is custom code for braid
            // We know it is correct because the unstructured points version of braid
            // uses every point in the coordset
            save_mesh["topologies"].remove_child("mesh");
            save_mesh["topologies"]["mesh"]["type"] = "points";
            save_mesh["topologies"]["mesh"]["coordset"] = "coords";
        }
        if (mesh_type == "points_implicit" || mesh_type == "points")
        {
            // the association doesn't matter for point meshes
            // we choose vertex by convention
            save_mesh["fields"]["radial"]["association"].reset();
            save_mesh["fields"]["radial"]["association"] = "vertex";
        }
        
        if (do_overlink_too)
        {
            save_mesh_overlink.set(save_mesh);
            vector_field_to_scalars_braid(save_mesh_overlink, dim);
            overlink_name_changer(save_mesh_overlink);
        }
        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));

        EXPECT_TRUE(load_mesh[0]["coordsets"]["mesh_mesh"].has_child("units"));
        EXPECT_TRUE(load_mesh[0]["coordsets"]["mesh_mesh"].has_child("labels"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_braid"].has_child("units"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_braid"].has_child("label"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_radial"].has_child("units"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_radial"].has_child("label"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_vel"].has_child("units"));
        EXPECT_TRUE(load_mesh[0]["fields"]["mesh_vel"].has_child("label"));

        if (do_overlink_too)
        {
            // but the saved mesh is in the single domain format
            EXPECT_EQ(load_mesh_overlink.number_of_children(), 1);
            EXPECT_EQ(load_mesh_overlink[0].number_of_children(), save_mesh_overlink.number_of_children());
            EXPECT_FALSE(load_mesh_overlink[0].diff(save_mesh_overlink, info, CONDUIT_EPSILON, true));

            EXPECT_TRUE(load_mesh_overlink[0]["coordsets"]["MMESH"].has_child("units"));
            EXPECT_TRUE(load_mesh_overlink[0]["coordsets"]["MMESH"].has_child("labels"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["braid"].has_child("units"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["braid"].has_child("label"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["radial"].has_child("units"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["radial"].has_child("label"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_u"].has_child("units"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_u"].has_child("label"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_v"].has_child("units"));
            EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_v"].has_child("label"));
            if (dim == "3")
            {
                EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_w"].has_child("units"));
                EXPECT_TRUE(load_mesh_overlink[0]["fields"]["vel_w"].has_child("label"));
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, read_silo_units_and_labels_for_meshes)
{
    Node load_mesh, info;
    const std::string basename = "multi_curv3d";
    const std::string fileext  = ".silo";
    const std::string filepath = utils::join_file_path("silo", basename + fileext);
    const std::string input_file = relay_test_silo_data_path(filepath);

    io::silo::load_mesh(input_file, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    Node units, labels;
    units["x"] = "cm";
    units["y"] = "cm";
    units["z"] = "cm";
    labels["x"] = "X Axis";
    labels["y"] = "Y Axis";
    labels["z"] = "Z Axis";

    NodeConstIterator l_itr = load_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        EXPECT_TRUE(l_curr.has_path("coordsets/mesh1/units"));
        EXPECT_TRUE(l_curr.has_path("coordsets/mesh1/labels"));
        EXPECT_FALSE(l_curr["coordsets/mesh1/units"].diff(units, info));
        EXPECT_FALSE(l_curr["coordsets/mesh1/labels"].diff(labels, info));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, read_silo_units_for_fields)
{
    Node load_mesh, info;
    const std::string basename = "galaxy0000";
    const std::string fileext  = ".silo";
    const std::string filepath = utils::join_file_path("silo", basename + fileext);
    const std::string input_file = relay_test_silo_data_path(filepath);

    io::silo::load_mesh(input_file, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    Node units;
    units["Mass"] = "Solar masses";
    units["vx"] = "Km/s";

    NodeConstIterator l_itr = load_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        EXPECT_TRUE(l_curr.has_path("fields/Mass/units"));
        EXPECT_FALSE(l_curr["fields/Mass/units"].diff(units["Mass"], info));
        EXPECT_TRUE(l_curr.has_path("fields/vx/units"));
        EXPECT_FALSE(l_curr["fields/vx/units"].diff(units["vx"], info));
    }
}

//-----------------------------------------------------------------------------
// 
// test read and write semantics
// 

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, read_and_write_semantics)
{
    for (int ndomains = 2; ndomains < 6; ndomains ++)
    {
        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);

        const std::string basename = "silo_spiral_" + std::to_string(ndomains) + "_domains";
        const std::string filename = basename + ".cycle_000000.root";

        remove_path_if_exists(filename);
        io::silo::write_mesh(save_mesh, basename);
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));
        io::silo::read_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            silo_name_changer("mesh", save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }
    }
}

//-----------------------------------------------------------------------------
// 
// special case tests
// 

//-----------------------------------------------------------------------------
// var is not defined on a domain
// 
// tests the silo "EMPTY" capability
TEST(conduit_relay_io_silo, missing_domain_var)
{
    Node save_mesh, load_mesh, info;
    const int ndomains = 4;
    blueprint::mesh::examples::spiral(ndomains, save_mesh);

    // remove information for a particular domain
    save_mesh[2]["fields"].remove_child("dist");

    const std::string basename = "silo_missing_domain_var_spiral";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);
    }
    save_mesh[2].remove_child("fields");

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// matset is not defined on a domain
// 
// tests the silo "EMPTY" capability
TEST(conduit_relay_io_silo, missing_domain_matset)
{
    Node save_mesh, load_mesh, info;
    const int ndomains = 4;
    blueprint::mesh::examples::spiral(ndomains, save_mesh);
    add_matset_to_spiral(save_mesh, ndomains);
    EXPECT_TRUE(blueprint::mesh::verify(save_mesh, info));

    // remove information for a particular domain
    save_mesh[2]["matsets"].remove_child("matset");

    const std::string basename = "silo_missing_domain_matset_spiral";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

    // make changes to save mesh so the diff will pass
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        if (save_mesh[child].has_path("matsets/matset"))
        {
            // get the matset for this domain
            Node &n_matset = save_mesh[child]["matsets"]["matset"];

            // clean up volume fractions
            Node vf_arr;
            n_matset["volume_fractions"].to_float64_array(vf_arr);
            n_matset["volume_fractions"].reset();
            n_matset["volume_fractions"].set(vf_arr);
            
            // cheat a little bit - we don't have these to start
            n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
            n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);
        }

        silo_name_changer("mesh", save_mesh[child]);
    }
    save_mesh[2].remove_child("matsets");

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// mesh is not defined on a domain
// 
// This case is much less interesting.
// data passes through the clean mesh filter which
// deletes domains that are missing topos.
// They simply are not part of the mesh and so silo 
// doesn't have to deal with it.
TEST(conduit_relay_io_silo, missing_domain_mesh_trivial)
{
    Node save_mesh, load_mesh, info;
    const int ndomains = 4;
    blueprint::mesh::examples::spiral(ndomains, save_mesh);

    // remove information for a particular domain
    save_mesh[2]["topologies"].remove_child("topo");

    const std::string basename = "silo_missing_domain_mesh_trivial_spiral";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);

    EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

    // make changes to save mesh so the diff will pass
    save_mesh.remove(2);
    save_mesh.rename_child("domain_000003", "domain_000002");
    save_mesh[2]["state"]["domain_id"].reset();
    save_mesh[2]["state"]["domain_id"] = 2;
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);
    }

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// mesh is not defined on a domain but there are multiple meshes
TEST(conduit_relay_io_silo, missing_domain_mesh)
{
    Node save_mesh, save_mesh2, load_mesh, load_mesh2, info, opts;
    const int ndomains = 4;
    blueprint::mesh::examples::spiral(ndomains, save_mesh);
    blueprint::mesh::examples::spiral(ndomains, save_mesh2);

    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        save_mesh[child]["coordsets"].rename_child("coords", "coords2");
        save_mesh[child]["topologies"]["topo"]["coordset"].reset();
        save_mesh[child]["topologies"]["topo"]["coordset"] = "coords2";
        save_mesh[child]["topologies"].rename_child("topo", "topo2");
        save_mesh[child]["fields"]["dist"]["topology"].reset();
        save_mesh[child]["fields"]["dist"]["topology"] = "topo2";
        save_mesh[child]["fields"].rename_child("dist", "dist2");

        save_mesh[child]["coordsets"]["coords"].set_external(save_mesh2[child]["coordsets"]["coords"]);
        save_mesh[child]["topologies"]["topo"].set_external(save_mesh2[child]["topologies"]["topo"]);
        save_mesh[child]["fields"]["dist"].set_external(save_mesh2[child]["fields"]["dist"]);
    }

    // remove information for a particular domain
    save_mesh[2]["topologies"].remove_child("topo");

    const std::string basename = "silo_missing_domain_mesh_spiral";
    const std::string filename = basename + ".cycle_000000.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename);

    opts["mesh_name"] = "mesh_topo2";
    io::silo::load_mesh(filename, opts, load_mesh);
    opts["mesh_name"] = "mesh_topo";
    io::silo::load_mesh(filename, opts, load_mesh2);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh2, info));

    // make changes to save mesh so the diff will pass
    save_mesh[2]["coordsets"].remove_child("coords");
    save_mesh[2]["fields"].remove_child("dist");
    for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
    {
        silo_name_changer("mesh", save_mesh[child]);
    }

    // we must merge the two meshes in load mesh
    // the indexing is tricky because one is missing a domain
    load_mesh[0]["coordsets"]["mesh_topo"].set_external(load_mesh2[0]["coordsets"]["mesh_topo"]);
    load_mesh[0]["topologies"]["mesh_topo"].set_external(load_mesh2[0]["topologies"]["mesh_topo"]);
    load_mesh[0]["fields"]["mesh_dist"].set_external(load_mesh2[0]["fields"]["mesh_dist"]);
    load_mesh[1]["coordsets"]["mesh_topo"].set_external(load_mesh2[1]["coordsets"]["mesh_topo"]);
    load_mesh[1]["topologies"]["mesh_topo"].set_external(load_mesh2[1]["topologies"]["mesh_topo"]);
    load_mesh[1]["fields"]["mesh_dist"].set_external(load_mesh2[1]["fields"]["mesh_dist"]);
    load_mesh[3]["coordsets"]["mesh_topo"].set_external(load_mesh2[2]["coordsets"]["mesh_topo"]);
    load_mesh[3]["topologies"]["mesh_topo"].set_external(load_mesh2[2]["topologies"]["mesh_topo"]);
    load_mesh[3]["fields"]["mesh_dist"].set_external(load_mesh2[2]["fields"]["mesh_dist"]);

    EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
    NodeConstIterator l_itr = load_mesh.children();
    NodeConstIterator s_itr = save_mesh.children();
    while (l_itr.has_next())
    {
        const Node &l_curr = l_itr.next();
        const Node &s_curr = s_itr.next();

        EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// explicit points (unstructured mesh) do not use every coord
TEST(conduit_relay_io_silo, unstructured_points)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::braid("points", 2, 2, 2, save_mesh);

    std::vector<int> new_conn;
    std::vector<float> new_field1;
    std::vector<float> new_field2;
    std::vector<float64> new_xcoords, new_ycoords, new_zcoords;

    int_accessor conn = save_mesh["topologies"]["mesh"]["elements"]["connectivity"].value();

    float_accessor field1 = save_mesh["fields"]["braid"]["values"].value();
    float_accessor field2 = save_mesh["fields"]["radial"]["values"].value();

    float_accessor xcoords = save_mesh["coordsets"]["coords"]["values"]["x"].value();
    float_accessor ycoords = save_mesh["coordsets"]["coords"]["values"]["y"].value();
    float_accessor zcoords = save_mesh["coordsets"]["coords"]["values"]["z"].value();

    for (int i = 1; i < conn.number_of_elements(); i += 2)
    {
        new_conn.push_back(conn[i]);
        new_field1.push_back(field1[i]);
        new_field2.push_back(field2[i]);

        new_xcoords.push_back(xcoords[conn[i]]);
        new_ycoords.push_back(ycoords[conn[i]]);
        new_zcoords.push_back(zcoords[conn[i]]);
    }
    save_mesh["topologies"]["mesh"]["elements"]["connectivity"].reset();
    save_mesh["topologies"]["mesh"]["elements"]["connectivity"].set(new_conn);

    save_mesh["fields"].remove_child("vel");
    save_mesh["fields"]["braid"]["values"].reset();
    save_mesh["fields"]["braid"]["values"].set(new_field1);
    save_mesh["fields"]["radial"]["values"].reset();
    save_mesh["fields"]["radial"]["values"].set(new_field2);

    // we have modified braid such that it only uses half of the points in the coordset

    const std::string basename = "silo_unstructured_points_braid";
    const std::string filename = basename + ".cycle_000100.root";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo"));

    // remove existing root file, directory and any output files
    remove_path_if_exists(filename);

    io::silo::save_mesh(save_mesh, basename);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // now we must remove the unused points and change to an implicit points topo so that the diff passes
    save_mesh["coordsets"]["coords"]["values"]["x"].reset();
    save_mesh["coordsets"]["coords"]["values"]["x"].set(new_xcoords);
    save_mesh["coordsets"]["coords"]["values"]["y"].reset();
    save_mesh["coordsets"]["coords"]["values"]["y"].set(new_ycoords);
    save_mesh["coordsets"]["coords"]["values"]["z"].reset();
    save_mesh["coordsets"]["coords"]["values"]["z"].set(new_zcoords);

    save_mesh["topologies"].remove_child("mesh");
    save_mesh["topologies"]["mesh"]["type"] = "points";
    save_mesh["topologies"]["mesh"]["coordset"] = "coords";

    // the association doesn't matter for point meshes
    // we choose vertex by convention
    save_mesh["fields"]["radial"]["association"].reset();
    save_mesh["fields"]["radial"]["association"] = "vertex";

    silo_name_changer("mesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------

// 
// save option tests
// 

// save options:
/// opts:
///
///      file_style: "default", "root_only", "multi_file", "overlink"
///            when # of domains == 1,  "default"   ==> "root_only"
///            else,                    "default"   ==> "multi_file"
///
///      silo_type: "default", "pdb", "hdf5", "unknown"
///            when the file we are writing to exists, "default" ==> "unknown"
///            else,                                   "default" ==> "hdf5"
///         note: these are additional silo_type options that we could add
///         support for in the future:
///           "hdf5_sec2", "hdf5_stdio", "hdf5_mpio", "hdf5_mpiposix", "taurus"
///
///      suffix: "default", "cycle", "none"
///            when cycle is present,  "default"   ==> "cycle"
///            else,                   "default"   ==> "none"
///
///      root_file_ext: "default", "root", "silo"
///            "default"   ==> "root"
///            if overlink, this parameter is unused.
///
///      mesh_name:  (used if present, default ==> "mesh")
///
///      ovl_topo_name: (used if present, default ==> "")
///
///      nameschemes: "default", "yes", "no"
///            "default" ==> "no"
///
///      unified_types: "default", "yes", "no"
///            "default" ==> "yes"
///            prefer single mesh/var types versus writing an entire array
///            of types. "yes" will prefer this if possible, "no" will 
///            always write the entire array.
///
///      number_of_files:  {# of files}
///            when "multi_file" or "overlink":
///                 <= 0, use # of files ==> # of domains
///                  > 0, # of files ==> number_of_files

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_file_style)
{
    // we will do overlink tests separately
    const std::vector<std::string> file_styles = {"default", "root_only", "multi_file"};
    for (int i = 0; i < file_styles.size(); i ++)
    {
        Node opts;
        opts["file_style"] = file_styles[i];

        const std::string basename = "silo_save_option_file_style_" + file_styles[i] + "_spiral";
        const std::string filename = basename + ".cycle_000000.root";

        for (int ndomains = 1; ndomains < 5; ndomains += 3)
        {
            Node save_mesh, load_mesh, info;
            blueprint::mesh::examples::spiral(ndomains, save_mesh);
            remove_path_if_exists(filename);
            EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));
            io::silo::save_mesh(save_mesh, basename, opts);
            io::silo::load_mesh(filename, load_mesh);
            EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

            // make changes to save mesh so the diff will pass
            for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
            {
                silo_name_changer("mesh", save_mesh[child]);
            }

            EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
            NodeConstIterator l_itr = load_mesh.children();
            NodeConstIterator s_itr = save_mesh.children();
            while (l_itr.has_next())
            {
                const Node &l_curr = l_itr.next();
                const Node &s_curr = s_itr.next();

                EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_number_of_files)
{
    const std::vector<int> number_of_files = {-1, 2};
    for (int i = 0; i < number_of_files.size(); i ++)
    {
        Node opts;
        opts["file_style"] = "multi_file";
        opts["number_of_files"] = number_of_files[i];

        const std::string basename = "silo_save_option_number_of_files_" + 
                                     std::to_string(number_of_files[i]) + 
                                     "_spiral";
        const std::string filename = basename + ".cycle_000000.root";

        const int ndomains = 5;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            silo_name_changer("mesh", save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_suffix)
{
    const std::vector<std::string> suffixes = {"default", "default", "cycle", "none"};
    const std::vector<std::string> file_suffixes = {
        "",              // cycle is not present
        ".cycle_000005", // cycle is present
        ".cycle_000005", // cycle is turned on
        "",              // cycle is turned off
    };
    const std::vector<std::string> include_cycle = {"no", "yes", "yes", "yes"};
    for (int i = 0; i < suffixes.size(); i ++)
    {
        Node opts;
        opts["suffix"] = suffixes[i];

        const std::string basename = "silo_save_option_suffix_" + suffixes[i] +
                                     "_" + include_cycle[i] + "_basic";
        const std::string filename = basename + file_suffixes[i] + ".root";

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::basic("rectilinear", 3, 4, 0, save_mesh);

        if (include_cycle[i] == "yes")
        {
            save_mesh["state/cycle"] = 5;
        }
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_root_file_ext)
{
    const std::vector<std::string> root_file_exts = {"default", "root", "silo"};

    for (int i = 0; i < root_file_exts.size(); i ++)
    {
        Node opts;
        opts["root_file_ext"] = root_file_exts[i];

        std::string actual_file_ext = root_file_exts[i];
        if (actual_file_ext == "default")
        {
            actual_file_ext = "root";
        }

        const std::string basename = "round_trip_save_option_root_file_ext_" + 
                                     root_file_exts[i] + "_basic";
        const std::string filename = basename + "." + actual_file_ext;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::basic("rectilinear", 3, 4, 0, save_mesh);
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));
        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_mesh_name)
{
    Node opts;
    opts["mesh_name"] = "mymesh";

    const std::string basename = "silo_save_option_mesh_name_basic";
    const std::string filename = basename + ".root";

    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::basic("rectilinear", 3, 4, 0, save_mesh);
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));
    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename, opts);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    silo_name_changer("mymesh", save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_silo_type)
{
    const std::vector<std::string> silo_types = {"default", "pdb", "hdf5", "unknown"};
    for (int i = 0; i < silo_types.size(); i ++)
    {
        Node opts;
        opts["silo_type"] = silo_types[i];

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::basic("rectilinear", 3, 4, 0, save_mesh);

        const std::string basename = "silo_save_option_silo_type_" + silo_types[i] + "_basic";
        const std::string filename = basename + ".root";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));
        
        silo_name_changer("mesh", save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// this tests the unified types setting
TEST(conduit_relay_io_silo, round_trip_save_option_unified_types)
{
    const std::vector<std::string> unified_types = {"default", "yes", "no"};
    for (int i = 0; i < unified_types.size(); i ++)
    {
        const std::string basename = "silo_save_option_unified_types_" + 
                                     unified_types[i] + "_spiral";
        const std::string filename = basename + ".cycle_000000.root";
        const int ndomains = 5;

        Node write_opts;
        write_opts["unified_types"] = unified_types[i];

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, write_opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            silo_name_changer("mesh", save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }

        // open silo files and do some checks

        DBfile *rootfile = DBOpen(filename.c_str(), DB_UNKNOWN, DB_READ);
        
        // check multimesh
        {
            EXPECT_TRUE(DBInqVarExists(rootfile, "mesh_topo"));
            EXPECT_TRUE(DBInqVarType(rootfile, "mesh_topo") == DB_MULTIMESH);
    
            DBmultimesh *mmesh_ptr = DBGetMultimesh(rootfile, "mesh_topo");
    
            // fetch pointers to elements inside the mmesh
            int *mesh_types = mmesh_ptr->meshtypes;
            int  block_type = mmesh_ptr->block_type;
    
            if (unified_types[i] == "no")
            {
                for (int i = 0; i < ndomains; i ++)
                {
                    EXPECT_EQ(mesh_types[i], DB_QUADMESH);
                }
                EXPECT_NE(block_type, DB_QUADMESH);
            }
            else
            {
                EXPECT_EQ(mesh_types, nullptr);
                EXPECT_EQ(block_type, DB_QUADMESH);
            }
    
            DBFreeMultimesh(mmesh_ptr);
        }

        // check multivar
        {
            EXPECT_TRUE(DBInqVarExists(rootfile, "mesh_dist"));
            EXPECT_TRUE(DBInqVarType(rootfile, "mesh_dist") == DB_MULTIVAR);

            DBmultivar *mvar_ptr = DBGetMultivar(rootfile, "mesh_dist");

            // fetch pointers to elements inside the mvar
            int *var_types = mvar_ptr->vartypes;
            int block_type = mvar_ptr->block_type;

            if (unified_types[i] == "no")
            {
                for (int i = 0; i < ndomains; i ++)
                {
                    EXPECT_EQ(var_types[i], DB_QUADVAR);
                }
                EXPECT_NE(block_type, DB_QUADVAR);
            }
            else
            {
                EXPECT_EQ(var_types, nullptr);
                EXPECT_EQ(block_type, DB_QUADVAR);
            }

            DBFreeMultivar(mvar_ptr);
        }

        // close root file

        DBClose(rootfile);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_nameschemes_root_only)
{
    const std::vector<std::string> nameschemes = {"default", "yes", "no"};
    for (int i = 0; i < nameschemes.size(); i ++)
    {
        const std::string basename = "silo_save_option_nameschemes_root_only_" + 
                                     nameschemes[i] + "_spiral";
        const std::string filename = basename + ".cycle_000000.root";
        const int ndomains = 5;

        Node write_opts;
        write_opts["nameschemes"] = nameschemes[i];
        write_opts["file_style"] = "root_only";

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);
        add_matset_to_spiral(save_mesh, ndomains);

        // remove fields from domain 2 and 3 to trigger empty logic
        save_mesh[2].remove_child("fields");
        save_mesh[3].remove_child("fields");

        // remove matsets from domain 2 to trigger empty logic
        save_mesh[2].remove_child("matsets");

        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, write_opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            if (save_mesh[child].has_path("matsets/matset"))
            {
                // get the matset for this domain
                Node &n_matset = save_mesh[child]["matsets"]["matset"];

                // clean up volume fractions
                Node vf_arr;
                n_matset["volume_fractions"].to_float64_array(vf_arr);
                n_matset["volume_fractions"].reset();
                n_matset["volume_fractions"].set(vf_arr);
                
                // cheat a little bit - we don't have these to start
                n_matset["sizes"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["sizes"]);
                n_matset["offsets"].set_external(load_mesh[child]["matsets"]["mesh_matset"]["offsets"]);
            }

            silo_name_changer("mesh", save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }

        std::set<int> empty_var_domains;
        empty_var_domains.insert(2);
        empty_var_domains.insert(3);

        std::set<int> empty_matset_domains;
        empty_matset_domains.insert(2);

        // open silo files and do some checks

        DBfile *rootfile = DBOpen(filename.c_str(), DB_UNKNOWN, DB_READ);

        // check multimesh
        {
            EXPECT_TRUE(DBInqVarExists(rootfile, "mesh_topo"));
            EXPECT_TRUE(DBInqVarType(rootfile, "mesh_topo") == DB_MULTIMESH);
    
            DBmultimesh *mmesh_ptr = DBGetMultimesh(rootfile, "mesh_topo");
    
            // fetch pointers to elements inside the mesh
            char **meshnames  = mmesh_ptr->meshnames;
            char  *file_ns    = mmesh_ptr->file_ns;
            char  *block_ns   = mmesh_ptr->block_ns;
            int   *empty_list = mmesh_ptr->empty_list;
            int    empty_cnt  = mmesh_ptr->empty_cnt;
    
            if (nameschemes[i] == "yes")
            {
                EXPECT_EQ(meshnames, nullptr);
                EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/topo|n");
            }
            else
            {
                for (int domid = 0; domid < ndomains; domid ++)
                {
                    const std::string meshname = 
                        conduit_fmt::format("domain_{:06d}/mesh/topo", domid);
                    EXPECT_EQ(meshnames[domid], meshname);
                }
                EXPECT_EQ(block_ns, nullptr);
            }

            EXPECT_EQ(file_ns, nullptr);
            EXPECT_EQ(empty_list, nullptr);
            EXPECT_EQ(empty_cnt, 0);
    
            DBFreeMultimesh(mmesh_ptr);
        }

        // check multivar
        {
            EXPECT_TRUE(DBInqVarExists(rootfile, "mesh_dist"));
            EXPECT_TRUE(DBInqVarType(rootfile, "mesh_dist") == DB_MULTIVAR);

            DBmultivar *mvar_ptr = DBGetMultivar(rootfile, "mesh_dist");

            // fetch pointers to elements inside the mmvar
            char **varnames   = mvar_ptr->varnames;
            char  *file_ns    = mvar_ptr->file_ns;
            char  *block_ns   = mvar_ptr->block_ns;
            int   *empty_list = mvar_ptr->empty_list;
            int    empty_cnt  = mvar_ptr->empty_cnt;
            
            if (nameschemes[i] == "yes")
            {
                EXPECT_EQ(varnames, nullptr);
                EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/dist|n");
                EXPECT_EQ(empty_cnt, 2);
                EXPECT_EQ(empty_list[0], 2);
                EXPECT_EQ(empty_list[1], 3);
            }
            else
            {
                for (int domid = 0; domid < ndomains; domid ++)
                {
                    if (empty_var_domains.find(domid) != empty_var_domains.end())
                    {
                        EXPECT_EQ(varnames[domid], std::string("EMPTY"));
                    }
                    else
                    {
                        const std::string varname = 
                            conduit_fmt::format("domain_{:06d}/mesh/dist", domid);
                        EXPECT_EQ(varnames[domid], varname);
                    }
                }
                EXPECT_EQ(block_ns, nullptr);
                EXPECT_EQ(empty_cnt, 0);
                EXPECT_EQ(empty_list, nullptr);
            }

            EXPECT_EQ(file_ns, nullptr);

            DBFreeMultivar(mvar_ptr);
        }

        // check multimat
        {
            EXPECT_TRUE(DBInqVarExists(rootfile, "mesh_matset"));
            EXPECT_TRUE(DBInqVarType(rootfile, "mesh_matset") == DB_MULTIMAT);

            DBmultimat *multimat_ptr = DBGetMultimat(rootfile, "mesh_matset");

            // fetch pointers to elements inside the mmvar
            char **matnames   = multimat_ptr->matnames;
            char  *file_ns    = multimat_ptr->file_ns;
            char  *block_ns   = multimat_ptr->block_ns;
            int   *empty_list = multimat_ptr->empty_list;
            int    empty_cnt  = multimat_ptr->empty_cnt;
            
            if (nameschemes[i] == "yes")
            {
                EXPECT_EQ(matnames, nullptr);
                EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/matset|n");
                EXPECT_EQ(empty_cnt, 1);
                EXPECT_EQ(empty_list[0], 2);
            }
            else
            {
                for (int domid = 0; domid < ndomains; domid ++)
                {
                    if (empty_matset_domains.find(domid) != empty_matset_domains.end())
                    {
                        EXPECT_EQ(matnames[domid], std::string("EMPTY"));
                    }
                    else
                    {
                        const std::string matname = 
                            conduit_fmt::format("domain_{:06d}/mesh/matset", domid);
                        EXPECT_EQ(matnames[domid], matname);
                    }
                }
                EXPECT_EQ(block_ns, nullptr);
                EXPECT_EQ(empty_cnt, 0);
                EXPECT_EQ(empty_list, nullptr);
            }

            EXPECT_EQ(file_ns, nullptr);

            DBFreeMultimat(multimat_ptr);
        }

        // check dom2filemap
        {
            EXPECT_FALSE(DBInqVarExists(rootfile, "dom2filemap"));
        }

        // close root file

        DBClose(rootfile);
    }
}

//-----------------------------------------------------------------------------
// also tests overlink nameschemes case
TEST(conduit_relay_io_silo, round_trip_save_option_nameschemes_n_files_n_domains)
{
    const std::vector<std::string> nameschemes = {"default", "yes", "no"};
    const std::vector<std::string> file_styles = {"default", "overlink"};
    for (int i = 0; i < nameschemes.size(); i ++)
    {
        for (int j = 0; j < file_styles.size(); j ++)
        {
            const std::string basename = std::string(file_styles[j] == "overlink" ? "overlink" : "silo") + 
                                         "_save_option_nameschemes_n_files_n_domains_" + 
                                         nameschemes[i] + "_spiral";
            const std::string filename = (file_styles[j] == "overlink" ? 
                                          basename + "/OvlTop.silo" :
                                          basename + ".cycle_000000.root");
            const int ndomains = 5;

            Node write_opts;
            write_opts["nameschemes"] = nameschemes[i];
            write_opts["file_style"] = file_styles[j];

            Node save_mesh, load_mesh, info;
            blueprint::mesh::examples::spiral(ndomains, save_mesh);
            add_matset_to_spiral(save_mesh, ndomains);

            // remove fields from domain 2 and 3 to trigger empty logic
            save_mesh[2].remove_child("fields");
            save_mesh[3].remove_child("fields");

            // we can't do this for the overlink case b/c overlink requires
            // a matset on every rank
            if (file_styles[j] == "default")
            {
                // remove matsets from domain 2 to trigger empty logic
                save_mesh[2].remove_child("matsets");
            }

            EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

            remove_path_if_exists(filename);
            io::silo::save_mesh(save_mesh, basename, write_opts);
            io::silo::load_mesh(filename, load_mesh);
            EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

            // make changes to save mesh so the diff will pass
            for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
            {
                if (save_mesh[child].has_path("matsets/matset"))
                {
                    // get the matset for this domain
                    Node &n_matset = save_mesh[child]["matsets"]["matset"];

                    // clean up volume fractions
                    Node vf_arr;
                    n_matset["volume_fractions"].to_float64_array(vf_arr);
                    n_matset["volume_fractions"].reset();
                    n_matset["volume_fractions"].set(vf_arr);

                    const std::string matset_name = (file_styles[j] == "overlink" ? 
                                                     "MMATERIAL" : 
                                                     "mesh_matset");
                    
                    // cheat a little bit - we don't have these to start
                    n_matset["sizes"].set_external(load_mesh[child]["matsets"][matset_name]["sizes"]);
                    n_matset["offsets"].set_external(load_mesh[child]["matsets"][matset_name]["offsets"]);
                }

                if (file_styles[j] == "overlink")
                {
                    overlink_name_changer(save_mesh[child]);
                }
                else
                {
                    silo_name_changer("mesh", save_mesh[child]);
                }
            }

            EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
            NodeConstIterator l_itr = load_mesh.children();
            NodeConstIterator s_itr = save_mesh.children();
            while (l_itr.has_next())
            {
                const Node &l_curr = l_itr.next();
                const Node &s_curr = s_itr.next();

                EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
            }

            std::set<int> empty_var_domains;
            empty_var_domains.insert(2);
            empty_var_domains.insert(3);

            std::set<int> empty_matset_domains;
            empty_matset_domains.insert(2);

            // open silo files and do some checks

            DBfile *rootfile = DBOpen(filename.c_str(), DB_UNKNOWN, DB_READ);

            const std::string file_namescheme = (file_styles[j] == "overlink" ? 
                                                 "|" + basename + "/domain%d.silo|n" : 
                                                 "|" + basename + ".cycle_000000/domain_%06d.silo|n");
            
            // check multimesh
            {
                const std::string topo_name = (file_styles[j] == "overlink" ? "MMESH" : "mesh_topo");
                EXPECT_TRUE(DBInqVarExists(rootfile, topo_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, topo_name.c_str()) == DB_MULTIMESH);
        
                DBmultimesh *mmesh_ptr = DBGetMultimesh(rootfile, topo_name.c_str());
        
                // fetch pointers to elements inside the mesh
                char **meshnames  = mmesh_ptr->meshnames;
                char  *file_ns    = mmesh_ptr->file_ns;
                char  *block_ns   = mmesh_ptr->block_ns;
                int   *empty_list = mmesh_ptr->empty_list;
                int    empty_cnt  = mmesh_ptr->empty_cnt;
        
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(meshnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|MESH");
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|mesh/topo");
                    }
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string meshname = 
                                conduit_fmt::format(
                                    basename + "/domain{:d}.silo:MESH", domid);
                            EXPECT_EQ(meshnames[domid], meshname);
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string meshname = 
                                conduit_fmt::format(
                                    basename + ".cycle_000000/domain_{:06d}.silo:mesh/topo", domid);
                            EXPECT_EQ(meshnames[domid], meshname);
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                }

                EXPECT_EQ(empty_list, nullptr);
                EXPECT_EQ(empty_cnt, 0);
        
                DBFreeMultimesh(mmesh_ptr);
            }

            // check multivar
            {
                const std::string var_name = (file_styles[j] == "overlink" ? "dist" : "mesh_dist");
                EXPECT_TRUE(DBInqVarExists(rootfile, var_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, var_name.c_str()) == DB_MULTIVAR);

                DBmultivar *mvar_ptr = DBGetMultivar(rootfile, var_name.c_str());

                // fetch pointers to elements inside the mmvar
                char **varnames   = mvar_ptr->varnames;
                char  *file_ns    = mvar_ptr->file_ns;
                char  *block_ns   = mvar_ptr->block_ns;
                int   *empty_list = mvar_ptr->empty_list;
                int    empty_cnt  = mvar_ptr->empty_cnt;
                
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(varnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|dist");
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|mesh/dist");
                    }
                    EXPECT_EQ(empty_cnt, 2);
                    EXPECT_EQ(empty_list[0], 2);
                    EXPECT_EQ(empty_list[1], 3);
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_var_domains.find(domid) != empty_var_domains.end())
                            {
                                EXPECT_EQ(varnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string varname = 
                                    conduit_fmt::format(
                                        basename + "/domain{:d}.silo:dist", domid);
                                EXPECT_EQ(varnames[domid], varname);
                            }
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_var_domains.find(domid) != empty_var_domains.end())
                            {
                                EXPECT_EQ(varnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string varname = 
                                    conduit_fmt::format(
                                        basename + ".cycle_000000/domain_{:06d}.silo:mesh/dist", domid);
                                EXPECT_EQ(varnames[domid], varname);
                            }
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                    EXPECT_EQ(empty_cnt, 0);
                    EXPECT_EQ(empty_list, nullptr);
                }

                DBFreeMultivar(mvar_ptr);
            }

            // check multimat
            {
                const std::string mat_name = (file_styles[j] == "overlink" ? "MMATERIAL" : "mesh_matset");
                EXPECT_TRUE(DBInqVarExists(rootfile, mat_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, mat_name.c_str()) == DB_MULTIMAT);

                DBmultimat *multimat_ptr = DBGetMultimat(rootfile, mat_name.c_str());

                // fetch pointers to elements inside the mmvar
                char **matnames   = multimat_ptr->matnames;
                char  *file_ns    = multimat_ptr->file_ns;
                char  *block_ns   = multimat_ptr->block_ns;
                int   *empty_list = multimat_ptr->empty_list;
                int    empty_cnt  = multimat_ptr->empty_cnt;
                
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(matnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|MATERIAL");
                        EXPECT_EQ(empty_cnt, 0);
                        EXPECT_EQ(empty_list, nullptr);
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|mesh/matset");
                        EXPECT_EQ(empty_cnt, 1);
                        EXPECT_EQ(empty_list[0], 2);
                    }
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string matname = 
                                conduit_fmt::format(
                                    basename + "/domain{:d}.silo:MATERIAL", domid);
                            EXPECT_EQ(matnames[domid], matname);
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_matset_domains.find(domid) != empty_matset_domains.end())
                            {
                                EXPECT_EQ(matnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string matname = 
                                    conduit_fmt::format(
                                        basename + ".cycle_000000/domain_{:06d}.silo:mesh/matset", domid);
                                EXPECT_EQ(matnames[domid], matname);
                            }
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                    EXPECT_EQ(empty_cnt, 0);
                    EXPECT_EQ(empty_list, nullptr);
                }

                DBFreeMultimat(multimat_ptr);
            }

            // check dom2filemap
            {
                EXPECT_FALSE(DBInqVarExists(rootfile, "dom2filemap"));
            }

            // close root file

            DBClose(rootfile);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_nameschemes_m_domains_n_files)
{
    const std::vector<std::string> nameschemes = {"default", "yes", "no"};
    const std::vector<std::string> file_styles = {"default", "overlink"};
    for (int i = 0; i < nameschemes.size(); i ++)
    {
        for (int j = 0; j < file_styles.size(); j ++)
        {
            const std::string basename = std::string(file_styles[j] == "overlink" ? "overlink" : "silo") + 
                                         "_save_option_nameschemes_m_domains_n_files_" + 
                                         nameschemes[i] + "_spiral";
            const std::string filename = (file_styles[j] == "overlink" ? 
                                          basename + "/OvlTop.silo" :
                                          basename + ".cycle_000000.root");
            const int ndomains = 5;

            Node save_mesh, load_mesh, info;
            blueprint::mesh::examples::spiral(ndomains, save_mesh);
            add_matset_to_spiral(save_mesh, ndomains);

            Node write_opts;
            write_opts["nameschemes"] = nameschemes[i];
            write_opts["number_of_files"] = 3;
            write_opts["file_style"] = file_styles[j];

            // remove fields from domain 2 and 3 to trigger empty logic
            save_mesh[2].remove_child("fields");
            save_mesh[3].remove_child("fields");

            // we can't do this for the overlink case b/c overlink requires
            // a matset on every rank
            if (file_styles[j] == "default")
            {
                // remove matsets from domain 2 to trigger empty logic
                save_mesh[2].remove_child("matsets");
            }

            EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

            remove_path_if_exists(filename);
            io::silo::save_mesh(save_mesh, basename, write_opts);
            io::silo::load_mesh(filename, load_mesh);
            EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

            // make changes to save mesh so the diff will pass
            for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
            {
                if (save_mesh[child].has_path("matsets/matset"))
                {
                    // get the matset for this domain
                    Node &n_matset = save_mesh[child]["matsets"]["matset"];

                    // clean up volume fractions
                    Node vf_arr;
                    n_matset["volume_fractions"].to_float64_array(vf_arr);
                    n_matset["volume_fractions"].reset();
                    n_matset["volume_fractions"].set(vf_arr);

                    const std::string matset_name = (file_styles[j] == "overlink" ? 
                                                     "MMATERIAL" : 
                                                     "mesh_matset");
                    
                    // cheat a little bit - we don't have these to start
                    n_matset["sizes"].set_external(load_mesh[child]["matsets"][matset_name]["sizes"]);
                    n_matset["offsets"].set_external(load_mesh[child]["matsets"][matset_name]["offsets"]);
                }

                if (file_styles[j] == "overlink")
                {
                    overlink_name_changer(save_mesh[child]);
                }
                else
                {
                    silo_name_changer("mesh", save_mesh[child]);
                }
            }

            EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
            NodeConstIterator l_itr = load_mesh.children();
            NodeConstIterator s_itr = save_mesh.children();
            while (l_itr.has_next())
            {
                const Node &l_curr = l_itr.next();
                const Node &s_curr = s_itr.next();

                EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
            }

            std::map<int, int> dom2filemap;
            dom2filemap[0] = 0;
            dom2filemap[1] = 0;
            dom2filemap[2] = 1;
            dom2filemap[3] = 1;
            dom2filemap[4] = 2;

            std::set<int> empty_var_domains;
            empty_var_domains.insert(2);
            empty_var_domains.insert(3);

            std::set<int> empty_matset_domains;
            empty_matset_domains.insert(2);

            // open silo files and do some checks

            DBfile *rootfile = DBOpen(filename.c_str(), DB_UNKNOWN, DB_READ);
            
            const std::string file_namescheme = (file_styles[j] == "overlink" ? 
                                                 "|" + basename + "/domfile%d.silo|#dom2filemap[n]" : 
                                                 "|" + basename + ".cycle_000000/file_%06d.silo|#dom2filemap[n]");

            // check multimesh
            {
                const std::string topo_name = (file_styles[j] == "overlink" ? "MMESH" : "mesh_topo");
                EXPECT_TRUE(DBInqVarExists(rootfile, topo_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, topo_name.c_str()) == DB_MULTIMESH);
        
                DBmultimesh *mmesh_ptr = DBGetMultimesh(rootfile, topo_name.c_str());
        
                // fetch pointers to elements inside the mesh
                char **meshnames  = mmesh_ptr->meshnames;
                char  *file_ns    = mmesh_ptr->file_ns;
                char  *block_ns   = mmesh_ptr->block_ns;
                int   *empty_list = mmesh_ptr->empty_list;
                int    empty_cnt  = mmesh_ptr->empty_cnt;
        
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(meshnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain%d/MESH|n");
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/topo|n");
                    }
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string meshname = 
                                conduit_fmt::format(
                                    basename + "/domfile{:d}.silo:domain{:d}/MESH",
                                    dom2filemap.at(domid),
                                    domid);
                            EXPECT_EQ(meshnames[domid], meshname);
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string meshname = 
                                conduit_fmt::format(
                                    basename + ".cycle_000000/file_{:06d}.silo:domain_{:06d}/mesh/topo",
                                    dom2filemap.at(domid),
                                    domid);
                            EXPECT_EQ(meshnames[domid], meshname);
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                }

                EXPECT_EQ(empty_list, nullptr);
                EXPECT_EQ(empty_cnt, 0);
        
                DBFreeMultimesh(mmesh_ptr);
            }

            // check multivar
            {
                const std::string var_name = (file_styles[j] == "overlink" ? "dist" : "mesh_dist");
                EXPECT_TRUE(DBInqVarExists(rootfile, var_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, var_name.c_str()) == DB_MULTIVAR);

                DBmultivar *mvar_ptr = DBGetMultivar(rootfile, var_name.c_str());

                // fetch pointers to elements inside the mmvar
                char **varnames   = mvar_ptr->varnames;
                char  *file_ns    = mvar_ptr->file_ns;
                char  *block_ns   = mvar_ptr->block_ns;
                int   *empty_list = mvar_ptr->empty_list;
                int    empty_cnt  = mvar_ptr->empty_cnt;
                
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(varnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain%d/dist|n");
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/dist|n");
                    }
                    EXPECT_EQ(empty_cnt, 2);
                    EXPECT_EQ(empty_list[0], 2);
                    EXPECT_EQ(empty_list[1], 3);
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_var_domains.find(domid) != empty_var_domains.end())
                            {
                                EXPECT_EQ(varnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string varname = 
                                    conduit_fmt::format(
                                        basename + "/domfile{:d}.silo:domain{:d}/dist",
                                        dom2filemap.at(domid),
                                        domid);
                                EXPECT_EQ(varnames[domid], varname);
                            }
                            
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_var_domains.find(domid) != empty_var_domains.end())
                            {
                                EXPECT_EQ(varnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string varname = 
                                    conduit_fmt::format(
                                        basename + ".cycle_000000/file_{:06d}.silo:domain_{:06d}/mesh/dist",
                                        dom2filemap.at(domid),
                                        domid);
                                EXPECT_EQ(varnames[domid], varname);
                            }
                            
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                    EXPECT_EQ(empty_list, nullptr);
                    EXPECT_EQ(empty_cnt, 0);
                }

                DBFreeMultivar(mvar_ptr);
            }

            // check multimat
            {
                const std::string mat_name = (file_styles[j] == "overlink" ? "MMATERIAL" : "mesh_matset");
                EXPECT_TRUE(DBInqVarExists(rootfile, mat_name.c_str()));
                EXPECT_TRUE(DBInqVarType(rootfile, mat_name.c_str()) == DB_MULTIMAT);

                DBmultimat *multimat_ptr = DBGetMultimat(rootfile, mat_name.c_str());

                // fetch pointers to elements inside the mmvar
                char **matnames   = multimat_ptr->matnames;
                char  *file_ns    = multimat_ptr->file_ns;
                char  *block_ns   = multimat_ptr->block_ns;
                int   *empty_list = multimat_ptr->empty_list;
                int    empty_cnt  = multimat_ptr->empty_cnt;
                
                if (nameschemes[i] == "yes")
                {
                    EXPECT_EQ(matnames, nullptr);
                    EXPECT_EQ(std::string(file_ns), file_namescheme);
                    if (file_styles[j] == "overlink")
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain%d/MATERIAL|n");
                        EXPECT_EQ(empty_cnt, 0);
                        EXPECT_EQ(empty_list, nullptr);
                    }
                    else
                    {
                        EXPECT_EQ(std::string(block_ns), "|domain_%06d/mesh/matset|n");
                        EXPECT_EQ(empty_cnt, 1);
                        EXPECT_EQ(empty_list[0], 2);
                    }
                }
                else
                {
                    if (file_styles[j] == "overlink")
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            const std::string matname = 
                                conduit_fmt::format(
                                    basename + "/domfile{:d}.silo:domain{:d}/MATERIAL",
                                    dom2filemap.at(domid),
                                    domid);
                            EXPECT_EQ(matnames[domid], matname);
                        }
                    }
                    else
                    {
                        for (int domid = 0; domid < ndomains; domid ++)
                        {
                            if (empty_matset_domains.find(domid) != empty_matset_domains.end())
                            {
                                EXPECT_EQ(matnames[domid], std::string("EMPTY"));
                            }
                            else
                            {
                                const std::string matname = 
                                    conduit_fmt::format(
                                        basename + ".cycle_000000/file_{:06d}.silo:domain_{:06d}/mesh/matset",
                                        dom2filemap.at(domid),
                                        domid);
                                EXPECT_EQ(matnames[domid], matname);
                            }
                        }
                    }
                    EXPECT_EQ(file_ns, nullptr);
                    EXPECT_EQ(block_ns, nullptr);
                    EXPECT_EQ(empty_list, nullptr);
                    EXPECT_EQ(empty_cnt, 0);
                }

                DBFreeMultimat(multimat_ptr);
            }

            // check dom2filemap
            {
                if (nameschemes[i] == "yes")
                {
                    EXPECT_TRUE(DBInqVarExists(rootfile, "dom2filemap"));
                    EXPECT_TRUE(DBInqVarType(rootfile, "dom2filemap") == DB_VARIABLE);
                    int* data_ptr = new int[5];
                    DBReadVar(rootfile, "dom2filemap", static_cast<void *>(data_ptr));
                    EXPECT_EQ(data_ptr[0], 0);
                    EXPECT_EQ(data_ptr[1], 0);
                    EXPECT_EQ(data_ptr[2], 1);
                    EXPECT_EQ(data_ptr[3], 1);
                    EXPECT_EQ(data_ptr[4], 2);

                    delete[] data_ptr;
                }
                else
                {
                    EXPECT_FALSE(DBInqVarExists(rootfile, "dom2filemap"));
                }
            }

            // close root file

            DBClose(rootfile);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_save_option_overlink1)
{
    const std::vector<std::string> ovl_topo_names = {"", "topo"};
    for (int i = 0; i < ovl_topo_names.size(); i ++)
    {
        Node opts;
        opts["file_style"] = "overlink";
        opts["ovl_topo_name"] = ovl_topo_names[i];

        std::string basename;
        if (ovl_topo_names[i].empty())
        {
            basename = "silo_save_option_overlink_spiral";
        }
        else
        {
            basename = "silo_save_option_overlink_spiral_" + ovl_topo_names[i];
        }
        const std::string filename = basename + "/OvlTop.silo";

        int ndomains = 2;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::spiral(ndomains, save_mesh);
        add_matset_to_spiral(save_mesh, ndomains);
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));
        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, opts);
        io::silo::load_mesh(filename, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        // make changes to save mesh so the diff will pass
        for (index_t child = 0; child < save_mesh.number_of_children(); child ++)
        {
            // get the matset for this domain
            Node &n_matset = save_mesh[child]["matsets"]["matset"];

            // clean up volume fractions
            Node vf_arr;
            n_matset["volume_fractions"].to_float64_array(vf_arr);
            n_matset["volume_fractions"].reset();
            n_matset["volume_fractions"].set(vf_arr);
            
            // cheat a little bit - we don't have these to start
            n_matset["sizes"].set_external(load_mesh[child]["matsets"]["MMATERIAL"]["sizes"]);
            n_matset["offsets"].set_external(load_mesh[child]["matsets"]["MMATERIAL"]["offsets"]);

            overlink_name_changer(save_mesh[child]);
        }

        EXPECT_EQ(load_mesh.number_of_children(), save_mesh.number_of_children());
        NodeConstIterator l_itr = load_mesh.children();
        NodeConstIterator s_itr = save_mesh.children();
        while (l_itr.has_next())
        {
            const Node &l_curr = l_itr.next();
            const Node &s_curr = s_itr.next();

            EXPECT_FALSE(l_curr.diff(s_curr, info, CONDUIT_EPSILON, true));
        }
    }
}

//-----------------------------------------------------------------------------
// this tests var attributes and padding dimensions
TEST(conduit_relay_io_silo, round_trip_save_option_overlink2)
{
    Node write_opts, read_opts;
    write_opts["file_style"] = "overlink";
    read_opts["matset_style"] = "multi_buffer_full";

    const std::string basename = "silo_save_option_overlink_basic";
    const std::string filename = basename + "/OvlTop.silo";

    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::basic("structured", 3, 3, 1, save_mesh);

    // add another field that is volume dependent
    Node &field2 = save_mesh["fields"]["field2"];
    field2["association"] = "element";
    field2["topology"] = "mesh";
    field2["volume_dependent"] = "true";
    field2["values"].set_external(save_mesh["fields"]["field"]["values"]);

    // add a matset to make overlink happy
    add_multi_buffer_full_matset(save_mesh, 4, "mesh");

    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));
    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename, write_opts);
    io::silo::load_mesh(filename, read_opts, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

    // make changes to save mesh so the diff will pass
    overlink_name_changer(save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));

    // open silo files and do some checks

    DBfile *rootfile = DBOpen(filename.c_str(), DB_UNKNOWN, DB_READ);
    EXPECT_TRUE(DBInqVarExists(rootfile, "VAR_ATTRIBUTES"));
    EXPECT_TRUE(DBInqVarType(rootfile, "VAR_ATTRIBUTES") == DB_ARRAY);

    DBcompoundarray *var_attr = DBGetCompoundarray(rootfile, "VAR_ATTRIBUTES");

    // fetch pointers to elements inside the compound array
    char **elemnames = var_attr->elemnames;
    int *elemlengths = var_attr->elemlengths;
    int nelems       = var_attr->nelems;
    int *values      = static_cast<int *>(var_attr->values);
    int nvalues      = var_attr->nvalues;
    int datatype     = var_attr->datatype;

    EXPECT_EQ(std::string(elemnames[0]), "field");
    EXPECT_EQ(std::string(elemnames[1]), "field2");
    EXPECT_EQ(elemlengths[0], 5);
    EXPECT_EQ(elemlengths[1], 5);
    EXPECT_EQ(nelems, 2);
    // for first var
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 0);
    EXPECT_EQ(values[2], 1);
    EXPECT_EQ(values[3], 0);
    EXPECT_EQ(values[4], 1);
    // for second var
    EXPECT_EQ(values[5], 1);
    EXPECT_EQ(values[6], 1);
    EXPECT_EQ(values[7], 1);
    EXPECT_EQ(values[8], 0);
    EXPECT_EQ(values[9], 1);
    EXPECT_EQ(nvalues, 10);
    EXPECT_EQ(datatype, DB_INT);

    DBFreeCompoundarray(var_attr);

    EXPECT_TRUE(DBInqVarExists(rootfile, "PAD_DIMS"));
    EXPECT_TRUE(DBInqVarType(rootfile, "PAD_DIMS") == DB_ARRAY);

    DBcompoundarray *pad_dims = DBGetCompoundarray(rootfile, "PAD_DIMS");

    // fetch pointers to elements inside the compound array
    elemnames   = pad_dims->elemnames;
    elemlengths = pad_dims->elemlengths;
    nelems      = pad_dims->nelems;
    values      = static_cast<int *>(pad_dims->values);
    nvalues     = pad_dims->nvalues;
    datatype    = pad_dims->datatype;

    EXPECT_EQ(std::string(elemnames[0]), "paddims");
    EXPECT_EQ(elemlengths[0], 6);
    EXPECT_EQ(nelems, 1);
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(values[1], 0);
    EXPECT_EQ(values[2], 0);
    EXPECT_EQ(values[3], 0);
    EXPECT_EQ(values[4], 0);
    EXPECT_EQ(values[5], 0);
    EXPECT_EQ(nvalues, 6);
    EXPECT_EQ(datatype, DB_INT);

    DBFreeCompoundarray(pad_dims);

    DBClose(rootfile);

    // now check domain file

    const std::string dom_filename = basename + "/domain0.silo";
    DBfile *domfile = DBOpen(dom_filename.c_str(), DB_UNKNOWN, DB_READ);

    EXPECT_TRUE(DBInqVarExists(domfile, "DOMAIN_NEIGHBOR_NUMS"));
    EXPECT_TRUE(DBInqVarType(domfile, "DOMAIN_NEIGHBOR_NUMS") == DB_ARRAY);

    DBcompoundarray *dom_neighbor_nums = DBGetCompoundarray(domfile, "DOMAIN_NEIGHBOR_NUMS");

    // fetch pointers to elements inside the compound array
    elemnames   = dom_neighbor_nums->elemnames;
    elemlengths = dom_neighbor_nums->elemlengths;
    nelems      = dom_neighbor_nums->nelems;
    values      = static_cast<int *>(dom_neighbor_nums->values);
    nvalues     = dom_neighbor_nums->nvalues;
    datatype    = dom_neighbor_nums->datatype;

    EXPECT_EQ(std::string(elemnames[0]), "num_neighbors");
    EXPECT_EQ(std::string(elemnames[1]), "neighbor_nums");
    EXPECT_EQ(elemlengths[0], 1);
    EXPECT_EQ(elemlengths[1], 0);
    EXPECT_EQ(nelems, 2);
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(nvalues, 1);
    EXPECT_EQ(datatype, DB_INT);

    DBFreeCompoundarray(dom_neighbor_nums);

    DBClose(domfile);
}

//-----------------------------------------------------------------------------
// this tests material i/o
TEST(conduit_relay_io_silo, round_trip_save_option_overlink3)
{
    Node save_mesh, load_mesh, info;
    const int nx = 100, ny = 100;
    const double radius = 0.25;
    blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, save_mesh);

    Node opts;
    opts["file_style"] = "overlink";

    const std::string basename = "silo_save_option_overlink_venn";
    const std::string filename = basename + "/OvlTop.silo";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", opts));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename, opts);
    io::silo::load_mesh(filename, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // make changes to save mesh so the diff will pass

    // The field mat_check has values that are one type and matset_values
    // that are another type. The silo writer converts both to double arrays
    // in this case, so we follow suit.
    Node mat_check_new_values, mat_check_new_matset_values;
    save_mesh["fields"]["mat_check"]["values"].to_double_array(mat_check_new_values);
    save_mesh["fields"]["mat_check"]["matset_values"].to_double_array(mat_check_new_matset_values);
    save_mesh["fields"]["mat_check"]["values"].set_external(mat_check_new_values);
    save_mesh["fields"]["mat_check"]["matset_values"].set_external(mat_check_new_matset_values);

    overlink_name_changer(save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
// we are testing vector fields get converted to scalars
TEST(conduit_relay_io_silo, round_trip_save_option_overlink4)
{
    const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
        std::make_pair("structured", "2"), std::make_pair("structured", "3"),
        std::make_pair("quads", "2"),
        std::make_pair("hexs", "3"),
    };
    for (int i = 0; i < mesh_types.size(); ++i)
    {
        std::string dim = mesh_types[i].second;
        index_t nx = 3;
        index_t ny = 4;
        index_t nz = (dim == "2" ? 0 : 2);

        std::string mesh_type = mesh_types[i].first;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::braid(mesh_type, nx, ny, nz, save_mesh);
        const index_t nele_x = nx - 1;
        const index_t nele_y = ny - 1;
        const index_t nele_z = (dim == "2" ? 0 : nz - 1);

        // provide a matset for braid
        braid_init_example_matset(nele_x, nele_y, nele_z, save_mesh["matsets"]["matset"]);

        Node write_opts, read_opts;
        write_opts["file_style"] = "overlink";
        read_opts["matset_style"] = "multi_buffer_full";

        const std::string basename = "silo_save_option_overlink_braid_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename + "/OvlTop.silo";
        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

        // remove existing root file, directory and any output files
        remove_path_if_exists(filename);
        io::silo::save_mesh(save_mesh, basename, write_opts);
        io::silo::load_mesh(filename, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        vector_field_to_scalars_braid(save_mesh, dim);

        // make changes to save mesh so the diff will pass
        overlink_name_changer(save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// check that all the shape types work (specifically polytopal ones)
TEST(conduit_relay_io_silo, round_trip_save_option_overlink5)
{
    const std::vector<std::pair<std::string, std::string>> mesh_types = {
        std::make_pair("uniform", "2"), std::make_pair("uniform", "3"),
        std::make_pair("rectilinear", "2"), std::make_pair("rectilinear", "3"),
        std::make_pair("structured", "2"), std::make_pair("structured", "3"),
        std::make_pair("quads", "2"),
        std::make_pair("polygons", "2"),
        std::make_pair("hexs", "3"),
        // std::make_pair("polyhedra", "3") // TODO
        // Overlink does not support tris, wedges, pyramids, or tets
    };
    for (int i = 0; i < mesh_types.size(); ++i)
    {
        const std::string dim = mesh_types[i].second;
        index_t nx = 3;
        index_t ny = 4;
        index_t nz = (dim == "2" ? 0 : 2);

        const std::string mesh_type = mesh_types[i].first;

        Node save_mesh, load_mesh, info;
        blueprint::mesh::examples::basic(mesh_type, nx, ny, nz, save_mesh);

        Node write_opts, read_opts;
        write_opts["file_style"] = "overlink";
        read_opts["matset_style"] = "multi_buffer_full";

        const std::string basename = "silo_save_option_overlink_basic_" + mesh_type + "_" + dim + "D";
        const std::string filename = basename + "/OvlTop.silo";
        const std::string domfile = basename + "/domain0.silo";

        // add a matset to make overlink happy
        int num_elems = (nx - 1) * (ny - 1);
        if (mesh_type == "tets")
        {
            num_elems *= 6;
        }
        add_multi_buffer_full_matset(save_mesh, num_elems, "mesh");

        EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));
        remove_path_if_exists(filename);
        remove_path_if_exists(domfile);
        io::silo::save_mesh(save_mesh, basename, write_opts);
        io::silo::load_mesh(filename, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh,info));

        // make changes to save mesh so the diff will pass
        if (mesh_type == "uniform")
        {
            silo_uniform_to_rect_conversion("coords", "mesh", save_mesh);
        }
        overlink_name_changer(save_mesh);

        // the loaded mesh will be in the multidomain format
        // but the saved mesh is in the single domain format
        EXPECT_EQ(load_mesh.number_of_children(), 1);
        EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());

        EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
// this tests specset i/o
TEST(conduit_relay_io_silo, round_trip_save_option_overlink6)
{
    Node save_mesh, load_mesh, info;
    blueprint::mesh::examples::misc("specsets", 10, 10, 1, save_mesh);
    save_mesh["matsets"].rename_child("mesh", "matset");
    save_mesh["specsets"].rename_child("mesh", "specset");
    save_mesh["specsets"]["specset"]["matset"].set("matset");

    Node write_opts;
    write_opts["file_style"] = "overlink";

    Node read_opts;
    read_opts["matset_style"] = "multi_buffer_full";

    const std::string basename = "silo_save_option_overlink_misc";
    const std::string filename = basename + "/OvlTop.silo";
    EXPECT_EQ(filename, io::blueprint::generate_root_filename(save_mesh, basename, "silo", write_opts));

    remove_path_if_exists(filename);
    io::silo::save_mesh(save_mesh, basename, write_opts);
    io::silo::load_mesh(filename, read_opts, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    // make changes to save mesh so the diff will pass
    vector_field_to_scalars_braid(save_mesh, "2");
    overlink_name_changer(save_mesh);

    // the loaded mesh will be in the multidomain format
    // but the saved mesh is in the single domain format
    EXPECT_EQ(load_mesh.number_of_children(), 1);
    EXPECT_EQ(load_mesh[0].number_of_children(), save_mesh.number_of_children());
    EXPECT_FALSE(load_mesh[0].diff(save_mesh, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------

// 
// read option tests
// 

// read options:
/// opts:
///      mesh_name: "{name}"
///          provide explicit mesh name, for cases where silo data includes
///          more than one mesh.
///          We only allow reading of a single mesh to keep these options on
///          par with the relay io blueprint options.
///
///      matset_style: "default", "multi_buffer_full", "sparse_by_element",
///            "multi_buffer_by_material"
///            "default"   ==> "sparse_by_element"

//-----------------------------------------------------------------------------
// test legacy mesh name option
TEST(conduit_relay_io_silo, round_trip_read_option_mesh_name)
{
    Node load_mesh, info, opts;
    const std::string path = utils::join_file_path("silo", "multi_curv3d.silo");
    const std::string input_file = relay_test_silo_data_path(path);

    opts["mesh_name"] = "mesh1_dup";

    io::silo::load_mesh(input_file, opts, load_mesh);
    EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

    EXPECT_TRUE(load_mesh[0].has_path("topologies/mesh1_dup"));

    EXPECT_TRUE(load_mesh[0]["topologies"].number_of_children() == 1);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_silo, round_trip_read_option_matset_style)
{
    // the matset type and the type we are requesting on read
    const std::vector<std::pair<std::string, std::string>> matset_types = {
        std::make_pair("full", "full"),
        std::make_pair("sparse_by_material", "sparse_by_material"),
        std::make_pair("sparse_by_element", "sparse_by_element"),
        std::make_pair("sparse_by_element", "full"),
        std::make_pair("sparse_by_material", "sparse_by_element"),
        std::make_pair("sparse_by_material", "default"),
    };

    for (int i = 0; i < matset_types.size(); i ++)
    {
        std::string matset_type = matset_types[i].first;
        std::string matset_request = matset_types[i].second;

        for (int j = 0; j < 2; j ++)
        {
            Node mesh_full, mesh_sbe, mesh_sbm, baseline_mesh, load_mesh, info;
            std::string size;
            int nx, ny;
            const double radius = 0.25;
            if (j == 0)
            {
                size = "small";
                nx = ny = 4;
            }
            else
            {
                size = "large";
                nx = ny = 100;
            }

            blueprint::mesh::examples::venn("full", nx, ny, radius, mesh_full);
            blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh_sbm);
            blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh_sbe);

            if (matset_type == "full")
            {
                baseline_mesh.set_external(mesh_full);
            }
            else if (matset_type == "sparse_by_material")
            {
                baseline_mesh.set_external(mesh_sbm);
            }
            else // (matset_type == "sparse_by_element")
            {
                baseline_mesh.set_external(mesh_sbe);
            }

            Node opts;
            if (matset_request == "full")
            {
                opts["matset_style"] = "multi_buffer_full";
            }
            else if (matset_request == "sparse_by_material")
            {
                opts["matset_style"] = "multi_buffer_by_material";
            }
            else if (matset_request == "sparse_by_element")
            {
                opts["matset_style"] = "sparse_by_element";
            }
            else
            {
                opts["matset_style"] = "default";
            }

            const std::string basename = "silo_venn2_" + matset_type + "_" + size;
            const std::string filename = basename + ".root";
            EXPECT_EQ(filename, io::blueprint::generate_root_filename(baseline_mesh, basename, "silo"));

            remove_path_if_exists(filename);
            io::silo::save_mesh(baseline_mesh, basename);
            io::silo::load_mesh(filename, opts, load_mesh);
            EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

            if (matset_request == "full")
            {
                baseline_mesh.set_external(mesh_full);
            }
            else if (matset_request == "sparse_by_material")
            {
                baseline_mesh.set_external(mesh_sbm);
            }
            else if (matset_request == "sparse_by_element")
            {
                baseline_mesh.set_external(mesh_sbe);
            }
            else
            {
                baseline_mesh.set_external(mesh_sbe);
            }

            // make changes to save mesh so the diff will pass

            // The field mat_check has values that are one type and matset_values
            // that are another type. The silo writer converts both to double arrays
            // in this case, so we follow suit.
            Node mat_check_new_values, mat_check_new_matset_values;
            baseline_mesh["fields"]["mat_check"]["values"].to_double_array(mat_check_new_values);
            if (baseline_mesh["fields"]["mat_check"]["matset_values"].dtype().is_object())
            {
                auto mat_vals_itr = baseline_mesh["fields"]["mat_check"]["matset_values"].children();
                while (mat_vals_itr.has_next())
                {
                    Node &mat_vals_for_mat = mat_vals_itr.next();
                    const std::string mat_name = mat_vals_itr.name();
                    mat_vals_for_mat.to_double_array(mat_check_new_matset_values[mat_name]);
                }
            }
            else
            {
                baseline_mesh["fields"]["mat_check"]["matset_values"].to_double_array(mat_check_new_matset_values);
            }
            baseline_mesh["fields"]["mat_check"]["values"].set_external(mat_check_new_values);
            baseline_mesh["fields"]["mat_check"]["matset_values"].set_external(mat_check_new_matset_values);

            silo_name_changer("mesh", baseline_mesh);

            // the loaded mesh will be in the multidomain format
            // but the saved mesh is in the single domain format
            EXPECT_EQ(load_mesh.number_of_children(), 1);
            EXPECT_EQ(load_mesh[0].number_of_children(), baseline_mesh.number_of_children());
            EXPECT_FALSE(load_mesh[0].diff(baseline_mesh, info, CONDUIT_EPSILON, true));
        }
    }
}

//-----------------------------------------------------------------------------

//
// read and write Silo and Overlink tests
//

//-----------------------------------------------------------------------------
// test reading in a handful of different overlink files
TEST(conduit_relay_io_silo, load_mesh_geometry)
{
    const std::vector<std::pair<std::string, std::vector<int>>> file_info = {
        std::make_pair("box2d",                  std::vector<int>{2, 4,    1}),
        std::make_pair("box3d",                  std::vector<int>{3, 8,    1}),
        std::make_pair("diamond",                std::vector<int>{2, 36,   33}),
        std::make_pair("testDisk2D_a",           std::vector<int>{2, 1994, 1920}),
        std::make_pair("donordiv.s2_materials2", std::vector<int>{2, 16,   9}),
        std::make_pair("donordiv.s2_materials3", std::vector<int>{2, 961,  900}),
    };

    for (size_t i = 0; i < file_info.size(); i ++)
    {
        const std::string &basename = file_info[i].first;
        const std::string filename = basename + ".silo";
        const int dim = file_info[i].second[0];
        const int coordset_length = file_info[i].second[1];
        const int topology_length = file_info[i].second[2];

        Node mesh, info;
        const std::string path = utils::join_file_path("overlink", filename);
        const std::string input_file = relay_test_silo_data_path(path);
        io::silo::load_mesh(input_file, mesh);

        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        EXPECT_EQ(blueprint::mesh::number_of_domains(mesh), 1);

        const Node &domain = *blueprint::mesh::domains(mesh).front();
        EXPECT_TRUE(domain.has_child("coordsets"));
        EXPECT_EQ(domain["coordsets"].number_of_children(), 1);
        EXPECT_TRUE(domain.has_child("topologies"));
        EXPECT_EQ(domain["topologies"].number_of_children(), 1);

        { // Coordset Validation //
            const Node &cset = domain["coordsets"].child(0);
            EXPECT_EQ(blueprint::mesh::coordset::dims(cset), dim);
            EXPECT_EQ(blueprint::mesh::coordset::length(cset), coordset_length);
            EXPECT_TRUE(blueprint::mesh::coordset::_explicit::verify(cset, info));
        }

        { // Topology Validation //
            const Node &topo = domain["topologies"].child(0);
            EXPECT_EQ(blueprint::mesh::topology::dims(topo), dim);
            EXPECT_EQ(blueprint::mesh::topology::length(topo), topology_length);
            EXPECT_TRUE(blueprint::mesh::topology::unstructured::verify(topo, info));
        }
    }
}

//-----------------------------------------------------------------------------
// read normal silo files containing multimeshes, multivars, and multimats
TEST(conduit_relay_io_silo, read_silo)
{
    const std::vector<std::vector<std::string>> file_info = {
        // dirname              basename        filext  meshname
        {".",                  "multi_curv3d", ".silo", ""            }, // test default case
        {".",                  "multi_curv3d", ".silo", "mesh1"       },
        // {".",                  "multi_curv3d", ".silo", "mesh1_back"  }, // this multimesh points to paths that do not exist
        {".",                  "multi_curv3d", ".silo", "mesh1_dup"   },
        // {".",                  "multi_curv3d", ".silo", "mesh1_front" }, // same here
        {".",                  "multi_curv3d", ".silo", "mesh1_hidden"},
        {".",                  "tire",         ".silo", ""            }, // test default case
        {".",                  "tire",         ".silo", "tire"        },
        {".",                  "galaxy0000",   ".silo", ""            }, // test default case
        {".",                  "galaxy0000",   ".silo", "StarMesh"    },
        {".",                  "emptydomains", ".silo", ""            }, // test default case
        {".",                  "emptydomains", ".silo", "mesh"        },
        {"multidir_test_data", "multidir0000", ".root", ""            }, // test default case
        {"multidir_test_data", "multidir0000", ".root", "Mesh"        },
        // tests nameschemes
        // TODO understand the file handle issue
        // {".",                  "ucd3d_root",   ".pdb",  ""            }, // test default case
        // {".",                  "ucd3d_root",   ".pdb",  "mesh1"       },
    };

    // TODO what to do in the case where a multimesh points to no data? (mesh1_back)
    // fail silently, as we do now?

    for (int i = 0; i < file_info.size(); i ++) 
    {
        const std::string dirname  = file_info[i][0];
        const std::string basename = file_info[i][1];
        const std::string fileext  = file_info[i][2];
        const std::string meshname = file_info[i][3];

        Node load_mesh, info, read_opts, write_opts;
        std::string filepath = utils::join_file_path(dirname, basename) + fileext;
        filepath = utils::join_file_path("silo", filepath);
        std::string input_file = relay_test_silo_data_path(filepath);

        read_opts["mesh_name"] = meshname;
        io::silo::load_mesh(input_file, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        const std::string out_name = "read_silo_" + basename + 
                                     (meshname.empty() ? "" : "_" + meshname);

        // TODO are these remove paths doing anything? Don't they need filenames?
        remove_path_if_exists(out_name + "_write_blueprint");
        io::blueprint::save_mesh(load_mesh, out_name + "_write_blueprint", "hdf5");

        remove_path_if_exists(out_name + "_write_silo");
        io::silo::save_mesh(load_mesh, out_name + "_write_silo");

        // overlink requires matsets and does not support point meshes
        if (load_mesh[0].has_child("matsets") && basename != "galaxy0000")
        {
            remove_path_if_exists(out_name + "_write_overlink");
            write_opts["file_style"] = "overlink";
            write_opts["ovl_topo_name"] = meshname;
            io::silo::save_mesh(load_mesh, out_name + "_write_overlink", write_opts);
        }
    }
}

//-----------------------------------------------------------------------------
// test that we can read silo without multimeshes, multivars, and multimats
TEST(conduit_relay_io_silo, read_simple_silo)
{
    const std::vector<std::vector<std::string>> file_info = {
        {"curv2d",          ".silo", "no"},
        {"curv2d_colmajor", ".silo", "no"},
        {"curv3d",          ".silo", "yes"},
        {"curv3d_colmajor", ".silo", "no"},
        // {"globe",           ".silo", "yes"}, // TODO need to add support for mixed shape topos
    };
    for (int i = 0; i < file_info.size(); i ++) 
    {
        const std::string basename   = file_info[i][0];
        const std::string fileext    = file_info[i][1];
        const std::string round_trip = file_info[i][2];

        Node load_mesh, info, write_opts;
        std::string filepath = basename + fileext;
        filepath = utils::join_file_path("silo", filepath);
        std::string input_file = relay_test_silo_data_path(filepath);

        io::silo::load_mesh(input_file, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        const std::string out_name = "read_silo_" + basename;

        // TODO are these remove paths doing anything? Don't they need filenames?
        remove_path_if_exists(out_name + "_write_blueprint");
        io::blueprint::save_mesh(load_mesh, out_name + "_write_blueprint", "hdf5");

        if (round_trip == "yes")
        {
            remove_path_if_exists(out_name + "_write_silo");
            io::silo::save_mesh(load_mesh, out_name + "_write_silo");

            // overlink requires matsets
            if (load_mesh[0].has_child("matsets"))
            {
                remove_path_if_exists(out_name + "_write_overlink");
                write_opts["file_style"] = "overlink";
                write_opts["ovl_topo_name"] = "MMESH";
                io::silo::save_mesh(load_mesh, out_name + "_write_overlink", write_opts);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// test that we can read the fake overlink files from the visit test data
TEST(conduit_relay_io_silo, read_fake_overlink)
{
    const std::vector<std::vector<std::string>> file_info = {
     // {"ev_0_0_100",              "OvlTop", ".silo", ""     }, // test default case
     // {"ev_0_0_100",              "OvlTop", ".silo", "MMESH"},
        // TODO uncomment once silo ucdmesh phzones are supported
        {"hl18spec",                "OvlTop", ".silo", ""     }, // test default case
        {"hl18spec",                "OvlTop", ".silo", "MMESH"},
     // {"regrovl_qh_1000_10001_4", "OvlTop", ".silo", ""     }, // test default case
     // {"regrovl_qh_1000_10001_4", "OvlTop", ".silo", "MMESH"},
        // TODO uncomment once silo ucdmesh phzones are supported
        {"utpyr4",                  "OvlTop", ".silo", ""     }, // test default case
        {"utpyr4",                  "OvlTop", ".silo", "MMESH"},
    };

    for (int i = 0; i < file_info.size(); i ++) 
    {
        const std::string dirname  = file_info[i][0];
        const std::string basename = file_info[i][1];
        const std::string fileext  = file_info[i][2];
        const std::string meshname = file_info[i][3];

        Node load_mesh, info, read_opts, write_opts;
        std::string filepath = utils::join_file_path(dirname, basename) + fileext;
        filepath = utils::join_file_path("fake_overlink", filepath);
        std::string input_file = relay_test_silo_data_path(filepath);

        read_opts["mesh_name"] = meshname;
        io::silo::load_mesh(input_file, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        const std::string out_name = "read_fake_overlink_" + dirname + 
                                     (meshname.empty() ? "" : "_" + meshname);

        remove_path_if_exists(out_name + "_write_blueprint");
        io::blueprint::save_mesh(load_mesh, out_name + "_write_blueprint", "hdf5");

        remove_path_if_exists(out_name + "_write_silo");
        io::silo::save_mesh(load_mesh, out_name + "_write_silo");

        remove_path_if_exists(out_name + "_write_overlink");
        write_opts["file_style"] = "overlink";
        write_opts["ovl_topo_name"] = "MMESH";
        io::silo::save_mesh(load_mesh, out_name + "_write_overlink", write_opts);
    }
}

//-----------------------------------------------------------------------------
// read overlink files in symlink format
// should be similar to reading raw silo
TEST(conduit_relay_io_silo, read_overlink_symlink_format)
{
    const std::vector<std::vector<std::string>> file_info = {
        {".", "box2d",                  ".silo", ""     }, // test default case
        {".", "box2d",                  ".silo", "MMESH"},
        {".", "box3d",                  ".silo", ""     }, // test default case
        {".", "box3d",                  ".silo", "MMESH"},
        {".", "c36_m5",                 ".silo", ""     }, // test default case
        {".", "c36_m5",                 ".silo", "MMESH"},
        {".", "cube20b",                ".silo", ""     }, // test default case
        {".", "cube20b",                ".silo", "MMESH"},
        {".", "diamond",                ".silo", ""     }, // test default case
        {".", "diamond",                ".silo", "MMESH"},
        // TODO check diamond filled boundary plot in visit
        // once https://github.com/visit-dav/visit/issues/19522
        // is resolved.
        {".", "donordiv.s2_materials2", ".silo", ""     }, // test default case
        {".", "donordiv.s2_materials2", ".silo", "MMESH"},
        {".", "donordiv.s2_materials3", ".silo", ""     }, // test default case
        {".", "donordiv.s2_materials3", ".silo", "MMESH"},
        {".", "hl18spec",               ".silo", ""     }, // test default case
        {".", "hl18spec",               ".silo", "MMESH"},
        {".", "testDisk2D_a",           ".silo", ""     }, // test default case
        {".", "testDisk2D_a",           ".silo", "MMESH"},
        {".", "tetra8",                 ".silo", ""     }, // test default case
        {".", "tetra8",                 ".silo", "MMESH"},
    };

    for (int i = 0; i < file_info.size(); i ++) 
    {
        const std::string dirname  = file_info[i][0];
        const std::string basename = file_info[i][1];
        const std::string fileext  = file_info[i][2];
        const std::string meshname = file_info[i][3];

        Node load_mesh, info, read_opts, write_opts;
        std::string filepath = utils::join_file_path(dirname, basename) + fileext;
        filepath = utils::join_file_path("overlink", filepath);
        std::string input_file = relay_test_silo_data_path(filepath);

        read_opts["mesh_name"] = meshname;
        io::silo::load_mesh(input_file, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        const std::string out_name = "read_overlink_symlink_" + basename + 
                                     (meshname.empty() ? "" : "_" + meshname);

        remove_path_if_exists(out_name + "_write_blueprint");
        io::blueprint::save_mesh(load_mesh, out_name + "_write_blueprint", "hdf5");

        remove_path_if_exists(out_name + "_write_silo");
        io::silo::save_mesh(load_mesh, out_name + "_write_silo");

        remove_path_if_exists(out_name + "_write_overlink");
        write_opts["file_style"] = "overlink";
        write_opts["ovl_topo_name"] = "MMESH";
        io::silo::save_mesh(load_mesh, out_name + "_write_overlink", write_opts);
    }
}

//-----------------------------------------------------------------------------
// read overlink directly from ovltop.silo
// this case is tricky and involves messing with paths
TEST(conduit_relay_io_silo, read_overlink_directly)
{
    const std::vector<std::vector<std::string>> file_info = {
        {"box2d",                  "OvlTop", ".silo", ""     }, // test default case
        {"box2d",                  "OvlTop", ".silo", "MMESH"},
        {"box3d",                  "OvlTop", ".silo", ""     }, // test default case
        {"box3d",                  "OvlTop", ".silo", "MMESH"},
        {"c36_m5",                 "OvlTop", ".silo", ""     }, // test default case
        {"c36_m5",                 "OvlTop", ".silo", "MMESH"},
        // TODO test cube20b species in VisIt.
        {"cube20b",                "OvlTop", ".silo", ""     }, // test default case
        {"cube20b",                "OvlTop", ".silo", "MMESH"},
        {"diamond",                "OvlTop", ".silo", ""     }, // test default case
        {"diamond",                "OvlTop", ".silo", "MMESH"},
        {"donordiv.s2_materials2", "OvlTop", ".silo", ""     }, // test default case
        {"donordiv.s2_materials2", "OvlTop", ".silo", "MMESH"},
        {"donordiv.s2_materials3", "OvlTop", ".silo", ""     }, // test default case
        {"donordiv.s2_materials3", "OvlTop", ".silo", "MMESH"},
        // TODO test hl18spec species in VisIt.
        {"hl18spec",               "OvlTop", ".silo", ""     }, // test default case
        {"hl18spec",               "OvlTop", ".silo", "MMESH"},
        {"testDisk2D_a",           "OvlTop", ".silo", ""     }, // test default case
        {"testDisk2D_a",           "OvlTop", ".silo", "MMESH"},
        {"tetra8",                 "OvlTop", ".silo", ""     }, // test default case
        {"tetra8",                 "OvlTop", ".silo", "MMESH"},
    };

    for (int i = 0; i < file_info.size(); i ++) 
    {
        const std::string dirname  = file_info[i][0];
        const std::string basename = file_info[i][1];
        const std::string fileext  = file_info[i][2];
        const std::string meshname = file_info[i][3];

        Node load_mesh, info, read_opts, write_opts;

        std::string filepath = utils::join_file_path(dirname, basename) + fileext;
        filepath = utils::join_file_path("overlink", filepath);
        std::string input_file = relay_test_silo_data_path(filepath);

        read_opts["mesh_name"] = meshname;
        io::silo::load_mesh(input_file, read_opts, load_mesh);
        EXPECT_TRUE(blueprint::mesh::verify(load_mesh, info));

        const std::string out_name = "read_overlink_direct_" + dirname + 
                                     (meshname.empty() ? "" : "_" + meshname);

        remove_path_if_exists(out_name + "_write_blueprint");
        io::blueprint::save_mesh(load_mesh, out_name + "_write_blueprint", "hdf5");

        remove_path_if_exists(out_name + "_write_silo");
        io::silo::save_mesh(load_mesh, out_name + "_write_silo");

        remove_path_if_exists(out_name + "_write_overlink");
        write_opts["file_style"] = "overlink";
        write_opts["ovl_topo_name"] = "MMESH";
        io::silo::save_mesh(load_mesh, out_name + "_write_overlink", write_opts);
    }
}

// TODO add tests for polytopal meshes once they are supported

// TODO somewhere I need to error on overlink when there are different var or mesh types across domains

// TODO exception tests? (EXPECT_THROW)
