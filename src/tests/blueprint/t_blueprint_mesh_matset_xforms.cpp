// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_matset_xforms.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_log.hpp"

#include <algorithm>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;

//-----------------------------------------------------------------------------
// Simply adds an "element_ids" field [0 1 2 ... N-1]
static void
convert_to_material_based(const Node &topo, Node &mset)
{
    const int nelem = static_cast<int>(blueprint::mesh::topology::length(topo));
    mset["element_ids"].set_dtype(DataType::c_int(nelem));
    DataArray<int> eids = mset["element_ids"].value();
    for(int i = 0; i < nelem; i++)
    {
        eids[i] = i;
    }
}

/// Test Cases ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_create_reverse_matmap)
{
    Node material_map;
    material_map["mat1"] = 5;
    material_map["mat2"] = 213423;
    material_map["mat3"] = 6;
    material_map["mat4"] = 0;

    const std::map<int, std::string> reverse_matmap = 
        blueprint::mesh::matset::create_reverse_material_map(material_map);
    EXPECT_EQ("mat4", reverse_matmap.at(0));
    EXPECT_EQ("mat1", reverse_matmap.at(5));
    EXPECT_EQ("mat3", reverse_matmap.at(6));
    EXPECT_EQ("mat2", reverse_matmap.at(213423));
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_count_zones_from_matset)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    CONDUIT_INFO("venn full count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("full", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(16, blueprint::mesh::matset::count_zones_from_matset(mset));
    }

    CONDUIT_INFO("venn sparse_by_material count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(16, blueprint::mesh::matset::count_zones_from_matset(mset));
    }

    CONDUIT_INFO("venn sparse_by_element count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(16, blueprint::mesh::matset::count_zones_from_matset(mset));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_is_material_in_zone)
{
    const int nx = 2, ny = 2;
    const double radius = 0.25;

    CONDUIT_INFO("venn full check mat in zone");
    {
        Node mesh;
        blueprint::mesh::examples::venn("full", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_FALSE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 0));
        EXPECT_TRUE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 3));
    }

    CONDUIT_INFO("venn sparse_by_material check mat in zone");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_FALSE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 0));
        EXPECT_TRUE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 3));
    }

    CONDUIT_INFO("venn sparse_by_element check mat in zone");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_FALSE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 0));
        EXPECT_TRUE(blueprint::mesh::matset::is_material_in_zone(mset, "circle_c", 3));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_basic)
{
    Node mesh;
    {
        blueprint::mesh::examples::basic("quads", 2, 2, 0, mesh);

        float64 mset_a_vfs[] = {1.0, 0.5, 0.5, 0.0};
        float64 mset_b_vfs[] = {0.0, 0.5, 0.5, 1.0};

        Node &mset = mesh["matsets/matset"];
        mset["topology"].set(mesh["topologies"].child_names().front());
        mset["volume_fractions/a"].set(&mset_a_vfs[0], 4);
        mset["volume_fractions/b"].set(&mset_b_vfs[0], 4);
    }
    Node &mset = mesh["matsets/matset"];

    Node silo, info;
    blueprint::mesh::matset::to_silo(mset, silo);
    std::cout << silo.to_yaml() << std::endl;

    { // Check General Contents //
        EXPECT_TRUE(silo.has_child("topology"));
        EXPECT_TRUE(silo.has_child("matlist"));
        EXPECT_TRUE(silo.has_child("mix_next"));
        EXPECT_TRUE(silo.has_child("mix_mat"));
        EXPECT_TRUE(silo.has_child("mix_vf"));
    }

    { // Check 'topology' Field //
        const std::string expected_topology = mset["topology"].as_string();
        const std::string actual_topology = silo["topology"].as_string();
        EXPECT_EQ(actual_topology, expected_topology);
    }

    // { // Check 'matlist' Field //
    //     // TODO(JRC): Need to make sure these are the same type.
    //     int64 expected_matlist_vec[] = {1, -1, -3, 2};
    //     Node expected_matlist(DataType::int64(4),
    //         &expected_matlist_vec[0], true);
    //     const Node &actual_matlist = silo["matlist"];

    //     EXPECT_FALSE(actual_matlist.diff(expected_matlist, info));
    // }
}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_venn_to_silo)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    Node mset_silo_baseline;
    
    // all of these cases should create the same silo output
    // (aside from "buffer_style" and "dominance" leaves)
    // we diff the 2 and 3 cases with the 1 to test this

    CONDUIT_INFO("venn full to silo");
    {
        Node mesh;
        blueprint::mesh::examples::venn("full", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        std::cout << mset.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::matset::to_silo(mset, mset_silo);
        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline.set(mset_silo);
    }

    CONDUIT_INFO("venn sparse_by_material to silo");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        std::cout << mset.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::matset::to_silo(mset, mset_silo);
        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "multi";
        mset_silo_baseline["dominance"] = "material";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

    CONDUIT_INFO("venn sparse_by_element to silo");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        std::cout << mset.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::matset::to_silo(mset, mset_silo);
        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "uni";
        mset_silo_baseline["dominance"] = "element";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

    CONDUIT_INFO("venn sparse_by_element (converted to material based) to silo")
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        Node &mset = mesh["matsets/matset"];
        convert_to_material_based(mesh["topologies/topo"], mset);

        std::cout << mset.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::matset::to_silo(mset, mset_silo);
        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "uni";
        mset_silo_baseline["dominance"] = "material";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_venn_to_silo_matset_values)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    Node mset_silo_baseline;
    
    // all of these cases should create the same silo output
    // (aside from "buffer_style" and "dominance" leaves)
    // we diff the 2 and 3 cases with the 1 to test this

    CONDUIT_INFO("venn full to silo");
    {
        Node mesh;
        blueprint::mesh::examples::venn("full", nx, ny, radius, mesh);
        const Node &field = mesh["fields/mat_check"];
        const Node &mset = mesh["matsets/matset"];

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::field::to_silo(field,
                                         mset,
                                         mset_silo);

        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline.set(mset_silo);
    }

    CONDUIT_INFO("venn sparse_by_material to silo");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh);
        const Node &field = mesh["fields/mat_check"];
        const Node &mset = mesh["matsets/matset"];


        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::field::to_silo(field,
                                         mset,
                                         mset_silo);

        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "multi";
        mset_silo_baseline["dominance"] = "material";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

    CONDUIT_INFO("venn sparse_by_element to silo");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &field = mesh["fields/mat_check"];
        const Node &mset = mesh["matsets/matset"];


        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::field::to_silo(field,
                                        mset,
                                        mset_silo);

        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "uni";
        mset_silo_baseline["dominance"] = "element";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

    CONDUIT_INFO("venn sparse_by_element (converted to material based) to silo")
    {
        Node mesh, info;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &field = mesh["fields/mat_check"];
        Node &mset = mesh["matsets/matset"];
        convert_to_material_based(mesh["topologies/topo"], mset);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node mset_silo;
        blueprint::mesh::field::to_silo(field,
                                        mset,
                                        mset_silo);

        std::cout << mset_silo.to_yaml() << std::endl;

        mset_silo_baseline["buffer_style"] = "uni";
        mset_silo_baseline["dominance"] = "material";

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info));
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_matset_style_transforms)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm, info;
    blueprint::mesh::examples::venn("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full -> full");
    {
        // diff full -> full with full

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        Node full_mset_baseline, full_field_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_full(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_full(mset, 
                                                     field, 
                                                     converted_matset_name, 
                                                     converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn full -> sparse_by_element");
    {
        // diff full -> sbe with sbe

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        Node sbe_mset_baseline, sbe_field_baseline;
        sbe_mset_baseline.set(mesh_sbe["matsets/matset"]);
        sbe_field_baseline.set(mesh_sbe["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_uni_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_uni_buffer_by_element(mset, 
                                                     field, 
                                                     converted_matset_name, 
                                                     converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        sbe_field_baseline["matset"].reset();
        sbe_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbe_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbe_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn full -> sparse_by_material");
    {
        // diff full -> sbm with sbm

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        Node sbm_mset_baseline, sbm_field_baseline;
        sbm_mset_baseline.set(mesh_sbm["matsets/matset"]);
        sbm_field_baseline.set(mesh_sbm["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_material(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_material(mset, 
                                                            field, 
                                                            converted_matset_name, 
                                                            converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        sbm_field_baseline["matset"].reset();
        sbm_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbm_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbm_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_element -> full");
    {
        // diff sbe -> full with full

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        Node full_mset_baseline, full_field_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_full(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_full(mset, 
                                                     field, 
                                                     converted_matset_name, 
                                                     converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_element -> sparse_by_material");
    {
        // diff sbe -> sbm with sbm

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        Node sbm_mset_baseline, sbm_field_baseline;
        sbm_mset_baseline.set(mesh_sbm["matsets/matset"]);
        sbm_field_baseline.set(mesh_sbm["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_material(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_material(mset, 
                                                            field, 
                                                            converted_matset_name, 
                                                            converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        sbm_field_baseline["matset"].reset();
        sbm_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbm_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbm_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_material -> full");
    {
        // diff sbm -> full with full

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        Node full_mset_baseline, full_field_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_full(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_full(mset, 
                                                     field, 
                                                     converted_matset_name, 
                                                     converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_material -> sparse_by_element");
    {
        // diff sbm -> sbe with sbe

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        Node sbe_mset_baseline, sbe_field_baseline;
        sbe_mset_baseline.set(mesh_sbe["matsets/matset"]);
        sbe_field_baseline.set(mesh_sbe["fields/importance"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;

        Node converted_mset, converted_field;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_uni_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_uni_buffer_by_element(mset, 
                                                     field, 
                                                     converted_matset_name, 
                                                     converted_field);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;

        sbe_field_baseline["matset"].reset();
        sbe_field_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbe_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbe_field_baseline, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_misc)
{
    Node mesh;
    blueprint::mesh::examples::misc("specsets", 4, 4, 1, mesh);
    const Node &matset = mesh["matsets/mesh"];
    const Node &specset = mesh["specsets/mesh"];

    Node silo_rep1, silo_rep2, silo_rep_matset, info;

    const std::string yaml_text = 
        "specnames: \n"
        "  - \"spec1\"\n"
        "  - \"spec2\"\n"
        "  - \"spec1\"\n"
        "  - \"spec2\"\n"
        "nmat: 2\n"
        "nmatspec: [2, 2]\n"
        "speclist: [3, -1, 9, 15, -3, 21, 27, -5, 33]\n"
        "nspecies_mf: 36\n"
        "species_mf: [0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 1.0, 0.0]\n"
        "mix_spec: [5, 7, 17, 19, 29, 31]\n"
        "mixlen: 6";
    Node baseline;
    baseline.parse(yaml_text, "yaml");


    // first test transforming specset to silo rep with a regular matset
    blueprint::mesh::specset::to_silo(specset, matset, silo_rep1);
    std::cout << silo_rep1.to_yaml() << std::endl;
    EXPECT_FALSE(silo_rep1.diff(baseline, info, CONDUIT_EPSILON, true));

    // next we test transforming specset to silo rep with a silo rep matset
    blueprint::mesh::matset::to_silo(matset, silo_rep_matset);
    blueprint::mesh::specset::to_silo(specset, silo_rep_matset, silo_rep2);
    std::cout << silo_rep2.to_yaml() << std::endl;
    EXPECT_FALSE(silo_rep2.diff(baseline, info, CONDUIT_EPSILON, true));
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_specset_edge_cases)
{
    CONDUIT_INFO("Case 1: Missing materials and material order is reversed in the specset.");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

        // remove some of the materials from the specset
        mesh["specsets"]["specset"]["matset_values"].remove_child("background");
        mesh["specsets"]["specset"]["matset_values"].remove_child("circle_b");
        // create a new specset that has the materials in reverse order
        mesh["specsets"]["specset2"]["matset"] = "matset";
        mesh["specsets"]["specset2"]["matset_values"]["circle_c"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_c"]);
        mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
        // remove the original specset and replace it with the new one
        mesh["specsets"].remove_child("specset");
        mesh["specsets"].rename_child("specset2", "specset");

        const Node &matset = mesh["matsets/matset"];
        const Node &specset = mesh["specsets/specset"];

        Node silo_rep;

        blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

        std::cout << specset.to_yaml() << std::endl;
        std::cout << silo_rep.to_yaml() << std::endl;

        const std::string yaml_text = 
            "nmatspec: [0, 2, 0, 3]\n"
            "specnames: \n"
              "- \"a_spec1\"\n"
              "- \"a_spec2\"\n"
              "- \"c_spec1\"\n"
              "- \"c_spec2\"\n"
              "- \"c_spec3\"\n"
            "speclist: [1, 6, 11, -1]\n"
            "nmat: 4\n"
            "nspecies_mf: 20\n"
            "species_mf: [0.0, 1.0, 1.0, 0.0, 0.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 0.0, 1.0, 0.75, 0.1875, 0.0625, 0.5, 0.5, 0.5, 0.375, 0.125]\n"
            "mix_spec: [16, 18, 18]\n"
            "mixlen: 3\n";
        Node baseline;
        baseline.parse(yaml_text, "yaml");

        EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("Case 2: Material order is scrambled in the specset.");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

        // create a new specset that has the materials in reverse order
        mesh["specsets"]["specset2"]["matset"] = "matset";
        mesh["specsets"]["specset2"]["matset_values"]["circle_c"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_c"]);
        mesh["specsets"]["specset2"]["matset_values"]["background"].set(
            mesh["specsets"]["specset"]["matset_values"]["background"]);
        mesh["specsets"]["specset2"]["matset_values"]["circle_b"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_b"]);
        mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
        // remove the original specset and replace it with the new one
        mesh["specsets"].remove_child("specset");
        mesh["specsets"].rename_child("specset2", "specset");

        const Node &matset = mesh["matsets/matset"];
        const Node &specset = mesh["specsets/specset"];

        Node silo_rep;

        blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

        std::cout << specset.to_yaml() << std::endl;
        std::cout << silo_rep.to_yaml() << std::endl;

        const std::string yaml_text = 
            "nmatspec: [1, 2, 2, 3]\n"
            "specnames: \n"
            "  - \"bg_spec1\"\n"
            "  - \"a_spec1\"\n"
            "  - \"a_spec2\"\n"
            "  - \"b_spec1\"\n"
            "  - \"b_spec2\"\n"
            "  - \"c_spec1\"\n"
            "  - \"c_spec2\"\n"
            "  - \"c_spec3\"\n"
            "speclist: [0, 0, 0, -1]\n"
            "nmat: 4\n"
            "nspecies_mf: 32\n"
            "species_mf: [1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.5, 0.5, 0.0, 1.0, 0.75, 0.1875, 0.0625, 1.0, 0.0, 1.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.375, 0.125]\n"
            "mix_spec: [26, 28, 30]\n"
            "mixlen: 3\n";
        Node baseline;
        baseline.parse(yaml_text, "yaml");

        EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("Case 3: Material order is scrambled in the matset.");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

        mesh["matsets"]["matset2"]["topology"] = "topo";
        mesh["matsets"]["matset2"]["volume_fractions"]["circle_c"].set(
            mesh["matsets"]["matset"]["volume_fractions"]["circle_c"]);
        mesh["matsets"]["matset2"]["volume_fractions"]["background"].set(
            mesh["matsets"]["matset"]["volume_fractions"]["background"]);
        mesh["matsets"]["matset2"]["volume_fractions"]["circle_b"].set(
            mesh["matsets"]["matset"]["volume_fractions"]["circle_b"]);
        mesh["matsets"]["matset2"]["volume_fractions"]["circle_a"].set(
            mesh["matsets"]["matset"]["volume_fractions"]["circle_a"]);

        // remove the original matset and replace it with the new one
        mesh["matsets"].remove_child("matset");
        mesh["matsets"].rename_child("matset2", "matset");

        const Node &matset = mesh["matsets/matset"];
        const Node &specset = mesh["specsets/specset"];

        Node silo_rep;

        blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

        std::cout << specset.to_yaml() << std::endl;
        std::cout << silo_rep.to_yaml() << std::endl;

        const std::string yaml_text = 
            "nmatspec: [3, 1, 2, 2]\n"
            "specnames: \n"
            "  - \"c_spec1\"\n"
            "  - \"c_spec2\"\n"
            "  - \"c_spec3\"\n"
            "  - \"bg_spec1\"\n"
            "  - \"b_spec1\"\n"
            "  - \"b_spec2\"\n"
            "  - \"a_spec1\"\n"
            "  - \"a_spec2\"\n"
            "speclist: [0, 0, 0, -1]\n"
            "nmat: 4\n"
            "nspecies_mf: 32\n"
            "species_mf: [1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.75, 0.1875, 0.0625, 1.0, 0.0, 1.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 1.0, 0.5, 0.5, 0.0, 1.0, 0.5, 0.375, 0.125, 1.0, 0.5, 0.5, 0.5, 0.5]\n"
            "mix_spec: [25, 29, 31]\n"
            "mixlen: 3\n";
        Node baseline;
        baseline.parse(yaml_text, "yaml");

        EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("Case 4: Missing 1st and last materials and material order is scrambled in the specset.");
    {
        Node mesh, info;
        blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

        // remove some of the materials from the specset
        mesh["specsets"]["specset"]["matset_values"].remove_child("background");
        mesh["specsets"]["specset"]["matset_values"].remove_child("circle_c");
        // create a new specset that has the materials in reverse order
        mesh["specsets"]["specset2"]["matset"] = "matset";
        mesh["specsets"]["specset2"]["matset_values"]["circle_b"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_b"]);
        mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
            mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
        // remove the original specset and replace it with the new one
        mesh["specsets"].remove_child("specset");
        mesh["specsets"].rename_child("specset2", "specset");

        const Node &matset = mesh["matsets/matset"];
        const Node &specset = mesh["specsets/specset"];

        Node silo_rep;

        blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

        std::cout << specset.to_yaml() << std::endl;
        std::cout << silo_rep.to_yaml() << std::endl;

        const std::string yaml_text = 
            "nmatspec: [0, 2, 2, 0]\n"
            "specnames: \n"
            "  - \"a_spec1\"\n"
            "  - \"a_spec2\"\n"
            "  - \"b_spec1\"\n"
            "  - \"b_spec2\"\n"
            "speclist: [1, 5, 9, -1]\n"
            "nmat: 4\n"
            "nspecies_mf: 16\n"
            "species_mf: [0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5]\n"
            "mix_spec: [13, 15, 17]\n"
            "mixlen: 3\n";
        Node baseline;
        baseline.parse(yaml_text, "yaml");

        EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
    }
}
