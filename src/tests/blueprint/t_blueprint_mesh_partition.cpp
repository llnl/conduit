// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_partition.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_relay.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_mesh_partition.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_mesh_utils_iterate_elements.hpp"
#include "conduit_relay.hpp"
#include "conduit_log.hpp"
#include "conduit_fmt/conduit_fmt.h"

#include <math.h>
#include <array>
#include <cmath>
#include <iostream>
#include <numeric>
#include <set>

#include "gtest/gtest.h"

#include "blueprint_test_helpers.hpp"

using std::cout;
using std::endl;

// Enable this macro to generate baselines.
// #define GENERATE_BASELINES

// #define USE_ERROR_HANDLER

#ifndef ALWAYS_PRINT
static const bool always_print = false;
#else
static const bool always_print = true;
#endif

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
std::string test_name() { return std::string("t_blueprint_mesh_partition"); }

//-----------------------------------------------------------------------------
int get_rank() { return 0; }

//-----------------------------------------------------------------------------
void barrier() { }

//-----------------------------------------------------------------------------
// Include some helper function definitions
#include "blueprint_baseline_helpers.hpp"

//-----------------------------------------------------------------------------
void
tmp_err_handler(const std::string &s1, const std::string &s2, int i1)
{
    cout << "s1=" << s1 << ", s2=" << s2 << ", i1=" << i1 << endl;

    while(1);
}

//-----------------------------------------------------------------------------
void
test_logical_selection_2d(const std::string &topo, const std::string &base)
{
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    // Make 10x10x1 cell mesh.
    conduit::Node input, output, options, msg;
    conduit::index_t vdims[] = {11,11,1};
    conduit::blueprint::mesh::examples::braid(topo, vdims[0], vdims[1], vdims[2], input);
    // Override with int64 because YAML loses int/uint information.
    conduit::int64 i100 = 100;
    input["state/cycle"].set(i100);
    input["state/domain_id"].set((int)0);

    // With no options (turn mapping off though because otherwise we add
    // the original vertex and element fields), test that output==input
    const char *opt0 =
"mapping: 0";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(input.diff(output, msg, 0.0, true), false) << msg.to_json();
    std::string b00 = baseline_file(base + "_00");
    save_visit(b00, output);

    // Select the whole thing but divide it into target domains.
    const char *opt1 =
"target: 2";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 2);
    std::string b01 = baseline_file(base + "_01");
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Select the whole thing but divide it into target domains.
    const char *opt2 =
"target: 4";
    options.reset(); options.parse(opt2, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 4);
    std::string b02 = baseline_file(base + "_02");
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif

    // Select the whole thing but go out of bounds to see if the selections
    // clamp to good values.
    const char *opt3 =
"selections:\n"
"   -\n"
"     type: \"logical\"\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [100,100,100]";
    options.reset(); options.parse(opt3, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    EXPECT_EQ(conduit::blueprint::mesh::is_multi_domain(output), false);
    std::string b03 = baseline_file(base + "_03");
    save_visit(b03, output);
#ifdef GENERATE_BASELINES
    make_baseline(b03, output);
#else
    EXPECT_EQ(compare_baseline(b03, output), true);
#endif

    // Select 3 parts.
    const char *opt4 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [4,9,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,0,0]\n"
"     end:   [9,4,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,5,0]\n"
"     end:   [9,9,0]";
    options.reset(); options.parse(opt4, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 3);
    std::string b04 = baseline_file(base + "_04");
    save_visit(b04, output);
#ifdef GENERATE_BASELINES
    make_baseline(b04, output);
#else
    EXPECT_EQ(compare_baseline(b04, output), true);
#endif

    // Select 3 parts with 4 targets.
    const char *opt5 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [4,9,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,0,0]\n"
"     end:   [9,4,0]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,5,0]\n"
"     end:   [9,9,0]\n"
"target: 4";
    options.reset(); options.parse(opt5, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 4);
    std::string b05 = baseline_file(base + "_05");
    save_visit(b05, output);
#ifdef GENERATE_BASELINES
    make_baseline(b05, output);
#else
    EXPECT_EQ(compare_baseline(b05, output), true);
#endif

    // TODO: try opt5 but target 2 to see if we combine down to 2 domains.
}

//-----------------------------------------------------------------------------
void
test_logical_selection_3d(const std::string &topo, const std::string &base)
{
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    // Make 10x10x1 cell mesh.
    conduit::Node input, output, options, msg;
    conduit::index_t vdims[] = {11,11,4};
    conduit::blueprint::mesh::examples::braid(topo, vdims[0], vdims[1], vdims[2], input);
    // Override with int64 because YAML loses int/uint information.
    conduit::int64 i100 = 100;
    input["state/cycle"].set(i100);
    input["state/domain_id"].set((int)0);

    // With no options (turn mapping off though because otherwise we add
    // the original vertex and element fields), test that output==input
    const char *opt0 =
"mapping: 0";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(input.diff(output, msg, 0.0, true), false) << msg.to_json();
    std::string b00 = baseline_file(base + "_00");
    save_visit(b00, output);

    // Select the whole thing but divide it into target domains.
    const char *opt1 =
"target: 2";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 2);
    std::string b01 = baseline_file(base + "_01");
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Select the whole thing but divide it into target domains.
    const char *opt2 =
"target: 4";
    options.reset(); options.parse(opt2, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 4);
    std::string b02 = baseline_file(base + "_02");
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif

    // Select the whole thing but go out of bounds to see if the selections
    // clamp to good values.
    const char *opt3 =
"selections:\n"
"   -\n"
"     type: \"logical\"\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [100,100,100]";
    options.reset(); options.parse(opt3, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    EXPECT_EQ(conduit::blueprint::mesh::is_multi_domain(output), false);
    std::string b03 = baseline_file(base + "_03");
    save_visit(b03, output);
#ifdef GENERATE_BASELINES
    make_baseline(b03, output);
#else
    EXPECT_EQ(compare_baseline(b03, output), true);
#endif

    // Select 3 parts.
    const char *opt4 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [4,9,2]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,0,0]\n"
"     end:   [9,4,2]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,5,0]\n"
"     end:   [9,9,2]";
    options.reset(); options.parse(opt4, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 3);
    std::string b04 = baseline_file(base + "_04");
    save_visit(b04, output);
#ifdef GENERATE_BASELINES
    make_baseline(b04, output);
#else
    EXPECT_EQ(compare_baseline(b04, output), true);
#endif

    // Select 3 parts with 4 targets.
    const char *opt5 =
"selections:\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [0,0,0]\n"
"     end:   [4,9,2]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,0,0]\n"
"     end:   [9,4,2]\n"
"   -\n"
"     type: logical\n"
"     domain_id: 0\n"
"     start: [5,5,0]\n"
"     end:   [9,9,2]\n"
"target: 4";
    options.reset(); options.parse(opt5, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 4);
    std::string b05 = baseline_file(base + "_05");
    save_visit(b05, output);
#ifdef GENERATE_BASELINES
    make_baseline(b05, output);
#else
    EXPECT_EQ(compare_baseline(b05, output), true);
#endif

    // TODO: try opt5 but target 2 to see if we combine down to 2 domains.
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, uniform_logical_2d)
{
    test_logical_selection_2d("uniform", "uniform_logical_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, rectilinear_logical_2d)
{
    test_logical_selection_2d("rectilinear", "rectilinear_logical_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, structured_logical_2d)
{
    test_logical_selection_2d("structured", "structured_logical_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, uniform_logical_3d)
{
    test_logical_selection_3d("uniform", "uniform_logical_3d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, rectilinear_logical_3d)
{
    test_logical_selection_3d("rectilinear", "rectilinear_logical_3d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, structured_logical_3d)
{
    test_logical_selection_3d("structured", "structured_logical_3d");
}

//-----------------------------------------------------------------------------
void
test_explicit_selection(const std::string &topo, const conduit::index_t vdims[3],
    const std::string &base, int (*spc)(conduit::index_t, conduit::index_t))
{
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    // Make 10x10x1 cell mesh.
    conduit::Node input, output, options, msg;
    conduit::blueprint::mesh::examples::braid(topo, vdims[0], vdims[1], vdims[2], input);
    // Override with int64 because YAML loses int/uint information.
    conduit::int64 i100 = 100;
    input["state/cycle"].set(i100);

    conduit::index_t nelem = conduit::blueprint::mesh::topology::length(input["topologies"][0]);

    // Select the whole thing. Check output==input
    options.reset();
    {
        std::vector<conduit::index_t> elem;
        for(conduit::index_t i = 0; i < nelem; i++)
            elem.push_back(i);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "explicit";
        sel1["elements"] = elem;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b00 = baseline_file(base + "_00");
    save_visit(b00, output);
#ifdef GENERATE_BASELINES
    make_baseline(b00, output);
#else
    EXPECT_EQ(compare_baseline(b00, output), true);
#endif

    // Select half of the cells.
    options.reset();
    {
        auto n2 = nelem / 2;
        std::vector<conduit::index_t> elem;
        for(conduit::index_t i = 0; i < n2; i++)
            elem.push_back(i);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "explicit";
        sel1["elements"] = elem;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b01 = baseline_file(base + "_01");
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Select a checkerboard of cells
    options.reset();
    std::vector<conduit::index_t> elem;
    {
        conduit::index_t ci = 0;
        for(conduit::index_t j = 0; j < vdims[1]-1; j++)
        for(conduit::index_t i = 0; i < vdims[0]-1; i++)
        {
            int n = spc(i,j);
            for(int k = 0; k < n; k++)
            {
                if((i+j) % 2 == 0)
                    elem.push_back(ci);
                ci++;
            }
        }

        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "explicit";
        sel1["elements"] = elem;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b02 = baseline_file(base + "_02");
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif

    // Make 2 selections
    options.reset();
    {
        auto n2 = nelem / 2;
        std::vector<conduit::index_t> elem, elem2;
        for(conduit::index_t i = 0; i < nelem; i++)
        {
            if(i < n2)
                elem.push_back(i);
            else
                elem2.push_back(i);
        }
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "explicit";
        sel1["elements"] = elem;
        conduit::Node &sel2 = options["selections"].append();
        sel2["type"] = "explicit";
        sel2["elements"] = elem2;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 2);
    std::string b03 = baseline_file(base + "_03");
    save_visit(b03, output);
#ifdef GENERATE_BASELINES
    make_baseline(b03, output);
#else
    EXPECT_EQ(compare_baseline(b03, output), true);
#endif

    // Take previous 2 domain selection and partition it into 5 domains.
    options["target"] = 5;
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 5);
    std::string b04 = baseline_file(base + "_04");
    save_visit(b04, output);
#ifdef GENERATE_BASELINES
    make_baseline(b04, output);
#else
    EXPECT_EQ(compare_baseline(b04, output), true);
#endif
}

//-----------------------------------------------------------------------------
int spc(conduit::index_t i, conduit::index_t j)
{
    return 1;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, uniform_explicit_2d)
{
    conduit::index_t vdims[] = {11,11,1};
    test_explicit_selection("uniform", vdims, "uniform_explicit_2d", spc);
}

//-----------------------------------------------------------------------------
// NOTE: VisIt does not support polygonal cells from Blueprint at this time so
//       I could not visually verify the mesh. The files look okay.
TEST(conduit_blueprint_mesh_partition, quads_poly_explicit_2d)
{
    conduit::index_t vdims[] = {11,11,0};
    test_explicit_selection("quads_poly", vdims, "quads_poly_explicit_2d", spc);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, hexs_explicit_2d)
{
    conduit::index_t vdims[] = {11,11,2};
    test_explicit_selection("hexs", vdims, "hexs_explicit_3d", spc);
}

//-----------------------------------------------------------------------------
// NOTE: verified by converting to VTK in another tool.
TEST(conduit_blueprint_mesh_partition, hexs_poly_explicit_3d)
{
    conduit::index_t vdims[] = {11,11,2};
    test_explicit_selection("hexs_poly", vdims, "hexs_poly_explicit_3d", spc);
}

//-----------------------------------------------------------------------------
void
test_ranges_selection_2d(const std::string &topo, const std::string &base)
{
#ifdef USE_ERROR_HANDLER
    conduit::utils::set_error_handler(tmp_err_handler);
#endif

    // Make 10x10x1 cell mesh.
    conduit::Node input, output, options, msg;
    conduit::index_t vdims[] = {11,11,1};
    if(topo == "quads")
    {
        vdims[2] = 0;
    }
    conduit::blueprint::mesh::examples::braid(topo, vdims[0], vdims[1], vdims[2], input);
    // Override with int64 because YAML loses int/uint information.
    conduit::int64 i100 = 100;
    input["state/cycle"].set(i100);

    conduit::index_t nelem = conduit::blueprint::mesh::topology::length(input["topologies"][0]);
    auto n2 = nelem / 2;

    // Select the whole thing. Check output==input
    options.reset();
    {
        std::vector<conduit::index_t> ranges;
        ranges.push_back(0);
        ranges.push_back(nelem-1);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "ranges";
        sel1["ranges"] = ranges;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b00 = baseline_file(base + "_00");
    save_visit(b00, output);
#ifdef GENERATE_BASELINES
    make_baseline(b00, output);
#else
    EXPECT_EQ(compare_baseline(b00, output), true);
#endif

    // Select out of bounds to make sure it clamps the range.
    options.reset();
    {
        std::vector<conduit::index_t> ranges;
        ranges.push_back(0);
        ranges.push_back(nelem * 2);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "ranges";
        sel1["ranges"] = ranges;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b01 = baseline_file(base + "_01");
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Select a single cell
    options.reset();
    {
        std::vector<conduit::index_t> ranges;
        ranges.push_back(0);
        ranges.push_back(0);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "ranges";
        sel1["ranges"] = ranges;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b02 = baseline_file(base + "_02");
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif

    // Select multiple ranges
    options.reset();
    {
        std::vector<conduit::index_t> ranges;
        // bottom half
        ranges.push_back(0);
        ranges.push_back(n2-1);
        // top row
        ranges.push_back((vdims[1]-1-1) * (vdims[0]-1));
        ranges.push_back((vdims[1]-1)   * (vdims[0]-1));
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "ranges";
        sel1["ranges"] = ranges;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 1);
    std::string b03 = baseline_file(base + "_03");
    save_visit(b03, output);
#ifdef GENERATE_BASELINES
    make_baseline(b03, output);
#else
    EXPECT_EQ(compare_baseline(b03, output), true);
#endif

    // Multiple range selections
    options.reset();
    {
        std::vector<conduit::index_t> ranges, ranges2;
        // domain 1
        ranges.push_back(0);
        ranges.push_back(n2-1);
        // domain 2
        ranges2.push_back(n2);
        ranges2.push_back(nelem-1);
        conduit::Node &sel1 = options["selections"].append();
        sel1["type"] = "ranges";
        sel1["ranges"] = ranges;
        conduit::Node &sel2 = options["selections"].append();
        sel2["type"] = "ranges";
        sel2["ranges"] = ranges2;
    }
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 2);
    std::string b04 = baseline_file(base + "_04");
    save_visit(b04, output);
#ifdef GENERATE_BASELINES
    make_baseline(b04, output);
#else
    EXPECT_EQ(compare_baseline(b04, output), true);
#endif

    // Take previous selection and target 5 domains to test partitioning
    options["target"] = 5;
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 5);
    std::string b05 = baseline_file(base + "_05");
    save_visit(b05, output);
#ifdef GENERATE_BASELINES
    make_baseline(b05, output);
#else
    EXPECT_EQ(compare_baseline(b05, output), true);
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, uniform_ranges_2d)
{
    test_ranges_selection_2d("uniform", "uniform_ranges_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, rectilinear_ranges_2d)
{
    test_ranges_selection_2d("rectilinear", "rectilinear_ranges_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, structured_ranges_2d)
{
    test_ranges_selection_2d("structured", "structured_ranges_2d");
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, quads_ranges_2d)
{
    test_ranges_selection_2d("quads", "quads_ranges_2d");
}

// TODO: Figure out where the point_merge class can live so that
//  these tests can be re-enabled.
#if DO_POINT_MERGE_TESTS
//-----------------------------------------------------------------------------
//-- Point merge
//-----------------------------------------------------------------------------
// The tolerance used by all of the point merge tests
static const double tolerance = 0.00001;

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition_point_merge, one)
{
    conduit::Node braid;
    conduit::blueprint::mesh::examples::braid("tets", 2, 2, 2, braid);
    auto &braid_coordset = braid["coordsets/coords"];

    // Test that 1 coordset in gives the exact same coordset out
    std::vector<const conduit::Node*> one;
    one.push_back(&braid_coordset);

    conduit::Node opts;
    opts["merge_tolerance"] = tolerance;

    conduit::Node output;
    conduit::blueprint::mesh::coordset::combine(one, output, &opts);

    conduit::Node info;
    bool different = braid_coordset.diff(output["coordsets/coords"], info);
    EXPECT_FALSE(different);
    if(different || always_print)
    {
        braid_coordset.print();
        output.print();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition_point_merge, same)
{
    // Test that two identical coordsets create the same coordset
    conduit::Node braid;
    conduit::blueprint::mesh::examples::braid("tets", 2, 2, 2, braid);
    auto &braid_coordset = braid["coordsets/coords"];

    std::vector<const conduit::Node*> same;
    same.push_back(&braid_coordset); same.push_back(&braid_coordset);

    conduit::Node opts;
    opts["merge_tolerance"] = tolerance;

    conduit::Node output;
    conduit::blueprint::mesh::coordset::combine(same, output, &opts);

    conduit::Node info;
    bool different = braid_coordset["type"].diff(output["type"], info);
    different &= braid_coordset["values"].diff(output["values"], info);
    EXPECT_FALSE(different);
    if(different || always_print)
    {
        std::cout << "Input (x2):" << std::endl;
        braid_coordset.print();
        std::cout << "Output:" << std::endl;
        output.print();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition_point_merge, different)
{
    // Test that two different coordsets create the union of their coordinates
    conduit::Node braid;
    conduit::blueprint::mesh::examples::braid("tets", 2, 2, 2, braid);
    auto &braid_coordset = braid["coordsets/coords"];
    conduit::Node polytess;
    conduit::blueprint::mesh::examples::polytess(1, 1, polytess);
    auto &polytess_coordset = polytess["coordsets/coords"];

    std::vector<const conduit::Node*> different;
    different.push_back(&braid_coordset); different.push_back(&polytess_coordset);

    conduit::Node opts;
    opts["merge_tolerance"] = tolerance;

    conduit::Node output;
    conduit::blueprint::mesh::coordset::combine(different, output, &opts);

    conduit::Node info;
    bool is_different0 = different[0]->diff(output["coordsets/coords"], info);
    EXPECT_TRUE(is_different0);
    bool is_different1 = different[1]->diff(output["coordsets/coords"], info);
    EXPECT_TRUE(is_different1);

    static const std::string filename = baseline_file("pointmerge_different");
#ifdef GENERATE_BASELINES
    make_baseline(filename, output);
#else
    conduit::Node ans; load_baseline(filename, ans);
    // NOTE: If rebaselined we won't need to compare paths anymore
    bool is_output_different = ans["coordsets/coords/type"].diff(output["type"], info);
    is_output_different |= ans["coordsets/coords/values"].diff(output["values"], info);
    is_output_different |= ans["pointmaps"].diff(output["pointmaps"], info);
    EXPECT_FALSE(is_output_different);
    if(is_output_different)
    {
        std::cout << "Baseline:" << std::endl;
        ans.print();
        std::cout << "Output:" << std::endl;
        output.print();
    }
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition_point_merge, multidomain4)
{
    // Use the spiral example to attach domains that we know are connected
    conduit::Node spiral;
    conduit::blueprint::mesh::examples::spiral(4, spiral);
    std::vector<const conduit::Node*> multidomain;

    const conduit::index_t ndom = spiral.number_of_children();
    for(conduit::index_t i = 0; i < ndom; i++)
    {
        conduit::Node &dom = spiral.child(i);
        multidomain.push_back(dom.fetch_ptr("coordsets/coords"));
    }

    conduit::Node opts;
    opts["merge_tolerance"] = tolerance;

    conduit::Node output;
    conduit::blueprint::mesh::coordset::combine(multidomain, output, &opts);

    static const std::string filename = baseline_file("pointmerge_multidomain4");
#ifdef GENERATE_BASELINES
    make_baseline(filename, output);
#else
    conduit::Node ans; load_baseline(filename, ans);
    conduit::Node info;
    // NOTE: If rebaselined we won't need to compare paths anymore
    bool is_different = ans["coordsets/coords/type"].diff(output["type"], info);
    is_different |= ans["coordsets/coords/values"].diff(output["values"], info);
    is_different |= ans["pointmaps"].diff(output["pointmaps"], info);
    EXPECT_FALSE(is_different);
    if(is_different || always_print)
    {
        std::cout << "Baseline:" << std::endl;
        ans.print();
        std::cout << "Output:" << std::endl;
        output.print();
    }
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition_point_merge, multidomain8)
{
    // Use the spiral example to attach domains that we know are connected, with more data
    //   this makes sure that multiple nodes are used in the spatial search implementation
    conduit::Node spiral;
    conduit::blueprint::mesh::examples::spiral(8, spiral);
    std::vector<const conduit::Node*> multidomain;

    const conduit::index_t ndom = spiral.number_of_children();
    for(conduit::index_t i = 0; i < ndom; i++)
    {
        conduit::Node &dom = spiral.child(i);
        multidomain.push_back(dom.fetch_ptr("coordsets/coords"));
    }

    conduit::Node opts;
    opts["merge_tolerance"] = tolerance;

    conduit::Node output;
    conduit::blueprint::mesh::coordset::combine(multidomain, output, &opts);

    static const std::string filename = baseline_file("pointmerge_multidomain8");
#ifdef GENERATE_BASELINES
    make_baseline(filename, output);
#else
    conduit::Node ans; load_baseline(filename, ans);
    conduit::Node info;
    // NOTE: If rebaselined we won't need to compare paths anymore
    bool is_different = ans["coordsets/coords/type"].diff(output["type"], info);
    is_different |= ans["coordsets/coords/values"].diff(output["values"], info);
    is_different |= ans["pointmaps"].diff(output["pointmaps"], info);
    EXPECT_FALSE(is_different);
    if(is_different || always_print)
    {
        std::cout << "Baseline:" << std::endl;
        ans.print();
        std::cout << "Output:" << std::endl;
        output.print();
    }
#endif
}
#endif

//-----------------------------------------------------------------------------
//-- Combine topology --
//-----------------------------------------------------------------------------
static bool ph_faces_unique(const conduit::Node &topo)
{
    bool unique = true;
    const auto sizes = topo["subelements/sizes"].as_int_accessor();
    const auto connectivity = topo["subelements/connectivity"].as_index_t_accessor();
    std::vector<conduit::index_t> zids;

    // Iterate over the face definitions and make ids for them.
    std::set<conduit::uint64> uniqueFaces;
    conduit::index_t offset = 0;
    for(conduit::index_t fi = 0; fi < sizes.number_of_elements(); fi++)
    {
        const auto npts = sizes[fi];

        zids.clear();
        zids.reserve(npts);
        for(conduit::index_t i = 0; i < npts; i++)
            zids.push_back(connectivity[offset + i]);

        auto faceId = conduit::utils::hash(&zids[0], npts);
        auto it = uniqueFaces.find(faceId);
        bool notFound = it == uniqueFaces.end();
        if(notFound)
            uniqueFaces.insert(faceId);
        unique &= notFound;
        offset += npts;
    }
    return unique;
}

//-----------------------------------------------------------------------------
// #define DEBUG_RECOMBINE_BRAID
TEST(conduit_blueprint_mesh_combine, recombine_braid)
{
    const auto recombine_braid_case = [](const std::string &case_name, const conduit::index_t *vdims)
    {
        std::cout << "-------- Start case " << case_name << " --------" << std::endl;
        conduit::Node braid;
        conduit::blueprint::mesh::examples::braid(case_name, vdims[0], vdims[1], vdims[2], braid);
    #ifdef DEBUG_RECOMBINE_BRAID
        const std::string base_name = "recombine_" + case_name;
        save_visit(base_name + "_original", braid);
    #endif

        // First split the braid mesh
        conduit::Node split;
        {
            static const std::string split_yaml = "target: 2";

            conduit::Node split_opts; split_opts.parse(split_yaml, "yaml");

            conduit::blueprint::mesh::partition(braid, split_opts, split);
        #ifdef DEBUG_RECOMBINE_BRAID
            save_visit(base_name + "_split", split);
        #endif

            conduit::Node verify_info;
            bool is_valid = conduit::blueprint::mesh::verify(split, verify_info);
            if(!is_valid)
            {
                verify_info.print();
            }
            EXPECT_TRUE(is_valid);
        }

        // Now put it back together
        conduit::Node combine;
        {
            static const std::string combine_yaml = R"(target: 1)";
            conduit::Node combine_opts; combine_opts.parse(combine_yaml, "yaml");

            std::vector<const conduit::Node*> chunks;
            std::vector<conduit_index_t> chunk_ids;
            for(conduit_index_t i = 0; i < split.number_of_children(); i++)
            {
                chunks.push_back(&split[i]);
                chunk_ids.push_back(i);
            }

            conduit::blueprint::mesh::Partitioner p;
            p.combine(0, chunks, chunk_ids, combine);
        #ifdef DEBUG_RECOMBINE_BRAID
            save_visit(base_name + "_combined", combine);
        #endif
            conduit::Node verify_info;
            bool is_valid = conduit::blueprint::mesh::verify(combine, verify_info);
            if(!is_valid)
            {
                verify_info.print();
            }
            EXPECT_TRUE(is_valid);

            // If the combined mesh is polyhedral then make sure all the face
            // definitions are unique.
            if(combine.has_path("topologies/mesh/elements/shape") &&
               combine["topologies/mesh/elements/shape"].as_string() == "polyhedral")
            {
                bool unique = ph_faces_unique(combine["topologies/mesh"]);
                EXPECT_TRUE(unique);
            }
        }

        // Compare combined mesh to baselines
        const std::string filename = baseline_file("recombine_braid_" + case_name);
        save_visit("recombine_braid_" + case_name + "_combined", combine);
    #ifdef GENERATE_BASELINES
        make_baseline(filename, combine);
    #else
        conduit::Node ans; load_baseline(filename, ans);
        conduit::Node info;
        bool is_different = ans.diff(combine, info, CONDUIT_EPSILON, true);
        EXPECT_FALSE(is_different);
        if(is_different || always_print)
        {
            info.print();
        }
    #endif

        std::cout << "-------- End case " << case_name << "   --------" << std::endl;
    };

    static const conduit::index_t dims2[] = {11,11,0};
    static const std::array<std::string, 3> cases2 = {
        "tris",
        "quads",
        "quads_poly"
    };
    for(const auto &c : cases2)
    {
        recombine_braid_case(c, dims2);
    }

    static const conduit::index_t dims3[] = {3,3,2};
    static const std::array<std::string, 5> cases3 = {
        "tets",
        "hexs",
        "hexs_poly",
        "wedges",
        "pyramids"
    };
    for(const auto &c : cases3)
    {
        recombine_braid_case(c, dims3);
    }
}

//-----------------------------------------------------------------------------
// #define DEBUG_COMBINE_MULTIDOMAIN
TEST(conduit_blueprint_mesh_combine, multidomain)
{
    const std::string base_name = "combine_multidomain";

    const auto combine_multidomain_case = [&base_name](conduit::index_t ndom) {
        std::cout << "-------- Start case " << ndom << " --------" << std::endl;
        conduit::Node spiral;
        conduit::blueprint::mesh::examples::spiral(ndom, spiral);
    #ifdef DEBUG_COMBINE_MULTIDOMAIN
        save_visit(base_name + std::to_string(ndom) + "_input", spiral);
    #endif

        static const std::string opts_yaml = "target: 1";
        conduit::Node opts; opts.parse(opts_yaml, "yaml");

        conduit::Node output;
        conduit::blueprint::mesh::partition(spiral, opts, output);

    #ifdef DEBUG_COMBINE_MULTIDOMAIN
        save_visit(base_name + std::to_string(ndom) + "_output", output);
    #endif

    // TODO: Rebaseline when rectilinear -> rectilinear is supported
    const std::string filename = baseline_file(base_name + std::to_string(ndom));
    #ifdef GENERATE_BASELINES
        make_baseline(filename, output);
    #else
        conduit::Node ans; load_baseline(filename, ans);
        conduit::Node info;
        bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
        EXPECT_FALSE(is_different);
        if(is_different || always_print)
        {
            info.print();
        }
    #endif
        std::cout << "-------- End case " << ndom << " --------" << std::endl;
    };

    std::array<conduit::index_t, 4> cases{
        2,
        4,
        7,
        9
    };
    for(const auto c : cases)
    {
        combine_multidomain_case(c);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_combine, to_poly)
{
    const auto to_polys_case = [](const std::string &case_name, const conduit::index_t vdims[3])
    {
        std::cout << "-------- Start case " << case_name << " --------" << std::endl;
        const std::string base_name = "combine_to_poly_" + case_name;

        // Make a polygonal or polyhedral domain
        conduit::Node poly_braid;
        if(vdims[2] > 1)
        {
            conduit::blueprint::mesh::examples::braid("hexs_poly",
                                                      2,
                                                      2,
                                                      2,
                                                      poly_braid);

            // Move the points to the side
            std::array<std::array<double, 8>, 3> new_coords = {{
                {-30.0, -10.0, -30.0, -10.0, -30.0, -10.0, -30.0, -10.0},
                {-30.0, -30.0, -10.0, -10.0, -30.0, -30.0, -10.0, -10.0},
                {-30.0, -30.0, -30.0, -30.0, -10.0, -10.0, -10.0, -10.0}
            }};
            conduit::Node &n_coords = poly_braid["coordsets/coords/values"];
            for(conduit::index_t d = 0; d < 3; d++)
            {
                conduit::Node &n_dim = n_coords[d];
                if(n_dim.dtype().is_float32())
                {
                    conduit::float32_array vals = n_dim.value();
                    for(conduit::index_t vi = 0; vi < 8; vi++)
                    {
                        vals[vi] = static_cast<conduit::float32>(new_coords[d][vi]);
                    }
                }
                else if(n_dim.dtype().is_float64())
                {
                    conduit::float64_array vals = n_dim.value();
                    for(conduit::index_t vi = 0; vi < 8; vi++)
                    {
                        vals[vi] = new_coords[d][vi];
                    }
                }
                else
                {
                    CONDUIT_ERROR("Could not translate coordinates from type "
                        << n_dim.dtype().name() << ".");
                }
            }
        }
        else
        {
            conduit::blueprint::mesh::examples::braid("quads_poly", 2,
                2, 0, poly_braid);

            // Move the points to the side
            std::array<std::array<double, 4>, 2> new_coords = {{
                {-30.0, -10.0, -30.0, -10.0},
                {-30.0, -30.0, -10.0, -10.0},
            }};
            conduit::Node &n_coords = poly_braid["coordsets/coords/values"];
            for(conduit::index_t d = 0; d < 2; d++)
            {
                conduit::Node &n_dim = n_coords[d];
                if(n_dim.dtype().is_float32())
                {
                    conduit::float32_array vals = n_dim.value();
                    for(conduit::index_t vi = 0; vi < 4; vi++)
                    {
                        vals[vi] = static_cast<conduit::float32 > (new_coords[d][vi]);
                    }
                }
                else if(n_dim.dtype().is_float64())
                {
                    conduit::float64_array vals = n_dim.value();
                    for(conduit::index_t vi = 0; vi < 4; vi++)
                    {
                        vals[vi] = new_coords[d][vi];
                    }
                }
                else
                {
                    CONDUIT_ERROR("Could not translate coordinates from type "
                        << n_dim.dtype().name() << ".");
                }
            }
        }

        conduit::Node braid;
        conduit::blueprint::mesh::examples::braid(case_name, vdims[0], vdims[1], vdims[2], braid);

        conduit::Node input;
        input["domain_00000"] = poly_braid;
        input["domain_00000/state/domain_id"] = 0;
        input["domain_00001"] = braid;
        input["domain_00001/state/domain_id"] = 1;
    #ifdef DEBUG_TO_POLY
        save_visit(base_name + "_input", input);
    #endif

        const std::string opts_yaml = "target: 1";
        conduit::Node opts; opts.parse(opts_yaml, "yaml");

        conduit::Node output;
        conduit::blueprint::mesh::partition(input, opts, output);

    #ifdef DEBUG_TO_POLY
        save_visit(base_name + "_output", output);
    #endif

        const std::string filename = baseline_file(base_name);
    #ifdef GENERATE_BASELINES
        make_baseline(filename, output);
    #else
        conduit::Node ans; load_baseline(filename, ans);
        conduit::Node info;
        bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
        EXPECT_FALSE(is_different);
        if(is_different || always_print)
        {
            info.print();
        }
    #endif
        std::cout << "-------- End case " << case_name << "   --------" << std::endl;
    };

    static const conduit::index_t dims2[] = {11,11,0};
    static const std::array<std::string, 2> cases2 = {
        "tris",
        "quads",
    };
    for(const auto &c : cases2)
    {
        to_polys_case(c, dims2);
    }

    static const conduit::index_t dims3[] = {3,3,2};
    static const std::array<std::string, 4> cases3 = {
        "tets",
        "hexs",
        "wedges",
        "pyramids"
    };
    for(const auto &c : cases3)
    {
        to_polys_case(c, dims3);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_combine, uniform)
{
    using namespace conduit::blueprint::mesh::examples;
    const auto uniform_cases = [](bool is3d)
    {
        std::vector<conduit::Node> domains;
        const conduit::index_t nz = (is3d) ? 3 : 1;
        const std::string case_name = (is3d) ? "3d" : "2d";
        const std::string base_file_name = "combine_uniform_" + case_name;
        std::cout << "-------- Start case " << case_name << " --------" << std::endl;

        // 0
        domains.emplace_back();
        basic("uniform", 11, 6, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");

        // 1
        domains.emplace_back();
        basic("uniform", 6, 6, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 0;
        domains.back()["coordsets/coords/origin/y"] = 5;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 2
        domains.emplace_back();
        basic("uniform", 6, 6, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 5;
        domains.back()["coordsets/coords/origin/y"] = 5;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 3
        domains.emplace_back();
        basic("uniform", 5, 6, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 0;
        domains.back()["coordsets/coords/origin/y"] = 10;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 4
        domains.emplace_back();
        basic("uniform", 3, 3, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 4;
        domains.back()["coordsets/coords/origin/y"] = 13;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 5
        domains.emplace_back();
        basic("uniform", 2, 4, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 4;
        domains.back()["coordsets/coords/origin/y"] = 10;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 6
        domains.emplace_back();
        basic("uniform", 4, 4, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 5;
        domains.back()["coordsets/coords/origin/y"] = 10;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 7
        domains.emplace_back();
        basic("uniform", 3, 6, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 8;
        domains.back()["coordsets/coords/origin/y"] = 10;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // 8
        domains.emplace_back();
        basic("uniform", 3, 3, nz, domains.back());
        domains.back().remove("coordsets/coords/origin");
        domains.back().remove("coordsets/coords/spacing");
        domains.back()["coordsets/coords/origin/x"] = 6;
        domains.back()["coordsets/coords/origin/y"] = 13;
        if(is3d)
            domains.back()["coordsets/coords/origin/z"] = 0;

        // Nodes that are reused through each partition call
        conduit::Node opts;
        opts["target"] = 1;
        conduit::Node output;

        // Mesh 0
        conduit::Node mesh0;
        for(conduit::index_t i = 0; i < static_cast<conduit::index_t>(domains.size()); i++)
        {
            domains[i]["state/domain_id"] = i;
            mesh0[(i < 10)
                ? ("domain_0000" + std::to_string(i))
                : ("domain_000" + std::to_string(i))] = domains[i];
        }
        save_visit(base_file_name + "_mesh0", mesh0);
        std::cout << "mesh0" << std::endl;
        conduit::blueprint::mesh::partition(mesh0, opts, output);
        save_visit(base_file_name + "_mesh0_output", output);

        {
            const std::string filename = baseline_file(base_file_name + "_mesh0");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }

        // Mesh1 missing a section
        conduit::Node mesh1;
        for(conduit::index_t i = 0; i < static_cast<conduit::index_t>(domains.size()); i++)
        {
            if(i == 6)
            {
                if(!is3d)
                {
                    continue;
                }
                else
                {
                    // For 3d make it so the domain exists just not lined up properly
                    domains[i]["coordsets/coords/origin/z"] = 1;
                }
            }
            mesh1[(i < 10)
                ? ("domain_0000" + std::to_string(i))
                : ("domain_000" + std::to_string(i))] = domains[i];
        }
        save_visit(base_file_name + "_mesh1", mesh1);

        std::cout << "mesh1" << std::endl;
        conduit::blueprint::mesh::partition(mesh1, opts, output);
        save_visit(base_file_name + "_mesh1_output", output);

        {
            const std::string filename = baseline_file(base_file_name + "_mesh1");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }

        // Mesh 2
        conduit::Node mesh2;
        mesh2["domain_00000"] = domains[1];
        mesh2["domain_00001"] = domains[2];
        std::cout << "mesh2" << std::endl;
        save_visit(base_file_name + "_mesh2", mesh2);
        conduit::blueprint::mesh::partition(mesh2, opts, output);
        // output.print();
        save_visit(base_file_name + "_mesh2_output", output);

        {
            const std::string filename = baseline_file(base_file_name + "_mesh2");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }

        std::cout << "mesh3" << std::endl;
        // change the spacing for domain00001, should suggest rectilinear
        mesh2["domain_00001/coordsets/coords/spacing/dx"] = 0.5;
        mesh2["domain_00001/coordsets/coords/spacing/dy"] = 1.0;
        if(is3d)
            mesh2["domain_00001/coordsets/coords/spacing/dz"] = 1.0;
        conduit::blueprint::mesh::partition(mesh2, opts, output);
        save_visit(base_file_name + "_mesh3_output", output);

        {
            const std::string filename = baseline_file(base_file_name + "_mesh3");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }

        std::cout << "-------- End case " << case_name << "   --------" << std::endl;
    };

    uniform_cases(false);
    uniform_cases(true);
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_combine, rectilinear)
{
    conduit::Node spiral;
    conduit::blueprint::mesh::examples::spiral(5, spiral);

    conduit::Node opts; opts["target"] = 1;
    conduit::Node combined;
    conduit::blueprint::mesh::partition(spiral, opts, combined);
    save_visit("combine_rectilinear_output1", combined);

    {
        const std::string filename = baseline_file("combine_rectilinear");
    #ifdef GENERATE_BASELINES
        make_baseline(filename, combined);
    #else
        conduit::Node ans; load_baseline(filename, ans);
        conduit::Node info;
        bool is_different = ans.diff(combined, info, CONDUIT_EPSILON, true);
        EXPECT_FALSE(is_different);
        if(is_different || always_print)
        {
            info.print();
        }
    #endif
    }
}

//-----------------------------------------------------------------------------
// STRUCTURED COMBINE FUNCTIONS AND TESTS
//-----------------------------------------------------------------------------
void create_structured_domain(conduit::Node &out, const int domain_id,
    const int dimension, const int *dims, const int *reorder,
    const double *origin, const double *scale, const double *g_origin)
{
    using namespace conduit;

    // Reorder the dims
    int real_dims[3];
    for(int i = 0; i < dimension; i++)
    {
        real_dims[i] = dims[reorder[i]];
    }

    // Number of verticies/elements
    int nverts = 1, nelems = 1;
    for(int i = 0; i < dimension; i++)
    {
        nverts *= real_dims[i];
        nelems *= (real_dims[i] - 1);
    }


    // Define the mesh in conduit blueprint form
    out["state/domain_id"] = (index_t)domain_id;
    out["topologies/mesh/type"] = "structured";
    out["topologies/mesh/coordset"] = "coords";
    out["topologies/mesh/elements/dims/i"] = real_dims[0]-1;
    out["topologies/mesh/elements/dims/j"] = real_dims[1]-1;
    if(dimension > 2)
        out["topologies/mesh/elements/dims/k"] = real_dims[2]-1;
    out["coordsets/coords/type"] = "explicit";
    Schema s;
    s["x"].set(DataType::c_double(nverts,               0, sizeof(double)*dimension));
    s["y"].set(DataType::c_double(nverts,   sizeof(double), sizeof(double)*dimension));
    if(dimension > 2)
        s["z"].set(DataType::c_double(nverts, 2*sizeof(double), sizeof(double)*dimension));
    out["coordsets/coords/values"].set(s);
    out["fields/vert_field/topology"] = "mesh";
    out["fields/vert_field/association"] = "vertex";
    out["fields/vert_field/values"].set(DataType::c_double(nverts));
    out["fields/dist/topology"] = "mesh";
    out["fields/dist/association"] = "vertex";
    out["fields/dist/values"].set(DataType::c_double(nverts));
    out["fields/elem_field/topology"] = "mesh";
    out["fields/elem_field/association"] = "element";
    out["fields/elem_field/values"].set(DataType::c_double(nelems));

    // Extract the pointers to the allocated memory
    double *coords = (double*)out["coordsets/coords/values"].element_ptr(0);
    double *vfield = (double*)out["fields/vert_field/values"].element_ptr(0);
    double *dist   = (double*)out["fields/dist/values"].element_ptr(0);
    double *efield = (double*)out["fields/elem_field/values"].element_ptr(0);

    int idx = 0;
    int id  = 0;
    if(dimension == 3)
    {
        for(int k = 0; k < real_dims[2]; k++)
        {
            for(int j = 0; j < real_dims[1]; j++)
            {
                for(int i = 0; i < real_dims[0]; i++, id++, idx+=3)
                {
                    const double temp[3] = {
                        (double)i * scale[reorder[0]],
                        (double)j * scale[reorder[1]],
                        (double)k * scale[reorder[2]]
                    };
                    coords[idx]   = origin[0] + temp[reorder[0]];
                    coords[idx+1] = origin[1] + temp[reorder[1]];
                    coords[idx+2] = origin[2] + temp[reorder[2]];
                    vfield[id] = id;

                    const double dx = coords[idx]   - g_origin[0];
                    const double dy = coords[idx+1] - g_origin[1];
                    const double dz = coords[idx+2] - g_origin[2];
                    dist[id] = std::sqrt(dx*dx + dy*dy + dz*dz);
                }
            }
        }

        id = 0;
        for(int k = 0; k < real_dims[2]-1; k++)
        {
            for(int j = 0; j < real_dims[1]-1; j++)
            {
                for(int i = 0; i < real_dims[0]-1; i++, id++)
                {
                    efield[id] = id;
                }
            }
        }
    }
    else if(dimension == 2)
    {
        for(int j = 0; j < real_dims[1]; j++)
        {
            for(int i = 0; i < real_dims[0]; i++, id++, idx+=2)
            {
                const double temp[2] = {(double)i * scale[0], (double)j * scale[1]};
                coords[idx]   = origin[0] + temp[reorder[0]];
                coords[idx+1] = origin[1] + temp[reorder[1]];
                vfield[id] = id;

                const double dx = coords[idx]   - g_origin[0];
                const double dy = coords[idx+1] - g_origin[1];
                dist[id] = std::sqrt(dx*dx + dy*dy);
            }
        }

        id = 0;
        for(int j = 0; j < real_dims[1]-1; j++)
        {
            for(int i = 0; i < real_dims[0]-1; i++, id++)
            {
                efield[id] = id;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void create_grain_case(conduit::Node &out)
{
    out.reset();

    // For all domains
    const int dims[3] = {3, 4, 2};
    const double g_origin[3] = {0., 0., 0.};

    // Cases
    const int origin_x[8] = {0,    1,   0,    1,   0,   1,     0,   1};
    const int origin_y[8] = {0,    0,   1,    1,   0,   0,     1,   1};
    const double scale_x[8]= {1., -1.,   1.,  -1.,  1., -1.,   1.,  -1.};
    const double scale_y[8]= {1.,  1.,  -1.,  -1.,  1.,  1.,  -1.,  -1.};
    const int reorder_x[8]= {0,    0,    0,    0,   1,   1,     1,   1};
    const int reorder_y[8]= {1,    1,    1,    1,   0,   0,     0,   0};
    int id = 0;

    // TODO: Make a 2D case
    // TODO: Also K
    for(int j = 0; j < 8; j++)
    {
        // {ymin, ymax}
        int y[2];
        y[0] = (int)g_origin[1] + (j * (dims[1]-1));
        y[1] = (int)g_origin[1] + ((j+1) * (dims[1]-1));

        const int start_case = j;
        for(int i = 0; i < 8; i++, id++)
        {
            // {xmin, xmax}
            int x[2];
            x[0] = (int)g_origin[0] + (i * (dims[0]-1));
            x[1] = (int)g_origin[0] + (i+1) * (dims[0]-1);

            const int c = (start_case + i) % 8;
            double o[3];
            double s[3];
            int   r[3];
            o[0] = (double)x[origin_x[c]];
            o[1] = (double)y[origin_y[c]];
            o[2] = 0;
            s[0] = scale_x[c];
            s[1] = scale_y[c];
            s[2] = 1.f;
            r[0] = reorder_x[c];
            r[1] = reorder_y[c];
            r[2] = 2;

            std::string domain_name = (id > 10 ? "domain_000" + std::to_string(id) : "domain_0000" + std::to_string(id));
            create_structured_domain(out[domain_name], id, 3, dims, r, o, s, g_origin);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(blueprint_mesh_combine, structured)
{
    const auto braid_cases = [](bool is3d) {
        const std::string base_name = "combine_structured";
        const std::string case_name = (is3d) ? "3d" : "2d";
        const std::string file_name = base_name + "_" + case_name;
        conduit::Node braid;
        conduit::blueprint::mesh::examples::braid("structured", 11, 11,
            (is3d) ? 11 : 1, braid);
        save_visit("Braid" + case_name + "Structured", braid);

        conduit::Node opts;
        opts["target"] = 2;

        conduit::Node split;
        conduit::blueprint::mesh::partition(braid, opts, split);
        save_visit(file_name + "_input", split);

        opts["target"] = 1;
        conduit::Node combined;
        conduit::blueprint::mesh::partition(split, opts, combined);
        save_visit(file_name + "_output", combined);
        {
            const std::string filename = baseline_file(file_name);
        #ifdef GENERATE_BASELINES
            make_baseline(filename, combined);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(combined, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }
    };
    braid_cases(false);
    braid_cases(true);

    const auto curve_test = [](const int *dims, conduit::Node &output) {
        using namespace conduit;
        const double PI_VALUE = 3.14159265359;
        const int N = dims[0]*dims[1]*dims[2];
        const int Nelem = (dims[0]-1)*(dims[1]-1)*(dims[2]-1);

        // Domain 0
        Node &n_d0 = output["domain_00000"];
        {
            const double dt = PI_VALUE / (dims[0]-1);
            int id  = 0;
            int idx = 0;
            int k, j, i;

            n_d0["state/domain_id"] = (index_t)0;
            n_d0["topologies/mesh/type"] = "structured";
            n_d0["topologies/mesh/coordset"] = "coords";
            n_d0["topologies/mesh/elements/dims/i"] = dims[0]-1;
            n_d0["topologies/mesh/elements/dims/j"] = dims[1]-1;
            if(dims[2] > 1)
                n_d0["topologies/mesh/elements/dims/k"] = dims[2]-1;
            n_d0["coordsets/coords/type"] = "explicit";
            Schema s;
            s["x"].set(DataType::c_double(N,               0, sizeof(double)*3));
            s["y"].set(DataType::c_double(N,   sizeof(double), sizeof(double)*3));
            s["z"].set(DataType::c_double(N, 2*sizeof(double), sizeof(double)*3));
            n_d0["coordsets/coords/values"].set(s);
            n_d0["fields/vert_field/topology"] = "mesh";
            n_d0["fields/vert_field/association"] = "vertex";
            n_d0["fields/vert_field/values"].set(DataType::c_double(N));
            n_d0["fields/elem_field/topology"] = "mesh";
            n_d0["fields/elem_field/association"] = "element";
            n_d0["fields/elem_field/values"].set(
                DataType::c_double(Nelem));

            double *coords = (double*)n_d0["coordsets/coords/values"].element_ptr(0);
            double *vfield = (double*)n_d0["fields/vert_field/values"].element_ptr(0);
            double *efield = (double*)n_d0["fields/elem_field/values"].element_ptr(0);

            for(k = 0; k < dims[2]; k++)
            {
                for(j = 0; j < dims[1]; j++)
                {
                    const int j1 = j+1;
                    double t = 0.;
                    for(i = 0; i < dims[0]; i++, id++, t+=dt)
                    {
                        vfield[id]     = id;
#if 1
                        coords[idx++] = std::cos(t) * j1;
                        coords[idx++] = std::sin(t) * j1;
#else
                        coords[idx++] = j1;
                        coords[idx++] = i;
#endif
                        coords[idx++] = k;
                    }
                }
            }

            for(int i = 0; i < Nelem; i++)
            {
                efield[i] = i;
            }
        }

        Node &n_d1 = output["domain_00001"];
        {
            int id  = 0;
            int idx = 0;
            int k, j, i;

            n_d1["state/domain_id"] = (index_t)1;
            n_d1["topologies/mesh/type"] = "structured";
            n_d1["topologies/mesh/coordset"] = "coords";
            n_d1["topologies/mesh/elements/dims/i"] = dims[0]-1;
            n_d1["topologies/mesh/elements/dims/j"] = dims[1]-1;
            if(dims[2] > 1)
                n_d1["topologies/mesh/elements/dims/k"] = dims[2]-1;
            n_d1["coordsets/coords/type"] = "explicit";
            Schema s;
            s["x"].set(DataType::c_double(N,               0, sizeof(double)*3));
            s["y"].set(DataType::c_double(N,   sizeof(double), sizeof(double)*3));
            s["z"].set(DataType::c_double(N, 2*sizeof(double), sizeof(double)*3));
            n_d1["coordsets/coords/values"].set(s);
            n_d1["fields/vert_field/topology"] = "mesh";
            n_d1["fields/vert_field/association"] = "vertex";
            n_d1["fields/vert_field/values"].set(DataType::c_double(N));
            n_d1["fields/elem_field/topology"] = "mesh";
            n_d1["fields/elem_field/association"] = "element";
            n_d1["fields/elem_field/values"].set(
                DataType::c_double((dims[0]-1)*(dims[1]-1)*(dims[2]-1)));

            double *coords = (double*)n_d1["coordsets/coords/values"].element_ptr(0);
            double *vfield = (double*)n_d1["fields/vert_field/values"].element_ptr(0);
            double *efield = (double*)n_d1["fields/elem_field/values"].element_ptr(0);

            for(k = 0; k < dims[2]; k++)
            {
                for(j = 0; j < dims[1]; j++)
                {
                    const int j1 = j+1;
                    for(i = 0; i < dims[0]; i++, id++)
                    {
                        vfield[id]     = id;
                        coords[idx++] = j1;
                        coords[idx++] = -i;
                        coords[idx++] = k;
                    }
                }
            }

            for(int i = 0; i < Nelem; i++)
            {
                efield[i] = i;
            }
        }
    };

    // TODO: Create a case that combines along a curved face
    {
        conduit::Node curved;
        const int dims[3] = {7,5,3};
        curve_test(dims, curved);
        save_visit("combine_structured_curved_3d_input", curved);

        conduit::Node opts; opts["target"] = 1;
        conduit::Node output;
        conduit::blueprint::mesh::partition(curved, opts, output);
        save_visit("combine_structured_curved_3d_output", output);

        {
            const std::string filename = baseline_file("combine_structured_curved_3d");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }
    }

    // TODO: Update the "grain_case" to permute the domains over K
    // TODO: Create a 2D case
    {
        conduit::Node grain;
        create_grain_case(grain);

        save_visit("combine_structured_grain_3d_input", grain);

        conduit::Node opts; opts["target"] = 1;
        conduit::Node output;
        conduit::blueprint::mesh::partition(grain, opts, output);
        save_visit("combine_structured_grain_3d_output", output);
        {
            const std::string filename = baseline_file("combine_structured_grain_3d");
        #ifdef GENERATE_BASELINES
            make_baseline(filename, output);
        #else
            conduit::Node ans; load_baseline(filename, ans);
            conduit::Node info;
            bool is_different = ans.diff(output, info, CONDUIT_EPSILON, true);
            EXPECT_FALSE(is_different);
            if(is_different || always_print)
            {
                info.print();
            }
        #endif
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, field_selection)
{
    std::string base("field_selection");
    conduit::Node input, output, options;
    partition::make_field_selection_example(input, -1);
    save_visit("fs", input);

    const char *opt0 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: 0\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 1\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 2\n"
"     field: selection_field\n"
"   -\n"
"     type: field\n"
"     domain_id: 3\n"
"     field: selection_field\n";
    options.reset(); options.parse(opt0, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 6);
    std::string b00 = baseline_file(base + "_00");
    save_visit(b00, output);
#ifdef GENERATE_BASELINES
    make_baseline(b00, output);
#else
    EXPECT_EQ(compare_baseline(b00, output), true);
#endif

    // Test domain_id: any
    const char *opt1 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: any\n"
"     field: selection_field\n";
    options.reset(); options.parse(opt1, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 6);
    std::string b01 = baseline_file(base + "_01");
    save_visit(b01, output);
#ifdef GENERATE_BASELINES
    make_baseline(b01, output);
#else
    EXPECT_EQ(compare_baseline(b01, output), true);
#endif

    // Test "target: 10". We can split field selections further as
    // explicit selections.
    const char *opt2 =
"selections:\n"
"   -\n"
"     type: field\n"
"     domain_id: any\n"
"     field: selection_field\n"
"target: 10\n";
    options.reset(); options.parse(opt2, "yaml");
    conduit::blueprint::mesh::partition(input, options, output);
    EXPECT_EQ(conduit::blueprint::mesh::number_of_domains(output), 10);
    std::string b02 = baseline_file(base + "_02");
    save_visit(b02, output);
#ifdef GENERATE_BASELINES
    make_baseline(b02, output);
#else
    EXPECT_EQ(compare_baseline(b02, output), true);
#endif
}

//-----------------------------------------------------------------------------
static bool
diff_to_silo(const conduit::Node &baseline, const conduit::Node &matset,
    conduit::Node &info)
{
    const conduit::Node &baseline_matset = baseline["matsets/matset"];
    // std::cout << "Baseline matset:" << baseline_matset.to_yaml() << std::endl;

    conduit::Node base_silo;
    conduit::blueprint::mesh::matset::to_silo(baseline_matset, base_silo);

    conduit::Node test_silo;
    conduit::blueprint::mesh::matset::to_silo(matset["matsets/matset"], test_silo);

    base_silo.remove_child("buffer_style");
    base_silo.remove_child("dominance");
    test_silo.remove_child("buffer_style");
    test_silo.remove_child("dominance");

    info.reset();
    return base_silo.diff(test_silo, info, CONDUIT_EPSILON, true);
}

//-----------------------------------------------------------------------------
// Multi-buffer, element-dominant matset
TEST(conduit_blueprint_mesh_partition, matset_multi_by_element)
{
    /// matset_type options:
    ///   full -> non sparse volume fractions and matset values
    ///   sparse_by_material ->  sparse (material dominant) volume fractions
    ///                          and matset values
    ///   sparse_by_element  ->  sparse (element dominant)
    ///                          volume fractions and matset values
    conduit::Node venn;
    conduit::blueprint::mesh::examples::venn("full", 4, 4, 0.33f, venn);

    save_visit("venn_multi_by_element", venn, true);

    conduit::Node venn_part, opts; opts["target"].set(4);
    conduit::blueprint::mesh::partition(venn, opts, venn_part);

    // Check partitioned result against baseline
    {
        const std::string name = "venn_multi_by_element_partitioned";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_part, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_part);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_part, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }

    conduit::Node venn_combined; opts["target"].set(1);
    conduit::blueprint::mesh::partition(venn_part, opts, venn_combined);

    // Test combined vs original "to_silo" results
    {
        conduit::Node info;
        EXPECT_FALSE(diff_to_silo(venn, venn_combined, info)) << info.to_yaml();
    }

    // Check combined result against baseline
    {
        const std::string name = "venn_multi_by_element_combined";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_combined, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_combined);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_combined, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }
}

//-----------------------------------------------------------------------------
// Multi-buffer, material-dominant matset
TEST(conduit_blueprint_mesh_partition, matset_multi_by_material)
{
    /// matset_type options:
    ///   full -> non sparse volume fractions and matset values
    ///   sparse_by_material ->  sparse (material dominant) volume fractions
    ///                          and matset values
    ///   sparse_by_element  ->  sparse (element dominant)
    ///                          volume fractions and matset values
    conduit::Node venn;
    conduit::blueprint::mesh::examples::venn("sparse_by_material", 4, 4, 0.33f, venn);

    save_visit("venn_multi_by_material", venn, true);

    conduit::Node venn_part, opts; opts["target"].set(4);
    conduit::blueprint::mesh::partition(venn, opts, venn_part);

    // Check partitioned result against baseline
    {
        const std::string name = "venn_multi_by_material_partitioned";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_part, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_part);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_part, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }

    conduit::Node venn_combined; opts["target"].set(1);
    conduit::blueprint::mesh::partition(venn_part, opts, venn_combined);

    // Test combined vs original "to_silo" results
    {
        conduit::Node info;
        EXPECT_FALSE(diff_to_silo(venn, venn_combined, info)) << info.to_yaml();
    }

    // Check combined result against baseline
    {
        const std::string name = "venn_multi_by_material_combined";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_combined, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_combined);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_combined, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }
}

//-----------------------------------------------------------------------------
// Uni-buffer, element-dominant matset
TEST(conduit_blueprint_mesh_partition, matset_uni_by_element)
{
    /// matset_type options:
    ///   full -> non sparse volume fractions and matset values
    ///   sparse_by_material ->  sparse (material dominant) volume fractions
    ///                          and matset values
    ///   sparse_by_element  ->  sparse (element dominant)
    ///                          volume fractions and matset values
    conduit::Node venn;
    conduit::blueprint::mesh::examples::venn("sparse_by_element", 4, 4, 0.33f, venn);

    save_visit("venn_uni_by_element", venn, true);

    conduit::Node venn_part, opts; opts["target"].set(4);
    conduit::blueprint::mesh::partition(venn, opts, venn_part);

    // Check partitioned result against baseline
    {
        const std::string name = "venn_uni_by_element_partitioned";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_part, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_part);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_part, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }

    conduit::Node venn_combined; opts["target"].set(1);
    conduit::blueprint::mesh::partition(venn_part, opts, venn_combined);

    // Test combined vs original "to_silo" results
    {
        conduit::Node info;
        EXPECT_FALSE(diff_to_silo(venn, venn_combined, info)) << info.to_yaml();
    }

    // Check combined result against baseline
    {
        const std::string name = "venn_uni_by_element_combined";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_combined, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_combined);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_combined, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }
}

//-----------------------------------------------------------------------------
// Uni-buffer, material-dominant matset
TEST(conduit_blueprint_mesh_partition, matset_uni_by_material)
{
    const int nx = 4;
    const int ny = 4;
    conduit::Node venn;
    conduit::blueprint::mesh::examples::venn("sparse_by_element", nx, ny, 0.33f, venn);

    // Add an element ids field
    const conduit::index_t N = conduit::blueprint::mesh::topology::length(venn["topologies"][0]);
    std::vector<conduit::index_t> ids;
    for(conduit::index_t i = 0; i < N; i++)
    {
        ids.push_back(i);
    }
    venn["matsets/matset/element_ids"].set(ids);

    save_visit("venn_uni_by_material", venn, true);

    conduit::Node venn_part, opts; opts["target"].set(4);
    conduit::blueprint::mesh::partition(venn, opts, venn_part);

    // Check partitioned result against baseline
    {
        const std::string name = "venn_uni_by_material_partitioned";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_part, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_part);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_part, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }

    conduit::Node venn_combined; opts["target"].set(1);
    conduit::blueprint::mesh::partition(venn_part, opts, venn_combined);

    // Test combined vs original "to_silo" results
    {
        conduit::Node info;
        EXPECT_FALSE(diff_to_silo(venn, venn_combined, info)) << info.to_yaml();
    }

    // Check combined result against baseline
    {
        const std::string name = "venn_uni_by_material_combined";
        const std::string baseline_fname = baseline_file(name);
        save_visit(name, venn_combined, true);
    #ifdef GENERATE_BASELINES
        make_baseline(baseline_fname, venn_combined);
    #else
        conduit::Node baseline, info;
        load_baseline(baseline_fname, baseline);
        EXPECT_FALSE(baseline.diff(venn_combined, info, CONDUIT_EPSILON, true)) << info.to_yaml();
    #endif
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, matset_mixed_topology)
{
    // Baseline mesh
    const int nx = 4;
    const int ny = 4;
    conduit::Node venn;
    conduit::blueprint::mesh::examples::venn("full", nx, ny, 0.33f, venn);

    conduit::Node venn_silo;
    conduit::blueprint::mesh::matset::to_silo(venn["matsets/matset"], venn_silo);

    // Input meshes, 1 for each flavor of matset
    std::array<conduit::Node, 4> meshes;
    conduit::blueprint::mesh::examples::venn("full", nx, ny, 0.33f, meshes[0]);
    conduit::blueprint::mesh::examples::venn("sparse_by_material", nx, ny, 0.33f, meshes[1]);
    conduit::blueprint::mesh::examples::venn("sparse_by_element", nx, ny, 0.33f, meshes[2]);
    conduit::blueprint::mesh::examples::venn("sparse_by_element", nx, ny, 0.33f, meshes[3]);
    // Make meshes[3] material dominant by adding element_ids
    {
        const conduit::index_t N = conduit::blueprint::mesh::topology::length(meshes[3]["topologies"][0]);
        std::vector<conduit::index_t> ids;
        for(conduit::index_t i = 0; i < N; i++)
        {
            ids.push_back(i);
        }
        meshes[3]["matsets/matset/element_ids"].set(ids);
    }

    // We've already tested partitioning / combining each of the above meshes in their
    //  rectilinear form; now we will use to_structured / to_unstructured and ensure
    //  the same result comes from to_silo
    const auto test = [](const conduit::Node &in, conduit::Node &out)
    {
        conduit::Node partitioned;
        conduit::Node opts;
        opts["target"].set(4);
        conduit::blueprint::mesh::partition(in, opts, partitioned);

        opts["target"].set(1);
        conduit::blueprint::mesh::partition(partitioned, opts, out);
    };

    // First test as rectilinear
    for(auto i = 0u; i < meshes.size(); i++)
    {
        conduit::Node result;
        test(meshes[i], result);

        conduit::Node info;
        bool diff = diff_to_silo(venn, result, info);
        EXPECT_FALSE(diff) << "Rectilinear case " << i << ": " << info.to_yaml();
    }

    // Now test as structured
    for(auto i = 0u; i < meshes.size(); i++)
    {
        // Transform the mesh
        const conduit::Node &mesh = meshes[i];
        // std::cout << "Mesh " << i << ":" << mesh.to_yaml() << std::endl;

        conduit::Node structured;
        conduit::blueprint::mesh::topology::rectilinear::to_structured(mesh["topologies/topo"],
            structured["topologies/topo"], structured["coordsets/coords"]);
        structured["fields"].set_external(mesh["fields"]);
        structured["matsets"].set_external(mesh["matsets"]);

        // Partition / combine
        conduit::Node result;
        test(mesh, result);

        // Compare to baseline
        conduit::Node info;
        bool diff = diff_to_silo(venn, result, info);
        EXPECT_FALSE(diff) << "Structured case " << i << ": " << info.to_yaml();
    }

    // Now test as unstructured
    for(auto i = 0u; i < meshes.size(); i++)
    {
        // Transform the mesh
        const conduit::Node &mesh = meshes[i];
        // std::cout << "Mesh " << i << ":" << mesh.to_yaml() << std::endl;

        conduit::Node unstructured;
        conduit::blueprint::mesh::topology::rectilinear::to_unstructured(mesh["topologies/topo"],
            unstructured["topologies/topo"], unstructured["coordsets/coords"]);
        unstructured["fields"].set_external(mesh["fields"]);
        unstructured["matsets"].set_external(mesh["matsets"]);

        // Partition / combine
        conduit::Node result;
        test(mesh, result);

        // Compare to baseline
        conduit::Node info;
        bool diff = diff_to_silo(venn, result, info);
        EXPECT_FALSE(diff) << "Unstructured case " << i << ": " << info.to_yaml();
    }
}


//-----------------------------------------------------------------------------
/**
 @brief Creates a matset for a spiral domain with the given number of elements.
        Flavors: 0 = multi-elem, 1 = multi-mat, 2 = uni-elem, 3 = uni-mat
*/
static void
make_spiral_matset(const conduit::index_t num_elements, const conduit::index_t flavor,
                   const conduit::index_t domain_id, const conduit::index_t total_domains,
                   conduit::Node &out_matset)
{
    out_matset["topology"].set("topo");

    // Uni buffer requires material map
    if(flavor > 1)
    {
        for(conduit::index_t i = 0; i < total_domains; i++)
        {
            const std::string mat_name("mat" + std::to_string(i));
            out_matset["material_map"][mat_name].set(i);
        }
    }

    const std::string mat_name("mat" + std::to_string(domain_id));
    switch(flavor)
    {
    case 1:
    {
        conduit::Node &mat_elem_ids = out_matset["element_ids"].add_child(mat_name);
        mat_elem_ids.set_dtype(conduit::DataType::index_t(num_elements));
        conduit::DataArray<conduit::index_t> data = mat_elem_ids.value();
        for(conduit::index_t i = 0; i < data.number_of_elements(); i++)
        {
            data[i] = i;
        }
        // Fallthrough
    }
    case 0:
    {
        conduit::Node &mat_vfs = out_matset["volume_fractions"].add_child(mat_name);
        mat_vfs.set_dtype(conduit::DataType::c_float(num_elements));
        conduit::DataArray<float> data = mat_vfs.value();
        for(conduit::index_t i = 0; i < data.number_of_elements(); i++)
        {
            data[i] = 1.f;
        }
        break;
    }
    default: //case 3
    {
        conduit::Node &mat_elem_ids = out_matset["element_ids"];
        mat_elem_ids.set_dtype(conduit::DataType::index_t(num_elements));
        conduit::DataArray<conduit::index_t> data = mat_elem_ids.value();
        for(conduit::index_t i = 0; i < data.number_of_elements(); i++)
        {
            data[i] = i;
        }
        // Fallthrough
    }
    case 2:
    {
        conduit::Node &mat_ids = out_matset["material_ids"];
        mat_ids.set_dtype(conduit::DataType::index_t(num_elements));
        conduit::DataArray<conduit::index_t> ids = mat_ids.value();
        for(conduit::index_t i = 0; i < ids.number_of_elements(); i++)
        {
            ids[i] = domain_id;
        }

        conduit::Node &mat_vfs = out_matset["volume_fractions"];
        mat_vfs.set_dtype(conduit::DataType::c_float(num_elements));
        conduit::DataArray<float> data = mat_vfs.value();
        for(conduit::index_t i = 0; i < data.number_of_elements(); i++)
        {
            data[i] = 1.f;
        }

        // conduit::Node &sizes = out_matset["sizes"];
        // sizes.set_dtype(conduit::DataType::index_t(num_elements));
        // conduit::DataArray<conduit::index_t> szs = sizes.value();
        // conduit::Node &offsets = out_matset["offsets"];
        // offsets.set_dtype(conduit::DataType::index_t(num_elements));
        // conduit::DataArray<conduit::index_t> offs = offsets.value();
        // conduit::index_t sum = 0;
        // for(conduit::index_t i = 0; i < szs.number_of_elements(); i++)
        // {
        //     szs[i] = 1;
        //     offs[i] = sum;
        //     sum++;
        // }
        break;
    }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, matset_spiral)
{
    std::array<conduit::Node, 4> spirals;
    {
        conduit::Node spiral;
        conduit::blueprint::mesh::examples::spiral(5, spiral);

        for(auto i = 0u; i < spirals.size(); i++)
        {
            spirals[i].set(spiral);
        }
    }

    // Add a matset to each domain
    for(conduit::index_t flavor = 0; flavor < (conduit::index_t)spirals.size(); flavor++)
    {
        conduit::Node &spiral = spirals[flavor];
        for(conduit::index_t i = 0; i < spiral.number_of_children(); i++)
        {
            conduit::Node &domain = spiral[i];
            const auto num_elements = conduit::blueprint::mesh::topology::length(domain["topologies/topo"]);
            conduit::Node &matset = domain["matsets/matset"];
            make_spiral_matset(num_elements, flavor, i, spiral.number_of_children(), matset);
            conduit::Node info;
            ASSERT_TRUE(conduit::blueprint::mesh::matset::verify(matset, info))
                << "Flavor " << flavor << ", domain " << i << ":" << info.to_yaml() << matset.to_yaml();
        }
    }

    // Test combining the spiral mesh with a matset down to 1 domain
    {
        // Use the first spiral mesh to create the baseline file
        const std::string baseline_fname = baseline_file("spiral_with_matset");
#ifdef GENERATE_BASELINES
        {
            conduit::Node opts, spiral_combined;
            opts["target"].set(1);
            conduit::blueprint::mesh::partition(spirals[0], opts, spiral_combined);
            make_baseline(baseline_fname, spiral_combined);
        }
#endif

        // Load the baseline mesh into a node, we will call diff_to_silo on this for each mesh
        conduit::Node baseline;
        load_baseline(baseline_fname, baseline);

        // Combine the spiral down to 1 domain and compare to baseline
        for(conduit::index_t flavor = 0; flavor < (conduit::index_t)spirals.size(); flavor++)
        {
            const std::string mesh_name("spiral_with_matset_" + std::to_string(flavor));
            conduit::Node &spiral = spirals[flavor];
            save_visit(mesh_name, spiral, true);
            conduit::Node opts, spiral_combined;
            opts["target"].set(1);
            conduit::blueprint::mesh::partition(spiral, opts, spiral_combined);
            const std::string combined_mesh_name = mesh_name + "_combined";
            save_visit(combined_mesh_name, spiral_combined, true);

            conduit::Node info;
            EXPECT_FALSE(diff_to_silo(baseline, spiral_combined, info))
                << "Flavor " << flavor << ":" << info.to_yaml();
        }
    }

    // we are not allowed to have zones without any materials defined on them, so there is no
    // reason to test that case.
}


using namespace conduit;

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, threshold_example)
{

    Node mesh;
    index_t base_grid_ele_i = 3;
    index_t base_grid_ele_j = 3;

    conduit::blueprint::mesh::examples::related_boundary(base_grid_ele_i,
                                                         base_grid_ele_j,
                                                         mesh);

    std::string output_base = "tout_bp_part_threshold_";

    // prefer hdf5, fall back to yaml
    std::string protocol = "yaml";

    if(check_if_hdf5_enabled())
    {
        protocol = "hdf5";
    }

    conduit::relay::io::blueprint::save_mesh(mesh,
                                             output_base + "input",
                                             protocol);

    // lets threshold the boundary mesh, remove any interior to the problem
    // elements

    // step 1: create a selection description of the zones we want to keep

    // loop over all domains
    Node opts;
    NodeConstIterator doms_itr = mesh.children();
    while(doms_itr.has_next())
    {
        const Node &dom = doms_itr.next();
        // fetch the field that we want to use to check
        // if the boundary ele are valid
        int64_accessor bndry_vals = dom["fields/bndry_val/values"].value();

        index_t domain_id = dom["state/domain_id"].to_value();

        std::vector<int64> ele_ids_to_keep;
        for(index_t i=0; i< bndry_vals.number_of_elements(); i++)
        {
            // this is our criteria to "keep" and element
            if(bndry_vals[i] == 1)
            {
                ele_ids_to_keep.push_back(i);
            }
        }

        // add selection description
        Node &d_sel = opts["selections"].append();
        d_sel["type"] = "explicit";
        d_sel["domain_id"] = domain_id;
        d_sel["elements"] = ele_ids_to_keep;
        d_sel["topology"] = "boundary";
    }

    opts["target"] = 3;
    // show our options
    opts.print();

    // use the partition function to select this subset
    Node res_thresh;
    conduit::blueprint::mesh::partition(mesh, opts, res_thresh);
    conduit::relay::io::blueprint::save_mesh(res_thresh,
                                             output_base + "result",
                                             protocol);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, generate_boundary_partition)
{
    int dims[] = {3,3,3};
    int nparts = 3;
    // Build the whole mesh
    conduit::Node whole, bopts;
    bopts["meshname"] = "main";
    bopts["datatype"] = "int32";
    conduit::blueprint::mesh::examples::tiled(dims[0], dims[1], dims[2], whole, bopts);

    // Make a Hilbert ordering of the zones and then make a new "parts"
    // field that indicates the parts we'll make from it.
    auto indices = conduit::blueprint::mesh::utils::topology::hilbert_ordering(whole.fetch_existing("topologies/main"));
    conduit::Node &f = whole["fields/parts"];
    f["topology"] = "main";
    f["association"] = "element";
    f["values"].set(conduit::DataType::int32(indices.size()));
    int *iptr = f["values"].as_int_ptr();
    int nzones_per_part = static_cast<int>(indices.size()) / nparts;
    for(size_t zi = 0; zi < indices.size(); zi++)
    {
       int target_part = indices[zi] / nzones_per_part;
       iptr[zi] = std::min(target_part, nparts - 1);
    }

    // Make a field on the boundary mesh that will let us partition it too.
    conduit::blueprint::mesh::generate_boundary_partition_field(
        whole["topologies/main"],
        whole["fields/parts"],
        whole["topologies/boundary"],
        whole["fields/bparts"]);

    const auto bparts = whole["fields/bparts/values"].as_int32_array();
#if 0
    // Generate the baseline vector. It looks good in VisIt.
    conduit::relay::io::blueprint::save_mesh(whole, "whole", "hdf5");
    std::cout << "std::vector<int> bparts_baseline{";
    for(conduit::index_t i = 0; i < bparts.number_of_elements(); i++)
    {
        if(i > 0) std::cout << ", ";
        std::cout << bparts[i];
    }
    std::cout << "};" << endl;
#endif
    // Generated by the above code.
    std::vector<int> bparts_baseline{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    EXPECT_EQ(bparts_baseline.size(), bparts.number_of_elements());
    for(conduit::index_t i = 0; i < bparts.number_of_elements(); i++)
    {
        EXPECT_EQ(bparts_baseline[i], bparts[i]);
    }
}

//-----------------------------------------------------------------------------
std::vector<double> blend(double x0, double x1, int n)
{
    std::vector<double> values(n);
    for(int i = 0; i < n; i++)
    {
        double t = static_cast<double>(i) / static_cast<double>(n - 1);
        values[i] = (1. - t) * x0 + t * x1;
    }
    return values;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, map_back_set_external)
{
#define F_XYC(X,Y,C) (static_cast<double>((C) + 1) * sqrt((X)*(X) + (Y)*(Y)))

    auto compute_nodal_field = [](conduit::Node &mesh, conduit::Node &f)
    {
        std::string type = mesh["coordsets/coords/type"].as_string();
        const auto x = mesh["coordsets/coords/values/x"].as_double_accessor();
        const auto y = mesh["coordsets/coords/values/y"].as_double_accessor();
        conduit::Node &values = f["values"];
        int nc = std::max(1, static_cast<int>(values.number_of_children()));
        bool single = values.number_of_children() == 0;

        if(type == "rectilinear")
        {
            for(int c = 0; c < nc; c++)
            {
                auto comp = single ? values.as_double_array() : values[c].as_double_array();
                int idx =0 ;
                for(int j = 0; j < y.number_of_elements(); j++)
                for(int i = 0; i < x.number_of_elements(); i++, idx++)
                    comp[idx] = F_XYC(x[i], y[j], c);
            }
        }
        else if(type == "unstructured")
        {
            for(int c = 0; c < nc; c++)
            {
                auto comp = single ? values.as_double_array() : values[c].as_double_array();
                for(int i = 0; i < x.number_of_elements(); i++)
                    comp[i] = F_XYC(x[i], y[i], c);
            }
        }
    };

    auto compute_zonal_field = [](conduit::Node &mesh, conduit::Node &f)
    {
        namespace topoutils = conduit::blueprint::mesh::utils::topology;

        // Turn to explicit coordinates just in case.
        conduit::Node expcoords;
        conduit::blueprint::mesh::coordset::to_explicit(mesh["coordsets/coords"], expcoords);
        const auto x = expcoords["values/x"].as_double_accessor();
        const auto y = expcoords["values/y"].as_double_accessor();
        conduit::Node &values = f["values"];
        topoutils::iterate_elements(mesh["topologies/main"],
            [&](const topoutils::entity &e)
        {
            // Make a zone center.
            double w = 1. / static_cast<double>(e.element_ids.size());
            double zc[] = {0., 0., 0.};
            for(const auto id : e.element_ids)
            {
                zc[0] = w * x[id];
                zc[1] = w * y[id];
            }

            int nc = std::max(1, static_cast<int>(values.number_of_children()));
            bool single = values.number_of_children() == 0;

            for(int c = 0; c < nc; c++)
            {
                auto comp = single ? values.as_double_array() : values[c].as_double_array();
                comp[e.entity_id] = F_XYC(zc[0], zc[1], c);
            }
        });
    };

    auto wrap = [](conduit::Node &root,
                   std::vector<double> &nodal,
                   std::vector<double> &zonal,
                   std::vector<double> &nodalvec,
                   std::vector<double> &zonalvec,
                   int nnodes, int nzones)
    {
        root["fields/nodal/topology"] = "main";
        root["fields/nodal/association"] = "vertex";
        root["fields/nodal/values"].set_external(&nodal[0], nnodes);

        root["fields/zonal/topology"] = "main";
        root["fields/zonal/association"] = "element";
        root["fields/zonal/values"].set_external(&zonal[0], nzones);

        root["fields/nodalvec/topology"] = "main";
        root["fields/nodalvec/association"] = "vertex";
        root["fields/nodalvec/values/x"].set_external(&nodalvec[0], nnodes, 0, 2*sizeof(double));
        root["fields/nodalvec/values/y"].set_external(&nodalvec[0], nnodes, sizeof(double), 2*sizeof(double));

        root["fields/zonalvec/topology"] = "main";
        root["fields/zonalvec/association"] = "element";
        root["fields/zonalvec/values/cx"].set_external(&zonalvec[0], nzones, 0, 2*sizeof(double));
        root["fields/zonalvec/values/cy"].set_external(&zonalvec[0], nzones, sizeof(double), 2*sizeof(double));
    };

    const double extents[] = {-2., 2., -2., 2.};
    constexpr int dims[] = {15,10};
    constexpr int nnodes = dims[0] * dims[1];
    constexpr int nzones = (dims[0] - 1) * (dims[1] - 1);

    conduit::Node mesh;
    mesh["coordsets/coords/type"] = "rectilinear";
    mesh["coordsets/coords/values/x"].set(blend(extents[0], extents[1], dims[0]));
    mesh["coordsets/coords/values/y"].set(blend(extents[2], extents[3], dims[1]));

    mesh["topologies/main/type"] = "rectilinear";
    mesh["topologies/main/coordset"] = "coords";

    mesh["state/cycle"] = 123;
    mesh["state/domain_id"] = 0;

    // Make a global vertex field or we get a failure during map_back.
    mesh["fields/global_vertex_ids/topology"] = "main";
    mesh["fields/global_vertex_ids/association"] = "vertex";
    mesh["fields/global_vertex_ids/values"].set(conduit::DataType::int32(nnodes));
    int *gids = mesh["fields/global_vertex_ids/values"].as_int_ptr();
    std::iota(gids, gids + nnodes, 0);

    // Make external fields.
    std::vector<double> nodal(nnodes, 0.), zonal(nzones, 0.),
                        nodalvec(nnodes * 2., 0.), zonalvec(nzones * 2, 0.);
    wrap(mesh, nodal, zonal, nodalvec, zonalvec, nnodes, nzones);
    //mesh.print();

    // Make results vectors
    std::vector<double> nodal_result(nnodes, 0.), zonal_result(nzones, 0.),
                        nodalvec_result(nnodes * 2, 0.), zonalvec_result(nzones * 2, 0.);
    conduit::Node results;
    results["coordsets"].set_external_node(mesh["coordsets"]);
    results["topologies"].set_external_node(mesh["topologies"]);
    wrap(results, nodal_result, zonal_result, nodalvec_result, zonalvec_result, nnodes, nzones);

    // Compute values into the results.
    compute_nodal_field(results, results["fields/nodal"]);
    compute_zonal_field(results, results["fields/zonal"]);
    compute_nodal_field(results, results["fields/nodalvec"]);
    compute_zonal_field(results, results["fields/zonalvec"]);
    //results.print();

    // Partition the mesh into 2 domains.
    conduit::Node part, options;
    options["target"] = 2;
    options["mapping"] = 1;
    options["original_element_ids"] = "eids";
    options["original_vertex_ids"] = "vids";
    //std::cout << "Calling partition" << std::endl;
    //options.print();
    conduit::blueprint::mesh::partition(mesh, options, part);

    auto domains = conduit::blueprint::mesh::domains(part);
    EXPECT_EQ(domains.size(), 2);
    for(auto &dom : domains)
    {
        EXPECT_TRUE(dom->has_path("fields/eids/values"));
        EXPECT_TRUE(dom->has_path("fields/vids/values"));
    }

    // Compute the fields on the part mesh.
    for(auto &dom : domains)
    {
        compute_nodal_field(*dom, dom->fetch_existing("fields/nodal"));
        compute_zonal_field(*dom, dom->fetch_existing("fields/zonal"));
        compute_nodal_field(*dom, dom->fetch_existing("fields/nodalvec"));
        compute_zonal_field(*dom, dom->fetch_existing("fields/zonalvec"));
        //dom->print();
    }

    // Map the fields back to the original mesh.
    conduit::Node mbopts;
    mbopts["fields"].append().set("nodal");
    mbopts["fields"].append().set("zonal");
    mbopts["fields"].append().set("nodalvec");
    mbopts["fields"].append().set("zonalvec");
    mbopts["original_element_ids"] = "eids";
    mbopts["original_vertex_ids"] = "vids";
    //std::cout << "Calling partition_map_back" << std::endl;
    //mbopts.print();
    conduit::blueprint::mesh::partition_map_back(part, mbopts, mesh);

    // Printing the nodal field, it should not contain non-zero values.
    //std::cout << "After map_back" << std::endl;
    //mesh["fields/nodal"].print();

    // Make sure mapping back the fields did not change addresses of the fields
    // in the original mesh. Y components should be at index 1.
    EXPECT_EQ(&nodal[0], mesh["fields/nodal/values"].as_double_ptr());
    EXPECT_EQ(&zonal[0], mesh["fields/zonal/values"].as_double_ptr());
    EXPECT_EQ(&nodalvec[0], mesh["fields/nodalvec/values/x"].as_double_ptr());
    EXPECT_EQ(&nodalvec[1], mesh["fields/nodalvec/values/y"].as_double_ptr());
    EXPECT_EQ(&zonalvec[0], mesh["fields/zonalvec/values/cx"].as_double_ptr());
    EXPECT_EQ(&zonalvec[1], mesh["fields/zonalvec/values/cy"].as_double_ptr());

    // Check lengths
    EXPECT_EQ(mesh["fields/nodal/values"].dtype().number_of_elements(), nnodes);
    EXPECT_EQ(mesh["fields/zonal/values"].dtype().number_of_elements(), nzones);
    EXPECT_EQ(mesh["fields/nodalvec/values/x"].dtype().number_of_elements(), nnodes);
    EXPECT_EQ(mesh["fields/nodalvec/values/y"].dtype().number_of_elements(), nnodes);
    EXPECT_EQ(mesh["fields/zonalvec/values/cx"].dtype().number_of_elements(), nzones);
    EXPECT_EQ(mesh["fields/zonalvec/values/cy"].dtype().number_of_elements(), nzones);

    // Check that the vectors are interleaved.
    EXPECT_EQ(mesh["fields/nodalvec/values/x"].dtype().offset(), 0);
    EXPECT_EQ(mesh["fields/nodalvec/values/x"].dtype().stride(), 2 * sizeof(double));
    EXPECT_EQ(mesh["fields/nodalvec/values/y"].dtype().offset(), sizeof(double));
    EXPECT_EQ(mesh["fields/nodalvec/values/y"].dtype().stride(), 2 * sizeof(double));
    EXPECT_EQ(mesh["fields/zonalvec/values/cx"].dtype().offset(), 0);
    EXPECT_EQ(mesh["fields/zonalvec/values/cx"].dtype().stride(), 2 * sizeof(double));
    EXPECT_EQ(mesh["fields/zonalvec/values/cy"].dtype().offset(), sizeof(double));
    EXPECT_EQ(mesh["fields/zonalvec/values/cy"].dtype().stride(), 2 * sizeof(double));

    // Make sure that the field values are the same as the results.
    const std::vector<std::string> fieldNames{"nodal", "zonal", "nodalve", "zonalvec"};
    for(const auto &name : fieldNames)
    {
        conduit::Node info;
        bool different = results["fields"][name].diff(mesh["fields"][name], info);
        if(different)
            info.print();
        EXPECT_FALSE(different);
    }
#undef F_XYC
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, partition_single_group)
{
    // This test is designed to make sure that we can send an mcarray with a single
    // component through partitioning when the partitioner causes domain recombining.
    // There was a problem where this was not quite working due to the mcarray being
    // treated as a scalar (had the wrong schema).
    const double extents[3][4] = {{0.,1., 0., 1.},{1.,2., 0., 1.},{0.,2., 1., 2.}};
    constexpr int dims[3][2] = {{5,5}, {5,5}, {9,5}};

    conduit::Node mesh;
    for(int d = 0; d < 3; d++)
    {
        std::string name = conduit_fmt::format("domain_{:05}",d);
        conduit::Node &dom = mesh[name];
        const int nnodes = dims[d][0] * dims[d][1];

        dom["coordsets/coords/type"] = "rectilinear";
        dom["coordsets/coords/values/x"].set(blend(extents[d][0], extents[d][1], dims[d][0]));
        dom["coordsets/coords/values/y"].set(blend(extents[d][2], extents[d][3], dims[d][1]));

        dom["topologies/main/type"] = "rectilinear";
        dom["topologies/main/coordset"] = "coords";

        dom["state/cycle"] = 123;
        dom["state/domain_id"] = d;

        // Make a mcarray with 1 component. We'll send this through partition to the part mesh
        // where it won't exist but will need to be created, testing the partitioner's vector vs
        // scalar logic for 1 component.
        dom["fields/single_group/topology"] = "main";
        dom["fields/single_group/association"] = "vertex";
        dom["fields/single_group/values/group0"].set(conduit::DataType::int32(nnodes));
        int *sg = dom["fields/single_group/values/group0"].as_int_ptr();
        std::iota(sg, sg + nnodes, 0);
    }

    // Repartition the mesh from 3 domains into 2 domains.
    conduit::Node part, options;
    options["target"] = 2;
    options["mapping"] = 1;
    conduit::blueprint::mesh::partition(mesh, options, part);

    // Check the part mesh.
    EXPECT_EQ(part.number_of_children(), 2);
    for(conduit::index_t d = 0; d < part.number_of_children(); d++)
    {
        conduit::Node &dom = part[d];
        EXPECT_TRUE(dom.has_path("fields/single_group/values/group0"));
        EXPECT_EQ(dom.fetch_existing("fields/single_group/values").number_of_children(), 1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, mixed2d)
{
    const std::string base("mixed2d");

    // Make a "mixed" tile.
    conduit::Node n_tile;
    generate::create_mixed_tile(n_tile);

    // Make sure the dimensions are 2.
    EXPECT_EQ(conduit::blueprint::mesh::topology::dims(n_tile["topologies/tile"]), 2);

    // Verify the tile.
    conduit::Node info;
    const bool v = conduit::blueprint::mesh::verify(n_tile, info);
    EXPECT_TRUE(v);
    if(!v)
    {
        info.print();
    }

    //conduit::relay::io::save(n_tile, "tile.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_tile, "tile", "hdf5");

    // Make 1 tiled domain
    conduit::Node n_options;
    //n_options["numDomains"] = 4;
    n_options["tile"].set_external(n_tile);
    n_options["meshname"] = "mesh";
    conduit::Node n_mesh;
    conduit::blueprint::mesh::examples::tiled(10,10,0, n_mesh, n_options);

    //conduit::relay::io::save(n_mesh, "tilemesh.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_mesh, "tilemesh", "hdf5");

    // Partition the 1 domain into 4 parts.
    std::cout << "Split 1 into 4" << std::endl;
    conduit::Node n_part, n_part_opts;
    n_part_opts["target"] = 4;
    conduit::blueprint::mesh::partition(n_mesh, n_part_opts, n_part);

    //conduit::relay::io::save(n_part, "part.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_part, "part", "hdf5");
    std::string b00 = baseline_file(base + "_00");
#ifdef GENERATE_BASELINES
    make_baseline(b00, n_part);
#else
    EXPECT_EQ(compare_baseline(b00, n_part), true);
#endif

    // Partition the 4 domains into 1 part.
    std::cout << "Combine 4 into 1" << std::endl;
    conduit::Node n_unpart;
    n_part_opts["target"] = 1;
    conduit::blueprint::mesh::partition(n_part, n_part_opts, n_unpart);

    //conduit::relay::io::save(n_unpart, "unpart.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_unpart, "unpart", "hdf5");
    std::string b01 = baseline_file(base + "_01");
#ifdef GENERATE_BASELINES
    make_baseline(b01, n_unpart);
#else
    EXPECT_EQ(compare_baseline(b01, n_unpart), true);
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_partition, mixed3d)
{
    const std::string base("mixed3d");

    // Make 1 tiled domain
    conduit::Node n_mesh;
    conduit::blueprint::mesh::examples::braid("mixed", 5,5,5, n_mesh);
    conduit::Node info;
    const bool v = conduit::blueprint::mesh::verify(n_mesh, info);
    EXPECT_TRUE(v);
    if(!v)
    {
        info.print();
    }

    //conduit::relay::io::save(n_mesh, "mixed3d.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_mesh, "mixed3d", "hdf5");

    // Partition the 1 domain into 2 parts.
    std::cout << "Split 1 into 2" << std::endl;
    conduit::Node n_part, n_part_opts;
    n_part_opts["target"] = 2;
    conduit::blueprint::mesh::partition(n_mesh, n_part_opts, n_part);

    // Verify the tile.
    for(int dom = 0; dom < 2; dom++)
    {
        conduit::Node info;
        const bool v = conduit::blueprint::mesh::verify(n_part[dom], info);
        EXPECT_TRUE(v);
        if(!v)
        {
            info.print();
        }
    }

    //conduit::relay::io::save(n_part, "part.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_part, "part", "hdf5");
    std::string b00 = baseline_file(base + "_00");
#ifdef GENERATE_BASELINES
    make_baseline(b00, n_part);
#else
    EXPECT_EQ(compare_baseline(b00, n_part), true);
#endif

    // Partition the 2 domains into 1 part.
    std::cout << "Combine 2 into 1" << std::endl;
    conduit::Node n_unpart;
    n_part_opts["target"] = 1;
    conduit::blueprint::mesh::partition(n_part, n_part_opts, n_unpart);

    // Make sure the mesh is good.
    info.reset();
    const bool v2 = conduit::blueprint::mesh::verify(n_unpart, info);
    EXPECT_TRUE(v2);
    if(!v2)
    {
        info.print();
    }

    //conduit::relay::io::save(n_unpart, "unpart.yaml", "yaml");
    //conduit::relay::io::blueprint::save_mesh(n_unpart, "unpart", "hdf5");
    std::string b01 = baseline_file(base + "_01");
#ifdef GENERATE_BASELINES
    make_baseline(b01, n_unpart);
#else
    EXPECT_EQ(compare_baseline(b01, n_unpart), true);
#endif
}
