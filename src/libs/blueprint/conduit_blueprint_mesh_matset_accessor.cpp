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
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_nmats_for_zone(&MatsetAccessor::get_error_nmats_for_zone),
   m_get_nzones_for_mat(&MatsetAccessor::get_error_nzones_for_mat)
{
// empty //
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_nmats_for_zone(&MatsetAccessor::get_error_nmats_for_zone),
   m_get_nzones_for_mat(&MatsetAccessor::get_error_nzones_for_mat)
{
    init(matset, nullptr, nullptr);
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset,
                               const Node &specset_or_field)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_nmats_for_zone(&MatsetAccessor::get_error_nmats_for_zone),
   m_get_nzones_for_mat(&MatsetAccessor::get_error_nzones_for_mat)
{
    if (specset_or_field.has_child("topology"))
    {
        // then it is a field
        const Node &field = specset_or_field;
        init(matset, &field, nullptr);
    }
    else
    {
        // then it is a specset
        const Node &specset = specset_or_field;
        init(matset, nullptr, &specset);
    }
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const Node &matset,
                               const Node &field,
                               const Node &specset)
 : m_get_mat_id(&MatsetAccessor::get_error_mat_id),
   m_get_elem_id(&MatsetAccessor::get_error_elem_id),
   m_get_vol_frac(&MatsetAccessor::get_error_vol_frac),
   m_get_mset_val(&MatsetAccessor::get_error_mset_val),
   m_get_nmats_for_zone(&MatsetAccessor::get_error_nmats_for_zone),
   m_get_nzones_for_mat(&MatsetAccessor::get_error_nzones_for_mat)
{
    init(matset, &field, &specset);
}

//---------------------------------------------------------------------------//
MatsetAccessor::MatsetAccessor(const MatsetAccessor &m_acc)
: m_get_mat_id(m_acc.m_get_mat_id),
  m_get_elem_id(m_acc.m_get_elem_id),
  m_get_vol_frac(m_acc.m_get_vol_frac),
  m_get_mset_val(m_acc.m_get_mset_val),
  m_get_nmats_for_zone(m_acc.m_get_nmats_for_zone),
  m_get_nzones_for_mat(m_acc.m_get_nzones_for_mat),
  m_multi_vol_fracs(m_acc.m_multi_vol_fracs),
  m_multi_mset_vals(m_acc.m_multi_mset_vals),
  m_multi_mat_idx_map(m_acc.m_multi_mat_idx_map),
  m_multi_mat_idx_map_acc(m_acc.m_multi_mat_idx_map_acc),
  m_sbm_elem_ids(m_acc.m_sbm_elem_ids),
  m_sbe_material_ids(m_acc.m_sbe_material_ids),
  m_sbe_vol_fracs(m_acc.m_sbe_vol_fracs),
  m_sbe_mset_vals(m_acc.m_sbe_mset_vals),
  m_sbe_o2m_idx(m_acc.m_sbe_o2m_idx)
{ }

//---------------------------------------------------------------------------//
MatsetAccessor &
MatsetAccessor::operator=(const MatsetAccessor &m_acc)
{
    if (this != &m_acc)
    {
        m_get_mat_id = m_acc.m_get_mat_id;
        m_get_elem_id = m_acc.m_get_elem_id;
        m_get_vol_frac = m_acc.m_get_vol_frac;
        m_get_mset_val = m_acc.m_get_mset_val;
        m_get_nmats_for_zone = m_acc.m_get_nmats_for_zone;
        m_get_nzones_for_mat = m_acc.m_get_nzones_for_mat;
        m_multi_vol_fracs = m_acc.m_multi_vol_fracs;
        m_multi_mset_vals = m_acc.m_multi_mset_vals;
        m_multi_mat_idx_map = m_acc.m_multi_mat_idx_map;
        m_multi_mat_idx_map_acc = m_acc.m_multi_mat_idx_map_acc;
        m_sbm_elem_ids = m_acc.m_sbm_elem_ids;
        m_sbe_material_ids = m_acc.m_sbe_material_ids;
        m_sbe_vol_fracs = m_acc.m_sbe_vol_fracs;
        m_sbe_mset_vals = m_acc.m_sbe_mset_vals;
        m_sbe_o2m_idx = m_acc.m_sbe_o2m_idx;
    }
    return *this;
}

//-----------------------------------------------------------------------------
void
MatsetAccessor::init(const Node &matset,
                     const Node *field,
                     const Node *specset)
{
    // TODO
    (void) specset;

    const bool is_uni_buffer       = blueprint::mesh::matset::is_uni_buffer(matset);
    const bool is_element_dominant = blueprint::mesh::matset::is_element_dominant(matset);

    if (is_uni_buffer)
    {
        // uni-buffer by element (sparse by element)
        if (is_element_dominant)
        {
            m_sbe_material_ids = matset["material_ids"].value();
            m_sbe_vol_fracs = matset["volume_fractions"].value();
            m_sbe_o2m_idx = o2mrelation::O2MIndex(matset);
            if (nullptr != field)
            {
                m_sbe_mset_vals = (*field)["matset_values"].value();
            }

            m_get_mat_id   = &MatsetAccessor::get_sbe_mat_id;
            m_get_elem_id  = &MatsetAccessor::get_sbe_elem_id;
            m_get_vol_frac = &MatsetAccessor::get_sbe_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_sbe_mset_val;
            }
            m_get_nmats_for_zone = &MatsetAccessor::get_sbe_nmats_for_zone;
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
        const std::vector<std::string> &matnames = matset["volume_fractions"].child_names();
        
        Node material_map;
        create_or_reuse_material_map(matset, material_map);
        const index_t num_materials = static_cast<index_t>(matnames.size());

        // multi-buffer material index map
        // we save an indirection array from material order id (the order materials appear
        // in the matset) to actual material id. Not all material sets are numbered from
        // 0 to N-1, so we must support this case.
        m_multi_mat_idx_map.set(DataType::index_t(num_materials));
        m_multi_mat_idx_map_acc = m_multi_mat_idx_map.value();
        for (index_t mat_idx = 0; mat_idx < num_materials; mat_idx ++)
        {
            m_multi_mat_idx_map_acc.set(mat_idx, material_map.child(mat_idx).to_index_t());
        }

        if (nullptr != field)
        {
            for (const auto &matname : matnames)
            {
                m_multi_vol_fracs.push_back(matset["volume_fractions"][matname].value());
                m_multi_mset_vals.push_back((*field)["matset_values"][matname].value());
            }
        }
        else
        {
            for (const auto &matname : matnames)
            {
                m_multi_vol_fracs.push_back(matset["volume_fractions"][matname].value());
            }
        }

        // multi-buffer by element (full)
        if (is_element_dominant)
        {
            m_get_mat_id   = &MatsetAccessor::get_full_mat_id;
            m_get_elem_id  = &MatsetAccessor::get_full_elem_id;
            m_get_vol_frac = &MatsetAccessor::get_full_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_full_mset_val;
            }
        }
        // multi-buffer by material (sparse by material)
        else
        {
            for (const auto &matname : matnames)
            {
                m_sbm_elem_ids.push_back(matset["element_ids"][matname].value());
            }

            m_get_mat_id   = &MatsetAccessor::get_sbm_mat_id;
            m_get_elem_id  = &MatsetAccessor::get_sbm_elem_id;
            m_get_vol_frac = &MatsetAccessor::get_sbm_vol_frac;
            if (nullptr != field)
            {
                m_get_mset_val = &MatsetAccessor::get_sbm_mset_val;
            }
            m_get_nzones_for_mat = &MatsetAccessor::get_sbm_nzones_for_mat;
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
MatsetAccessor::get_full_mat_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    return m_multi_mat_idx_map_acc[mat_idx];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_full_elem_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) mat_idx;
    return zone_idx;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_full_vol_frac(const index_t zone_idx, const index_t mat_idx) const
{
    return m_multi_vol_fracs[mat_idx][zone_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_full_mset_val(const index_t zone_idx, const index_t mat_idx) const
{
    return m_multi_mset_vals[mat_idx][zone_idx];
}

//-----------------------------------------------------------------------------
//
// -- getters for multi-buffer by material (sparse by material) matsets --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbm_mat_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    return m_multi_mat_idx_map_acc[mat_idx];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbm_elem_id(const index_t zone_idx, const index_t mat_idx) const
{
    return m_sbm_elem_ids[mat_idx][zone_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbm_vol_frac(const index_t zone_idx, const index_t mat_idx) const
{
    return m_multi_vol_fracs[mat_idx][zone_idx];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbm_mset_val(const index_t zone_idx, const index_t mat_idx) const
{
    return m_multi_mset_vals[mat_idx][zone_idx];
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbm_nzones_for_mat(const index_t mat_idx) const
{
    return m_sbm_elem_ids[mat_idx].number_of_elements();
}

//-----------------------------------------------------------------------------
//
// -- getters for uni-buffer by element (sparse by element) matsets --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbe_mat_id(const index_t zone_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(zone_idx, mat_idx);
    return m_sbe_material_ids[data_index];
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_sbe_elem_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) mat_idx;
    return zone_idx;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbe_vol_frac(const index_t zone_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(zone_idx, mat_idx);
    return m_sbe_vol_fracs[data_index];
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_sbe_mset_val(const index_t zone_idx, const index_t mat_idx) const
{
    const index_t data_index = m_sbe_o2m_idx.index(zone_idx, mat_idx);
    return m_sbe_mset_vals[data_index];
}

//-----------------------------------------------------------------------------
index_t
MatsetAccessor::get_sbe_nmats_for_zone(const index_t zone_idx) const
{
    return m_sbe_o2m_idx.size(zone_idx);
}

//-----------------------------------------------------------------------------
//
// -- getters for the error case --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_mat_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch mat_id from material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_elem_id(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch elem_id from material set.");
    return 0;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_error_vol_frac(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch vol_frac from material set.");
    return 0.0;
}

//-----------------------------------------------------------------------------
float64 
MatsetAccessor::get_error_mset_val(const index_t zone_idx, const index_t mat_idx) const
{
    (void) zone_idx;
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch mset_val from field.");
    return 0.0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_nmats_for_zone(const index_t zone_idx) const
{
    (void) zone_idx;
    CONDUIT_ERROR("Impossible to fetch number of materials for zone from "
                  "non-sparse by element material set.");
    return 0;
}

//-----------------------------------------------------------------------------
index_t 
MatsetAccessor::get_error_nzones_for_mat(const index_t mat_idx) const
{
    (void) mat_idx;
    CONDUIT_ERROR("Impossible to fetch number of zones for a material from "
                  "non-sparse by material material set.");
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


