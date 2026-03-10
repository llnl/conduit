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
// the venn_specsets("full", ...) example creates irrelevant species mass
// fractions, as it adds non-trivial mass fractions for species for materials
// that are not present in some zones. If we want to use the "full"
// representation to diff with species sets converted from the other
// representations, we need to clear the irrelevant zone mass fractions, as the
// converters will default initialize the unused zone mass fractions to zero.
void
modify_full_specset_to_clear_irrelevant_zone_mass_fractions(const Node &full_matset,
                                                            Node &full_specset)
{
    // we need to modify the full specset baseline so that zones with
    // no materials in them have zeros for the species mass fractions.
    Node &matset_values = full_specset["matset_values"];
    const std::vector<std::string> &matnames = matset_values.child_names();
    for (const auto &matname : matnames)
    {
        Node &material = matset_values[matname];
        const std::vector<std::string> &specnames_for_mat = material.child_names();
        for (const auto &specname : specnames_for_mat)
        {
            float64_array mass_fractions = material[specname].value();
            const index_t num_zones = mass_fractions.number_of_elements();
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                // if the material is not in the zone, the species mass fraction
                // must be 0.
                if (! blueprint::mesh::matset::is_material_in_zone(full_matset,
                                                                   matname,
                                                                   zone_id,
                                                                   CONDUIT_EPSILON))
                {
                    mass_fractions[zone_id] = 0.0;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------

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
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_count_materials_from_matset)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    CONDUIT_INFO("venn full count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("full", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(4, blueprint::mesh::matset::count_materials_from_matset(mset));
    }

    CONDUIT_INFO("venn sparse_by_material count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(4, blueprint::mesh::matset::count_materials_from_matset(mset));
    }

    CONDUIT_INFO("venn sparse_by_element count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh);
        const Node &mset = mesh["matsets/matset"];

        EXPECT_EQ(4, blueprint::mesh::matset::count_materials_from_matset(mset));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_count_materials_from_specset)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    CONDUIT_INFO("venn full count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh);
        const Node &specset = mesh["specsets/specset"];

        EXPECT_EQ(4, blueprint::mesh::specset::count_materials_from_specset(specset));
    }

    CONDUIT_INFO("venn sparse_by_material count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh);
        const Node &specset = mesh["specsets/specset"];

        EXPECT_EQ(4, blueprint::mesh::specset::count_materials_from_specset(specset));
    }

    CONDUIT_INFO("venn sparse_by_element count zones");
    {
        Node mesh;
        blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh);
        const Node &specset = mesh["specsets/specset"];

        EXPECT_EQ(4, blueprint::mesh::specset::count_materials_from_specset(specset));
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

    { // Check 'matlist' Field //
        int64 expected_matlist_vec[] = {0, -1, -3, 1};
        Node expected_matlist(DataType::int64(4),
            &expected_matlist_vec[0], true);
        const Node &actual_matlist = silo["matlist"];

        EXPECT_FALSE(actual_matlist.diff(expected_matlist, info, CONDUIT_EPSILON, true));
    }
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

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info, CONDUIT_EPSILON, true));
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

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info, CONDUIT_EPSILON, true));
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

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info, CONDUIT_EPSILON, true));
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

        EXPECT_FALSE(mset_silo.diff(mset_silo_baseline,info,CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_matset_style_transforms)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm, info;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full -> full");
    {
        // diff full -> full with full

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];
        Node full_mset_baseline, full_field_baseline, full_sset_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);
        full_sset_baseline.set(mesh_full["specsets/specset"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_element(mset, 
                                                           field, 
                                                           converted_matset_name, 
                                                           converted_field);
        blueprint::mesh::specset::to_multi_buffer_by_element(mset, 
                                                             sset, 
                                                             converted_matset_name, 
                                                             converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";
        full_sset_baseline["matset"].reset();
        full_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(full_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn full -> sparse_by_element");
    {
        // diff full -> sbe with sbe

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];
        Node sbe_mset_baseline, sbe_field_baseline, sbe_sset_baseline;
        sbe_mset_baseline.set(mesh_sbe["matsets/matset"]);
        sbe_field_baseline.set(mesh_sbe["fields/importance"]);
        sbe_sset_baseline.set(mesh_sbe["specsets/specset"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_uni_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_uni_buffer_by_element(mset, 
                                                         field, 
                                                         converted_matset_name, 
                                                         converted_field);
        blueprint::mesh::specset::to_uni_buffer_by_element(mset, 
                                                           sset, 
                                                           converted_matset_name, 
                                                           converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        sbe_field_baseline["matset"].reset();
        sbe_field_baseline["matset"] = "matset2";
        sbe_sset_baseline["matset"].reset();
        sbe_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbe_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbe_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(sbe_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn full -> sparse_by_material");
    {
        // diff full -> sbm with sbm

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];
        Node sbm_mset_baseline, sbm_field_baseline, sbm_sset_baseline;
        sbm_mset_baseline.set(mesh_sbm["matsets/matset"]);
        sbm_field_baseline.set(mesh_sbm["fields/importance"]);
        sbm_sset_baseline.set(mesh_sbm["specsets/specset"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_material(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_material(mset, 
                                                            field, 
                                                            converted_matset_name, 
                                                            converted_field);
        blueprint::mesh::specset::to_multi_buffer_by_material(mset, 
                                                              sset, 
                                                              converted_matset_name, 
                                                              converted_sset);
        
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        sbm_field_baseline["matset"].reset();
        sbm_field_baseline["matset"] = "matset2";
        sbm_sset_baseline["matset"].reset();
        sbm_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbm_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbm_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(sbm_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_element -> full");
    {
        // diff sbe -> full with full

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];
        Node full_mset_baseline, full_field_baseline, full_sset_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);
        full_sset_baseline.set(mesh_full["specsets/specset"]);

        // remove irrelevant zone mass fractions for a clean diff
        modify_full_specset_to_clear_irrelevant_zone_mass_fractions(full_mset_baseline, full_sset_baseline);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_element(mset, 
                                                           field, 
                                                           converted_matset_name, 
                                                           converted_field);
        blueprint::mesh::specset::to_multi_buffer_by_element(mset, 
                                                             sset, 
                                                             converted_matset_name, 
                                                             converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";
        full_sset_baseline["matset"].reset();
        full_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(full_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_element -> sparse_by_material");
    {
        // diff sbe -> sbm with sbm

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];
        Node sbm_mset_baseline, sbm_field_baseline, sbm_sset_baseline;
        sbm_mset_baseline.set(mesh_sbm["matsets/matset"]);
        sbm_field_baseline.set(mesh_sbm["fields/importance"]);
        sbm_sset_baseline.set(mesh_sbm["specsets/specset"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_material(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_material(mset, 
                                                            field, 
                                                            converted_matset_name, 
                                                            converted_field);
        blueprint::mesh::specset::to_multi_buffer_by_material(mset, 
                                                              sset, 
                                                              converted_matset_name, 
                                                              converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        sbm_field_baseline["matset"].reset();
        sbm_field_baseline["matset"] = "matset2";
        sbm_sset_baseline["matset"].reset();
        sbm_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbm_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbm_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(sbm_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_material -> full");
    {
        // diff sbm -> full with full

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];
        Node full_mset_baseline, full_field_baseline, full_sset_baseline;
        full_mset_baseline.set(mesh_full["matsets/matset"]);
        full_field_baseline.set(mesh_full["fields/importance"]);
        full_sset_baseline.set(mesh_full["specsets/specset"]);

        // remove irrelevant zone mass fractions for a clean diff
        modify_full_specset_to_clear_irrelevant_zone_mass_fractions(full_mset_baseline, full_sset_baseline);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_multi_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_multi_buffer_by_element(mset, 
                                                           field, 
                                                           converted_matset_name, 
                                                           converted_field);
        blueprint::mesh::specset::to_multi_buffer_by_element(mset, 
                                                             sset, 
                                                             converted_matset_name, 
                                                             converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        full_field_baseline["matset"].reset();
        full_field_baseline["matset"] = "matset2";
        full_sset_baseline["matset"].reset();
        full_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(full_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(full_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(full_sset_baseline, info, CONDUIT_EPSILON, true));
    }

    CONDUIT_INFO("venn sparse_by_material -> sparse_by_element");
    {
        // diff sbm -> sbe with sbe

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];
        Node sbe_mset_baseline, sbe_field_baseline, sbe_sset_baseline;
        sbe_mset_baseline.set(mesh_sbe["matsets/matset"]);
        sbe_field_baseline.set(mesh_sbe["fields/importance"]);
        sbe_sset_baseline.set(mesh_sbe["specsets/specset"]);

        std::cout << mset.to_yaml() << std::endl;
        std::cout << field.to_yaml() << std::endl;
        std::cout << sset.to_yaml() << std::endl;

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        blueprint::mesh::matset::to_uni_buffer_by_element(mset, converted_mset);
        blueprint::mesh::field::to_uni_buffer_by_element(mset, 
                                                         field, 
                                                         converted_matset_name, 
                                                         converted_field);
        blueprint::mesh::specset::to_uni_buffer_by_element(mset, 
                                                           sset, 
                                                           converted_matset_name, 
                                                           converted_sset);
        std::cout << converted_mset.to_yaml() << std::endl;
        std::cout << converted_field.to_yaml() << std::endl;
        std::cout << converted_sset.to_yaml() << std::endl;

        sbe_field_baseline["matset"].reset();
        sbe_field_baseline["matset"] = "matset2";
        sbe_sset_baseline["matset"].reset();
        sbe_sset_baseline["matset"] = "matset2";

        EXPECT_FALSE(converted_mset.diff(sbe_mset_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_field.diff(sbe_field_baseline, info, CONDUIT_EPSILON, true));
        EXPECT_FALSE(converted_sset.diff(sbe_sset_baseline, info, CONDUIT_EPSILON, true));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_matset_style_unsupported_transforms)
{
    const int nx = 4, ny = 4;
    const double radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm, info;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full -> uni-buffer by material");
    {
        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        EXPECT_THROW(blueprint::mesh::matset::to_uni_buffer_by_material(mset, converted_mset), conduit::Error);
        EXPECT_THROW(blueprint::mesh::field::to_uni_buffer_by_material(mset, 
                                                                       field, 
                                                                       converted_matset_name, 
                                                                       converted_field), conduit::Error);
        EXPECT_THROW(blueprint::mesh::specset::to_uni_buffer_by_material(mset, 
                                                                         sset, 
                                                                         converted_matset_name, 
                                                                         converted_sset), conduit::Error);
    }

    CONDUIT_INFO("venn sparse_by_element -> uni-buffer by material");
    {
        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        EXPECT_THROW(blueprint::mesh::matset::to_uni_buffer_by_material(mset, converted_mset), conduit::Error);
        EXPECT_THROW(blueprint::mesh::field::to_uni_buffer_by_material(mset, 
                                                                       field, 
                                                                       converted_matset_name, 
                                                                       converted_field), conduit::Error);
        EXPECT_THROW(blueprint::mesh::specset::to_uni_buffer_by_material(mset, 
                                                                         sset, 
                                                                         converted_matset_name, 
                                                                         converted_sset), conduit::Error);
    }

    CONDUIT_INFO("venn sparse_by_material -> full");
    {
        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];

        Node converted_mset, converted_field, converted_sset;
        std::string converted_matset_name = "matset2";
        EXPECT_THROW(blueprint::mesh::matset::to_uni_buffer_by_material(mset, converted_mset), conduit::Error);
        EXPECT_THROW(blueprint::mesh::field::to_uni_buffer_by_material(mset, 
                                                                       field, 
                                                                       converted_matset_name, 
                                                                       converted_field), conduit::Error);
        EXPECT_THROW(blueprint::mesh::specset::to_uni_buffer_by_material(mset, 
                                                                         sset, 
                                                                         converted_matset_name, 
                                                                         converted_sset), conduit::Error);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_misc)
{
    Node mesh;
    blueprint::mesh::examples::misc("specsets", 4, 4, 1, mesh);
    const Node &matset = mesh["matsets/mesh"];
    const Node &specset = mesh["specsets/mesh"];

    Node silo_rep, silo_rep_matset, info;

    const std::string yaml_text = 
        "topology: \"mesh\"\n"
        "buffer_style: \"multi\"\n"
        "dominance: \"element\"\n"
        "material_map: \n"
        "  mat1: 0\n"
        "  mat2: 1\n"
        "nmat: 2\n"
        "nmatspec: [2, 2]\n"
        "specnames: \n"
        "  - \"spec1\"\n"
        "  - \"spec2\"\n"
        "  - \"spec1\"\n"
        "  - \"spec2\"\n"
        "matlist: [1, -1, 0, 1, -3, 0, 1, -5, 0]\n"
        "speclist: [1, -1, 7, 9, -3, 15, 17, -5, 23]\n"
        "mix_vf: [0.5, 0.5, 0.5, 0.5, 0.5, 0.5]\n"
        "mix_mat: [0, 1, 0, 1, 0, 1]\n"
        "mix_next: [2, 0, 4, 0, 6, 0]\n"
        "nspecies_mf: 24\n"
        "species_mf: [0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 1.0, 0.0]\n"
        "mix_spec: [3, 5, 11, 13, 19, 21]\n"
        "mixlen: 6";
    Node baseline;
    baseline.parse(yaml_text, "yaml");

    blueprint::mesh::specset::to_silo(specset, matset, silo_rep);
    std::cout << silo_rep.to_yaml() << std::endl;
    EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
}

// //-----------------------------------------------------------------------------
// TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_specset_edge_cases)
// {
//     CONDUIT_INFO("Case 1: Missing materials and material order is reversed in the specset.");
//     {
//         Node mesh, info;
//         blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

//         // remove some of the materials from the specset
//         mesh["specsets"]["specset"]["matset_values"].remove_child("background");
//         mesh["specsets"]["specset"]["matset_values"].remove_child("circle_b");
//         // create a new specset that has the materials in reverse order
//         mesh["specsets"]["specset2"]["matset"] = "matset";
//         mesh["specsets"]["specset2"]["matset_values"]["circle_c"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_c"]);
//         mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
//         // remove the original specset and replace it with the new one
//         mesh["specsets"].remove_child("specset");
//         mesh["specsets"].rename_child("specset2", "specset");

//         const Node &matset = mesh["matsets/matset"];
//         const Node &specset = mesh["specsets/specset"];

//         Node silo_rep;

//         blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

//         std::cout << specset.to_yaml() << std::endl;
//         std::cout << silo_rep.to_yaml() << std::endl;

//         const std::string yaml_text = 
//             "nmatspec: [0, 2, 0, 3]\n"
//             "specnames: \n"
//               "- \"a_spec1\"\n"
//               "- \"a_spec2\"\n"
//               "- \"c_spec1\"\n"
//               "- \"c_spec2\"\n"
//               "- \"c_spec3\"\n"
//             "speclist: [1, 6, 11, -1]\n"
//             "nmat: 4\n"
//             "nspecies_mf: 20\n"
//             "species_mf: [0.0, 1.0, 1.0, 0.0, 0.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 0.0, 1.0, 0.75, 0.1875, 0.0625, 0.5, 0.5, 0.5, 0.375, 0.125]\n"
//             "mix_spec: [16, 18, 18]\n"
//             "mixlen: 3\n";
//         Node baseline;
//         baseline.parse(yaml_text, "yaml");

//         EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
//     }

//     CONDUIT_INFO("Case 2: Material order is scrambled in the specset.");
//     {
//         Node mesh, info;
//         blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

//         // create a new specset that has the materials in reverse order
//         mesh["specsets"]["specset2"]["matset"] = "matset";
//         mesh["specsets"]["specset2"]["matset_values"]["circle_c"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_c"]);
//         mesh["specsets"]["specset2"]["matset_values"]["background"].set(
//             mesh["specsets"]["specset"]["matset_values"]["background"]);
//         mesh["specsets"]["specset2"]["matset_values"]["circle_b"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_b"]);
//         mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
//         // remove the original specset and replace it with the new one
//         mesh["specsets"].remove_child("specset");
//         mesh["specsets"].rename_child("specset2", "specset");

//         const Node &matset = mesh["matsets/matset"];
//         const Node &specset = mesh["specsets/specset"];

//         Node silo_rep;

//         blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

//         std::cout << specset.to_yaml() << std::endl;
//         std::cout << silo_rep.to_yaml() << std::endl;

//         const std::string yaml_text = 
//             "nmatspec: [1, 2, 2, 3]\n"
//             "specnames: \n"
//             "  - \"bg_spec1\"\n"
//             "  - \"a_spec1\"\n"
//             "  - \"a_spec2\"\n"
//             "  - \"b_spec1\"\n"
//             "  - \"b_spec2\"\n"
//             "  - \"c_spec1\"\n"
//             "  - \"c_spec2\"\n"
//             "  - \"c_spec3\"\n"
//             "speclist: [0, 0, 0, -1]\n"
//             "nmat: 4\n"
//             "nspecies_mf: 32\n"
//             "species_mf: [1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.5, 0.5, 0.0, 1.0, 0.75, 0.1875, 0.0625, 1.0, 0.0, 1.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.375, 0.125]\n"
//             "mix_spec: [26, 28, 30]\n"
//             "mixlen: 3\n";
//         Node baseline;
//         baseline.parse(yaml_text, "yaml");

//         EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
//     }

//     CONDUIT_INFO("Case 3: Material order is scrambled in the matset.");
//     {
//         Node mesh, info;
//         blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

//         mesh["matsets"]["matset2"]["topology"] = "topo";
//         mesh["matsets"]["matset2"]["volume_fractions"]["circle_c"].set(
//             mesh["matsets"]["matset"]["volume_fractions"]["circle_c"]);
//         mesh["matsets"]["matset2"]["volume_fractions"]["background"].set(
//             mesh["matsets"]["matset"]["volume_fractions"]["background"]);
//         mesh["matsets"]["matset2"]["volume_fractions"]["circle_b"].set(
//             mesh["matsets"]["matset"]["volume_fractions"]["circle_b"]);
//         mesh["matsets"]["matset2"]["volume_fractions"]["circle_a"].set(
//             mesh["matsets"]["matset"]["volume_fractions"]["circle_a"]);

//         // remove the original matset and replace it with the new one
//         mesh["matsets"].remove_child("matset");
//         mesh["matsets"].rename_child("matset2", "matset");

//         const Node &matset = mesh["matsets/matset"];
//         const Node &specset = mesh["specsets/specset"];

//         Node silo_rep;

//         blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

//         std::cout << specset.to_yaml() << std::endl;
//         std::cout << silo_rep.to_yaml() << std::endl;

//         const std::string yaml_text = 
//             "nmatspec: [3, 1, 2, 2]\n"
//             "specnames: \n"
//             "  - \"c_spec1\"\n"
//             "  - \"c_spec2\"\n"
//             "  - \"c_spec3\"\n"
//             "  - \"bg_spec1\"\n"
//             "  - \"b_spec1\"\n"
//             "  - \"b_spec2\"\n"
//             "  - \"a_spec1\"\n"
//             "  - \"a_spec2\"\n"
//             "speclist: [0, 0, 0, -1]\n"
//             "nmat: 4\n"
//             "nspecies_mf: 32\n"
//             "species_mf: [1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.75, 0.1875, 0.0625, 1.0, 0.0, 1.0, 0.5, 0.5, 0.75, 0.1875, 0.0625, 1.0, 0.5, 0.5, 0.0, 1.0, 0.5, 0.375, 0.125, 1.0, 0.5, 0.5, 0.5, 0.5]\n"
//             "mix_spec: [25, 29, 31]\n"
//             "mixlen: 3\n";
//         Node baseline;
//         baseline.parse(yaml_text, "yaml");

//         EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
//     }

//     CONDUIT_INFO("Case 4: Missing 1st and last materials and material order is scrambled in the specset.");
//     {
//         Node mesh, info;
//         blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);

//         // remove some of the materials from the specset
//         mesh["specsets"]["specset"]["matset_values"].remove_child("background");
//         mesh["specsets"]["specset"]["matset_values"].remove_child("circle_c");
//         // create a new specset that has the materials in reverse order
//         mesh["specsets"]["specset2"]["matset"] = "matset";
//         mesh["specsets"]["specset2"]["matset_values"]["circle_b"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_b"]);
//         mesh["specsets"]["specset2"]["matset_values"]["circle_a"].set(
//             mesh["specsets"]["specset"]["matset_values"]["circle_a"]);
//         // remove the original specset and replace it with the new one
//         mesh["specsets"].remove_child("specset");
//         mesh["specsets"].rename_child("specset2", "specset");

//         const Node &matset = mesh["matsets/matset"];
//         const Node &specset = mesh["specsets/specset"];

//         Node silo_rep;

//         blueprint::mesh::specset::to_silo(specset, matset, silo_rep);

//         std::cout << specset.to_yaml() << std::endl;
//         std::cout << silo_rep.to_yaml() << std::endl;

//         const std::string yaml_text = 
//             "nmatspec: [0, 2, 2, 0]\n"
//             "specnames: \n"
//             "  - \"a_spec1\"\n"
//             "  - \"a_spec2\"\n"
//             "  - \"b_spec1\"\n"
//             "  - \"b_spec2\"\n"
//             "speclist: [1, 5, 9, -1]\n"
//             "nmat: 4\n"
//             "nspecies_mf: 16\n"
//             "species_mf: [0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.0, 1.0, 0.0, 1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5]\n"
//             "mix_spec: [13, 15, 17]\n"
//             "mixlen: 3\n";
//         Node baseline;
//         baseline.parse(yaml_text, "yaml");

//         EXPECT_FALSE(silo_rep.diff(baseline, info, CONDUIT_EPSILON, true));
//     }
// }

// //-----------------------------------------------------------------------------
// TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_create_or_reuse_matmap)
// {
//     const int nx = 4, ny = 4;
//     const double radius = 0.25;

//     Node info;
//     std::vector<Node> venn_examples(3);
//     blueprint::mesh::examples::venn("full", nx, ny, radius, venn_examples[0]);
//     blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, venn_examples[1]);
//     blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, venn_examples[2]);


//     const std::string yaml_text = 
//         "material_map: \n"
//         "  circle_a: 1\n"
//         "  circle_b: 2\n"
//         "  circle_c: 3\n"
//         "  background: 0\n";
//     Node baseline;
//     baseline.parse(yaml_text, "yaml");

//     for (const Node &venn_example : venn_examples)
//     {
//         const Node &mset = venn_example["matsets/matset"];
//         Node matmap;
//         Node matmap_copy;
//         blueprint::mesh::matset::create_or_reuse_material_map(mset, matmap);
//         blueprint::mesh::matset::create_or_copy_material_map(mset, matmap_copy);

//         EXPECT_FALSE(matmap.diff(baseline["material_map"], info, CONDUIT_EPSILON, true));
//         EXPECT_FALSE(matmap_copy.diff(baseline["material_map"], info, CONDUIT_EPSILON, true));
//     }
// }

// //-----------------------------------------------------------------------------
// TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_renumber_mat_ids)
// {
//     // multi-buffer test
//     {
//         const std::string yaml_text1 = 
//             "topology: \"topo\"\n"
//             "volume_fractions: \n"
//             "  background: [1.0, 1.0, 1.0, 0.0]\n"
//             "  circle_a: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "  circle_b: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "  circle_c: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "material_map: \n"
//             "  circle_a: 6\n"
//             "  circle_b: 8\n"
//             "  circle_c: 3\n"
//             "  background: 9\n";
//         Node matset;
//         matset.parse(yaml_text1, "yaml");

//         const std::string yaml_text2 = 
//             "topology: \"topo\"\n"
//             "volume_fractions: \n"
//             "  background: [1.0, 1.0, 1.0, 0.0]\n"
//             "  circle_a: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "  circle_b: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "  circle_c: [0.0, 0.0, 0.0, 0.333333333333333]\n"
//             "material_map: \n"
//             "  circle_a: 0\n"
//             "  circle_b: 1\n"
//             "  circle_c: 2\n"
//             "  background: 3\n";
//         Node baseline;
//         baseline.parse(yaml_text2, "yaml");

//         // renumber with new matset
//         Node renumbered_matset;
//         blueprint::mesh::matset::renumber_material_ids(matset, renumbered_matset);

//         // renumber in-place
//         blueprint::mesh::matset::renumber_material_ids(matset);

//         Node info;
//         EXPECT_FALSE(renumbered_matset.diff(baseline, info, CONDUIT_EPSILON, true));
//         EXPECT_FALSE(matset.diff(baseline, info, CONDUIT_EPSILON, true));
//     }

//     // uni-buffer test
//     {
//         const std::string yaml_text1 = 
//             "topology: \"topo\"\n"
//             "material_map: \n"
//             "  circle_a: 6\n"
//             "  circle_b: 8\n"
//             "  circle_c: 9\n"
//             "  background: 3\n"
//             "volume_fractions: [1.0, 1.0, 1.0, 0.333333333333333, 0.333333333333333, 0.333333333333333]\n"
//             "material_ids: [3, 3, 3, 6, 8, 9]\n"
//             "sizes: [1, 1, 1, 3]\n"
//             "offsets: [0, 1, 2, 3]\n";
//         Node matset;
//         matset.parse(yaml_text1, "yaml");

//         const std::string yaml_text2 = 
//             "topology: \"topo\"\n"
//             "material_map: \n"
//             "  circle_a: 0\n"
//             "  circle_b: 1\n"
//             "  circle_c: 2\n"
//             "  background: 3\n"
//             "volume_fractions: [1.0, 1.0, 1.0, 0.333333333333333, 0.333333333333333, 0.333333333333333]\n"
//             "material_ids: [3, 3, 3, 0, 1, 2]\n"
//             "sizes: [1, 1, 1, 3]\n"
//             "offsets: [0, 1, 2, 3]\n";
//         Node baseline;
//         baseline.parse(yaml_text2, "yaml");

//         // renumber with new matset
//         Node renumbered_matset;
//         blueprint::mesh::matset::renumber_material_ids(matset, renumbered_matset);

//         // renumber in-place
//         blueprint::mesh::matset::renumber_material_ids(matset);

//         Node info;
//         EXPECT_FALSE(renumbered_matset.diff(baseline, info, CONDUIT_EPSILON, true));
//         EXPECT_FALSE(matset.diff(baseline, info, CONDUIT_EPSILON, true));
//     }
// }

// // //-----------------------------------------------------------------------------
// // TEST(conduit_blueprint_mesh_matset_xforms, mesh_util_to_silo_misc_FOR_FUN)
// // {
// //     Node mesh;
// //     blueprint::mesh::examples::venn_specsets("full", 2, 2, 0.25, mesh);
// //     mesh.print();
// //     const Node &matset = mesh["matsets/matset"];
// //     const Node &specset = mesh["specsets/specset"];

// //     Node silo_rep1, silo_rep2, silo_rep_matset, info;

// //     blueprint::mesh::matset::to_silo(matset, silo_rep_matset);
// //     blueprint::mesh::specset::to_silo(specset, silo_rep_matset, silo_rep2);

// //     std::cout << silo_rep_matset.to_yaml() << std::endl;
// //     std::cout << silo_rep2.to_yaml() << std::endl;
// // }
