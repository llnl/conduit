// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_transform.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"

#include <algorithm>
#include <set>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::utils;
namespace bputils = conduit::blueprint::mesh::utils;

//-----------------------------------------------------------------------------
bool
check_if_hdf5_enabled()
{
    Node io_protos;
    relay::io::about(io_protos["io"]);
    return io_protos["io/protocols/hdf5"].as_string() == "enabled";
}

/// Testing Constants ///

typedef void (*XformCoordsFun)(const Node&, Node&);
typedef void (*XformTopoFun)(const Node&, Node&, Node&);
typedef bool (*VerifyFun)(const Node&, Node&);

/// Testing Helpers ///

index_t braid_bound_npts_z(const std::string &mesh_type, index_t npts_z)
{
    if(mesh_type == "tris"  ||
       mesh_type == "quads" ||
       mesh_type == "quads_poly" ||
       mesh_type == "quads_and_tris" ||
       mesh_type == "quads_and_tris_offsets")
    {
        return 0;
    }
    else
    {
        return npts_z;
    }
}

std::string get_braid_type(const std::string &mesh_type)
{
    std::string braid_type;
    try
    {
        Node mesh;
        blueprint::mesh::examples::braid(mesh_type,
                                         2,
                                         2,
                                         braid_bound_npts_z(mesh_type,2),
                                         mesh);
        braid_type = mesh_type;
    }
    catch(conduit::Error &) // actual exception is unused
    {
        braid_type = "hexs";
    }

    return braid_type;
}



// TODO(JRC): It would be useful to eventually have this type of procedure
// available as an abstracted iteration strategy within Conduit (e.g. leaf iterate).
void set_node_data(Node &node, const DataType &dtype)
{
    std::vector<Node*> node_bag(1, &node);
    while(!node_bag.empty())
    {
        Node* curr_node = node_bag.back(); node_bag.pop_back();
        DataType curr_dtype = curr_node->dtype();

        bool are_types_equivalent =
            (curr_dtype.is_floating_point() && dtype.is_floating_point()) ||
            (curr_dtype.is_integer() && dtype.is_integer()) ||
            (curr_dtype.is_string() && dtype.is_string());
        if(curr_dtype.is_object() || curr_dtype.is_list())
        {
            NodeIterator curr_node_it = curr_node->children();
            while(curr_node_it.has_next()) { node_bag.push_back(&curr_node_it.next()); }
        }
        else if(are_types_equivalent)
        {
            Node temp_node;
            curr_node->to_data_type(dtype.id(), temp_node);
            curr_node->set(temp_node);
        }
    }
}


bool verify_node_data(Node &node, const DataType &dtype)
{
    bool is_data_valid = true;

    std::vector<Node*> node_bag(1, &node);
    while(!node_bag.empty())
    {
        Node* curr_node = node_bag.back(); node_bag.pop_back();
        DataType curr_dtype = curr_node->dtype();

        bool are_types_equivalent =
            (curr_dtype.is_floating_point() && dtype.is_floating_point()) ||
            (curr_dtype.is_integer() && dtype.is_integer()) ||
            (curr_dtype.is_string() && dtype.is_string());
        if(curr_dtype.is_object() || curr_dtype.is_list())
        {
            NodeIterator curr_node_it = curr_node->children();
            while(curr_node_it.has_next()) { node_bag.push_back(&curr_node_it.next()); }
        }
        else if(are_types_equivalent)
        {
            is_data_valid &= curr_dtype.id() == dtype.id();
        }
    }

    return is_data_valid;
}

/// Transform Tests ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, coordset_transforms)
{
    XformCoordsFun xform_funs[3][3] = {
        {NULL, blueprint::mesh::coordset::uniform::to_rectilinear, blueprint::mesh::coordset::uniform::to_explicit},
        {NULL, NULL, blueprint::mesh::coordset::rectilinear::to_explicit},
        {NULL, NULL, NULL}};

    VerifyFun verify_funs[3] = {
        blueprint::mesh::coordset::uniform::verify,
        blueprint::mesh::coordset::rectilinear::verify,
        blueprint::mesh::coordset::_explicit::verify};

    for(size_t xi = 0; xi < bputils::COORD_TYPES.size(); xi++)
    {
        const std::string icoordset_type = bputils::COORD_TYPES[xi];
        const std::string icoordset_braid = get_braid_type(icoordset_type);

        Node imesh;
        blueprint::mesh::examples::braid(icoordset_braid,
                                         2,
                                         3,
                                         braid_bound_npts_z(icoordset_braid,4),
                                         imesh);
        const Node &icoordset = imesh["coordsets"].child(0);

        for(size_t xj = xi + 1; xj < bputils::COORD_TYPES.size(); xj++)
        {
            const std::string jcoordset_type = bputils::COORD_TYPES[xj];
            const std::string jcoordset_braid = get_braid_type(jcoordset_type);

            // NOTE: The following lines are for debugging purposes only.
            std::cout << "Testing coordset " << icoordset_type << " -> " <<
                jcoordset_type << "..." << std::endl;

            Node jmesh;
            blueprint::mesh::examples::braid(jcoordset_braid,
                                             2,
                                             3,
                                             braid_bound_npts_z(jcoordset_braid,4),
                                             jmesh);
            Node &jcoordset = jmesh["coordsets"].child(0);

            XformCoordsFun to_new_coordset = xform_funs[xi][xj];
            VerifyFun verify_new_coordset = verify_funs[xj];

            Node xcoordset, info;
            to_new_coordset(icoordset, xcoordset);

            EXPECT_TRUE(verify_new_coordset(xcoordset, info));
            EXPECT_FALSE(jcoordset.diff(xcoordset, info));
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, coordset_transform_dtypes)
{
    XformCoordsFun xform_funs[3][3] = {
        {NULL, blueprint::mesh::coordset::uniform::to_rectilinear, blueprint::mesh::coordset::uniform::to_explicit},
        {NULL, NULL, blueprint::mesh::coordset::rectilinear::to_explicit},
        {NULL, NULL, NULL}};

    for(size_t xi = 0; xi < bputils::COORD_TYPES.size(); xi++)
    {
        const std::string icoordset_type = bputils::COORD_TYPES[xi];
        const std::string icoordset_braid = get_braid_type(icoordset_type);

        Node imesh;
        blueprint::mesh::examples::braid(icoordset_braid,
                                         2,
                                         3,
                                         braid_bound_npts_z(icoordset_braid,4),
                                         imesh);
        const Node &icoordset = imesh["coordsets"].child(0);

        for(size_t xj = xi + 1; xj < bputils::COORD_TYPES.size(); xj++)
        {
            Node jcoordset;
            const std::string jcoordset_type = bputils::COORD_TYPES[xj];
            XformCoordsFun to_new_coordset = xform_funs[xi][xj];

            for(size_t ii = 0; ii < bputils::INT_DTYPES.size(); ii++)
            {
                for(size_t fi = 0; fi < bputils::FLOAT_DTYPES.size(); fi++)
                {
                    // NOTE: The following lines are for debugging purposes only.
                    std::cout << "Testing " <<
                        "int-" << 32 * (ii + 1) << "/float-" << 32 * (fi + 1) << " coordset " <<
                        icoordset_type << " -> " << jcoordset_type << "..." << std::endl;

                    Node icoordset = imesh["coordsets"].child(0);
                    Node jcoordset;

                    set_node_data(icoordset, bputils::INT_DTYPES[ii]);
                    set_node_data(icoordset, bputils::FLOAT_DTYPES[fi]);

                    to_new_coordset(icoordset, jcoordset);

                    EXPECT_TRUE(verify_node_data(jcoordset, bputils::INT_DTYPES[ii]));
                    EXPECT_TRUE(verify_node_data(jcoordset, bputils::FLOAT_DTYPES[fi]));
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, topology_transforms)
{
    XformTopoFun xform_funs[5][5] = {
        {NULL, NULL, NULL, NULL, NULL},
        {NULL, NULL, blueprint::mesh::topology::uniform::to_rectilinear, blueprint::mesh::topology::uniform::to_structured, blueprint::mesh::topology::uniform::to_unstructured},
        {NULL, NULL, NULL, blueprint::mesh::topology::rectilinear::to_structured, blueprint::mesh::topology::rectilinear::to_unstructured},
        {NULL, NULL, NULL, NULL, blueprint::mesh::topology::structured::to_unstructured},
        {NULL, NULL, NULL, NULL, NULL}};
    VerifyFun verify_topology_funs[] = {
        blueprint::mesh::topology::points::verify,
        blueprint::mesh::topology::uniform::verify,
        blueprint::mesh::topology::rectilinear::verify,
        blueprint::mesh::topology::structured::verify,
        blueprint::mesh::topology::unstructured::verify};
    VerifyFun verify_coordset_funs[] = {
        blueprint::mesh::coordset::verify,
        blueprint::mesh::coordset::uniform::verify,
        blueprint::mesh::coordset::rectilinear::verify,
        blueprint::mesh::coordset::_explicit::verify,
        blueprint::mesh::coordset::_explicit::verify};

    // NOTE(JRC): We skip the "points" topology during this general check
    // because its rules are peculiar and specific.
    for(size_t xi = 1; xi < bputils::TOPO_TYPES.size(); xi++)
    {
        const std::string itopology_type = bputils::TOPO_TYPES[xi];
        const std::string itopology_braid = get_braid_type(itopology_type);

        Node imesh;
        blueprint::mesh::examples::braid(itopology_braid,
                                         2,
                                         3,
                                         braid_bound_npts_z(itopology_braid,4),
                                         imesh);
        const Node &itopology = imesh["topologies"].child(0);
        const Node &icoordset = imesh["coordsets"].child(0);

        for(size_t xj = xi + 1; xj < bputils::TOPO_TYPES.size(); xj++)
        {
            const std::string jtopology_type = bputils::TOPO_TYPES[xj];
            const std::string jtopology_braid = get_braid_type(jtopology_type);

            // NOTE: The following lines are for debugging purposes only.
            std::cout << "Testing topology " << itopology_type << " -> " <<
                jtopology_type << "..." << std::endl;

            Node jmesh;
            blueprint::mesh::examples::braid(jtopology_braid,
                                             2,
                                             3,
                                             braid_bound_npts_z(jtopology_braid,4),
                                             jmesh);
            Node &jtopology = jmesh["topologies"].child(0);
            Node &jcoordset = jmesh["coordsets"].child(0);

            XformTopoFun to_new_topology = xform_funs[xi][xj];
            VerifyFun verify_new_topology = verify_topology_funs[xj];
            VerifyFun verify_new_coordset = verify_coordset_funs[xj];

            Node info;
            Node &xtopology = imesh["topologies/test"];
            Node &xcoordset = imesh["coordsets/test"];
            to_new_topology(itopology, xtopology, xcoordset);

            EXPECT_TRUE(verify_new_topology(xtopology, info));
            EXPECT_TRUE(verify_new_coordset(xcoordset, info));
            EXPECT_EQ(xtopology["coordset"].as_string(), xcoordset.name());

            // NOTE(JRC): This is necessary because the 'coordset' value
            // will be different from the transform topology since it
            // will always create a unique personal one and reference it.
            Node dxtopology = xtopology;
            dxtopology["coordset"].set(itopology["coordset"].as_string());

            EXPECT_FALSE(jtopology.diff(dxtopology, info));
            EXPECT_FALSE(jcoordset.diff(xcoordset, info));

            imesh["topologies"].remove("test");
            imesh["coordsets"].remove("test");
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, topology_transform_dtypes)
{
    XformTopoFun xform_funs[5][5] = {
        {NULL, NULL, NULL, NULL, NULL},
        {NULL, NULL, blueprint::mesh::topology::uniform::to_rectilinear, blueprint::mesh::topology::uniform::to_structured, blueprint::mesh::topology::uniform::to_unstructured},
        {NULL, NULL, NULL, blueprint::mesh::topology::rectilinear::to_structured, blueprint::mesh::topology::rectilinear::to_unstructured},
        {NULL, NULL, NULL, NULL, blueprint::mesh::topology::structured::to_unstructured},
        {NULL, NULL, NULL, NULL, NULL}};

    // NOTE(JRC): We skip the "points" topology during this general check
    // because its rules are peculiar and specific.
    for(size_t xi = 1; xi < bputils::TOPO_TYPES.size(); xi++)
    {
        const std::string itopology_type = bputils::TOPO_TYPES[xi];
        const std::string itopology_braid = get_braid_type(itopology_type);

        // NOTE(JRC): For the data type checks, we're only interested in the parts
        // of the subtree that are being transformed; we cull all other data.
        Node ibase;
        blueprint::mesh::examples::braid(itopology_braid,
                                         2,
                                         3,
                                         braid_bound_npts_z(itopology_braid,4),
                                         ibase);
        {
            Node temp;
            temp["coordsets"].set(ibase["coordsets"]);
            temp["topologies"].set(ibase["topologies"]);
            ibase.set(temp);
        }

        for(size_t xj = xi + 1; xj < bputils::TOPO_TYPES.size(); xj++)
        {
            const std::string jtopology_type = bputils::TOPO_TYPES[xj];
            XformTopoFun to_new_topology = xform_funs[xi][xj];

            for(size_t ii = 0; ii < bputils::INT_DTYPES.size(); ii++)
            {
                for(size_t fi = 0; fi < bputils::FLOAT_DTYPES.size(); fi++)
                {
                    // NOTE: The following lines are for debugging purposes only.
                    std::cout << "Testing " <<
                        "int-" << 32 * (ii + 1) << "/float-" << 32 * (fi + 1) << " topology " <<
                        itopology_type << " -> " << jtopology_type << "..." << std::endl;

                    Node imesh = ibase;
                    Node &itopology = imesh["topologies"].child(0);
                    Node &icoordset = imesh["coordsets"].child(0);

                    // FIXME(JRC): I think these should be references! i.e. Node &jtopology
                    // Changing them in this way causes this test to fail.
                    Node jmesh;
                    Node jtopology = jmesh["topologies"][itopology.name()];
                    Node jcoordset = jmesh["coordsets"][icoordset.name()];

                    set_node_data(imesh, bputils::INT_DTYPES[ii]);
                    set_node_data(imesh, bputils::FLOAT_DTYPES[fi]);

                    to_new_topology(itopology, jtopology, jcoordset);

                    EXPECT_TRUE(verify_node_data(jmesh, bputils::INT_DTYPES[ii]));
                    EXPECT_TRUE(verify_node_data(jmesh, bputils::FLOAT_DTYPES[fi]));
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, polygonal_transforms)
{
    // TODO(JRC): Refactor this code once 'ShapeType' and 'ShapeCascade' are
    // exposed internally via a utilities header.

    const std::string TOPO_TYPE_LIST[5]      = {"lines", "tris", "quads","tets","hexs"};
    const index_t TOPO_TYPE_INDICES[5]       = {      2,      3,       4,     4,     8};
    const index_t TOPO_TYPE_FACES[5]         = {      1,      1,       1,     4,     6};
    const index_t TOPO_TYPE_FACE_INDICES[5]  = {      2,      3,       4,     3,     4};

    const index_t MESH_DIMS[3] = {3, 3, 3};

    for(index_t ti = 0; ti < 5; ti++)
    {
        const std::string &topo_type = TOPO_TYPE_LIST[ti];
        const index_t &topo_indices = TOPO_TYPE_INDICES[ti];
        const index_t &topo_faces = TOPO_TYPE_FACES[ti];
        const index_t &topo_findices = TOPO_TYPE_FACE_INDICES[ti];
        const bool is_topo_3d = TOPO_TYPE_FACES[ti] > 1;

        // NOTE: The following lines are for debugging purposes only.
        std::cout << "Testing topology type '" << topo_type << "' -> " <<
            "polygonal..." << std::endl;

        Node topo_mesh, info;
        blueprint::mesh::examples::braid(topo_type,
                                         MESH_DIMS[0],
                                         MESH_DIMS[1],
                                         braid_bound_npts_z(topo_type,MESH_DIMS[2]),
                                         topo_mesh);
        const Node &topo_node = topo_mesh["topologies"].child(0);

        Node topo_poly;
        blueprint::mesh::topology::unstructured::to_polygonal(topo_node, topo_poly);

        { // Verify Non-Elements Components //
            Node topo_noelem, poly_noelem;
            topo_noelem.set_external(topo_node);
            topo_noelem.remove("elements");
            poly_noelem.set_external(topo_poly);
            poly_noelem.remove("elements");
            if (ti == 3 || ti == 4)
            {
                poly_noelem.remove("subelements");
            }
            EXPECT_FALSE(topo_noelem.diff(poly_noelem, info));
        }

        { // Verify Element Components //
            EXPECT_EQ(topo_poly["elements/shape"].as_string(),
                is_topo_3d ? "polyhedral" : "polygonal");

            const Node &topo_conn = topo_node["elements/connectivity"];
            Node &poly_conn = topo_poly["elements/connectivity"];
            Node poly_subconn;
            // BHAN - Error when trying to convert empty poly_subconn,
            // set to element/connectivity for polygonal (unused)
            if (is_topo_3d)
            {
                poly_subconn = topo_poly["subelements/connectivity"];
            }
            else
            {
                poly_subconn = topo_poly["elements/connectivity"];
            }
            EXPECT_EQ(poly_conn.dtype().id(), topo_conn.dtype().id());

            const index_t topo_len = topo_conn.dtype().number_of_elements();
            const index_t poly_len = poly_conn.dtype().number_of_elements();
            const index_t topo_elems = topo_len / topo_indices;
            const index_t poly_stride = poly_len / topo_elems;

            EXPECT_EQ(poly_stride, is_topo_3d ? topo_faces : topo_findices );
            EXPECT_EQ(poly_len % topo_elems, 0);

            Node topo_conn_array, poly_conn_array, poly_subconn_array;
            topo_conn.to_int64_array(topo_conn_array);
            poly_conn.to_int64_array(poly_conn_array);
            poly_subconn.to_int64_array(poly_subconn_array);
            const int64_array topo_data = topo_conn_array.as_int64_array();
            const int64_array poly_data = poly_conn_array.as_int64_array();
            const int64_array poly_subdata = poly_subconn_array.as_int64_array();

            Node poly_size;
            poly_size = topo_poly["elements/sizes"];

            // BHAN - Error when trying to convert empty poly_subsize,
            // set to element/sizes for polygonal (unused)
            Node poly_subsize;
            if (is_topo_3d)
            {
                poly_subsize = topo_poly["subelements/sizes"];
            }
            else
            {
                poly_subsize = topo_poly["elements/sizes"];
            }

            Node poly_size_array;
            poly_size.to_int64_array(poly_size_array);
            const int64_array poly_size_data = poly_size_array.as_int64_array();

            Node poly_subsize_array;
            poly_subsize.to_int64_array(poly_subsize_array);
            const int64_array poly_subsize_data = poly_subsize_array.as_int64_array();

            for(index_t ep = 0, et = 0; ep < poly_len;
                ep += poly_stride, et += topo_indices)
            {
                EXPECT_EQ(poly_size_data[ep / poly_stride],
                          is_topo_3d ? topo_faces : topo_findices);

                for(index_t efo = ep; efo < ep + poly_stride;
                    efo += is_topo_3d ? 1 : topo_findices)
                {
                    EXPECT_EQ(is_topo_3d ? poly_subsize_data[efo / poly_stride] :
                                           poly_size_data[efo / poly_stride],
                                           topo_findices);

                    const std::set<index_t> topo_index_set(
                        &topo_data[et],
                        &topo_data[et + topo_indices]);

                    std::set<index_t> poly_index_set;
                    if (is_topo_3d)
                    {
                        std::set<index_t> polyhedral_index_set(
                            &poly_subdata[poly_data[efo] * topo_findices],
                            &poly_subdata[poly_data[efo] * topo_findices + topo_findices]);

                        poly_index_set = polyhedral_index_set;
                    }
                    else
                    {
                        std::set<index_t> polygonal_index_set(
                            &poly_data[efo],
                            &poly_data[efo + topo_findices]);

                        poly_index_set = polygonal_index_set;
                    }
                    // set of face indices is completely unique (no duplicates)
                    EXPECT_EQ(poly_index_set.size(), topo_findices);
                    // all polygonal face indices can be found in the base element
                    EXPECT_TRUE(std::includes(
                        topo_index_set.begin(), topo_index_set.end(),
                        poly_index_set.begin(), poly_index_set.end()));
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, to_poly_alias_call)
{

    Node topo_mesh, info;
    blueprint::mesh::examples::braid("hexs",
                                     5,
                                     5,
                                     5,
                                     topo_mesh);
    const Node &topo_node = topo_mesh["topologies"].child(0);

    Node topo_poly_call1, topo_poly_call2;
    blueprint::mesh::topology::unstructured::to_polygonal(topo_node,
                                                          topo_poly_call1);
    blueprint::mesh::topology::unstructured::to_polytopal(topo_node,
                                                          topo_poly_call2);
    EXPECT_FALSE(topo_poly_call1.diff(topo_poly_call2, info));
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, adjset_transforms)
{
    // run test -> pairwise, then test -> pairwise -> maxshare
    // use different grid configurations, e.g.
    // {{2, 1, 1}, {2, 2, 1}, {2, 2, 2}}
    const std::string ADJSET_ELEM_TYPES[4]      = {"quads", "quads", "hexs", "hexs"};
    const index_t ADJSET_DOM_DIMS[4][3]         = {{2, 1, 1}, {2, 2, 1}, {2, 2, 1}, {2, 2, 2}};
    const index_t ADJSET_POINT_DIMS[4][3]       = {{3, 3, 0}, {3, 3, 0}, {3, 3, 3}, {3, 3, 3}};

    for(index_t ai = 0; ai < 4; ai++)
    {
        const std::string &adjset_etype = ADJSET_ELEM_TYPES[ai];
        const index_t *adjset_ddims = &ADJSET_DOM_DIMS[ai][0];
        const index_t *adjset_pdims = &ADJSET_POINT_DIMS[ai][0];

        const index_t adjset_num_doms = adjset_ddims[0] * adjset_ddims[1] * adjset_ddims[2];

        // NOTE: The following lines are for debugging purposes only.
        std::cout << "Testing adjset for " <<
            "(" << adjset_ddims[0] << ", " << adjset_ddims[1] << ", " << adjset_ddims[2] << ") domains w/ " <<
            "(" << adjset_pdims[0]-1 << ", " << adjset_pdims[1]-1 << ", " << adjset_pdims[2]-1 << ") " <<
            "'" << adjset_etype << "' elements..." << std::endl;

        Node mesh, info;
        blueprint::mesh::examples::grid(adjset_etype,
            adjset_pdims[0], adjset_pdims[1], adjset_pdims[2],
            adjset_ddims[0], adjset_ddims[1], adjset_ddims[2],
            mesh);

        // NOTE: The following lines are for debugging purposes only.
        std::cout << "  Testing max-share -> pairwise transform..." << std::endl;

        for(Node *domain : blueprint::mesh::domains(mesh))
        {
            Node &domain_adjset = (*domain)["adjsets"].child(0);
            ASSERT_TRUE(blueprint::mesh::adjset::verify(domain_adjset, info));

            Node &pairwise_adjset = (*domain)["adjsets"][domain_adjset.name() + "_pairwise"];
            blueprint::mesh::adjset::to_pairwise(domain_adjset, pairwise_adjset);
            ASSERT_TRUE(blueprint::mesh::adjset::verify(pairwise_adjset, info));
            ASSERT_TRUE(blueprint::mesh::adjset::is_pairwise(pairwise_adjset));

            ASSERT_EQ(pairwise_adjset["association"].as_string(), domain_adjset["association"].as_string());
            ASSERT_EQ(pairwise_adjset["topology"].as_string(), domain_adjset["topology"].as_string());
            ASSERT_EQ(pairwise_adjset["groups"].number_of_children(), adjset_num_doms - 1);
        }

        // NOTE: The following lines are for debugging purposes only.
        std::cout << "  Testing pairwise -> max-share transform..." << std::endl;

        for(Node *domain : blueprint::mesh::domains(mesh))
        {
            Node &domain_adjset = (*domain)["adjsets"].child(0);
            Node &pairwise_adjset = (*domain)["adjsets"][domain_adjset.name() + "_pairwise"];

            Node &maxshare_adjset = (*domain)["adjsets"][domain_adjset.name() + "_maxshare"];
            blueprint::mesh::adjset::to_maxshare(pairwise_adjset, maxshare_adjset);
            ASSERT_TRUE(blueprint::mesh::adjset::verify(maxshare_adjset, info));
            ASSERT_TRUE(blueprint::mesh::adjset::is_maxshare(maxshare_adjset));

            ASSERT_EQ(maxshare_adjset["association"].as_string(), domain_adjset["association"].as_string());
            ASSERT_EQ(maxshare_adjset["topology"].as_string(), domain_adjset["topology"].as_string());
            // TODO: Calculate this amount based on input domain configuration.
            // ASSERT_EQ(maxshare_adjset["groups"].number_of_children(), adjset_num_doms - 1);
        }
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, adjset_transform_dtypes)
{
    XformCoordsFun xform_funs[2] = {blueprint::mesh::adjset::to_pairwise, blueprint::mesh::adjset::to_maxshare};
    const std::string xform_types[2] = {"pairwise", "max-share"};

    for(size_t xi = 0; xi < sizeof(xform_funs) / sizeof(xform_funs[0]); xi++)
    {
        XformCoordsFun to_new_adjset = xform_funs[xi];
        const std::string &xform_type = xform_types[xi];

        Node ibase, info;
        blueprint::mesh::examples::grid("quads",2,2,0,2,2,1,ibase);

        for(size_t ii = 0; ii < bputils::INT_DTYPES.size(); ii++)
        {
            // NOTE: The following lines are for debugging purposes only.
            std::cout << "Testing " <<
                "int-" << 32 * (ii + 1) << " adjset " <<
                "baseline" << " -> " << xform_type << "..." << std::endl;

            Node imesh = ibase, jmesh;
            set_node_data(imesh, bputils::INT_DTYPES[ii]);

            for(const std::string &domain_name : imesh.child_names())
            {
                Node &idomain = imesh[domain_name];
                Node &jdomain = jmesh[domain_name];

                Node &iadjset = idomain["adjsets"].child(0);
                Node &jadjset = jdomain["adjsets"][iadjset.name()];

                to_new_adjset(iadjset, jadjset);
            }

            EXPECT_TRUE(verify_node_data(jmesh, bputils::INT_DTYPES[ii]));
        }
    }
}



//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_transform, paint_adjset)
{
    std::string protocol = "hdf5";
    if(!check_if_hdf5_enabled())
    {
        protocol = "yaml";
    }

    conduit::Node mesh;
    blueprint::mesh::examples::grid("quads",10,10,0,3,3,1,mesh);
    blueprint::mesh::paint_adjset("mesh_adj",
                                  "adjset_vals",
                                  mesh);

    conduit::relay::io::blueprint::save_mesh(mesh,"tout_paint_adjset_simple_2d",protocol);

    mesh.reset();
    blueprint::mesh::examples::grid("hexs",5,5,5,2,2,2,mesh);
    blueprint::mesh::paint_adjset("mesh_adj",
                                  "adjset_vals",
                                  mesh);

    conduit::relay::io::blueprint::save_mesh(mesh,"tout_paint_adjset_simple_3d",protocol);

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_examples, generate_sad_face)
{
    // this tests demos an issue with face (line) generation + adjsets
    // the line between the x's is falsely identified as a shared face
    // b/c it is defined by two shared vertices
    //

    /*
      ----------------
      |    |    |    |
      | d1 | d1 | d1 |
      |    |    |    |
      ----------------
      |    |    |    |
      | d1 | d0 | d1 |
      |    |    |    |
      -----x----x-----
      |    |    |    |
      | d0 | d0 | d0 |
      |    |    |    |
      ----------------
    */
    
    /*
      Domain 0 Vertex Ids:
           ------
           8    9
           ------
           |    |
           | d0 |
           |    |
      -----x----x-----
      4    5    6    7
      -----x----x-----
      |    |    |    |
      | d0 | d0 | d0 |
      |    |    |    |
      ----------------
      0    1    2    3
      ----------------
    */

    /*
      Domain 1 Vertex Ids:

      ----------------
      8    9    10   11
      ----------------
      |    |    |    |
      | d1 | d1 | d1 |
      |    |    |    |
      ----------------
      4    5    6    7
      ----------------
      |    |    |    |
      | d1 |    | d1 |
      |    |    |    |
      ----------------
      0    1    2    3
      ----------------
    */


    Node res;
    Node &d0 = res["domain0"];
    Node &d1 = res["domain1"];

    d0["state/domain_id"] = 0;
    d0["coordsets/coords/type"] = "explicit";
                                      /* 0    1    2    3         4    5    6    7         8   9 */ 
    d0["coordsets/coords/values/x"] = { 0.0, 1.0, 2.0, 3.0, /**/ 0.0, 1.0, 2.0, 3.0, /**/ 1.0, 2.0 };
 
    d0["coordsets/coords/values/y"] = { 0.0, 0.0, 0.0, 0.0, /**/ 1.0, 1.0, 1.0, 1.0, /**/ 2.0, 2.0 };

    d0["topologies/main/type"] = "unstructured";
    d0["topologies/main/coordset"] = "coords";
    d0["topologies/main/elements/shape"] = "quad";
    d0["topologies/main/elements/connectivity"] = {0, 1, 5, 4,
                                                   1, 2, 6, 5,
                                                   2, 3, 7, 6,
                                                   5, 6, 9, 8};
    d1["state/domain_id"] = 1;
    d1["coordsets/coords/type"] = "explicit";
                                      /* 0    1    2    3         4    5    6    7          8   9    10   11 */ 
    d1["coordsets/coords/values/x"] = { 0.0, 1.0, 2.0, 3.0, /**/ 0.0, 1.0, 2.0, 3.0, /**/ 0.0, 1.0, 2.0, 3.0 };
 
    d1["coordsets/coords/values/y"] = { 1.0, 1.0, 1.0, 1.0, /**/ 2.0, 2.0, 2.0, 2.0, /**/ 3.0, 3.0, 3.0, 3.0 };

    d1["topologies/main/type"] = "unstructured";
    d1["topologies/main/coordset"] = "coords";
    d1["topologies/main/elements/shape"] = "quad";
    d1["topologies/main/elements/connectivity"] = {0, 1, 5, 4,
                                                   2, 3, 7, 6,
                                                   4, 5, 9, 8,
                                                   5, 6, 10, 9,
                                                   6, 7, 11, 10 };

    // add adjsets
    d0["adjsets/main_adjset/association"] = "vertex";
    d0["adjsets/main_adjset/topology"] = "main";
    d0["adjsets/main_adjset/groups/group_0_1/neighbors"] = 1;
    d0["adjsets/main_adjset/groups/group_0_1/values"] = {4,5,8,9,6,7};

    // add adjsets
    d1["adjsets/main_adjset/association"] = "vertex";
    d1["adjsets/main_adjset/topology"] = "main";
    d1["adjsets/main_adjset/groups/group_0_1/neighbors"] = 0;
    d1["adjsets/main_adjset/groups/group_0_1/values"] = {0, 1, 5, 6, 2, 3};

    Node maps;
    conduit::blueprint::mesh::generate_lines(res,
                                             "main_adjset",
                                             "main_face_adjset",
                                             "main_faces",
                                             maps["s2d"],
                                             maps["d2s"]);

    blueprint::mesh::paint_adjset("main_adjset",
                                  "main_adjset_vals",
                                  res);

    blueprint::mesh::paint_adjset("main_face_adjset",
                                  "main_face_adjset_vals",
                                  res);
    res.print();
    
    // this issue is fixed when:
    // domain0, main_face element 6 main_face_adjset_vals == 0 && 
    // element 6 is not in the faces adjset as a group entry for domain 0

    index_t_accessor fadj_vals= res.fetch_existing("domain0/adjsets/main_face_adjset/groups")[0]["values"].value();
    EXPECT_EQ(fadj_vals.count(6),0);
    index_t_accessor fadj_count= res.fetch_existing("domain0/fields/main_face_adjset_vals_group_count/values").value();
    EXPECT_EQ(fadj_count[6],0);


    Node info;
    if(!conduit::blueprint::mesh::verify(res,info))
    {
        info.print();
    }

    std::string protocol = "hdf5";
    if(!check_if_hdf5_enabled())
    {
        protocol = "yaml";
    }

    conduit::relay::io::blueprint::save_mesh(res,"sad_face",protocol);
}
