// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_utils.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include "blueprint_test_helpers.hpp"

#include <cstddef>
#include <algorithm>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
using namespace generate;

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
    conduit::relay::io::blueprint::save_mesh(root, filebase, protocol);
#else
    std::cout << "Skip writing " << filebase << std::endl;
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, topology_length)
{
    // Test with a strided structured topo. the topology::length function used
    // to crash with it.
    conduit::Node n;
    conduit::Node &coords = n["coordsets/coarse_coords"];
    coords["type"] = "explicit";
    coords["values/x"] = std::vector<double>{
        0., 1.,
        0., 1.,
        0., 1.,
        0., 1.,
        0., 1.,
        0., 1.,
        0., 1.};
    coords["values/y"] = std::vector<double>{
        0., 0.,
        1., 1.,
        2., 2.,
        3., 3.,
        4., 4.,
        5., 5.,
        6., 6.};

    conduit::Node &topo = n["topologies/coarse"];
    topo["type"] = "structured";
    topo["coordset"] = "coarse_coords";
    topo["elements/dims/i"] = 1;
    topo["elements/dims/j"] = 6;
    topo["elements/dims/k"] = 0;
    topo["elements/dims/offsets"] = std::vector<int>{2,2};
    topo["elements/dims/strides"] = std::vector<int>{1,5};

    conduit::Node info;
    bool ok = conduit::blueprint::mesh::verify(n, info);
    if(!ok)
        info.print();
    EXPECT_TRUE(ok);

    conduit::index_t len = conduit::blueprint::mesh::topology::length(topo);
    EXPECT_EQ(len, 6);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, shapetype)
{
    struct test_case
    {
        std::string meshtype;
        std::string shapetype;
        int sdim;
        // answers
        int tdim;
        bool is_poly, is_polygonal, is_polyhedral, is_mixed;
    };
    std::vector<test_case> tests{
        // non-unstructured topo types.
        {"points", "point", 2, 0, false, false, false, false},
        {"points", "point", 3, 0, false, false, false, false},
        {"points_implicit", "point", 2, 0, false, false, false, false},
        {"points_implicit", "point", 3, 0, false, false, false, false},

        {"uniform", "line", 1, 1, false, false, false, false},
        {"uniform", "quad", 2, 2, false, false, false, false},
        {"uniform", "hex", 3, 3, false, false, false, false},

        {"rectilinear", "line", 1, 1, false, false, false, false},
        {"rectilinear", "quad", 2, 2, false, false, false, false},
        {"rectilinear", "hex", 3, 3, false, false, false, false},

        {"structured", "quad", 2, 2, false, false, false, false},
        {"structured", "hex", 3, 3, false, false, false, false},

        // unstructured types
        {"lines", "line", 2, 1, false, false, false, false},
        {"lines", "line", 3, 1, false, false, false, false},

        {"tris", "tri", 2, 2, false, false, false, false},

        {"quads", "quad", 2, 2, false, false, false, false},
        {"quads_poly", "polygonal", 2, 2, true, true, false, false},

        {"mixed_2d", "mixed", 2, 2, false, false, false, true},

        {"tets", "tet", 3, 3, false, false, false, false},

        {"pyramids", "pyramid", 3, 3, false, false, false, false},
        {"wedges", "wedge", 3, 3, false, false, false, false},

        {"hexs", "hex", 3, 3, false, false, false, false},
        {"hexs_poly", "polyhedral", 3, 3, true, false, true, false},

        {"mixed", "mixed", 3, 3, false, false, false, true}
    };

    for(const auto &t : tests)
    {
        const conduit::index_t s = 5;
        conduit::Node mesh;
        conduit::blueprint::mesh::examples::braid(t.meshtype,
            s, t.sdim >= 2 ? s : 0, t.sdim >= 3 ? s : 0,
            mesh);

        // Make sure we can pass various topology types to ShapeType.
        const conduit::Node &topo = mesh["topologies/mesh"];
        conduit::blueprint::mesh::utils::ShapeType shape(topo);

        EXPECT_EQ(conduit::blueprint::mesh::coordset::dims(mesh["coordsets/coords"]), t.sdim);
        EXPECT_EQ(shape.type(), t.shapetype);
        EXPECT_EQ(shape.dim, t.tdim);
        EXPECT_EQ(shape.is_poly(), t.is_poly);
        EXPECT_EQ(shape.is_polygonal(), t.is_polygonal);
        EXPECT_EQ(shape.is_polyhedral(), t.is_polyhedral);
        EXPECT_EQ(shape.is_mixed(), t.is_mixed);
    }

    // Initialize with a bad shape name.
    conduit::blueprint::mesh::utils::ShapeType shape("bogus");
    EXPECT_FALSE(shape.is_poly());
    EXPECT_FALSE(shape.is_polygonal());
    EXPECT_FALSE(shape.is_polyhedral());
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_validate_element_0d)
{
    conduit::Node root, info;
    create_2_domain_0d_mesh(root, 0, 1);
    save_mesh(root, "adjset_validate_element_0d");
    bool res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_0d_bad");
    res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //info.print();

    EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
    const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
    EXPECT_EQ(n0.number_of_children(), 1);
    const conduit::Node &c0 = n0[0];
    EXPECT_TRUE(c0.has_path("element"));
    EXPECT_TRUE(c0.has_path("neighbor"));
    EXPECT_EQ(c0["element"].to_int(), 0);
    EXPECT_EQ(c0["neighbor"].to_int(), 1);

    EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
    const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
    EXPECT_EQ(n1.number_of_children(), 1);
    const conduit::Node &c1 = n1[0];
    EXPECT_TRUE(c1.has_path("element"));
    EXPECT_TRUE(c1.has_path("neighbor"));
    EXPECT_EQ(c1["element"].to_int(), 2);
    EXPECT_EQ(c1["neighbor"].to_int(), 0);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_validate_element_1d)
{
    conduit::Node root, info;
    create_2_domain_1d_mesh(root, 0, 1);
    save_mesh(root, "adjset_validate_element_1d");
    bool res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{1});
    info.reset();
    save_mesh(root, "adjset_validate_element_1d_bad");  
    res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //info.print();

    EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
    const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
    EXPECT_EQ(n0.number_of_children(), 1);
    const conduit::Node &c0 = n0[0];
    EXPECT_TRUE(c0.has_path("element"));
    EXPECT_TRUE(c0.has_path("neighbor"));
    EXPECT_EQ(c0["element"].to_int(), 0);
    EXPECT_EQ(c0["neighbor"].to_int(), 1);

    EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
    const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
    EXPECT_EQ(n1.number_of_children(), 1);
    const conduit::Node &c1 = n1[0];
    EXPECT_TRUE(c1.has_path("element"));
    EXPECT_TRUE(c1.has_path("neighbor"));
    EXPECT_EQ(c1["element"].to_int(), 1);
    EXPECT_EQ(c1["neighbor"].to_int(), 0);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_validate_element_2d)
{
    conduit::Node root, info;
    create_2_domain_2d_mesh(root, 0, 1);
    save_mesh(root, "adjset_validate_element_2d");
    bool res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_TRUE(res);
    //info.print();

    // Now, adjust the adjset for domain1 so it includes an element not present in domain 0
    root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0,2,4});
    info.reset();
    save_mesh(root, "adjset_validate_element_2d_bad");
    res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //info.print();

    EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
    const conduit::Node &n = info["domain1/main_adjset/domain0_1"];
    EXPECT_EQ(n.number_of_children(), 1);
    const conduit::Node &c = n[0];
    EXPECT_TRUE(c.has_path("element"));
    EXPECT_TRUE(c.has_path("neighbor"));

    EXPECT_EQ(c["element"].to_int(), 2);
    EXPECT_EQ(c["neighbor"].to_int(), 0);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_validate_element_3d)
{
    conduit::Node root, info;
    create_2_domain_3d_mesh(root, 0, 1);
    save_mesh(root, "adjset_validate_element_3d");
    bool res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    root["domain0/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{0});
    root["domain1/adjsets/main_adjset/groups/domain0_1/values"].set(std::vector<int>{2});
    info.reset();
    save_mesh(root, "adjset_validate_element_3d_bad");
    res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_FALSE(res);
    //info.print();

    EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
    const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
    EXPECT_EQ(n0.number_of_children(), 1);
    const conduit::Node &c0 = n0[0];
    EXPECT_TRUE(c0.has_path("element"));
    EXPECT_TRUE(c0.has_path("neighbor"));
    EXPECT_EQ(c0["element"].to_int(), 0);
    EXPECT_EQ(c0["neighbor"].to_int(), 1);

    EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
    const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
    EXPECT_EQ(n1.number_of_children(), 1);
    const conduit::Node &c1 = n1[0];
    EXPECT_TRUE(c1.has_path("element"));
    EXPECT_TRUE(c1.has_path("neighbor"));
    EXPECT_EQ(c1["element"].to_int(), 2);
    EXPECT_EQ(c1["neighbor"].to_int(), 0);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_validate_vertex_3d)
{
    conduit::Node root, info;
    create_2_domain_3d_mesh(root, 0, 1);
    // Add adjsets
    conduit::Node &d0_adjset = root["domain0/adjsets/main_adjset"];
    d0_adjset["association"] = "vertex";
    d0_adjset["topology"] = "main";
    conduit::Node &d0_01 = d0_adjset["groups/domain0_1"];
    d0_01["neighbors"] = 1;
    d0_01["values"].set(std::vector<int>{1,2,3,5,6,7,9,10,11,13,14,15});

    conduit::Node &d1_adjset = root["domain1/adjsets/main_adjset"];
    d1_adjset["association"] = "vertex";
    d1_adjset["topology"] = "main";
    conduit::Node &d1_01 = d1_adjset["groups/domain0_1"];
    d1_01["neighbors"] = 0;
    d1_01["values"].set(std::vector<int>{0,1,2,4,5,6,8,9,10,12,13,14});

    EXPECT_TRUE(conduit::blueprint::mesh::adjset::verify(d0_adjset, info));
    info.reset();
    EXPECT_TRUE(conduit::blueprint::mesh::adjset::verify(d1_adjset, info));

    save_mesh(root, "adjset_validate_vertex_3d");
    bool res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    EXPECT_TRUE(res);

    // Now, adjust the adjsets so they are wrong on both domains.
    d0_01["values"].set(std::vector<int>{1,2,3,5,6,7,9,10,11,13,14,15,/*wrong*/0,4,8,12});
    d1_01["values"].set(std::vector<int>{0,1,2,4,5,6,8,9,10,12,13,14,/*wrong*/3,7,11,15});
    info.reset();
    save_mesh(root, "adjset_validate_vertex_3d_bad");
    res = conduit::blueprint::mesh::utils::adjset::validate(root, "main_adjset", info);
    //info.print();
    EXPECT_FALSE(res);

    EXPECT_TRUE(info.has_path("domain0/main_adjset/domain0_1"));
    const conduit::Node &n0 = info["domain0/main_adjset/domain0_1"];
    EXPECT_EQ(n0.number_of_children(), 4);
    const std::vector<int> d0err_vertex{0,4,8,12};
    for(conduit::index_t i = 0; i < 4; i++)
    {
        EXPECT_EQ(n0[i]["neighbor"].to_int(), 1);
        EXPECT_TRUE(std::find(d0err_vertex.begin(), d0err_vertex.end(), n0[i]["vertex"].to_int()) != d0err_vertex.end());
    }

    EXPECT_TRUE(info.has_path("domain1/main_adjset/domain0_1"));
    const conduit::Node &n1 = info["domain1/main_adjset/domain0_1"];
    EXPECT_EQ(n1.number_of_children(), 4);
    const std::vector<int> d1err_vertex{3,7,11,15};
    for(conduit::index_t i = 0; i < 4; i++)
    {
        EXPECT_EQ(n1[i]["neighbor"].to_int(), 0);
        EXPECT_TRUE(std::find(d1err_vertex.begin(), d1err_vertex.end(), n1[i]["vertex"].to_int()) != d1err_vertex.end());
    }
}

//-----------------------------------------------------------------------------
template <typename CoordType, typename ConnType>
void test_rewrite_connectivity(int dims)
{
    conduit::Node n;
    n["coordsets/coords1/type"] = "explicit";
    n["coordsets/coords1/values/x"].set(std::vector<CoordType>{0.f,1.f,2.f,0.f,1.f,2.f,0.f,1.f,2.f});
    n["coordsets/coords1/values/y"].set(std::vector<CoordType>{0.f,0.f,0.f,1.f,1.f,1.f,2.f,2.f,2.f});
    if(dims > 2)
        n["coordsets/coords1/values/z"].set(std::vector<CoordType>{5.f,5.f,5.f,5.f,5.f,5.f,5.f,5.f,5.f});
    n["topologies/mesh1/type"] = "unstructured";
    n["topologies/mesh1/coordset"] = "coords1";
    n["topologies/mesh1/elements/shape"] = "quad";
    n["topologies/mesh1/elements/connectivity"].set(std::vector<ConnType>{0,1,4,3, 1,2,5,4, 3,4,7,6, 4,5,8,7});
    n["topologies/mesh1/elements/sizes"].set(std::vector<ConnType>{4,4,4,4});
    n["topologies/mesh1/elements/offsets"].set(std::vector<ConnType>{0,4,8,12});

    n["coordsets/coords2/type"] = "explicit";
    n["coordsets/coords2/values/x"].set(std::vector<CoordType>{1.f,2.f,1.f,2.f});
    n["coordsets/coords2/values/y"].set(std::vector<CoordType>{1.f,1.f,2.f,2.f});
    if(dims > 2)
        n["coordsets/coords2/values/z"].set(std::vector<CoordType>{5.f,5.f,5.f,5.f});
    n["topologies/mesh2/type"] = "unstructured";
    n["topologies/mesh2/coordset"] = "coords2";
    n["topologies/mesh2/elements/shape"] = "tri";
    n["topologies/mesh2/elements/connectivity"].set(std::vector<ConnType>{0,1,2, 1,3,2});
    n["topologies/mesh2/elements/sizes"].set(std::vector<ConnType>{3,3});
    n["topologies/mesh2/elements/offsets"].set(std::vector<ConnType>{0,3});

    conduit::Node info;
    EXPECT_TRUE(conduit::blueprint::mesh::topology::verify(n["topologies/mesh1"], info));
    EXPECT_TRUE(conduit::blueprint::mesh::topology::verify(n["topologies/mesh2"], info));

    // Make mesh2 use coords1
    conduit::blueprint::mesh::utils::topology::unstructured::rewrite_connectivity(n["topologies/mesh2"],
                                                                                  n["coordsets/coords1"]);

    // Make sure that mesh2's connectivity uses coords1 ids.
    auto conn = n["topologies/mesh2/elements/connectivity"].as_int_accessor();
    EXPECT_EQ(conn[0], 4);
    EXPECT_EQ(conn[1], 5);
    EXPECT_EQ(conn[2], 7);

    EXPECT_EQ(conn[3], 5);
    EXPECT_EQ(conn[4], 8);
    EXPECT_EQ(conn[5], 7);

    EXPECT_EQ(n["topologies/mesh2/coordset"].as_string(), "coords1");
}

void test_rewrite_connectivity_top(int dims)
{
    test_rewrite_connectivity<float, conduit::int32>(dims);
    test_rewrite_connectivity<float, conduit::uint32>(dims);
    test_rewrite_connectivity<float, conduit::int64>(dims);
    test_rewrite_connectivity<float, conduit::uint64>(dims);
    test_rewrite_connectivity<float, conduit::index_t>(dims);

    test_rewrite_connectivity<double, conduit::int32>(dims);
    test_rewrite_connectivity<double, conduit::uint32>(dims);
    test_rewrite_connectivity<double, conduit::int64>(dims);
    test_rewrite_connectivity<double, conduit::uint64>(dims);
    test_rewrite_connectivity<double, conduit::index_t>(dims);
}

TEST(conduit_blueprint_mesh_utils, rewrite_connectivity_2d)
{
    test_rewrite_connectivity_top(2);
}

TEST(conduit_blueprint_mesh_utils, rewrite_connectivity_3d)
{
    test_rewrite_connectivity_top(3);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, copy_fields)
{
    conduit::Node n;
    n["fields/f1/association"] = "vertex";
    n["fields/f1/topology"] = "mesh1";
    n["fields/f1/values"].set(std::vector<double>{0., 1., 2., 3.});
    n["fields/f2/association"] = "vertex";
    n["fields/f2/topology"] = "mesh1";
    n["fields/f2/values"].set(std::vector<double>{4., 5., 6., 7.});
    n["fields/f3/association"] = "vertex";
    n["fields/f3/topology"] = "mesh2";
    n["fields/f3/values"].set(std::vector<double>{8., 9., 10., 11.});
    n["fields/f4/association"] = "vertex";
    n["fields/f4/topology"] = "mesh2";
    n["fields/f4/values"].set(std::vector<double>{12., 13., 14., 15.});

    conduit::Node opts, fields;
    opts["exclusions"].append().set("f2");
    conduit::blueprint::mesh::utils::copy_fields(n["fields"], fields, opts);
    EXPECT_EQ(fields.number_of_children(), 3);
    EXPECT_EQ(fields[0].name(), "f1");
    EXPECT_EQ(fields[1].name(), "f3");
    EXPECT_EQ(fields[2].name(), "f4");

    opts.reset();
    fields.reset();
    opts["topology"] = "mesh2";
    conduit::blueprint::mesh::utils::copy_fields(n["fields"], fields, opts);
    EXPECT_EQ(fields.number_of_children(), 2);
    EXPECT_EQ(fields[0].name(), "f3");
    EXPECT_EQ(fields[1].name(), "f4");

    fields.reset();
    opts["exclusions"].append().set("f3");
    conduit::blueprint::mesh::utils::copy_fields(n["fields"], fields, opts);
    EXPECT_EQ(fields.number_of_children(), 1);
    EXPECT_EQ(fields[0].name(), "f4");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, slice_array)
{
    struct A
    {
        double x,y,z;
        double f1,f2;
    };
    A data[] = {{0., 1., 2., 3., 4.},
                {5., 6., 7., 8., 9.},
                {10., 11., 12., 13., 14.},
                {15., 16., 17., 18., 19.},
                {20., 21., 22., 23., 24.}};
    index_t nelem = 5;
    Node n;
    n["fields/vector/values/x"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, x), sizeof(A));
    n["fields/vector/values/y"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, y), sizeof(A));
    n["fields/vector/values/z"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, z), sizeof(A));
    n["fields/f1/values"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, f1), sizeof(A));
    n["fields/f2/values"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, f2), sizeof(A));

    // First, a basic check.
    auto ax = n["fields/vector/values/x"].as_double_array();
    auto ay = n["fields/vector/values/y"].as_double_array();
    auto az = n["fields/vector/values/z"].as_double_array();
    auto af1 = n["fields/f1/values"].as_double_array();
    auto af2 = n["fields/f2/values"].as_double_array();
    for(index_t i = 0; i < nelem; i++)
    {
        EXPECT_EQ(data[i].x, ax[i]);
        EXPECT_EQ(data[i].y, ay[i]);
        EXPECT_EQ(data[i].z, az[i]);
        EXPECT_EQ(data[i].f1, af1[i]);
        EXPECT_EQ(data[i].f2, af2[i]);
    }

    // Check the compacted values.
    Node ncx, ncy, ncz, ncf1, ncf2;
    n["fields/vector/values/x"].compact_to(ncx);
    n["fields/vector/values/y"].compact_to(ncy);
    n["fields/vector/values/z"].compact_to(ncz);
    n["fields/f1/values"].compact_to(ncf1);
    n["fields/f2/values"].compact_to(ncf2);
    ax = ncx.as_double_array();
    ay = ncy.as_double_array();
    az = ncz.as_double_array();
    af1 = ncf1.as_double_array();
    af2 = ncf2.as_double_array();
    for(index_t i = 0; i < nelem; i++)
    {
        EXPECT_EQ(data[i].x, ax[i]);
        EXPECT_EQ(data[i].y, ay[i]);
        EXPECT_EQ(data[i].z, az[i]);
        EXPECT_EQ(data[i].f1, af1[i]);
        EXPECT_EQ(data[i].f2, af2[i]);
    }

    // Slice the arrays with index_t indices
    std::vector<index_t> idx1{0, 2, 4};
    Node nsx, nsy, nsz, nsf1, nsf2;
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/x"], idx1, nsx);
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/y"], idx1, nsy);
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/z"], idx1, nsz);
    conduit::blueprint::mesh::utils::slice_array(n["fields/f1/values"], idx1, nsf1);
    conduit::blueprint::mesh::utils::slice_array(n["fields/f2/values"], idx1, nsf2);
    ax = nsx.as_double_array();
    ay = nsy.as_double_array();
    az = nsz.as_double_array();
    af1 = nsf1.as_double_array();
    af2 = nsf2.as_double_array();
    EXPECT_EQ(static_cast<int>(af1.number_of_elements()), static_cast<int>(idx1.size()));
    EXPECT_EQ(static_cast<int>(ax.number_of_elements()), static_cast<int>(idx1.size()));
    for(size_t i = 0; i < idx1.size(); i++)
    {
        index_t orig = idx1[i];
        EXPECT_EQ(data[orig].x, ax[i]);
        EXPECT_EQ(data[orig].y, ay[i]);
        EXPECT_EQ(data[orig].z, az[i]);
        EXPECT_EQ(data[orig].f1, af1[i]);
        EXPECT_EQ(data[orig].f2, af2[i]);
    }

    // Slice the arrays with int indices
    std::vector<int> idx2{1, 3};
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/x"], idx2, nsx);
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/y"], idx2, nsy);
    conduit::blueprint::mesh::utils::slice_array(n["fields/vector/values/z"], idx2, nsz);
    conduit::blueprint::mesh::utils::slice_array(n["fields/f1/values"], idx2, nsf1);
    conduit::blueprint::mesh::utils::slice_array(n["fields/f2/values"], idx2, nsf2);
    ax = nsx.as_double_array();
    ay = nsy.as_double_array();
    az = nsz.as_double_array();
    af1 = nsf1.as_double_array();
    af2 = nsf2.as_double_array();
    EXPECT_EQ(static_cast<int>(af1.number_of_elements()), static_cast<int>(idx2.size()));
    EXPECT_EQ(static_cast<int>(ax.number_of_elements()), static_cast<int>(idx2.size()));
    for(index_t i = 0; i < idx2.size(); i++)
    {
        index_t orig = idx2[i];
        EXPECT_EQ(data[orig].x, ax[i]);
        EXPECT_EQ(data[orig].y, ay[i]);
        EXPECT_EQ(data[orig].z, az[i]);
        EXPECT_EQ(data[orig].f1, af1[i]);
        EXPECT_EQ(data[orig].f2, af2[i]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, slice_field)
{
    struct A
    {
        double x,y,z;
        int f1;
        float f2;
    };
    A data[] = {{0., 1., 2., 3, 4.f},
                {5., 6., 7., 8, 9.f},
                {10., 11., 12., 13, 14.f},
                {15., 16., 17., 18, 19.f},
                {20., 21., 22., 23, 24.f}};
    index_t nelem = 5;
    Node n;
    n["fields/vector/association"] = "element";
    n["fields/vector/topology"] = "ignored";
    n["fields/vector/values/x"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, x), sizeof(A));
    n["fields/vector/values/y"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, y), sizeof(A));
    n["fields/vector/values/z"].set_external(reinterpret_cast<double *>(data), nelem, offsetof(A, z), sizeof(A));
    n["fields/f1/association"] = "element";
    n["fields/f1/topology"] = "ignored";
    n["fields/f1/values"].set_external(reinterpret_cast<int *>(data), nelem, offsetof(A, f1), sizeof(A));
    n["fields/f2/association"] = "element";
    n["fields/f2/topology"] = "ignored";
    n["fields/f2/values"].set_external(reinterpret_cast<float *>(data), nelem, offsetof(A, f2), sizeof(A));

    // Slice the arrays with index_t indices
    std::vector<index_t> idx1{0, 2, 4};
    Node nsvector, nsf1, nsf2;
    conduit::blueprint::mesh::utils::slice_field(n["fields/vector/values"], idx1, nsvector["values"]);
    conduit::blueprint::mesh::utils::slice_field(n["fields/f1/values"], idx1, nsf1["values"]);
    conduit::blueprint::mesh::utils::slice_field(n["fields/f2/values"], idx1, nsf2["values"]);
    auto ax = nsvector["values/x"].as_double_array();
    auto ay = nsvector["values/y"].as_double_array();
    auto az = nsvector["values/z"].as_double_array();
    auto af1 = nsf1["values"].as_int_array();
    auto af2 = nsf2["values"].as_float_array();
    EXPECT_EQ(static_cast<int>(af1.number_of_elements()), static_cast<int>(idx1.size()));
    EXPECT_EQ(static_cast<int>(ax.number_of_elements()), static_cast<int>(idx1.size()));
    for(size_t i = 0; i < idx1.size(); i++)
    {
        index_t orig = idx1[i];
        EXPECT_EQ(data[orig].x, ax[i]);
        EXPECT_EQ(data[orig].y, ay[i]);
        EXPECT_EQ(data[orig].z, az[i]);
        EXPECT_EQ(data[orig].f1, af1[i]);
        EXPECT_EQ(data[orig].f2, af2[i]);
    }

    // Slice the arrays with int indices
    std::vector<int> idx2{1, 3};
    conduit::blueprint::mesh::utils::slice_field(n["fields/vector/values"], idx2, nsvector["values"]);
    conduit::blueprint::mesh::utils::slice_field(n["fields/f1/values"], idx2, nsf1["values"]);
    conduit::blueprint::mesh::utils::slice_field(n["fields/f2/values"], idx2, nsf2["values"]);
    auto bx = nsvector["values/x"].as_double_array();
    auto by = nsvector["values/y"].as_double_array();
    auto bz = nsvector["values/z"].as_double_array();
    auto bf1 = nsf1["values"].as_int_array();
    auto bf2 = nsf2["values"].as_float_array();
    EXPECT_EQ(static_cast<int>(bf1.number_of_elements()), static_cast<int>(idx2.size()));
    EXPECT_EQ(static_cast<int>(bx.number_of_elements()), static_cast<int>(idx2.size()));
    for(size_t i = 0; i < idx2.size(); i++)
    {
        index_t orig = idx2[i];
        EXPECT_EQ(data[orig].x, bx[i]);
        EXPECT_EQ(data[orig].y, by[i]);
        EXPECT_EQ(data[orig].z, bz[i]);
        EXPECT_EQ(data[orig].f1, bf1[i]);
        EXPECT_EQ(data[orig].f2, bf2[i]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, adjset_compare_pointwise_2d)
{
    conduit::Node root, info;
    create_2_domain_2d_mesh(root, 0, 1);
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
    bool eq = conduit::blueprint::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", info);
    if(!eq)
        info.print();
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
    eq = conduit::blueprint::mesh::utils::adjset::compare_pointwise(root, "pt_adjset", info);
    if(!eq)
       info.print();
    EXPECT_TRUE(eq);

    // Test that the fails_pointwise adjset actually fails.
    info.reset();
    eq = conduit::blueprint::mesh::utils::adjset::compare_pointwise(root, "fails_pointwise", info);
    //if(!eq)
    //   info.print();
    EXPECT_FALSE(eq);
    EXPECT_TRUE(info.number_of_children() > 0);

    // Test that the notevenclose adjset actually fails.
    info.reset();
    eq = conduit::blueprint::mesh::utils::adjset::compare_pointwise(root, "notevenclose", info);
    //if(!eq)
    //   info.print();
    EXPECT_TRUE(info.number_of_children() > 0);
    EXPECT_FALSE(eq);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, topology_search_2d)
{
    conduit::Node root, info;
    create_2_domain_2d_mesh(root, 0, 1);
    save_mesh(root, "topology_search_2d");
    auto domains = conduit::blueprint::mesh::domains(root);

    // Look for domain 0 in domain 1. They should intersect by 2 zones.
    auto res01 = conduit::blueprint::mesh::utils::topology::search(
                   domains[1]->fetch_existing("topologies/main"),
                   domains[0]->fetch_existing("topologies/main"));
    const std::vector<int> answers01{0,0,1,0,0,0,1};
    EXPECT_EQ(res01.size(), answers01.size());
    for(size_t i = 0; i < res01.size(); i++)
    {
        EXPECT_EQ(res01[i], answers01[i]);
    }

    // Look for domain 1 in domain 0. They should intersect by 2 zones.
    auto res10 = conduit::blueprint::mesh::utils::topology::search(
                   domains[0]->fetch_existing("topologies/main"),
                   domains[1]->fetch_existing("topologies/main"));
    const std::vector<int> answers10{1,0,0,0,1,0};
    EXPECT_EQ(res10.size(), answers10.size());
    for(size_t i = 0; i < res10.size(); i++)
    {
        EXPECT_EQ(res10[i], answers10[i]);
    }

    // Turn the main_adjset adjset into a topology.
    conduit::Node main_adjset_topo;
    conduit::blueprint::mesh::utils::adjset::to_topo(root, "main_adjset", main_adjset_topo);
    //main_adjset_topo.print();

    // Pull domain 0 adjset zones out as a topology and look for them in domain 1.
    const conduit::Node &t01 = main_adjset_topo.fetch_existing("topologies/main_adjset_0_group_0_1");
    EXPECT_EQ(t01["type"].as_string(), "unstructured");
    EXPECT_EQ(t01["elements/shape"].as_string(), "quad");
    EXPECT_EQ(conduit::blueprint::mesh::topology::length(t01), 2);
    auto resT01 = conduit::blueprint::mesh::utils::topology::search(
                      domains[1]->fetch_existing("topologies/main"),
                      t01);
    const std::vector<int> answersT01{1,1};
    EXPECT_EQ(resT01.size(), answersT01.size());
    for(size_t i = 0; i < resT01.size(); i++)
    {
        EXPECT_EQ(resT01[i], answersT01[i]);
    }

    // Pull domain 1 adjset zones out as a topology and look for them in domain 0.
    const conduit::Node &t10 = main_adjset_topo.fetch_existing("topologies/main_adjset_1_group_0_1");
    EXPECT_EQ(t10["type"].as_string(), "unstructured");
    EXPECT_EQ(t10["elements/shape"].as_string(), "quad");
    EXPECT_EQ(conduit::blueprint::mesh::topology::length(t10), 2);
    auto resT10 = conduit::blueprint::mesh::utils::topology::search(
                      domains[0]->fetch_existing("topologies/main"),
                      t10);
    const std::vector<int> answersT10{1,1};
    EXPECT_EQ(resT10.size(), answersT10.size());
    for(size_t i = 0; i < resT10.size(); i++)
    {
        EXPECT_EQ(resT10[i], answersT10[i]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, topology_search_2d_nohit)
{
    // A consists of 3 quads in the 1st column
    // B is a single quad on the right
    // C consists of 4 tris on the lower left.
    //
    // A and B do not overlap.
    // A and C partially overlap but are different types.
    const char *example = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.]
      y: [0.,0.,0.,0.,1.,1.,1.,1.,2.,2.,2.,2.,3.,3.,3.,3.]
topologies:
  A:
    type: unstructured
    coordset: coords
    elements:
      shape: quad
      connectivity: [0,1,5,4,4,5,9,8,8,9,13,12]
      offsets: [0,4,8]
  B:
    type: unstructured
    coordset: coords
    elements:
      shape: quad
      connectivity: [6,7,11,10]
      offsets: [0]
  C:
    type: unstructured
    coordset: coords
    elements:
      shape: tri
      connectivity: [0,1,4,1,5,4,1,2,5,2,6,5]
      offsets: [0,3,6,9]
)";

    conduit::Node n;
    n.parse(example, "yaml");

    // Look for B in A
    auto resBA = conduit::blueprint::mesh::utils::topology::search(
                   n.fetch_existing("topologies/A"),
                   n.fetch_existing("topologies/B"));
    const std::vector<int> answers01{0,0,1,0,0,0,1};
    EXPECT_EQ(resBA.size(), 1);
    EXPECT_EQ(resBA[0], 0);

    // Look for C in A
    auto resCA = conduit::blueprint::mesh::utils::topology::search(
                   n.fetch_existing("topologies/A"),
                   n.fetch_existing("topologies/C"));
    const std::vector<int> answersCA{0,0,0,0};
    EXPECT_EQ(resCA.size(), 4);
    EXPECT_EQ(resCA, answersCA);
};

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, is_canonical)
{
    const char *adjsets = R"(
adjsets:
  correct:
    topology: mesh
    association: vertex
    groups:
      group_0_1:
        neighbors: 1
        values: [0,1,2]
  wrong_prefix:
    topology: mesh
    association: vertex
    groups:
      prefix_0_1:
        neighbors: 1
        values: [0,1,2]
  out_of_order:
    topology: mesh
    association: vertex
    groups:
      group_1_0:
        neighbors: 1
        values: [0,1,2]
  multi:
    topology: mesh
    association: vertex
    groups:
      group_0_1_2:
        neighbors: [1,2]
        values: [0,1,2]
  multi_out_of_order:
    topology: mesh
    association: vertex
    groups:
      group_0_2_1:
        neighbors: [1,2]
        values: [0,1,2]
)";

    conduit::Node n;
    n.parse(adjsets);
    EXPECT_TRUE( conduit::blueprint::mesh::utils::adjset::is_canonical(n["adjsets/correct"]));
    EXPECT_FALSE(conduit::blueprint::mesh::utils::adjset::is_canonical(n["adjsets/wrong_prefix"]));
    EXPECT_FALSE(conduit::blueprint::mesh::utils::adjset::is_canonical(n["adjsets/out_of_order"]));
    EXPECT_TRUE( conduit::blueprint::mesh::utils::adjset::is_canonical(n["adjsets/multi"]));
    EXPECT_FALSE(conduit::blueprint::mesh::utils::adjset::is_canonical(n["adjsets/multi_out_of_order"]));
}

void verify_lerp_result(const conduit::Node& expected, const conduit::Node& res, double eps = 1.e-8);
void verify_lerp_result(const conduit::Node& expected, const conduit::Node& res, double eps)
{
    // 1. Does number of components match?
    ASSERT_EQ(expected.number_of_children(), res.number_of_children());

    // 2. Does length of each result component match length of first expected component?
    const int component_len = expected.child(0).dtype().number_of_elements();
    conduit::NodeConstIterator rescomp = res.children();
    while (rescomp.has_next()) {
        const conduit::Node& comp = rescomp.next();
        EXPECT_EQ(component_len, comp.dtype().number_of_elements());
    }

    // 3. For each component, does each value in res match the corresponding value in expected?
    conduit::NodeConstIterator expcomp = expected.children();
    rescomp = res.children();
    while (expcomp.has_next() && rescomp.has_next()) {
        float64_array rcomp = rescomp.next().as_float64_array();
        float64_array ecomp = expcomp.next().as_float64_array();

        for (int i = 0; i < component_len; ++i)
        {
            EXPECT_NEAR(ecomp[i], rcomp[i], eps);
        }
    }
}

void append_array(conduit::Node& n, std::initializer_list<double> data)
{
    conduit::Node& c = n.append();
    c.set(data.begin(), data.size());
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, lerp_1d)
{
    {
        conduit::Node A; A.set({ 1. });
        conduit::Node B; B.set({ 3. });
        int n = 3;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {1., 2., 3.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 1. });
        conduit::Node B; B.set({ 3. });
        int n = 6;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {1., 1.4, 1.8, 2.2, 2.6, 3.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 2. });
        conduit::Node B; B.set({ 0. });

        conduit::Node res;
        int n = 4;
        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {2., 1.3333333333, 0.6666666666, 0.} );

        verify_lerp_result(expected, res);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, lerp_2d)
{
    {
        conduit::Node A; A.set({ 1., 0. });
        conduit::Node B; B.set({ 3., 4. });
        int n = 3;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {1., 2., 3.} );
        append_array(expected, {0., 2., 4.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 1., 0. });
        conduit::Node B; B.set({ 3., 4. });
        int n = 6;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {1., 1.4, 1.8, 2.2, 2.6, 3.} );
        append_array(expected, {0., 0.8, 1.6, 2.4, 3.2, 4.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 2., 1. });
        conduit::Node B; B.set({ 0., -2. });

        conduit::Node res;
        int n = 4;
        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {2., 1.3333333333, 0.6666666666, 0.} );
        append_array(expected, {1., 0., -1., -2.} );

        verify_lerp_result(expected, res);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, lerp_3d)
{
    {
        conduit::Node A; A.set({ 1., 0., 0. });
        conduit::Node B; B.set({ 3., 4., -1. });
        int n = 3;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {1., 2., 3.} );
        append_array(expected, {0., 2., 4.} );
        append_array(expected, {0., -0.5, -1.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 1., 0., 0. });
        conduit::Node B; B.set({ 3., 4., -1. });
        int n = 6;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, { 1., 1.4, 1.8, 2.2, 2.6, 3. });
        append_array(expected, {0., 0.8, 1.6, 2.4, 3.2, 4.} );
        append_array(expected, {0, -0.2, -0.4, -0.6, -0.8, -1.} );

        verify_lerp_result(expected, res);
    }

    {
        conduit::Node A; A.set({ 2., 1., 1. });
        conduit::Node B; B.set({ 0., -2., 1. });
        int n = 4;
        conduit::Node res;

        conduit::blueprint::mesh::utils::lerp(A, B, n, res);

        conduit::Node expected;
        append_array(expected, {2., 1.3333333333, 0.6666666666, 0.} );
        append_array(expected, {1., 0., -1., -2.} );
        append_array(expected, {1., 1., 1., 1.} );

        verify_lerp_result(expected, res);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, lerp_3d_grid)
{
    {
        conduit::Node A; A.set({ 0., 0., 0. });
        conduit::Node B; B.set({ 2., 0., 0. });
        conduit::Node C; C.set({ 0., 1., 0. });
        conduit::Node D; D.set({ 2., 1., 0. });
        int n = 3;
        conduit::Node leftside;
        conduit::blueprint::mesh::utils::lerp(A, C, n, leftside);
        conduit::Node rightside;
        conduit::blueprint::mesh::utils::lerp(B, D, n, rightside);
        conduit::Node gridpoints;
        int m = 4;
        conduit::blueprint::mesh::utils::lerp(leftside, rightside, m, gridpoints);

        conduit::Node expected;
        append_array(expected, {0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2} );
        append_array(expected, {0, 0, 0, 0, .5, .5, .5, .5, 1, 1, 1, 1} );
        append_array(expected, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} );

        verify_lerp_result(expected, gridpoints);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, lerp_3d_cube)
{
    {
        conduit::Node A; A.set({ 0., 0., 0. });
        conduit::Node B; B.set({ 2., 0., 0. });
        conduit::Node C; C.set({ 0., 1., 0. });
        conduit::Node D; D.set({ 2., 1., 0. });
        conduit::Node E; E.set({ 0., 0., 1. });
        conduit::Node F; F.set({ 2., 0., 1. });
        conduit::Node G; G.set({ 0., 1., 1. });
        conduit::Node H; H.set({ 2., 1., 1. });

        // First interpolate along z-dimension.  This results in "posts"
        // at NW, NE, SW, SE corners.
        int n = 3;
        conduit::Node SW;
        conduit::blueprint::mesh::utils::lerp(A, E, n, SW);
        conduit::Node SE;
        conduit::blueprint::mesh::utils::lerp(B, F, n, SE);
        conduit::Node NW;
        conduit::blueprint::mesh::utils::lerp(C, G, n, NW);
        conduit::Node NE;
        conduit::blueprint::mesh::utils::lerp(D, H, n, NE);

        // Check each "post".
        conduit::Node expSW;
        append_array(expSW, {0, 0, 0} );
        append_array(expSW, {0, 0, 0} );
        append_array(expSW, {0, .5, 1} );

        conduit::Node expSE;
        append_array(expSE, {2, 2, 2} );
        append_array(expSE, {0, 0, 0} );
        append_array(expSE, {0, .5, 1} );

        conduit::Node expNW;
        append_array(expNW, {0, 0, 0} );
        append_array(expNW, {1, 1, 1} );
        append_array(expNW, {0, .5, 1} );

        conduit::Node expNE;
        append_array(expNE, {2, 2, 2} );
        append_array(expNE, {1, 1, 1} );
        append_array(expNE, {0, .5, 1} );

        verify_lerp_result(expSW, SW);
        verify_lerp_result(expSE, SE);
        verify_lerp_result(expNW, NW);
        verify_lerp_result(expNE, NE);

        // Next interpolate along y-dimension.  This results in left
        // and right grids.
        int m = 5;
        conduit::Node gridleft;
        conduit::blueprint::mesh::utils::lerp(SW, NW, m, gridleft);
        conduit::Node gridright;
        conduit::blueprint::mesh::utils::lerp(SE, NE, m, gridright);

        // Check the two "grids".
        conduit::Node expgridleft;
        append_array(expgridleft, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} );
        append_array(expgridleft, {0, .25, .5, .75, 1, 0, .25, .5, .75, 1, 0, .25, .5, .75, 1} );
        append_array(expgridleft, {0, 0, 0, 0, 0, .5, .5, .5, .5, .5, 1, 1, 1, 1, 1} );

        conduit::Node expgridright;
        append_array(expgridright, {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2} );
        append_array(expgridright, {0, .25, .5, .75, 1, 0, .25, .5, .75, 1, 0, .25, .5, .75, 1} );
        append_array(expgridright, {0, 0, 0, 0, 0, .5, .5, .5, .5, .5, 1, 1, 1, 1, 1} );

        verify_lerp_result(expgridleft, gridleft);
        verify_lerp_result(expgridright, gridright);

        // Finally interpolate along x-dimension.
        int p = 4;
        conduit::Node cube;
        conduit::blueprint::mesh::utils::lerp(gridleft, gridright, p, cube);

        conduit::Node expcube;
        append_array(expcube, {0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2,
             0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2,
             0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2,
             0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2,
             0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2, 0, .6666666666, 1.3333333333, 2} );
        append_array(expcube, {0, 0, 0, 0, .25, .25, .25, .25, .5, .5, .5, .5, .75, .75, .75, .75, 1, 1, 1, 1,
             0, 0, 0, 0, .25, .25, .25, .25, .5, .5, .5, .5, .75, .75, .75, .75, 1, 1, 1, 1,
             0, 0, 0, 0, .25, .25, .25, .25, .5, .5, .5, .5, .75, .75, .75, .75, 1, 1, 1, 1} );
        append_array(expcube, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5, .5,
             1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1} );

        verify_lerp_result(expcube, cube);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, mesh_info)
{
    namespace topoutils = conduit::blueprint::mesh::utils::topology;
    conduit::Node root;
    const char *yaml = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0.,0.1, 0.3, 0.6, 1.,  0.,0.1, 0.3, 0.6, 1.,  0., 0.1, 0.3, 0.6, 1.]
      y: [0., 0., 0., 0., 0.,  1., 1., 1., 1., 1.,  2.5, 2.5, 2.5, 2.5, 2.5]
topologies:
  main:
    type: unstructured
    coordset: coords
    elements:
      shape: quad
      connectivity: [0,1,6,5, 1,2,7,6, 2,3,8,7, 3,4,9,8, 5,6,11,10, 6,7,12,11, 7,8,13,12, 8,9,14,13]
      sizes: [4,4,4,4,4,4]
      offsets: [0,4,8,12,16,20]
)";
    root.parse(yaml);

    topoutils::MeshInfo info;
    topoutils::compute_mesh_info(root.fetch_existing("topologies/main"), info);

    const double eps = 1.e-12;
    EXPECT_NEAR(info.minExtents[0], 0., eps);
    EXPECT_NEAR(info.minExtents[1], 0., eps);
    EXPECT_NEAR(info.minExtents[2], 0., eps);
    EXPECT_NEAR(info.maxExtents[0], 1., eps);
    EXPECT_NEAR(info.maxExtents[1], 2.5, eps);
    EXPECT_NEAR(info.maxExtents[2], 0., eps);
    EXPECT_NEAR(info.minEdgeLength, 0.1, eps);
    EXPECT_NEAR(info.maxEdgeLength, 1.5, eps);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, mesh_info_3d)
{
    namespace topoutils = conduit::blueprint::mesh::utils::topology;
    conduit::Node root;
    const char *yaml = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0.,0.1, 0.3, 0.,0.1, 0.3, 0.,0.1, 0.3,  0.,0.1, 0.3, 0.,0.1, 0.3, 0.,0.1, 0.3]
      y: [1., 1., 1.,  2., 2., 2., 4., 4., 4.,  1., 1., 1.,  2., 2., 2., 4., 4., 4.]
      z: [2., 2., 2.,  2., 2., 2., 2., 2., 2.,  6., 6., 6.,  6., 6., 6., 6., 6., 6.]
topologies:
  main:
    type: unstructured
    coordset: coords
    elements:
      shape: hex
      connectivity: [0,1,4,3,9,10,13,12, 1,2,5,4,10,11,14,13, 3,4,7,6,12,13,16,15, 4,5,8,7,13,14,17,16]
      sizes: [4,4,4,4]
      offsets: [0,6,12,18]
)";
    root.parse(yaml);

    topoutils::MeshInfo info;
    topoutils::compute_mesh_info(root.fetch_existing("topologies/main"), info);

    const double eps = 1.e-12;
    EXPECT_NEAR(info.minExtents[0], 0., eps);
    EXPECT_NEAR(info.minExtents[1], 1., eps);
    EXPECT_NEAR(info.minExtents[2], 2., eps);
    EXPECT_NEAR(info.maxExtents[0], 0.3, eps);
    EXPECT_NEAR(info.maxExtents[1], 4., eps);
    EXPECT_NEAR(info.maxExtents[2], 6., eps);
    EXPECT_NEAR(info.minEdgeLength, 0.1, eps);
    EXPECT_NEAR(info.maxEdgeLength, 4., eps);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_utils, mesh_info_ph)
{
    namespace topoutils = conduit::blueprint::mesh::utils::topology;
    conduit::Node root;
    const char *yaml = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0.,0.1, 0.3, 0.,0.1, 0.3, 0.,0.1, 0.3,  0.,0.1, 0.3, 0.,0.1, 0.3, 0.,0.1, 0.3]
      y: [1., 1., 1.,  2., 2., 2., 4., 4., 4.,  1., 1., 1.,  2., 2., 2., 4., 4., 4.]
      z: [2., 2., 2.,  2., 2., 2., 2., 2., 2.,  6., 6., 6.,  6., 6., 6., 6., 6., 6.]
topologies:
  main:
    type: unstructured
    coordset: coords
    elements:
      shape: polyhedral
      connectivity: [0,1,2,3,4,5, 1,6,7,8,9,10, 11,12,3,13,14,15, 12,16,8,17,18,19]
      sizes: [6,6,6,6]
      offsets: [0,6,12,18]
    subelements:
      shape: polygonal
      #              0         1          2         3          4        5           6          7          8          9        10           11         12         13         14       15           16         17         18       19
      connectivity: [0,9,12,3, 1,4,13,10, 0,1,10,9, 3,12,13,4, 0,3,4,1, 9,10,13,12, 2,5,14,11, 1,2,11,10, 5,4,13,14, 1,4,5,2, 10,11,14,13, 3,12,15,6, 4,7,16,13, 6,15,16,7, 3,6,7,4, 12,13,16,15, 5,8,17,14, 7,16,17,8, 4,7,8,5, 13,14,17,16]
      sizes: [4,4,4,4,4, 4,4,4,4,4, 4,4,4,4,4, 4,4,4,4,4]
      offsets: [0,4,8,12,16, 20,24,28,32,36, 40,44,48,52,56, 60,64,68,72,76]
)";
    root.parse(yaml);

    topoutils::MeshInfo info;
    topoutils::compute_mesh_info(root.fetch_existing("topologies/main"), info);

    const double eps = 1.e-12;
    EXPECT_NEAR(info.minExtents[0], 0., eps);
    EXPECT_NEAR(info.minExtents[1], 1., eps);
    EXPECT_NEAR(info.minExtents[2], 2., eps);
    EXPECT_NEAR(info.maxExtents[0], 0.3, eps);
    EXPECT_NEAR(info.maxExtents[1], 4., eps);
    EXPECT_NEAR(info.maxExtents[2], 6., eps);
    EXPECT_NEAR(info.minEdgeLength, 0.1, eps);
    EXPECT_NEAR(info.maxEdgeLength, 4., eps);
}
