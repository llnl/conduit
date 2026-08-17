// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_examples_generate.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_relay.hpp"
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;

//-----------------------------------------------------------------------------
bool
check_if_hdf5_enabled()
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    return io_protos["io/protocols/hdf5"].as_string() == "enabled";
}

//-----------------------------------------------------------------------------
void
test_save_mesh_helper(const conduit::Node &dsets,
                      const std::string &base_name)
{
    Node opts;
    opts["file_style"] = "root_only";
    opts["suffix"] = "none";


    std::string ofile_yaml = base_name + "_yaml.root";
    conduit::utils::remove_path_if_exists(ofile_yaml);

    relay::io::blueprint::save_mesh(dsets, base_name + "_yaml", "yaml", opts);
    EXPECT_TRUE(conduit::utils::is_file(ofile_yaml));

    if(check_if_hdf5_enabled())
    {
        std::string ofile_hdf5 = base_name + "_hdf5.root";
        conduit::utils::remove_path_if_exists(ofile_hdf5);
        relay::io::blueprint::save_mesh(dsets, base_name + "_hdf5", "hdf5", opts);
        EXPECT_TRUE(conduit::utils::is_file(ofile_hdf5));
    }
}



//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples_generate, gen_driver_all_examples)
{
    // exercise them all
    std::string obase = "tout_mesh_bp_ex_gen_driver_";

    std::vector<std::string> example_names = {"braid",
                                              "basic",
                                              "strided_structured",
                                              "grid",
                                              "spiral",
                                              "polytess",
                                              "polychain",
                                              "misc",
                                              "adjset_uniform",
                                              "gyre",
                                              "julia",
                                              "julia_nestsets_simple",
                                              "julia_nestsets_complex",
                                              "polystar",
                                              "related_boundary",
                                              "rz_cylinder",
                                              "tiled",
                                              "venn",
                                              "bent_multi_grid",
                                              "bent_multi_grid_amr" };

    for(const std::string &example_name : example_names)
    {
        Node res;
        conduit::blueprint::mesh::examples::generate(example_name,res);
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "[" << example_name << "]" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        res.print();
        test_save_mesh_helper(res,obase + example_name);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples_generate, generate_venn_choices)
{
    Node res, opts;

    // test default settings
    conduit::blueprint::mesh::examples::generate("venn", res);
    EXPECT_FALSE(res.has_path("matsets/matset/material_map"));

    opts["nx"]     = 100;
    opts["ny"]     = 100;
    opts["radius"] = 0.25;

    // test full
    opts["matset_type"]           = "full";
    opts["generate_material_map"] = "yes";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("matsets/matset/material_map"));

    opts["generate_material_map"] = "no";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_FALSE(res.has_path("matsets/matset/material_map"));

    opts["generate_material_map"] = "default";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_FALSE(res.has_path("matsets/matset/material_map"));

    // test sparse_by_element
    opts["matset_type"]           = "sparse_by_element";
    opts["generate_material_map"] = "yes";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("matsets/matset/material_map"));

    opts["generate_material_map"] = "no";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_THROW(res.has_path("matsets/matset/material_map"), conduit::Error);

    opts["generate_material_map"] = "default";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("matsets/matset/material_map"));

    // test sparse_by_material
    opts["matset_type"]           = "sparse_by_material";
    opts["generate_material_map"] = "yes";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("matsets/matset/material_map"));

    opts["generate_material_map"] = "no";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_FALSE(res.has_path("matsets/matset/material_map"));

    opts["generate_material_map"] = "default";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_FALSE(res.has_path("matsets/matset/material_map"));

    opts["radius"] = 5; // large radius forces no background which
    // forces material map in default case
    opts["generate_material_map"] = "default";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("matsets/matset/material_map"));

    // generate specsets
    conduit::blueprint::mesh::examples::generate("venn", res);
    EXPECT_FALSE(res.has_path("specsets"));
    
    opts.reset();
    opts["matset_type"] = "full";
    opts["nx"]     = 100;
    opts["ny"]     = 100;
    opts["radius"] = 0.25;
    opts["generate_specset"] = "yes";
    conduit::blueprint::mesh::examples::generate("venn", opts, res);
    EXPECT_TRUE(res.has_path("specsets"));
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples_generate, gen_driver_bad_inputs)
{
    Node res;
    // unknown example name
    EXPECT_THROW(conduit::blueprint::mesh::examples::generate("banana",res),
                 conduit::Error);
    // bad options
    Node opts;
    opts["banana"]= 42;
    EXPECT_THROW(conduit::blueprint::mesh::examples::generate("braid",opts,res);,
                 conduit::Error);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples_generate, all_options)
{
    Node opts;
    conduit::blueprint::mesh::examples::generate_default_options(opts);
    opts.print();
}

