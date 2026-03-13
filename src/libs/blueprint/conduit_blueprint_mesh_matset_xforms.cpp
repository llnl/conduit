// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_matset_xforms.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// std lib includes
#include <algorithm>
#include <cmath>
#include <string>
#include <map>
#include <vector>

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_blueprint_mesh.hpp"
#include "conduit_blueprint_mesh_utils.hpp"
#include "conduit_blueprint_o2mrelation.hpp"
#include "conduit_blueprint_o2mrelation_iterator.hpp"
#include "conduit_blueprint_o2mrelation_index.hpp"
#include "conduit_blueprint_mesh_matset_accessor.hpp"

using namespace conduit;
// access conduit blueprint mesh utilities
namespace bputils = conduit::blueprint::mesh::utils;
// access one-to-many index types
namespace o2mrelation = conduit::blueprint::o2mrelation;
// access material sets, material field data, and species sets
using MatsetAccessor = conduit::blueprint::mesh::matset::MatsetAccessor;

//-----------------------------------------------------------------------------
// -- begin conduit --
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
// -- begin conduit::blueprint::mesh::matset::detail --
//-----------------------------------------------------------------------------
namespace detail
{
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <class ForEachValue>
void
walk_matset_value_by_element(const MatsetAccessor &m_acc,
                             ForEachValue &&for_each_value,
                             const float64 epsilon = CONDUIT_EPSILON)
{
    auto for_each_element = [](const index_t elem_idx,
                               const index_t nmats)
    {
        (void) elem_idx;
        (void) nmats;
    };
    walk_matset_by_element(m_acc,
                           for_each_value,
                           for_each_element,
                           epsilon);
}

//-----------------------------------------------------------------------------
template <class ForEachValue, class ForEachElement>
void
walk_matset_by_element(const MatsetAccessor &m_acc,
                       ForEachValue &&for_each_value,
                       ForEachElement &&for_each_element,
                       const float64 epsilon = CONDUIT_EPSILON)
{
    if (! m_acc.is_element_dominant())
    {
        CONDUIT_ERROR("Walking by element is only supported for element-dominant material sets.");
    }

    const index_t num_elems = m_acc.num_elems();

    // full
    if (m_acc.is_multi_buffer())
    {
        const index_t nmats = m_acc.num_mats();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            index_t nmats_in_elem = 0;
            for (index_t mat_idx = 0; mat_idx < nmats; mat_idx ++)
            {
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                if (vol_frac > epsilon)
                {
                    // elem_idx is an index over all elements
                    // mat_idx is an index over all materials
                    // nmats_in_elem is running count of materials in the current zone
                    for_each_value(elem_idx, mat_idx, nmats_in_elem);
                    nmats_in_elem ++;
                }
            }
            for_each_element(elem_idx, nmats_in_elem);
        }
    }
    // sparse by element
    else
    {
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            const index_t nmats_in_elem = m_acc.num_mats_for_elem(elem_idx);
            for (index_t mat_idx = 0; mat_idx < nmats_in_elem; mat_idx ++)
            {
                // elem_idx is an index over all elements
                // mat_idx is an index over all materials in the current zone
                // we pass it twice because it is also the running count of materials
                // in the current zone
                for_each_value(elem_idx, mat_idx, mat_idx);
            }
            for_each_element(elem_idx, nmats_in_elem);
        }
    }
}

//-----------------------------------------------------------------------------
template <class ForEachSpeciesValue, class ForEachValue, class ForEachElement>
void
walk_matset_species_by_element(const MatsetAccessor &m_acc,
                               ForEachSpeciesValue &&for_each_species_value,
                               ForEachValue &&for_each_value,
                               ForEachElement &&for_each_element,
                               const float64 epsilon = CONDUIT_EPSILON)
{
    if (! m_acc.is_element_dominant())
    {
        CONDUIT_ERROR("Walking by element is only supported for element-dominant material sets.");
    }

    const index_t num_elems = m_acc.num_elems();

    // full
    if (m_acc.is_multi_buffer())
    {
        const index_t nmats = m_acc.num_mats();
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            index_t nmats_in_elem = 0;
            for (index_t mat_idx = 0; mat_idx < nmats; mat_idx ++)
            {
                const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                if (vol_frac > epsilon)
                {
                    const index_t num_spec_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                    for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                    {
                        for_each_species_value(elem_idx, mat_idx, spec_idx);
                    }

                    // elem_idx is an index over all elements
                    // mat_idx is an index over all materials
                    // nmats_in_elem is running count of materials in the current zone
                    for_each_value(elem_idx, mat_idx, nmats_in_elem);
                    nmats_in_elem ++;
                }
            }
            for_each_element(elem_idx, nmats_in_elem);
        }
    }
    // sparse by element
    else
    {
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            const index_t nmats_in_elem = m_acc.num_mats_for_elem(elem_idx);
            for (index_t mat_idx = 0; mat_idx < nmats_in_elem; mat_idx ++)
            {
                const index_t num_spec_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                {
                    for_each_species_value(elem_idx, mat_idx, spec_idx);
                }

                // elem_idx is an index over all elements
                // mat_idx is an index over all materials in the current zone
                // we pass it twice because it is also the running count of materials
                // in the current zone
                for_each_value(elem_idx, mat_idx, mat_idx);
            }
            for_each_element(elem_idx, nmats_in_elem);
        }
    }
}

//-----------------------------------------------------------------------------
template <class ForEachValue>
void
walk_matset_value_by_material(const MatsetAccessor &m_acc,
                              ForEachValue &&for_each_value,
                              const float64 epsilon = CONDUIT_EPSILON)
{
    auto for_each_material = [](const index_t mat_idx,
                                const index_t num_elems_for_mat)
    {
        (void) mat_idx;
        (void) num_elems_for_mat;
    };
    walk_matset_by_material(m_acc,
                            for_each_value,
                            for_each_material,
                            epsilon);
}

//-----------------------------------------------------------------------------
template <class ForEachValue, class ForEachMaterial>
void
walk_matset_by_material(const MatsetAccessor &m_acc,
                        ForEachValue &&for_each_value,
                        ForEachMaterial &&for_each_material,
                        const float64 epsilon = CONDUIT_EPSILON)
{
    const index_t num_materials = m_acc.num_mats();

    if (m_acc.is_element_dominant())
    {
        // elem-dom multi-buffer "full"
        if (m_acc.is_multi_buffer())
        {
            // we *can* walk this elem-dom representation by material, and sometimes
            // we have to. But it is not very efficient.

            const index_t num_zones = m_acc.num_elems();
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                index_t num_elems_for_mat = 0;
                for (index_t elem_idx = 0; elem_idx < num_zones; elem_idx ++)
                {
                    const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                    if (vol_frac > epsilon)
                    {
                        // elem_idx is an index over all elements
                        // mat_idx is an index over all materials
                        // num_elems_for_mat is running count of elements for the current material
                        for_each_value(mat_idx, elem_idx, num_elems_for_mat);
                        num_elems_for_mat ++;
                    }
                }
                for_each_material(mat_idx, num_elems_for_mat);
            }
        }
        // elem-dom uni-buffer "sparse by element"
        else
        {
            CONDUIT_ERROR("Walking by material is not supported for element-dominant uni-buffer material sets.");
        }
    }
    else
    {
        // mat-dom multi-buffer "sparse by material"
        if (m_acc.is_multi_buffer())
        {
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (int mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                const index_t num_elems_for_mat = m_acc.num_elems_for_mat(mat_idx);
                for (index_t elem_idx = 0; elem_idx < num_elems_for_mat; elem_idx ++)
                {
                    // elem_idx is an index over all elements the current material is in
                    // mat_idx is an index over all materials
                    // we pass elem_idx twice because it is also the running count of
                    // elements for the current material
                    for_each_value(mat_idx, elem_idx, elem_idx);
                }
                for_each_material(mat_idx, num_elems_for_mat);
            }
        }
        // mat-dom uni-buffer - currently unsupported
        else
        {
            CONDUIT_ERROR("material-dominant uni-buffer material set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
template <class ForEachSpeciesValue, class ForEachValue, class ForEachMaterial>
void
walk_matset_species_by_material(const MatsetAccessor &m_acc,
                                ForEachSpeciesValue &&for_each_species_value,
                                ForEachValue &&for_each_value,
                                ForEachMaterial &&for_each_material,
                                const float64 epsilon = CONDUIT_EPSILON)
{
    const index_t num_materials = m_acc.num_mats();

    if (m_acc.is_element_dominant())
    {
        // elem-dom multi-buffer "full"
        if (m_acc.is_multi_buffer())
        {
            // we *can* walk this elem-dom representation by material, and sometimes
            // we have to. But it is not very efficient.

            const index_t num_zones = m_acc.num_elems();
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                index_t num_elems_for_mat = 0;
                for (index_t elem_idx = 0; elem_idx < num_zones; elem_idx ++)
                {
                    const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                    if (vol_frac > epsilon)
                    {
                        const index_t num_spec_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                        for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                        {
                            for_each_species_value(mat_idx, elem_idx, spec_idx);
                        }

                        // elem_idx is an index over all elements
                        // mat_idx is an index over all materials
                        // num_elems_for_mat is running count of elements for the current material
                        for_each_value(mat_idx, elem_idx, num_elems_for_mat);
                        num_elems_for_mat ++;
                    }
                }
                for_each_material(mat_idx, num_elems_for_mat);
            }
        }
        // elem-dom uni-buffer "sparse by element"
        else
        {
            CONDUIT_ERROR("Walking by material is not supported for element-dominant uni-buffer material sets.");
        }
    }
    else
    {
        // mat-dom multi-buffer "sparse by material"
        if (m_acc.is_multi_buffer())
        {
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (int mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                const index_t num_elems_for_mat = m_acc.num_elems_for_mat(mat_idx);
                for (index_t elem_idx = 0; elem_idx < num_elems_for_mat; elem_idx ++)
                {
                    const index_t num_spec_for_mat = m_acc.num_spec_for_mat(elem_idx, mat_idx);
                    for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                    {
                        for_each_species_value(mat_idx, elem_idx, spec_idx);
                    }
                    
                    // elem_idx is an index over all elements the current material is in
                    // mat_idx is an index over all materials
                    // we pass elem_idx twice because it is also the running count of
                    // elements for the current material
                    for_each_value(mat_idx, elem_idx, elem_idx);
                }
                for_each_material(mat_idx, num_elems_for_mat);
            }
        }
        // mat-dom uni-buffer - currently unsupported
        else
        {
            CONDUIT_ERROR("material-dominant uni-buffer material set is unsupported.");
        }
    }
}

//-------------------------------------------------------------------------
// helper for multi-buffer material sets that do not have 
// material maps.
void
create_material_map(const conduit::Node &matset,
                    conduit::Node &material_map)
{
    // We must be multi-buffer, so we can assume we have a 
    // "volume_fractions" child that is an object.
    const std::vector<std::string> &matnames = matset["volume_fractions"].child_names();
    index_t mat_id = 0;
    for (const auto &matname : matnames)
    {
        material_map[matname].set(mat_id);
        mat_id ++;
    }
}

//-----------------------------------------------------------------------------
void
store_material_specset_data_for_zone_to_silo_arrays(
    const index_t &num_mats_in_zone,
    const index_t_array &local_material_ids,
    const float64_array &local_volume_fractions,
    const std::map<index_t, index_t> &mat_id_to_array_index,
    const index_t_accessor &nmatspec,
    const index_t zone_id,
    index_t_array &matlist,
    std::vector<float64> &mix_vf,
    std::vector<index_t> &mix_mat,
    std::vector<int> &mix_next,
    index_t_array &speclist,
    std::vector<int> &mix_spec,
    int &current_position,
    int &current_spec_position)
{
    // if zone is clean
    if (1 == num_mats_in_zone)
    {
        const index_t matno = local_material_ids[0];
        matlist[zone_id] = matno;

        // I can use the material number to determine which part of the speclist to index into
        const index_t mat_index = mat_id_to_array_index.at(matno);
        const index_t num_species_for_this_material = nmatspec[mat_index];
        if (num_species_for_this_material == 1)
        {
            // This is an optimization for if the material has only one
            // species. See MIR.C in VisIt in the MIR::SpeciesSelect() 
            // function to see how this optimization is used.
            speclist[zone_id] = 0;
        }
        else
        {
            // Either there are multiple species for this material or there 
            // are none. If there are none, then the value computed here
            // will ultimately not be used by Silo readers. There must be 
            // a value here though even when there are no species for the
            // material because we must have entries in the different silo
            // species arrays for each material.
            speclist[zone_id] = current_spec_position;
        }
        current_spec_position += num_species_for_this_material;
    }
    // if zone is mixed
    else
    {
        // a negated 1-index into the mixed arrays
        const int matlist_entry = -1 * current_position;
        matlist[zone_id] = matlist_entry;

        // We save the negated 1-index into the mix_spec array
        // (same as the matlist array)
        speclist[zone_id] = matlist_entry;

        // for mixed zones, the numbers in the speclist are negated 1-indices into
        // the silo mixed data arrays. To turn them into zero-indices, we must add
        // 1 and negate the result. Example:
        // indices: -1 -2 -3 -4 ...
        // become:   0  1  2  3 ...

        for (index_t mat = 0; mat < num_mats_in_zone; mat ++)
        {
            const index_t curr_mat_id = local_material_ids[mat];
            const index_t mat_index = mat_id_to_array_index.at(curr_mat_id);
            const float64 curr_vol_frac = local_volume_fractions[mat];

            mix_vf.push_back(curr_vol_frac);
            mix_mat.push_back(curr_mat_id);

            current_position ++;
            if (mat + 1 == num_mats_in_zone)
            {
                mix_next.push_back(0);
            }
            else
            {
                mix_next.push_back(current_position);
            }

            const index_t num_species_for_this_material = nmatspec[mat_index];
            if (num_species_for_this_material == 1)
            {
                // This is an optimization for if the material has only one
                // species. See MIR.C in VisIt in the MIR::SpeciesSelect() 
                // function to see how this optimization is used.
                mix_spec.push_back(0);
            }
            else
            {
                // Either there are multiple species for this material or there 
                // are none. If there are none, then the value computed here
                // will ultimately not be used by Silo readers. There must be 
                // a value here though even when there are no species for the
                // material because we must have entries in the different silo
                // species arrays for each material.
                mix_spec.push_back(current_spec_position);
            }
            current_spec_position += num_species_for_this_material;
        }
    }
}

//-----------------------------------------------------------------------------
void
store_material_field_data_for_zone_to_silo_arrays(
    const index_t &num_mats_in_zone,
    const index_t_array &local_material_ids,
    const float64_array &local_volume_fractions,
    const float64_array &local_matset_values,
    const index_t zone_id,
    index_t_array &matlist,
    std::vector<float64> &mix_vf,
    std::vector<index_t> &mix_mat,
    std::vector<int> &mix_next,
    std::vector<float64> &field_mixvar_values,
    int &current_position)
{
    // if zone is clean
    if (1 == num_mats_in_zone)
    {
        matlist[zone_id] = local_material_ids[0];
    }
    else
    {
        // a negated 1-index into the mixed arrays
        matlist[zone_id] = -1 * current_position;

        for (int mat = 0; mat < num_mats_in_zone; mat ++)
        {
            const index_t curr_mat_id = local_material_ids[mat];
            const float64 curr_vol_frac = local_volume_fractions[mat];
            const float64 curr_mset_val = local_matset_values[mat];

            mix_vf.push_back(curr_vol_frac);
            mix_mat.push_back(curr_mat_id);
            field_mixvar_values.push_back(curr_mset_val);

            current_position ++;
            if (mat + 1 == num_mats_in_zone)
            {
                mix_next.push_back(0);
            }
            else
            {
                mix_next.push_back(current_position);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
store_material_data_for_zone_to_silo_arrays(
    const index_t &num_mats_in_zone,
    const index_t_array &local_material_ids,
    const float64_array &local_volume_fractions,
    const index_t zone_id,
    index_t_array &matlist,
    std::vector<float64> &mix_vf,
    std::vector<index_t> &mix_mat,
    std::vector<int> &mix_next,
    int &current_position)
{
    // if zone is clean
    if (1 == num_mats_in_zone)
    {
        matlist[zone_id] = local_material_ids[0];
    }
    else
    {
        // a negated 1-index into the mixed arrays
        matlist[zone_id] = -1 * current_position;

        for (index_t mat = 0; mat < num_mats_in_zone; mat ++)
        {
            const index_t curr_mat_id = local_material_ids[mat];
            const float64 curr_vol_frac = local_volume_fractions[mat];

            mix_vf.push_back(curr_vol_frac);
            mix_mat.push_back(curr_mat_id);

            current_position ++;
            if (mat + 1 == num_mats_in_zone)
            {
                mix_next.push_back(0);
            }
            else
            {
                mix_next.push_back(current_position);
            }
        }
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Single implementation that supports the case where just matset
// is passed, the case where the field+matset is passed, and the case where the
// specset+matset is passed.
//
// We smooth this out for the API by providing the non detail variants.
//-----------------------------------------------------------------------------
void
to_silo(const conduit::Node &matset,
        const conduit::Node &field,
        const conduit::Node &specset,
        conduit::Node &dest,
        const float64 epsilon)
{
    // output includes the following:
    // for matsets:
    //  - topology
    //  - material_map
    //  - matlist
    //  - mix_next
    //  - mix_mat
    //  - mix_vf
    //  - buffer_style
    //  - dominance
    // for fields:
    //  - field_mixvar_values
    //  - field_values (optional)
    // for specsets:
    //  - nmatspec
    //  - specnames
    //  - speclist
    //  - nmat
    //  - nspecies_mf
    //  - species_mf
    //  - mix_spec
    //  - mixlen

    //
    // make sure output is empty to start
    //
    dest.reset();

    //
    // set output topology
    //
    dest["topology"].set(matset["topology"]);

    //
    // note buffer style and dominance for downstream consumers
    //
    const bool multi_buffer = is_multi_buffer(matset);
    const bool element_dominant = is_element_dominant(matset);
    if (multi_buffer)
    {
        dest["buffer_style"] = "multi";
    }
    else
    {
        dest["buffer_style"] = "uni";
    }
    if (element_dominant)
    {
        dest["dominance"] = "element";
    }
    else
    {
        dest["dominance"] = "material";
    }

    //
    // determine if we are transforming a field as well
    //
    const bool transform_field = field.has_child("matset_values");

    //
    // determine if we are transforming a specset as well
    //
    const bool transform_specset = specset.has_child("matset_values");

    // we can't transform both at once
    if (transform_field && transform_specset)
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_silo"
                      " cannot transform both field and specset at the same time.");
    }

    //
    // fetch or create the material map
    //
    Node &material_map = dest["material_map"];
    create_or_copy_material_map(matset, material_map);

    // We declare a map that is only used for writing specsets.
    // Maps actual material numbers to indicies into the material map
    // We need this map so that, no matter what material numbers we see,
    // we can figure out their order in the material map for when we calculate
    // species indices.
    std::map<index_t, index_t> mat_id_to_array_index;

    // we need the number of materials
    const index_t nmat = count_materials_from_matset(matset);

    //
    // specset preprocessing:
    // 1. fetch the number of materials for the specset output
    // 2. create and fill nmatspec for the specset output
    // 3. create and fill specnames for the specset output
    //
    if (transform_specset)
    {
        const index_t nmat_specset = conduit::blueprint::mesh::specset::count_materials_from_specset(specset);
        CONDUIT_ASSERT(nmat >= nmat_specset, "blueprint::mesh::specset::to_silo number of materials in the matset "
                                             "must be greater than or equal to the number of materials in the specset.");
        
        // number of materials
        dest["nmat"] = nmat;

        // create nmatspec
        dest["nmatspec"].set(DataType::index_t(nmat));
        index_t_array nmatspec = dest["nmatspec"].value();

        CONDUIT_ASSERT(nmat == material_map.number_of_children(),
                       "blueprint::mesh::specset::to_silo mismatch between number of materials "
                       "and materials in the material map.");

        Node &dest_specnames = dest["specnames"];

        for (index_t matmap_index = 0; matmap_index < nmat; matmap_index ++)
        {
            const Node &matmap_entry = material_map.child(matmap_index);
            const std::string matname = matmap_entry.name();

            // save material id correspondence with array position
            mat_id_to_array_index[matmap_entry.to_index_t()] = matmap_index;

            // get the number of species for this material
            const int num_species_for_this_material = 
                conduit::blueprint::mesh::specset::get_num_species_for_material(specset, matname);

            // is this material present in the specset?
            if (num_species_for_this_material > 0)
            {
                // save the number of species for this material in the output
                nmatspec[matmap_index] = num_species_for_this_material;

                // get the specie names for this material and add to the specnames.
                // the specnames array is the length of the sum of the dest_nmatspec array
                // so for all materials with species, the species names will appear
                // in this list in order.
                NodeConstIterator spec_itr;
                if (blueprint::mesh::specset::is_multi_buffer(specset))
                {
                    spec_itr = specset["matset_values"][matname].children();
                }
                else
                {
                    spec_itr = specset["species_names"][matname].children();
                }
                while (spec_itr.has_next())
                {
                    spec_itr.next();
                    const std::string specname = spec_itr.name();
                    dest_specnames.append().set(specname);
                }
            }
            else
            {
                // if this material has no species, then we set to zero.
                nmatspec[matmap_index] = 0;
            }
        }
    }

    //
    // copy field values if they are present
    //
    if (transform_field)
    {
        if (field.has_child("values"))
        {
            dest["field_values"].set(field["values"]);
        }
    }

    //
    // get the number of zones in the material set
    //
    const int num_zones = count_zones_from_matset(matset);

    //
    // create destination silo arrays
    //

    // for matsets
    dest["matlist"].set(DataType::index_t(num_zones));
    index_t_array matlist = dest["matlist"].value();
    std::vector<float64> mix_vf;
    std::vector<index_t> mix_mat;
    std::vector<int> mix_next;
    
    // for fields
    std::vector<float64> field_mixvar_values;

    // for specsets
    index_t_accessor nmatspec; // we need to read from this; it has already been created
    index_t_array speclist;
    if (transform_specset)
    {
        nmatspec = dest["nmatspec"].value();
        dest["speclist"].set(DataType::index_t(num_zones));
        speclist = dest["speclist"].value();
    }
    // The function silo_write_specset() in conduit_relay_io_silo.cpp
    // depends on this being a float64. If we change this here,
    // we must also change it there.
    std::vector<float64> species_mf;
    std::vector<int> mix_spec;

    //
    // create a 1-index into the mixed arrays for bookkeeping
    //
    int current_position = 1;
    // TODO if we pre-calculate the number of materials in each zone, we can
    // get away from using this running sum and make this more GPU-friendly.

    //
    // create a 1-index into the species mass fractions array for bookkeeping
    //
    int current_spec_position = 1;
    // TODO we can precalculate the number of species in each zone and get
    // away from using this running sum and make this more GPU-friendly.
    // We could also put values in for every species for every material
    // in each zone even if the zone does not contain each material. Then
    // we can algorithmically find out where each index should be, but we 
    // waste a lot of space.

    //
    // Now we have a switchyard for choosing which case we are in.
    // While there is shared logic, we need a separate case for each of the 
    // matset representations, and we additionally need a case for
    // if we are doing fields or not. We use our matset/field/specset walkers
    // to walk the data structures and write the results to the silo arrays.
    // 
    // if we are working with fields
    if (transform_field)
    {
        if (element_dominant)
        {
            Node n;
            n["local_material_ids"].set(DataType::index_t(nmat));
            n["local_volume_fractions"].set(DataType::float64(nmat));
            n["local_matset_values"].set(DataType::float64(nmat));
            index_t_array local_material_ids = n["local_material_ids"].value();
            float64_array local_volume_fractions = n["local_volume_fractions"].value();
            float64_array local_matset_values = n["local_matset_values"].value();

            MatsetAccessor m_acc = MatsetAccessor(matset, field);

            // we need to gather info from each value for the zones
            auto for_each_value = [&](const index_t zone_idx,
                                      const index_t mat_idx,
                                      const index_t curr_material_index)
            {
                local_material_ids[curr_material_index] = m_acc.get_mat_id(zone_idx, mat_idx);
                local_volume_fractions[curr_material_index] = m_acc.get_vol_frac(zone_idx, mat_idx);
                local_matset_values[curr_material_index] = m_acc.get_mset_val(zone_idx, mat_idx);
            };

            auto for_each_element = [&](const index_t elem_idx,
                                        const index_t num_mats_in_zone)
            {
                store_material_field_data_for_zone_to_silo_arrays(
                    num_mats_in_zone, local_material_ids, local_volume_fractions,
                    local_matset_values, elem_idx, matlist, mix_vf, mix_mat, mix_next,
                    field_mixvar_values, current_position);
            };

            walk_matset_by_element(m_acc, for_each_value, for_each_element, epsilon);
        }
        else // material dominant
        {
            //
            // create an intermediate representation
            // we could do this for all matset types, but it is less efficient
            // it is required for material dominant matsets
            //
            // for each zone, the material ids of the materials in that zone
            std::vector<std::vector<index_t>> material_ids(num_zones);
            // for each zone, the volume fractions of the materials in that zone
            std::vector<std::vector<float64>> vol_fracs(num_zones);
            // for each zone, the matset vals of the field in that zone
            std::vector<std::vector<float64>> mset_vals(num_zones);

            MatsetAccessor m_acc = MatsetAccessor(matset, field);

            auto for_each_value = [&](const index_t mat_idx,
                                      const index_t zone_idx,
                                      const index_t)
            {
                const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
                material_ids[real_zone_id].push_back(m_acc.get_mat_id(zone_idx, mat_idx));
                vol_fracs[real_zone_id].push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
                mset_vals[real_zone_id].push_back(m_acc.get_mset_val(zone_idx, mat_idx));
            };

            walk_matset_value_by_material(m_acc, for_each_value, epsilon);

            Node n;
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                const index_t num_mats_in_zone = static_cast<index_t>(material_ids[zone_id].size());
                n["local_material_ids"].set_external(material_ids[zone_id]);
                n["local_volume_fractions"].set_external(vol_fracs[zone_id]);
                n["local_matset_values"].set_external(mset_vals[zone_id]);
                index_t_array local_material_ids = n["local_material_ids"].value();
                float64_array local_volume_fractions = n["local_volume_fractions"].value();
                float64_array local_matset_values = n["local_matset_values"].value();

                store_material_field_data_for_zone_to_silo_arrays(
                    num_mats_in_zone, local_material_ids, local_volume_fractions,
                    local_matset_values, zone_id, matlist, mix_vf, mix_mat, mix_next,
                    field_mixvar_values, current_position);
            }
        }
    }
    else if (transform_specset)
    {
        if (element_dominant)
        {
            Node n;
            n["local_material_ids"].set(DataType::index_t(nmat));
            n["local_volume_fractions"].set(DataType::float64(nmat));
            index_t_array local_material_ids = n["local_material_ids"].value();
            float64_array local_volume_fractions = n["local_volume_fractions"].value();

            MatsetAccessor m_acc = MatsetAccessor(matset, specset);

            // for each species mass fraction
            auto for_each_species_value = [&](const index_t zone_idx,
                                              const index_t mat_idx,
                                              const index_t spec_idx)
            {
                species_mf.push_back(m_acc.get_mass_frac(zone_idx, mat_idx, spec_idx));
            };

            // we need to gather info from each value for the zones
            auto for_each_value = [&](const index_t zone_idx,
                                      const index_t mat_idx,
                                      const index_t curr_material_index)
            {
                local_material_ids[curr_material_index] = m_acc.get_mat_id(zone_idx, mat_idx);;
                local_volume_fractions[curr_material_index] = m_acc.get_vol_frac(zone_idx, mat_idx);;
            };

            auto for_each_element = [&](const index_t elem_idx,
                                        const index_t num_mats_in_zone)
            {
                store_material_specset_data_for_zone_to_silo_arrays(
                    num_mats_in_zone,
                    local_material_ids,
                    local_volume_fractions,
                    mat_id_to_array_index,
                    nmatspec,
                    elem_idx,
                    matlist,
                    mix_vf,
                    mix_mat,
                    mix_next,
                    speclist,
                    mix_spec,
                    current_position,
                    current_spec_position);
            };

            walk_matset_species_by_element(m_acc,
                                           for_each_species_value,
                                           for_each_value,
                                           for_each_element,
                                           epsilon);
        }
        else // material dominant
        {
            //
            // create an intermediate representation
            // we could do this for all matset types, but it is less efficient
            // it is required for material dominant matsets
            //
            // for each zone, the material ids of the materials in that zone
            std::vector<std::vector<index_t>> material_ids(num_zones);
            // for each zone, the volume fractions of the materials in that zone
            std::vector<std::vector<float64>> vol_fracs(num_zones);
            // num_zones by num_materials mf vals vectors
            std::vector<std::vector<std::vector<float64>>> mf_vals(num_zones, 
                                                                   std::vector<std::vector<float64>>(nmat));

            MatsetAccessor m_acc = MatsetAccessor(matset, specset);

            // for each species mass fraction
            auto for_each_species_value = [&](const index_t mat_idx,
                                              const index_t zone_idx,
                                              const index_t spec_idx)
            {
                const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
                const float64 mf_val = m_acc.get_mass_frac(zone_idx, mat_idx, spec_idx);
                mf_vals[real_zone_id][mat_idx].push_back(mf_val);
            };

            // for each mat_id vol_frac pair
            auto for_each_value = [&](const index_t mat_idx,
                                      const index_t zone_idx,
                                      const index_t)
            {
                const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
                material_ids[real_zone_id].push_back(m_acc.get_mat_id(zone_idx, mat_idx));
                vol_fracs[real_zone_id].push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
            };

            // nothing to do for each material
            auto for_each_material = [](const index_t, const index_t){};

            walk_matset_species_by_material(m_acc,
                                            for_each_species_value,
                                            for_each_value,
                                            for_each_material,
                                            epsilon);

            Node n;
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                const index_t num_mats_in_zone = static_cast<index_t>(material_ids[zone_id].size());
                n["local_material_ids"].set_external(material_ids[zone_id]);
                n["local_volume_fractions"].set_external(vol_fracs[zone_id]);
                index_t_array local_material_ids = n["local_material_ids"].value();
                float64_array local_volume_fractions = n["local_volume_fractions"].value();

                // iterating over all materials, not just the ones in this zone
                for (index_t mat_idx = 0; mat_idx < nmat; mat_idx ++)
                {
                    const index_t nspecs_in_zone_mat = static_cast<index_t>(mf_vals[zone_id][mat_idx].size());
                    if (0 < nspecs_in_zone_mat)
                    {
                        for (const float64 &mf_val : mf_vals[zone_id][mat_idx])
                        {
                            species_mf.push_back(mf_val);
                        }
                    }
                }

                store_material_specset_data_for_zone_to_silo_arrays(
                    num_mats_in_zone,
                    local_material_ids,
                    local_volume_fractions,
                    mat_id_to_array_index,
                    nmatspec,
                    zone_id,
                    matlist,
                    mix_vf,
                    mix_mat,
                    mix_next,
                    speclist,
                    mix_spec,
                    current_position,
                    current_spec_position);
            }
        }
    }
    else
    {
        if (element_dominant)
        {
            Node n;
            n["local_material_ids"].set(DataType::index_t(nmat));
            n["local_volume_fractions"].set(DataType::float64(nmat));
            index_t_array local_material_ids = n["local_material_ids"].value();
            float64_array local_volume_fractions = n["local_volume_fractions"].value();
            
            MatsetAccessor m_acc = MatsetAccessor(matset);

            // we need to gather info from each value for the zones
            auto for_each_value = [&](const index_t zone_idx,
                                      const index_t mat_idx,
                                      const index_t curr_material_index)
            {
                local_material_ids[curr_material_index] = m_acc.get_mat_id(zone_idx, mat_idx);
                local_volume_fractions[curr_material_index] = m_acc.get_vol_frac(zone_idx, mat_idx);
            };

            auto for_each_element = [&](const index_t elem_idx,
                                        const index_t num_mats_in_zone)
            {
                store_material_data_for_zone_to_silo_arrays(
                    num_mats_in_zone, local_material_ids, local_volume_fractions, 
                    elem_idx, matlist, mix_vf, mix_mat, mix_next, current_position);
            };

            walk_matset_by_element(m_acc, for_each_value, for_each_element, epsilon);
        }
        else // material_dominant
        {
            //
            // create an intermediate representation
            // we could do this for all matset types, but it is less efficient
            // it is required for material dominant matsets
            //
            // for each zone, the material ids of the materials in that zone
            std::vector<std::vector<index_t>> material_ids(num_zones);
            // for each zone, the volume fractions of the materials in that zone
            std::vector<std::vector<float64>> vol_fracs(num_zones);

            MatsetAccessor m_acc = MatsetAccessor(matset);

            auto for_each_value = [&](const index_t mat_idx,
                                      const index_t zone_idx,
                                      const index_t)
            {
                const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
                material_ids[real_zone_id].push_back(m_acc.get_mat_id(zone_idx, mat_idx));
                vol_fracs[real_zone_id].push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
            };

            walk_matset_value_by_material(m_acc, for_each_value, epsilon);

            Node n;
            for (int zone_id = 0; zone_id < num_zones; zone_id ++)
            {
                const index_t num_mats_in_zone = static_cast<index_t>(material_ids[zone_id].size());
                n["local_material_ids"].set_external(material_ids[zone_id]);
                n["local_volume_fractions"].set_external(vol_fracs[zone_id]);
                index_t_array local_material_ids = n["local_material_ids"].value();
                float64_array local_volume_fractions = n["local_volume_fractions"].value();

                store_material_data_for_zone_to_silo_arrays(
                    num_mats_in_zone, local_material_ids, local_volume_fractions, 
                    zone_id, matlist, mix_vf, mix_mat, mix_next, current_position);
            }
        }
    }

    //
    // save the results
    //
    dest["matlist"].set(matlist);
    dest["mix_vf"].set(mix_vf);
    dest["mix_mat"].set(mix_mat);
    dest["mix_next"].set(mix_next);
    
    if (transform_field)
    {
        dest["field_mixvar_values"].set(field_mixvar_values);
    }

    if (transform_specset)
    {
        // length of the species_mf array
        dest["nspecies_mf"] = static_cast<int>(species_mf.size());

        // mass fractions of the matspecies in an array of length nspecies_mf
        dest["species_mf"].set(species_mf);

        // array of length mixlen containing indices into the species_mf array
        dest["mix_spec"].set(mix_spec);

        // length of mix_spec array
        dest["mixlen"] = static_cast<int>(mix_spec.size());
    }
}

//-----------------------------------------------------------------------------
// field copy
void
copy_matset_independent_parts_of_field(const conduit::Node &src_field,
                                       const std::string &dest_matset_name,
                                       conduit::Node &dest_field)
{
    // copy over everything except the matset values and matset name
    auto field_child_itr = src_field.children();
    while (field_child_itr.has_next())
    {
        const Node &n_field_info = field_child_itr.next();
        std::string field_child_name = field_child_itr.name();

        if (field_child_name != "matset_values" &&
            field_child_name != "matset")
        {
            dest_field[field_child_name].set(n_field_info);
        }
    }
    dest_field["matset"] = dest_matset_name;
}

//-----------------------------------------------------------------------------
// take the maximum element id
int
determine_num_elems_in_multi_buffer_by_material(const conduit::Node &elem_ids)
{
    int running_max = 0;

    auto eid_itr = elem_ids.children();
    while (eid_itr.has_next())
    {
        const Node &mat_elem_ids = eid_itr.next();
        int64_accessor mat_elem_ids_vals = mat_elem_ids.value();
        const int num_vf = mat_elem_ids_vals.dtype().number_of_elements();
        for (int i = 0; i < num_vf; i ++)
        {
            const int element_id = mat_elem_ids_vals[i];
            running_max = std::max(running_max, element_id + 1);
        }
    }

    return running_max;
}

//-----------------------------------------------------------------------------
template <typename SaveFunc>
void
walk_uni_buffer_by_element_to_multi_buffer_by_element_specset(
    const conduit::Node &src_matset,
    const conduit::Node &src_specset,
    const std::map<int, std::string> &reverse_matmap,
    float64_accessor &matset_values,
    int64_accessor &material_ids,
    SaveFunc save)
{
    // Create one to many index objects to index into the sparse by element 
    // matset and specset.
    auto o2m_matset_idx = o2mrelation::O2MIndex(src_matset);
    auto o2m_specset_idx = o2mrelation::O2MIndex(src_specset);

    const int num_elems = o2m_matset_idx.size();

    // iterate through matset
    for (int elem_id = 0; elem_id < num_elems; elem_id ++)
    {
        // Based on the element id, we can query how many materials there
        // are in the current zone.
        const index_t num_mats_in_zone = o2m_matset_idx.size(elem_id);

        // Each time we step through a material, we need to track how far
        // in the species matset_values we have read for that zone.
        // So if a zone has some number of materials and each have a
        // different number of species, then we need to track how many
        // values we have read for the materials that came before.
        int material_offset = 0;

        // iterate through the materials in this zone
        for (index_t local_mat_id = 0; local_mat_id < num_mats_in_zone; local_mat_id ++)
        {
            // what is the index into the material ids array
            const index_t material_ids_index = o2m_matset_idx.index(elem_id, local_mat_id);

            // what is the real material id
            const int mat_id = material_ids[material_ids_index];

            // fetch the material name
            const std::string &matname = reverse_matmap.at(mat_id);

            // fetch the number of species for this material
            const int num_species_for_this_material = src_specset["species_names"][matname].number_of_children();

            // iterate through each species for the current material
            for (int spec_val_idx = 0; spec_val_idx < num_species_for_this_material; spec_val_idx ++)
            {
                // fetch the species name from the original specset (based on the order 
                // the names appear in the species_names)
                const std::string &specname = src_specset["species_names"][matname].child(spec_val_idx).name();

                // We need an index that is between 0 and the number of species in this zone.
                // The way to calculate this is to add our running sum (material_offset)
                // with the current species value index, which ranges between 0 and the
                // number of species for this material.
                const int local_spec_id = material_offset + spec_val_idx;

                // Now we can provide the one (element id) to many (species in element)
                // relation with our element id and the local species index, which is
                // an index into the number of species in this element. The result is
                // an index into the "matset_values", which store species mass fractions.
                const index_t spec_mf_idx = o2m_specset_idx.index(elem_id, local_spec_id);

                // fetch the species mass fraction
                const float64 val = matset_values[spec_mf_idx];

                // save the species mass fraction in its new home
                save(matname, specname, elem_id, val);
            }

            // we have read num species, now we must move our offset
            material_offset += num_species_for_this_material;
        }
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void
read_from_map_write_out(std::map<std::string, std::vector<T>> &datamap,
                        conduit::Node &destination)
{
    for (auto & mapitem : datamap)
    {
        const std::string &key = mapitem.first;
        const std::vector<T> &data_vector = mapitem.second;

        destination[key].set(data_vector);
    }
}

//-----------------------------------------------------------------------------
// takes sparse by material data and stores it in a map
void
create_sbm_specset_rep(const conduit::Node &elem_id_src,
                       const conduit::Node &values_src,
                       std::map<std::string, std::pair<int64_accessor, std::map<std::string, float64_accessor>>> &sbm_rep)
{
    auto eid_itr = elem_id_src.children();
    while (eid_itr.has_next())
    {
        const Node &mat_elem_ids = eid_itr.next();
        const std::string matname = eid_itr.name();
        sbm_rep[matname].first = mat_elem_ids.value();
    }

    auto val_itr = values_src.children();
    while (val_itr.has_next())
    {
        const Node &mset_vals = val_itr.next();
        const std::string matname = val_itr.name();

        auto spec_itr = mset_vals.children();
        while (spec_itr.has_next())
        {
            const Node &spec_mf = spec_itr.next();
            const std::string specname = spec_itr.name();

            sbm_rep[matname].second[specname] = spec_mf.value();
        }
    }
}

//-----------------------------------------------------------------------------
// venn full -> sparse by element
void
multi_buffer_by_element_to_uni_buffer_by_element_matset(const conduit::Node &src_matset,
                                                        conduit::Node &dest_matset,
                                                        const float64 epsilon)
{
    Node &material_map = dest_matset["material_map"];
    create_or_copy_material_map(src_matset, material_map);

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_elems = m_acc.num_elems();

    std::vector<float64> vol_fracs;
    std::vector<index_t> mat_ids;
    dest_matset["sizes"].set(DataType::index_t(num_elems));
    index_t_array sizes = dest_matset["sizes"].value();
    dest_matset["offsets"].set(DataType::index_t(num_elems));
    index_t_array offsets = dest_matset["offsets"].value();

    index_t offset = 0;
    // we need to gather info from each value for the zones
    auto for_each_value = [&](const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        mat_ids.push_back(m_acc.get_mat_id(zone_idx, mat_idx));
        vol_fracs.push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
    };

    auto for_each_element = [&](const index_t elem_idx,
                                const index_t nmats)
    {
        // save the size and offset information
        sizes[elem_idx] = nmats;
        offsets[elem_idx] = offset;
        offset += nmats;
    };

    walk_matset_by_element(m_acc, for_each_value, for_each_element, epsilon);

    dest_matset["volume_fractions"].set(vol_fracs);
    dest_matset["material_ids"].set(mat_ids);
}

//-----------------------------------------------------------------------------
// venn full -> sparse by element
void
multi_buffer_by_element_to_uni_buffer_by_element_field(const conduit::Node &src_matset,
                                                       const conduit::Node &src_field,
                                                       conduit::Node &dest_field,
                                                       const float64 epsilon)
{
    std::vector<float64> matset_values;

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    // what we will do for each mset_val we encounter
    auto for_each_value = [&](const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        matset_values.push_back(m_acc.get_mset_val(zone_idx, mat_idx));
    };

    walk_matset_value_by_element(m_acc, for_each_value, epsilon);

    dest_field["matset_values"].set(matset_values);
}

//-----------------------------------------------------------------------------
// venn full -> sparse by element
void
multi_buffer_by_element_to_uni_buffer_by_element_specset(const conduit::Node &src_matset,
                                                         const conduit::Node &src_specset,
                                                         conduit::Node &dest_specset,
                                                         const float64 epsilon)
{
    // map from material id to volume fractions in the full representation
    std::map<int, float64_accessor> full_vol_fracs;
    // map from material ids to a map from species ids to matset values in the full representation
    std::map<int, std::map<int, float64_accessor>> full_specset_vals;
    // a map from material id to the number of species that material has
    std::map<int, int> num_species_for_mat;
    
    auto mat_itr = src_matset["volume_fractions"].children();
    auto smat_itr = src_specset["matset_values"].children();
    int mat_idx = 0;
    while (mat_itr.has_next() && smat_itr.has_next())
    {
        const Node &mat_vol_fracs = mat_itr.next();
        const std::string matname = mat_itr.name();

        const Node &specset_vals = smat_itr.next();
        const std::string smatname = smat_itr.name();

        CONDUIT_ASSERT(matname == smatname, "Materials must be ordered the same in "
            "specsets and their matsets.");

        full_vol_fracs[mat_idx] = mat_vol_fracs.value();

        // species for a material
        const std::vector<std::string> &specnames = specset_vals.child_names();

        const int num_species_for_this_material = static_cast<int>(specnames.size());
        num_species_for_mat[mat_idx] = num_species_for_this_material;

        // for each species for this material
        for (int spec_id = 0; spec_id < num_species_for_this_material; spec_id ++)
        {
            const std::string &specname = specnames[spec_id];

            // create an entry in the species names
            dest_specset["species_names"][matname][specname];

            // and store a pointer to the species value
            full_specset_vals[mat_idx][spec_id] = specset_vals[specname].value();
        }

        mat_idx ++;
    }

    const int nmats = mat_idx;

    std::vector<double> matset_values;
    std::vector<int> sizes;
    std::vector<int> offsets;

    const int num_elems = src_matset["volume_fractions"][0].dtype().number_of_elements();
    int offset = 0;

    for (int elem_id = 0; elem_id < num_elems; elem_id ++)
    {
        int size = 0;
        for (int mat_id = 0; mat_id < nmats; mat_id ++)
        {
            const float64 vol_frac = full_vol_fracs[mat_id][elem_id];
            if (vol_frac > epsilon)
            {
                const int num_species_for_this_material = num_species_for_mat.at(mat_id);
                for (int spec_id = 0; spec_id < num_species_for_this_material; spec_id ++)
                {
                    const float64 spec_val = full_specset_vals.at(mat_id).at(spec_id)[elem_id];
                    matset_values.push_back(spec_val);
                    size ++;
                }
            }
        }
        sizes.push_back(size);
        offsets.push_back(offset);
        offset += size;
    }

    dest_specset["matset_values"].set(matset_values);
    dest_specset["sizes"].set(sizes);
    dest_specset["offsets"].set(offsets);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> full
void
uni_buffer_by_element_to_multi_buffer_by_element_matset(const conduit::Node &src_matset,
                                                        conduit::Node &dest_matset)
{
    // copy material map since we have it
    dest_matset["material_map"].set(src_matset["material_map"]);

    // create container for new volume fractions
    Node &new_vol_fracs = dest_matset["volume_fractions"];

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_mats = m_acc.num_mats();
    const index_t num_elems = m_acc.num_elems();

    std::vector<float64_array> new_vol_fracs_vec(num_mats);
    // initialize sizes of the vol frac arrays
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        new_vol_fracs[matname].set(DataType::float64(num_elems));
        new_vol_fracs_vec[mat_order_id] = new_vol_fracs[matname].as_float64_array();
        new_vol_fracs_vec[mat_order_id].fill(0.0);
    }

    // what we will do for each mat_id/vol_frac we encounter
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        new_vol_fracs_vec[mat_order_id][elem_idx] = m_acc.get_vol_frac(elem_idx, mat_idx);
    };

    walk_matset_value_by_element(m_acc, for_each_value);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> full
void
uni_buffer_by_element_to_multi_buffer_by_element_field(const conduit::Node &src_matset,
                                                       const conduit::Node &src_field,
                                                       conduit::Node &dest_field)
{
    // map material numbers to material names
    const std::map<int, std::string> reverse_matmap = create_reverse_material_map(src_matset["material_map"]);

    // create container for new matset vals
    Node &new_matset_vals = dest_field["matset_values"];

    std::map<std::string, float64_array> new_mset_vals_map;

    const int num_elems = count_zones_from_matset(src_matset);

    // initialize sizes
    for (const auto & mapitem : reverse_matmap)
    {
        const std::string &matname = mapitem.second;
        new_matset_vals[matname].set(DataType::float64(num_elems));
        new_mset_vals_map[matname] = new_matset_vals[matname].as_float64_array();
        new_mset_vals_map[matname].fill(0.0);
    }
    
    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    // what we will do for each mat_id/mset_val we encounter
    auto for_each_value = [&](const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const std::string &matname = reverse_matmap.at(m_acc.get_mat_id(zone_idx, mat_idx));
        new_mset_vals_map[matname][zone_idx] = m_acc.get_mset_val(zone_idx, mat_idx);
    };

    walk_matset_value_by_element(m_acc, for_each_value);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> full
void
uni_buffer_by_element_to_multi_buffer_by_element_specset(const conduit::Node &src_matset,
                                                         const conduit::Node &src_specset,
                                                         conduit::Node &dest_specset)
{
    // map material numbers to material names
    const std::map<int, std::string> reverse_matmap = create_reverse_material_map(src_matset["material_map"]);

    const int num_elems = src_matset["sizes"].dtype().number_of_elements();

    // The output (full representation) matset_values
    Node &full_matset_vals = dest_specset["matset_values"];
    // We create a map that will reference the output specset, which we will write to
    std::map<std::string, std::map<std::string, float64_array>> new_matset_vals;
    
    // fetch the sparse by element species names child
    const Node &sbe_species_names = src_specset["species_names"];
    
    // grab the material names from the species_names tree
    const std::vector<std::string> &matnames = sbe_species_names.child_names();
    
    // iterate over material names
    for (int mat_id = 0; mat_id < static_cast<int>(matnames.size()); mat_id ++)
    {
        // grab current material name
        const std::string &matname = matnames[mat_id];
        
        // get the species names associated with this material
        const std::vector<std::string> &specnames_for_mat = sbe_species_names[matname].child_names();
        
        // iterate through the species names
        for (const auto &specname : specnames_for_mat)
        {
            // create an output array in the right spot with the total number of elements
            full_matset_vals[matname][specname].set(DataType::float64(num_elems));
            
            // save a pointer to the output arrays in the c++ map we created
            new_matset_vals[matname][specname] = full_matset_vals[matname][specname].value();
        }
    }

    // Get ptr to matset values and mat ids
    float64_accessor matset_values = src_specset["matset_values"].value();
    int64_accessor material_ids = src_matset["material_ids"].value();

    auto save_function = [&](const std::string &matname,
                             const std::string &specname,
                             const int elem_id,
                             const float64 val)
    {
        new_matset_vals.at(matname).at(specname)[elem_id] = val;
    };

    walk_uni_buffer_by_element_to_multi_buffer_by_element_specset(src_matset,
                                                                  src_specset,
                                                                  reverse_matmap,
                                                                  matset_values,
                                                                  material_ids,
                                                                  save_function);

    // There is nothing more to do as we took care to save species mass fractions
    // in the right place.
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_matset(const conduit::Node &src_matset,
                                                         conduit::Node &dest_matset)
{
    // copy material map since we have it
    dest_matset["material_map"].set(src_matset["material_map"]);

    // map material numbers to material names
    const std::map<int, std::string> reverse_matmap = create_reverse_material_map(src_matset["material_map"]);

    // create containers for new vol fracs and elem ids
    std::map<std::string, std::vector<float64>> new_vol_fracs;
    std::map<std::string, std::vector<int64>> new_elem_ids;

    MatsetAccessor m_acc = MatsetAccessor(src_matset);

    // what we will do for each mat_id/vol_frac we encounter
    auto for_each_value = [&](const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t curr_material_index)
    {
        (void) curr_material_index;
        const std::string &matname = reverse_matmap.at(m_acc.get_mat_id(zone_idx, mat_idx));
        new_vol_fracs[matname].push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
        new_elem_ids[matname].push_back(zone_idx);
    };

    walk_matset_value_by_element(m_acc, for_each_value);

    read_from_map_write_out(new_vol_fracs, dest_matset["volume_fractions"]);
    read_from_map_write_out(new_elem_ids, dest_matset["element_ids"]);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_field(const conduit::Node &src_matset,
                                                        const conduit::Node &src_field,
                                                        conduit::Node &dest_field)
{
    // map material numbers to material names
    const std::map<int, std::string> reverse_matmap = create_reverse_material_map(src_matset["material_map"]);

    // create container for new matset vals
    std::map<std::string, std::vector<float64>> new_mset_vals;

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    // what we will do for each mat_id/mset_val we encounter
    auto for_each_value = [&](const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const std::string &matname = reverse_matmap.at(m_acc.get_mat_id(zone_idx, mat_idx));
        new_mset_vals[matname].push_back(m_acc.get_mset_val(zone_idx, mat_idx));
    };

    walk_matset_value_by_element(m_acc, for_each_value);

    read_from_map_write_out(new_mset_vals, dest_field["matset_values"]);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_specset(const conduit::Node &src_matset,
                                                          const conduit::Node &src_specset,
                                                          conduit::Node &dest_specset)
{
    // map material numbers to material names
    const std::map<int, std::string> reverse_matmap = create_reverse_material_map(src_matset["material_map"]);

    // We create a map that we will write to
    std::map<std::string, std::map<std::string, std::vector<float64>>> new_matset_vals;
    
    // Get ptr to matset values and mat ids
    float64_accessor matset_values = src_specset["matset_values"].value();
    int64_accessor material_ids = src_matset["material_ids"].value();

    auto save_function = [&](const std::string &matname,
                             const std::string &specname,
                             const int elem_id,
                             const float64 val)
    {
        (void) elem_id;
        new_matset_vals[matname][specname].push_back(val);
    };

    walk_uni_buffer_by_element_to_multi_buffer_by_element_specset(src_matset,
                                                                  src_specset,
                                                                  reverse_matmap,
                                                                  matset_values,
                                                                  material_ids,
                                                                  save_function);

    // fetch the sparse by element species names child
    const Node &sbe_species_names = src_specset["species_names"];
    
    // grab the material names from the species_names tree
    const std::vector<std::string> &matnames = sbe_species_names.child_names();

    // for each material, save the species mass fractions for each species
    for (const auto &matname : matnames)
    {
        read_from_map_write_out(new_matset_vals.at(matname), dest_specset["matset_values"][matname]);
    }
}

//-----------------------------------------------------------------------------
// venn full -> sparse_by_material
void
multi_buffer_by_element_to_multi_buffer_by_material_matset(const conduit::Node &src_matset,
                                                           conduit::Node &dest_matset,
                                                           const float64 epsilon)
{
    if (src_matset.has_child("material_map"))
    {
        dest_matset["material_map"].set(src_matset["material_map"]);
    }

    const int num_zones = count_zones_from_matset(src_matset);

    Node material_map;
    create_or_reuse_material_map(src_matset, material_map);

    Node n;
    n["local_element_ids"].set(DataType::index_t(num_zones));
    n["local_volume_fractions"].set(DataType::float64(num_zones));
    index_t_array local_element_ids = n["local_element_ids"].value();
    float64_array local_volume_fractions = n["local_volume_fractions"].value();

    MatsetAccessor m_acc = MatsetAccessor(src_matset);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t eid_id)
    {
        local_element_ids[eid_id] = zone_idx;
        local_volume_fractions[eid_id] = m_acc.get_vol_frac(zone_idx, mat_idx);
    };

    // what we will do for each material's elem_ids/vol_fracs
    auto for_each_material = [&](const index_t mat_idx,
                                 const index_t num_elems_for_mat)
    {
        const std::string matname = material_map.child(mat_idx).name();
        dest_matset["volume_fractions"][matname].set(DataType::float64(num_elems_for_mat));
        float64_array volume_fractions = dest_matset["volume_fractions"][matname].value();
        dest_matset["element_ids"][matname].set(DataType::index_t(num_elems_for_mat));
        index_t_array element_ids = dest_matset["element_ids"][matname].value();

        for (index_t eid_id = 0; eid_id < num_elems_for_mat; eid_id ++)
        {
            element_ids[eid_id] = local_element_ids[eid_id];
            volume_fractions[eid_id] = local_volume_fractions[eid_id];
        }
    };

    walk_matset_by_material(m_acc, for_each_value, for_each_material, epsilon);
}

//-----------------------------------------------------------------------------
// venn full -> sparse_by_material
void
multi_buffer_by_element_to_multi_buffer_by_material_field(const conduit::Node &src_matset,
                                                          const conduit::Node &src_field,
                                                          conduit::Node &dest_field,
                                                          const float64 epsilon)
{
    const index_t num_zones = count_zones_from_matset(src_matset);

    Node n;
    n["local_matset_values"].set(DataType::float64(num_zones));
    float64_array local_matset_values = n["local_matset_values"].value();

    Node material_map;
    create_or_reuse_material_map(src_matset, material_map);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t eid_id)
    {
        local_matset_values[eid_id] = m_acc.get_mset_val(zone_idx, mat_idx);
    };

    // what we will do for each material's mset_vals
    auto for_each_material = [&](const index_t mat_idx,
                                 const index_t num_elems_for_mat)
    {
        const std::string matname = material_map.child(mat_idx).name();
        Node &result_mset_vals = dest_field["matset_values"][matname];
        result_mset_vals.set(DataType::float64(num_elems_for_mat));
        float64_array result_mset_vals_arr = result_mset_vals.value();
        for (index_t eid_id = 0; eid_id < num_elems_for_mat; eid_id ++)
        {
            result_mset_vals_arr[eid_id] = local_matset_values[eid_id];
        }
    };

    walk_matset_by_material(m_acc, for_each_value, for_each_material, epsilon);
}

//-----------------------------------------------------------------------------
// venn full -> sparse_by_material
void
multi_buffer_by_element_to_multi_buffer_by_material_specset(const conduit::Node &src_matset,
                                                            const conduit::Node &src_specset,
                                                            conduit::Node &dest_specset,
                                                            const float64 epsilon)
{
    auto mat_itr = src_matset["volume_fractions"].children();
    auto smat_itr = src_specset["matset_values"].children();
    while (mat_itr.has_next() && smat_itr.has_next())
    {
        const Node &mat_vol_fracs = mat_itr.next();
        const std::string matname = mat_itr.name();
        
        const Node &full_mat_vals = smat_itr.next();
        const std::string smatname = smat_itr.name();

        CONDUIT_ASSERT(matname == smatname, "Materials must be ordered the same in "
            "material dependent specsets and their matsets.");

        const float64_accessor full_vol_fracs = mat_vol_fracs.value();

        const int num_elems = full_vol_fracs.dtype().number_of_elements();

        const std::vector<std::string> &specnames_for_mat = full_mat_vals.child_names();

        for (const auto &specname : specnames_for_mat)
        {
            float64_accessor full_spec_mset_vals = full_mat_vals[specname].value();
            std::vector<float64> mset_vals;
            for (int elem_id = 0; elem_id < num_elems; elem_id ++)
            {
                if (full_vol_fracs[elem_id] > epsilon)
                {
                    mset_vals.push_back(full_spec_mset_vals[elem_id]);
                }
            }
            dest_specset["matset_values"][matname][specname].set(mset_vals);
        }
    }
}

//-----------------------------------------------------------------------------
// venn sparse by material -> full
void
multi_buffer_by_material_to_multi_buffer_by_element_matset(const conduit::Node &src_matset,
                                                           conduit::Node &dest_matset)
{
    if (src_matset.has_child("material_map"))
    {
        dest_matset["material_map"].set(src_matset["material_map"]);
    }

    const index_t num_materials = count_materials_from_matset(src_matset);
    const index_t num_elems = count_zones_from_matset(src_matset);

    std::vector<float64_array> mat_idx_to_data(num_materials);

    // create the output data arrays and save a pointer to each one
    const std::vector<std::string> &matnames = src_matset["volume_fractions"].child_names();
    for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
    {
        const std::string &matname = matnames[mat_idx];
        dest_matset["volume_fractions"][matname].set(DataType::float64(num_elems));
        float64_array dest_data = dest_matset["volume_fractions"][matname].value();
        dest_data.fill(0.0);

        mat_idx_to_data[mat_idx] = dest_data;
    }

    MatsetAccessor m_acc = MatsetAccessor(src_matset);

    // what we will do for each vol_frac/elem_id pair
    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t)
    {
        const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
        mat_idx_to_data[mat_idx][real_zone_id] = m_acc.get_vol_frac(zone_idx, mat_idx);
    };

    walk_matset_value_by_material(m_acc, for_each_value);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> full
void
multi_buffer_by_material_to_multi_buffer_by_element_field(const conduit::Node &src_matset,
                                                          const conduit::Node &src_field,
                                                          conduit::Node &dest_field)
{
    const index_t num_materials = count_materials_from_matset(src_matset);
    const index_t num_elems = count_zones_from_matset(src_matset);

    std::vector<float64_array> mat_idx_to_data(num_materials);

    // create the output data arrays and save a pointer to each one
    const std::vector<std::string> &matnames = src_matset["volume_fractions"].child_names();
    for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
    {
        const std::string &matname = matnames[mat_idx];
        dest_field["matset_values"][matname].set(DataType::float64(num_elems));
        float64_array dest_data = dest_field["matset_values"][matname].value();
        dest_data.fill(0.0);

        mat_idx_to_data[mat_idx] = dest_data;
    }

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    // what we will do for each mset_val
    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t)
    {
        const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
        mat_idx_to_data[mat_idx][real_zone_id] = m_acc.get_mset_val(zone_idx, mat_idx);;
    };

    walk_matset_value_by_material(m_acc, for_each_value);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> full
void
multi_buffer_by_material_to_multi_buffer_by_element_specset(const conduit::Node &src_matset,
                                                            const conduit::Node &src_specset,
                                                            conduit::Node &dest_specset)
{
    // sparse by material representation
    // we map material names to element ids and maps from species names to mass fractions
    std::map<std::string, std::pair<int64_accessor, std::map<std::string, float64_accessor>>> sbm_rep;

    create_sbm_specset_rep(src_matset["element_ids"], src_specset["matset_values"], sbm_rep);

    const int num_elems = determine_num_elems_in_multi_buffer_by_material(src_matset["element_ids"]);

    for (const auto &mapitem : sbm_rep)
    {
        const std::string &matname = mapitem.first;
        const int64_accessor sbm_eids = mapitem.second.first;

        const std::map<std::string, float64_accessor> &spec_mf_map = mapitem.second.second;
        for (const auto &spec_mf_map_item : spec_mf_map)
        {
            const std::string specname = spec_mf_map_item.first;
            const float64_accessor sbm_spec_mf_vals = spec_mf_map_item.second;

            dest_specset["matset_values"][matname][specname].set(DataType::float64(num_elems));
            float64_array dest_data = dest_specset["matset_values"][matname][specname].value();
            dest_data.fill(0.0);

            const int num_mf = sbm_spec_mf_vals.dtype().number_of_elements();
            for (int spec_mf_id = 0; spec_mf_id < num_mf; spec_mf_id ++)
            {
                const int elem_id = sbm_eids[spec_mf_id];
                const float64 value = sbm_spec_mf_vals[spec_mf_id];

                dest_data[elem_id] = value;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// venn sparse by material -> sparse by element
void
multi_buffer_by_material_to_uni_buffer_by_element_matset(const conduit::Node &src_matset,
                                                         conduit::Node &dest_matset)
{
    Node &material_map = dest_matset["material_map"];
    create_or_copy_material_map(src_matset, material_map);

    const int num_elems = determine_num_elems_in_multi_buffer_by_material(src_matset["element_ids"]);

    // There is no way to pack the volume fractions correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which volume fractions are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_vol_fracs(num_elems);
    std::vector<std::vector<int64>> intermediate_mat_ids(num_elems);

    MatsetAccessor m_acc = MatsetAccessor(src_matset);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t)
    {
        const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
        intermediate_mat_ids[real_zone_id].push_back(m_acc.get_mat_id(zone_idx, mat_idx));
        intermediate_vol_fracs[real_zone_id].push_back(m_acc.get_vol_frac(zone_idx, mat_idx));
    };

    walk_matset_value_by_material(m_acc, for_each_value);

    std::vector<float64> vol_fracs;
    std::vector<int64> mat_ids;
    std::vector<int64> sizes;
    std::vector<int64> offsets;

    // final pass
    int64 offset = 0;
    for (int elem_id = 0; elem_id < num_elems; elem_id ++)
    {
        const int64 size = static_cast<int64>(intermediate_vol_fracs[elem_id].size());
        for (int64 mat_vf_id = 0; mat_vf_id < size; mat_vf_id ++)
        {
            vol_fracs.push_back(intermediate_vol_fracs[elem_id][mat_vf_id]);
            mat_ids.push_back(intermediate_mat_ids[elem_id][mat_vf_id]);
        }
        sizes.push_back(size);
        offsets.push_back(offset);
        offset += size;
    }

    dest_matset["volume_fractions"].set(vol_fracs);
    dest_matset["material_ids"].set(mat_ids);
    dest_matset["sizes"].set(sizes);
    dest_matset["offsets"].set(offsets);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> sparse by element
void
multi_buffer_by_material_to_uni_buffer_by_element_field(const conduit::Node &src_matset,
                                                        const conduit::Node &src_field,
                                                        conduit::Node &dest_field)
{
    const int num_elems = determine_num_elems_in_multi_buffer_by_material(src_matset["element_ids"]);

    // There is no way to pack the matset values correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which matset values are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_mset_vals(num_elems);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t zone_idx,
                              const index_t)
    {
        const index_t real_zone_id = m_acc.get_elem_id(zone_idx, mat_idx);
        intermediate_mset_vals[real_zone_id].push_back(m_acc.get_mset_val(zone_idx, mat_idx));
    };

    walk_matset_value_by_material(m_acc, for_each_value);

    std::vector<float64> mset_vals;

    // final pass
    for (int elem_id = 0; elem_id < num_elems; elem_id ++)
    {
        int size = static_cast<int>(intermediate_mset_vals[elem_id].size());
        for (int mat_vf_id = 0; mat_vf_id < size; mat_vf_id ++)
        {
            mset_vals.push_back(intermediate_mset_vals[elem_id][mat_vf_id]);
        }
    }

    dest_field["matset_values"].set(mset_vals);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> sparse by element
void
multi_buffer_by_material_to_uni_buffer_by_element_specset(const conduit::Node &src_matset,
                                                          const conduit::Node &src_specset,
                                                          conduit::Node &dest_specset)
{
    // sparse by material representation
    // we map material names to element ids and maps from species names to mass fractions
    std::map<std::string, std::pair<int64_accessor, std::map<std::string, float64_accessor>>> sbm_rep;

    create_sbm_specset_rep(src_matset["element_ids"], src_specset["matset_values"], sbm_rep);

    // create the species_names
    const std::vector<std::string> &matnames = src_specset["matset_values"].child_names();
    for (const auto &matname : matnames)
    {
        const std::vector<std::string> &specnames = src_specset["matset_values"][matname].child_names();
        for (const auto &specname : specnames)
        {
            dest_specset["species_names"][matname][specname];
        }
    }

    // fetch the number of elements
    const int num_elems = determine_num_elems_in_multi_buffer_by_material(src_matset["element_ids"]);

    // There is no way to pack the matset values correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which matset values are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_mset_vals(num_elems);

    for (const auto &mapitem : sbm_rep)
    {
        int64_accessor sbm_eids = mapitem.second.first;
        const int num_mf = sbm_eids.dtype().number_of_elements();

        const std::map<std::string, float64_accessor> &spec_mf_map = mapitem.second.second;
        for (const auto &spec_mf_map_item : spec_mf_map)
        {
            const float64_accessor sbm_spec_mf_vals = spec_mf_map_item.second;

            for (int spec_mf_id = 0; spec_mf_id < num_mf; spec_mf_id ++)
            {
                const int64 elem_id = sbm_eids[spec_mf_id];
                const float64 spec_mf_val = sbm_spec_mf_vals[spec_mf_id];

                intermediate_mset_vals[elem_id].push_back(spec_mf_val);
            }
        }
    }

    std::vector<float64> mset_vals;
    std::vector<int64> sizes;
    std::vector<int64> offsets;

    // final pass
    int64 offset = 0;
    for (int elem_id = 0; elem_id < num_elems; elem_id ++)
    {
        const int64 size = static_cast<int64>(intermediate_mset_vals[elem_id].size());
        for (int64 mat_vf_id = 0; mat_vf_id < size; mat_vf_id ++)
        {
            mset_vals.push_back(intermediate_mset_vals[elem_id][mat_vf_id]);
        }
        sizes.push_back(size);
        offsets.push_back(offset);
        offset += size;
    }

    dest_specset["matset_values"].set(mset_vals);
    dest_specset["sizes"].set(sizes);
    dest_specset["offsets"].set(offsets);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
to_silo(const conduit::Node &matset,
        conduit::Node &dest,
        const float64 epsilon)
{
    // extra seat belt here b/c we want to avoid folks entering
    // the detail version of to_silo with surprising results.

    if(!matset.dtype().is_object() )
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_silo passed matset node"
                      " must be a valid matset tree.");
    }

    conduit::Node field, specset;

    detail::to_silo(matset,
                    field,
                    specset,
                    dest,
                    epsilon);
}

//-----------------------------------------------------------------------------
std::map<int, std::string>
create_reverse_material_map(const conduit::Node &src_material_map)
{
    std::map<int, std::string> reverse_matmap;
    // fill out map
    auto matmap_itr = src_material_map.children();
    while (matmap_itr.has_next())
    {
        const Node &matmap_entry = matmap_itr.next();
        const std::string matname = matmap_itr.name();
        reverse_matmap[matmap_entry.to_int()] = matname;
    }
    return reverse_matmap;
}

//-------------------------------------------------------------------------
// renumbers material ids to run between 0 and N-1 where N is the number of
// materials.
void
renumber_material_ids(const conduit::Node &src_matset,
                      conduit::Node &dest_matset)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::renumber_material_ids"
                      " passed matset node must be a valid matset tree.");
    }

    dest_matset.set(src_matset);
    renumber_material_ids(dest_matset);
}

//-------------------------------------------------------------------------
// renumbers material ids to run between 0 and N-1 where N is the number of
// materials.
void
renumber_material_ids(conduit::Node &matset)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::renumber_material_ids"
                      " passed matset node must be a valid matset tree.");
    }

    if (is_uni_buffer(matset))
    {
        // if we are sparse by element we have more to do
        if (is_element_dominant(matset))
        {
            // we must have material map in this case
            std::map<index_t, index_t> old_to_new;
            const std::vector<std::string> &matnames = matset["material_map"].child_names();
            const index_t num_mats = static_cast<index_t>(matnames.size());
            for (index_t i = 0; i < num_mats; i ++)
            {
                const std::string &matname = matnames[i];
                const index_t old = matset["material_map"][matname].to_index_t();
                matset["material_map"][matname].set(i);
                old_to_new[old] = i;
            }

            index_t_accessor mat_ids = matset["material_ids"].as_index_t_accessor();
            for (index_t i = 0; i < mat_ids.number_of_elements(); i ++)
            {
                const index_t old_mat_id = mat_ids[i];
                mat_ids.set(i, old_to_new.at(old_mat_id));
            }
        }
        // unsupported uni-buffer by material
        else
        {
            CONDUIT_ERROR("conduit::blueprint::mesh::matset::renumber_material_ids() "
                          "material-dominant uni-buffer material set is unsupported.")
        }
    }
    else // multi-buffer case
    {
        // if we have a material map to modify
        if (matset.has_child("material_map"))
        {
            const std::vector<std::string> &matnames = matset["material_map"].child_names();
            const index_t num_mats = static_cast<index_t>(matnames.size());
            for (index_t i = 0; i < num_mats; i ++)
            {
                const std::string &matname = matnames[i];
                matset["material_map"][matname].set(i);
            }
        }
    }
}

//-------------------------------------------------------------------------
// this will use set external if the matmap already exists
void
create_or_reuse_material_map(const conduit::Node &matset,
                             conduit::Node &material_map)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::create_or_reuse_material_map"
                      " passed matset node must be a valid matset tree.");
    }

    material_map.reset();

    if (matset.has_child("material_map"))
    {
        material_map.set_external(matset["material_map"]);
    }
    else
    {
        detail::create_material_map(matset, material_map);
    }
}

//-------------------------------------------------------------------------
// this will use set if the matmap already exists
void
create_or_copy_material_map(const conduit::Node &matset,
                            conduit::Node &material_map)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::create_or_copy_material_map"
                      " passed matset node must be a valid matset tree.");
    }

    material_map.reset();

    if (matset.has_child("material_map"))
    {
        material_map.set(matset["material_map"]);
    }
    else
    {
        detail::create_material_map(matset, material_map);
    }
}

//-------------------------------------------------------------------------
index_t 
count_zones_from_matset(const conduit::Node &matset)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::count_zones_in_matset"
                      " passed matset node must be a valid matset tree.");
    }

    const bool element_dominant = is_element_dominant(matset);
    const bool multi_buffer = is_multi_buffer(matset);

    if (element_dominant)
    {
        // venn full
        if (multi_buffer)
        {
            if (matset["volume_fractions"].number_of_children() > 0)
            {
                return matset["volume_fractions"][0].dtype().number_of_elements();
            }
            else
            {
                return 0;
            }
        }
        // venn sparse by element
        else
        {
            return matset["sizes"].dtype().number_of_elements();
        }
    }
    else
    {
        // venn sparse by material
        if (multi_buffer)
        {
            return detail::determine_num_elems_in_multi_buffer_by_material(matset["element_ids"]);
        }
        // material-dominant uni-buffer
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::count_zones_in_matset() "
                          "material-dominant uni-buffer material set is unsupported.");
        }
    }

    return -1;
}

//-------------------------------------------------------------------------
index_t 
count_materials_from_matset(const conduit::Node &matset)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::count_materials_from_matset"
                      " passed matset node must be a valid matset tree.");
    }

    const bool element_dominant = is_element_dominant(matset);
    const bool multi_buffer = is_multi_buffer(matset);

    if (element_dominant)
    {
        // venn full
        if (multi_buffer)
        {
            return matset["volume_fractions"].number_of_children();
        }
        // venn sparse by element
        else
        {
            return matset["material_map"].number_of_children();
        }
    }
    else
    {
        // venn sparse by material
        if (multi_buffer)
        {
            return matset["volume_fractions"].number_of_children();
        }
        // material-dominant uni-buffer
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::count_materials_from_matset() "
                          "material-dominant uni-buffer material set is unsupported.");
        }
    }

    return -1;
}

//-------------------------------------------------------------------------
bool 
is_material_in_zone(const conduit::Node &matset,
                    const std::string &matname,
                    const index_t zone_id,
                    const float64 epsilon)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::is_material_in_zone"
                      " passed matset node must be a valid matset tree.");
    }
    // full
    if (is_element_dominant(matset) && is_multi_buffer(matset))
    {
        if (matset["volume_fractions"].has_child(matname))
        {
            const float64_accessor vfs = matset["volume_fractions"][matname].value();
            return vfs[zone_id] > epsilon;
        }
        else
        {
            // obviously the material is not present in the zone; it is not
            // present in the matset
            return false;
        }
    }
    // sparse_by_element
    else if (is_element_dominant(matset))
    {
        const index_t_accessor sizes = matset["sizes"].value();
        const index_t_accessor offsets = matset["offsets"].value();
        const index_t_accessor material_ids = matset["material_ids"].value();
        const index_t size = sizes[zone_id];
        const index_t offset = offsets[zone_id];
        std::map<int, std::string> reverse_matmap = mesh::matset::create_reverse_material_map(matset["material_map"]);
        // look at materials in this zone
        for (index_t idx = 0; idx < size; idx ++)
        {
            const index_t mat_id = material_ids[idx + offset];
            const std::string &curr_matname = reverse_matmap.at(mat_id);
            if (curr_matname == matname)
            {
                // we found the right material in this zone
                return true;
            }
        }
        // not found in this zone
        return false;
    }
    // sparse_by_material
    else if (is_material_dominant(matset))
    {
        if (matset["element_ids"].has_child(matname))
        {
            const index_t_accessor elem_ids = matset["element_ids"][matname].value();
            return elem_ids.count(zone_id) > 0;
        }
        else
        {
            // obviously the material is not present in the zone; it is not
            // present in the matset
            return false;
        }
    }
    else
    {
        CONDUIT_ERROR("Unknown matset type.");
    }
    return false;
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_element(const conduit::Node &src_matset,
                           conduit::Node &dest_matset)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_multi_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    dest_matset.reset();

    // set the topology
    dest_matset["topology"].set(src_matset["topology"]);

    const bool elem_dom = is_element_dominant(src_matset);
    const bool multi_buf = is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            // nothing to do
            dest_matset.set(src_matset);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            detail::uni_buffer_by_element_to_multi_buffer_by_element_matset(src_matset, 
                                                                            dest_matset);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            detail::multi_buffer_by_material_to_multi_buffer_by_element_matset(src_matset,
                                                                               dest_matset);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::to_multi_buffer_by_element() "
                          "material-dominant uni-buffer material set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_element(const conduit::Node &src_matset,
                         conduit::Node &dest_matset,
                         const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_uni_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    dest_matset.reset();

    // set the topology
    dest_matset["topology"].set(src_matset["topology"]);

    const bool elem_dom = is_element_dominant(src_matset);
    const bool multi_buf = is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            detail::multi_buffer_by_element_to_uni_buffer_by_element_matset(src_matset, 
                                                                            dest_matset, 
                                                                            epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            // nothing to do
            dest_matset.set(src_matset);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            detail::multi_buffer_by_material_to_uni_buffer_by_element_matset(src_matset,
                                                                             dest_matset);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::to_uni_buffer_by_element() "
                          "material-dominant uni-buffer material set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_material(const conduit::Node &src_matset,
                            conduit::Node &dest_matset,
                            const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_multi_buffer_by_material"
                      " passed matset node must be a valid matset tree.");
    }

    dest_matset.reset();

    // set the topology
    dest_matset["topology"].set(src_matset["topology"]);

    const bool elem_dom = is_element_dominant(src_matset);
    const bool multi_buf = is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            detail::multi_buffer_by_element_to_multi_buffer_by_material_matset(src_matset, 
                                                                               dest_matset, 
                                                                               epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            detail::uni_buffer_by_element_to_multi_buffer_by_material_matset(src_matset,
                                                                             dest_matset);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            // nothing to do
            dest_matset.set(src_matset);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::to_multi_buffer_by_material() "
                          "material-dominant uni-buffer material set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_material(const conduit::Node &src_matset,
                          conduit::Node &dest_matset,
                          const float64 epsilon)
{
    (void) src_matset;
    (void) dest_matset;
    (void) epsilon;
    CONDUIT_ERROR("blueprint::mesh::matset::to_uni_buffer_by_material() "
                  "converting from a material-dominant uni-buffer material set is unsupported.");
}

//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::specset --
//-----------------------------------------------------------------------------
namespace specset
{
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void
to_silo(const conduit::Node &specset,
        const conduit::Node &matset,
        conduit::Node &dest,
        const float64 epsilon)
{
    if (! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_silo passed specset node "
                      "must be a valid specset tree.");
    }

    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_silo passed matset node "
                      "must be a valid matset tree or a valid intermediate silo "
                      "representation of a matset.");
    }

    conduit::Node field;

    blueprint::mesh::matset::detail::to_silo(matset,
                                             field,
                                             specset,
                                             dest,
                                             epsilon);
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_element(const conduit::Node &src_matset,
                           const conduit::Node &src_specset,
                           const std::string &dest_matset_name,
                           conduit::Node &dest_specset)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_element"
                      " passed specset node must be a valid specset tree.");
    }

    dest_specset.reset();

    // set the matset
    dest_specset["matset"] = dest_matset_name;

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            // nothing to do
            dest_specset.set(src_specset);
            dest_specset["matset"].reset();
            dest_specset["matset"] = dest_matset_name;
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            conduit::blueprint::mesh::matset::detail::uni_buffer_by_element_to_multi_buffer_by_element_specset(
                src_matset, src_specset, dest_specset);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_material_to_multi_buffer_by_element_specset(
                src_matset, src_specset, dest_specset);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_element() "
                          "material-dominant uni-buffer material/species set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_element(const conduit::Node &src_matset,
                         const conduit::Node &src_specset,
                         const std::string &dest_matset_name,
                         conduit::Node &dest_specset,
                         const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_uni_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_uni_buffer_by_element"
                      " passed specset node must be a valid specset tree.");
    }

    dest_specset.reset();

    // set the matset
    dest_specset["matset"] = dest_matset_name;

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_element_to_uni_buffer_by_element_specset(
                src_matset, src_specset, dest_specset, epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            // nothing to do
            dest_specset.set(src_specset);
            dest_specset["matset"].reset();
            dest_specset["matset"] = dest_matset_name;
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_material_to_uni_buffer_by_element_specset(
                src_matset, src_specset, dest_specset);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::specset::to_uni_buffer_by_element() "
                          "material-dominant uni-buffer material/species set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_material(const conduit::Node &src_matset,
                            const conduit::Node &src_specset,
                            const std::string &dest_matset_name,
                            conduit::Node &dest_specset,
                            const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_material"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_material"
                      " passed specset node must be a valid specset tree.");
    }

    dest_specset.reset();

    // set the matset
    dest_specset["matset"] = dest_matset_name;

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_element_to_multi_buffer_by_material_specset(
                src_matset, src_specset, dest_specset, epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            conduit::blueprint::mesh::matset::detail::uni_buffer_by_element_to_multi_buffer_by_material_specset(
                src_matset, src_specset, dest_specset);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            // nothing to do
            dest_specset.set(src_specset);
            dest_specset["matset"].reset();
            dest_specset["matset"] = dest_matset_name;
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::specset::to_multi_buffer_by_material() "
                          "material-dominant uni-buffer material/species set is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_material(const conduit::Node &src_matset,
                          const conduit::Node &src_specset,
                          const std::string &dest_matset_name,
                          conduit::Node &dest_specset,
                          const float64 epsilon)
{
    (void) src_matset;
    (void) src_specset;
    (void) dest_matset_name;
    (void) dest_specset;
    (void) epsilon;
    CONDUIT_ERROR("blueprint::mesh::specset::to_uni_buffer_by_material() "
                  "converting from a material-dominant uni-buffer material/species set is unsupported.");
}

//-----------------------------------------------------------------------------
index_t
get_num_species_for_material(const conduit::Node &specset,
                             const std::string &matname)
{
    if (is_multi_buffer(specset))
    {
        if (specset["matset_values"].has_child(matname))
        {
            return specset["matset_values"][matname].number_of_children();
        }
        else
        {
            return 0;
        }
    }
    else // uni buffer
    {
        if (specset["species_names"].has_child(matname))
        {
            return specset["species_names"][matname].number_of_children();
        }
        else
        {
            return 0;
        }
    }
}

//-----------------------------------------------------------------------------
void
get_material_names(const conduit::Node &specset,
                   std::vector<std::string> &matnames)
{
    // extra seat belt here
    if (! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::get_material_names"
                      " passed specset node must be a valid specset tree.");
    }

    if (is_multi_buffer(specset))
    {
        matnames = specset["matset_values"].child_names();
    }
    else // uni buffer
    {
        matnames = specset["species_names"].child_names();
    }
}

//-------------------------------------------------------------------------
index_t 
count_materials_from_specset(const conduit::Node &specset)
{
    // extra seat belt here
    if (! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::count_materials_from_specset"
                      " passed specset node must be a valid specset tree.");
    }

    if (is_multi_buffer(specset))
    {
        return specset["matset_values"].number_of_children();
    }
    else
    {
        return specset["species_names"].number_of_children();
    }

    return -1;
}

//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::specset --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::blueprint::mesh::field --
//-----------------------------------------------------------------------------
namespace field
{
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void
to_silo(const conduit::Node &field,
        const conduit::Node &matset,
        conduit::Node &dest,
        const float64 epsilon)
{
    // extra seat belts here b/c we want to avoid folks entering
    // the detail version of to_silo with surprising results.

    if(!field.dtype().is_object() )
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_silo passed field node"
                      " must be a valid matset tree.");
    }

    if(!matset.dtype().is_object() )
    {
        CONDUIT_ERROR("blueprint::mesh::matset::to_silo passed matset node"
                      " must be a valid matset tree.");
    }

    conduit::Node specset;

    conduit::blueprint::mesh::matset::detail::to_silo(matset,
                                                      field,
                                                      specset,
                                                      dest,
                                                      epsilon);
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_element(const conduit::Node &src_matset,
                           const conduit::Node &src_field,
                           const std::string &dest_matset_name,
                           conduit::Node &dest_field)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_field.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_element"
                      " passed field node must be a valid field tree.");
    }

    // if this field is NOT material dependent
    if (! src_field.has_child("matset_values"))
    {
        // nothing to do
        dest_field.set(src_field);
        dest_field["matset"].reset();
        dest_field["matset"] = dest_matset_name;
        return;
    }

    dest_field.reset();
    conduit::blueprint::mesh::matset::detail::copy_matset_independent_parts_of_field(
        src_field,
        dest_matset_name,
        dest_field);

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            // nothing to do
            dest_field.set(src_field);
            dest_field["matset"].reset();
            dest_field["matset"] = dest_matset_name;
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            conduit::blueprint::mesh::matset::detail::uni_buffer_by_element_to_multi_buffer_by_element_field(
                src_matset, src_field, dest_field);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_material_to_multi_buffer_by_element_field(
                src_matset, src_field, dest_field);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_element() "
                          "material-dominant uni-buffer material set/field is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_element(const conduit::Node &src_matset,
                         const conduit::Node &src_field,
                         const std::string &dest_matset_name,
                         conduit::Node &dest_field,
                         const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_uni_buffer_by_element"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_field.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_uni_buffer_by_element"
                      " passed field node must be a valid field tree.");
    }

    // if this field is NOT material dependent
    if (! src_field.has_child("matset_values"))
    {
        // nothing to do
        dest_field.set(src_field);
        dest_field["matset"].reset();
        dest_field["matset"] = dest_matset_name;
        return;
    }

    dest_field.reset();
    conduit::blueprint::mesh::matset::detail::copy_matset_independent_parts_of_field(
        src_field,
        dest_matset_name,
        dest_field);

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_element_to_uni_buffer_by_element_field(
                src_matset, src_field, dest_field, epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            // nothing to do
            dest_field.set(src_field);
            dest_field["matset"].reset();
            dest_field["matset"] = dest_matset_name;
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_material_to_uni_buffer_by_element_field(
                src_matset, src_field, dest_field);
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::field::to_uni_buffer_by_element() "
                          "material-dominant uni-buffer material set/field is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_multi_buffer_by_material(const conduit::Node &src_matset,
                            const conduit::Node &src_field,
                            const std::string &dest_matset_name,
                            conduit::Node &dest_field,
                            const float64 epsilon)
{
    // extra seat belt here
    if (! src_matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_material"
                      " passed matset node must be a valid matset tree.");
    }

    if (! src_field.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_material"
                      " passed field node must be a valid field tree.");
    }

    // if this field is NOT material dependent
    if (! src_field.has_child("matset_values"))
    {
        // nothing to do
        dest_field.set(src_field);
        dest_field["matset"].reset();
        dest_field["matset"] = dest_matset_name;
        return;
    }

    dest_field.reset();
    conduit::blueprint::mesh::matset::detail::copy_matset_independent_parts_of_field(
        src_field,
        dest_matset_name,
        dest_field);

    const bool elem_dom = conduit::blueprint::mesh::matset::is_element_dominant(src_matset);
    const bool multi_buf = conduit::blueprint::mesh::matset::is_multi_buffer(src_matset);

    if (elem_dom)
    {
        // multi-buffer element-dominant "full" representation
        if (multi_buf)
        {
            conduit::blueprint::mesh::matset::detail::multi_buffer_by_element_to_multi_buffer_by_material_field(
                src_matset, src_field, dest_field, epsilon);
        }
        // uni-buffer element-dominant "sparse by element" representation
        else
        {
            conduit::blueprint::mesh::matset::detail::uni_buffer_by_element_to_multi_buffer_by_material_field(
                src_matset, src_field, dest_field);
        }
    }
    else
    {
        // multi-buffer material-dominant "sparse by material" representation
        if (multi_buf)
        {
            // nothing to do
            dest_field.set(src_field);
            dest_field["matset"].reset();
            dest_field["matset"] = dest_matset_name;
        }
        // uni-buffer material-dominant "???" representation
        else
        {
            CONDUIT_ERROR("blueprint::mesh::field::to_multi_buffer_by_material() "
                          "material-dominant uni-buffer material set/field is unsupported.");
        }
    }
}

//-----------------------------------------------------------------------------
void
to_uni_buffer_by_material(const conduit::Node &src_matset,
                          const conduit::Node &src_field,
                          const std::string &dest_matset_name,
                          conduit::Node &dest_field,
                          const float64 epsilon)
{
    (void) src_matset;
    (void) src_field;
    (void) dest_matset_name;
    (void) dest_field;
    (void) epsilon;
    CONDUIT_ERROR("blueprint::mesh::field::to_uni_buffer_by_material() "
                  "converting from a material-dominant uni-buffer material set/field is unsupported.");
}

//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::field --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint:::mesh --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

