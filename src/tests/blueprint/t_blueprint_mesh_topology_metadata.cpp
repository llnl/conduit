// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_topology_metadata.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mesh_topology_metadata.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"
#include "conduit_annotations.hpp"

#include <algorithm>
#include <iostream>
#include <array>
#include <cmath>
#include "gtest/gtest.h"

#include "blueprint_test_helpers.hpp"

using std::cout;
using std::endl;

// Use the reference one. (for setting baselines initially)
//using TopologyMetadata = conduit::blueprint::mesh::utils::reference::TopologyMetadata;

// Use the new one
using TopologyMetadata = conduit::blueprint::mesh::utils::TopologyMetadata;

using index_t = conduit::index_t;

// Enable this macro to generate baselines.
//#define GENERATE_BASELINES

// Enable this macro when debugging to make Conduit hang where it would throw.
//#define USE_ERROR_HANDLER

//-----------------------------------------------------------------------------
#ifdef _WIN32
const std::string sep("\\");
#else
const std::string sep("/");
#endif

//-----------------------------------------------------------------------------
std::string
baseline_dir()
{
    std::string path(__FILE__);
    auto idx = path.rfind(sep);
    if(idx != std::string::npos)
        path = path.substr(0, idx);
    path = path + sep + std::string("baselines");
    return path;
}

//-----------------------------------------------------------------------------
std::string test_name() { return std::string("t_blueprint_mesh_topology_metadata"); }

//-----------------------------------------------------------------------------
int get_rank() { return 0; }

//-----------------------------------------------------------------------------
void barrier() { }

//-----------------------------------------------------------------------------
// Include some helper function definitions
#include "blueprint_baseline_helpers.hpp"

//---------------------------------------------------------------------------
void
yaml_print(std::ostream &os, const conduit::Node &node)
{
    // Override these parameters so we get more of our output.
    conduit::Node opts;
    opts["num_elements_threshold"] = 10000;
    opts["num_children_threshold"] = 10000;

    std::string s;
    node.to_summary_string_stream(cout, opts);
}

//-----------------------------------------------------------------------------
void
tmp_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << endl;

    while(1);
}

//-----------------------------------------------------------------------------
void
test_topmd(const std::string &base, conduit::Node &topo, conduit::Node &coords)
{
    TopologyMetadata md(topo, coords);
    index_t maxdim = conduit::blueprint::mesh::utils::topology::dims(topo);

    // This node will hold a representation of the metadata.
    conduit::Node rep;

    // Add all topos to a vis node.
    conduit::Node vis;
    vis["coordsets/coords"].set_external(coords);
    for(index_t d = maxdim; d >= 0; d--)
    {
        // Add the topology to the rep.
        std::stringstream oss;
        oss << "topologies/topo" << d;
        std::string tname(oss.str());
        rep[tname].set_external(md.get_topology(d));

        // Add the topology to the vis.
        vis[tname].set_external(md.get_topology(d));
    }
#ifdef GENERATE_BASELINES
    // Save all topos together to a dataset for visualization.
    save_visit(base, vis);
#endif
    for(index_t d = maxdim; d >= 0; d--)
    {
        std::stringstream oss;
        oss << "lengths/topo" << d;
        std::string mname(oss.str());
        rep[mname].set(conduit::blueprint::mesh::utils::topology::length(md.get_topology(d)));
    }

    std::vector<std::string> mapkeys{"values", "sizes", "offsets"};

    // Get all the maps and add them to the rep.
    for(index_t e = maxdim; e >= 0; e--)
    for(index_t a = maxdim; a >= 0; a--)
    {
        {
            std::stringstream oss;
            oss << "associations/global/map" << e << a << "/data";
            std::string mname(oss.str());
            md.get_dim_map(TopologyMetadata::GLOBAL, e, a, rep[mname]);

            // Add some lengths so we do not have to count when looking at the output.
            for(const auto &key : mapkeys)
            {
                if(rep[mname].has_child(key))
                {
                    std::stringstream oss2;
                    oss2 << "associations/global/map" << e << a << "/sizes/" << key;
                    std::string mname2(oss2.str());
                    rep[mname2].set(rep[mname][key].dtype().number_of_elements());
                }
            }
        }
        {
            std::stringstream oss;
            oss << "associations/local/map" << e << a << "/data";
            std::string mname(oss.str());
            md.get_dim_map(TopologyMetadata::LOCAL, e, a, rep[mname]);

            // Add some lengths so we do not have to count when looking at the output.
            for(const auto &key : mapkeys)
            {
                if(rep[mname].has_child(key))
                {
                    std::stringstream oss2;
                    oss2 << "associations/local/map" << e << a << "/sizes/" << key;
                    std::string mname2(oss2.str());
                    rep[mname2].set(rep[mname][key].dtype().number_of_elements());
                }
            }
        }
    }

    for(index_t d = maxdim; d >= 0; d--)
    {
        const std::vector<index_t> &le2ge = md.get_local_to_global_map(d);

        std::stringstream oss;
        oss << "local_to_global/map" << d << "/data";
        std::string mname(oss.str());
        conduit::Node &m = rep[mname];
        m.set(le2ge);

        std::stringstream oss2;
        oss2 << "local_to_global/map" << d << "/size";
        std::string mname2(oss2.str());
        conduit::Node &s = rep[mname2];
        s.set(le2ge.size());
    }

    // Test the 3,0 API for 3D meshes to see whether we get the same answer
    // from G(3,0) as we would from unstructured::points()
    if(md.dimension() == 3)
    {
        auto N = md.get_topology_length(3);
        for(index_t ei = 0; ei < N; ei++)
        {
            // Get G(3,0) for the element and compare it to the points() function.
            auto elempts = md.get_global_association(ei, 3, 0);
            std::vector<index_t> elempts_vec(elempts.size());
            std::copy(elempts.begin(), elempts.end(), elempts_vec.begin());
            std::sort(elempts_vec.begin(), elempts_vec.end());

            std::vector<index_t> pts = conduit::blueprint::mesh::utils::topology::unstructured::points(md.get_topology(3), ei);
            bool equal = (pts == elempts_vec);
            EXPECT_TRUE(equal);
        }
    }

    std::string b = baseline_file(base);
#ifdef GENERATE_BASELINES
    make_baseline(b, rep);
#else
    conduit::Node baseline_rep;
    // NOTE: We force writing to a file to avoid any problems in Node::diff.
    bool pass = compare_baseline(b, rep, baseline_rep, true);
    if(!pass)
    {
        std::cout << "BASELINE: " << b << endl;
        yaml_print(std::cout, baseline_rep);
        std::cout << "CURRENT:" << endl;
        yaml_print(std::cout, rep);
    }
    EXPECT_EQ(pass, true);
#endif
}

//-----------------------------------------------------------------------------
void
make_custom_tets(conduit::Node &node)
{
    std::vector<float> xc{0.f, 0.f, 1.f, 0.f, 0.f};
    std::vector<float> yc{0.f, 0.f, 0.f, 1.f, 0.f};
    std::vector<float> zc{0.f, 1.f, 0.f, 0.f, -1.f};
    // 2 tets where they share a face but it is rotated relative to the 2nd tet.
    std::vector<int> conn{0,1,2,3,
                          3,2,0,4
                         };

    node["coordsets/coords/type"] = "explicit";
    node["coordsets/coords/values/x"].set(xc);
    node["coordsets/coords/values/y"].set(yc);
    node["coordsets/coords/values/z"].set(zc);

    node["topologies/mesh/coordset"] = "coords";
    node["topologies/mesh/type"] = "unstructured";
    node["topologies/mesh/elements/shape"] = "tet";
    node["topologies/mesh/elements/connectivity"].set(conn);
}

//-----------------------------------------------------------------------------
void
make_custom_hexs(conduit::Node &node)
{
    std::vector<float> xc{0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,

                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,

                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f
                         };
    std::vector<float> yc{0.f, 0.0f, 0.f,
                          0.5f, 0.5f, 0.5f,
                          1.f, 1.f, 1.f,

                          0.f, 0.0f, 0.f,
                          0.5f, 0.5f, 0.5f,
                          1.f, 1.f, 1.f,

                          0.f, 0.0f, 0.f,
                          0.5f, 0.5f, 0.5f,
                          1.f, 1.f, 1.f
                         };
    std::vector<float> zc{0.f, 0.0f, 0.f,
                          0.f, 0.0f, 0.f,
                          0.f, 0.0f, 0.f,

                          0.5f, 0.5f, 0.5f,
                          0.5f, 0.5f, 0.5f,
                          0.5f, 0.5f, 0.5f,

                          1.f, 1.0f, 1.f,
                          1.f, 1.0f, 1.f,
                          1.f, 1.0f, 1.f
                         };

    // hexes where we change orientations a lot.
    std::vector<int> conn{0,9,10,1,3,12,13,4,
                          10,11,2,1,13,14,5,4,
                          3,6,15,12,4,7,16,13,
                          7,4,5,8,16,13,14,17,
                          22,19,18,21,13,10,9,12,
                          14,11,20,23,13,10,19,22,
                          12,15,24,21,13,16,25,22,
                          26,23,22,25,17,14,13,16
                         };

    node["coordsets/coords/type"] = "explicit";
    node["coordsets/coords/values/x"].set(xc);
    node["coordsets/coords/values/y"].set(yc);
    node["coordsets/coords/values/z"].set(zc);

    node["topologies/mesh/coordset"] = "coords";
    node["topologies/mesh/type"] = "unstructured";
    node["topologies/mesh/elements/shape"] = "hex";
    node["topologies/mesh/elements/connectivity"].set(conn);
}

//-----------------------------------------------------------------------------
void
make_custom_poly(conduit::Node &node)
{
    std::vector<double> xc{0.,4.,4.,2.,0.,5.,8.,7.,8.,6.,8.,0.,4.,8.,0.,2.,4.,8.};
    std::vector<double> yc{0.,0.,2.,4.,4.,0.,0.,1.,3.,4.,4.,5.,6.,6.,7.,7.,7.,7.};

    std::vector<int> conn{0,1,2,3,4,
                          1,5,7,8,10,9,2,
                          5,6,7,
                          7,6,8,
                          2,9,12,3,
                          11,15,14,
                          4,3,12,16,15,11,
                          12,9,13,17,16,
                          9,10,13
                         };
    std::vector<int> connsize{5,7,3,3,4,3,6,5,3};

    node["coordsets/coords/type"] = "explicit";
    node["coordsets/coords/values/x"].set(xc);
    node["coordsets/coords/values/y"].set(yc);

    node["topologies/mesh/coordset"] = "coords";
    node["topologies/mesh/type"] = "unstructured";

    node["topologies/mesh/elements/shape"] = "polygonal";
    node["topologies/mesh/elements/connectivity"].set(conn);
    node["topologies/mesh/elements/sizes"].set(connsize);

    conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(
        node["topologies/mesh"], 
        node["topologies/mesh/elements/offsets"]);
}

//-----------------------------------------------------------------------------
void
make_custom_ph(conduit::Node &node)
{
    std::vector<float> xc{0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,
                          0.f, 0.5f, 1.f,
                          0.f, 0.25f, 0.5f, 0.75f, 1.f,
                          0.f, 0.25f, 0.5f, 0.75f, 1.f,
                          0.f, 0.25f, 0.5f, 0.75f, 1.f,
                          0.f, 0.25f, 0.5f, 0.75f, 1.f,
                          0.f, 0.25f, 0.5f, 0.75f, 1.f};
    std::vector<float> yc{0.f,   0.0f,  0.f,
                          0.5f,  0.5f,  0.5f,
                          1.f,   1.f,   1.f,
                          0.f,   0.f,   0.f,   0.f,   0.f,
                          0.25f, 0.25f, 0.25f, 0.25f, 0.25f,
                          0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
                          0.75f, 0.75f, 0.75f, 0.75f, 0.75f,
                          1.f,   1.f,   1.f,   1.f,   1.f};
    std::vector<float> zc{0.5f,  0.5f,  0.5f,
                          0.5f,  0.5f,  0.5f,
                          0.5f,  0.5f,  0.5f,
                          1.f,  1.1f, 1.f,  1.1f, 1.f,
                          1.1f, 1.2f, 1.1f, 1.2f, 1.1f,
                          1.f,  1.1f, 1.f,  1.1f, 1.f,
                          1.1f, 1.2f, 1.1f, 1.2f, 1.1f,
                          1.f,  1.1f, 1.f,  1.1f, 1.f};

    std::vector<int> conn{
                         0,2,6,8,12,16,17,20,21,
                         2,4,7,9,13,18,19,22,23,
                         1,3,8,10,14,24,25,28,29,
                         3,5,9,11,15,26,27,30,31
                         };
    std::vector<int> connsize{9,9,9,9};

    std::vector<int> faceconn{// YZ faces
                              0,9,14,19,3,
                              3,19,24,29,6,
                              1,11,16,21,4,
                              4,21,26,31,7,
                              2,13,18,23,5,
                              5,23,28,33,8,
                              // XZ faces
                              0,1,11,10,9,
                              1,2,13,12,11,
                              3,4,21,20,19,
                              4,5,23,22,21,
                              6,7,31,30,29,
                              7,8,33,32,31,
                              // XY faces
                              1,0,3,4,
                              2,1,4,5,
                              4,3,6,7,
                              5,4,7,8,
                              9,10,15,14,
                              10,11,16,15,
                              11,12,17,16,
                              12,13,18,17,
                              14,15,20,19,
                              15,16,21,20,
                              16,17,22,21,
                              17,18,23,22,
                              19,20,25,24,
                              20,21,26,25,
                              21,22,27,26,
                              22,23,28,27,
                              24,25,30,29,
                              25,26,31,30,
                              26,27,32,31,
                              27,28,33,32
                             };
    std::vector<int> facesize{// YZ faces
                              5,5,
                              5,5,
                              5,5,
                              // XZ faces
                              5,5,
                              5,5,
                              5,5,
                              // XY faces
                              4,4,
                              4,4,
                              4,4,4,4,
                              4,4,4,4,
                              4,4,4,4,
                              4,4,4,4
                             };
    node["coordsets/coords/type"] = "explicit";
    node["coordsets/coords/values/x"].set(xc);
    node["coordsets/coords/values/y"].set(yc);
    node["coordsets/coords/values/z"].set(zc);

    node["topologies/mesh/coordset"] = "coords";
    node["topologies/mesh/type"] = "unstructured";

    node["topologies/mesh/elements/shape"] = "polyhedral";
    node["topologies/mesh/elements/connectivity"].set(conn);
    node["topologies/mesh/elements/sizes"].set(connsize);

    node["topologies/mesh/subelements/shape"] = "polygonal";
    node["topologies/mesh/subelements/connectivity"].set(faceconn);
    node["topologies/mesh/subelements/sizes"].set(facesize);

    conduit::blueprint::mesh::utils::topology::unstructured::generate_offsets(
        node["topologies/mesh"], 
        node["topologies/mesh/elements/offsets"],
        node["topologies/mesh/subelements/offsets"]);
}

//-----------------------------------------------------------------------------
void
make_dataset(conduit::Node &node, const std::string &type, conduit::DataType &dtype)
{
    if(type == "custom_tets")
        make_custom_tets(node);
    else if(type == "custom_hexs")
        make_custom_hexs(node);
    else if(type == "custom_poly")
        make_custom_poly(node);
    else if(type == "custom_ph")
        make_custom_ph(node);
    else
    {
        int dims[] = {3,3,2};
        if(type == "lines" || type == "tris" || type == "quads" ||
           type == "quads_poly" || type == "mixed_2d")
        {
            dims[2] = 0;
        }
        conduit::blueprint::mesh::examples::braid(type, dims[0],dims[1],dims[2], node);
    }
    conduit::Node &topo = node["topologies/mesh"];
    //std::cout << type << " length: "
    //          << conduit::blueprint::mesh::utils::topology::length(topo)
    //          << std::endl;

    // Make sure that these items are the requested data type (if they exist).
    std::vector<std::string> copy_keys{"elements/connectivity",
                                       "elements/sizes",
                                       "elements/offsets",
                                       "subelements/connectivity",
                                       "subelements/sizes",
                                       "subelements/offsets"
                                       };
    for(const auto &key : copy_keys)
    {
        if(topo.has_path(key))
        {
            conduit::Node &src = topo[key];
            conduit::Node dest;
            dest.set(conduit::DataType(dtype.id(), src.dtype().number_of_elements()));
            src.to_data_type(dtype.id(), dest);
            // replace src with the converted type.
            src.set(dest);
        }
    }
}

//-----------------------------------------------------------------------------
std::vector<index_t>
test_dtype_ids()
{
    return std::vector<index_t>{CONDUIT_INT32_ID, CONDUIT_INT64_ID};
}

//-----------------------------------------------------------------------------
void
test_mesh_type(const std::string &type)
{
    CONDUIT_ANNOTATE_MARK_SCOPE((std::string("test_mesh_type: ") + type).c_str());
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    // For each data type, make a mesh that has data of those types and then
    // do the topmd test.
    auto dtypes = test_dtype_ids();
    for(auto dtid : dtypes)
    {
        conduit::DataType dtype(dtid);
        CONDUIT_ANNOTATE_MARK_SCOPE(dtype.name().c_str());

        conduit::Node node;
        make_dataset(node, type, dtype);
        // Now do the test.
        std::stringstream oss;
        oss << "topology_metadata_" << type << "_" << dtype.name();
        test_topmd(oss.str(), node["topologies/mesh"], node["coordsets/coords"]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, points)
{
    // NOTE: When the reference::TopologyMetadata is used to make this baseline,
    //       it makes double the connectivity for topo0 (all points specified
    //       twice). That does not seem right. The baselines for this test case
    //       have only one copy of the points.
    test_mesh_type("points");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, lines)
{
    test_mesh_type("lines");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, tris)
{
    test_mesh_type("tris");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, quads)
{
    test_mesh_type("quads");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, tets)
{
    test_mesh_type("tets");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, pyramids)
{
    test_mesh_type("pyramids");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, wedges)
{
    test_mesh_type("wedges");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, hexs)
{
    test_mesh_type("hexs");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, custom_tets)
{
    test_mesh_type("custom_tets");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, custom_hexs)
{
    test_mesh_type("custom_hexs");
}

// Polygon cases

///-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, quads_poly)
{
    test_mesh_type("quads_poly");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, custom_poly)
{
    test_mesh_type("custom_poly");
}

// PH cases
//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, hexs_poly)
{
    // NOTE: The topo2 elements/shape was modified in the baseline to "quad"
    //       since the new TopologyMetadata class tries to make single shapes
    //       if they happen to all be the same.
    test_mesh_type("hexs_poly");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, custom_ph)
{
    test_mesh_type("custom_ph");
}

#if 0
// These topologies are not supported by reference::TopologyMetadata (and probably
// not the new class either as additional connectivity handling is needed.)

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, mixed_2d)
{
    test_mesh_type("mixed_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_topology_metadata, rectilinear)
{
    test_mesh_type("rectilinear");
}
#endif

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);

#if defined(CONDUIT_USE_CALIPER)
    std::string tout_file = "tout_blueprint_mesh_topology_metadata.txt";
    if(conduit::annotations::supported())
    {
        // clean up output file if it exists
        conduit::utils::remove_path_if_exists(tout_file);
    }
    conduit::Node opts;
    opts["config"] = "runtime-report";
    opts["output_file"] = tout_file;
    // Allow setting / overrides.
    std::vector<std::string> keys{"config", "services", "output_file"};
    for(int i = 1; i < argc; i++)
    {
        for(const auto &key : keys)
        {
            std::string arg(std::string("-") + key);
            if(arg == argv[i] && (i+1) < argc)
            {
                opts[key] = argv[i+1];
                ++i;
                break;
            }
        }
    }
    conduit::annotations::initialize(opts);
#endif

    result = RUN_ALL_TESTS();

#if defined(CONDUIT_USE_CALIPER)
    conduit::annotations::finalize();
#endif
    return result;
}
