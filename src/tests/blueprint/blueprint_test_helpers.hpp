// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: blueprint_test_helpers.hpp
///
//-----------------------------------------------------------------------------

#ifndef BLUEPRINT_TEST_HELPERS_HPP
#define BLUEPRINT_TEST_HELPERS_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include <conduit.hpp>
#include <conduit_node.hpp>
#include <conduit_blueprint_mesh_examples.hpp>
#include <conduit_blueprint_table.hpp>
#include <conduit_log.hpp>

#include <gtest/gtest.h>

//-----------------------------------------------------------------------------
// -- begin table --
//-----------------------------------------------------------------------------
namespace table
{

//-----------------------------------------------------------------------------
inline void
compare_to_baseline_leaf(const conduit::Node &test,
                         const conduit::Node &baseline)
{
    if(test.dtype().is_empty() || test.dtype().is_list() || test.dtype().is_object()
        || baseline.dtype().is_empty() || baseline.dtype().is_list()
        || baseline.dtype().is_object())
    {
        CONDUIT_ERROR("compare_to_baseline_leaf only operates on leaf nodes.");
    }
    // Sometimes when we read from a file the data types don't match.
    // Convert test to the same type as baseline then compare.
    conduit::Node temp, info;
    if(test.dtype().id() != baseline.dtype().id())
    {
        test.to_data_type(baseline.dtype().id(), temp);
    }
    else
    {
        temp.set_external(test);
    }
    EXPECT_FALSE(baseline.diff(temp, info)) << "Column " << test.name() << info.to_json();
}

//-----------------------------------------------------------------------------
inline void
compare_to_baseline_values(const conduit::Node &test,
                           const conduit::Node &baseline)
{
    ASSERT_EQ(baseline.number_of_children(), test.number_of_children());
    for(conduit::index_t j = 0; j < baseline.number_of_children(); j++)
    {
        const conduit::Node &baseline_value = baseline[j];
        const conduit::Node &test_value = test[j];
        EXPECT_EQ(baseline_value.name(), test_value.name());
        if(baseline_value.dtype().is_list() || baseline_value.dtype().is_object())
        {
            // mcarray
            ASSERT_EQ(baseline_value.number_of_children(), test_value.number_of_children());
            EXPECT_EQ(baseline_value.dtype().is_list(), test_value.dtype().is_list());
            EXPECT_EQ(baseline_value.dtype().is_object(), test_value.dtype().is_object());
            for(conduit::index_t k = 0; k < baseline_value.number_of_children(); k++)
            {
                const conduit::Node &baseline_comp = baseline_value[k];
                const conduit::Node &test_comp = test_value[k];
                EXPECT_EQ(baseline_comp.name(), test_comp.name());
                compare_to_baseline_leaf(test_comp, baseline_comp);
            }
        }
        else
        {
            // data array
            compare_to_baseline_leaf(test_value, baseline_value);
        }
    }
}

//-----------------------------------------------------------------------------
inline void
compare_to_baseline(const conduit::Node &test,
    const conduit::Node &baseline, bool order_matters = true)
{
    conduit::Node info;
    ASSERT_TRUE(conduit::blueprint::table::verify(baseline, info)) << info.to_json();
    ASSERT_TRUE(conduit::blueprint::table::verify(test, info)) << info.to_json();
    if(baseline.has_child("values"))
    {
        const conduit::Node &baseline_values = baseline["values"];
        const conduit::Node &test_values = test["values"];
        compare_to_baseline_values(test_values, baseline_values);
    }
    else
    {
        ASSERT_EQ(baseline.number_of_children(), test.number_of_children());
        for(conduit::index_t i = 0; i < baseline.number_of_children(); i++)
        {
            if(order_matters)
            {
                ASSERT_EQ(baseline[i].name(), test[i].name())
                    << "baseline[i].name() = " << baseline[i].name()
                    << " test[i].name() = " << test[i].name();
                const conduit::Node &baseline_values = baseline[i]["values"];
                const conduit::Node &test_values = test[i]["values"];
                compare_to_baseline_values(test_values, baseline_values);
            }
            else
            {
                ASSERT_TRUE(test.has_child(baseline[i].name()))
                    << "With name = " << baseline[i].name()
                    << test.schema().to_json();
                const conduit::Node &b = baseline[i];
                const conduit::Node &baseline_values = b["values"];
                const conduit::Node &test_values = test[b.name()]["values"];
                compare_to_baseline_values(test_values, baseline_values);
            }
        }
    }
}

}
//-----------------------------------------------------------------------------
// -- end table --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin parition --
//-----------------------------------------------------------------------------
namespace partition
{

//-----------------------------------------------------------------------------
/**
Make a field that selects domains like this.
+----+----+
| 3  |  5 |
|  +-|-+  |
|  +4|4|  |
+--+-+-+--|
|  +1|1|  |
|  +-|-+  |
| 0  |  2 |
+----+----+

*/
//-----------------------------------------------------------------------------
inline void
add_field_selection_field(int cx, int cy, int cz,
    int iquad, int jquad, conduit::index_t main_dom, conduit::index_t fill_dom,
    conduit::Node &output)
{
    std::vector<conduit::int64> values(cx*cy*cz, main_dom);
    int sq = 2*jquad + iquad;
    int idx = 0;
    for(int k = 0; k < cz; k++)
    for(int j = 0; j < cy; j++)
    for(int i = 0; i < cx; i++)
    {
        int ci = (i < cx/2) ? 0 : 1;
        int cj = (j < cy/2) ? 0 : 1;
        int csq = 2*cj + ci;
        if(csq == sq)
            values[idx] = fill_dom;
        idx++;
    }
    output["fields/selection_field/association"] = "element";
    output["fields/selection_field/topology"] = "mesh";
    output["fields/selection_field/values"].set(values);
}

//-----------------------------------------------------------------------------
inline void
make_field_selection_example(conduit::Node &output, int mask)
{
    int nx = 11, ny = 11, nz = 3;
    int m = 1, dc = 0;
    for(int i = 0; i < 4; i++)
    {
        if(m & mask)
            dc++;
        m <<= 1;
    }

    if(mask & 1)
    {
        conduit::Node &dom0 = (dc > 1) ? output.append() : output;
        conduit::blueprint::mesh::examples::braid("uniform", nx, ny, nz, dom0);
        dom0["state/cycle"] = 1;
        dom0["state/domain_id"] = 0;
        dom0["coordsets/coords/origin/x"] = 0.;
        dom0["coordsets/coords/origin/y"] = 0.;
        dom0["coordsets/coords/origin/z"] = 0.;
        add_field_selection_field(nx-1, ny-1, nz-1, 1,1, 0, 11, dom0);
    }

    if(mask & 2)
    {
        conduit::Node &dom1 = (dc > 1) ? output.append() : output;
        conduit::blueprint::mesh::examples::braid("uniform", nx, ny, nz, dom1);
        auto dx = dom1["coordsets/coords/spacing/dx"].to_float();
        dom1["state/cycle"] = 1;
        dom1["state/domain_id"] = 1;
        dom1["coordsets/coords/origin/x"] = dx * static_cast<double>(nx-1);
        dom1["coordsets/coords/origin/y"] = 0.;
        dom1["coordsets/coords/origin/z"] = 0.;
        add_field_selection_field(nx-1, ny-1, nz-1, 0,1, 22, 11, dom1);
    }

    if(mask & 4)
    {
        conduit::Node &dom2 = (dc > 1) ? output.append() : output;
        conduit::blueprint::mesh::examples::braid("uniform", nx, ny, nz, dom2);
        auto dy = dom2["coordsets/coords/spacing/dy"].to_float();
        dom2["state/cycle"] = 1;
        dom2["state/domain_id"] = 2;
        dom2["coordsets/coords/origin/x"] = 0.;
        dom2["coordsets/coords/origin/y"] = dy * static_cast<double>(ny-1);
        dom2["coordsets/coords/origin/z"] = 0.;
        add_field_selection_field(nx-1, ny-1, nz-1, 1,0, 33, 44, dom2);
    }

    if(mask & 8)
    {
        conduit::Node &dom3 = (dc > 1) ? output.append() : output;
        conduit::blueprint::mesh::examples::braid("uniform", nx, ny, nz, dom3);
        auto dx = dom3["coordsets/coords/spacing/dx"].to_float();
        auto dy = dom3["coordsets/coords/spacing/dy"].to_float();
        dom3["state/cycle"] = 1;
        dom3["state/domain_id"] = 3;
        dom3["coordsets/coords/origin/x"] = dx * static_cast<double>(nx-1);
        dom3["coordsets/coords/origin/y"] = dy * static_cast<double>(ny-1);
        dom3["coordsets/coords/origin/z"] = 0.;
        add_field_selection_field(nx-1, ny-1, nz-1, 0,0, 55, 44, dom3);
    }
}

}
//-----------------------------------------------------------------------------
// -- end parition --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin generate --
//-----------------------------------------------------------------------------
namespace generate
{

//---------------------------------------------------------------------------
void create_2_domain_0d_mesh(conduit::Node &root, int rank, int size)
{
    // The adjset is properly set up.
    //
    // dom0 *       *       *
    // dom1         *       *       *
    const char *example = R"(
domain0:
  state:
    domain_id: 0
  coordsets:
    coords:
      type: explicit
      values:
        x: [0.,1.,2.]
        y: [0.,0.,0.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: point
        connectivity: [0,1,2]
        offsets: [0,1,2]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 1
          values: [1,2]
domain1:
  state:
    domain_id: 1
  coordsets:
    coords:
      type: explicit
      values:
        x: [1.,2.,3.]
        y: [0.,0.,0.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: point
        connectivity: [0,1,2]
        offsets: [0,1,2]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 0
          values: [0,1]
)";

    conduit::Node n;
    n.parse(example, "yaml");
    if(rank == 0 || size == 1)
        root["domain0"].set(n["domain0"]);
    if(rank == 1 || size == 1)
        root["domain1"].set(n["domain1"]);
}

//---------------------------------------------------------------------------
void create_2_domain_1d_mesh(conduit::Node &root, int rank, int size)
{
    // The adjset is properly set up.
    //
    // dom0 *-------*-------*
    // dom1         *-------*-------*
    const char *example = R"(
domain0:
  state:
    domain_id: 0
  coordsets:
    coords:
      type: explicit
      values:
        x: [0.,1.,2.]
        y: [0.,0.,0.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: line
        connectivity: [0,1,1,2]
        offsets: [0,2]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 1
          values: 1
domain1:
  state:
    domain_id: 1
  coordsets:
    coords:
      type: explicit
      values:
        x: [1.,2.,3.]
        y: [0.,0.,0.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: line
        connectivity: [0,1,1,2]
        offsets: [0,2]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 0
          values: 0
)";

    conduit::Node n;
    n.parse(example, "yaml");
    if(rank == 0 || size == 1)
        root["domain0"].set(n["domain0"]);
    if(rank == 1 || size == 1)
        root["domain1"].set(n["domain1"]);
}

//---------------------------------------------------------------------------
void create_2_domain_2d_mesh(conduit::Node &root, int rank, int size)
{
    // The adjset is properly set up. Note we make the vertex adjset a little
    // weird on purpose.
    const char *example = R"(
domain0:
  state:
    domain_id: 0
  coordsets:
    coords:
      type: explicit
      values:
        x: [0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.]
        y: [0.,0.,0.,0.,1.,1.,1.,1.,2.,2.,2.,2.,3.,3.,3.,3.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: quad
        connectivity: [0,1,5,4,1,2,6,5,2,3,7,6,4,5,9,8,8,9,13,12,9,10,14,13,10,11,15,14]
        offsets: [0,4,8,12,16,20,24]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 1
          values: [2, 6]
    pt_adjset:
      association: vertex
      topology: main
      groups:
        nonsense_name:
          neighbors: 1
          values: [15,11,7,3,14,10,6,2]
    fails_pointwise:
      association: vertex
      topology: main
      groups:
        group_0_1:
          neighbors: 1
          values: [14,3,10,7,6,11,2,15]
    notevenclose:
      association: vertex
      topology: main
      groups:
        group_0_1:
          neighbors: 1
          values: [12,8,4,0,1,5,9,13]
domain1:
  state:
    domain_id: 1
  coordsets:
    coords:
      type: explicit
      values:
        x: [2.,3.,4.,2.,3.,4.,2.,3.,4.,2.,3.,4.]
        y: [0.,0.,0.,1.,1.,1.,2.,2.,2.,3.,3.,3.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: quad
        connectivity: [0,1,4,3,1,2,5,4,3,4,7,6,4,5,8,7,6,7,10,9,7,8,11,10]
        offsets: [0,4,8,12,16,20]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 0
          values: [0,4]
    pt_adjset:
      association: vertex
      topology: main
      groups:
        nonsense_name:
          neighbors: 0
          values: [10,7,4,1,9,6,3,0]
    fails_pointwise:
      association: vertex
      topology: main
      groups:
        group_0_1:
          neighbors: 0
          values: [0,4,3,1,9,7,6,10]
    notevenclose:
      association: vertex
      topology: main
      groups:
        group_0_1:
          neighbors: 0
          values: [0,2,3,5,4,8,9,11]
)";

    conduit::Node n;
    n.parse(example, "yaml");
    if(rank == 0 || size == 1)
        root["domain0"].set(n["domain0"]);
    if(rank == 1 || size == 1)
        root["domain1"].set(n["domain1"]);
}

//---------------------------------------------------------------------------
void create_2_domain_3d_mesh(conduit::Node &root, int rank, int size)
{
    // The adjset is properly set up.
    //
    // dom0 *-------*-------*-------*
    // dom1         *-------*-------*-------*
    const char *example = R"(
domain0:
  state:
    domain_id: 0
  coordsets:
    coords:
      type: explicit
      values:
        x: [0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.,0.,1.,2.,3.]
        y: [0.,0.,0.,0.,1.,1.,1.,1.,0.,0.,0.,0.,1.,1.,1.,1.]
        z: [0.,0.,0.,0.,0.,0.,0.,0.,1.,1.,1.,1.,1.,1.,1.,1.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: hex
        connectivity: [0,1,5,4,8,9,13,12,1,2,6,5,9,10,14,13,2,3,7,6,10,11,15,14]
        offsets: [0,8,16]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 1
          values: [1,2]
domain1:
  state:
    domain_id: 1
  coordsets:
    coords:
      type: explicit
      values:
        x: [1.,2.,3.,4.,1.,2.,3.,4.,1.,2.,3.,4.,1.,2.,3.,4.]
        y: [0.,0.,0.,0.,1.,1.,1.,1.,0.,0.,0.,0.,1.,1.,1.,1.]
        z: [0.,0.,0.,0.,0.,0.,0.,0.,1.,1.,1.,1.,1.,1.,1.,1.]
  topologies:
    main:
      type: unstructured
      coordset: coords
      elements:
        shape: hex
        connectivity: [0,1,5,4,8,9,13,12,1,2,6,5,9,10,14,13,2,3,7,6,10,11,15,14]
        offsets: [0,8,16]
  adjsets:
    main_adjset:
      association: element
      topology: main
      groups:
        domain0_1:
          neighbors: 0
          values: [0,1]
)";

    conduit::Node n;
    n.parse(example, "yaml");
    if(rank == 0 || size == 1)
        root["domain0"].set(n["domain0"]);
    if(rank == 1 || size == 1)
        root["domain1"].set(n["domain1"]);
}

void create_mixed_tile(conduit::Node &n_tile)
{
    // Define a tile.
    const char *yaml = R"(
coordsets:
  coords:
    type: explicit
    values:
      x: [0., 1., 2., 3., 0., 2., 3., 0., 1., 2., 3.]
      y: [0., 0., 0., 0., 1., 1., 1., 2., 2., 2., 2.]
topologies:
  tile:
    type: unstructured
    coordset: coords
    elements:
      shape: mixed
      connectivity: [4,8,7, 0,1,5,9,8,4, 1,2,3,6,5, 5,6,10,9]
      sizes: [3,6,5,4]
      offsets: [0,3,9,14]
      shapes: [0,2,2,1]
      shape_map:
        tri: 0
        quad: 1
        polygonal: 2
fields:
  area:
    association: element
    topology: tile
    values: [0.5, 3., 1.5, 1.]
left: [0,4,7]
right: [3,6,10]
bottom: [0,1,2,3]
top: [7,8,9,10]
)";

    n_tile.parse(yaml);
}

}
//-----------------------------------------------------------------------------
// -- end generate --
//-----------------------------------------------------------------------------

#endif
