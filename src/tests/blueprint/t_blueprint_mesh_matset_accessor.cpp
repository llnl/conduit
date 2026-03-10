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

        EXPECT_EQ(0, m_acc.num_spec_for_mat(0, 0));
    }

    CONDUIT_INFO("construct with matset and field");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, field);

        EXPECT_TRUE(m_acc.has_field());
        EXPECT_FALSE(m_acc.has_specset());

        EXPECT_EQ(0, m_acc.num_spec_for_mat(0, 0));
    }

    CONDUIT_INFO("construct with matset and specset");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        EXPECT_FALSE(m_acc.has_field());
        EXPECT_TRUE(m_acc.has_specset());

        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
    }

    CONDUIT_INFO("construct with matset, field, and specset");
    {
        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        EXPECT_TRUE(m_acc.has_field());
        EXPECT_TRUE(m_acc.has_specset());

        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
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
        EXPECT_EQ(16, m_acc.num_elems());
        EXPECT_EQ(4, m_acc.num_mats());

        EXPECT_THROW(m_acc.num_mats_for_elem(0), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(0), conduit::Error);
    }

    CONDUIT_INFO("venn sparse_by_element layout information");
    {
        const Node &mset = mesh_sbe["matsets/matset"];

        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_TRUE(m_acc.is_uni_buffer());
        EXPECT_FALSE(m_acc.is_multi_buffer());
        EXPECT_TRUE(m_acc.is_element_dominant());
        EXPECT_FALSE(m_acc.is_material_dominant());
        EXPECT_EQ(16, m_acc.num_elems());
        EXPECT_EQ(4, m_acc.num_mats());

        EXPECT_NO_THROW(m_acc.num_mats_for_elem(0));
        EXPECT_THROW(m_acc.num_elems_for_mat(0), conduit::Error);
    }

    CONDUIT_INFO("venn sparse_by_material layout information");
    {
        const Node &mset = mesh_sbm["matsets/matset"];

        MatsetAccessor m_acc = MatsetAccessor(mset);

        EXPECT_FALSE(m_acc.is_uni_buffer());
        EXPECT_TRUE(m_acc.is_multi_buffer());
        EXPECT_FALSE(m_acc.is_element_dominant());
        EXPECT_TRUE(m_acc.is_material_dominant());
        EXPECT_EQ(16, m_acc.num_elems());
        EXPECT_EQ(4, m_acc.num_mats());

        EXPECT_THROW(m_acc.num_mats_for_elem(0), conduit::Error);
        EXPECT_NO_THROW(m_acc.num_elems_for_mat(0));
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

        // you cannot ask a full matset for num mats for elem
        EXPECT_THROW(m_acc.num_mats_for_elem(0), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(1), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(2), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(3), conduit::Error);

        // you cannot ask a full matset for num elems for mat
        EXPECT_THROW(m_acc.num_elems_for_mat(0), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(1), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(2), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(3), conduit::Error);

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

        EXPECT_EQ(1, m_acc.num_mats_for_elem(0));
        EXPECT_EQ(1, m_acc.num_mats_for_elem(1));
        EXPECT_EQ(1, m_acc.num_mats_for_elem(2));
        EXPECT_EQ(3, m_acc.num_mats_for_elem(3));

        // you cannot ask a sbe matset for num elems for mat
        EXPECT_THROW(m_acc.num_elems_for_mat(0), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(1), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(2), conduit::Error);
        EXPECT_THROW(m_acc.num_elems_for_mat(3), conduit::Error);

        // 1 material in element 0
        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
        // 1 material in element 1
        EXPECT_EQ(1, m_acc.num_spec_for_mat(1, 0));
        // 1 material in element 2
        EXPECT_EQ(1, m_acc.num_spec_for_mat(2, 0));
        // 3 materials in element 3
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 0));
        EXPECT_EQ(2, m_acc.num_spec_for_mat(3, 1));
        EXPECT_EQ(3, m_acc.num_spec_for_mat(3, 2));
    }

    CONDUIT_INFO("venn sparse_by_material sizes information");
    {
        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &sset = mesh_sbm["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, sset);

        // you cannot ask a sbm matset for num mats for elem
        EXPECT_THROW(m_acc.num_mats_for_elem(0), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(1), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(2), conduit::Error);
        EXPECT_THROW(m_acc.num_mats_for_elem(3), conduit::Error);

        EXPECT_EQ(3, m_acc.num_elems_for_mat(0));
        EXPECT_EQ(1, m_acc.num_elems_for_mat(1));
        EXPECT_EQ(1, m_acc.num_elems_for_mat(2));
        EXPECT_EQ(1, m_acc.num_elems_for_mat(3));

        // 3 elements for material 0
        EXPECT_EQ(1, m_acc.num_spec_for_mat(0, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(1, 0));
        EXPECT_EQ(1, m_acc.num_spec_for_mat(2, 0));
        // 1 element for material 1
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 1));
        // 1 element for material 2
        EXPECT_EQ(2, m_acc.num_spec_for_mat(0, 2));
        // 1 element for material 3
        EXPECT_EQ(3, m_acc.num_spec_for_mat(0, 3));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_data_retrieval)
{
    const index_t nx = 2, ny = 2;
    const float64 radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full data retrieval");
    {
        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* background */ {0, 0, 0, 0},
            /* circle_a   */ {1, 1, 1, 1},
            /* circle_b   */ {2, 2, 2, 2},
            /* circle_c   */ {3, 3, 3, 3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* background */ {0, 0, 0, 0},
            /* circle_a   */ {1, 1, 1, 1},
            /* circle_b   */ {2, 2, 2, 2},
            /* circle_c   */ {3, 3, 3, 3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* background */ {1.0, 1.0, 1.0, 0.0},
            /* circle_a   */ {0.0, 0.0, 0.0, 0.333333333333333},
            /* circle_b   */ {0.0, 0.0, 0.0, 0.333333333333333},
            /* circle_c   */ {0.0, 0.0, 0.0, 0.333333333333333},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* background */ {0.0, 0.5, 0.5, 0.0},
            /* circle_a   */ {0.0, 0.0, 0.0, 0.100000001490116},
            /* circle_b   */ {0.0, 0.0, 0.0, 0.200000002980232},
            /* circle_c   */ {0.0, 0.0, 0.0, 0.600000023841858},
        };

        // index [mat_idx][spec_idx][elem_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* background  */ {
            /*    bg_spec1 */    {1.0, 1.0, 1.0, 1.0},
            },
            /* circle_a    */ {
            /*    a_spec1  */    {0.0, 0.5, 0.0, 0.5},
            /*    a_spec2  */    {1.0, 0.5, 1.0, 0.5},
            },
            /* circle_b    */ {
            /*    b_spec1  */    {0.0, 0.0, 0.5, 0.5},
            /*    b_spec2  */    {1.0, 1.0, 0.5, 0.5},
            },
            /* circle_c    */ {
            /*    c_spec1  */    {1.0, 0.75, 0.75, 0.5},
            /*    c_spec2  */    {0.0, 0.1875, 0.1875, 0.375},
            /*    c_spec3  */    {0.0, 0.0625, 0.0625, 0.125},
            },
        };

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over elements
        const index_t num_elems = m_acc.num_elems();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            // we ask for the total number of materials
            const index_t num_mats = m_acc.num_mats();
            for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[mat_idx][elem_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[mat_idx][elem_idx], mat_order_id);
                EXPECT_EQ(elem_idx, elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[mat_idx][elem_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[mat_idx][elem_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[mat_idx][spec_idx][elem_idx], mf_val);
                }
            }
        }
    }

    CONDUIT_INFO("venn sparse_by_element data retrieval");
    {
        // index [elem_idx][mat_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* element 0 */ {0},
            /* element 1 */ {0},
            /* element 2 */ {0},
            /* element 3 */ {1, 2, 3},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* element 0 */ {3},
            /* element 1 */ {3},
            /* element 2 */ {3},
            /* element 3 */ {0, 1, 2},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* element 0 */ {1.0},
            /* element 1 */ {1.0},
            /* element 2 */ {1.0},
            /* element 3 */ {0.333333333333333, 0.333333333333333, 0.333333333333333},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* element 0 */ {0.0},
            /* element 1 */ {0.5},
            /* element 2 */ {0.5},
            /* element 3 */ {0.100000001490116, 0.200000002980232, 0.600000023841858},
        };

        // index [elem_idx][mat_idx][spec_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* element 0     */ {
            /*    background */    {1.0},
            },
            /* element 1     */ {
            /*    background */    {1.0},
            },
            /* element 2     */ {
            /*    background */    {1.0},
            },
            /* element 3     */ {
            /*    circle_a   */    {0.5, 0.5},
            /*    circle_b   */    {0.5, 0.5},
            /*    circle_c   */    {0.5, 0.375, 0.125},
            },
        };

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over elements
        const index_t num_elems = m_acc.num_elems();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            // we ask for the number of materials in this element
            const index_t num_mats_for_elem = m_acc.num_mats_for_elem(elem_idx);
            for (index_t mat_idx = 0; mat_idx < num_mats_for_elem; mat_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[elem_idx][mat_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[elem_idx][mat_idx], mat_order_id);
                EXPECT_EQ(elem_idx, elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[elem_idx][mat_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[elem_idx][mat_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[elem_idx][mat_idx][spec_idx], mf_val);
                }
            }
        }
    }

    CONDUIT_INFO("venn sparse_by_material data retrieval");
    {
        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* background */ {0, 0, 0},
            /* circle_a   */ {1},
            /* circle_b   */ {2},
            /* circle_c   */ {3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* background */ {0, 0, 0},
            /* circle_a   */ {1},
            /* circle_b   */ {2},
            /* circle_c   */ {3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> elem_ids_baseline = {
            /* background */ {0, 1, 2},
            /* circle_a   */ {3},
            /* circle_b   */ {3},
            /* circle_c   */ {3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* background */ {1.0, 1.0, 1.0},
            /* circle_a   */ {0.333333333333333},
            /* circle_b   */ {0.333333333333333},
            /* circle_c   */ {0.333333333333333},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* background */ {0.0, 0.5, 0.5},
            /* circle_a   */ {0.100000001490116},
            /* circle_b   */ {0.200000002980232},
            /* circle_c   */ {0.600000023841858},
        };

        // index [mat_idx][spec_idx][elem_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* background  */ {
            /*    bg_spec1 */    {1.0, 1.0, 1.0},
            },
            /* circle_a    */ {
            /*    a_spec1  */    {0.5},
            /*    a_spec2  */    {0.5},
            },
            /* circle_b    */ {
            /*    b_spec1  */    {0.5},
            /*    b_spec2  */    {0.5},
            },
            /* circle_c    */ {
            /*    c_spec1  */    {0.5},
            /*    c_spec2  */    {0.375},
            /*    c_spec3  */    {0.125},
            },
        };

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over materials
        const index_t num_mats = m_acc.num_mats();
        for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
        {
            // we ask for the number of elements for this material
            const index_t num_elems_for_mat = m_acc.num_elems_for_mat(mat_idx);
            for (index_t elem_idx = 0; elem_idx < num_elems_for_mat; elem_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[mat_idx][elem_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[mat_idx][elem_idx], mat_order_id);
                EXPECT_EQ(elem_ids_baseline[mat_idx][elem_idx], elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[mat_idx][elem_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[mat_idx][elem_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[mat_idx][spec_idx][elem_idx], mf_val);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// the goal here is to test several things at once:
// 1. random material ids
// 2. material order is different between matset/field/specset
// 3. a material map is included for all cases
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_data_retrieval_special_cases)
{
    const index_t nx = 2, ny = 2;
    const float64 radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    Node &material_map = mesh_sbe["matsets"]["matset"]["material_map"];

    material_map["circle_a"].set(6);
    material_map["circle_b"].set(2);
    material_map["circle_c"].set(9);
    material_map["background"].set(17);

    // add material maps with strange material numbers to each matset
    mesh_full["matsets"]["matset"]["material_map"].set(material_map);
    mesh_sbm["matsets"]["matset"]["material_map"].set(material_map);

    // update material ids for sbe matset
    index_t_accessor sbe_mat_ids = mesh_sbe["matsets"]["matset"]["material_ids"].value();
    sbe_mat_ids.set(0, 17);
    sbe_mat_ids.set(1, 17);
    sbe_mat_ids.set(2, 17);
    sbe_mat_ids.set(3, 6);
    sbe_mat_ids.set(4, 2);
    sbe_mat_ids.set(5, 9);

    // scramble material order in importance field for full matset mesh rep
    Node full_importance_circle_b, full_importance_background;
    full_importance_circle_b.set(mesh_full["fields"]["importance"]["matset_values"]["circle_b"]);
    full_importance_background.set(mesh_full["fields"]["importance"]["matset_values"]["background"]);
    mesh_full["fields"]["importance"]["matset_values"].remove_child("circle_b");
    mesh_full["fields"]["importance"]["matset_values"].remove_child("background");
    mesh_full["fields"]["importance"]["matset_values"]["background"].set(full_importance_background);
    mesh_full["fields"]["importance"]["matset_values"]["circle_b"].set(full_importance_circle_b);

    // scramble material order in importance field for sbm matset mesh rep
    Node sbm_importance_circle_b, sbm_importance_background;
    sbm_importance_circle_b.set(mesh_sbm["fields"]["importance"]["matset_values"]["circle_b"]);
    sbm_importance_background.set(mesh_sbm["fields"]["importance"]["matset_values"]["background"]);
    mesh_sbm["fields"]["importance"]["matset_values"].remove_child("circle_b");
    mesh_sbm["fields"]["importance"]["matset_values"].remove_child("background");
    mesh_sbm["fields"]["importance"]["matset_values"]["background"].set(sbm_importance_background);
    mesh_sbm["fields"]["importance"]["matset_values"]["circle_b"].set(sbm_importance_circle_b);

    // scramble material order in matset element ids for sbm matset mesh rep
    Node sbm_element_ids_circle_a, sbm_element_ids_background;
    sbm_element_ids_circle_a.set(mesh_sbm["matsets"]["matset"]["element_ids"]["circle_a"]);
    sbm_element_ids_background.set(mesh_sbm["matsets"]["matset"]["element_ids"]["background"]);
    mesh_sbm["matsets"]["matset"]["element_ids"].remove_child("circle_a");
    mesh_sbm["matsets"]["matset"]["element_ids"].remove_child("background");
    mesh_sbm["matsets"]["matset"]["element_ids"]["background"].set(sbm_element_ids_background);
    mesh_sbm["matsets"]["matset"]["element_ids"]["circle_a"].set(sbm_element_ids_circle_a);

    // scramble material order in specset for full matset mesh rep
    Node full_specset_circle_a;
    full_specset_circle_a.set(mesh_full["specsets"]["specset"]["matset_values"]["circle_a"]);
    mesh_full["specsets"]["specset"]["matset_values"].remove_child("circle_a");
    mesh_full["specsets"]["specset"]["matset_values"]["circle_a"].set(full_specset_circle_a);

    // scramble material order in specset for sbm matset mesh rep
    Node sbm_specset_circle_a;
    sbm_specset_circle_a.set(mesh_sbm["specsets"]["specset"]["matset_values"]["circle_a"]);
    mesh_sbm["specsets"]["specset"]["matset_values"].remove_child("circle_a");
    mesh_sbm["specsets"]["specset"]["matset_values"]["circle_a"].set(sbm_specset_circle_a);

    // scramble material order in specset for sbe matset mesh rep
    Node sbe_specset_circle_b;
    sbe_specset_circle_b.set(mesh_sbe["specsets"]["specset"]["species_names"]["circle_b"]);
    mesh_sbe["specsets"]["specset"]["species_names"].remove_child("circle_b");
    mesh_sbe["specsets"]["specset"]["species_names"]["circle_b"].set(sbe_specset_circle_b);

    CONDUIT_INFO("venn full complicated data retrieval");
    {
        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* circle_a   */ {6, 6, 6, 6},
            /* circle_b   */ {2, 2, 2, 2},
            /* circle_c   */ {9, 9, 9, 9},
            /* background */ {17, 17, 17, 17},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* circle_a   */ {0, 0, 0, 0},
            /* circle_b   */ {1, 1, 1, 1},
            /* circle_c   */ {2, 2, 2, 2},
            /* background */ {3, 3, 3, 3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* circle_a   */ {0.0, 0.0, 0.0, 0.333333333333333},
            /* circle_b   */ {0.0, 0.0, 0.0, 0.333333333333333},
            /* circle_c   */ {0.0, 0.0, 0.0, 0.333333333333333},
            /* background */ {1.0, 1.0, 1.0, 0.0},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* circle_a   */ {0.0, 0.0, 0.0, 0.100000001490116},
            /* circle_b   */ {0.0, 0.0, 0.0, 0.200000002980232},
            /* circle_c   */ {0.0, 0.0, 0.0, 0.600000023841858},
            /* background */ {0.0, 0.5, 0.5, 0.0},
        };

        // index [mat_idx][spec_idx][elem_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* circle_a    */ {
            /*    a_spec1  */    {0.0, 0.5, 0.0, 0.5},
            /*    a_spec2  */    {1.0, 0.5, 1.0, 0.5},
            },
            /* circle_b    */ {
            /*    b_spec1  */    {0.0, 0.0, 0.5, 0.5},
            /*    b_spec2  */    {1.0, 1.0, 0.5, 0.5},
            },
            /* circle_c    */ {
            /*    c_spec1  */    {1.0, 0.75, 0.75, 0.5},
            /*    c_spec2  */    {0.0, 0.1875, 0.1875, 0.375},
            /*    c_spec3  */    {0.0, 0.0625, 0.0625, 0.125},
            },
            /* background  */ {
            /*    bg_spec1 */    {1.0, 1.0, 1.0, 1.0},
            },
        };

        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over elements
        const index_t num_elems = m_acc.num_elems();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            // we ask for the total number of materials
            const index_t num_mats = m_acc.num_mats();
            for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[mat_idx][elem_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[mat_idx][elem_idx], mat_order_id);
                EXPECT_EQ(elem_idx, elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[mat_idx][elem_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[mat_idx][elem_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[mat_idx][spec_idx][elem_idx], mf_val);
                }
            }
        }
    }

    CONDUIT_INFO("venn sparse_by_element complicated data retrieval");
    {
        // index [elem_idx][mat_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* element 0 */ {17},
            /* element 1 */ {17},
            /* element 2 */ {17},
            /* element 3 */ {6, 2, 9},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* element 0 */ {3},
            /* element 1 */ {3},
            /* element 2 */ {3},
            /* element 3 */ {0, 1, 2},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* element 0 */ {1.0},
            /* element 1 */ {1.0},
            /* element 2 */ {1.0},
            /* element 3 */ {0.333333333333333, 0.333333333333333, 0.333333333333333},
        };

        // index [elem_idx][mat_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* element 0 */ {0.0},
            /* element 1 */ {0.5},
            /* element 2 */ {0.5},
            /* element 3 */ {0.100000001490116, 0.200000002980232, 0.600000023841858},
        };

        // index [elem_idx][mat_idx][spec_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* element 0     */ {
            /*    background */    {1.0},
            },
            /* element 1     */ {
            /*    background */    {1.0},
            },
            /* element 2     */ {
            /*    background */    {1.0},
            },
            /* element 3     */ {
            /*    circle_a   */    {0.5, 0.5},
            /*    circle_b   */    {0.5, 0.5},
            /*    circle_c   */    {0.5, 0.375, 0.125},
            },
        };

        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over elements
        const index_t num_elems = m_acc.num_elems();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            // we ask for the number of materials in this element
            const index_t num_mats_for_elem = m_acc.num_mats_for_elem(elem_idx);
            for (index_t mat_idx = 0; mat_idx < num_mats_for_elem; mat_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[elem_idx][mat_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[elem_idx][mat_idx], mat_order_id);
                EXPECT_EQ(elem_idx, elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[elem_idx][mat_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[elem_idx][mat_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[elem_idx][mat_idx][spec_idx], mf_val);
                }
            }
        }
    }

    CONDUIT_INFO("venn sparse_by_material complicated data retrieval");
    {
        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_ids_baseline = {
            /* circle_a   */ {6},
            /* circle_b   */ {2},
            /* circle_c   */ {9},
            /* background */ {17, 17, 17},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> mat_order_ids_baseline = {
            /* circle_a   */ {0},
            /* circle_b   */ {1},
            /* circle_c   */ {2},
            /* background */ {3, 3, 3},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<index_t>> elem_ids_baseline = {
            /* circle_a   */ {3},
            /* circle_b   */ {3},
            /* circle_c   */ {3},
            /* background */ {0, 1, 2},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> vol_fracs_baseline = {
            /* circle_a   */ {0.333333333333333},
            /* circle_b   */ {0.333333333333333},
            /* circle_c   */ {0.333333333333333},
            /* background */ {1.0, 1.0, 1.0},
        };

        // index [mat_idx][elem_idx]
        const std::vector<std::vector<float64>> mset_vals_baseline = {
            /* circle_a   */ {0.100000001490116},
            /* circle_b   */ {0.200000002980232},
            /* circle_c   */ {0.600000023841858},
            /* background */ {0.0, 0.5, 0.5},
        };

        // index [mat_idx][spec_idx][elem_idx]
        const std::vector<std::vector<std::vector<float64>>> mf_vals_baseline = {
            /* circle_a    */ {
            /*    a_spec1  */    {0.5},
            /*    a_spec2  */    {0.5},
            },
            /* circle_b    */ {
            /*    b_spec1  */    {0.5},
            /*    b_spec2  */    {0.5},
            },
            /* circle_c    */ {
            /*    c_spec1  */    {0.5},
            /*    c_spec2  */    {0.375},
            /*    c_spec3  */    {0.125},
            },
            /* background  */ {
            /*    bg_spec1 */    {1.0, 1.0, 1.0},
            },
        };

        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];

        MatsetAccessor m_acc = MatsetAccessor(mset, field, sset);

        // we iterate over materials
        const index_t num_mats = m_acc.num_mats();
        for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
        {
            // we ask for the number of elements for this material
            const index_t num_elems_for_mat = m_acc.num_elems_for_mat(mat_idx);
            for (index_t elem_idx = 0; elem_idx < num_elems_for_mat; elem_idx ++)
            {
                const index_t mat_id = m_acc.get_mat_id(elem_idx, mat_idx);
                const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
                const index_t elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                const float64 mset_val = m_acc.get_mset_val(elem_idx, mat_idx);

                EXPECT_EQ(mat_ids_baseline[mat_idx][elem_idx], mat_id);
                EXPECT_EQ(mat_order_ids_baseline[mat_idx][elem_idx], mat_order_id);
                EXPECT_EQ(elem_ids_baseline[mat_idx][elem_idx], elem_id);
                EXPECT_FLOAT_EQ(vol_fracs_baseline[mat_idx][elem_idx], vol_frac);
                EXPECT_FLOAT_EQ(mset_vals_baseline[mat_idx][elem_idx], mset_val);

                const index_t num_specs_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_specs_for_mat; spec_idx ++)
                {
                    const float64 mf_val = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);

                    EXPECT_EQ(mf_vals_baseline[mat_idx][spec_idx][elem_idx], mf_val);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_mesh_matset_accessor, matset_accessor_data_retrieval_errors)
{
    const index_t nx = 2, ny = 2;
    const float64 radius = 0.25;

    Node mesh_full, mesh_sbe, mesh_sbm;
    blueprint::mesh::examples::venn_specsets("full", nx, ny, radius, mesh_full);
    blueprint::mesh::examples::venn_specsets("sparse_by_element", nx, ny, radius, mesh_sbe);
    blueprint::mesh::examples::venn_specsets("sparse_by_material", nx, ny, radius, mesh_sbm);

    CONDUIT_INFO("venn full data retrieval errors");
    {
        const Node &mset = mesh_full["matsets/matset"];
        const Node &field = mesh_full["fields/importance"];
        const Node &sset = mesh_full["specsets/specset"];

        MatsetAccessor m_acc_only_matset = MatsetAccessor(mset);

        EXPECT_NO_THROW(m_acc_only_matset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_only_matset.get_mset_val(0, 0), conduit::Error);
        EXPECT_THROW(m_acc_only_matset.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_field = MatsetAccessor(mset, field);

        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mset_val(0, 0));
        EXPECT_THROW(m_acc_matset_and_field.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_specset = MatsetAccessor(mset, sset);

        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_matset_and_specset.get_mset_val(0, 0), conduit::Error);
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mass_frac(0, 0, 0));

        MatsetAccessor m_acc_matset_field_specset = MatsetAccessor(mset, field, sset);

        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mset_val(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mass_frac(0, 0, 0));
    }

    CONDUIT_INFO("venn sparse_by_element data retrieval errors");
    {
        const Node &mset = mesh_sbe["matsets/matset"];
        const Node &field = mesh_sbe["fields/importance"];
        const Node &sset = mesh_sbe["specsets/specset"];

        MatsetAccessor m_acc_only_matset = MatsetAccessor(mset);

        EXPECT_NO_THROW(m_acc_only_matset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_only_matset.get_mset_val(0, 0), conduit::Error);
        EXPECT_THROW(m_acc_only_matset.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_field = MatsetAccessor(mset, field);

        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mset_val(0, 0));
        EXPECT_THROW(m_acc_matset_and_field.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_specset = MatsetAccessor(mset, sset);

        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_matset_and_specset.get_mset_val(0, 0), conduit::Error);
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mass_frac(0, 0, 0));

        MatsetAccessor m_acc_matset_field_specset = MatsetAccessor(mset, field, sset);

        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mset_val(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mass_frac(0, 0, 0));
    }

    CONDUIT_INFO("venn sparse_by_material data retrieval errors");
    {
        const Node &mset = mesh_sbm["matsets/matset"];
        const Node &field = mesh_sbm["fields/importance"];
        const Node &sset = mesh_sbm["specsets/specset"];

        MatsetAccessor m_acc_only_matset = MatsetAccessor(mset);

        EXPECT_NO_THROW(m_acc_only_matset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_only_matset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_only_matset.get_mset_val(0, 0), conduit::Error);
        EXPECT_THROW(m_acc_only_matset.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_field = MatsetAccessor(mset, field);

        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_field.get_mset_val(0, 0));
        EXPECT_THROW(m_acc_matset_and_field.get_mass_frac(0, 0, 0), conduit::Error);

        MatsetAccessor m_acc_matset_and_specset = MatsetAccessor(mset, sset);

        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_vol_frac(0, 0));
        EXPECT_THROW(m_acc_matset_and_specset.get_mset_val(0, 0), conduit::Error);
        EXPECT_NO_THROW(m_acc_matset_and_specset.get_mass_frac(0, 0, 0));

        MatsetAccessor m_acc_matset_field_specset = MatsetAccessor(mset, field, sset);

        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mat_order_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_elem_id(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_vol_frac(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mset_val(0, 0));
        EXPECT_NO_THROW(m_acc_matset_field_specset.get_mass_frac(0, 0, 0));
    }
}
