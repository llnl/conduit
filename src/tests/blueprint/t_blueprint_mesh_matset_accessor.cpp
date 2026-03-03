// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_mesh_matset_accessor.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_log.hpp"
#include "conduit_blueprint_mesh_matset_accessor.hpp"

#include <algorithm>
#include <vector>
#include <string>
#include "gtest/gtest.h"

using namespace conduit;

using MatsetAccessor = conduit::blueprint::mesh::matset::MatsetAccessor;

//-----------------------------------------------------------------------------

/// Test Cases ///

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_constructions)
{
    const index_t nx = 4, ny = 4;
    const float64 radius = 0.25;

    Node mesh;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh);

    const Node &mset = mesh["matsets/matset"];
    const Node &field = mesh["fields/importance"];
    const Node &sset = mesh["specsets/specset"];

    CONDUIT_INFO("construct with matset only");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_FALSE(m_acc.has_field());
        EXPECT_FALSE(m_acc.has_specset());
    }

    CONDUIT_INFO("construct with matset and field");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, field);

        EXPECT_TRUE(m_acc.has_field());
        EXPECT_FALSE(m_acc.has_specset());
    }

    CONDUIT_INFO("construct with matset and specset");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        EXPECT_FALSE(m_acc.has_field());
        EXPECT_TRUE(m_acc.has_specset());
    }

    CONDUIT_INFO("construct with matset, field, and specset");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        EXPECT_TRUE(m_acc.has_field());
        EXPECT_TRUE(m_acc.has_specset());
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_layout_information)
{
    const index_t nx = 4, ny = 4;
    const float64 radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm;
    blueprint::mesh::examples::venn("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full layout information");
    {
        const Node &mset = mesh_full["matsets/matset"];

        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_FALSE(m_acc.is_uni_buffer());
        EXPECT_TRUE(m_acc.is_multi_buffer());
        EXPECT_TRUE(m_acc.is_element_dominant());
        EXPECT_FALSE(m_acc.is_material_dominant());
        EXPECT_EQ(16, m_acc.num_zones());
        EXPECT_EQ(4, m_acc.num_mats());
    }

    CONDUIT_INFO("venn sparse_by_element layout information");
    {
        const Node &mset = mesh_sbe["matsets/matset"];

        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_TRUE(m_acc.is_uni_buffer());
        EXPECT_FALSE(m_acc.is_multi_buffer());
        EXPECT_TRUE(m_acc.is_element_dominant());
        EXPECT_FALSE(m_acc.is_material_dominant());
        EXPECT_EQ(16, m_acc.num_zones());
        EXPECT_EQ(4, m_acc.num_mats());
    }

    CONDUIT_INFO("venn sparse_by_material layout information");
    {
        const Node &mset = mesh_sbm["matsets/matset"];

        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_FALSE(m_acc.is_uni_buffer());
        EXPECT_TRUE(m_acc.is_multi_buffer());
        EXPECT_FALSE(m_acc.is_element_dominant());
        EXPECT_TRUE(m_acc.is_material_dominant());
        EXPECT_EQ(16, m_acc.num_zones());
        EXPECT_EQ(4, m_acc.num_mats());
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_sizes_information)
{
    const index_t nx = 2, ny = 2;
    const float64 radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full sizes information");
    {
        const Node &mset = mesh_full["matsets/matset"];
        const Node &sset = mesh_full["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        // you cannot ask a full matset for num mats for zone
        EXPECT_THROW(m_acc.num_mats_for_zone(0), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(1), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(2), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(3), conduit::Error);

        // you cannot ask a full matset for num zones for mat
        EXPECT_THROW(m_acc.num_zones_for_mat(0), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(1), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(2), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(3), conduit::Error);

        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(1, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(2, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(3, 0));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 1));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(1, 1));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(2, 1));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 1));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 2));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(1, 2));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(2, 2));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 2));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(0, 3));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(1, 3));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(2, 3));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(3, 3));
    }

    CONDUIT_INFO("venn sparse_by_element sizes information");
    {
        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &sset = mesh_sbe["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        EXPECT_EQ(1, m_acc.num_mats_for_zone(0));
        EXPECT_EQ(1, m_acc.num_mats_for_zone(1));
        EXPECT_EQ(1, m_acc.num_mats_for_zone(2));
        EXPECT_EQ(3, m_acc.num_mats_for_zone(3));

        // you cannot ask a sbe matset for num zones for mat
        EXPECT_THROW(m_acc.num_zones_for_mat(0), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(1), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(2), conduit::Error);
        EXPECT_THROW(m_acc.num_zones_for_mat(3), conduit::Error);

        // 1 material in zone 0
        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
        // 1 material in zone 1
        EXPECT_EQ(1, m_acc.num_spec_for_mat(1, 0));
        // 1 material in zone 2
        EXPECT_EQ(1, m_acc.num_spec_for_mat(2, 0));
        // 3 materials in zone 3
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 0));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 1));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(3, 2));
    }

    CONDUIT_INFO("venn sparse_by_material sizes information");
    {
        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &sset = mesh_sbm["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        // you cannot ask a sbm matset for num mats for zone
        EXPECT_THROW(m_acc.num_mats_for_zone(0), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(1), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(2), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_zone(3), conduit::Error);

        EXPECT_EQ(3, m_acc.num_zones_for_mat(0));
        EXPECT_EQ(1, m_acc.num_zones_for_mat(1));
        EXPECT_EQ(1, m_acc.num_zones_for_mat(2));
        EXPECT_EQ(1, m_acc.num_zones_for_mat(3));

        // 3 zones for material 0
        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(1, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(2, 0));
        // 1 zone for material 1
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 1));
        // 1 zone for material 2
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 2));
        // 1 zone for material 3
        EXPECT_EQ(3, m_acc.num_spec_for_mat(0, 3));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_data_retrieval)
{
    // TODO
}
