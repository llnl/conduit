// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_examples.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"
#include "conduit_fmt/conduit_fmt.h"
#if defined (CONDUIT_RELAY_IO_SILO_ENABLED)
#include "conduit_relay_io_silo.hpp"
#endif

#include <math.h>
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;

index_t OUTPUT_NUM_AXIS_POINTS = 5;

std::string PROTOCOL_VER = CONDUIT_VERSION;

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

    relay::io::blueprint::save_mesh(dsets, base_name + "_yaml", "yaml", opts);

    if(check_if_hdf5_enabled())
    {
        relay::io::blueprint::save_mesh(dsets, base_name + "_hdf5", "hdf5", opts);
    }
}


//-----------------------------------------------------------------------------
void
braid_save_helper(const conduit::Node &dsets,
                  const std::string &base_name)
{
    Node root, info;
    root["data"].set_external(dsets);

    Node bpindex;
    NodeConstIterator itr = dsets.children();
    while(itr.has_next())
    {
        const Node &mesh = itr.next();
        const std::string mesh_name = itr.name();

        // NOTE: Skip all domains containing one or more mixed-shape topologies
        // because this type of mesh isn't fully supported yet.
        bool is_domain_index_valid = true;
        NodeConstIterator topo_iter = mesh["topologies"].children();
        while(topo_iter.has_next())
        {
            const Node &topo = topo_iter.next();
            is_domain_index_valid &= (
                !::conduit::blueprint::mesh::topology::unstructured::verify(topo, info) ||
                !topo["elements"].has_child("element_types"));
        }

        if(is_domain_index_valid)
        {
            ::conduit::blueprint::mesh::generate_index(mesh,
                                                       mesh_name,
                                                       1,
                                                       bpindex[mesh_name]);
        }
    }

    std::string ofile = base_name + "_yaml.root";
    root["blueprint_index"] = bpindex;
    root["protocol/name"].set("yaml");
    root["protocol/version"].set(CONDUIT_VERSION);

    root["number_of_files"].set(1);
    root["number_of_trees"].set(1);
    root["file_pattern"].set(ofile);
    root["tree_pattern"].set("/data");

    relay::io::save(root,ofile,"yaml");

    if(check_if_hdf5_enabled())
    {
        ofile = base_name + "_hdf5.root";
        root["protocol/name"].set("hdf5");
        root["file_pattern"].set(ofile);
        relay::io::save(root,ofile,"hdf5");
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_2d)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);

    bool silo_enabled = io_protos["io/protocols/conduit_silo"].as_string() == "enabled";
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // we are using one node to hold group of example meshes purely out of convenience
    Node dsets;
    // can be overridden via command line
    index_t npts_x = OUTPUT_NUM_AXIS_POINTS;
    index_t npts_y = OUTPUT_NUM_AXIS_POINTS;
    index_t npts_z = 0; // 2D examples ...

    blueprint::mesh::examples::braid("uniform",
                                      npts_x,
                                      npts_y,
                                      npts_z,
                                      dsets["uniform"]);

    blueprint::mesh::examples::braid("rectilinear",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["rect"]);

    blueprint::mesh::examples::braid("structured",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["struct"]);

    blueprint::mesh::examples::braid("lines",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["lines"]);

    blueprint::mesh::examples::braid("tris",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["tris"]);

    blueprint::mesh::examples::braid("quads",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["quads"]);

    blueprint::mesh::examples::braid("points",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["points"]);

    blueprint::mesh::examples::braid("points_implicit",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["points_implicit"]);

    blueprint::mesh::examples::braid("mixed_2d",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["mixed_2d"]);

    Node info;
    NodeConstIterator itr = dsets.children();
    while(itr.has_next())
    {
        const Node &mesh = itr.next();
        EXPECT_TRUE(blueprint::mesh::verify(mesh,info));
        CONDUIT_INFO(info.to_yaml());
    }

    braid_save_helper(dsets,"braid_2d_examples");
#if defined (CONDUIT_RELAY_IO_SILO_ENABLED)
    if(silo_enabled)
    {
        // we removed datasets above, so we need an updated iterator
        itr = dsets.children();
        while(itr.has_next())
        {
            const Node &mesh = itr.next();
            std::string name = itr.name();

            // Skip output of silo mesh for mixed mesh of tris and quads for now.
            // The silo output is not yet defined and it throws an exception
            // in conduit_silo.cpp::silo_write_ucd_zonelist()
            // in the following line that is looking for the 'shape' node:
            //   std::string topo_shape = shape_block->fetch("shape").as_string();
            // which does not exist for indexed_stream meshes.
            // The silo writer needs to be updated for this case.
            if(name == "mixed_2d")
            {
                CONDUIT_INFO("\tNOTE: skipping output to SILO -- ")
                CONDUIT_INFO("feature is unavailable for mixed element meshes")
                continue;
            }

            relay::io::silo::save_mesh(mesh, "braid_2d_" + name +  "_example");
        }
    }
#endif
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_3d)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);

    bool silo_enabled = io_protos["io/protocols/conduit_silo"].as_string() == "enabled";
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // we are using one node to hold group of example meshes purely out of convenience
    Node dsets;
    // can be overridden via command line
    index_t npts_x = OUTPUT_NUM_AXIS_POINTS;
    index_t npts_y = OUTPUT_NUM_AXIS_POINTS;
    index_t npts_z = OUTPUT_NUM_AXIS_POINTS; // 3D examples ...

    blueprint::mesh::examples::braid("uniform",
                                      npts_x,
                                      npts_y,
                                      npts_z,
                                      dsets["uniform"]);

    blueprint::mesh::examples::braid("rectilinear",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["rect"]);

    blueprint::mesh::examples::braid("structured",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["struct"]);

    blueprint::mesh::examples::braid("points",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["points"]);

    blueprint::mesh::examples::braid("points_implicit",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["points_implicit"]);

    blueprint::mesh::examples::braid("lines",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["lines"]);

    blueprint::mesh::examples::braid("tets",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["tets"]);

    blueprint::mesh::examples::braid("hexs",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["hexs"]);

    blueprint::mesh::examples::braid("mixed",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["mixed"]);

    blueprint::mesh::examples::braid("wedges",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["wedges"]);

    blueprint::mesh::examples::braid("pyramids",
                                     npts_x,
                                     npts_y,
                                     npts_z,
                                     dsets["pyramids"]);

    Node info;
    NodeConstIterator itr = dsets.children();
    while(itr.has_next())
    {
        const Node &mesh = itr.next();
        EXPECT_TRUE(blueprint::mesh::verify(mesh,info));
        CONDUIT_INFO(info.to_yaml());
    }

    braid_save_helper(dsets,"braid_3d_examples");
#if defined (CONDUIT_RELAY_IO_SILO_ENABLED)
    if(silo_enabled)
    {
        // we removed datasets above, so we need an updated iterator
        itr = dsets.children();
        while(itr.has_next())
        {
            const Node &mesh = itr.next();
            std::string name = itr.name();

            // Skip output of silo mesh for mixed mesh for now.
            // The silo output is not yet defined and it throws an exception
            // in conduit_silo.cpp::silo_write_ucd_zonelist()
            // in the following line that is looking for the 'shape' node:
            //              std::string topo_shape = shape_block->fetch("shape").as_string();
            // which does not exist for indexed_stream meshes.
            // The silo writer needs to be updated for this case.
            if (name == "mixed")
            {
                CONDUIT_INFO("\tNOTE: skipping output to SILO -- ")
                CONDUIT_INFO("feature is unavailable for mixed element meshes")
                continue;
            }

            relay::io::silo::save_mesh(mesh, "braid_3d_" + name +  "_example");
        }
    }
#endif
}


//-----------------------------------------------------------------------------
void add_domain_node(Node& spec, const char* dom_name, int domain_id,
    int npts_x, int npts_y, int npts_z,
    Node& x, Node& y, Node& z)
{
    Node& dom = spec[dom_name];
    dom["npts_x"] = npts_x;
    dom["npts_y"] = npts_y;
    if (npts_z > 0)
    {
        dom["npts_z"] = npts_z;
    }
    dom["corner_xs"].set(x);
    dom["corner_ys"].set(y);
    if (z.dtype().number_of_elements() > 0)
    {
        dom["corner_zs"].set(z);
    }
    dom["domain_id"] = domain_id;
}

TEST(conduit_blueprint_mesh_examples, mesh_2D_enh_red_connectivity)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);

    bool silo_enabled = io_protos["io/protocols/conduit_silo"].as_string() == "enabled";
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // we are using one node to hold group of example meshes purely out of convenience
    Node dsets;
    Node empty_z;

    {
        Node spec;

        Node d0x; d0x = { 0., 4., 4., 0. };
        Node d0y; d0y = { 0., 0., 4., 4. };
        add_domain_node(spec, "domain0", 0, 4, 5, 0, d0x, d0y, empty_z);

        Node d1x; d1x = { 4., 6., 6., 4. };
        Node d1y; d1y = { 0., 0., 7., 4. };
        add_domain_node(spec, "domain1", 1, 4, 5, 0, d1x, d1y, empty_z);

        Node d2x; d2x = { 0., 4., 6., 0. };
        Node d2y; d2y = { 4., 4., 7., 7. };
        add_domain_node(spec, "domain2", 2, 4, 4, 0, d2x, d2y, empty_z);

        Node d3x; d3x = { 0., 6., 6., 0. };
        Node d3y; d3y = { 7., 7., 10., 10. };
        add_domain_node(spec, "domain3", 3, 4, 5, 0, d3x, d3y, empty_z);

        Node d4x; d4x = { 6., 10., 10., 6. };
        Node d4y; d4y = { 0., 0., 7., 7.};
        add_domain_node(spec, "domain4", 4, 4, 5, 0, d4x, d4y, empty_z);

        Node d5x; d5x = { 6., 10., 10., 6. };
        Node d5y; d5y = { 7., 7., 10., 10. };
        add_domain_node(spec, "domain5", 5, 4, 5, 0, d5x, d5y, empty_z);

        blueprint::mesh::examples::bent_multi_grid(spec, dsets["bentgrid_2d_visitghost"]);
    }

    Node info;
    NodeConstIterator itr = dsets.children();
    while (itr.has_next())
    {
        const Node& mesh = itr.next();
        std::string meshname = itr.name();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        test_save_mesh_helper(mesh, meshname);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_3D_enh_red_connectivity)
{
    Node io_protos;
    relay::io::about(io_protos["io"]);

    bool silo_enabled = io_protos["io/protocols/conduit_silo"].as_string() == "enabled";
    bool hdf5_enabled = io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // we are using one node to hold group of example meshes purely out of convenience
    Node dsets;

    {
        Node spec;

        Node d0x; d0x = { 0., 4., 4., 0., 0., 4., 4., 0. };
        Node d0y; d0y = { 0., 0., 4., 4., 0., 0., 4., 4. };
        Node d0z; d0z = { 0., 0., 0., 0., 3., 3., 3., 3. };
        add_domain_node(spec, "domain0", 0, 4, 5, 4, d0x, d0y, d0z);

        Node d1x; d1x = { 4., 6., 6., 4., 4., 6., 6., 4. };
        Node d1y; d1y = { 0., 0., 7., 4., 0., 0., 7., 4. };
        Node d1z; d1z = { 0., 0., 0., 0., 3., 7., 7., 3. };
        add_domain_node(spec, "domain1", 1, 4, 5, 4, d1x, d1y, d1z);

        Node d2x; d2x = { 0., 4., 6., 0., 0., 4., 6., 0. };
        Node d2y; d2y = { 4., 4., 7., 7., 4., 4., 7., 7. };
        Node d2z; d2z = { 0., 0., 0., 0., 3., 3., 7., 7. };
        add_domain_node(spec, "domain2", 2, 4, 4, 4, d2x, d2y, d2z);

        Node d3x; d3x = { 0., 6., 6., 0., 0., 6., 6., 0. };
        Node d3y; d3y = { 7., 7., 10., 10., 7., 7., 10., 10. };
        Node d3z; d3z = { 0., 0., 0., 0., 7., 7., 7., 7. };
        add_domain_node(spec, "domain3", 3, 4, 5, 4, d3x, d3y, d3z);

        Node d4x; d4x = { 6., 10., 10., 6., 6., 10., 10., 6. };
        Node d4y; d4y = { 0., 0., 7., 7., 0., 0., 7., 7. };
        Node d4z; d4z = { 0., 0., 0., 0., 7., 7., 7., 7. };
        add_domain_node(spec, "domain4", 4, 4, 5, 4, d4x, d4y, d4z);

        Node d5x; d5x = { 6., 10., 10., 6., 6., 10., 10., 6. };
        Node d5y; d5y = { 7., 7., 10., 10., 7., 7., 10., 10. };
        Node d5z; d5z = { 0., 0., 0., 0., 7., 7., 7., 7. };
        add_domain_node(spec, "domain5", 5, 4, 5, 4, d5x, d5y, d5z);

        Node d6x; d6x = { 0., 4., 4., 0., 0., 6., 6., 0. };
        Node d6y; d6y = { 0., 0., 4., 4., 0., 0., 7., 7. };
        Node d6z; d6z = { 3., 3., 3., 3., 7., 7., 7., 7. };
        add_domain_node(spec, "domain6", 6, 4, 5, 4, d6x, d6y, d6z);

        Node d7x; d7x = { 0., 6., 6., 0., 0., 6., 6., 0. };
        Node d7y; d7y = { 0., 0., 7., 7., 0., 0., 7., 7.};
        Node d7z; d7z = { 7., 7., 7., 7., 10., 10., 10., 10. };
        add_domain_node(spec, "domain7", 7, 4, 5, 2, d7x, d7y, d7z);

        Node d8x; d8x = { 6., 10., 10., 6., 6., 10., 10., 6. };
        Node d8y; d8y = { 0., 0., 7., 7., 0., 0., 7., 7. };
        Node d8z; d8z = { 7., 7., 7., 7., 10., 10., 10., 10. };
        add_domain_node(spec, "domain8", 8, 4, 5, 2, d8x, d8y, d8z);

        Node d9x; d9x = { 0., 6., 6., 0., 0., 6., 6., 0. };
        Node d9y; d9y = { 7., 7., 10., 10., 7., 7., 10., 10. };
        Node d9z; d9z = { 7., 7., 7., 7., 10., 10., 10., 10. };
        add_domain_node(spec, "domain9", 9, 4, 5, 2, d9x, d9y, d9z);

        Node d10x; d10x = { 6., 10., 10., 6., 6., 10., 10., 6. };
        Node d10y; d10y = { 7., 7., 10., 10., 7., 7., 10., 10. };
        Node d10z; d10z = { 7., 7., 7., 7., 10., 10., 10., 10. };
        add_domain_node(spec, "domain10", 10, 4, 5, 2, d10x, d10y, d10z);

        blueprint::mesh::examples::bent_multi_grid(spec, dsets["bentgrid_3d_visitghost"]);
    }

    Node info;
    NodeConstIterator itr = dsets.children();
    while (itr.has_next())
    {
        const Node& mesh = itr.next();
        std::string meshname = itr.name();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        test_save_mesh_helper(mesh, meshname);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, braid_too_small_npts)
{

    std::vector<std::string> braid_type_strings;


    braid_type_strings.push_back("uniform");
    braid_type_strings.push_back("lines");
    braid_type_strings.push_back("rectilinear");
    braid_type_strings.push_back("structured");

    // 2d extra
    braid_type_strings.push_back("tris");
    braid_type_strings.push_back("quads");


    // 3d extra

    braid_type_strings.push_back("tets");
    braid_type_strings.push_back("hexs");
    braid_type_strings.push_back("wedges");
    braid_type_strings.push_back("pyramids");

    Node mesh;

    for(size_t i = 0; i < braid_type_strings.size(); i++)
    {
        mesh.reset();
        EXPECT_THROW(blueprint::mesh::examples::braid(braid_type_strings[i],
                                                      1,
                                                      1,
                                                      1,
                                                      mesh), conduit::Error);
    }

    braid_type_strings.clear();
    braid_type_strings.push_back("points");
    braid_type_strings.push_back("points_implicit");

    for(size_t i = 0; i < braid_type_strings.size(); i++)
    {
        mesh.reset();
        blueprint::mesh::examples::braid(braid_type_strings[i],
                                         1,
                                         1,
                                         1,
                                         mesh);
        Node info;
        EXPECT_TRUE(blueprint::mesh::verify(mesh,info));
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, strided_structured_2d)
{
    Node res;
    Node desc;
    blueprint::mesh::examples::strided_structured(desc, 4, 3, 0, res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    CONDUIT_INFO(info.to_yaml());

    // also add a topo that shows the orig points
    // and fields assoced with orig points for debugging
    res["topologies/orig_pts/type"] = "points";
    res["topologies/orig_pts/coordset"] = "coords";
    
    res["fields/orig_vert_vals"] = res["fields/vert_vals"];
    res["fields/orig_vert_vals"].remove("offsets");
    res["fields/orig_vert_vals"].remove("strides");
    res["fields/orig_vert_vals/topology"] = "orig_pts";

    res["fields/orig_ele_vals"] = res["fields/ele_vals"];
    res["fields/orig_ele_vals"].remove("offsets");
    res["fields/orig_ele_vals"].remove("strides");
    res["fields/orig_ele_vals/topology"] = "orig_pts";

    test_save_mesh_helper(res, "strided_structured_2d");

    res.reset();
    desc.reset();
    info.reset();

    desc["vertex_data/shape"].set(DataType::index_t(3));
    index_t_array vertex_shape = desc["vertex_data/shape"].as_index_t_array();
    vertex_shape[0] = 7;
    vertex_shape[1] = 8;
    vertex_shape[2] = 0;
    desc["vertex_data/origin"].set(DataType::index_t(3));
    index_t_array vertex_origin = desc["vertex_data/origin"].as_index_t_array();
    vertex_origin[0] = 2;
    vertex_origin[1] = 1;
    vertex_origin[2] = 0;
    desc["element_data/shape"].set(DataType::index_t(3));
    index_t_array element_shape = desc["element_data/shape"].as_index_t_array();
    element_shape[0] = 6;
    element_shape[1] = 4;
    element_shape[2] = 0;
    desc["element_data/origin"].set(DataType::index_t(3));
    index_t_array element_origin = desc["element_data/origin"].as_index_t_array();
    element_origin[0] = 1;
    element_origin[1] = 1;
    element_origin[2] = 0;

    blueprint::mesh::examples::strided_structured(desc, 4, 3, 0, res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    CONDUIT_INFO(info.to_yaml());

    // also add a topo that shows the orig points
    // and fields assoced with orig points for debugging
    res["topologies/orig_pts/type"] = "points";
    res["topologies/orig_pts/coordset"] = "coords";
    
    res["fields/orig_vert_vals"] = res["fields/vert_vals"];
    res["fields/orig_vert_vals"].remove("offsets");
    res["fields/orig_vert_vals"].remove("strides");
    res["fields/orig_vert_vals/topology"] = "orig_pts";

    res["fields/orig_ele_vals"] = res["fields/ele_vals"];
    res["fields/orig_ele_vals"].remove("offsets");
    res["fields/orig_ele_vals"].remove("strides");
    res["fields/orig_ele_vals/topology"] = "orig_pts";

    test_save_mesh_helper(res, "strided_structured_2d_pad");
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, strided_structured_3d)
{
    Node res;
    Node desc;
    blueprint::mesh::examples::strided_structured(desc, 6, 5, 3, res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    CONDUIT_INFO(info.to_yaml());

    // also add a topo that shows the orig points
    // and fields assoced with orig points for debugging
    res["topologies/orig_pts/type"] = "points";
    res["topologies/orig_pts/coordset"] = "coords";
    
    res["fields/orig_vert_vals"] = res["fields/vert_vals"];
    res["fields/orig_vert_vals"].remove("offsets");
    res["fields/orig_vert_vals"].remove("strides");
    res["fields/orig_vert_vals/topology"] = "orig_pts";

    res["fields/orig_ele_vals"] = res["fields/ele_vals"];
    res["fields/orig_ele_vals"].remove("offsets");
    res["fields/orig_ele_vals"].remove("strides");
    res["fields/orig_ele_vals/topology"] = "orig_pts";

    test_save_mesh_helper(res, "strided_structured_3d");

    res.reset();
    desc.reset();
    info.reset();

    desc["vertex_data/shape"].set(DataType::index_t(3));
    index_t_array vertex_shape = desc["vertex_data/shape"].as_index_t_array();
    vertex_shape[0] = 10;
    vertex_shape[1] = 8;
    vertex_shape[2] = 3;
    desc["vertex_data/origin"].set(DataType::index_t(3));
    index_t_array vertex_origin = desc["vertex_data/origin"].as_index_t_array();
    vertex_origin[0] = 2;
    vertex_origin[1] = 1;
    vertex_origin[2] = 0;
    desc["element_data/shape"].set(DataType::index_t(3));
    index_t_array element_shape = desc["element_data/shape"].as_index_t_array();
    element_shape[0] = 6;
    element_shape[1] = 4;
    element_shape[2] = 2;
    desc["element_data/origin"].set(DataType::index_t(3));
    index_t_array element_origin = desc["element_data/origin"].as_index_t_array();
    element_origin[0] = 1;
    element_origin[1] = 1;
    element_origin[2] = 0;

    blueprint::mesh::examples::strided_structured(desc, 4, 3, 2, res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    CONDUIT_INFO(info.to_yaml());

    // also add a topo that shows the orig points
    // and fields assoced with orig points for debugging
    res["topologies/orig_pts/type"] = "points";
    res["topologies/orig_pts/coordset"] = "coords";
    
    res["fields/orig_vert_vals"] = res["fields/vert_vals"];
    res["fields/orig_vert_vals"].remove("offsets");
    res["fields/orig_vert_vals"].remove("strides");
    res["fields/orig_vert_vals/topology"] = "orig_pts";

    res["fields/orig_ele_vals"] = res["fields/ele_vals"];
    res["fields/orig_ele_vals"].remove("offsets");
    res["fields/orig_ele_vals"].remove("strides");
    res["fields/orig_ele_vals/topology"] = "orig_pts";

    test_save_mesh_helper(res, "strided_structured_3d_pad");

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, strided_structured_colmajor)
{
    Node res;

    const std::string yaml_text = ""
    "coordsets:\n"
    "  coords:\n"
    "    type: \"explicit\"\n"
    "    values:\n"
    "      x: [-10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0,\n"
    "          -10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0,\n"
    "          -10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0,\n"
    "          -10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0,\n"
    "          -10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0,\n"
    "          -10.0,  -6.6,  -3.3,   0.0,   3.3,   6.6,  10.0]\n"
    "      y: [-10.0, -10.0, -10.0, -10.0, -10.0, -10.0, -10.0,\n"
    "           -6.0,  -6.0,  -6.0,  -6.0,  -6.0,  -6.0,  -6.0,\n"
    "           -2.0,  -2.0,  -2.0,  -2.0,  -2.0,  -2.0,  -2.0,\n"
    "            2.0,   2.0,   2.0,   2.0,   2.0,   2.0,   2.0,\n"
    "            6.0,   6.0,   6.0,   6.0,   6.0,   6.0,   6.0,\n"
    "            10.0, 10.0,  10.0,  10.0,  10.0,  10.0,  10.0]\n"
    "  col_maj_coords:\n"
    "    type: \"explicit\"\n"
    "    values:\n"
    "      x: [-10.0, -10.0, -10.0, -10.0, -10.0, -10.0,\n"
    "           -6.6,  -6.6,  -6.6,  -6.6,  -6.6,  -6.6,\n"
    "           -3.3,  -3.3,  -3.3,  -3.3,  -3.3,  -3.3,\n"
    "            0.0,   0.0,   0.0,   0.0,   0.0,   0.0,\n"
    "            3.3,   3.3,   3.3,   3.3,   3.3,   3.3,\n"
    "            6.6,   6.6,   6.6,   6.6,   6.6,   6.6,\n"
    "           10.0,  10.0,  10.0,  10.0,  10.0,  10.0]\n"
    "      y: [-10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0,\n"
    "          -10.0,  -6.0,  -2.0,   2.0,   6.0,  10.0]\n"
    "topologies:\n"
    "  mesh:\n"
    "    type: \"structured\"\n"
    "    coordset: \"coords\"\n"
    "    elements:\n"
    "      dims:\n"
    "        i: 3\n"
    "        j: 2\n"
    "        offsets: [2, 2]\n"
    "        strides: [1, 7]\n"
    "  full_mesh:\n"
    "    type: \"structured\"\n"
    "    coordset: \"coords\"\n"
    "    elements:\n"
    "      dims:\n"
    "        i: 6\n"
    "        j: 5\n"
    "        offsets: [0, 0]\n"
    "        strides: [1, 7]\n"
    "  mesh_col_maj:\n"
    "    type: \"structured\"\n"
    "    coordset: \"col_maj_coords\"\n"
    "    elements:\n"
    "      dims:\n"
    "        i: 3\n"
    "        j: 2\n"
    "        offsets: [2, 2]\n"
    "        strides: [6, 1]\n"
    "  full_mesh_col_maj:\n"
    "    type: \"structured\"\n"
    "    coordset: \"col_maj_coords\"\n"
    "    elements:\n"
    "      dims:\n"
    "        i: 6\n"
    "        j: 5\n"
    "        strides: [6, 1]\n";

    res.parse(yaml_text, "yaml");

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    CONDUIT_INFO(info.to_yaml());

    test_save_mesh_helper(res, "strided_structured_colmajor");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mixed_mesh_simple_2d)
{
    Node data, verify_info;

    //
    // Create example mesh.
    //

    // create simple mixed 2d mesh with triangles and quads
    /*
    3     *-------*-------*       (8, 9, 10)
         / \     / \     / \
        / d \ e / f \ g / h \
       /     \ /     \ /     \
    1  *------*-------*------*    (4, 5, 6, 7)
       |   a  |   b   |  c   |
    0  *------*-------*------*
       0      1       2      3
    */

    data["coordsets/coords/type"] = "explicit";
    data["coordsets/coords/values/x"].set(DataType::float64(11));
    data["coordsets/coords/values/y"].set(DataType::float64(11));

    data["coordsets/coords/values/x"] = { 0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.5, 1.5, 2.5 };
    data["coordsets/coords/values/y"] = { 0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          3.0, 3.0, 3.0};

    data["topologies/topo/type"] = "unstructured";
    data["topologies/topo/coordset"] = "coords";
    data["topologies/topo/elements/shape"] = "mixed";
    data["topologies/topo/elements/shape_map/tri"]  = 5;
    data["topologies/topo/elements/shape_map/quad"] = 9;
    data["topologies/topo/elements/shapes"] = { 9, 9, 9,
                                                5, 5, 5, 5, 5};
    data["topologies/topo/elements/sizes"] =  { 4, 4, 4,
                                                3, 3, 3, 3, 3};
    data["topologies/topo/elements/offsets"] =  {0, 4, 8,
                                                 12, 15, 18, 21, 24};

    data["topologies/topo/elements/connectivity"] =  {0, 1, 5, 4,
                                                      1, 2, 6, 5,
                                                      2, 3, 7, 6,
                                                      4, 5, 8,
                                                      8, 5, 9,
                                                      5, 6, 9,
                                                      9, 6, 10,
                                                      6, 7, 10};

    data["fields/ele_id/topology"] = "topo";
    data["fields/ele_id/association"] = "element";
    data["fields/ele_id/values"] = { 0, 1, 2,
                                     3, 4, 5, 6, 7};

    // also add a points topo to help with debugging

    data["topologies/pts/type"] = "points";
    data["topologies/pts/coordset"] = "coords";
    data["fields/pts_id/topology"] = "pts";
    data["fields/pts_id/association"] = "element";
    data["fields/pts_id/values"] = { 0, 1, 2, 3,
                                     4, 5, 6, 7, 
                                     8, 9, 10};

    //data.print();

    EXPECT_TRUE(blueprint::mesh::verify(data, verify_info));
    CONDUIT_INFO(verify_info.to_yaml());

    test_save_mesh_helper(data, "mixed_mesh_simple_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mixed_mesh_polygonal_2d)
{
    Node data, verify_info;

    //
    // Create example mesh.
    //

    // create simple mixed 2d mesh with triangles, quads, and a polygon
    /*
    3     *-------*-------*       (8, 9, 10)
         / \     /         \
        / d \ e /     f     \
       /     \ /   ---*---   \
    1  *------*---/   |   \--*    (4, 5, 6, 7)
       |   a  |   b   |  c   |
    0  *------*-------*------*
       0      1       2      3
    */

    data["coordsets/coords/type"] = "explicit";
    data["coordsets/coords/values/x"].set(DataType::float64(11));
    data["coordsets/coords/values/y"].set(DataType::float64(11));

    data["coordsets/coords/values/x"] = { 0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.5, 1.5, 2.5 };
    data["coordsets/coords/values/y"] = { 0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.5, 1.0,
                                          3.0, 3.0, 3.0};

    data["topologies/topo/type"] = "unstructured";
    data["topologies/topo/coordset"] = "coords";
    data["topologies/topo/elements/shape"] = "mixed";
    data["topologies/topo/elements/shape_map/tri"]  = 5;
    data["topologies/topo/elements/shape_map/quad"] = 9;
    data["topologies/topo/elements/shape_map/polygonal"] = 7;
    data["topologies/topo/elements/shapes"] = { 9, 9, 9,
                                                5, 5,
                                                7};
    data["topologies/topo/elements/sizes"] =  { 4, 4, 4,
                                                3, 3,
                                                5};
    data["topologies/topo/elements/offsets"] =  {0, 4, 8,
                                                 12, 15,
                                                 18};

    data["topologies/topo/elements/connectivity"] =  {0, 1, 5, 4,
                                                      1, 2, 6, 5,
                                                      2, 3, 7, 6,
                                                      4, 5, 8,
                                                      8, 5, 9,
                                                      5, 6, 7, 10, 9};

    data["fields/ele_id/topology"] = "topo";
    data["fields/ele_id/association"] = "element";
    data["fields/ele_id/values"] = { 0, 1, 2,
                                     3, 4,
                                     5};

    // also add a points topo to help with debugging

    data["topologies/pts/type"] = "points";
    data["topologies/pts/coordset"] = "coords";
    data["fields/pts_id/topology"] = "pts";
    data["fields/pts_id/association"] = "element";
    data["fields/pts_id/values"] = { 0, 1, 2, 3,
                                     4, 5, 6, 7, 
                                     8, 9, 10};

    //data.print();

    EXPECT_TRUE(blueprint::mesh::verify(data, verify_info));
    CONDUIT_INFO(verify_info.to_yaml());

    test_save_mesh_helper(data, "mixed_mesh_polygonal_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mixed_mesh_simple_3d)
{
    Node data, verify_info;

    //
    // Create example mesh.
    //

    // create simple mixed 3d mesh with hexs and pyramids
    /*
            *       *      * ( x: 0.5, 1.5, 2.5). 16, 17, 18
                             ( y:   2,   2,   2)
                             ( z: 0.5, 0.5, 0.5)
   1    *------*-------*------*  (12, 13, 14, 15) (back upper)
  z    /      /.      /.     /|
 0  1 *------*-------*------* |  (8, 9, 10, 11) (front upper)
    y |   a  |   b   |  c   | / 
    0 *------*-------*------*    (4, 5, 6, 7) (back lower)
      0      1       2      3
    */

    data["coordsets/coords/type"] = "explicit";
    data["coordsets/coords/values/x"] = { 0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.5, 1.5, 2.5};

    data["coordsets/coords/values/y"] = { 0.0, 0.0, 0.0, 0.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          2.0, 2.0, 2.0};

    data["coordsets/coords/values/z"] = { 0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          0.5, 0.5, 0.5};


    data["topologies/topo/type"] = "unstructured";
    data["topologies/topo/coordset"] = "coords";
    data["topologies/topo/elements/shape"] = "mixed";
    data["topologies/topo/elements/shape_map/hex"]  = 12;
    data["topologies/topo/elements/shape_map/pyramid"]  = 14;
    data["topologies/topo/elements/shapes"] =  { 12, 12, 12, 14, 14, 14 };
    data["topologies/topo/elements/sizes"] =   { 8, 8, 8, 5, 5, 5};
    data["topologies/topo/elements/offsets"] = { 0, 8, 16,
                                                 24, 29, 34};

    data["topologies/topo/elements/connectivity"] =  {  0,  1,  5,  4,  8,  9, 13, 12,
                                                        1,  2,  6,  5,  9, 10, 14, 13,
                                                        2,  3,  7,  6, 10, 11, 15, 14,
                                                        8,  9, 13, 12, 16,
                                                        9, 10, 14, 13, 17,
                                                       10, 11, 15, 14, 18
                                                    };

    data["fields/ele_id/topology"] = "topo";
    data["fields/ele_id/association"] = "element";
    data["fields/ele_id/values"] = { 0, 1, 2, 3, 4, 5};

    // also add a points topo to help with debugging

    data["topologies/pts/type"] = "points";
    data["topologies/pts/coordset"] = "coords";
    data["fields/pts_id/topology"] = "pts";
    data["fields/pts_id/association"] = "element";
    data["fields/pts_id/values"] = {  0,  1,  2,  3,
                                      4,  5,  6,  7,
                                      8,  9, 10, 11,
                                     12, 13, 14, 15,
                                     16, 17, 18};

    //data.print();

    EXPECT_TRUE(blueprint::mesh::verify(data, verify_info));
    CONDUIT_INFO(verify_info.to_yaml());

    test_save_mesh_helper(data, "mixed_mesh_simple_3d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mixed_mesh_polyhedral_3d)
{
    Node data, verify_info;

    // TODO we should explore why this looks wrong in VisIt. I think
    // generate_sides is the culprit. https://github.com/LLNL/conduit/issues/1289

    //
    // Create example mesh.
    //

    // create mixed 3d mesh with hexs and pyramids and concave polyhedra
    /*
            *       *      * ( x: 0.5, 1.5, 2.5). 16, 17, 18
                             ( y:   2,   2,   2)
                             ( z: 0.5, 0.5, 0.5)
   1    *------*-------*------*  (12, 13, 14, 15) (back upper)
  z    /      /.      /.     /|
 0  1 *------*-------*------* |  (8, 9, 10, 11) (front upper)
    y |   a  |   b   |  c   | / 
    0 *------*-------*------*    (4, 5, 6, 7) (back lower)
      0      1       2      3
    */

    data["coordsets/coords/type"] = "explicit";
    data["coordsets/coords/values/x"] = { 0.75, 1.0, 2.0, 3.0,
                                          0.75, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.0, 1.0, 2.0, 3.0,
                                          0.5, 1.5, 2.5};

    data["coordsets/coords/values/y"] = { 0.75, 0.0, 0.0, 0.0,
                                          0.75, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          2.0, 2.0, 2.0};

    data["coordsets/coords/values/z"] = { 0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          0.0, 0.0, 0.0, 0.0,
                                          1.0, 1.0, 1.0, 1.0,
                                          0.5, 0.5, 0.5};


    data["topologies/topo/type"] = "unstructured";
    data["topologies/topo/coordset"] = "coords";
    data["topologies/topo/elements/shape"] = "mixed";
    data["topologies/topo/elements/shape_map/hex"]  = 12;
    data["topologies/topo/elements/shape_map/pyramid"]  = 14;
    data["topologies/topo/elements/shape_map/polyhedral"]  = 42;
    data["topologies/topo/elements/shapes"] =  { 42, 12, 12, 14, 14, 14 };
    data["topologies/topo/elements/sizes"] =   { 6, 8, 8, 5, 5, 5};
    data["topologies/topo/elements/offsets"] = { 0, 6, 14,
                                                 22, 27, 32};

    data["topologies/topo/elements/connectivity"] =  {  0,  1,  2,  3,  4,  5,
                                                        1,  2,  6,  5,  9, 10, 14, 13,
                                                        2,  3,  7,  6, 10, 11, 15, 14,
                                                        8,  9, 13, 12, 16,
                                                        9, 10, 14, 13, 17,
                                                       10, 11, 15, 14, 18
                                                    };

    data["topologies/topo/subelements/shape"] = "mixed";
    data["topologies/topo/subelements/shape_map/quad"]  = 9;
    data["topologies/topo/subelements/shapes"] =  { 9, 9, 9, 9, 9, 9 };

    data["topologies/topo/subelements/sizes"] = {4, 4, 4, 4, 4, 4};
    data["topologies/topo/subelements/offsets"] = {0, 4, 8, 12, 16, 20};
    data["topologies/topo/subelements/connectivity"] = {0, 1, 5, 4,  
                                                        0, 8, 9, 1,
                                                        1, 9, 13, 5,
                                                        5, 13, 12, 4,
                                                        4, 12, 8, 0,
                                                        9, 8, 12, 13};

    data["fields/ele_id/topology"] = "topo";
    data["fields/ele_id/association"] = "element";
    data["fields/ele_id/values"] = { 0, 1, 2, 3, 4, 5};

    // also add a points topo to help with debugging

    data["topologies/pts/type"] = "points";
    data["topologies/pts/coordset"] = "coords";
    data["fields/pts_id/topology"] = "pts";
    data["fields/pts_id/association"] = "element";
    data["fields/pts_id/values"] = {  0,  1,  2,  3,
                                      4,  5,  6,  7,
                                      8,  9, 10, 11,
                                     12, 13, 14, 15,
                                     16, 17, 18};

    //data.print();

    EXPECT_TRUE(blueprint::mesh::verify(data, verify_info));
    CONDUIT_INFO(verify_info.to_yaml());

    test_save_mesh_helper(data, "mixed_mesh_polyhedral_3d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, julia)
{
    Node res;
    blueprint::mesh::examples::julia(500,   500, // nx, ny
                                     -2.0,  2.0, // x range
                                     -2.0,  2.0, // y range
                                     0.285, 0.01, // c value
                                     res);
    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res,info));
    CONDUIT_INFO(info.to_yaml());

    test_save_mesh_helper(res,"julia_example");
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, spiral)
{
    int ndoms = 10;
    Node res;
    blueprint::mesh::examples::spiral(ndoms,res["spiral"]);
    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res["spiral/domain_000000"],info));
    CONDUIT_INFO(info.to_yaml());

    blueprint::mesh::generate_index(res["spiral/domain_000000"],
                                    "",
                                    ndoms,
                                    res["blueprint_index/spiral"]);

    // save json
    res["protocol/name"] = "json";
    res["protocol/version"] = PROTOCOL_VER;

    res["number_of_files"] = 1;
    res["number_of_trees"] = ndoms;
    res["file_pattern"] = "spiral_example.blueprint_root";
    res["tree_pattern"] = "spiral/domain_%06d";

    CONDUIT_INFO("Creating: spiral_example.blueprint_root")
    relay::io::save(res,"spiral_example.blueprint_root","json");
}



//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, polytess)
{
    const index_t nlevels = 3;
    const index_t nz = 1;
    Node res;
    blueprint::mesh::examples::polytess(nlevels,
                                        nz,
                                        res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res,info));
    CONDUIT_INFO(info.to_yaml());

    test_save_mesh_helper(res,"polytess_example");
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, polytess_3d)
{
    Node res;
    blueprint::mesh::examples::polytess(3, 10, res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res,info));
    CONDUIT_INFO(info.to_yaml());

    if(conduit::utils::is_file("polytess_3d_example_hdf5.root"))
    {
        conduit::utils::remove_file("polytess_3d_example_hdf5.root");
    }

    test_save_mesh_helper(res,"polytess_3d_example");
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, 2d_braid_zero_z_check)
{
    Node mesh, info;
    // these checks make sure braid generates valid fields even when
    // # of z pointers == 0
    int npts_x = 5;
    int npts_y = 5;
    int npts_z = 0;

    std::vector<std::string> braid_type_strings;
    braid_type_strings.push_back("points");
    braid_type_strings.push_back("points_implicit");
    braid_type_strings.push_back("lines");
    braid_type_strings.push_back("rectilinear");
    braid_type_strings.push_back("structured");
    braid_type_strings.push_back("tris");
    braid_type_strings.push_back("quads");

    for(size_t i = 0; i < braid_type_strings.size(); i++)
    {
        mesh.reset();
        blueprint::mesh::examples::braid(braid_type_strings[i],
                                          npts_x,
                                          npts_y,
                                          npts_z,
                                          mesh);
        // make the braid vertex-assoced field has with more than zero entries
        EXPECT_GT(mesh["fields/braid/values"].dtype().number_of_elements(),0);
        mesh.print();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_misc)
{
    // for each "misc" mesh example, generate a small 2d mesh and
    // save to yaml
    std::vector<std::string> misc_type_strings;
    misc_type_strings.push_back("matsets");
    misc_type_strings.push_back("specsets");
    misc_type_strings.push_back("nestsets");

    Node mesh;
    int npts_x = 5;
    int npts_y = 5;

    for(size_t i = 0; i < misc_type_strings.size(); i++)
    {
        mesh.reset();
        blueprint::mesh::examples::misc(misc_type_strings[i],
                                        npts_x,
                                        npts_y,
                                        1,
                                        mesh);
        mesh.save("misc_example_" + misc_type_strings[i] + ".yaml");
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, check_gen_index_state_prop)
{
    // braid provides cycle and time, make sure those make it into
    // an index created with generate_index
    Node mesh;
    blueprint::mesh::examples::braid("hexs",
                                      5,
                                      5,
                                      5,
                                      mesh);

    Node idx;
    blueprint::mesh::generate_index(mesh,
                                    "",
                                    1,
                                    idx);

    EXPECT_TRUE(idx.has_path("state/cycle"));
    EXPECT_TRUE(idx.has_path("state/time"));
}



//-----------------------------------------------------------------------------
void venn_test_small_yaml(const std::string &venn_type, const bool specsets)
{
    // provide small example save to yaml for folks to look at
    const int nx = 2, ny = 2;
    const double radius = 0.25;

    Node res, info, n_idx;
    if (specsets)
    {
        blueprint::mesh::examples::venn_specsets(venn_type, nx, ny, radius, res);
    }
    else
    {
        blueprint::mesh::examples::venn(venn_type, nx, ny, radius, res);
    }

    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    info.print();
    blueprint::mesh::generate_index(res,"",1,n_idx);
    EXPECT_TRUE(blueprint::verify("mesh/index",n_idx,info));

    std::string ofbase = "venn_" + std::string(specsets ? "specsets_" : "") + "small_example_" + venn_type;

    // save yaml and hdf5 versions
    relay::io::blueprint::save_mesh(res,
                                    ofbase + "_yaml",
                                    "yaml");

    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled =io_protos["io/protocols/hdf5"].as_string() == "enabled";

    if(hdf5_enabled)
    {
        relay::io::blueprint::save_mesh(res,
                                        ofbase + "_hdf5",
                                        "hdf5");
    }

}

//-----------------------------------------------------------------------------
void venn_test(const std::string &venn_type, const bool specsets)
{
    const int nx = 100, ny = 100;
    const double radius = 0.25;

    Node res, info;
    if (specsets)
    {
        blueprint::mesh::examples::venn_specsets(venn_type, nx, ny, radius, res);
    }
    else
    {
        blueprint::mesh::examples::venn(venn_type, nx, ny, radius, res);
    }

    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    utils::log::remove_valid(info);
    CONDUIT_INFO(info.to_yaml());
    CONDUIT_INFO(res.schema().to_json());

    std::string ofbase = "venn_" + std::string(specsets ? "specsets_" : "") + "example_" + venn_type;
    std::cout << "[Saving " << ofbase << "]" << std::endl;

    // save yaml and hdf5 versions
    relay::io::blueprint::save_mesh(res,
                                    ofbase + "_yaml",
                                    "yaml");

    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled =io_protos["io/protocols/hdf5"].as_string() == "enabled";

    if(hdf5_enabled)
    {
        relay::io::blueprint::save_mesh(res,
                                        ofbase + "_hdf5",
                                        "hdf5");
    }

    {
        std::cout << "[Verifying field area is correct]" << std::endl;

        Node &area_actual = res["fields/area/values"];
        Node area_expected;
        area_expected.set(std::vector<double>(
            area_actual.dtype().number_of_elements(), 1.0 / (nx * ny)));

        bool actual_matches_expected = !area_expected.diff(area_actual, info);
        EXPECT_TRUE(actual_matches_expected);
        if(!actual_matches_expected)
        {
            CONDUIT_INFO(info.to_yaml());
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, venn_full)
{
    venn_test("full", false);
    venn_test("full", true);
    venn_test_small_yaml("full", false);
    venn_test_small_yaml("full", true);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, venn_sparse_by_material)
{
    venn_test("sparse_by_material", false);
    venn_test("sparse_by_material", true);
    venn_test_small_yaml("sparse_by_material", false);
    venn_test_small_yaml("sparse_by_material", true);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, venn_sparse_by_element)
{
    venn_test("sparse_by_element", false);
    venn_test("sparse_by_element", true);
    venn_test_small_yaml("sparse_by_element", false);
    venn_test_small_yaml("sparse_by_element", true);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_julia_nestset_simple)
{

    Node mesh;

    Node res;
    blueprint::mesh::examples::julia_nestsets_simple(-2.0,  2.0, // x range
                                                     -2.0,  2.0, // y range
                                                     0.285, 0.01, // c value
                                                     res["julia_nestset_simple"]);

    index_t ndoms = res["julia_nestset_simple"].number_of_children();

    Node info;
    if(!blueprint::mesh::verify(res["julia_nestset_simple"],info))
    {
      info.print();
    }

    blueprint::mesh::generate_index(res["julia_nestset_simple/domain_000000"],
                                    "",
                                    ndoms,
                                    res["blueprint_index/julia_nestset_simple"]);

    // save json
    res["protocol/name"] = "json";
    res["protocol/version"] = PROTOCOL_VER;

    res["number_of_files"] = 1;
    res["number_of_trees"] = ndoms;
    res["file_pattern"] = "julia_nestset_simple.blueprint_root";
    res["tree_pattern"] = "julia_nestset_simple/domain_%06d";

    CONDUIT_INFO("Creating: julia_nestset_simple.blueprint_root")
    relay::io::save(res,"julia_nestset_simple.blueprint_root","json");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, mesh_julia_nestset)
{

    Node mesh;

    Node res;
    blueprint::mesh::examples::julia_nestsets_complex(20,   20, // nx, ny
                                                      -2.0,  2.0, // x range
                                                      -2.0,  2.0, // y range
                                                      0.285, 0.01, // c value
                                                      3, // amr levels
                                                      res["julia_nestset_complex"]);

    index_t ndoms = res["julia_nestset_complex"].number_of_children();

    Node info;
    if(!blueprint::mesh::verify(res["julia_nestset_complex"],info))
    {
      info.print();
    }

    blueprint::mesh::generate_index(res["julia_nestset_complex/domain_000000"],
                                    "",
                                    ndoms,
                                    res["blueprint_index/julia_nestset_complex"]);

    // save json
    res["protocol/name"] = "json";
    res["protocol/version"] = PROTOCOL_VER;

    res["number_of_files"] = 1;
    res["number_of_trees"] = ndoms;
    res["file_pattern"] = "julia_nestset_complex_example.blueprint_root";
    res["tree_pattern"] = "julia_nestset_complex/domain_%06d";

    CONDUIT_INFO("Creating: julia_nestset_complex_example.blueprint_root")
    relay::io::save(res,"julia_nestset_complex_example.blueprint_root","json");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, save_adjset_uniform)
{

    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled =io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // skip if we don't have hdf5 since this example has several domains
    if(!hdf5_enabled)
        return;

    Node mesh,idx;
    blueprint::mesh::examples::adjset_uniform(mesh);
    blueprint::mesh::generate_index(mesh[0],
                                    "",
                                    8,
                                    mesh["blueprint_index/adj_uniform"]);

    mesh["protocol/name"] = "hdf5";
    mesh["protocol/version"] = PROTOCOL_VER;

    mesh["number_of_files"] = 1;
    mesh["number_of_trees"] = 8;
    mesh["file_pattern"] = "adj_uniform_example.blueprint_root";
    mesh["tree_pattern"] = "domain_%06d";

    CONDUIT_INFO("Creating: adj_uniform_example.blueprint_root")
    relay::io::save(mesh,"adj_uniform_example.blueprint_root","json");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, related_boundary)
{

    Node io_protos;
    relay::io::about(io_protos["io"]);
    bool hdf5_enabled =io_protos["io/protocols/hdf5"].as_string() == "enabled";

    // skip if we don't have hdf5 since this example has several domains
    if(!hdf5_enabled)
        return;

    Node mesh;
    index_t base_grid_ele_i = 10;
    index_t base_grid_ele_j = 5;

    conduit::blueprint::mesh::examples::related_boundary(base_grid_ele_i,
                                                         base_grid_ele_j,
                                                         mesh);

    conduit::relay::io::blueprint::save_mesh(mesh,
                                             "related_boundary_base_i_10_j_5",
                                             "hdf5");

    mesh.reset();
    base_grid_ele_i = 9;
    base_grid_ele_j = 7;

    conduit::blueprint::mesh::examples::related_boundary(base_grid_ele_i,
                                                         base_grid_ele_j,
                                                         mesh);

    conduit::relay::io::blueprint::save_mesh(mesh,
                                             "related_boundary_base_i_9_j_7",
                                             "hdf5");

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, basic_bad_inputs)
{
    Node res;

    // several with bad inputs
    EXPECT_THROW(blueprint::mesh::examples::basic("uniform",
                                                  -1,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("structured",
                                                  2,
                                                  1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("uniform",
                                                  1,
                                                  1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("uniform",
                                                  2,
                                                  -1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("tets",
                                                  2,
                                                  2,
                                                  1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("hexs",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("wedges",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("pyramids",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::basic("polyhedra",
                                                  2,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    // a few ok
    blueprint::mesh::examples::basic("uniform",
                                     5,
                                     0,
                                     0,
                                     res);

    blueprint::mesh::examples::basic("rectilinear",
                                     5,
                                     0,
                                     0,
                                     res);

    blueprint::mesh::examples::basic("uniform",
                                     2,
                                     2,
                                     2,
                                     res);

    blueprint::mesh::examples::basic("tets",
                                     2,
                                     2,
                                     2,
                                     res);

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, grid_bad_inputs)
{
    Node res;

    // several with bad inputs
    EXPECT_THROW(blueprint::mesh::examples::grid("uniform",
                                                  2,
                                                  2,
                                                  2,
                                                  -1,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::grid("uniform",
                                                  2,
                                                  2,
                                                  2,
                                                  1,
                                                  1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::grid("uniform",
                                                  2,
                                                  2,
                                                  2,
                                                  2,
                                                  -1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::grid("hexs",
                                                  2,
                                                  2,
                                                  2,
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::grid("polyhedra",
                                                  2,
                                                  2,
                                                  2,
                                                  2,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    // a few ok
    blueprint::mesh::examples::grid("uniform",
                                     2,
                                     2,
                                     2,
                                     2,
                                     2,
                                     2,
                                     res);

    blueprint::mesh::examples::grid("tets",
                                     2,
                                     2,
                                     2,
                                     2,
                                     2,
                                     2,
                                     res);

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, braid_bad_inputs)
{
    Node res;

    // several with bad inputs
    EXPECT_THROW(blueprint::mesh::examples::braid("uniform",
                                                  -1,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("uniform",
                                                  1,
                                                  1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("uniform",
                                                  2,
                                                  -1,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("tets",
                                                  2,
                                                  2,
                                                  1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("hexs",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("wedges",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("pyramids",
                                                  2,
                                                  2,
                                                  0,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("hexs_poly",
                                                  2,
                                                  2,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("points",
                                                  0,
                                                  0,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("points_implicit",
                                                  0,
                                                  0,
                                                  -1,
                                                  res),conduit::Error);

    EXPECT_THROW(blueprint::mesh::examples::braid("points",
                                                  1,
                                                  1,
                                                  -1,
                                                  res),conduit::Error);

    // a few ok
    blueprint::mesh::examples::braid("points",
                                     1,
                                     1,
                                     0,
                                     res);

    res.print();
    // check conn array, should have 1 entry
    EXPECT_EQ(res["topologies/mesh/elements/connectivity"].dtype().number_of_elements(),1);

    // should be 2d
    EXPECT_EQ(res["coordsets/coords/values"].number_of_children(),2);

    blueprint::mesh::examples::braid("points",
                                     1,
                                     1,
                                     2,
                                     res);
    // should be 3d
    EXPECT_EQ(res["coordsets/coords/values"].number_of_children(),3);


    blueprint::mesh::examples::braid("uniform",
                                     2,
                                     2,
                                     2,
                                     res);

    blueprint::mesh::examples::braid("tets",
                                     2,
                                     2,
                                     2,
                                     res);

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, braid_diff_dims)
{
    Node mesh;
    conduit::blueprint::mesh::examples::braid("quads", 2, 3, 0, mesh);
    test_save_mesh_helper(mesh,"braid_quads_2_3");

    conduit::blueprint::mesh::examples::braid("tris", 2, 3, 0, mesh);
    test_save_mesh_helper(mesh,"braid_tris_2_3");

    conduit::blueprint::mesh::examples::braid("tets", 2, 3, 4, mesh);
    test_save_mesh_helper(mesh,"braid_tets_2_3_4");

    conduit::blueprint::mesh::examples::braid("hexs", 2, 3, 4, mesh);
    test_save_mesh_helper(mesh,"braid_hexs_2_3_4");

    conduit::blueprint::mesh::examples::braid("wedges", 2, 3, 4, mesh);
    test_save_mesh_helper(mesh,"braid_wedges_2_3_4");

    conduit::blueprint::mesh::examples::braid("pyramids", 2, 3, 4, mesh);
    test_save_mesh_helper(mesh,"braid_pyramids_2_3_4");

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, number_of_domains)
{
    // test both multi dom and non multi dom case

    Node data;
    // braid
    blueprint::mesh::examples::braid("uniform",
                                      10,10,10,
                                      data);

    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(data),1);

    // now check a multi domain mesh ...

    // spiral with 3 domains
    conduit::blueprint::mesh::examples::spiral(3,data);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(data),3);

    // spiral with 7 domains
    conduit::blueprint::mesh::examples::spiral(7,data);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(data),7);


}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, polychain)
{
    Node res;
    blueprint::mesh::examples::polychain(7, res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res,info));
    CONDUIT_INFO(info.to_yaml());

    test_save_mesh_helper(res,"polychain_example");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, gyre)
{

    index_t nsteps = 5;
    float64 t = 0.0;
    float64 dt = 1.0;
    Node res;
    for(index_t i=0; i < nsteps; i++)
    {
        blueprint::mesh::examples::gyre(25,25,2,t,res);
        Node info;
        EXPECT_TRUE(blueprint::mesh::verify(res,info));
        t+= dt;
        std::string ofbase = conduit_fmt::format("gyre_{:04d}",i);
        test_save_mesh_helper(res,ofbase);
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, polystar)
{
    Node res;
    blueprint::mesh::examples::polystar(res);

    Node info;
    EXPECT_TRUE(blueprint::mesh::verify(res,info));
    CONDUIT_INFO(info.to_yaml());

    test_save_mesh_helper(res,"polystar_example");
}

//-----------------------------------------------------------------------------
void add_oneD_matset_field(Node& mesh, const std::string& topo_name, int nele)
{
    Node& fields = mesh["fields"];
    Node& matsets = mesh["matsets"];

    Node& f1 = fields["f1"];
    f1["association"] = "element";
    f1["topology"] = topo_name;
    f1["volume_dependent"] = "false";
    f1["matset"] = "matset";
    f1["matset_values/001"].set(DataType::float64(nele));
    f1["values"].set(DataType::float64(nele));

    Node& ms1 = matsets["matset"];
    ms1["topology"] = topo_name;
    ms1["volume_fractions/001"].set(DataType::float64(nele));

    float64* f1ms001vals = f1["matset_values/001"].value();
    float64* f1vals = f1["values"].value();
    float64* ms1vfracs = ms1["volume_fractions/001"].value();

    float64 dval = 1. / nele;

    for (int i = 0; i < nele; ++i)
    {
        f1ms001vals[i] = 1. - i*dval;
        f1vals[i] = 1. - i * dval;
        ms1vfracs[i] = 1.0;
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, oneDtostrip)
{
    const int nvert = 5;
    {
        Node mesh;
        blueprint::mesh::examples::basic("rectilinear", nvert, 0, 0, mesh);
        add_oneD_matset_field(mesh, "mesh", nvert-1);

        test_save_mesh_helper(mesh, "oneD_struct_orig");

        Node info;
        EXPECT_TRUE(blueprint::mesh::can_generate_strip(mesh, "mesh", info));
        CONDUIT_INFO(info.to_yaml());

        blueprint::mesh::generate_strip(mesh, "mesh", "mesh_strip");

        test_save_mesh_helper(mesh, "oneD_struct_strip");

        info.reset();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        CONDUIT_INFO(info.to_yaml());
    }

    {
        Node mesh;
        blueprint::mesh::examples::basic("uniform", nvert, 0, 0, mesh);
        add_oneD_matset_field(mesh, "mesh", nvert-1);

        test_save_mesh_helper(mesh, "oneD_unif_orig");

        Node info;
        EXPECT_TRUE(blueprint::mesh::can_generate_strip(mesh, "mesh", info));
        CONDUIT_INFO(info.to_yaml());

        blueprint::mesh::generate_strip(mesh, "mesh", "mesh_strip");

        test_save_mesh_helper(mesh, "oneD_unif_strip");

        info.reset();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        CONDUIT_INFO(info.to_yaml());
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, oneDtostrip_fullfeatured)
{
    const int nvert = 5;
    {
        Node mesh;
        blueprint::mesh::examples::basic("rectilinear", nvert, 0, 0, mesh);
        add_oneD_matset_field(mesh, "mesh", nvert-1);

        test_save_mesh_helper(mesh, "oneD_struct_orig_feature");

        Node info;
        EXPECT_TRUE(blueprint::mesh::can_generate_strip(mesh, "mesh", info));
        CONDUIT_INFO(info.to_yaml());

        conduit::Node options;
        options["field_prefix"] = "strip_";
        blueprint::mesh::generate_strip(mesh["topologies/mesh"],
            mesh["topologies/strip_mesh"],
            mesh["coordsets/strip_coords"],
            mesh["fields"],
            options);

        test_save_mesh_helper(mesh, "oneD_struct_strip_feature");

        info.reset();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        CONDUIT_INFO(info.to_yaml());
    }

    {
        Node mesh;
        blueprint::mesh::examples::basic("uniform", nvert, 0, 0, mesh);
        add_oneD_matset_field(mesh, "mesh", nvert-1);

        test_save_mesh_helper(mesh, "oneD_unif_orig_feature");

        Node info;
        EXPECT_TRUE(blueprint::mesh::can_generate_strip(mesh, "mesh", info));
        CONDUIT_INFO(info.to_yaml());

        conduit::Node options;
        options["field_prefix"] = "strip_";
        blueprint::mesh::generate_strip(mesh["topologies/mesh"],
            mesh["topologies/strip_mesh"],
            mesh["coordsets/strip_coords"],
            mesh["fields"],
            options);

        test_save_mesh_helper(mesh, "oneD_unif_strip_feature");

        info.reset();
        EXPECT_TRUE(blueprint::mesh::verify(mesh, info));
        CONDUIT_INFO(info.to_yaml());
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, rz_cylinder)
{
    index_t nz = 10;
    index_t nr = 15;

    Node res, info;

    std::string mesh_type = "uniform";
    std::string ofbase = conduit_fmt::format("rz_cylinder_{:s}_z_{:d}_r_{:d}",mesh_type,nz,nr);
    conduit::blueprint::mesh::examples::rz_cylinder(mesh_type,nz,nr,res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    std::cout << "verify: " << ofbase << std::endl;
    info.print();
    test_save_mesh_helper(res,ofbase);

    mesh_type = "rectilinear";
    ofbase = conduit_fmt::format("rz_cylinder_{:s}_z_{:d}_r_{:d}",mesh_type,nz,nr);
    conduit::blueprint::mesh::examples::rz_cylinder(mesh_type,nz,nr,res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    std::cout << "verify: " << ofbase << std::endl;
    info.print();
    test_save_mesh_helper(res,ofbase);

    mesh_type = "structured";
    ofbase = conduit_fmt::format("rz_cylinder_{:s}_z_{:d}_r_{:d}",mesh_type,nz,nr);
    conduit::blueprint::mesh::examples::rz_cylinder(mesh_type,nz,nr,res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    std::cout << "verify: " << ofbase << std::endl;
    info.print();
    test_save_mesh_helper(res,ofbase);

    mesh_type = "unstructured";
    ofbase = conduit_fmt::format("rz_cylinder_{:s}_z_{:d}_r_{:d}",mesh_type,nz,nr);
    conduit::blueprint::mesh::examples::rz_cylinder(mesh_type,nz,nr,res);
    EXPECT_TRUE(blueprint::mesh::verify(res, info));
    std::cout << "verify: " << ofbase << std::endl;
    info.print();
    test_save_mesh_helper(res,ofbase);

    // bad args tests
    EXPECT_THROW( conduit::blueprint::mesh::examples::rz_cylinder("garbage",
                                                                  nz,
                                                                  nr,
                                                                  res),
                  conduit::Error);

    EXPECT_THROW( conduit::blueprint::mesh::examples::rz_cylinder("uniform",
                                                                  0,
                                                                  0,
                                                                  res),
                  conduit::Error);

    EXPECT_THROW( conduit::blueprint::mesh::examples::rz_cylinder("uniform",
                                                                  -1,
                                                                  10,
                                                                  res),
                  conduit::Error);

}
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);

    // allow override of the data size via the command line
    if(argc == 2)
    {
        OUTPUT_NUM_AXIS_POINTS = atoi(argv[1]);
    }

    result = RUN_ALL_TESTS();
    return result;
}


