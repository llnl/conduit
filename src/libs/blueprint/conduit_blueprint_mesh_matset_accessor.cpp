// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_matset_accessor.cpp
///
//-----------------------------------------------------------------------------
#include <sstream>

#include "conduit_blueprint_mesh_matset_accessor.hpp"

#include "conduit_error.hpp"
#include "conduit_utils.hpp"
#include "conduit_blueprint_mesh.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint --
//-----------------------------------------------------------------------------
namespace blueprint
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh --
//-----------------------------------------------------------------------------
namespace mesh
{

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::matset --
//-----------------------------------------------------------------------------
namespace matset
{

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// MatsetAccessor
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// MatsetAccessor Construction and Destruction
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor()
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_mat_order_id(&MatsetAccessor::get_error_mat_order_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_mass_frac(&MatsetAccessor::get_error_mass_frac),
   m_get_nmats_for_elem(&MatsetAccessor::get_error_nmats_for_elem),
   m_get_nelems_for_mat(&MatsetAccessor::get_error_nelems_for_mat),
   m_get_nspec_for_mat(&MatsetAccessor::get_error_nspec_for_mat)
{
    // empty
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_mat_order_id(&MatsetAccessor::get_error_mat_order_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_mass_frac(&MatsetAccessor::get_error_mass_frac),
   m_get_nmats_for_elem(&MatsetAccessor::get_error_nmats_for_elem),
   m_get_nelems_for_mat(&MatsetAccessor::get_error_nelems_for_mat),
   m_get_nspec_for_mat(&MatsetAccessor::get_error_nspec_for_mat)
{
    init(matset, nullptr, nullptr);
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset,
                               const Node &specset_or_field)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_mat_order_id(&MatsetAccessor::get_error_mat_order_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_mass_frac(&MatsetAccessor::get_error_mass_frac),
   m_get_nmats_for_elem(&MatsetAccessor::get_error_nmats_for_elem),
   m_get_nelems_for_mat(&MatsetAccessor::get_error_nelems_for_mat),
   m_get_nspec_for_mat(&MatsetAccessor::get_error_nspec_for_mat)
{
    if (specset_or_field.has_child("topology"))
    {
        init(matset, &specset_or_field, nullptr);
    }
    else
    {
        init(matset, nullptr, &specset_or_field);
    }
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset,
                               const Node &field,
                               const Node &specset)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_mat_order_id(&MatsetAccessor::get_error_mat_order_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_mass_frac(&MatsetAccessor::get_error_mass_frac),
   m_get_nmats_for_elem(&MatsetAccessor::get_error_nmats_for_elem),
   m_get_nelems_for_mat(&MatsetAccessor::get_error_nelems_for_mat),
   m_get_nspec_for_mat(&MatsetAccessor::get_error_nspec_for_mat)
{
    init(matset, &field, &specset);
}   

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const MatsetAccessor &other_matset_accessor)
: m_get_mat_id(&MatsetAccessor::get_error_mat_id),
  m_get_mat_order_id(&MatsetAccessor::get_error_mat_order_id),
  m_get_elem_id(&MatsetAccessor::get_error_elem_id),
  m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
  m_get_mset_val(&MatsetAccessor::get_error_mset_val),
  m_get_mass_frac(&MatsetAccessor::get_error_mass_frac),
  m_get_nmats_for_elem(&MatsetAccessor::get_error_nmats_for_elem),
  m_get_nelems_for_mat(&MatsetAccessor::get_error_nelems_for_mat),
  m_get_nspec_for_mat(&MatsetAccessor::get_error_nspec_for_mat)
{
    if (other_matset_accessor.m_src_matset != nullptr)
    {
        init(*other_matset_accessor.m_src_matset,
             other_matset_accessor.m_src_field,
             other_matset_accessor.m_src_specset);
    }
}

//---------------------------------------------------------------------------//
MatsetAccessor &
MatsetAccessor::operator=(const MatsetAccessor &other_matset_accessor)
{
    if (this != &other_matset_accessor)
    {
        if (other_matset_accessor.m_src_matset != nullptr)
        {
            init(*other_matset_accessor.m_src_matset,
                 other_matset_accessor.m_src_field,
                 other_matset_accessor.m_src_specset);
        }
        else
        {
            reset_state();
        }
    }
    return *this;
}

//-----------------------------------------------------------------------------
void
MatsetAccessor::reset_state()
{
    m_get_mat_id = &MatsetAccessor::get_error_mat_id;
    m_get_mat_order_id = &MatsetAccessor::get_error_mat_order_id;
    m_get_elem_id = &MatsetAccessor::get_error_elem_id;
    m_get_vol_frac = &MatsetAccessor::get_error_vol_frac;
    m_get_mset_val = &MatsetAccessor::get_error_mset_val;
    m_get_mass_frac = &MatsetAccessor::get_error_mass_frac;
    m_get_nmats_for_elem = &MatsetAccessor::get_error_nmats_for_elem;
    m_get_nelems_for_mat = &MatsetAccessor::get_error_nelems_for_mat;
    m_get_nspec_for_mat = &MatsetAccessor::get_error_nspec_for_mat;

    m_is_uni_buffer = false;
    m_is_element_dominant = false;
    m_num_elems = 0;
    m_num_mats = 0;
    m_has_field = false;
    m_has_specset = false;
    m_src_matset = nullptr;
    m_src_field = nullptr;
    m_src_specset = nullptr;

    m_internal_data.reset();
    m_internal_nmatspec = index_t_accessor();
    m_internal_nmatspec_offsets = index_t_accessor();
    m_multi_vol_fracs.clear();
    m_multi_mset_vals.clear();
    m_internal_multi_mat_idx_map = index_t_accessor();
    m_multi_mass_fracs.clear();
    m_sbm_elem_ids.clear();
    m_sbe_material_ids = index_t_accessor();
    m_internal_sbe_mat_order_ids = index_t_accessor();
    m_sbe_vol_fracs = float64_accessor();
    m_sbe_mset_vals = float64_accessor();
    m_sbe_o2m_idx = o2mrelation::O2MIndex();
    m_sbe_mass_fracs = float64_accessor();
    m_sbe_specset_o2m_idx = o2mrelation::O2MIndex();
}

//-----------------------------------------------------------------------------
void
MatsetAccessor::init(const Node &matset,
                     const Node *field,
                     const Node *specset)
{
    reset_state();

    // extra seat belts here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::MatsetAccessor"
                      " passed matset node must be a valid matset tree.");
    }

    m_src_matset = &matset;
    m_src_field = field;
    m_src_specset = specset;

    if (nullptr != field)
    {
        if (! (*field).dtype().is_object())
        {
            CONDUIT_ERROR("blueprint::mesh::matset::MatsetAccessor"
                          " passed field node must be a valid field tree.");
        }

        m_has_field = true;
    }
    if (nullptr != specset)
    {
        if (! (*specset).dtype().is_object())
        {
            CONDUIT_ERROR("blueprint::mesh::matset::MatsetAccessor"
                          " passed specset node must be a valid specset tree.");
        }
        m_has_specset = true;
    }

    m_is_uni_buffer       = matset::is_uni_buffer(matset);
    m_is_element_dominant = matset::is_element_dominant(matset);

    m_num_elems = count_elements_from_matset(matset);
    m_num_mats  = count_materials_from_matset(matset);

    Node material_map;
    create_or_reuse_material_map(matset, material_map);

    if (m_is_uni_buffer)
    {
        // uni-buffer by element (sparse by element)
        if (m_is_element_dominant)
        {
            // set our accessors
            m_sbe_material_ids = matset["material_ids"].value();
            m_sbe_vol_fracs = matset["volume_fractions"].value();
            m_sbe_o2m_idx = o2mrelation::O2MIndex(matset);

            //
            // save material order ids
            //
            const index_t num_vol_fracs = matset["volume_fractions"].dtype().number_of_elements();
            m_internal_data["sbe_mat_order_ids"].set(DataType::index_t(num_vol_fracs));
            m_internal_sbe_mat_order_ids = m_internal_data["sbe_mat_order_ids"].value();
            // create a map from material id to material order id
            std::map<index_t, index_t> mat_id_to_order_id;
            for (index_t mat_idx = 0; mat_idx < m_num_mats; mat_idx ++)
            {
                const index_t mat_id = material_map.child(mat_idx).to_index_t();
                mat_id_to_order_id[mat_id] = mat_idx;
            }
            // now we can fill the mat order id array using the information we've collected
            for (index_t vf_elem = 0; vf_elem < num_vol_fracs; vf_elem ++)
            {
                const index_t mat_id = m_sbe_material_ids[vf_elem];
                const index_t mat_order_id = mat_id_to_order_id.at(mat_id);
                m_internal_sbe_mat_order_ids.set(vf_elem, mat_order_id);
            }

            if (nullptr != field)
            {
                m_sbe_mset_vals = (*field)["matset_values"].value();
            }
            if (nullptr != specset)
            {
                m_sbe_mass_fracs = (*specset)["matset_values"].value();
                m_sbe_specset_o2m_idx = o2mrelation::O2MIndex(*specset);

                // For sparse by element material sets, the matset accessor uses the following
                // ranges:
                //     0 <= elem_idx < num elements
                //     0 <= mat_idx < num mats for element elem_idx
                //     0 <= spec_idx < num species for material mat_idx in element elem_idx
                // To know which species mass fraction to fetch for a given (elem_idx, mat_idx, spec_idx),
                // we need to know the number of species contained in all of the materials that
                // appear in element elem_idx. Consider this example:

                // matset: 
                //   topology: "topo"
                //   material_map: 
                //     circle_a: 1
                //     circle_b: 2
                //     circle_c: 3
                //     background: 4
                //   volume_fractions: [1.0, 1.0, 1.0, 0.33, 0.33, 0.33]
                //   material_ids:     [4,   4,   4,   1,    2,    3   ]
                //   sizes:            [1,   1,   1,   3               ]
                //   offsets:          [0,   1,   2,   3               ]

                // specset: 
                //   matset: "matset"
                //   species_names: 
                //     background: 
                //       bg_spec1: 
                //     circle_a: 
                //       a_spec1: 
                //       a_spec2: 
                //     circle_b: 
                //       b_spec1: 
                //       b_spec2: 
                //     circle_c: 
                //       c_spec1: 
                //       c_spec2: 
                //       c_spec3: 
                //   matset_values: [1.0, 1.0, 1.0, 0.4, 0.6, 0.55, 0.45, 0.5, 0.375, 0.125]
                //   sizes:         [1,   1,   1,   7                                      ]
                //   offsets:       [0,   1,   2,   3                                      ]

                // Let's say I want to extract the mass fraction for
                // (elem_idx, mat_idx, spec_idx) = (3, 1, 1).
                // Here is the relevant information for element 3 (remember that elements start at 0):

                // material information:
                //   volume_fractions: [0.33, 0.33, 0.33]
                //   material_ids:     [1,    2,    3   ]
                //   size:              3
                //   offset:            3
                // species information:
                //   matset_values: [0.4, 0.6, 0.55, 0.45, 0.5, 0.375, 0.125]
                //   size:           7
                //   offset:         3

                // We have three materials in element 3, with material ids 1, 2, and 3.
                // We have 7 species mass fractions in element 3.

                // We are looking to get data for (elem_idx, mat_idx, spec_idx) = (3, 1, 1), so
                // now we want to extract the species data for mat_idx = 1. Because this is a
                // sparse by element material set, 0 <= mat_idx < num mats for element elem_idx, so
                // mat_idx = 1 corresponds in this case to material id 2.

                // I can extract the volume fraction and material id for mat_idx = 1, but I cannot
                // get the species value of 0.45 for (elem_idx, mat_idx, spec_idx) = (3, 1, 1)
                // without first knowing how many species there are in the current material and all
                // the materials that came before mat_idx = 1. I need to know how many species there
                // were for mat_idx = 0 in this case.

                // One approach would be as follows:
                //    1. Fetch material ids for the current and previous materials
                //          "current" material mat_idx = 1 -> material id = 2
                //          "previous" material mat_idx = 0 -> material id = 1
                //       Our intention is to somehow look up the number of material species
                //       for each of these materials.
                //    2. But how to get the number of material species? We won't have access to the 
                //       material map or species names during execution, just pointers to data arrays.
                //       Due to the fact that material ids need not go from 0 to the number of materials
                //       minus 1, it is very convoluted to create a mapping scheme from material ids
                //       to the number of material species that can be flattened into data arrays.
                //       So this approach is not workable.

                // Our approach then is to add two new data arrays that can help us, the
                // number of material species and the number of material species offsets.
                // These arrays will encode information that is relevant to each element's materials.
                //    "nmatspec" will record the number of material species for material mat_idx
                //    in element elem_idx.
                //    "nmatspec_offsets" will record the number of material species seen thus far
                //    in the current element elem_idx up to the current material mat_idx.
                // The easiest way to explain is via example:

                // matset: 
                //   topology: "topo"
                //   material_map: 
                //     circle_a: 1
                //     circle_b: 2
                //     circle_c: 3
                //     background: 4
                //   volume_fractions: [1.0, 1.0, 1.0, 0.33, 0.33, 0.33]
                //   material_ids:     [4,   4,   4,   1,    2,    3   ]
                //***nmatspec:*********[1,   1,   1,   2,    2,    3   ]****************
                //***nmatspec_offsets:*[0,   0,   0,   0,    2,    4   ]****************
                //   sizes:            [1,   1,   1,   3               ]
                //   offsets:          [0,   1,   2,   3               ]

                // specset: 
                //   matset: "matset"
                //   species_names: 
                //     background: 
                //       bg_spec1: 
                //     circle_a: 
                //       a_spec1: 
                //       a_spec2: 
                //     circle_b: 
                //       b_spec1: 
                //       b_spec2: 
                //     circle_c: 
                //       c_spec1: 
                //       c_spec2: 
                //       c_spec3: 
                //   matset_values: [1.0, 1.0, 1.0, 0.4, 0.6, 0.55, 0.45, 0.5, 0.375, 0.125]
                //   sizes:         [1,   1,   1,   7                                      ]
                //   offsets:       [0,   1,   2,   3                                      ]

                // These arrays tell us the answers we are looking for. Let us proceed by taking
                // another look at our earlier example, mass fraction extraction for 
                // (elem_idx, mat_idx, spec_idx) = (3, 1, 1).

                // Here is now the relevant information for element 3 (remember that elements start at 0):

                // material information:
                //   volume_fractions: [0.33, 0.33, 0.33]
                //   material_ids:     [1,    2,    3   ]
                //***nmatspec:*********[2,    2,    3   ]*********
                //***nmatspec_offsets:*[0,    2,    4   ]*********
                //   size:              3
                //   offset:            3 // irrelevant now since we have used the offset to get to the right place
                // species information:
                //   matset_values: [0.4, 0.6, 0.55, 0.45, 0.5, 0.375, 0.125]
                //   size:           7
                //   offset:         3 // irrelevant now since we have used the offset to get to the right place

                // Now we have what we need to interpret the species values appropriately.
                // For mat_idx = 1, we see that we are dealing with
                //    volume fraction = 0.33
                //    material id     = 2
                //    num species     = 2
                //    species offset  = 2

                // So in the matset values array, we need to go to index 2 (species offset) to get
                // the species data for this material. Additionally, there are num species = 2 values
                // to read for this material. So now we can move another level down the hierarchy
                // and only look at information for elem_idx = 3, mat_idx = 1:

                // Here is the relevant information for elem_idx 3, mat_idx 1: (remember that both start at 0):
                // material information:
                //   volume_fraction: 0.33
                //   material_id:     2  
                //***nmatspec:********2**
                //***nmatspec_offset:*2** // irrelevant now since we have used the offset to get to the right place
                // species information:
                //   matset_values: [0.55, 0.45]

                // Now we can finally extract data for spec_idx = 1, and arrive at our requested
                // mass fraction for (elem_idx, mat_idx, spec_idx) = (3, 1, 1), 0.45.

                // The following code below creates the nmatspec and nmatspec_offsets arrays.
                // We pay a price at the start when creating these arrays, but we enable quick
                // access of species mass fraction data.

                m_internal_data["nmatspec"].set(DataType::index_t(num_vol_fracs));
                m_internal_nmatspec = m_internal_data["nmatspec"].value();
                m_internal_data["nmatspec_offsets"].set(DataType::index_t(num_vol_fracs));
                m_internal_nmatspec_offsets = m_internal_data["nmatspec_offsets"].value();

                // create a map from material id to nmatspec
                std::map<index_t, index_t> mat_id_to_nmatspec;
                for (index_t mat_idx = 0; mat_idx < m_num_mats; mat_idx ++)
                {
                    const Node &matmap_entry = material_map.child(mat_idx);
                    const std::string matname = matmap_entry.name();
                    const index_t mat_id = matmap_entry.to_index_t();

                    index_t nmatspec = 0;
                    if ((*specset)["species_names"].has_child(matname))
                    {
                        nmatspec = (*specset)["species_names"][matname].number_of_children();
                    }

                    mat_id_to_nmatspec[mat_id] = nmatspec;
                }

                // now we can fill the arrays using the information we've collected
                for (index_t elem_idx = 0; elem_idx < m_num_elems; elem_idx ++)
                {
                    const index_t num_mats_in_elem = m_sbe_o2m_idx.size(elem_idx);
                    index_t nmatspec_offset = 0;
                    for (index_t mat_idx = 0; mat_idx < num_mats_in_elem; mat_idx ++)
                    {
                        const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
                        const index_t mat_id = m_sbe_material_ids[data_index];
                        const index_t nmatspec_for_mat = mat_id_to_nmatspec.at(mat_id);

                        m_internal_nmatspec.set(data_index, nmatspec_for_mat);
                        m_internal_nmatspec_offsets.set(data_index, nmatspec_offset);
                        nmatspec_offset += nmatspec_for_mat;
                    }
                }
            }

            // set our fetch methods
            m_get_mat_id       = &MatsetAccessor::get_sbe_mat_id;
            m_get_mat_order_id = &MatsetAccessor::get_sbe_mat_order_id;
            m_get_elem_id      = &MatsetAccessor::get_sbe_elem_id;
            m_get_vol_frac     = &MatsetAccessor::get_sbe_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_sbe_mset_val;
            }
            m_get_nmats_for_elem = &MatsetAccessor::get_sbe_nmats_for_elem;

            if (nullptr != specset)
            {
                m_get_mass_frac = &MatsetAccessor::get_sbe_mass_frac;
                m_get_nspec_for_mat = &MatsetAccessor::get_sbe_nspec_for_mat;
            }
        }
        // uni-buffer by material (unsupported)
        else
        {
            CONDUIT_ERROR("conduit::blueprint::mesh::matset::MatsetAccessor "
                          "uni-buffer by material matset is currently unsupported.");
        }
    }
    else // multi-buffer case
    {
        // We need to have an array of zeroes we can use when we don't have
        // data for something. We may have a matset with materials in the
        // material_map that do not have corresponding volume_fractions. The
        // correct answer when I ask what the volume fraction is for that material
        // in any zone is "0.0". Having an array like this means we can have the
        // correct answer on hand without needing any conditionals to check for this
        // case inside of any loops.
        if (m_is_element_dominant)
        {
            // multi-buffer by element (full)
            m_internal_data["zeroes"].set(DataType::float64(m_num_elems));
            float64_array zeroes_arr = m_internal_data["zeroes"].value();
            zeroes_arr.fill(0.0);
        }
        else
        {
            // multi-buffer by material (sparse by material)
            m_internal_data["zeroes"].set(DataType::float64(0));
        }

        // multi-buffer material index map
        // we save an indirection array from material order id (the order materials appear
        // in the matset) to actual material id. Not all material sets are numbered from
        // 0 to N-1, so we must support this case.
        m_internal_data["multi_mat_idx_map"].set(DataType::index_t(m_num_mats));
        m_internal_multi_mat_idx_map = m_internal_data["multi_mat_idx_map"].value();

        if (nullptr != specset)
        {
            // number of material species map
            // we save an array from the material order id (the order materials appear
            // in the matset) to the number of species for that material.
            m_internal_data["nmatspec"].set(DataType::index_t(m_num_mats));
            m_internal_nmatspec = m_internal_data["nmatspec"].value();

            // we save an additional offsets array so we can quickly get the index
            // of our desired species array since we are flattening:
            // e.g.
            //    mat1:
            //       spec1:
            //       spec2:
            //       spec3:
            //    mat2:
            //       spec4:
            //       spec5:
            // becomes
            //    [spec1, spec2, spec3, spec4, spec5]
            // then the nmatspec array is
            //    [3, 2]
            // and the nmatspec offsets array is
            //    [0, 3]
            m_internal_data["nmatspec_offsets"].set(DataType::index_t(m_num_mats));
            m_internal_nmatspec_offsets = m_internal_data["nmatspec_offsets"].value();
        }

        index_t nmatspec_offset = 0;

        // now we loop over materials, saving relevant information as we go
        // we are careful to loop over materials in the order of the matset,
        // as fields and specsets need not have the same material order
        for (index_t mat_idx = 0; mat_idx < m_num_mats; mat_idx ++)
        {
            const Node &matmap_entry = material_map.child(mat_idx);
            const std::string matname = matmap_entry.name();
            const index_t mat_id = matmap_entry.to_index_t();

            // save material map entry
            m_internal_multi_mat_idx_map.set(mat_idx, mat_id);

            // save volume fraction array
            if (matset["volume_fractions"].has_child(matname))
            {
                m_multi_vol_fracs.push_back(matset["volume_fractions"][matname].value());
            }
            else
            {
                m_multi_vol_fracs.push_back(m_internal_data["zeroes"].value());
            }

            if (nullptr != field)
            {
                // save matset values array
                if (field->fetch_existing("matset_values").has_child(matname))
                {
                    m_multi_mset_vals.push_back(field->fetch_existing("matset_values")[matname].value());
                }
                else
                {
                    m_multi_mset_vals.push_back(m_internal_data["zeroes"].value());
                }
            }

            if (nullptr != specset)
            {
                // save nmatspec value
                index_t nmatspec = 0;
                if ((*specset)["matset_values"].has_child(matname))
                {
                    nmatspec = (*specset)["matset_values"][matname].number_of_children();
                }
                m_internal_nmatspec.set(mat_idx, nmatspec);

                // save nmatspec offset value
                m_internal_nmatspec_offsets.set(mat_idx, nmatspec_offset);
                nmatspec_offset += nmatspec;

                for (index_t spec_idx = 0; spec_idx < nmatspec; spec_idx ++)
                {
                    if ((*specset)["matset_values"].has_child(matname))
                    {
                        m_multi_mass_fracs.push_back((*specset)["matset_values"][matname].child(spec_idx).value());
                    }
                    else
                    {
                        m_multi_mass_fracs.push_back(m_internal_data["zeroes"].value());
                    }
                }
            }
        }

        // multi-buffer by element (full)
        if (m_is_element_dominant)
        {
            m_get_mat_id       = &MatsetAccessor::get_full_mat_id;
            m_get_mat_order_id = &MatsetAccessor::get_full_mat_order_id;
            m_get_elem_id      = &MatsetAccessor::get_full_elem_id;
            m_get_vol_frac     = &MatsetAccessor::get_full_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_full_mset_val;
            }
            if (nullptr != specset)
            {
                m_get_mass_frac = &MatsetAccessor::get_full_mass_frac;
                m_get_nspec_for_mat = &MatsetAccessor::get_full_nspec_for_mat;
            }
        }
        // multi-buffer by material (sparse by material)
        else
        {
            for (index_t mat_idx = 0; mat_idx < m_num_mats; mat_idx ++)
            {
                const std::string matname = material_map.child(mat_idx).name();
                if (matset["element_ids"].has_child(matname))
                {
                    m_sbm_elem_ids.push_back(matset["element_ids"][matname].value());
                }
                else
                {
                    m_sbm_elem_ids.push_back(m_internal_data["zeroes"].value());
                }
            }

            m_get_mat_id       = &MatsetAccessor::get_sbm_mat_id;
            m_get_mat_order_id = &MatsetAccessor::get_sbm_mat_order_id;
            m_get_elem_id      = &MatsetAccessor::get_sbm_elem_id;
            m_get_vol_frac     = &MatsetAccessor::get_sbm_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_sbm_mset_val;
            }
            m_get_nelems_for_mat = &MatsetAccessor::get_sbm_nelems_for_mat;
            if (nullptr != specset)
            {
                m_get_mass_frac = &MatsetAccessor::get_sbm_mass_frac;
                m_get_nspec_for_mat = &MatsetAccessor::get_sbm_nspec_for_mat;
            }
        }
    }
}

//-----------------------------------------------------------------------------
//
// -- getters for multi-buffer by element (full) matsets --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_full_mat_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return m_internal_multi_mat_idx_map[mat_idx];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_full_mat_order_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return mat_idx;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_full_elem_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) mat_idx;
    return elem_idx;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_full_vol_frac(const index_t elem_idx, const index_t mat_idx) const
{
    return m_multi_vol_fracs[mat_idx][elem_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_full_mset_val(const index_t elem_idx, const index_t mat_idx) const
{
    return m_multi_mset_vals[mat_idx][elem_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_full_mass_frac(const index_t elem_idx,
                                   const index_t mat_idx,
                                   const index_t spec_idx) const
{
    const index_t spec_data_index = m_internal_nmatspec_offsets[mat_idx] + spec_idx;
    return m_multi_mass_fracs[spec_data_index][elem_idx];
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_full_nspec_for_mat(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return m_internal_nmatspec[mat_idx];
}

//-----------------------------------------------------------------------------
//
// -- getters for multi-buffer by material (sparse by material) matsets --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbm_mat_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return m_internal_multi_mat_idx_map[mat_idx];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbm_mat_order_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return mat_idx;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbm_elem_id(const index_t elem_idx, const index_t mat_idx) const
{
    return m_sbm_elem_ids[mat_idx][elem_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbm_vol_frac(const index_t elem_idx, const index_t mat_idx) const
{
    return m_multi_vol_fracs[mat_idx][elem_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbm_mset_val(const index_t elem_idx, const index_t mat_idx) const
{
    return m_multi_mset_vals[mat_idx][elem_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbm_mass_frac(const index_t elem_idx,
                                  const index_t mat_idx,
                                  const index_t spec_idx) const
{
    const index_t spec_data_index = m_internal_nmatspec_offsets[mat_idx] + spec_idx;
    return m_multi_mass_fracs[spec_data_index][elem_idx];
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbm_nelems_for_mat(const index_t mat_idx) const
{
    return m_sbm_elem_ids[mat_idx].number_of_elements();
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbm_nspec_for_mat(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    return m_internal_nmatspec[mat_idx];
}

//-----------------------------------------------------------------------------
//
// -- getters for uni-buffer by element (sparse by element) matsets --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbe_mat_id(const index_t elem_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    return m_sbe_material_ids[data_index];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbe_mat_order_id(const index_t elem_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    return m_internal_sbe_mat_order_ids[data_index];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbe_elem_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) mat_idx;
    return elem_idx;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbe_vol_frac(const index_t elem_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    return m_sbe_vol_fracs[data_index];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbe_mset_val(const index_t elem_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    return m_sbe_mset_vals[data_index];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbe_mass_frac(const index_t elem_idx,
                                  const index_t mat_idx,
                                  const index_t spec_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    const index_t nmatspec_offset = m_internal_nmatspec_offsets[data_index];

    // We need an index that is between 0 and the number of species in this element.
    // The way to calculate this is to add out num material species offset
    // with the current species value index, which ranges between 0 and the
    // number of species for this material.
    const index_t local_spec_id = nmatspec_offset + spec_idx;

    // Now we can provide the one (element id) to many (species in element)
    // relation with our element id and the local species index, which is
    // an index into the number of species in this element. The result is
    // an index into the "matset_values", which store species mass fractions.
    const index_t spec_mf_idx = m_sbe_specset_o2m_idx.index(elem_idx, local_spec_id);

    // fetch the species mass fraction
    return m_sbe_mass_fracs[spec_mf_idx];
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbe_nmats_for_elem(const index_t elem_idx) const
{
    return m_sbe_o2m_idx.size(elem_idx);
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbe_nspec_for_mat(const index_t elem_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(elem_idx, mat_idx);
    return m_internal_nmatspec[data_index];
}

//-----------------------------------------------------------------------------
//
// -- getters for the error case --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_mat_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch mat_id from material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_mat_order_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch mat_order_id from material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_elem_id(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch elem_id from material set.");
    return 0;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_error_vol_frac(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch vol_frac from material set.");
    return 0.0;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_error_mset_val(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch mset_val from field.");
    return 0.0;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_error_mass_frac(const index_t elem_idx,
                                    const index_t mat_idx,
                                    const index_t spec_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    (void) spec_idx;
    CONDUIT_ERROR("Impossible to fetch mass_frac from specset.");
    return 0.0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_nmats_for_elem(const index_t elem_idx) const
{
    (void) elem_idx;
    CONDUIT_ERROR("Impossible to fetch number of materials for element from "
                  "non-sparse by element material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_nelems_for_mat(const index_t mat_idx) const
{
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch number of elements for a material from "
                  "non-sparse by material material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_nspec_for_mat(const index_t elem_idx, const index_t mat_idx) const
{
    (void) elem_idx;
    (void) mat_idx;
    // CONDUIT_ERROR("Impossible to fetch number of species for a material from "
    //               "specset.");
    // no need to error in this case, as we wish to support looping over 
    // elements/materials and then species in the case that species
    // do not exist. Then loops can be more general with minimal performance
    // costs.
    return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// End MatsetAccessor
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
