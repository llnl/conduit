// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mpi_mesh_polytopal.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_relay_io.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mpi.hpp"
#include "conduit_relay_mpi_io_blueprint.hpp"
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;

/// Transform Tests ///

// These tests do a polytopal transformation of structured AMR meshes into
// a polygonal 2D mesh or a polyhedral 3D mesh.  The 2D mesh has two domains
// and looks like this, with domain 0 being a coarse domain and domain 1
// being a fine domain refined by a ratio of 2 in each dimension
//
//  _____________________________________________
//  |    |    |    |    |__|__|__|__|__|__|__|__|
//  |____|____|____|____|__|__|__|__|__|__|__|__|
//  |    |    |    |    |__|__|__|__|__|__|__|__|
//  |____|____|____|____|__|__|__|__|__|__|__|__|
//  |    |    |    |    |__|__|__|__|__|__|__|__|
//  |____|____|____|____|__|__|__|__|__|__|__|__|
//  |    |    |    |    |__|__|__|__|__|__|__|__|
//  |____|____|____|____|__|__|__|__|__|__|__|__|
//
//        domain 0              domain 1
//
//  The 3D version of the mesh is like the above but also extended to a width
//  of 4 coarse zones in in the k dimension.

void test_polytopal_create_coarse_domain_2d(Node& domain)
{
    domain["state/domain_id"] = 0;
    domain["state/level_id"] = 0;

    std::vector<double> xcoords;
    for (int i = 0; i < 5; ++i)
    {
        xcoords.push_back(0.0);
        xcoords.push_back(0.25);
        xcoords.push_back(0.5);
        xcoords.push_back(0.75);
        xcoords.push_back(1.0);
    }
    std::vector<double> ycoords;
    double yval = 0.0;
    for (int i = 0; i < 5; ++i)
    {
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        yval += 0.25;
    }

    Node& coords = domain["coordsets/coords"];
    coords["type"] = "explicit";
    coords["values/x"].set(xcoords);
    coords["values/y"].set(ycoords);


    Node& topo = domain["topologies/topo"];
    topo["coordset"] = "coords";
    topo["type"] = "structured";

    topo["elements/origin/i0"] = 0;
    topo["elements/origin/j0"] = 0;
    topo["elements/dims/i"] = 4;
    topo["elements/dims/j"] = 4;

    // Set up an element-centered field
    Node& elt_field = domain["fields/elt_field"];
    elt_field["association"] = "element";
    elt_field["topology"] = "topo";

    std::vector<double> vals(16, 0.5);
    elt_field["values"].set(vals);

    // Set up vertex fields xpos and ypos which will hold the x and y
    // coordinate positions.  These are redundant to the coordinate positions
    // held in the coordset, and will be used to test that the transformation
    // of vertex fields matches the transformation of coordsets.
    Node& xpos = domain["fields/xpos"];
    xpos["association"] = "vertex";
    xpos["topology"] = "topo";
    xpos["values"].set(xcoords);

    Node& ypos = domain["fields/ypos"];
    ypos["association"] = "vertex";
    ypos["topology"] = "topo";
    ypos["values"].set(ycoords);

    Node& adjset = domain["adjsets/adjset"];
    adjset["association"] =  "vertex";
    adjset["topology"] =  "topo";

    Node& group = adjset["groups/group_000001"];
    std::vector<int> nbrs(2);
    nbrs[0] = 0;
    nbrs[1] = 1;
    group["neighbors"].set(nbrs);
    group["rank"] = 1;

    group["windows/window_000000/level_id"] = 0;
    group["windows/window_000000/origin/i"] = 4;
    group["windows/window_000000/origin/j"] = 0;
    group["windows/window_000000/dims/i"] = 1;
    group["windows/window_000000/dims/j"] = 5;
    group["windows/window_000000/ratio/i"] = 2;
    group["windows/window_000000/ratio/j"] = 2;
    group["windows/window_000001/level_id"] = 1;
    group["windows/window_000001/origin/i"] = 8;
    group["windows/window_000001/origin/j"] = 0;
    group["windows/window_000001/dims/i"] = 1;
    group["windows/window_000001/dims/j"] = 9;
    group["windows/window_000001/ratio/i"] = 2;
    group["windows/window_000001/ratio/j"] = 2;
}

void test_polytopal_create_coarse_domain_3d(Node& domain)
{
    domain["state/domain_id"] = 0;
    domain["state/level_id"] = 0;

    std::vector<double> xcoords;
    for (int k = 0; k < 5; ++k)
    {
        for (int j = 0; j < 5; ++j)
        {
            xcoords.push_back(0.0);
            xcoords.push_back(0.25);
            xcoords.push_back(0.5);
            xcoords.push_back(0.75);
            xcoords.push_back(1.0);
        }
    }

    std::vector<double> ycoords;
    double yval = 0.0;
    for (int k = 0; k < 5; ++k)
    {
        for (int j = 0; j < 5; ++j)
        {
            for (int i = 0; i < 5; ++i)
            {
                ycoords.push_back(yval);
            }
            yval += 0.25;
        }
    }

    std::vector<double> zcoords;
    double zval = 0.0;
    for (int k = 0; k < 5; ++k)
    {
        for (int j = 0; j < 5; ++j)
        {
            for (int i = 0; i < 5; ++i)
            {
                zcoords.push_back(zval);
            }
        }
        zval += 0.25;
    }

    Node& coords = domain["coordsets/coords"];
    coords["type"] = "explicit";
    coords["values/x"].set(xcoords);
    coords["values/y"].set(ycoords);
    coords["values/z"].set(zcoords);


    Node& topo = domain["topologies/topo"];
    topo["coordset"] = "coords";
    topo["type"] = "structured";

    topo["elements/origin/i0"] = 0;
    topo["elements/origin/j0"] = 0;
    topo["elements/origin/k0"] = 0;
    topo["elements/dims/i"] = 4;
    topo["elements/dims/j"] = 4;
    topo["elements/dims/k"] = 4;

    //Set up an element-centered field
    Node& elt_field = domain["fields/elt_field"];
    elt_field["association"] = "element";
    elt_field["topology"] = "topo";

    std::vector<double> vals(64, 0.5);
    elt_field["values"].set(vals);

    Node& adjset = domain["adjsets/adjset"];
    adjset["association"] =  "vertex";
    adjset["topology"] =  "topo";

    Node& group = adjset["groups/group_000001"];
    std::vector<int> nbrs(2);
    nbrs[0] = 0;
    nbrs[1] = 1;
    group["neighbors"].set(nbrs);
    group["rank"] = 1;

    group["windows/window_000000/level_id"] = 0;
    group["windows/window_000000/origin/i"] = 4;
    group["windows/window_000000/origin/j"] = 0;
    group["windows/window_000000/origin/k"] = 0;
    group["windows/window_000000/dims/i"] = 1;
    group["windows/window_000000/dims/j"] = 5;
    group["windows/window_000000/dims/k"] = 5;
    group["windows/window_000000/ratio/i"] = 2;
    group["windows/window_000000/ratio/j"] = 2;
    group["windows/window_000000/ratio/k"] = 2;
    group["windows/window_000001/level_id"] = 1;
    group["windows/window_000001/origin/i"] = 8;
    group["windows/window_000001/origin/j"] = 0;
    group["windows/window_000001/origin/k"] = 0;
    group["windows/window_000001/dims/i"] = 1;
    group["windows/window_000001/dims/j"] = 9;
    group["windows/window_000001/dims/k"] = 9;
    group["windows/window_000001/ratio/i"] = 2;
    group["windows/window_000001/ratio/j"] = 2;
    group["windows/window_000001/ratio/k"] = 2;
}

void test_polytopal_create_fine_domain_2d(Node& domain)
{
    domain["state/domain_id"] = 1;
    domain["state/level_id"] = 1;

    std::vector<double> xcoords;
    for (int i = 0; i < 9; ++i)
    {
        xcoords.push_back(1.0);
        xcoords.push_back(1.125);
        xcoords.push_back(1.25);
        xcoords.push_back(1.375);
        xcoords.push_back(1.5);
        xcoords.push_back(1.625);
        xcoords.push_back(1.75);
        xcoords.push_back(1.875);
        xcoords.push_back(1.0);
    }
    std::vector<double> ycoords;
    double yval = 0.0;
    for (int i = 0; i < 9; ++i)
    {
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        ycoords.push_back(yval);
        yval += 0.125;
    }

    Node& coords = domain["coordsets/coords"];
    coords["type"] = "explicit";
    coords["values/x"].set(xcoords);
    coords["values/y"].set(ycoords);

    Node& topo = domain["topologies/topo"];
    topo["coordset"] = "coords";
    topo["type"] = "structured";

    topo["elements/origin/i0"] = 8;
    topo["elements/origin/j0"] = 0;
    topo["elements/dims/i"] = 8;
    topo["elements/dims/j"] = 8;

    //Set up an element-centered field
    Node& elt_field = domain["fields/elt_field"];
    elt_field["association"] = "element";
    elt_field["topology"] = "topo";

    std::vector<double> vals(64, 1.5);
    elt_field["values"].set(vals);

    // Set up vertex fields xpos and ypos which will hold the x and y
    // coordinate positions.  These are redundant to the coordinate positions
    // held in the coordset, and will be used to test that the transformation
    // of vertex fields matches the transformation of coordsets.
    Node& xpos = domain["fields/xpos"];
    xpos["association"] = "vertex";
    xpos["topology"] = "topo";
    xpos["values"].set(xcoords);

    Node& ypos = domain["fields/ypos"];
    ypos["association"] = "vertex";
    ypos["topology"] = "topo";
    ypos["values"].set(ycoords);

    Node& adjset = domain["adjsets/adjset"];
    adjset["association"] =  "vertex";
    adjset["topology"] =  "topo";

    Node& group = adjset["groups/group_000000"];
    std::vector<int> nbrs(2);
    nbrs[0] = 1;
    nbrs[1] = 0;
    group["neighbors"].set(nbrs);
    group["rank"] = 0;

    group["windows/window_000001/level_id"] = 1;
    group["windows/window_000001/origin/i"] = 8;
    group["windows/window_000001/origin/j"] = 0;
    group["windows/window_000001/dims/i"] = 1;
    group["windows/window_000001/dims/j"] = 9;
    group["windows/window_000001/ratio/i"] = 2;
    group["windows/window_000001/ratio/j"] = 2;
    group["windows/window_000000/level_id"] = 0;
    group["windows/window_000000/origin/i"] = 4;
    group["windows/window_000000/origin/j"] = 0;
    group["windows/window_000000/dims/i"] = 1;
    group["windows/window_000000/dims/j"] = 5;
    group["windows/window_000000/ratio/i"] = 2;
    group["windows/window_000000/ratio/j"] = 2;
}

void test_polytopal_create_fine_domain_3d(Node& domain)
{
    domain["state/domain_id"] = 1;
    domain["state/level_id"] = 1;

    std::vector<double> xcoords;
    for (int k = 0; k < 9; ++k)
    {
        for (int j = 0; j < 9; ++j)
        {
            xcoords.push_back(1.0);
            xcoords.push_back(1.125);
            xcoords.push_back(1.25);
            xcoords.push_back(1.375);
            xcoords.push_back(1.5);
            xcoords.push_back(1.625);
            xcoords.push_back(1.75);
            xcoords.push_back(1.875);
            xcoords.push_back(1.0);
        }
    }

    std::vector<double> ycoords;
    double yval = 0.0;
    for (int k = 0; k < 9; ++k)
    {
        for (int j = 0; j < 9; ++j)
        {
            for (int i = 0; i < 9; ++i)
            {   
                ycoords.push_back(yval);
            }
            yval += 0.125;
        }
    }

    std::vector<double> zcoords;
    double zval = 0.0;
    for (int k = 0; k < 9; ++k)
    {
        for (int j = 0; j < 9; ++j)
        {
            for (int i = 0; i < 9; ++i)
            {
                zcoords.push_back(zval);
            }
        }
        zval += 0.125;
    }

    Node& coords = domain["coordsets/coords"];
    coords["type"] = "explicit";
    coords["values/x"].set(xcoords);
    coords["values/y"].set(ycoords);
    coords["values/z"].set(zcoords);

    Node& topo = domain["topologies/topo"];
    topo["coordset"] = "coords";
    topo["type"] = "structured";

    topo["elements/origin/i0"] = 8;
    topo["elements/origin/j0"] = 0;
    topo["elements/origin/k0"] = 0;
    topo["elements/dims/i"] = 8;
    topo["elements/dims/j"] = 8;
    topo["elements/dims/k"] = 8;

    //Set up an element-centered field
    Node& elt_field = domain["fields/elt_field"];
    elt_field["association"] = "element";
    elt_field["topology"] = "topo";

    std::vector<double> vals(512, 1.5);
    elt_field["values"].set(vals);

    Node& adjset = domain["adjsets/adjset"];
    adjset["association"] =  "vertex";
    adjset["topology"] =  "topo";

    Node& group = adjset["groups/group_000000"];
    std::vector<int> nbrs(2);
    nbrs[0] = 1;
    nbrs[1] = 0;
    group["neighbors"].set(nbrs);
    group["rank"] = 0;

    group["windows/window_000001/level_id"] = 1;
    group["windows/window_000001/origin/i"] = 8;
    group["windows/window_000001/origin/j"] = 0;
    group["windows/window_000001/origin/k"] = 0;
    group["windows/window_000001/dims/i"] = 1;
    group["windows/window_000001/dims/j"] = 9;
    group["windows/window_000001/dims/k"] = 9;
    group["windows/window_000001/ratio/i"] = 2;
    group["windows/window_000001/ratio/j"] = 2;
    group["windows/window_000001/ratio/k"] = 2;
    group["windows/window_000000/level_id"] = 0;
    group["windows/window_000000/origin/i"] = 4;
    group["windows/window_000000/origin/j"] = 0;
    group["windows/window_000000/origin/k"] = 0;
    group["windows/window_000000/dims/i"] = 1;
    group["windows/window_000000/dims/j"] = 5;
    group["windows/window_000000/dims/k"] = 5;
    group["windows/window_000000/ratio/i"] = 2;
    group["windows/window_000000/ratio/j"] = 2;
    group["windows/window_000000/ratio/k"] = 2;
}

//-----------------------------------------------------------------------------
bool
check_if_hdf5_enabled()
{
    Node io_protos;
    conduit::relay::io::about(io_protos["io"]);
    return io_protos["io/protocols/hdf5"].as_string() == "enabled";
}


void test_verify_topologies_2d(Node& struct_topo, Node& poly_topo)
{
    Node info;
    EXPECT_TRUE(conduit::blueprint::mesh::topology::unstructured::verify(
        poly_topo,info));
    EXPECT_EQ(poly_topo["elements/shape"].as_string(), "polygonal");

    int num_elems_structured =
        struct_topo["elements/dims/i"].as_int() *
        struct_topo["elements/dims/j"].as_int();
    int num_sizes = poly_topo["elements/sizes"].dtype().number_of_elements();       
    int num_offsets = poly_topo["elements/offsets"].dtype().number_of_elements();
    EXPECT_EQ(num_elems_structured, num_sizes);
    EXPECT_EQ(num_elems_structured, num_offsets);
}

void test_verify_topologies_3d(Node& struct_topo, Node& poly_topo)
{
    Node info;
    EXPECT_TRUE(conduit::blueprint::mesh::topology::unstructured::verify(
        poly_topo,info));
    EXPECT_EQ(poly_topo["elements/shape"].as_string(), "polyhedral");

    int num_elems_structured =
        struct_topo["elements/dims/i"].as_int() *
        struct_topo["elements/dims/j"].as_int() *
        struct_topo["elements/dims/k"].as_int();

    int num_sizes = poly_topo["elements/sizes"].dtype().number_of_elements();       
    int num_offsets = poly_topo["elements/offsets"].dtype().number_of_elements();
    EXPECT_EQ(num_elems_structured, num_sizes);
    EXPECT_EQ(num_elems_structured, num_offsets);
}

void test_verify_fields_2d(Node& poly_topo)
{
    Node info;
    EXPECT_TRUE(conduit::blueprint::mesh::field::verify(
       poly_topo["fields/elt_field"],info));
    EXPECT_TRUE(conduit::blueprint::mesh::field::verify(
       poly_topo["fields/xpos"],info));
    EXPECT_TRUE(conduit::blueprint::mesh::field::verify(
       poly_topo["fields/ypos"],info));
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, amr_2d_transform_serial)
{
    int par_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }

    Node mesh, info;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_coarse_domain_2d(domain_0);
        test_polytopal_create_fine_domain_2d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_to_polygonal_amr_2d_transform_serial_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);


    Node poly;
    conduit::blueprint::mpi::mesh::to_polygonal(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);

        mesh_topo = mesh["domain_000001/topologies/topo"];
        poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);

        Node& domain_0 = poly["domain_000000"];
        Node& domain_1 = poly["domain_000001"];
        test_verify_fields_2d(domain_0);
        test_verify_fields_2d(domain_1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, amr_3d_transform_serial)
{
    int par_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }

    Node mesh, info;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_coarse_domain_3d(domain_0);
        test_polytopal_create_fine_domain_3d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_to_polyhedral_amr_3d_transform_serial_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);


    Node poly;
    conduit::blueprint::mpi::mesh::to_polyhedral(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);

        mesh_topo = mesh["domain_000001/topologies/topo"];
        poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, amr_2d_transform_parallel)
{
    Node mesh, info;

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }


    if (par_size == 1) return;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        test_polytopal_create_coarse_domain_2d(domain_0);
    }
    else if (par_rank == 1)
    {
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_fine_domain_2d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_to_polygonal_amr_2d_transform_parallel_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    Node poly;

    conduit::blueprint::mpi::mesh::to_polygonal(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);

        Node& domain_0 = poly["domain_000000"];
        test_verify_fields_2d(domain_0);
    }
    else if (par_rank == 1)
    {
        Node& mesh_topo = mesh["domain_000001/topologies/topo"];
        Node& poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);

        Node& domain_1 = poly["domain_000001"];
        test_verify_fields_2d(domain_1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, amr_3d_transform_parallel)
{
    Node mesh, info;

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }


    if (par_size == 1) return;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        test_polytopal_create_coarse_domain_3d(domain_0);
    }
    else if (par_rank == 1)
    {
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_fine_domain_3d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_to_polyhedral_amr_3d_transform_parallel_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    Node poly;

    conduit::blueprint::mpi::mesh::to_polyhedral(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);
    }
    else if (par_rank == 1)
    {
        Node& mesh_topo = mesh["domain_000001/topologies/topo"];
        Node& poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, to_polytopal_amr_2d_transform_parallel)
{
    Node mesh, info;

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }

    if (par_size == 1) return;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        test_polytopal_create_coarse_domain_2d(domain_0);
    }
    else if (par_rank == 1)
    {
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_fine_domain_2d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_polytopal_amr_2d_transform_parallel_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    Node poly;
    conduit::blueprint::mpi::mesh::to_polytopal(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);
    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);
    }
    else if (par_rank == 1)
    {
        Node& mesh_topo = mesh["domain_000001/topologies/topo"];
        Node& poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_2d(mesh_topo, poly_topo);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_polytopal, to_polytopal_amr_3d_transform_parallel)
{
    Node mesh, info;

    int par_rank;
    int par_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &par_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &par_size);

    // for file output prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }

    if (par_size == 1) return;

    if (par_rank == 0)
    {
        Node& domain_0 = mesh["domain_000000"];
        test_polytopal_create_coarse_domain_3d(domain_0);
    }
    else if (par_rank == 1)
    {
        Node& domain_1 = mesh["domain_000001"];
        test_polytopal_create_fine_domain_3d(domain_1);
    }

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",mesh,info, MPI_COMM_WORLD));

    std::string output_base = "tout_to_polytopal_amr_3d_transform_parallel_";

    conduit::relay::mpi::io::blueprint::save_mesh(mesh,
                                                  output_base + "input",
                                                  protocol,
                                                  MPI_COMM_WORLD);

    Node poly;
    conduit::blueprint::mpi::mesh::to_polytopal(mesh, poly, "topo", MPI_COMM_WORLD);

    EXPECT_TRUE( conduit::blueprint::mpi::verify("mesh",poly,info, MPI_COMM_WORLD));

    conduit::relay::mpi::io::blueprint::save_mesh(poly,
                                                  output_base + "result",
                                                  protocol,
                                                  MPI_COMM_WORLD);
    if (par_rank == 0)
    {
        Node& mesh_topo = mesh["domain_000000/topologies/topo"];
        Node& poly_topo = poly["domain_000000/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);
    }
    else if (par_rank == 1)
    {
        Node& mesh_topo = mesh["domain_000001/topologies/topo"];
        Node& poly_topo = poly["domain_000001/topologies/topo"];
        test_verify_topologies_3d(mesh_topo, poly_topo);
    }
}


int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}
