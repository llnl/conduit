// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_docs_blueprint_examples.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_relay.hpp"
#include "t_conduit_docs_tutorial_helpers.hpp"
#include "t_config.hpp"

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;

std::string
relay_test_data_path(const std::string &test_fname)
{
    std::string res = utils::join_file_path(CONDUIT_T_SRC_DIR,"relay");
    res = utils::join_file_path(res,"data");
    return utils::join_file_path(res,test_fname);
}

//-----------------------------------------------------------------------------
// 65-114
TEST(conduit_docs, relay_io_handle_1)
{
    BEGIN_EXAMPLE("relay_io_handle_example_1");
    // setup node with example data to save
    Node n;
    n["a/data"]   = 1.0;
    n["a/more_data"] = 2.0;
    n["a/b/my_string"] = "value";
    std::cout << "\nNode to write:" << std::endl;
    n.print();

    // save to hdf5 file using the path-based api
    conduit::relay::io::save(n,"my_output.hdf5");

    // inspect and modify with an IOHandle
    conduit::relay::io::IOHandle h;
    h.open("my_output.hdf5");

    // check for and read a path we are interested in
    if( h.has_path("a/data") )
    {
        Node nread;
        h.read("a/data",nread);
        std::cout << "\nValue at \"a/data\" = "
                  << nread.to_float64()
                  << std::endl;
    }

    // check for and remove a path we don't want
    if( h.has_path("a/more_data") )
    {
        h.remove("a/more_data");
        std::cout << "\nRemoved \"a/more_data\""
                  << std::endl;
    }

    // verify the data was removed
    if( !h.has_path("a/more_data") )
    {
        std::cout << "\nPath \"a/more_data\" is no more"
                  << std::endl;
    }

    std::cout << "\nWriting to \"a/c\""
              << std::endl;
    // write some new data
    n = 42.0;
    h.write(n,"a/c");

    // find the names of the children of "a"
    std::vector<std::string> cld_names;
    h.list_child_names("a",cld_names);

    // print the names
    std::cout << "\nChildren of \"a\": ";
    std::vector<std::string>::const_iterator itr;
    for (itr = cld_names.begin();
         itr < cld_names.end();
         ++itr)
    {
        std::cout << "\"" << *itr << "\" ";
    }

    std::cout << std::endl;

    Node nread;
    // read the entire contents
    h.read(nread);

    std::cout << "\nRead Result:" << std::endl;
    nread.print();
    END_EXAMPLE("relay_io_handle_example_1");
}

//-----------------------------------------------------------------------------
// 65-114
TEST(conduit_docs, relay_io_handle_sidre_1)
{
    // this example requires hdf5 support
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping relay_io_handle_sidre_1");
        return;
    }

    BEGIN_EXAMPLE("relay_io_handle_example_sidre_1");
    // this example reads a sample hdf5 sidre style file

    std::string input_fname = relay_test_data_path(
                                    "texample_sidre_basic_ds_demo.sidre_hdf5");

    // open our sidre file for read with an IOHandle
    conduit::relay::io::IOHandle h;
    h.open(input_fname,"sidre_hdf5");

    // find the names of the children at the root
    std::vector<std::string> cld_names;
    h.list_child_names(cld_names);

    // print the names
    std::cout << "\nChildren at root: ";
    std::vector<std::string>::const_iterator itr;
    for (itr = cld_names.begin();
         itr < cld_names.end();
         ++itr)
    {
        std::cout << "\"" << *itr << "\" ";
    }

    Node nread;
    // read the entire contents
    h.read(nread);

    std::cout << "\nRead Result:" << std::endl;
    nread.print();
    END_EXAMPLE("relay_io_handle_example_sidre_1");
}

//-----------------------------------------------------------------------------
// 65-114
TEST(conduit_docs, relay_io_handle_sidre_2)
{
    // this example requires hdf5 support
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    if(!hdf5_enabled)
    {
        CONDUIT_INFO("HDF5 disabled, skipping relay_io_handle_example_sidre_2");
        return;
    }


    BEGIN_EXAMPLE("relay_io_handle_example_sidre_2");
    // this example reads a sample hdf5 sidre datastore, grouped by a root file
    std::string input_fname = relay_test_data_path(
                                    "out_spio_blueprint_example.root");

    // read using the root file
    conduit::relay::io::IOHandle h;
    h.open(input_fname,"sidre_hdf5");

    // find the names of the children at the root
    std::vector<std::string> cld_names;
    h.list_child_names(cld_names);

    // the "root" (/) of the Sidre-based IOHandle to the datastore provides
    // access to the root file itself, and all of the data groups

    // print the names
    std::cout << "\nChildren at root: ";
    std::vector<std::string>::const_iterator itr;
    for (itr = cld_names.begin();
         itr < cld_names.end();
         ++itr)
    {
        std::cout << "\"" << *itr << "\" ";
    }

    Node nroot;
    // read the entire root file contents
    h.read("root",nroot);

    std::cout << "\nRead \"root\" Result:" << std::endl;
    nroot.print();

    Node nread;
    // read all of data group 0
    h.read("0",nread);

    std::cout << "\nRead \"0\" Result:" << std::endl;
    nread.print();

    // reset, or trees will blend in this case
    nread.reset();

    // read a subpath of data group 1
    h.read("1/mesh",nread);

    std::cout << "\nRead \"1/mesh\" Result:" << std::endl;
    nread.print();

    END_EXAMPLE("relay_io_handle_example_sidre_2");
}




