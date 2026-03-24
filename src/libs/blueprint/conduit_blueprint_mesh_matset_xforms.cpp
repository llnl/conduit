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
// for each element:
//     for each material:
//         do_for_each_material()
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
// for each element:
//     for each material:
//         do_for_each_material()
//     do_for_each_elem()
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
// for each element:
//     for each material:
//         for each species:
//             do_for_each_species()
//         do_for_each_material()
//     do_for_each_elem()
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
            index_t nspec_in_elem = 0;
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
                    nspec_in_elem += num_spec_for_mat;
                }
            }
            for_each_element(elem_idx, nmats_in_elem, nspec_in_elem);
        }
    }
    // sparse by element
    else
    {
        for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
        {
            const index_t nmats_in_elem = m_acc.num_mats_for_elem(elem_idx);
            index_t nspec_in_elem = 0;
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

                nspec_in_elem += num_spec_for_mat;
            }
            for_each_element(elem_idx, nmats_in_elem, nspec_in_elem);
        }
    }
}

//-----------------------------------------------------------------------------
// for each material:
//     for each element:
//         do_for_each_elem()
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
// for each material:
//     for each element:
//         do_for_each_elem()
//     do_for_each_material()
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

            const index_t num_elems = m_acc.num_elems();
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                index_t num_elems_for_mat = 0;
                for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
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
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
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
// for each material:
//     for each element:
//         for each species:
//             do_for_each_species()
//         do_for_each_elem()
//     do_for_each_material()
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

            const index_t num_elems = m_acc.num_elems();
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                index_t num_elems_for_mat = 0;
                for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
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
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
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

//-----------------------------------------------------------------------------
// for each material:
//     for each species:
//         for each element:
//             do_for_each_elem()
//         do_for_each_species()
//     do_for_each_material()
template <class ForEachElementValue,
          class ForEachMaterialSpecies,
          class ForEachMaterial>
void
walk_matset_element_by_material_species(const MatsetAccessor &m_acc,
                                        ForEachElementValue &&for_each_element_value,
                                        ForEachMaterialSpecies &&for_each_material_species,
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

            const index_t num_elems = m_acc.num_elems();
            // Material ids need not be within in the range [0, N-1), so we iterate
            // over the order materials appear in the matset.
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                const index_t num_spec_for_mat = m_acc.num_spec_for_mat(0, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                {
                    index_t num_elems_for_spec = 0;
                    for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
                    {
                        const float64 vol_frac = m_acc.get_vol_frac(elem_idx, mat_idx);
                        if (vol_frac > epsilon)
                        {
                            // mat_idx is an index over all materials
                            // spec_idx is an index over all species for material mat_idx
                            // elem_idx is an index over all elements
                            // num_elems_for_spec is running count of elements for the current species
                            for_each_element_value(mat_idx, spec_idx, elem_idx, num_elems_for_spec);
                            num_elems_for_spec ++;
                        }
                    }
                    for_each_material_species(mat_idx, spec_idx, num_elems_for_spec);
                }
                for_each_material(mat_idx, num_spec_for_mat);
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
            for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
            {
                const index_t num_spec_for_mat = m_acc.num_spec_for_mat(0, mat_idx);
                for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
                {
                    const index_t num_elems_for_spec = m_acc.num_elems_for_mat(mat_idx);
                    for (index_t elem_idx = 0; elem_idx < num_elems_for_spec; elem_idx ++)
                    {
                        // mat_idx is an index over all materials
                        // spec_idx is an index over all species for material mat_idx
                        // elem_idx is an index over all elements the material is in
                        // we pass elem_idx twice because it is also the running count of
                        // elements for the current material species
                        for_each_element_value(mat_idx, spec_idx, elem_idx, elem_idx);
                    }
                    for_each_material_species(mat_idx, spec_idx, num_elems_for_spec);
                }
                for_each_material(mat_idx, num_spec_for_mat);
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

//-------------------------------------------------------------------------
// helper for multi-buffer species sets that do not have 
// species_names.
void
create_species_names(const conduit::Node &specset,
                     conduit::Node &species_names)
{
    // We must be multi-buffer, so we can assume we have a 
    // "matset_values" child that is an object.
    const std::vector<std::string> &matnames = specset["matset_values"].child_names();
    for (const auto &matname : matnames)
    {
        const std::vector<std::string> &specnames = 
            specset["matset_values"][matname].child_names();
        for (const auto &specname : specnames)
        {
            species_names[matname][specname];
        }
    }
}

//-----------------------------------------------------------------------------
// Single implementation that supports the case where just matset
// is passed, and the case where the field is passed.
//
// This is in the detail name space b/c the calling convention is a little
// strange:
//   empty field  node -- first arg, triggers one path, non empty another
//
// We smooth this out for the API by providing the non detail variants,
// which error when passed empty nodes.
//-----------------------------------------------------------------------------
void
to_silo(const conduit::Node &field,
        const conduit::Node &matset,
        conduit::Node &dest,
        const float64 epsilon)
{
    Node temp, data;
    const DataType int_dtype = bputils::find_widest_dtype(matset, bputils::DEFAULT_INT_DTYPES);
    const DataType float_dtype = bputils::find_widest_dtype(matset, bputils::DEFAULT_FLOAT_DTYPE);
    // if matset_values is not empty, we will
    // apply the same xform to it as we do to the volume fractions.
    const bool xform_matset_values = field.has_child("matset_values");

    // NOTE: matset values are always treated as a float64.
    // we could map to the widest int or float type in the future.

    // Extract Material Set Metadata //
    const bool mset_is_unibuffer = blueprint::mesh::matset::is_uni_buffer(matset);
    const bool mset_is_matdom = blueprint::mesh::matset::is_material_dominant(matset);

    // setup the material map, which provides a map from material names
    // to to material numbers
    Node matset_mat_map;

    // mset_is_unibuffer will always have the material_map, other cases
    // it is optional. If not given, the map from material names to ids
    // is implied by the order the materials are presented in the matset node
    if(matset.has_child("material_map") )
    {
        // uni-buffer case provides the map we are looking for
        matset_mat_map.set_external(matset["material_map"]);
    }
    else // if(!mset_is_unibuffer)
    {
        // material_map is implied, construct it here for use and output
        NodeConstIterator vf_itr = matset["volume_fractions"].children();
        while(vf_itr.has_next())
        {
            vf_itr.next();
            std::string curr_mat_name = vf_itr.name();
            temp.reset();
            temp.set(vf_itr.index());
            temp.to_data_type(int_dtype.id(), matset_mat_map[curr_mat_name]);
        }
    }

    const Node mset_mat_map(matset_mat_map);

    // find the number of elements in the matset
    index_t matset_num_elems = 0;
    if(mset_is_matdom)
    {
        if(mset_is_unibuffer)
        {
            const DataAccessor<index_t> eids = matset["element_ids"].value();
            const index_t N = eids.number_of_elements();
            for(index_t i = 0; i < N; i++)
            {
                matset_num_elems = std::max(matset_num_elems, eids[i] + 1);
            }
        }
        else
        {
            NodeConstIterator eids_iter = matset["element_ids"].children();
            while(eids_iter.has_next())
            {
                const Node &eids_node = eids_iter.next();
                const DataType eids_dtype(eids_node.dtype().id(), 1);
                for(index_t ei = 0; ei < eids_node.dtype().number_of_elements(); ei++)
                {
                    temp.set_external(eids_dtype, (void*)eids_node.element_ptr(ei));
                    const index_t elem_index = temp.to_int();
                    matset_num_elems = std::max(matset_num_elems, elem_index + 1);
                }
            }
        }
    }
    else // if(!mset_is_matdom)
    {
        // may need to do a bit of sculpting here; embed the base array into
        // something w/ "values" child, as below
        Node mat_vfs;
        if(mset_is_unibuffer)
        {
            mat_vfs.set_external(matset);
        }
        else
        {
            const Node &temp_vfs = matset["volume_fractions"].child(0);
            if(temp_vfs.dtype().is_object())
            {
                mat_vfs.set_external(temp_vfs);
            }
            else // if(temp_vfs.dtype().is_number())
            {
                mat_vfs["values"].set_external(temp_vfs);
            }
        }

        blueprint::o2mrelation::O2MIterator mat_iter(mat_vfs);
        matset_num_elems = mat_iter.elements(o2mrelation::ONE);
    }
    const index_t mset_num_elems = matset_num_elems;

    // Organize Per-Zone Material Data //

    // create a sparse map from each zone, to each material and its value.
    std::vector< std::map<index_t, float64> > elem_mat_maps(mset_num_elems);
    std::vector< std::map<index_t, float64> > elem_matset_values_maps(mset_num_elems);
    if(mset_is_unibuffer)
    {
        const Node &mat_vfs = matset["volume_fractions"];
        const Node &mat_mids = matset["material_ids"];

        Node mat_eids;
        if(mset_is_matdom)
        {
            mat_eids.set_external(matset["element_ids"]);
        }

        blueprint::o2mrelation::O2MIterator mat_iter(matset);
        while(mat_iter.has_next(o2mrelation::DATA))
        {
            const index_t elem_ind_index = mat_iter.next(o2mrelation::ONE);

            // -- get element id -- //
            // this is either "elem_ind_index" from the o2m, or
            // this index applied to the material-to-elements map
            if(mset_is_matdom)
            {
                temp.set_external(
                    DataType(mat_eids.dtype().id(), 1),
                    (void*)mat_eids.element_ptr(elem_ind_index));
            }

            const index_t elem_index = mset_is_matdom ? temp.to_index_t() : elem_ind_index;

            // we now have the element index, find all material indicies
            // using the o2m-many iter
            mat_iter.to_front(o2mrelation::MANY);
            while(mat_iter.has_next(o2mrelation::MANY))
            {
                mat_iter.next(o2mrelation::MANY);
                const index_t mat_ind_index = mat_iter.index(o2mrelation::DATA);

                // this index now allows us to fetch the
                //  vol frac
                //  matset value
                //  material id

                // get the vf and convert it to a float64
                temp.set_external(
                    DataType(mat_vfs.dtype().id(), 1),
                    (void*)mat_vfs.element_ptr(mat_ind_index));
                const float64 mat_vf = temp.to_float64();

                float64 curr_matset_value = 0;
                // process matset values if passed and convert it to a float64
                if(xform_matset_values)
                {
                    const Node matset_values = field["matset_values"];
                    temp.set_external(
                        DataType(matset_values.dtype().id(), 1),
                        (void*)matset_values.element_ptr(mat_ind_index));
                        curr_matset_value = temp.to_float64();
                }

                // get the material id as an index_t
                temp.set_external(
                    DataType(mat_mids.dtype().id(), 1),
                    (void*)mat_mids.element_ptr(mat_ind_index));
                const index_t mat_id = temp.to_index_t();

                // if this elem has a non-zero (or non-trivial) volume fraction for this
                // material, add it do the map
                if(mat_vf > epsilon)
                {
                    elem_mat_maps[elem_index][mat_id] = mat_vf;

                    // process matset values if passed
                    if(xform_matset_values)
                    {
                        elem_matset_values_maps[elem_index][mat_id] = curr_matset_value;
                    }
                }
            }
        }
    }
    else // if(!mset_is_unibuffer)
    {
        NodeConstIterator mats_iter = matset["volume_fractions"].children();
        while(mats_iter.has_next())
        {
            const Node& mat_node = mats_iter.next();
            const std::string& mat_name = mats_iter.name();
            const index_t mat_id = mset_mat_map[mat_name].to_index_t();

            // NOTE(JRC): This is required because per-material subtrees aren't
            // necessarily 'o2mrelation'-compliant; they can just be raw arrays.
            // To make subsequent processing uniform, we make raw arrays 'o2mrelation's.
            Node mat_vfs;
            if(mat_node.dtype().is_number())
            {
                mat_vfs["values"].set_external(mat_node);
            }
            else
            {
                mat_vfs.set_external(mat_node);
            }

            Node mat_eids;
            if(mset_is_matdom)
            {
                mat_eids.set_external(matset["element_ids"][mat_name]);
            }

            // this is a multi-buffer case, make sure we are pointing
            // to the correct values for this pass
            Node mat_data;
            {
                const std::string vf_path =
                    blueprint::o2mrelation::data_paths(mat_vfs).front();
                mat_data.set_external(mat_vfs[vf_path]);
            }

            blueprint::o2mrelation::O2MIterator mat_iter(mat_vfs);
            for(index_t mat_index = 0; mat_iter.has_next(); mat_index++)
            {
                const index_t mat_itr_index = mat_iter.next();

                // get the current vf value as a float64
                temp.set_external(
                    DataType(mat_data.dtype().id(), 1),
                    (void*)mat_data.element_ptr(mat_itr_index));
                const float64 mat_vf = temp.to_float64();

                // if material dominant:
                //  we use indirection array to find the element index.
                //
                // if element dominant:
                //  the o2m_index is the element index

                if(mset_is_matdom)
                {
                    temp.set_external(
                        DataType(mat_eids.dtype().id(), 1),
                        (void*)mat_eids.element_ptr(mat_index));
                }
                const index_t mat_elem = mset_is_matdom ? temp.to_index_t() : mat_index;

                // we now have both the element and material index.

                // if this elem has a non-zero (or non-trivial) volume fraction for this
                // material, add it do the map
                if(mat_vf > epsilon)
                {
                    elem_mat_maps[mat_elem][mat_id] = mat_vf;
                }
            }
        }

        /// handle case where matset_values was passed
        /// this requires another o2m traversal
        if(xform_matset_values)
        {
            NodeConstIterator matset_values_iter = field["matset_values"].children();
            while(matset_values_iter.has_next())
            {
                const Node& curr_node = matset_values_iter.next();
                const std::string& mat_name = matset_values_iter.name();
                const index_t mat_id = mset_mat_map[mat_name].to_index_t();

                // NOTE(JRC): This is required because per-material subtrees aren't
                // necessarily 'o2mrelation'-compliant; they can just be raw arrays.
                // To make subsequent processing uniform, we make raw arrays 'o2mrelation's.

                Node o2m;
                if(curr_node.dtype().is_number())
                {
                    o2m["values"].set_external(curr_node);
                }
                else
                {
                    o2m.set_external(curr_node);
                }

                Node mat_eids;
                if(mset_is_matdom)
                {
                    mat_eids.set_external(matset["element_ids"][mat_name]);
                }

                // this is a multi-buffer case, make sure we are pointing
                // to the correct values for this pass
                Node matset_values_data;
                {
                    const std::string path =
                        blueprint::o2mrelation::data_paths(o2m).front();
                    matset_values_data.set_external(o2m[path]);
                }

                blueprint::o2mrelation::O2MIterator o2m_iter(o2m);
                for(index_t o2m_index = 0; o2m_iter.has_next(); o2m_index++)
                {
                    const index_t o2m_access_index = o2m_iter.next();

                    // if material dominant:
                    //  we use indirection array to find the element index.
                    //
                    // if element dominant:
                    //  the o2m_index is the element index
                    if(mset_is_matdom)
                    {
                        temp.set_external(
                            DataType(mat_eids.dtype().id(), 1),
                            (void*)mat_eids.element_ptr(o2m_index));
                    }


                    const index_t mat_elem = mset_is_matdom ? temp.to_index_t() : o2m_index;

                    // we now have both the element and material index.
                    // check if the volume fractions have an entry for this case,
                    // if so we will add the corresponding mixvar to its map

                    // if elem_mat_maps[mat_elem] has entry mat_id, add entry to
                    // elem_matset_values_maps
                    if( elem_mat_maps[mat_elem].find(mat_id) != elem_mat_maps[mat_elem].end())
                    {
                        temp.set_external(
                          DataType(matset_values_data.dtype().id(), 1),
                          (void*)matset_values_data.element_ptr(o2m_access_index));
                        const float64 curr_matset_value  = temp.to_float64();
                        elem_matset_values_maps[mat_elem][mat_id] = curr_matset_value;
                    }
                }
            }
        }
    }

    index_t matset_num_slots = 0;
    for(const std::map<index_t, float64> &elem_mat_map: elem_mat_maps)
    {
        matset_num_slots += (elem_mat_map.size() > 1) ? elem_mat_map.size() : 0;
    }
    const index_t mset_num_slots = matset_num_slots;

    // Generate Silo Data Structures //

    dest.reset();
    dest["topology"].set(matset["topology"]);
    // in some cases, this method will sort the material names
    // so always include the material map
    dest["material_map"].set(matset_mat_map);
    dest["matlist"].set(DataType(int_dtype.id(), mset_num_elems));
    dest["mix_next"].set(DataType(int_dtype.id(), mset_num_slots));
    dest["mix_mat"].set(DataType(int_dtype.id(), mset_num_slots));
    dest["mix_vf"].set(DataType(float_dtype.id(), mset_num_slots));

    if(xform_matset_values)
    {
        dest["field_mixvar_values"].set(DataType(float_dtype.id(), mset_num_slots));
        if(field.has_child("values"))
        {
            dest["field_values"].set(field["values"]);
        }
    }

    for(index_t elem_index = 0, slot_index = 0; elem_index < mset_num_elems; elem_index++)
    {
        const std::map<index_t, float64>& elem_mat_map = elem_mat_maps[elem_index];
        CONDUIT_ASSERT(elem_mat_map.size() != 0, "A zone has no materials.");
        if (elem_mat_map.size() == 1)
        {
            temp.reset();
            temp.set(elem_mat_map.begin()->first);
            data.set_external(int_dtype, dest["matlist"].element_ptr(elem_index));
            temp.to_data_type(int_dtype.id(), data);
        }
        else
        {
            index_t next_slot_index = slot_index;
            for(const auto& zone_mix_mat : elem_mat_map)
            {
                temp.reset();
                temp.set(zone_mix_mat.first);
                data.set_external(int_dtype, dest["mix_mat"].element_ptr(next_slot_index));
                temp.to_data_type(int_dtype.id(), data);

                // also do matset_values if passed
                // elem_index ==> element index
                // zone_mix_mat.first ==> material index
                // process matset values if passed
                if(xform_matset_values)
                {
                    temp.reset();
                    temp.set(elem_matset_values_maps[elem_index][zone_mix_mat.first]);
                    data.set_external(float_dtype, dest["field_mixvar_values"].element_ptr(next_slot_index));
                    temp.to_data_type(float_dtype.id(), data);
                }

                temp.reset();
                temp.set(zone_mix_mat.second);
                data.set_external(float_dtype, dest["mix_vf"].element_ptr(next_slot_index));
                temp.to_data_type(float_dtype.id(), data);

                temp.reset();
                temp.set(next_slot_index + 1 + 1);
                data.set_external(int_dtype, dest["mix_next"].element_ptr(next_slot_index));
                temp.to_data_type(int_dtype.id(), data);

                ++next_slot_index;
            }

            temp.reset();
            temp.set(0);
            data.set_external(int_dtype, dest["mix_next"].element_ptr(next_slot_index - 1));
            temp.to_data_type(int_dtype.id(), data);


            temp.reset();
            temp.set(~slot_index);
            data.set_external(int_dtype, dest["matlist"].element_ptr(elem_index));
            temp.to_data_type(int_dtype.id(), data);

            slot_index += elem_mat_map.size();
        }
    }

    // extra hooks for downstream data consumers

    dest["buffer_style"] = mesh::matset::is_multi_buffer(matset) ? "multi" : "uni";
    dest["dominance"] = mesh::matset::is_element_dominant(matset) ? "element" : "material";
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
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        mat_ids.push_back(m_acc.get_mat_id(elem_idx, mat_idx));
        vol_fracs.push_back(m_acc.get_vol_frac(elem_idx, mat_idx));
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
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        matset_values.push_back(m_acc.get_mset_val(elem_idx, mat_idx));
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
    // create the species_names
    Node &species_names = dest_specset["species_names"];
    specset::create_or_copy_species_names(src_specset, species_names);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_elems = m_acc.num_elems();

    std::vector<float64> matset_values;
    dest_specset["sizes"].set(DataType::index_t(num_elems));
    index_t_array sizes = dest_specset["sizes"].value();
    dest_specset["offsets"].set(DataType::index_t(num_elems));
    index_t_array offsets = dest_specset["offsets"].value();

    index_t offset = 0;
    // for each species mass fraction
    auto for_each_species_value = [&](const index_t elem_idx,
                                      const index_t mat_idx,
                                      const index_t spec_idx)
    {
        matset_values.push_back(m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx));
    };

    auto for_each_value = [](const index_t, const index_t, const index_t){};

    auto for_each_element = [&](const index_t elem_idx,
                                const index_t,
                                const index_t nspec_in_elem)
    {
        // save the size and offset information
        sizes[elem_idx] = nspec_in_elem;
        offsets[elem_idx] = offset;
        offset += nspec_in_elem;
    };

    walk_matset_species_by_element(m_acc,
                                   for_each_species_value,
                                   for_each_value,
                                   for_each_element,
                                   epsilon);

    dest_specset["matset_values"].set(matset_values);
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
    // create container for new matset vals
    Node &new_mset_vals = dest_field["matset_values"];

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);
    const index_t num_mats = m_acc.num_mats();
    const index_t num_elems = m_acc.num_elems();

    std::vector<float64_array> new_mset_vals_vec(num_mats);
    // initialize sizes
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        new_mset_vals[matname].set(DataType::float64(num_elems));
        new_mset_vals_vec[mat_order_id] = new_mset_vals[matname].as_float64_array();
        new_mset_vals_vec[mat_order_id].fill(0.0);
    }

    // what we will do for each mat_id/mset_val we encounter
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        new_mset_vals_vec[mat_order_id][elem_idx] = m_acc.get_mset_val(elem_idx, mat_idx);
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
    Node &new_mset_vals = dest_specset["matset_values"];

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_mats = m_acc.num_mats();
    const index_t num_elems = m_acc.num_elems();

    std::vector<std::vector<float64_array>> new_mset_vals_vec(num_mats);
    // initialize sizes of the matset values arrays
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        if (src_specset["species_names"].has_child(matname))
        {
            const std::vector<std::string> &specnames_for_mat = 
                src_specset["species_names"][matname].child_names();

            const index_t num_spec_for_mat = static_cast<index_t>(specnames_for_mat.size());
            for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
            {
                const std::string &specname = specnames_for_mat[spec_idx];

                new_mset_vals[matname][specname].set(DataType::float64(num_elems));
                new_mset_vals_vec[mat_order_id].push_back(
                    new_mset_vals[matname][specname].as_float64_array());
                new_mset_vals_vec[mat_order_id][spec_idx].fill(0.0);
            }
        }
    }

    // for each species mass fraction
    auto for_each_species_value = [&](const index_t elem_idx,
                                      const index_t mat_idx,
                                      const index_t spec_idx)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        const float64 spec_mf = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);
        new_mset_vals_vec[mat_order_id][spec_idx][elem_idx] = spec_mf;
    };
    auto for_each_value = [](const index_t, const index_t, const index_t){};
    auto for_each_element = [](const index_t, const index_t, const index_t){};
    walk_matset_species_by_element(m_acc,
                                   for_each_species_value,
                                   for_each_value,
                                   for_each_element);
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_matset(const conduit::Node &src_matset,
                                                         conduit::Node &dest_matset)
{
    // copy material map since we have it
    dest_matset["material_map"].set(src_matset["material_map"]);

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_mats = m_acc.num_mats();

    std::vector<std::vector<float64>> new_vol_fracs_vec(num_mats);
    std::vector<std::vector<index_t>> new_elem_ids_vec(num_mats);

    // what we will do for each mat_id/vol_frac we encounter
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        new_vol_fracs_vec[mat_order_id].push_back(m_acc.get_vol_frac(elem_idx, mat_idx));
        new_elem_ids_vec[mat_order_id].push_back(elem_idx);
    };
    walk_matset_value_by_element(m_acc, for_each_value);

    // create containers for new vol fracs and elem ids
    Node &new_vol_fracs = dest_matset["volume_fractions"];
    Node &new_elem_ids = dest_matset["element_ids"];
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        new_vol_fracs[matname].set(new_vol_fracs_vec[mat_order_id]);
        new_elem_ids[matname].set(new_elem_ids_vec[mat_order_id]);
    }
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_field(const conduit::Node &src_matset,
                                                        const conduit::Node &src_field,
                                                        conduit::Node &dest_field)
{
    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);
    const index_t num_mats = m_acc.num_mats();

    // create container for new matset vals
    std::vector<std::vector<float64>> new_mset_vals_vec(num_mats);

    // what we will do for each mat_id/mset_val we encounter
    auto for_each_value = [&](const index_t elem_idx,
                              const index_t mat_idx,
                              const index_t)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        new_mset_vals_vec[mat_order_id].push_back(m_acc.get_mset_val(elem_idx, mat_idx));
    };
    walk_matset_value_by_element(m_acc, for_each_value);

    // create containers for new vol fracs and elem ids
    Node &new_mset_vals = dest_field["matset_values"];
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        new_mset_vals[matname].set(new_mset_vals_vec[mat_order_id]);
    }
}

//-----------------------------------------------------------------------------
// venn sparse by element -> sparse by material
void
uni_buffer_by_element_to_multi_buffer_by_material_specset(const conduit::Node &src_matset,
                                                          const conduit::Node &src_specset,
                                                          conduit::Node &dest_specset)
{
    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_mats = m_acc.num_mats();

    // create container for new matset vals
    // index [mat_order_id][spec_idx][mass fraction id]
    std::vector<std::vector<std::vector<float64>>> new_mset_vals_vec(num_mats);
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        if (src_specset["species_names"].has_child(matname))
        {
            const index_t num_spec_for_mat = 
                src_specset["species_names"][matname].number_of_children();
            new_mset_vals_vec[mat_order_id].resize(num_spec_for_mat);
        }
    }

    // for each species mass fraction
    auto for_each_species_value = [&](const index_t elem_idx,
                                      const index_t mat_idx,
                                      const index_t spec_idx)
    {
        const index_t mat_order_id = m_acc.get_mat_order_id(elem_idx, mat_idx);
        const float64 spec_mf = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);
        new_mset_vals_vec[mat_order_id][spec_idx].push_back(spec_mf);
    };
    auto for_each_value = [](const index_t, const index_t, const index_t){};
    auto for_each_element = [](const index_t, const index_t, const index_t){};
    walk_matset_species_by_element(m_acc,
                                   for_each_species_value,
                                   for_each_value,
                                   for_each_element);

    // create containers for new vol fracs and elem ids
    Node &new_mset_vals = dest_specset["matset_values"];
    for (index_t mat_order_id = 0; mat_order_id < num_mats; mat_order_id ++)
    {
        const std::string &matname = src_matset["material_map"].child(mat_order_id).name();
        if (src_specset["species_names"].has_child(matname))
        {
            const std::vector<std::string> &specnames_for_mat = 
                src_specset["species_names"][matname].child_names();

            const index_t num_spec_for_mat = static_cast<index_t>(specnames_for_mat.size());
            for (index_t spec_idx = 0; spec_idx < num_spec_for_mat; spec_idx ++)
            {
                const std::string &specname = specnames_for_mat[spec_idx];
                new_mset_vals[matname][specname].set(new_mset_vals_vec[mat_order_id][spec_idx]);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// venn full -> sparse_by_material
void
multi_buffer_by_element_to_multi_buffer_by_material_matset(const conduit::Node &src_matset,
                                                           conduit::Node &dest_matset,
                                                           const float64 epsilon)
{
    Node material_map;
    if (src_matset.has_child("material_map"))
    {
        dest_matset["material_map"].set(src_matset["material_map"]);
        material_map.set_external(dest_matset["material_map"]);
    }
    else
    {
        create_or_reuse_material_map(src_matset, material_map);
    }

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_elems = m_acc.num_elems();

    Node n;
    n["local_element_ids"].set(DataType::index_t(num_elems));
    n["local_volume_fractions"].set(DataType::float64(num_elems));
    index_t_array local_element_ids = n["local_element_ids"].value();
    float64_array local_volume_fractions = n["local_volume_fractions"].value();

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t eid_id)
    {
        local_element_ids[eid_id] = elem_idx;
        local_volume_fractions[eid_id] = m_acc.get_vol_frac(elem_idx, mat_idx);
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
    Node material_map;
    create_or_reuse_material_map(src_matset, material_map);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);
    const index_t num_elems = m_acc.num_elems();

    Node n;
    n["local_matset_values"].set(DataType::float64(num_elems));
    float64_array local_matset_values = n["local_matset_values"].value();

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t eid_id)
    {
        local_matset_values[eid_id] = m_acc.get_mset_val(elem_idx, mat_idx);
    };

    // what we will do for each material's mset_vals
    auto for_each_material = [&](const index_t mat_idx,
                                 const index_t num_elems_for_mat)
    {
        const std::string matname = material_map.child(mat_idx).name();
        dest_field["matset_values"][matname].set(DataType::float64(num_elems_for_mat));
        float64_array matset_values = dest_field["matset_values"][matname].value();
        for (index_t eid_id = 0; eid_id < num_elems_for_mat; eid_id ++)
        {
            matset_values[eid_id] = local_matset_values[eid_id];
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
    Node material_map;
    create_or_reuse_material_map(src_matset, material_map);

    Node species_names;
    specset::create_or_reuse_species_names(src_specset, species_names);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_elems = m_acc.num_elems();

    Node n;
    n["spec_mf"].set(DataType::float64(num_elems));
    float64_array mset_vals = n["spec_mf"].value();

    auto for_each_element_value = [&](const index_t mat_idx,
                                      const index_t spec_idx,
                                      const index_t elem_idx,
                                      const index_t curr_elem_count)
    {
        mset_vals[curr_elem_count] = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);
    };
    auto for_each_material_species = [&](const index_t mat_idx,
                                         const index_t spec_idx,
                                         const index_t num_elems_for_spec)
    {
        const std::string matname = material_map.child(mat_idx).name();
        const std::string specname = species_names[matname].child(spec_idx).name();
        dest_specset["matset_values"][matname][specname].set(DataType::float64(num_elems_for_spec));
        float64_array new_mset_vals = dest_specset["matset_values"][matname][specname].value();
        for (index_t eid_id = 0; eid_id < num_elems_for_spec; eid_id ++)
        {
            new_mset_vals[eid_id] = mset_vals[eid_id];
        }
    };
    auto for_each_material = [](const index_t, const index_t){};
    walk_matset_element_by_material_species(m_acc,
                                            for_each_element_value,
                                            for_each_material_species,
                                            for_each_material,
                                            epsilon);
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

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_mats = m_acc.num_mats();
    const index_t num_elems = m_acc.num_elems();

    // index [mat_idx] gives you the volume fraction array for that material
    std::vector<float64_array> mat_idx_to_data(num_mats);

    // create the output data arrays and save a pointer to each one
    const std::vector<std::string> &matnames = src_matset["volume_fractions"].child_names();
    for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
    {
        const std::string &matname = matnames[mat_idx];
        dest_matset["volume_fractions"][matname].set(DataType::float64(num_elems));
        mat_idx_to_data[mat_idx] = dest_matset["volume_fractions"][matname].value();
        mat_idx_to_data[mat_idx].fill(0.0);
    }

    // what we will do for each vol_frac/elem_id pair
    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        mat_idx_to_data[mat_idx][real_elem_id] = m_acc.get_vol_frac(elem_idx, mat_idx);
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
    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);
    const index_t num_mats = m_acc.num_mats();
    const index_t num_elems = m_acc.num_elems();

    // index [mat_idx] gives you the matset values array for that material
    std::vector<float64_array> mat_idx_to_data(num_mats);

    // create the output data arrays and save a pointer to each one
    const std::vector<std::string> &matnames = src_matset["volume_fractions"].child_names();
    for (index_t mat_idx = 0; mat_idx < num_mats; mat_idx ++)
    {
        const std::string &matname = matnames[mat_idx];
        dest_field["matset_values"][matname].set(DataType::float64(num_elems));
        mat_idx_to_data[mat_idx] = dest_field["matset_values"][matname].value();
        mat_idx_to_data[mat_idx].fill(0.0);
    }

    // what we will do for each mset_val
    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        mat_idx_to_data[mat_idx][real_elem_id] = m_acc.get_mset_val(elem_idx, mat_idx);
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
    Node material_map;
    create_or_reuse_material_map(src_matset, material_map);

    Node species_names;
    specset::create_or_reuse_species_names(src_specset, species_names);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_elems = m_acc.num_elems();

    Node n;
    n["spec_mf"].set(DataType::float64(num_elems));
    float64_array mset_vals = n["spec_mf"].value();
    mset_vals.fill(0.0);

    auto for_each_element_value = [&](const index_t mat_idx,
                                      const index_t spec_idx,
                                      const index_t elem_idx,
                                      const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        mset_vals[real_elem_id] = m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx);
    };
    auto for_each_material_species = [&](const index_t mat_idx,
                                         const index_t spec_idx,
                                         const index_t)
    {
        const std::string matname = material_map.child(mat_idx).name();
        const std::string specname = species_names[matname].child(spec_idx).name();
        dest_specset["matset_values"][matname][specname].set(mset_vals);
        mset_vals.fill(0.0);
    };
    auto for_each_material = [](const index_t, const index_t){};
    walk_matset_element_by_material_species(m_acc,
                                            for_each_element_value,
                                            for_each_material_species,
                                            for_each_material);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> sparse by element
void
multi_buffer_by_material_to_uni_buffer_by_element_matset(const conduit::Node &src_matset,
                                                         conduit::Node &dest_matset)
{
    Node &material_map = dest_matset["material_map"];
    create_or_copy_material_map(src_matset, material_map);

    MatsetAccessor m_acc = MatsetAccessor(src_matset);
    const index_t num_elems = m_acc.num_elems();

    // There is no way to pack the volume fractions correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which volume fractions are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_vol_fracs(num_elems);
    std::vector<std::vector<index_t>> intermediate_mat_ids(num_elems);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        intermediate_mat_ids[real_elem_id].push_back(m_acc.get_mat_id(elem_idx, mat_idx));
        intermediate_vol_fracs[real_elem_id].push_back(m_acc.get_vol_frac(elem_idx, mat_idx));
    };
    walk_matset_value_by_material(m_acc, for_each_value);

    std::vector<float64> vol_fracs;
    std::vector<index_t> mat_ids;
    dest_matset["sizes"].set(DataType::index_t(num_elems));
    index_t_array sizes = dest_matset["sizes"].value();
    dest_matset["offsets"].set(DataType::index_t(num_elems));
    index_t_array offsets = dest_matset["offsets"].value();

    // final pass
    index_t offset = 0;
    for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
    {
        const index_t nmats = 
            static_cast<index_t>(intermediate_vol_fracs[elem_idx].size());
        for (index_t mat_vf_id = 0; mat_vf_id < nmats; mat_vf_id ++)
        {
            vol_fracs.push_back(intermediate_vol_fracs[elem_idx][mat_vf_id]);
            mat_ids.push_back(intermediate_mat_ids[elem_idx][mat_vf_id]);
        }
        sizes[elem_idx] = nmats;
        offsets[elem_idx] = offset;
        offset += nmats;
    }

    dest_matset["volume_fractions"].set(vol_fracs);
    dest_matset["material_ids"].set(mat_ids);
}

//-----------------------------------------------------------------------------
// venn sparse by material -> sparse by element
void
multi_buffer_by_material_to_uni_buffer_by_element_field(const conduit::Node &src_matset,
                                                        const conduit::Node &src_field,
                                                        conduit::Node &dest_field)
{
    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_field);
    const index_t num_elems = m_acc.num_elems();

    // There is no way to pack the matset values correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which matset values are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_mset_vals(num_elems);

    auto for_each_value = [&](const index_t mat_idx,
                              const index_t elem_idx,
                              const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        intermediate_mset_vals[real_elem_id].push_back(m_acc.get_mset_val(elem_idx, mat_idx));
    };
    walk_matset_value_by_material(m_acc, for_each_value);

    std::vector<float64> mset_vals;

    // final pass
    for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
    {
        const index_t nmats = 
            static_cast<index_t>(intermediate_mset_vals[elem_idx].size());
        for (index_t mat_vf_id = 0; mat_vf_id < nmats; mat_vf_id ++)
        {
            mset_vals.push_back(intermediate_mset_vals[elem_idx][mat_vf_id]);
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
    Node &species_names = dest_specset["species_names"];
    specset::create_or_copy_species_names(src_specset, species_names);

    MatsetAccessor m_acc = MatsetAccessor(src_matset, src_specset);
    const index_t num_elems = m_acc.num_elems();

    // There is no way to pack the matset values correctly without
    // first knowing the sizes. So we create an intermediate representation
    // in which matset values are packed by element. Later we smooth this out.
    std::vector<std::vector<float64>> intermediate_mset_vals(num_elems);

    auto for_each_element_value = [&](const index_t mat_idx,
                                      const index_t spec_idx,
                                      const index_t elem_idx,
                                      const index_t)
    {
        const index_t real_elem_id = m_acc.get_elem_id(elem_idx, mat_idx);
        intermediate_mset_vals[real_elem_id].push_back(
            m_acc.get_mass_frac(elem_idx, mat_idx, spec_idx));
    };
    auto for_each_material_species = [](const index_t, const index_t, const index_t){};
    auto for_each_material = [](const index_t, const index_t){};
    walk_matset_element_by_material_species(m_acc,
                                            for_each_element_value,
                                            for_each_material_species,
                                            for_each_material);

    std::vector<float64> mset_vals;
    dest_specset["sizes"].set(DataType::index_t(num_elems));
    index_t_array sizes = dest_specset["sizes"].value();
    dest_specset["offsets"].set(DataType::index_t(num_elems));
    index_t_array offsets = dest_specset["offsets"].value();

    // final pass
    index_t offset = 0;
    for (index_t elem_idx = 0; elem_idx < num_elems; elem_idx ++)
    {
        const index_t nspecs = static_cast<index_t>(intermediate_mset_vals[elem_idx].size());
        for (index_t mat_vf_id = 0; mat_vf_id < nspecs; mat_vf_id ++)
        {
            mset_vals.push_back(intermediate_mset_vals[elem_idx][mat_vf_id]);
        }
        sizes[elem_idx] = nspecs;
        offsets[elem_idx] = offset;
        offset += nspecs;
    }

    dest_specset["matset_values"].set(mset_vals);
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

    conduit::Node field;

    detail::to_silo(field,
                    matset,
                    dest,
                    epsilon);
}

//-----------------------------------------------------------------------------
// TODO I want this function gone
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
                          "material-dominant uni-buffer material set is unsupported.");
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
count_elements_from_matset(const conduit::Node &matset)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::count_elements_from_matset"
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
            o2mrelation::O2MIndex o2m_idx = o2mrelation::O2MIndex(matset);
            return o2m_idx.size();
        }
    }
    else
    {
        // venn sparse by material
        if (multi_buffer)
        {
            // take the maximum element id
            index_t running_max = 0;

            auto eid_itr = matset["element_ids"].children();
            while (eid_itr.has_next())
            {
                const Node &mat_elem_ids = eid_itr.next();
                index_t_accessor mat_elem_ids_vals = mat_elem_ids.value();
                const index_t num_vf = mat_elem_ids_vals.dtype().number_of_elements();
                for (index_t i = 0; i < num_vf; i ++)
                {
                    const index_t element_id = mat_elem_ids_vals[i];
                    running_max = std::max(running_max, element_id + 1);
                }
            }

            return running_max;
        }
        // material-dominant uni-buffer
        else
        {
            CONDUIT_ERROR("blueprint::mesh::matset::count_elements_from_matset() "
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
                    const index_t elem_id,
                    const float64 epsilon)
{
    // extra seat belt here
    if (! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::matset::is_material_in_zone"
                      " passed matset node must be a valid matset tree.");
    }

    if (is_uni_buffer(matset))
    {
        if (is_element_dominant(matset))
        {
            if (matset["material_map"].has_child(matname))
            {
                const index_t mat_id = matset["material_map"][matname].to_index_t();
                MatsetAccessor m_acc = MatsetAccessor(matset);
                const index_t num_mats_in_elem = m_acc.num_mats_for_elem(elem_id);
                for (index_t mat_idx = 0; mat_idx < num_mats_in_elem; mat_idx ++)
                {
                    const index_t curr_mat_id = m_acc.get_mat_id(elem_id, mat_idx);
                    if (curr_mat_id == mat_id)
                    {
                        // we found the right material in this zone
                        return true;
                    }
                }
                // not found in this zone
                return false;
            }
            else
            {
                // obviously the material is not present in the zone; it is not
                // present in the matset
                return false;
            }
        }
        else // material-dominant
        {
            // unsupported uni-buffer by material
            CONDUIT_ERROR("conduit::blueprint::mesh::matset::is_material_in_zone() "
                          "material-dominant uni-buffer material set is unsupported.");
            return false;
        }
    }
    else // multi-buffer
    {
        if (is_element_dominant(matset))
        {
            // full
            if (matset["volume_fractions"].has_child(matname))
            {
                const float64_accessor vfs = matset["volume_fractions"][matname].value();
                return vfs[elem_id] > epsilon;
            }
            else
            {
                // obviously the material is not present in the zone; it is not
                // present in the matset
                return false;
            }
        }
        else // material-dominant
        {
            // sparse_by_material
            if (matset["element_ids"].has_child(matname))
            {
                const index_t_accessor elem_ids = matset["element_ids"][matname].value();
                return elem_ids.count(elem_id) > 0;
            }
            else
            {
                // obviously the material is not present in the zone; it is not
                // present in the matset
                return false;
            }
        }
    }
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
        conduit::Node &dest)
{
    if(! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_silo passed specset node "
                      "must be a valid specset tree.");
    }

    if(! matset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::to_silo passed matset node "
                      "must be a valid matset tree or a valid intermediate silo "
                      "representation of a matset.");
    }

    // need to check if passed matset is already in the silo rep
    Node silo_matset;

    if (! (matset.has_child("topology") && 
           matset.has_child("material_map") &&
           matset.has_child("mix_vf") && 
           matset.has_child("mix_mat") &&
           matset.has_child("mix_next") &&
           matset.has_child("matlist") &&
           matset.has_child("buffer_style") &&
           matset.has_child("dominance")))
    {
        // if not, create a silo rep
        conduit::blueprint::mesh::matset::to_silo(matset, silo_matset);
    }
    else
    {
        // if it is, use it and continue
        silo_matset.set_external(matset);
    }

    // TODO change this once we write specset converters
    // I think the right path will be to rewrite this function using the 
    // sparse by element (uni_buffer element_dominant) specset flavor.
    // So we will convert all specsets to that form and then convert to silo.
    // Should be simpler and get rid of a lot of the indexing madness.
    if (silo_matset["buffer_style"].as_string() != "multi")
    {
        CONDUIT_ERROR("TODO cannot handle uni buffer specsets");
    }
    if (silo_matset["dominance"].as_string() != "element")
    {
        CONDUIT_ERROR("TODO cannot handle material dominant specsets");
    }

    const int nmat = silo_matset["material_map"].number_of_children();
    CONDUIT_ASSERT(nmat >= specset["matset_values"].number_of_children(),
        "blueprint::mesh::specset::to_silo number of materials in the matset "
        "must be greater than or equal to the number of materials in the specset.");

    auto matmap_itr = silo_matset["material_map"].children();
    int matmap_index = 0;
    // Map actual material numbers to indicies into the material map
    // We need this map so that, no matter what material numbers we get thrown at us,
    // we can figure out their order in the material map for when we calculate
    // species indices.
    std::map<int, int> mat_id_to_array_index;
    while (matmap_itr.has_next())
    {
        const Node &matmap_entry = matmap_itr.next();
        mat_id_to_array_index[matmap_entry.as_int()] = matmap_index;
        matmap_index ++;
    }

    //
    // set nmatspec and specnames arrays
    //
    dest["nmatspec"].set(DataType::index_t(nmat));
    index_t_array nmatspec = dest["nmatspec"].value();
    // we have to be very careful to always go in the order of the material map
    int matmap_idx = 0;
    matmap_itr.to_front();
    while (matmap_itr.has_next())
    {
        matmap_itr.next();
        const std::string matname = matmap_itr.name();

        // is this material present in the specset?
        if (specset["matset_values"].has_child(matname))
        {
            const Node &individual_mat_spec = specset["matset_values"][matname];
            // get the number of species for this material
            const int num_species_for_this_material = individual_mat_spec.number_of_children();
            // save the number of species for this material in the output
            nmatspec[matmap_idx] = num_species_for_this_material;

            // get the specie names for this material and add to the specnames.
            // the specnames array is the length of the sum of the nmatspec array
            // so for all materials with species, the species names will appear
            // in this list in order.
            auto spec_itr = individual_mat_spec.children();
            while (spec_itr.has_next())
            {
                spec_itr.next();
                const std::string specname = spec_itr.name();
                Node &specname_entry = dest["specnames"].append();
                specname_entry.set(specname);
            }
        }
        else
        {
            // if this material has no species, then we set to zero.
            nmatspec[matmap_idx] = 0;
        }

        matmap_idx ++;
    }

    // we sum up the nmatspec to get the number of species across all materials
    const int num_species_across_mats = nmatspec.sum();

    // we have to go in order by zones as they appear

    // first we need number of zones
    const int num_zones = silo_matset["matlist"].dtype().number_of_elements();

    // TODO
    // I may wish to go through and check if the material is even in the zone
    // to avoid writing unneeded data
    // that could be expensive though

    // The function silo_write_specset() in conduit_relay_io_silo.cpp
    // depends on this being a float64. If we change this here,
    // we must also change it there.
    std::vector<float64> species_mf;
    
    // need to iterate across all species for all materials at once
    for (int zone_id = 0; zone_id < num_zones; zone_id ++)
    {
        // we must iterate using the material map since it has the "correct"
        // ordering of materials. Ordering may be different for the specset.
        // We choose the material map order to be the one source of truth.
        matmap_itr.to_front();
        while (matmap_itr.has_next())
        {
            matmap_itr.next();
            const std::string matname = matmap_itr.name();

            // is this material present in the specset?
            if (specset["matset_values"].has_child(matname))
            {
                // if so, we just load all the species mass fractions in
                const Node &individual_mat_spec = specset["matset_values"][matname];
                // iterate through each specie
                auto spec_itr = individual_mat_spec.children();
                while (spec_itr.has_next())
                {
                    const Node &spec = spec_itr.next();
                    float64_accessor species_mass_fractions = spec.value();
                    // grab the specie mass fraction for this zone id
                    species_mf.push_back(species_mass_fractions[zone_id]);
                }
            }
        }
    }

    const int nspecies_mf = static_cast<int>(species_mf.size());

    // get pointers to the silo material representation data
    const int_accessor silo_matlist = silo_matset["matlist"].value();
    const int_accessor silo_mix_mat = silo_matset["mix_mat"].value();
    const int_accessor silo_mix_next = silo_matset["mix_next"].value();

    auto calculate_species_index = [&](const int zone_id, const int mat_index)
    {
        // To get the value for the speclist for this zone, we must determine
        // the correct 1-index in the species_mf array that corresponds to the 
        // material in this zone. We have organized the species_mf array such 
        // that there are entries for each material's species for each zone,
        // even if those materials are not present in that zone. Thus there are
        // the same number of species entries for each zone in the species_mf
        // array. So we need to determine what I am calling an "outer_index" 
        // that tells us the starting index of the current zone in the species_mf
        // array.

        // how many entries per zone? Use the calculated num_species_across_mats
        const int outer_index = zone_id * num_species_across_mats;

        // Next we need the inner or "local_index", which corresponds to the 
        // starting 1-index of the relevant material's species within this zone.
        // We can use the nmatspec array to determine where that starts for our
        // given material, which we fetch via material number, which we have used
        // to get an index into the nmatspec array.

        // We wish to offset the local index by 1, hence starting from 1 when we take the sum.

        // local index is the number of species for each material
        // BEFORE this material plus 1, since it is 1 indexed.
        // So if mat0 has 2 species and mat1 has 3 species, then
        // the 1-index start of mat2 will be 2 + 3 + 1 = 6.

        const int local_index = [&]()
        {
            int sum = 1;
            for (index_t i = 0; i < mat_index; i ++)
            {
                sum += nmatspec[i];
            }
            return sum;
        }();

        // we save the final index for this zone
        return outer_index + local_index;

        // This can produce an out of bounds index in very specific cases.
        // If a material has no species, the index produced by this function is 
        // useless, but downstream data consumers shouldn't be reading the index
        // anyway. If a material has no species and it is the last one in the 
        // material map and the final zone is mixed and contains that material,
        // then we can get an index that is out of bounds. This is ok because 
        // downstream tools like VisIt read based on the number of species, so
        // even though the index is garbage it goes unused.
    };

    dest["speclist"].set(DataType::int64(num_zones));
    int64_array speclist = dest["speclist"].value();
    std::vector<int> mix_spec;

    // now we create the speclist and mix_spec arrays, traversing through the zones
    for (int zone_id = 0; zone_id < num_zones; zone_id ++)
    {
        const int matlist_entry = silo_matlist[zone_id];
        // is this zone clean?
        if (matlist_entry >= 0) // this relies on matset_ptr->allowmat0 == 0
        {
            // clean

            // I can use the material number to determine which part of the speclist to index into
            const int &matno = matlist_entry;
            const int mat_index = mat_id_to_array_index[matno];
            if (nmatspec[mat_index] == 1)
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
                speclist[zone_id] = calculate_species_index(zone_id, mat_index);
            }
        }
        else
        {
            // mixed

            // We don't need to compute this as it is the same as the 
            // matlist entry.
            // We save the negated 1-index into the mix_spec array
            speclist[zone_id] = matlist_entry;

            // for mixed zones, the numbers in the speclist are negated 1-indices into
            // the silo mixed data arrays. To turn them into zero-indices, we must add
            // 1 and negate the result. Example:
            // indices: -1 -2 -3 -4 ...
            // become:   0  1  2  3 ...

            int mix_id = -1 * (matlist_entry + 1);

            // when silo_mix_next[mix_id] is 0, we are on the last one
            while (mix_id >= 0)
            {                
                // I can use the material number to determine which part of the speclist to index into
                const int matno = silo_mix_mat[mix_id];
                const int mat_index = mat_id_to_array_index[matno];
                if (nmatspec[mat_index] == 1)
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
                    mix_spec.push_back(calculate_species_index(zone_id, mat_index));
                }

                // since mix_id is a 1-index, we must subtract one
                // this makes sure that mix_id = 0 is the last case,
                // since it will make our mix_id == -1, which ends
                // the while loop.
                mix_id = silo_mix_next[mix_id] - 1;
            }
        }
    }

    // get the length of the mixed data arrays
    const int mixlen = static_cast<int>(mix_spec.size());

    // number of materials
    dest["nmat"] = nmat;
    
    // number of species associated with each material
    // we already saved dest["nmatspec"]
    
    // indices into species_mf and mix_spec
    // we already saved dest["speclist"]
    
    // length of the species_mf array
    dest["nspecies_mf"] = nspecies_mf;
    
    // mass fractions of the matspecies in an array of length nspecies_mf
    dest["species_mf"].set(species_mf);
    
    // array of length mixlen containing indices into the species_mf array
    dest["mix_spec"].set(mix_spec);
    
    // length of mix_spec array
    dest["mixlen"] = mixlen;
    
    // species names
    // we already saved species names
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
// this will use set external if the species_names already exist
void
create_or_reuse_species_names(const conduit::Node &specset,
                             conduit::Node &species_names)
{
    // extra seat belt here
    if (! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::create_or_reuse_species_names"
                      " passed specset node must be a valid specset tree.");
    }

    species_names.reset();

    if (specset.has_child("species_names"))
    {
        species_names.set_external(specset["species_names"]);
    }
    else
    {
        conduit::blueprint::mesh::matset::detail::create_species_names(specset, species_names);
    }
}

//-------------------------------------------------------------------------
// this will use set if the species_names already exist
void
create_or_copy_species_names(const conduit::Node &specset,
                            conduit::Node &species_names)
{
    // extra seat belt here
    if (! specset.dtype().is_object())
    {
        CONDUIT_ERROR("blueprint::mesh::specset::create_or_copy_species_names"
                      " passed specset node must be a valid specset tree.");
    }

    species_names.reset();

    if (specset.has_child("species_names"))
    {
        species_names.set(specset["species_names"]);
    }
    else
    {
        conduit::blueprint::mesh::matset::detail::create_species_names(specset, species_names);
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

    conduit::blueprint::mesh::matset::detail::to_silo(field,
                                                      matset,
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

