// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_verify.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include <algorithm>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
namespace bputils = conduit::blueprint::mesh::utils;

/// Testing Constants ///

std::vector<std::string> get_log_keywords()
{
    Node log_node;
    log::info(log_node,"","");
    log::optional(log_node,"","");
    log::error(log_node,"","");
    log::validation(log_node,false);
    return log_node.child_names();
}

const std::vector<std::string> LOG_KEYWORDS = get_log_keywords();
const std::vector<std::string> COORDINATE_COORDSYSS[] =
    {bputils::CARTESIAN_AXES, bputils::CYLINDRICAL_AXES, bputils::SPHERICAL_AXES};

typedef bool (*VerifyFun)(const Node&, Node&);

/// Helper Functions ///
/// Testing Helpers ///
index_t braid_bound_npts_z(const std::string &mesh_type, index_t npts_z)
{
    if(mesh_type == "tris"  ||
       mesh_type == "quads" ||
       mesh_type == "quads_poly")
    {
        return 0;
    }
    else
    {
        return npts_z;
    }
}


bool is_valid_coordsys(bool (*coordsys_valid_fun)(const Node&, Node&),
                       const std::vector<std::string>& coordsys)
{
    Node n, info;

    bool is_valid = true;
    for(size_t ci = 0; ci < coordsys.size(); ci++)
    {
        const std::string& coordsys_dim = coordsys[ci];

        n[coordsys_dim].set("test");
        is_valid &= !coordsys_valid_fun(n,info);

        n[coordsys_dim].set(10);
        is_valid &= coordsys_valid_fun(n,info);

        // FIXME: The coordinate system checking functions shouldn't accept
        // systems such as (y) or (x, z); all successive dimensions should
        // require the existence of previous coordsys dimensions.
        /*
        if( ci > 0 )
        {
            const std::string& prev_dim = coordsys[ci-1];
            n.remove(prev_dim);
            is_valid &= !coordsys_valid_fun(n,info);
            n[coordsys_dim].set(10);
        }
        */
    }

    return is_valid;
}


bool has_empty_warning(const Node &info)
{
    bool res = false;

    if((info.dtype().is_object() && info.has_child("info")) &&
       (info["info"].dtype().is_object() || info["info"].dtype().is_list()))
    {
        NodeConstIterator iitr = info["info"].children();
        while(iitr.has_next())
        {
            const Node &ichld = iitr.next();
            res |= ichld.dtype().is_char8_str() &&
                   ichld.to_string().find("is an empty mesh", 0) != std::string::npos;
        }
    }

    return res;
}


bool has_consistent_validity(const Node &n)
{
    // TODO(JRC): This function will have problems for given nodes containing
    // nested lists.
    bool is_consistent = !n.dtype().is_object() ||
        (n.has_child("valid") && n["valid"].dtype().is_string() &&
        (n["valid"].as_string() == "true" || n["valid"].as_string() == "false"));

    NodeConstIterator itr = n.children();
    while(itr.has_next())
    {
        const Node &chld= itr.next();
        const std::string chld_name = itr.name();
        if(std::find(LOG_KEYWORDS.begin(), LOG_KEYWORDS.end(), chld_name) ==
            LOG_KEYWORDS.end())
        {
            is_consistent &= has_consistent_validity(chld);
            if(is_consistent)
            {
                bool n_valid = n["valid"].as_string() == "true";
                bool c_valid = chld["valid"].as_string() == "true";
                is_consistent &= !(n_valid && !c_valid);
            }
        }
    }

    return is_consistent;
}

/// Wrapper Functions ///

bool verify_coordset_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("coordset",n,info);
}

bool verify_topology_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("topology",n,info);
}

bool verify_matset_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("matset",n,info);
}

bool verify_specset_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("specset",n,info);
}

bool verify_field_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("field",n,info);
}

bool verify_adjset_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("adjset",n,info);
}

bool verify_nestset_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("nestset",n,info);
}

bool verify_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("index",n,info);
}

bool verify_coordset_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("coordset/index",n,info);
}

bool verify_topology_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("topology/index",n,info);
}

bool verify_matset_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("matset/index",n,info);
}

bool verify_specset_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("specset/index",n,info);
}

bool verify_field_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("field/index",n,info);
}

bool verify_adjset_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("adjset/index",n,info);
}

bool verify_nestset_index_protocol(const Node &n, Node &info)
{
    return blueprint::mesh::verify("nestset/index",n,info);
}

bool verify_mesh_multi_domain_protocol(const Node &n, Node &info)
{
    // we can only call is_multi_domain if verify is true
    return  blueprint::mesh::verify(n,info) && 
            blueprint::mesh::is_multi_domain(n);
}


/// Helper for mesh verify checks ///

#define CHECK_MESH(verify, n, info, expected)    \
{                                                \
                                                 \
    if(verify(n, info) != expected)              \
    {                                            \
        std::cout << "[verify test failed!]\n"   \
                  << "[expected result: "        \
                  << expected << "]\n"           \
                  << "[details:]\n";             \
        n.print();                               \
        info.print();                            \
    }                                            \
                                                 \
    EXPECT_EQ(verify(n, info), expected);        \
    EXPECT_TRUE(has_consistent_validity(info));  \
}                                                \

#define CHECK_MATSET(n, iub, ied)                                       \
{                                                                       \
    EXPECT_EQ(blueprint::mesh::matset::is_uni_buffer(n), iub);          \
    EXPECT_EQ(blueprint::mesh::matset::is_multi_buffer(n), !iub);       \
    EXPECT_EQ(blueprint::mesh::matset::is_element_dominant(n), ied);    \
    EXPECT_EQ(blueprint::mesh::matset::is_material_dominant(n), !ied);  \
}                                                                       \

/// Mesh Coordinate Set Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_logical_dims)
{
    VerifyFun verify_coordset_logical = blueprint::mesh::logical_dims::verify;

    Node n, info;
    CHECK_MESH(verify_coordset_logical,n,info,false);

    EXPECT_TRUE(is_valid_coordsys(verify_coordset_logical,bputils::LOGICAL_AXES));

    EXPECT_FALSE(is_valid_coordsys(verify_coordset_logical,bputils::CARTESIAN_AXES));
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_uniform_origin)
{
    VerifyFun verify_uniform_origin = blueprint::mesh::coordset::uniform::origin::verify;

    Node n, info;
    // FIXME: The origin verification function shouldn't accept an empty node.
    // CHECK_MESH(verify_uniform_origin,n,info,false);

    EXPECT_TRUE(is_valid_coordsys(verify_uniform_origin,bputils::CARTESIAN_AXES));
    EXPECT_TRUE(is_valid_coordsys(verify_uniform_origin,bputils::SPHERICAL_AXES));
    EXPECT_TRUE(is_valid_coordsys(verify_uniform_origin,bputils::CYLINDRICAL_AXES));

    EXPECT_FALSE(is_valid_coordsys(verify_uniform_origin,bputils::LOGICAL_AXES));
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_uniform_spacing)
{
    VerifyFun verify_uniform_spacing = blueprint::mesh::coordset::uniform::spacing::verify;

    Node n, info;
    // FIXME: The spacing verification function shouldn't accept an empty node.
    // CHECK_MESH(verify_uniform_spacing,n,info,false);

    EXPECT_TRUE(is_valid_coordsys(verify_uniform_spacing,{"dx","dy","dz"}));
    EXPECT_TRUE(is_valid_coordsys(verify_uniform_spacing,{"dr","dtheta","dphi"}));
    EXPECT_TRUE(is_valid_coordsys(verify_uniform_spacing,{"dz","dr"}));

    EXPECT_FALSE(is_valid_coordsys(verify_uniform_spacing,{"di","dj","dk"}));
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_uniform)
{
    VerifyFun verify_uniform_coordset = blueprint::mesh::coordset::uniform::verify;

    Node n, info;
    CHECK_MESH(verify_uniform_coordset,n,info,false);

    n["type"].set("uniform");
    CHECK_MESH(verify_uniform_coordset,n,info,false);

    n["dims"]["i"].set(1);
    n["dims"]["j"].set(2);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    n["dims"]["k"].set("test");
    CHECK_MESH(verify_uniform_coordset,n,info,false);
    n["dims"]["k"].set(3);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    Node dims = n["dims"];
    n.remove("dims");

    n["origin"]["x"].set(10);
    n["origin"]["y"].set(20);
    CHECK_MESH(verify_uniform_coordset,n,info,false);

    n["dims"].set(dims);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    n["origin"]["z"].set("test");
    CHECK_MESH(verify_uniform_coordset,n,info,false);
    n["origin"]["z"].set(30);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    n["spacing"]["dx"].set(0.1);
    n["spacing"]["dy"].set(0.2);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    n["spacing"]["dz"].set("test");
    CHECK_MESH(verify_uniform_coordset,n,info,false);
    n["spacing"]["dz"].set(0.3);
    CHECK_MESH(verify_uniform_coordset,n,info,true);

    n["type"].set("rectilinear");
    CHECK_MESH(verify_uniform_coordset,n,info,false);
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_rectilinear)
{
    VerifyFun verify_rectilinear_coordset = blueprint::mesh::coordset::rectilinear::verify;

    Node n, info;
    CHECK_MESH(verify_rectilinear_coordset,n,info,false);

    n["values"].set("test");
    CHECK_MESH(verify_rectilinear_coordset,n,info,false);

    n["type"].set("rectilinear");
    CHECK_MESH(verify_rectilinear_coordset,n,info,false);

    for(size_t ci = 0; ci < 3; ci++)
    {
        const std::vector<std::string>& coord_coordsys = COORDINATE_COORDSYSS[ci];

        n["values"].reset();
        for(size_t cj = 0; cj < coord_coordsys.size(); cj++)
        {
            n["values"][coord_coordsys[cj]].set(DataType::float64(10));
            CHECK_MESH(verify_rectilinear_coordset,n,info,true);
            // info.print();
        }
    }

    // check case where number of elements for each child doesn't match
    // (rectilinear coordsets use cross product of input coord arrays, they
    //  don't need to be mcarrays)
    for(size_t ci = 0; ci < 3; ci++)
    {
        const std::vector<std::string>& coord_coordsys = COORDINATE_COORDSYSS[ci];

        n["values"].reset();
        for(size_t cj = 0; cj < coord_coordsys.size(); cj++)
        {
            n["values"][coord_coordsys[cj]].set(DataType::float64(cj + 5));
            CHECK_MESH(verify_rectilinear_coordset,n,info,true);
            // info.print();
        }
    }

    n["type"].set("uniform");
    CHECK_MESH(verify_rectilinear_coordset,n,info,false);


    // FIXME: The logical coordinate system shouldn't be an accepted value
    // for the rectilinear verify function.
    /*
    n["values"].reset();
    for(index_t ci = 0; ci < bputils::LOGICAL_AXES.size(); ci++)
    {
        n["values"][bputils::LOGICAL_AXES[ci]].set(DataType::float64(10));
        CHECK_MESH(verify_rectilinear_coordset,n,info,false);
    }
    */
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_explicit)
{
    VerifyFun verify_explicit_coordset = blueprint::mesh::coordset::_explicit::verify;

    Node n, info;
    CHECK_MESH(verify_explicit_coordset,n,info,false);

    n["values"].set("test");
    CHECK_MESH(verify_explicit_coordset,n,info,false);

    n["type"].set("explicit");
    CHECK_MESH(verify_explicit_coordset,n,info,false);

    for(size_t ci = 0; ci < 3; ci++)
    {
        const std::vector<std::string>& coord_coordsys = COORDINATE_COORDSYSS[ci];

        n["values"].reset();
        for(size_t cj = 0; cj < coord_coordsys.size(); cj++)
        {
            n["values"][coord_coordsys[cj]].set(DataType::float64(10));
            CHECK_MESH(verify_explicit_coordset,n,info,true);
        }
    }

    n["type"].set("uniform");
    CHECK_MESH(verify_explicit_coordset,n,info,false);

    // FIXME: The logical coordinate system shouldn't be an accepted value
    // for the explicit verify function.
    /*
    n["values"].reset();
    for(index_t ci = 0; ci < bputils::LOGICAL_AXES.size(); ci++)
    {
        n["values"][bputils::LOGICAL_AXES[ci]].set(DataType::float64(10));
        CHECK_MESH(verify_explicit_coordset,n,info,false);
    }
    */
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_types)
{
    VerifyFun verify_coordset = blueprint::mesh::coordset::verify;

    Node n, info;

    const std::string coordset_types[] = {"uniform", "rectilinear", "explicit"};
    const std::string coordset_fids[] = {"uniform", "rectilinear", "quads"};
    for(index_t ci = 0; ci < 3; ci++)
    {
        n.reset();
        blueprint::mesh::examples::braid(coordset_fids[ci],10,10,0,n);
        Node& coordset_node = n["coordsets/coords"];
        CHECK_MESH(verify_coordset,coordset_node,info,true);

        coordset_node["type"].set(0);
        CHECK_MESH(verify_coordset,coordset_node,info,false);

        coordset_node["type"].set("unstructured");
        CHECK_MESH(verify_coordset,coordset_node,info,false);

        if(ci != 2)
        {
            coordset_node["type"].set(coordset_types[2]);
            EXPECT_FALSE(blueprint::mesh::topology::verify(coordset_node,info));
        }

        coordset_node["type"].set(coordset_types[ci]);
        CHECK_MESH(verify_coordset,coordset_node,info,true);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_coordsys)
{
    VerifyFun verify_coordset_coordsys = blueprint::mesh::coordset::coord_system::verify;

    Node n, info;
    CHECK_MESH(verify_coordset_coordsys,n,info,false);

    const std::string coordsys_types[] = {"cartesian", "cylindrical", "spherical"};
    for(index_t ci = 0; ci < 3; ci++)
    {
        n.reset();
        info.reset();

        n["type"].set(coordsys_types[ci]);
        CHECK_MESH(verify_coordset_coordsys,n,info,false);

        n["axes"].set(0);
        CHECK_MESH(verify_coordset_coordsys,n,info,false);

        n["axes"].reset();
        const std::vector<std::string>& coordsys = COORDINATE_COORDSYSS[ci];
        for(size_t ai = 0; ai < coordsys.size(); ai++)
        {
            n["axes"][coordsys[ai]].set(10);
            CHECK_MESH(verify_coordset_coordsys,n,info,true);
        }

        n["type"].set(coordsys_types[(ci == 0) ? 2 : ci - 1]);
        CHECK_MESH(verify_coordset_coordsys,n,info,false);

        n["type"].set("barycentric");
        CHECK_MESH(verify_coordset_coordsys,n,info,false);

        n["type"].set(10);
        CHECK_MESH(verify_coordset_coordsys,n,info,false);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, coordset_general)
{
    VerifyFun verify_coordset_funs[] = {
        blueprint::mesh::coordset::verify,
        verify_coordset_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_coordset = verify_coordset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_coordset,mesh,info,false);

        blueprint::mesh::examples::braid("uniform",10,10,10,mesh);
        Node& n = mesh["coordsets"]["coords"];

        n.remove("type");
        CHECK_MESH(verify_coordset,n,info,false);
        n["type"].set("structured");
        CHECK_MESH(verify_coordset,n,info,false);
        n["type"].set("rectilinear");
        CHECK_MESH(verify_coordset,n,info,false);

        n["type"].set("uniform");
        CHECK_MESH(verify_coordset,n,info,true);
    }
}

/// Mesh Topology Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_points)
{
    VerifyFun verify_points_topology = blueprint::mesh::topology::points::verify;
    Node n, info;

    CHECK_MESH(verify_points_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set("points");
    CHECK_MESH(verify_points_topology,n,info,true);

    n["coordset"].set(1);
    CHECK_MESH(verify_points_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set(1);
    CHECK_MESH(verify_points_topology,n,info,false);

    n["type"].set("uniform");
    CHECK_MESH(verify_points_topology,n,info,false);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_uniform)
{
    VerifyFun verify_uniform_topology = blueprint::mesh::topology::uniform::verify;
    Node n, info;

    CHECK_MESH(verify_uniform_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set("uniform");
    CHECK_MESH(verify_uniform_topology,n,info,true);

    n["coordset"].set(1);
    CHECK_MESH(verify_uniform_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set(1);
    CHECK_MESH(verify_uniform_topology,n,info,false);

    n["type"].set("points");
    CHECK_MESH(verify_uniform_topology,n,info,false);
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_rectilinear)
{
    VerifyFun verify_rectilinear_topology = blueprint::mesh::topology::rectilinear::verify;
    Node n, info;

    CHECK_MESH(verify_rectilinear_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set("rectilinear");
    CHECK_MESH(verify_rectilinear_topology,n,info,true);

    n["coordset"].set(1);
    CHECK_MESH(verify_rectilinear_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set(1);
    CHECK_MESH(verify_rectilinear_topology,n,info,false);

    n["type"].set("points");
    CHECK_MESH(verify_rectilinear_topology,n,info,false);
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_structured)
{
    VerifyFun verify_structured_topology = blueprint::mesh::topology::structured::verify;

    Node n, info;
    CHECK_MESH(verify_structured_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set("structured");
    CHECK_MESH(verify_structured_topology,n,info,false);

    n["elements"].set(0);
    CHECK_MESH(verify_structured_topology,n,info,false);

    n["elements"].reset();
    n["elements"]["dims"].set(0);
    CHECK_MESH(verify_structured_topology,n,info,false);

    n["elements"]["dims"].reset();
    n["elements"]["dims"]["x"].set(5);
    n["elements"]["dims"]["y"].set(10);
    CHECK_MESH(verify_structured_topology,n,info,false);

    n["elements"]["dims"].reset();
    n["elements"]["dims"]["i"].set(15);
    n["elements"]["dims"]["j"].set(20);
    CHECK_MESH(verify_structured_topology,n,info,true);

    n["elements"]["dims"]["k"].set(25);
    CHECK_MESH(verify_structured_topology,n,info,true);

    n["type"].set("unstructured");
    CHECK_MESH(verify_structured_topology,n,info,false);
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_unstructured)
{
    VerifyFun verify_unstructured_topology = blueprint::mesh::topology::unstructured::verify;

    Node n, info;
    CHECK_MESH(verify_unstructured_topology,n,info,false);

    n["coordset"].set("coords");
    n["type"].set("unstructured");
    CHECK_MESH(verify_unstructured_topology,n,info,false);

    n["elements"].set(0);
    CHECK_MESH(verify_unstructured_topology,n,info,false);

    { // Single Shape Topology Tests //
        n["elements"].reset();
        n["elements"]["shape"].set("undefined");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["shape"].set("quad");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["connectivity"].set("quad");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["connectivity"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["connectivity"].set(DataType::int32(1));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["elements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["offsets"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["elements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["elements"].remove("offsets");
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["type"].set("structured");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["type"].set("unstructured");
        CHECK_MESH(verify_unstructured_topology,n,info,true);
    }

    { // Single Shape Unstructured Polygon Tests //
        n["elements"].reset();
        n["elements"]["shape"].set("undefined");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["shape"].set("polygonal");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["connectivity"].set(DataType::int32(10));
        n["elements"]["offsets"].set(DataType::int32(10));
        n["elements"]["sizes"].set(DataType::int32(10));     
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["connectivity"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["offsets"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"].remove("offsets");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["sizes"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["sizes"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["type"].set("structured");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["type"].set("unstructured");
        CHECK_MESH(verify_unstructured_topology,n,info,true);
    }

    { // Single Shape Unstructured Polyhedral Tests //
        n["elements"].reset();
        n["elements"]["shape"].set("undefined");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["shape"].set("polyhedral");
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["sizes"].set(DataType::int32(10));     
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["subelements"]["shape"].set("polygonal");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["subelements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["sizes"].set(DataType::int32(10));     
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["connectivity"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["subelements"]["connectivity"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["connectivity"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["offsets"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"].remove("offsets");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["subelements"]["offsets"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"].remove("offsets");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["offsets"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["sizes"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["sizes"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["subelements"]["sizes"].set(DataType::float64(10));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["sizes"].set(DataType::int32(10));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["type"].set("structured");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["type"].set("unstructured");
        CHECK_MESH(verify_unstructured_topology,n,info,true);
    }

    { // Mixed Shape Topology List Tests //
        n["elements"].reset();

        n["elements"]["a"].set(0);
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["elements"]["a"]["shape"].set("quad");
        n["elements"]["a"]["connectivity"].set(DataType::int32(5));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["b"]["shape"].set("undefined");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["b"]["shape"].set("quad");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["b"]["connectivity"].set(DataType::float32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["b"]["connectivity"].set(DataType::int32(1));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["c"]["shape"].set("tri");
        n["elements"]["c"]["connectivity"].set(DataType::int32(5));
        n["elements"]["c"]["offsets"].set(DataType::int32(5));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["d"]["shape"].set("polygonal");
        n["elements"]["d"]["connectivity"].set(DataType::int32(6));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["elements"]["d"]["offsets"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["d"]["sizes"].set(DataType::int32(2));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["d"]["sizes"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["elements"]["e"]["shape"].set("polyhedral");
        n["elements"]["e"]["connectivity"].set(DataType::int32(6));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["e"]["offsets"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["e"]["sizes"].set(DataType::int32(2));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["elements"]["e"]["sizes"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,false);

        n["subelements"]["e"]["shape"].set("polygonal");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["e"]["connectivity"].set(DataType::int32(6));
        CHECK_MESH(verify_unstructured_topology,n,info,true);
        n["subelements"]["e"]["offsets"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,false);
        n["subelements"]["e"]["sizes"].set(DataType::int32(3));
        CHECK_MESH(verify_unstructured_topology,n,info,true);

        n["type"].set("structured");
        CHECK_MESH(verify_unstructured_topology,n,info,false);
    }

    { // Multiple Shape Topology Stream Tests //
        // FIXME: Implement once multiple unstructured shape topologies are implemented.
        n["elements"].reset();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_types)
{
    VerifyFun verify_topology = blueprint::mesh::topology::verify;

    Node n, info;

    const std::string topology_types[] = {
        "points", "uniform", "rectilinear", "structured", "unstructured"};
    const std::string topology_fids[] = {
        "points_implicit", "uniform", "rectilinear", "structured", "quads"};
    const index_t topo_type_count = sizeof(topology_types) / sizeof(std::string);

    for(index_t ti = 0; ti < topo_type_count; ti++)
    {
        n.reset();
        blueprint::mesh::examples::braid(topology_fids[ti],10,10,0,n);
        Node& topology_node = n["topologies/mesh"];
        CHECK_MESH(verify_topology,topology_node,info,true);

        topology_node["type"].set(0);
        CHECK_MESH(verify_topology,topology_node,info,false);

        topology_node["type"].set("explicit");
        CHECK_MESH(verify_topology,topology_node,info,false);

        if(ti != 4)
        {
            topology_node["type"].set(topology_types[4]);
            CHECK_MESH(verify_topology,topology_node,info,false);
        }

        topology_node["type"].set(topology_types[ti]);
        CHECK_MESH(verify_topology,topology_node,info,true);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_shape)
{
    VerifyFun verify_topology = blueprint::mesh::topology::verify;

    Node n, info;

    const std::string topology_shapes[] = {
        "point", "line",
        "tri", "quad", "polygonal",
        "tet", "hex", "polyhedral",
        "wedge" , "pyramid"};
    const std::string topology_fids[] = {
        "points", "lines",
        "tris", "quads", "quads_poly",
        "tets", "hexs", "hexs_poly",
        "wedges" , "pyramids"};
    const index_t topo_shape_count = sizeof(topology_shapes) / sizeof(std::string);

    for(index_t ti = 0; ti < topo_shape_count; ti++)
    {
        n.reset();
        blueprint::mesh::examples::braid(topology_fids[ti],
                                         10,
                                         10,
                                         braid_bound_npts_z(topology_fids[ti], 2),
                                         n);
        Node& topology_node = n["topologies/mesh"];
        CHECK_MESH(verify_topology,topology_node,info,true);

        topology_node["elements/shape"].set(0);
        CHECK_MESH(verify_topology,topology_node,info,false);

        topology_node["elements/shape"].set("unstructured");
        CHECK_MESH(verify_topology,topology_node,info,false);

        topology_node["elements/shape"].set(topology_shapes[ti]);
        CHECK_MESH(verify_topology,topology_node,info,true);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, topology_general)
{
    VerifyFun verify_topology_funs[] = {
        blueprint::mesh::topology::verify,
        verify_topology_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_topology = verify_topology_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_topology,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        Node& n = mesh["topologies"]["mesh"];

        { // Type Field Tests //
            n.remove("type");
            CHECK_MESH(verify_topology,n,info,false);
            n["type"].set("explicit");
            CHECK_MESH(verify_topology,n,info,false);

            // FIXME: Remove the comments from the following line once the verify functions
            // for uniform and rectilinear topologies have been implemented.
            const std::string topology_types[] = {/*"uniform", "rectilinear", */"structured"};
            for(index_t ti = 0; ti < 1; ti++)
            {
                n["type"].set(topology_types[ti]);
                CHECK_MESH(verify_topology,n,info,false);
            }

            n["type"].set("unstructured");
            CHECK_MESH(verify_topology,n,info,true);
        }

        { // Coordset Field Tests //
            n.remove("coordset");
            CHECK_MESH(verify_topology,n,info,false);

            n["coordset"].set(0);
            CHECK_MESH(verify_topology,n,info,false);

            n["coordset"].set("coords");
            CHECK_MESH(verify_topology,n,info,true);
        }

        { // Grid Function Field Tests //
            n["grid_function"].set(10);
            CHECK_MESH(verify_topology,n,info,false);
            n["grid_function"].set("coords_gf");
            CHECK_MESH(verify_topology,n,info,true);
        }
    }
}

/// Mesh Matsets Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, matset_general)
{
    VerifyFun verify_matset_funs[] = {
        blueprint::mesh::matset::verify,
        verify_matset_protocol};

    const bool is_uni_buffer = true, is_multi_buffer = false;
    const bool is_element_dominant = true, is_material_dominant = false;

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_matset = verify_matset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_matset,mesh,info,false);

        blueprint::mesh::examples::misc("matsets",10,10,1,mesh);
        Node& n = mesh["matsets"]["mesh"];
        CHECK_MESH(verify_matset,n,info,true);

        { // Topology Field Tests //
            n.remove("topology");
            CHECK_MESH(verify_matset,n,info,false);
            n["topology"].set(10);
            CHECK_MESH(verify_matset,n,info,false);
            n["topology"].set("mesh");
            CHECK_MESH(verify_matset,n,info,true);
        }

        { // Volume Fractions Field Tests //
            Node vfs = n["volume_fractions"];

            n.remove("volume_fractions");
            CHECK_MESH(verify_matset,n,info,false);
            n["volume_fractions"].set("values");
            CHECK_MESH(verify_matset,n,info,false);
            n["volume_fractions"].set(DataType::float64(10));
            CHECK_MESH(verify_matset,n,info,false);

            n["volume_fractions"].reset();
            n["volume_fractions"]["x"].set("Hello, ");
            n["volume_fractions"]["y"].set("World!");
            CHECK_MESH(verify_matset,n,info,false);

            n["volume_fractions"].reset();
            n["volume_fractions"]["m1"].set(DataType::float64(5));
            n["volume_fractions"]["m2"].set(DataType::float64(5));
            CHECK_MESH(verify_matset,n,info,true);

            { // Uni-Buffer Tests //
                n.reset();
                n["topology"].set("mesh");

                n["volume_fractions"].set(DataType::float64(5));
                CHECK_MESH(verify_matset,n,info,false);

                n["material_ids"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,false);
                n.remove("material_ids");
                n["material_map"]["m1"].set(1);
                CHECK_MESH(verify_matset,n,info,false);
                n["material_ids"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_uni_buffer,is_element_dominant);

                n["indices"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,true);
                n["sizes"].set(DataType::uint32(5));
                n["offsets"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_uni_buffer,is_element_dominant);

                //--------------------------------------//
                // check for bad material_maps
                //--------------------------------------//

                // - bad material_maps - //

                //  not an object (empty)
                n["material_map"].reset();
                CHECK_MESH(verify_matset,n,info,false);

                //  not an object (leaf)
                n["material_map"] = "bananas";
                CHECK_MESH(verify_matset,n,info,false);

                // child leaves not an integer
                n["material_map"].reset();
                n["material_map/m1"] = "bananas";
                CHECK_MESH(verify_matset,n,info,false);
            }

            { // Multi-Buffer Tests //
                n.reset();
                n["topology"].set("mesh");

                n["volume_fractions"]["m1"]["values"].set(DataType::float64(8));
                n["volume_fractions"]["m1"]["indices"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_multi_buffer,is_element_dominant);

                n["volume_fractions"]["m2"]["indices"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,false);
                n["volume_fractions"]["m2"]["values"].set(DataType::float64(10));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_multi_buffer,is_element_dominant);

                n["volume_fractions"]["m3"]["sizes"].set(DataType::uint32(3));
                n["volume_fractions"]["m3"]["values"].set(DataType::float64(30));
                CHECK_MESH(verify_matset,n,info,false);
                n["volume_fractions"]["m3"]["offsets"].set(DataType::uint32(3));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_multi_buffer,is_element_dominant);

                n["volume_fractions"]["m4"]["test"]["values"].set(DataType::uint32(3));
                CHECK_MESH(verify_matset,n,info,false);
                n["volume_fractions"]["m4"]["indices"].set(DataType::uint32(5));
                CHECK_MESH(verify_matset,n,info,false);
                
                // make sure we are in good shape for opt material_map checks
                n["volume_fractions"].remove("m4");
                CHECK_MESH(verify_matset,n,info,true);

                //--------------------------------------//
                // check for optional use of material_map
                //--------------------------------------//

                // - good material_map - //
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = 1;
                n["material_map/m3"] = 2;

                CHECK_MESH(verify_matset,n,info,true);

                // - bad material_maps - //

                //  not an object (empty)
                n["material_map"].reset();
                CHECK_MESH(verify_matset,n,info,false);

                //  not an object (leaf)
                n["material_map"] = "bananas";
                CHECK_MESH(verify_matset,n,info,false);

                // child leaves not an integer
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = "bananas";
                n["material_map/m3"] = "mangoes";
                CHECK_MESH(verify_matset,n,info,false);

                // material_map ents don't match vfs
                // (more mat map ents than vfs)
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = 1;
                n["material_map/m3"] = 2;
                n["material_map/m4"] = 4;

                CHECK_MESH(verify_matset,n,info,false);

                // material_map ents are subset of vfs
                // (should be true)
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m3"] = 2;
                CHECK_MESH(verify_matset,n,info,true);
            }

            { // Element ID Tests //
                // Multi-Buffer Volume Fractions //
                n.reset();
                n["topology"].set("mesh");
                n["volume_fractions"]["m1"].set(DataType::float64(5));
                n["volume_fractions"]["m2"].set(DataType::float64(5));

                n["element_ids"].reset();
                n["element_ids"].set(DataType::float64(5));
                CHECK_MESH(verify_matset,n,info,false);
                n["element_ids"].set(DataType::int64(5));
                CHECK_MESH(verify_matset,n,info,false);

                n["element_ids"].reset();
                n["element_ids"]["m1"].set(DataType::int64(5));
                CHECK_MESH(verify_matset,n,info,false);
                n["element_ids"]["m2"].set(DataType::int64(5));
                CHECK_MESH(verify_matset,n,info,true);
                n["element_ids"]["m3"].set(DataType::int64(5));
                CHECK_MESH(verify_matset,n,info,false);

                n["element_ids"].reset();
                n["element_ids"]["m1"].set(DataType::int64(5));
                n["element_ids"]["m2"].set(DataType::float32(5));
                CHECK_MESH(verify_matset,n,info,false);
                n["element_ids"]["m2"].set(DataType::int32(5));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_multi_buffer,is_material_dominant);

                //--------------------------------------//
                // check for optional use of material_map
                //--------------------------------------//

                // - good material_map - //
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = 1;
                CHECK_MESH(verify_matset,n,info,true);

                // - bad material_maps - //

                //  not an object (empty)
                n["material_map"].reset();
                CHECK_MESH(verify_matset,n,info,false);

                //  not an object (leaf)
                n["material_map"] = "bananas";
                CHECK_MESH(verify_matset,n,info,false);

                // child leaves not an integer
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = "bananas";
                n["material_map/m3"] = "mangoes";
                CHECK_MESH(verify_matset,n,info,false);

                // material_map ents don't match vfs
                n["material_map"].reset();
                n["material_map/banana"] = 0;
                n["material_map/mango"] = 1;
                CHECK_MESH(verify_matset,n,info,false);


                n.remove("material_map");

                // Uni-Buffer Volume Fractions //
                n["volume_fractions"].reset();
                n["volume_fractions"].set(DataType::float64(5));
                n["material_map"]["m1"].set(1);
                n["material_ids"].set(DataType::uint32(5));

                n["element_ids"].reset();
                n["element_ids"]["m1"].set(DataType::int64(5));
                n["element_ids"]["m2"].set(DataType::int64(5));
                CHECK_MESH(verify_matset,n,info,false);

                n["element_ids"].reset();
                n["element_ids"].set(DataType::float64(5));
                CHECK_MESH(verify_matset,n,info,false);
                n["element_ids"].set(DataType::int32(5));
                CHECK_MESH(verify_matset,n,info,true);
                CHECK_MATSET(n,is_uni_buffer,is_material_dominant);
                
                //--------------------------------------//
                // check for optional use of material_map
                //--------------------------------------//

                // - good material_map - //
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = 1;
                CHECK_MESH(verify_matset,n,info,true);

                // - bad material_maps - //

                //  not an object (empty)
                n["material_map"].reset();
                CHECK_MESH(verify_matset,n,info,false);

                //  not an object (leaf)
                n["material_map"] = "bananas";
                CHECK_MESH(verify_matset,n,info,false);

                // child leaves not an integer
                n["material_map"].reset();
                n["material_map/m1"] = 0;
                n["material_map/m2"] = "bananas";
                n["material_map/m3"] = "mangoes";
                CHECK_MESH(verify_matset,n,info,false);

            }

            n.reset();
            n["topology"].set("mesh");
            n["volume_fractions"].set(vfs);
            CHECK_MESH(verify_matset,n,info,true);
        }
    }
}

/// Mesh Specsets Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, specset_general)
{
    VerifyFun verify_specset_funs[] = {
        blueprint::mesh::specset::verify,
        verify_specset_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_specset = verify_specset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_specset,mesh,info,false);

        blueprint::mesh::examples::misc("specsets",10,10,1,mesh);
        Node& n = mesh["specsets"]["mesh"];
        CHECK_MESH(verify_specset,n,info,true);

        { // Matset Field Tests //
            n.remove("matset");
            CHECK_MESH(verify_specset,n,info,false);
            n["matset"].set(10);
            CHECK_MESH(verify_specset,n,info,false);
            n["matset"].set("mesh");
            CHECK_MESH(verify_specset,n,info,true);
        }

        { // Matset Values Field Tests //
            Node mfs = n["matset_values"];

            n.remove("matset_values");
            CHECK_MESH(verify_specset,n,info,false);
            n["matset_values"].set("values");
            CHECK_MESH(verify_specset,n,info,false);
            n["matset_values"].set(DataType::float64(10));
            CHECK_MESH(verify_specset,n,info,false);

            n["matset_values"].reset();
            n["matset_values"]["x"].set("Hello, ");
            n["matset_values"]["y"].set("World!");
            CHECK_MESH(verify_specset,n,info,false);

            n["matset_values"].reset();
            n["matset_values"]["m1"].set(DataType::float64(5));
            n["matset_values"]["m2"].set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,false);

            n["matset_values"].reset();
            n["matset_values"]["m1"].append().set(DataType::float64(5));
            n["matset_values"]["m1"].append().set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,false);
            n["matset_values"]["m2"].append().set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,false);

            n["matset_values"].reset();
            n["matset_values"]["m1"]["s1"].set(DataType::float64(5));
            n["matset_values"]["m2"]["s1"].set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,true);

            n["matset_values"].reset();
            n["matset_values"]["m1"]["s1"].set(DataType::float64(5));
            n["matset_values"]["m2"]["s2"].set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,true);
            n["matset_values"]["m2"]["s3"].set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,true);

            n["matset_values"].reset();
            n["matset_values"]["m1"]["s1"].set(DataType::float64(5));
            n["matset_values"]["m1"]["s2"].set(DataType::float64(6));
            CHECK_MESH(verify_specset,n,info,true);

            n["matset_values"].reset();
            n["matset_values"]["m1"]["s1"].set(DataType::float64(5));
            n["matset_values"]["m1"]["s2"].set(DataType::float64(5));
            CHECK_MESH(verify_specset,n,info,true);
            n["matset_values"]["m2"]["s3"].set(DataType::float64(6));
            CHECK_MESH(verify_specset,n,info,true);

            n["matset_values"].reset();
            n["matset_values"].set(mfs);
            CHECK_MESH(verify_specset,n,info,true);
        }
    }
}

// / Mesh Field Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, field_general)
{
    VerifyFun verify_field_funs[] = {
        blueprint::mesh::field::verify,
        verify_field_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_field = verify_field_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_field,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        Node& n = mesh["fields"]["braid"];

        { // Topology Field Tests //
            n.remove("topology");
            CHECK_MESH(verify_field,n,info,false);
            n["topology"].set(10);
            CHECK_MESH(verify_field,n,info,false);
            n["topology"].set("mesh");
            CHECK_MESH(verify_field,n,info,true);
        }

        { // Values Field Tests //
            Node values = n["values"];

            n.remove("values");
            CHECK_MESH(verify_field,n,info,false);
            n["values"].set("values");
            CHECK_MESH(verify_field,n,info,false);
            n["values"].set(DataType::float64(10));
            CHECK_MESH(verify_field,n,info,true);

            n["values"].reset();
            n["values"]["x"].set("Hello, ");
            n["values"]["y"].set("World!");
            CHECK_MESH(verify_field,n,info,false);

            n["values"].reset();
            n["values"]["x"].set(DataType::float64(5));
            n["values"]["y"].set(DataType::float64(5));
            CHECK_MESH(verify_field,n,info,true);

            n["values"].set(values);
            CHECK_MESH(verify_field,n,info,true);
        }

        { // Association/Basis Field Tests //
            n.remove("association");
            CHECK_MESH(verify_field,n,info,false);

            n["association"].set("zone");
            CHECK_MESH(verify_field,n,info,false);
            n["association"].set("vertex");
            CHECK_MESH(verify_field,n,info,true);

            n.remove("association");
            n["basis"].set(0);
            CHECK_MESH(verify_field,n,info,false);
            n["basis"].set("basis");
            CHECK_MESH(verify_field,n,info,true);

            n["association"].set("vertex");
            CHECK_MESH(verify_field,n,info,true);
        }
    }
}

/// Mesh Domain Adjacencies Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, adjset_general)
{
    VerifyFun verify_adjset_funs[] = {
        blueprint::mesh::adjset::verify,
        verify_adjset_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_adjset = verify_adjset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_adjset,mesh,info,false);

        blueprint::mesh::examples::grid("quads",10,10,0,2,2,1,mesh);
        Node& n = mesh.child(0)["adjsets"].child(0);
        CHECK_MESH(verify_adjset,n,info,true);

        { // Topology Field Tests //
            n.remove("topology");
            CHECK_MESH(verify_adjset,n,info,false);
            n["topology"].set(10);
            CHECK_MESH(verify_adjset,n,info,false);
            n["topology"].set("mesh");
            CHECK_MESH(verify_adjset,n,info,true);
        }

        { // Groups Field Tests //
            Node groups = n["groups"];

            n.remove("groups");
            CHECK_MESH(verify_adjset,n,info,false);
            n["groups"].set("groups");
            CHECK_MESH(verify_adjset,n,info,false);
            n["groups"].set(DataType::float64(10));
            CHECK_MESH(verify_adjset,n,info,false);

            n["groups"].reset();
            n["groups"]["g1"].set("Hello, ");
            CHECK_MESH(verify_adjset,n,info,false);
            n["groups"]["g2"].set("World!");
            CHECK_MESH(verify_adjset,n,info,false);

            n["groups"].reset();
            n["groups"]["g1"]["neighbors"].set(DataType::int32(5));
            CHECK_MESH(verify_adjset,n,info,true);
            n["groups"]["g1"]["values"].set(DataType::float32(5));
            CHECK_MESH(verify_adjset,n,info,false);
            n["groups"]["g1"]["values"].set(DataType::int32(5));
            CHECK_MESH(verify_adjset,n,info,true);

            n["groups"].reset();
            n["groups"]["g1"]["neighbors"].set(DataType::int32(5));
            n["groups"]["g1"]["values"].set(DataType::int32(5));
            CHECK_MESH(verify_adjset,n,info,true);
            n["groups"]["g2"]["neighbors"].set(DataType::int32(5));
            n["groups"]["g2"]["values"].set(DataType::int32(5));
            CHECK_MESH(verify_adjset,n,info,true);

            n["groups"].reset();
            n["groups"].set(groups);
            CHECK_MESH(verify_adjset,n,info,true);
        }

        { // Association Field Tests //
            n.remove("association");
            CHECK_MESH(verify_adjset,n,info,false);

            n["association"].set("zone");
            CHECK_MESH(verify_adjset,n,info,false);
            n["association"].set("vertex");
            CHECK_MESH(verify_adjset,n,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, adjset_structured)
{
    VerifyFun verify_adjset_funs[] = {
        blueprint::mesh::adjset::verify,
        verify_adjset_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_adjset = verify_adjset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_adjset,mesh,info,false);

        blueprint::mesh::examples::adjset_uniform(mesh);

        NodeConstIterator itr = mesh.children();
        while(itr.has_next())
        {
           const Node &chld= itr.next();
           const std::string chld_name = itr.name();

           const Node& n = chld["adjsets/adjset"];
           CHECK_MESH(verify_adjset,n,info[chld_name],true);
        }
    }
}

/// Mesh Domain Nesting (AMR) Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, nestset_types)
{
    VerifyFun verify_nestset = blueprint::mesh::nestset::verify;

    Node n, info;

    const std::string nestset_types[] = {"parent", "child"};
    for(index_t ti = 0; ti < 2; ti++)
    {
        n.reset();
        blueprint::mesh::examples::misc("nestsets",5,5,1,n);
        Node& nestset_node = n.child(0)["nestsets/mesh_nest"];
        Node& window_node = nestset_node["windows"].child(0);
        CHECK_MESH(verify_nestset,nestset_node,info,true);

        window_node["domain_type"].set(0);
        CHECK_MESH(verify_nestset,nestset_node,info,false);

        window_node["domain_type"].set("ancestor");
        CHECK_MESH(verify_nestset,nestset_node,info,false);

        window_node["domain_type"].set(nestset_types[ti]);
        CHECK_MESH(verify_nestset,nestset_node,info,true);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, nestset_general)
{
    VerifyFun verify_nestset_funs[] = {
        blueprint::mesh::nestset::verify,
        verify_nestset_protocol};
    const std::string nestset_logical_fields[3] = {"ratio", "origin", "dims"};

    Node logical_template, window_template_slim, window_template_full;
    {
        logical_template["i"].set(DataType::int32(1));
        logical_template["j"].set(DataType::int32(1));

        window_template_slim["domain_id"].set(DataType::int32(1));
        window_template_slim["domain_type"].set("child");
        window_template_slim["ratio"].set(logical_template);

        window_template_full.set(window_template_slim);
        window_template_full["origin"].set(logical_template);
        window_template_full["dims"].set(logical_template);
    }

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_nestset = verify_nestset_funs[fi];

        Node mesh, info;
        CHECK_MESH(verify_nestset,mesh,info,false);

        blueprint::mesh::examples::misc("nestsets",5,5,1,mesh);
        Node& n = mesh.child(0)["nestsets"].child(0);
        CHECK_MESH(verify_nestset,n,info,true);

        { // Topology Field Tests //
            n.remove("topology");
            CHECK_MESH(verify_nestset,n,info,false);
            n["topology"].set(10);
            CHECK_MESH(verify_nestset,n,info,false);
            n["topology"].set("mesh");
            CHECK_MESH(verify_nestset,n,info,true);
        }

        { // Windows Field Tests //
            Node windows = n["windows"];

            n.remove("windows");
            CHECK_MESH(verify_nestset,n,info,false);
            n["windows"].set("windows");
            CHECK_MESH(verify_nestset,n,info,false);
            n["windows"].set(DataType::float64(10));
            CHECK_MESH(verify_nestset,n,info,false);

            n["windows"].reset();
            n["windows"]["w1"].set("Hello, ");
            CHECK_MESH(verify_nestset,n,info,false);
            n["windows"]["w2"].set("World!");
            CHECK_MESH(verify_nestset,n,info,false);

            n["windows"].reset();
            n["windows"]["w1"].set(window_template_slim);
            CHECK_MESH(verify_nestset,n,info,true);
            n["windows"]["w1"]["domain_id"].set(DataType::float32(1));
            CHECK_MESH(verify_nestset,n,info,false);

            n["windows"].reset();
            n["windows"]["w1"].set(window_template_slim);
            CHECK_MESH(verify_nestset,n,info,true);
            n["windows"]["w1"]["domain_type"].set(0);
            CHECK_MESH(verify_nestset,n,info,false);
            n["windows"]["w1"]["domain_type"].set("ancestor");
            CHECK_MESH(verify_nestset,n,info,false);

            for(index_t fi = 0; fi < 3; fi++)
            {
                const std::string& logical_field = nestset_logical_fields[fi];

                n["windows"].reset();
                n["windows"]["w1"].set(window_template_full);
                CHECK_MESH(verify_nestset,n,info,true);
                n["windows"]["w1"][logical_field].set(0);
                CHECK_MESH(verify_nestset,n,info,false);
                n["windows"]["w1"][logical_field].reset();
                n["windows"]["w1"][logical_field]["i"].set(DataType::int32(1));
                CHECK_MESH(verify_nestset,n,info,false);
                n["windows"]["w1"][logical_field]["j"].set(DataType::int32(1));
                CHECK_MESH(verify_nestset,n,info,true);
            }

            n["windows"].reset();
            n["windows"]["w1"].set(window_template_full);
            CHECK_MESH(verify_nestset,n,info,true);
            n["windows"]["w2"].set(window_template_slim);
            CHECK_MESH(verify_nestset,n,info,true);
            n["windows"]["w3"].set(window_template_full);
            CHECK_MESH(verify_nestset,n,info,true);

            n["windows"].reset();
            n["windows"].set(windows);
            CHECK_MESH(verify_nestset,n,info,true);
        }

        { // Association Field Tests //
            n.remove("association");
            CHECK_MESH(verify_nestset,n,info,false);

            n["association"].set("zone");
            CHECK_MESH(verify_nestset,n,info,false);
            n["association"].set("element");
            CHECK_MESH(verify_nestset,n,info,true);
        }
    }
}

/// Mesh Index Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_coordset)
{
    VerifyFun verify_coordset_index_funs[] = {
        blueprint::mesh::coordset::index::verify,
        verify_coordset_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_coordset_index = verify_coordset_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_coordset_index,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        Node& cindex = index["coordsets"]["coords"];
        CHECK_MESH(verify_coordset_index,cindex,info,true);

        { // Type Field Tests //
            cindex.remove("type");
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["type"].set("undefined");
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["type"].set("explicit");
            CHECK_MESH(verify_coordset_index,cindex,info,true);
        }

        { // Coord System Field Tests //
            Node coordsys = cindex["coord_system"];
            cindex.remove("coord_system");

            CHECK_MESH(verify_coordset_index,cindex,info,false);
            cindex["coord_system"].set("invalid");
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["coord_system"].reset();
            cindex["coord_system"]["type"].set("logical");
            cindex["coord_system"]["axes"]["i"].set(10);
            cindex["coord_system"]["axes"]["j"].set(10);
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["coord_system"].reset();
            cindex["coord_system"].set(coordsys);
            CHECK_MESH(verify_coordset_index,cindex,info,true);
        }

        { // Path Field Tests //
            cindex.remove("path");
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["path"].set(5);
            CHECK_MESH(verify_coordset_index,cindex,info,false);

            cindex["path"].set("path");
            CHECK_MESH(verify_coordset_index,cindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_topology)
{
    VerifyFun verify_topo_index_funs[] = {
        blueprint::mesh::topology::index::verify,
        verify_topology_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_topo_index = verify_topo_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_topo_index,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        Node& tindex = index["topologies"]["mesh"];
        CHECK_MESH(verify_topo_index,tindex,info,true);

        { // Type Field Tests //
            tindex.remove("type");
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["type"].set("undefined");
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["type"].set("unstructured");
            CHECK_MESH(verify_topo_index,tindex,info,true);
        }

        { // Coordset Field Tests //
            tindex.remove("coordset");
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["coordset"].set(0);
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["coordset"].set("path");
            CHECK_MESH(verify_topo_index,tindex,info,true);
        }

        { // Path Field Tests //
            tindex.remove("path");
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["path"].set(5);
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["path"].set("path");
            CHECK_MESH(verify_topo_index,tindex,info,true);
        }

        { // Grid Function Field Tests //
            tindex["grid_function"].set(10);
            CHECK_MESH(verify_topo_index,tindex,info,false);

            tindex["grid_function"].set("path");
            CHECK_MESH(verify_topo_index,tindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_matset)
{
    VerifyFun verify_matset_index_funs[] = {
        blueprint::mesh::matset::index::verify,
        verify_matset_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_matset_index = verify_matset_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_matset_index,mesh,info,false);

        blueprint::mesh::examples::misc("matsets",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        Node& mindex = index["matsets"]["mesh"];
        CHECK_MESH(verify_matset_index,mindex,info,true);

        { // Topology Field Tests //
            mindex.remove("topology");
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["topology"].set(0);
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["topology"].set("path");
            CHECK_MESH(verify_matset_index,mindex,info,true);
        }

        if(mindex.has_child("materials"))
        { // Materials Field Tests //
            mindex.remove("materials");
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["materials"];
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["materials/mat1"].set(1);
            CHECK_MESH(verify_matset_index,mindex,info,true);
            mindex["materials/mat2"].set(2);
            CHECK_MESH(verify_matset_index,mindex,info,true);
        }

        if(mindex.has_child("material_maps"))
        { // Materials Field Tests //
            mindex.remove("material_map");
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["material_map"];
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["material_map/mat1"].set(1);
            CHECK_MESH(verify_matset_index,mindex,info,true);
            mindex["material_map/mat2"].set(2);
            CHECK_MESH(verify_matset_index,mindex,info,true);
        }

        { // Path Field Tests //
            mindex.remove("path");
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["path"].set(5);
            CHECK_MESH(verify_matset_index,mindex,info,false);

            mindex["path"].set("path");
            CHECK_MESH(verify_matset_index,mindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_specset)
{
    VerifyFun verify_specset_index_funs[] = {
        blueprint::mesh::specset::index::verify,
        verify_specset_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_specset_index = verify_specset_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_specset_index,mesh,info,false);

        blueprint::mesh::examples::misc("specsets",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        Node& mindex = index["specsets"]["mesh"];
        CHECK_MESH(verify_specset_index,mindex,info,true);

        { // Matset Field Tests //
            mindex.remove("matset");
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["matset"].set(0);
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["matset"].set("path");
            CHECK_MESH(verify_specset_index,mindex,info,true);
        }

        { // Species Field Tests //
            mindex.remove("species");
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["species"];
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["species/mat1/spec1"].set(1);
            CHECK_MESH(verify_specset_index,mindex,info,true);
            mindex["species/mat1/spec2"].set(2);
            CHECK_MESH(verify_specset_index,mindex,info,true);

            mindex["species/mat2/spec1"].set(1);
            CHECK_MESH(verify_specset_index,mindex,info,true);
            mindex["species/mat2/spec2"].set(2);
            CHECK_MESH(verify_specset_index,mindex,info,true);
        }

        { // Path Field Tests //
            mindex.remove("path");
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["path"].set(5);
            CHECK_MESH(verify_specset_index,mindex,info,false);

            mindex["path"].set("path");
            CHECK_MESH(verify_specset_index,mindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_field)
{
    VerifyFun verify_field_index_funs[] = {
        blueprint::mesh::field::index::verify,
        verify_field_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_field_index = verify_field_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_field_index,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        Node& findex = index["fields"]["braid"];
        CHECK_MESH(verify_field_index,findex,info,true);

        { // Topology Field Tests //
            Node topo = findex["topology"];
            findex.remove("topology");

            CHECK_MESH(verify_field_index,findex,info,false);
            findex["topology"].set(0);
            CHECK_MESH(verify_field_index,findex,info,false);

            findex["topology"].set("path");
            CHECK_MESH(verify_field_index,findex,info,true);

            findex["topology"].reset();
            findex["topology"].set(topo);
            CHECK_MESH(verify_field_index,findex,info,true);
        }

        { // Component Count Field Tests //
            Node comps = findex["number_of_components"];
            findex.remove("number_of_components");

            CHECK_MESH(verify_field_index,findex,info,false);
            findex["number_of_components"].set("three");
            CHECK_MESH(verify_field_index,findex,info,false);

            findex["number_of_components"].set(3);
            CHECK_MESH(verify_field_index,findex,info,true);

            findex["number_of_components"].reset();
            findex["number_of_components"].set(comps);
            CHECK_MESH(verify_field_index,findex,info,true);
        }

        { // Path Field Tests //
            Node path = findex["path"];
            findex.remove("path");

            CHECK_MESH(verify_field_index,findex,info,false);
            findex["path"].set(0);
            CHECK_MESH(verify_field_index,findex,info,false);

            findex["path"].set("path");
            CHECK_MESH(verify_field_index,findex,info,true);

            findex["path"].reset();
            findex["path"].set(path);
            CHECK_MESH(verify_field_index,findex,info,true);
        }

        { // Association Field Tests //
            findex["association"].set("zone");
            CHECK_MESH(verify_field_index,findex,info,false);
            findex["association"].set("vertex");
            CHECK_MESH(verify_field_index,findex,info,true);

            findex.remove("association");
            findex["basis"].set(0);
            CHECK_MESH(verify_field_index,findex,info,false);
            findex["basis"].set("basis");
            CHECK_MESH(verify_field_index,findex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_adjset)
{
    VerifyFun verify_adjset_index_funs[] = {
        blueprint::mesh::adjset::index::verify,
        verify_adjset_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_adjset_index = verify_adjset_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_adjset_index,mesh,info,false);

        blueprint::mesh::examples::grid("quads",10,10,0,2,2,1,mesh);
        blueprint::mesh::generate_index(mesh["domain0"],"quads",1,index);
        Node& aindex = index["adjsets"].child(0);
        CHECK_MESH(verify_adjset_index,aindex,info,true);

        { // Topology Field Tests //
            Node topo = aindex["topology"];
            aindex.remove("topology");

            CHECK_MESH(verify_adjset_index,aindex,info,false);
            aindex["topology"].set(0);
            CHECK_MESH(verify_adjset_index,aindex,info,false);

            aindex["topology"].set("path");
            CHECK_MESH(verify_adjset_index,aindex,info,true);

            aindex["topology"].reset();
            aindex["topology"].set(topo);
            CHECK_MESH(verify_adjset_index,aindex,info,true);
        }

        { // Path Field Tests //
            Node path = aindex["path"];
            aindex.remove("path");

            CHECK_MESH(verify_adjset_index,aindex,info,false);
            aindex["path"].set(0);
            CHECK_MESH(verify_adjset_index,aindex,info,false);

            aindex["path"].set("path");
            CHECK_MESH(verify_adjset_index,aindex,info,true);

            aindex["path"].reset();
            aindex["path"].set(path);
            CHECK_MESH(verify_adjset_index,aindex,info,true);
        }

        { // Association Field Tests //
            Node assoc = aindex["association"];
            aindex.remove("association");

            aindex["association"].set("zone");
            CHECK_MESH(verify_adjset_index,aindex,info,false);
            aindex["association"].set("vertex");
            CHECK_MESH(verify_adjset_index,aindex,info,true);

            aindex["association"].reset();
            aindex["association"].set(assoc);
            CHECK_MESH(verify_adjset_index,aindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_nestset)
{
    VerifyFun verify_nestset_index_funs[] = {
        blueprint::mesh::nestset::index::verify,
        verify_nestset_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_nestset_index = verify_nestset_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_nestset_index,mesh,info,false);

        blueprint::mesh::examples::misc("nestsets",5,5,0,mesh);
        blueprint::mesh::generate_index(mesh["domain0"],"quads",1,index);
        Node& aindex = index["nestsets"].child(0);
        CHECK_MESH(verify_nestset_index,aindex,info,true);

        { // Topology Field Tests //
            Node topo = aindex["topology"];
            aindex.remove("topology");

            CHECK_MESH(verify_nestset_index,aindex,info,false);
            aindex["topology"].set(0);
            CHECK_MESH(verify_nestset_index,aindex,info,false);

            aindex["topology"].set("path");
            CHECK_MESH(verify_nestset_index,aindex,info,true);

            aindex["topology"].reset();
            aindex["topology"].set(topo);
            CHECK_MESH(verify_nestset_index,aindex,info,true);
        }

        { // Path Field Tests //
            Node path = aindex["path"];
            aindex.remove("path");

            CHECK_MESH(verify_nestset_index,aindex,info,false);
            aindex["path"].set(0);
            CHECK_MESH(verify_nestset_index,aindex,info,false);

            aindex["path"].set("path");
            CHECK_MESH(verify_nestset_index,aindex,info,true);

            aindex["path"].reset();
            aindex["path"].set(path);
            CHECK_MESH(verify_nestset_index,aindex,info,true);
        }

        { // Association Field Tests //
            Node assoc = aindex["association"];
            aindex.remove("association");

            aindex["association"].set("zone");
            CHECK_MESH(verify_nestset_index,aindex,info,false);
            aindex["association"].set("vertex");
            CHECK_MESH(verify_nestset_index,aindex,info,true);

            aindex["association"].reset();
            aindex["association"].set(assoc);
            CHECK_MESH(verify_nestset_index,aindex,info,true);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, index_general)
{
    VerifyFun verify_index_funs[] = {
        blueprint::mesh::index::verify,
        verify_index_protocol};

    for(index_t fi = 0; fi < 2; fi++)
    {
        VerifyFun verify_index = verify_index_funs[fi];

        Node mesh, index, info;
        CHECK_MESH(verify_index,mesh,info,false);

        blueprint::mesh::examples::braid("quads",10,10,0,mesh);
        blueprint::mesh::generate_index(mesh,"quads",1,index);
        CHECK_MESH(verify_index,index,info,true);

        { // Coordsets Field Tests //
            info.reset();
            Node coords = index["coordsets"];
            index.remove("coordsets");

            CHECK_MESH(verify_index,index,info,false);
            index["coordsets"].set("coords");
            CHECK_MESH(verify_index,index,info,false);

            index["coordsets"].reset();
            index["coordsets"]["coords1"].set("coords");
            index["coordsets"]["coords2"].set("coords");
            CHECK_MESH(verify_index,index,info,false);

            index["coordsets"].reset();
            index["coordsets"].set(coords);
            CHECK_MESH(verify_index,index,info,true);
        }

        { // Topologies Field Tests //
            info.reset();
            Node topos = index["topologies"];
            index.remove("topologies");

            CHECK_MESH(verify_index,index,info,false);
            index["topologies"].set("topo");
            CHECK_MESH(verify_index,index,info,false);

            index["topologies"].reset();
            index["topologies"]["topo1"].set("topo");
            index["topologies"]["topo2"].set("topo");
            CHECK_MESH(verify_index,index,info,false);

            index["topologies"].reset();
            index["topologies"]["mesh"]["type"].set("invalid");
            index["topologies"]["mesh"]["path"].set("quads/topologies/mesh");
            index["topologies"]["mesh"]["coordset"].set("coords");
            CHECK_MESH(verify_index,index,info,false);
            index["topologies"]["mesh"]["type"].set("unstructured");
            CHECK_MESH(verify_index,index,info,true);

            index["topologies"]["mesh"]["coordset"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["topologies"]["mesh"]["coordset"].set("coords");
            CHECK_MESH(verify_index,index,info,true);

            index["coordsets"]["coords"]["type"].set("invalid");
            CHECK_MESH(verify_index,index,info,false);
            index["coordsets"]["coords"]["type"].set("explicit");
            CHECK_MESH(verify_index,index,info,true);

            index["topologies"].reset();
            index["topologies"].set(topos);
            CHECK_MESH(verify_index,index,info,true);
        }

        { // Matsets Field Tests //
            info.reset();
            Node matsets = index["matsets"];
            index.remove("matsets");
            CHECK_MESH(verify_index,index,info,true);

            index["matsets"].set("matset");
            CHECK_MESH(verify_index,index,info,false);

            index["matsets"].reset();
            index["matsets"]["matset1"].set("matset1");
            index["matsets"]["matset1"].set("matset2");
            CHECK_MESH(verify_index,index,info,false);

            index["matsets"].reset();
            index["matsets"]["matset"]["topology"].set("mesh");
            index["matsets"]["matset"]["materials"].set("invalid");
            index["matsets"]["matset"]["path"].set("quads/matsets/matset");
            CHECK_MESH(verify_index,index,info,false);
            index["matsets"]["matset"]["materials"]["mat1"];
            CHECK_MESH(verify_index,index,info,true);
            index["matsets"]["matset"]["materials"]["mat2"];
            CHECK_MESH(verify_index,index,info,true);

            index["matsets"]["matset"]["topology"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["matsets"]["matset"]["topology"].set("mesh");
            CHECK_MESH(verify_index,index,info,true);

            // TODO(JRC): Change this code so that the "matsets" section is
            // re-added once it's included in the test Blueprint mesh.
            index["matsets"].reset();
            index.remove("matsets");
            CHECK_MESH(verify_index,index,info,true);
        }

        { // Fields Field Tests //
            info.reset();
            Node fields = index["fields"];
            index.remove("fields");
            CHECK_MESH(verify_index,index,info,true);

            index["fields"].set("field");
            CHECK_MESH(verify_index,index,info,false);

            index["fields"].reset();
            index["fields"]["field1"].set("field1");
            index["fields"]["field1"].set("field2");
            CHECK_MESH(verify_index,index,info,false);

            index["fields"].reset();
            index["fields"]["field"]["number_of_components"].set("invalid");
            index["fields"]["field"]["association"].set("vertex");
            index["fields"]["field"]["path"].set("quads/fields/braid");
            index["fields"]["field"]["topology"].set("mesh");
            CHECK_MESH(verify_index,index,info,false);
            index["fields"]["field"]["number_of_components"].set(1);
            CHECK_MESH(verify_index,index,info,true);

            index["fields"]["field"]["topology"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["fields"]["field"]["topology"].set("mesh");
            CHECK_MESH(verify_index,index,info,true);

            index["topologies"]["mesh"]["type"].set("invalid");
            CHECK_MESH(verify_index,index,info,false);
            index["topologies"]["mesh"]["type"].set("unstructured");
            CHECK_MESH(verify_index,index,info,true);

            index["fields"].reset();
            index["fields"].set(fields);
            CHECK_MESH(verify_index,index,info,true);
        }

        { // Adjsets Field Tests //
            info.reset();
            Node adjsets = index["adjsets"];
            index.remove("adjsets");
            CHECK_MESH(verify_index,index,info,true);

            index["adjsets"].set("adjset");
            CHECK_MESH(verify_index,index,info,false);

            index["adjsets"].reset();
            index["adjsets"]["adjset1"].set("adjset1");
            index["adjsets"]["adjset1"].set("adjset2");
            CHECK_MESH(verify_index,index,info,false);

            index["adjsets"].reset();
            index["adjsets"]["adjset"]["topology"].set("mesh");
            index["adjsets"]["adjset"]["association"].set("vertex");
            index["adjsets"]["adjset"]["path"].set(0);
            CHECK_MESH(verify_index,index,info,false);
            index["adjsets"]["adjset"]["path"].set("quads/adjsets/adjset");
            CHECK_MESH(verify_index,index,info,true);

            index["adjsets"]["adjset"]["topology"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["adjsets"]["adjset"]["topology"].set("mesh");
            CHECK_MESH(verify_index,index,info,true);

            index["adjsets"]["adjset"]["association"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["adjsets"]["adjset"]["association"].set("element");
            CHECK_MESH(verify_index,index,info,true);

            // TODO(JRC): Change this code so that the "adjsets" section is
            // re-added once it's included in the test Blueprint mesh.
            index["adjsets"].reset();
            index.remove("adjsets");
            CHECK_MESH(verify_index,index,info,true);
        }

        { // Nestsets Field Tests //
            info.reset();
            Node nestsets = index["nestsets"];
            index.remove("nestsets");
            CHECK_MESH(verify_index,index,info,true);

            index["nestsets"].set("nestset");
            CHECK_MESH(verify_index,index,info,false);

            index["nestsets"].reset();
            index["nestsets"]["nestset1"].set("nestset1");
            index["nestsets"]["nestset1"].set("nestset2");
            CHECK_MESH(verify_index,index,info,false);

            index["nestsets"].reset();
            index["nestsets"]["nestset"]["topology"].set("mesh");
            index["nestsets"]["nestset"]["association"].set("vertex");
            index["nestsets"]["nestset"]["path"].set(0);
            CHECK_MESH(verify_index,index,info,false);
            index["nestsets"]["nestset"]["path"].set("quads/nestsets/nestset");
            CHECK_MESH(verify_index,index,info,true);

            index["nestsets"]["nestset"]["topology"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["nestsets"]["nestset"]["topology"].set("mesh");
            CHECK_MESH(verify_index,index,info,true);

            index["nestsets"]["nestset"]["association"].set("nonexistent");
            CHECK_MESH(verify_index,index,info,false);
            index["nestsets"]["nestset"]["association"].set("element");
            CHECK_MESH(verify_index,index,info,true);

            // TODO(JRC): Change this code so that the "nestsets" section is
            // re-added once it's included in the test Blueprint mesh.
            index["nestsets"].reset();
            index.remove("nestsets");
            CHECK_MESH(verify_index,index,info,true);
        }
    }
}

/// Mesh Integration Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, mesh_multi_domain)
{
    Node mesh, info;
    EXPECT_TRUE(blueprint::mesh::verify(mesh,info) &&
                blueprint::mesh::is_multi_domain(mesh));
    EXPECT_TRUE(has_empty_warning(info));

    Node domains[2];
    blueprint::mesh::examples::braid("quads",10,10,0,domains[0]);
    blueprint::mesh::to_multi_domain(domains[0],mesh);
    EXPECT_TRUE(blueprint::mesh::is_multi_domain(mesh));

    blueprint::mesh::examples::braid("quads",5,5,0,domains[1]);
    mesh.append().set_external(domains[1]);
    EXPECT_TRUE(blueprint::mesh::is_multi_domain(mesh));

    { // Redundant "to_multi_domain" Tests //
        Node temp;
        blueprint::mesh::to_multi_domain(mesh,temp);
        EXPECT_TRUE(blueprint::mesh::is_multi_domain(temp));
    }

    for(index_t di = 0; di < 2; di++)
    {
        Node& domain = mesh.child(di);
        EXPECT_FALSE(blueprint::mesh::is_multi_domain(domain));

        // is_multi_domain can only be called if mesh verify is true
        Node coordsets = domain["coordsets"];
        domain.remove("coordsets");
        EXPECT_FALSE(blueprint::mesh::verify(mesh,info) &&
                     blueprint::mesh::is_multi_domain(mesh));

        domain["coordsets"].reset();
        domain["coordsets"].set(coordsets);
        EXPECT_TRUE(blueprint::mesh::is_multi_domain(mesh));
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, mesh_general)
{
    VerifyFun verify_mesh_funs[] = {
        blueprint::mesh::verify, // single_domain verify
        blueprint::mesh::verify, // multi_domain verify
        verify_mesh_multi_domain_protocol};

    for(index_t fi = 0; fi < 3; fi++)
    {
        VerifyFun verify_mesh = verify_mesh_funs[fi];

        Node mesh, mesh_data, info;
        CHECK_MESH(verify_mesh,mesh,info,true);
        EXPECT_TRUE(has_empty_warning(info));

        blueprint::mesh::examples::braid("quads",10,10,0,mesh_data);

        Node* domain_ptr = NULL;
        if(fi == 0)
        {
            mesh.set_external(mesh_data);
            domain_ptr = &mesh;
        }
        else
        {
            blueprint::mesh::to_multi_domain(mesh_data,mesh);
            domain_ptr = &mesh.child(0);
        }
        Node& domain = *domain_ptr;

        CHECK_MESH(verify_mesh,mesh,info,true);
        EXPECT_FALSE(has_empty_warning(info));
        // info.print();

        { // Coordsets Field Tests //
            Node coordsets = domain["coordsets"];
            domain.remove("coordsets");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["coordsets"].set("path");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["coordsets"].reset();
            domain["coordsets"]["coords"]["type"].set("invalid");
            domain["coordsets"]["coords"]["values"]["x"].set(DataType::float64(10));
            domain["coordsets"]["coords"]["values"]["y"].set(DataType::float64(10));
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["coordsets"]["coords"]["type"].set("explicit");
            CHECK_MESH(verify_mesh,mesh,info,true);
            domain["coordsets"]["coords2"]["type"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["coordsets"].reset();
            domain["coordsets"].set(coordsets);
            CHECK_MESH(verify_mesh,mesh,info,true);
        }

        { // Topologies Field Tests //
            Node topologies = domain["topologies"];
            domain.remove("topologies");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["topologies"].set("path");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["topologies"].reset();
            domain["topologies"]["mesh"]["type"].set("invalid");
            domain["topologies"]["mesh"]["coordset"].set("coords");
            domain["topologies"]["mesh"]["elements"]["shape"].set("quad");
            domain["topologies"]["mesh"]["elements"]["connectivity"].set(DataType::int32(10));
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["topologies"]["mesh"]["type"].set("unstructured");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["coordsets"]["coords"]["type"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["coordsets"]["coords"]["type"].set("explicit");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["topologies"]["mesh"]["coordset"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["topologies"]["mesh"]["coordset"].set("coords");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["topologies"]["grid"]["type"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["topologies"].reset();
            domain["topologies"].set(topologies);
            CHECK_MESH(verify_mesh,mesh,info,true);
        }

        { // Matsets Field Tests //
            Node matsets = domain["matsets"];
            domain.remove("matsets");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["matsets"].set("path");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["matsets"].reset();
            domain["matsets"]["mesh"]["topology"].set("mesh");
            domain["matsets"]["mesh"]["volume_fractions"];
            CHECK_MESH(verify_mesh,mesh,info,false);

            Node &vfs = domain["matsets"]["mesh"]["volume_fractions"];
            vfs["mat1"].set(DataType::float32(10));
            CHECK_MESH(verify_mesh,mesh,info,true);
            vfs["mat2"].set(DataType::float32(10));
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["matsets"]["mesh"]["topology"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["matsets"]["mesh"]["topology"].set("mesh");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["matsets"]["boundary"]["topology"].set("mesh");
            CHECK_MESH(verify_mesh,mesh,info,false);

            // TODO(JRC): Change this code so that the "matsets" section is
            // re-added once it's included in the test Blueprint mesh.
            domain["matsets"].reset();
            domain.remove("matsets");
            CHECK_MESH(verify_mesh,mesh,info,true);
        }

        { // Fields Field Tests //
            Node fields = domain["fields"];
            domain.remove("fields");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["fields"].set("path");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["fields"].reset();
            domain["fields"]["temp"]["association"].set("invalid");
            domain["fields"]["temp"]["topology"].set("mesh");
            domain["fields"]["temp"]["values"].set(DataType::float64(10));
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["fields"]["temp"]["association"].set("vertex");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["topologies"]["mesh"]["type"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["topologies"]["mesh"]["type"].set("unstructured");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["fields"]["temp"]["topology"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["fields"]["temp"]["topology"].set("mesh");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["fields"]["accel"]["association"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["fields"].reset();
            domain["fields"].set(fields);
            CHECK_MESH(verify_mesh,mesh,info,true);
        }

        { // Grid Function Field Tests //
            Node topologies = domain["topologies"];
            Node fields = domain["fields"];
            domain.remove("fields");

            domain["topologies"]["mesh"]["grid_function"].set("braid");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["fields"].set(fields);
            domain["topologies"]["mesh"]["grid_function"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["topologies"]["mesh"]["grid_function"].set("braid");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["fields"]["braid"]["association"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["fields"]["braid"]["association"].set("vertex");

            domain["topologies"].reset();
            domain["topologies"].set(topologies);
            CHECK_MESH(verify_mesh,mesh,info,true);
        }

        { // Adjsets Field Tests //
            Node adjsets = domain["adjsets"];
            domain.remove("adjsets");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["adjsets"].set("path");
            CHECK_MESH(verify_mesh,mesh,info,false);

            domain["adjsets"].reset();
            domain["adjsets"]["mesh"]["association"].set("vertex");
            domain["adjsets"]["mesh"]["topology"].set("mesh");
            domain["adjsets"]["mesh"]["groups"];
            CHECK_MESH(verify_mesh,mesh,info,false);

            Node &groups = domain["adjsets"]["mesh"]["groups"];
            groups["g1"]["neighbors"].set(DataType::int32(10));
            CHECK_MESH(verify_mesh,mesh,info,true);
            groups["g1"]["values"].set(DataType::float32(10));
            CHECK_MESH(verify_mesh,mesh,info,false);
            groups["g1"]["values"].set(DataType::int32(10));
            CHECK_MESH(verify_mesh,mesh,info,true);
            groups["g2"].set(groups["g1"]);
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["adjsets"]["mesh"]["topology"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["adjsets"]["mesh"]["topology"].set("mesh");
            CHECK_MESH(verify_mesh,mesh,info,true);

            domain["adjsets"]["mesh"]["association"].set("invalid");
            CHECK_MESH(verify_mesh,mesh,info,false);
            domain["adjsets"]["mesh"]["association"].set("element");
            CHECK_MESH(verify_mesh,mesh,info,true);

            // TODO(JRC): Change this code so that the "adjsets" section is
            // re-added once it's included in the test Blueprint mesh.
            domain["adjsets"].reset();
            domain.remove("adjsets");
            CHECK_MESH(verify_mesh,mesh,info,true);
        }
    }
}

// `logical` is not currently a valid coordinate system type 
// (cartesian, spherical, etc), however if uniform mesh is presented
// w/o an origin, the blueprint index gen code is selecting `logical`
// which results in the generated index to fail verify
// we resolved this by 
//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, simple_uniform_check_index)
{
    Node mesh;
std::string mesh_yaml="\n"
"coordsets:\n"
"  coords:\n"
"    type: \"uniform\"\n"
"    dims:\n"
"      i: 513\n"
"      j: 513\n"
// "    origin:\n"
// "      x: 0.0\n"
// "      y: 0.0\n"
"topologies:\n"
"  topo:\n"
"    coordset: \"coords\"\n"
"    type: \"uniform\"\n";
    mesh.parse(mesh_yaml,"yaml");

    Node info;
    bool res = blueprint::mesh::verify(mesh,info);
    EXPECT_TRUE(res);
    Node bp_index;
    blueprint::mesh::generate_index(mesh,"",1,bp_index);

    res = blueprint::mesh::index::verify(bp_index,info);
    EXPECT_TRUE(res);
    std::cout << info.to_yaml() << std::endl;

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, mesh_bad_spacing_name)
{

    Node n_test;
    n_test["coordsets/coords/type"]= "uniform";
    n_test["coordsets/coords/dims/i"] = 10;
    n_test["coordsets/coords/dims/j"] = 10;
    n_test["coordsets/coords/dims/k"] = 10;
    n_test["coordsets/coords/spacing/x"] = 10;
    n_test["coordsets/coords/spacing/y"] = 10;
    n_test["coordsets/coords/spacing/z"] = 10;
    n_test["topologies/topo/coordset"] = "coords";
    n_test["topologies/topo/type"] = "uniform";
    Node info;
    bool res = blueprint::mesh::verify(n_test,info);
    // info.print();
    Node n_idx;
    blueprint::mesh::generate_index(n_test,"",1,n_idx);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_verify, empty_mesh_vs_gen_index)
{
    Node empty;
    Node info;
    bool res = blueprint::mesh::verify(empty,info);
    EXPECT_TRUE(res);
    // expect this to throw, while mesh bp can have empty domains
    // we can't create an index for the input domain case
    Node n_idx;
    EXPECT_THROW(blueprint::mesh::generate_index(empty,"",1,n_idx),conduit::Error);
    
    
}

