// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_handle_sidre.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"
#include <iostream>
#include <vector>
#include "gtest/gtest.h"

#include "t_config.hpp"


using namespace conduit;
using namespace conduit::relay;

std::string
relay_test_data_path(const std::string &test_fname)
{
    std::string res = utils::join_file_path(CONDUIT_T_SRC_DIR,"relay");
    res = utils::join_file_path(res,"data");
    return utils::join_file_path(res,test_fname);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_sidre_basic)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping spiral_multi_file test");
        return;
    }

    std::string tbase = "texample_sidre_basic_ds_demo.";
    std::vector<std::string> tprotos;
    // sidre hdf5 is the only currently supported protocol
    tprotos.push_back("sidre_hdf5");

    //
    // create an equiv conduit tree for testing
    //

    conduit::int64    conduit_vals_1[5] = {0,1,2,3,4};
    conduit::float64  conduit_vals_2[6] = { 1.0, 2.0,
                                            1.0, 2.0,
                                            1.0, 2.0,};
    // Note: use a std::vector for the empty array as workaround for
    // MSVC error C2466 about allocating arrays of constant size 0
    std::vector<conduit::int64> conduit_vals_0;

    Node n;
    n["my_scalars/i64"].set_int64(1);
    n["my_scalars/f64"].set_float64(10.0);
    n["my_strings/s0"] = "s0 string";
    n["my_strings/s1"] = "s1 string";
    n["my_arrays/a0_i64"].set(conduit_vals_0.data(),0);
    n["my_arrays/a5_i64"].set(conduit_vals_1,5);
    n["my_arrays/a5_i64_ext"].set_external(conduit_vals_1,5);
    n["my_arrays/b_v0"].set(conduit_vals_2,0);
    n["my_arrays/b_v1"].set(conduit_vals_2,
                            3,
                            0,
                            2 * sizeof(conduit::float64));
    n["my_arrays/b_v2"].set(conduit_vals_2,
                            3,
                            sizeof(conduit::float64),
                            2 * sizeof(conduit::float64));

    // change val to make sure this is reflected as external
    conduit_vals_1[4] = -5;
    CONDUIT_INFO("Conduit Test Tree:");
    n.print();


    for(size_t i =0; i < tprotos.size(); i++)
    {
        io::IOHandle h;
        std::string protocol = tprotos[i];
        h.open(relay_test_data_path(tbase + protocol),
               protocol);
        // check expected child naes
        std::vector<std::string> rchld;
        h.list_child_names(rchld);

        // print names
        for(int i=0;i< rchld.size();i++)
        {
            std::cout << rchld[i] << std::endl;
        }

        // check names are as we expect
        EXPECT_EQ(rchld.size(),3);
        if(rchld.size() == 3)
        {
            EXPECT_EQ(rchld[0],"my_scalars");
            EXPECT_EQ(rchld[1],"my_strings");
            EXPECT_EQ(rchld[2],"my_arrays");
        }

        // check read of every case of leaf


        Node n_leaf;
        h.read("my_scalars/i64",n_leaf); n_leaf.print();
        h.read("my_scalars/f64",n_leaf); n_leaf.print();
        h.read("my_strings/s0",n_leaf); n_leaf.print();
        h.read("my_strings/s1",n_leaf); n_leaf.print();
        h.read("my_arrays/a0_i64",n_leaf); n_leaf.print();
        h.read("my_arrays/a5_i64",n_leaf); n_leaf.print();
        h.read("my_arrays/a5_i64_ext",n_leaf); n_leaf.print();
        h.read("my_arrays/b_v0",n_leaf); n_leaf.print();
        h.read("my_arrays/b_v1",n_leaf); n_leaf.print();
        h.read("my_arrays/b_v2",n_leaf); n_leaf.print();

        CONDUIT_INFO("Full Read Test:");

        Node n_read, n_info;

        h.read(n_read);
        n_read.print();
        EXPECT_FALSE(n.diff(n_read,n_info));

        // check subpath read.
        n_read.reset();

        h.read("my_arrays",n_read);
        // n_read.print();
        EXPECT_FALSE(n["my_arrays"].diff(n_read,n_info));

        // flush is a no op for read only sidre handles
        // but still want to test it in case folks call it generically
        h.flush();

        h.close();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_sidre_with_root)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping spiral_multi_file test");
        return;
    }

    io::IOHandle h;
    h.open(relay_test_data_path("out_spio_blueprint_example.root"),
           "sidre_hdf5");

    std::vector<std::string> rchld;
    h.list_child_names(rchld);
    EXPECT_TRUE(rchld.size() > 0 );

    for(int i=0;i< rchld.size();i++)
    {
        std::cout << rchld[i] << std::endl;
    }

    Node n, n_info;
    h.read("root",n);
    EXPECT_EQ(n["number_of_trees"].to_int64(),4);
    EXPECT_EQ(n["number_of_files"].to_int64(),4);

    n.reset();
    h.read("root/blueprint_index",n);
    // make sure the mesh bp index is valid
    EXPECT_TRUE(conduit::blueprint::mesh::index::verify(n["mesh"],n_info));
    n.print();

    n.reset();
    h.read("root/number_of_trees",n);
    EXPECT_EQ(n.to_int64(),4);

    n.reset();
    h.read("root/protocol",n);
    n.print();

    EXPECT_TRUE(h.has_path("root"));
    EXPECT_TRUE(h.has_path("root/blueprint_index"));
    EXPECT_TRUE(h.has_path("0"));
    EXPECT_TRUE(h.has_path("0/mesh"));

    // bad path checks
    EXPECT_FALSE(h.has_path("loot"));
    EXPECT_FALSE(h.has_path("loot/blueprint_index"));
    EXPECT_FALSE(h.has_path("-1000"));
    EXPECT_FALSE(h.has_path("1000"));
    EXPECT_FALSE(h.has_path("0/nesh"));

    // list child_names test
    std::vector<std::string> tchld;
    h.list_child_names("root",tchld);
    EXPECT_TRUE(tchld.size() > 0 );

    // children found
    std::cout << "root children " << std::endl;
    for(int i=0;i< tchld.size();i++)
    {
        std::cout << tchld[i] << std::endl;
    }

    // TODO FIX THIS
    h.list_child_names("0/mesh",tchld);
    EXPECT_TRUE(tchld.size() > 0 );

    // children found
    std::cout << "0/mesh children "  << std::endl;
    for(int i=0;i< tchld.size();i++)
    {
        std::cout << tchld[i] << std::endl;
    }

    h.list_child_names("loot",tchld);
    EXPECT_TRUE(tchld.size() == 0 );
    h.list_child_names("loot/blueprint_index",tchld);
    EXPECT_TRUE(tchld.size() == 0 );
    h.list_child_names("-1000",tchld);
    EXPECT_TRUE(tchld.size() == 0 );
    h.list_child_names("1000",tchld);
    EXPECT_TRUE(tchld.size() == 0 );
    h.list_child_names("0/nesh",tchld);
    EXPECT_TRUE(tchld.size() == 0 );

    // check data for each domain
    for(int i=0;i<4;i++)
    {
        std::ostringstream oss;
        oss << i;
        std::string tree_root = oss.str();

        CONDUIT_INFO("Reading tree: " << tree_root);

        // read the entire tree
        n.reset();
        h.read(tree_root,n);

        // read the entire mesh and make sure mesh bp verify is true
        n.reset();
        h.read(tree_root + "/mesh",n);
        EXPECT_TRUE(conduit::blueprint::mesh::verify(n, n_info));

        n.reset();
        h.read(tree_root + "/mesh/fields/rank",n);
        n.print();

        // we expect the "rank" field to be filled with
        // values that equal the domain id
        int64_array vals = n["values"].value();
        EXPECT_EQ(vals[0],i);
    }

    h.close();
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_sidre_bad_reads)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping spiral_multi_file test");
        return;
    }

    io::IOHandle h;
    h.open(relay_test_data_path("out_spio_blueprint_example.root"),
           "sidre_hdf5");

    Node n;
    EXPECT_THROW(h.read("GARBAGE!",n),conduit::Error);
    EXPECT_THROW(h.read("-1",n),conduit::Error); // neg is invalid
    EXPECT_THROW(h.read("100000",n),conduit::Error); // to big

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_handle, test_sidre_read_mesh_bp)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping spiral_multi_file test");
        return;
    }

    Node mesh;
    relay::io::blueprint::read_mesh(
                        relay_test_data_path("out_spio_blueprint_example.root"),
                        mesh);
    mesh.print();
    Node opts;
    opts["file_style"] = "root_only";
    opts["suffix"] = "none";
    opts["number_of_files"] = 1;

    std::string tout = "tout_sidre_mesh_then_save";
    utils::remove_path_if_exists(tout);
    relay::io::blueprint::write_mesh(mesh, tout,"hdf5",opts);

    Node n_read;
    relay::io::blueprint::read_mesh(tout + ".root",n_read);

}




