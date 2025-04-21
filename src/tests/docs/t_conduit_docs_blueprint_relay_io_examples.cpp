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

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;

//-----------------------------------------------------------------------------
bool
check_if_hdf5_and_silo_are_enabled()
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool res = io_protos["io/protocols/hdf5"].as_string() == "enabled";
    res = res && io_protos["io/protocols/silo"].as_string() == "enabled";
    return res;
}

//-----------------------------------------------------------------------------
TEST(conduit_docs, blueprint_relay_io_example_multi_domain_spiral)
{
    // these examples require hdf5 and silo
    if(!check_if_hdf5_and_silo_are_enabled())
    {
        CONDUIT_INFO("Examples requires Relay with HDF5 and Silo Support");
        return;
    }

    BEGIN_EXAMPLE("blueprint_relay_io_example_multi_domain_spiral");

    // node to hold mesh data and relay options
    conduit::Node mesh, opts;

    //create an multi-domain example mesh
    blueprint::mesh::examples::spiral(7, mesh);
    mesh.print();

    // save to blueprint yaml
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_spiral_bp_yml","yaml");
    // save to blueprint hdf5
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_spiral_bp_hdf5","hdf5");
    // save to silo
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_spiral_bp_silo","silo");

    END_EXAMPLE("blueprint_relay_io_example_multi_domain_spiral");

}

//-----------------------------------------------------------------------------
TEST(conduit_docs, blueprint_relay_io_example_venn)
{
    // these examples require hdf5 and silo
    if(!check_if_hdf5_and_silo_are_enabled())
    {
        CONDUIT_INFO("Examples requires Relay with HDF5 and Silo Support");
        return;
    }

    BEGIN_EXAMPLE("blueprint_relay_io_example_venn");

    // node to hold mesh data and relay options
    conduit::Node mesh, opts;
    //create an example mesh with materials
    blueprint::mesh::examples::venn("sparse_by_element", 100, 100, .25, mesh);
    mesh.print();

    // save to blueprint yaml
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_bp_yml","yaml");
    // save to blueprint hdf5
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_bp_hdf5","hdf5");
    // save to silo
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_bp_silo","silo");
    // save to overlink flavored silo (note: overlink requires materials)
    opts["file_style"] = "overlink";
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_overlink","silo",opts);

    END_EXAMPLE("blueprint_relay_io_example_venn");
}


//-----------------------------------------------------------------------------
TEST(conduit_docs, blueprint_relay_io_example_venn_convert)
{
    // these examples require hdf5 and silo
    if(!check_if_hdf5_and_silo_are_enabled())
    {
        CONDUIT_INFO("Examples requires Relay with HDF5 and Silo Support");
        return;
    }

    BEGIN_EXAMPLE("blueprint_relay_io_example_venn_convert");

    // node to hold mesh data and relay options
    conduit::Node mesh, opts;
    //create an example mesh with materials
    blueprint::mesh::examples::venn("sparse_by_element", 100, 100, .25, mesh);
    // save to blueprint hdf5
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_bp_hdf5_convert","hdf5");

    // reset mesh in-memory to start over
    mesh.reset();
    // load mesh data from blueprint hdf5
    conduit::relay::io::blueprint::load_mesh("tout_venn_bp_hdf5_src.root",mesh);
    mesh.print();
    // save to overlink flavored silo (note: overlink requires materials)
    opts["file_style"] = "overlink";
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_overlink_convert","silo",opts);

    END_EXAMPLE("blueprint_relay_io_example_venn_convert");
}

//-----------------------------------------------------------------------------
TEST(conduit_docs, blueprint_relay_io_example_venn_overlink_read)
{
    // these examples require hdf5 and silo
    if(!check_if_hdf5_and_silo_are_enabled())
    {
        CONDUIT_INFO("Examples requires Relay with HDF5 and Silo Support");
        return;
    }

    BEGIN_EXAMPLE("blueprint_relay_io_example_venn_ol_read");

    // node to hold mesh data and relay options
    conduit::Node mesh, opts;
    //create an example mesh with materials
    blueprint::mesh::examples::venn("sparse_by_element", 100, 100, .25, mesh);
    // save to overlink flavored silo (note: overlink requires materials)
    opts["file_style"] = "overlink";
    conduit::relay::io::blueprint::save_mesh(mesh,"tout_venn_overlink_read","silo",opts);

    // reset mesh in-memory to start over
    mesh.reset();
    // load mesh data from blueprint hdf5
    std::string root_path = conduit::utils::join_path("tout_venn_overlink_read","OvlTop.silo");
    conduit::relay::io::blueprint::load_mesh(root_path,mesh);
    mesh.print();

    END_EXAMPLE("blueprint_relay_io_example_venn_ol_read");
}

