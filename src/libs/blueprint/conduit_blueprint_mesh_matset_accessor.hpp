// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_matset_accessor.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_MATSET_ACCESSOR_HPP
#define CONDUIT_BLUEPRINT_MESH_MATSET_ACCESSOR_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint_exports.h"
#include "conduit_blueprint_o2mrelation_index.hpp"

using namespace conduit;
// access one-to-many index types
namespace o2mrelation = conduit::blueprint::o2mrelation;


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
// -- begin conduit::blueprint::mesh::matset::MatsetAccessor --
//-----------------------------------------------------------------------------
///
/// class: conduit::blueprint::mesh::MatsetAccessor
///
/// description:
///  Generic accessor for material sets, material fields, and species sets
///
//-----------------------------------------------------------------------------
class CONDUIT_BLUEPRINT_API MatsetAccessor
{
public:
//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::mesh::matset::MatsetAccessor public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// MatsetAccessor Function pointer types
//-----------------------------------------------------------------------------
using GetMatIdPtr        = index_t (MatsetAccessor::*)(index_t, index_t) const;
using GetElemIdPtr       = index_t (MatsetAccessor::*)(index_t, index_t) const;
using GetVolFracPtr      = double  (MatsetAccessor::*)(index_t, index_t) const;
using GetMsetValPtr      = double  (MatsetAccessor::*)(index_t, index_t) const;
using GetMassFracPtr     = double  (MatsetAccessor::*)(index_t, index_t, index_t) const;
using GetNMatsForZonePtr = index_t (MatsetAccessor::*)(index_t) const;
using GetNZonesForMatPtr = index_t (MatsetAccessor::*)(index_t) const;
using GetNMatSpecPtr     = index_t (MatsetAccessor::*)(index_t, index_t) const;

//-----------------------------------------------------------------------------
/// MatsetAccessor Construction and Destruction
//-----------------------------------------------------------------------------
    /// Default constructor.
    MatsetAccessor();

    /// Copy constructor.
    MatsetAccessor(const MatsetAccessor &m_acc);

    /// Construct with just a matset
    MatsetAccessor(const Node &matset);

    /// Construct with a matset and either a specset or a field
    MatsetAccessor(const Node &matset, const Node &specset_or_field);

    /// Construct with a matset, specset, and field
    MatsetAccessor(const Node &matset, const Node &field, const Node &specset);

    /// Destructor
    ~MatsetAccessor() { };

    /// Assignment operator.
    MatsetAccessor &operator=(const MatsetAccessor &m_acc);

//-----------------------------------------------------------------------------
/// Get information about the sizes
//-----------------------------------------------------------------------------
    // use this for elem-dom sparse matsets
    inline
    index_t     num_mats_for_zone(const index_t zone_idx) const
    {
        return (this->*m_get_nmats_for_zone)(zone_idx);
    }

    // use this for mat-dom sparse matsets
    inline
    index_t     num_zones_for_mat(const index_t mat_idx) const
    {
        return (this->*m_get_nzones_for_mat)(mat_idx);
    }

    inline
    index_t     num_spec_for_mat(const index_t zone_idx,
                                 const index_t mat_idx) const
    {
        return (this->*m_get_nspec_for_mat)(zone_idx, mat_idx);
    }

//-----------------------------------------------------------------------------
/// Retrieve data
//-----------------------------------------------------------------------------
    inline
    index_t     get_mat_id(const index_t zone_idx,
                           const index_t mat_idx) const
    {
        return (this->*m_get_mat_id)(zone_idx, mat_idx);
    }

    inline
    index_t     get_elem_id(const index_t zone_idx,
                            const index_t mat_idx) const
    {
        return (this->*m_get_elem_id)(zone_idx, mat_idx);
    }

    inline
    float64     get_vol_frac(const index_t zone_idx,
                             const index_t mat_idx) const
    {
        return (this->*m_get_vol_frac)(zone_idx, mat_idx);
    }

    inline
    float64     get_mset_val(const index_t zone_idx,
                             const index_t mat_idx) const
    {
        return (this->*m_get_mset_val)(zone_idx, mat_idx);
    }

    inline
    float64     get_mass_frac(const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t spec_idx) const
    {
        return (this->*m_get_mass_frac)(zone_idx, mat_idx, spec_idx);
    }

private:

//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::mesh::matset::MatsetAccessor private members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- private methods that help with init, and handle different matset flavors --
//
//-----------------------------------------------------------------------------

    void init(const Node &matset,
              const Node *field,
              const Node *specset);

    //
    // Per layout implementations, only declarations here
    //

    // multi-buffer by element (full)
    // 0 <= zone_idx < num zones
    // 0 <= mat_idx < num mats
    // 0 <= spec_idx < num species for material mat_idx
    index_t get_full_mat_id(const index_t zone_idx, const index_t mat_idx) const;
    index_t get_full_elem_id(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_full_vol_frac(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_full_mset_val(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_full_mass_frac(const index_t zone_idx,
                               const index_t mat_idx,
                               const index_t spec_idx) const;
    // omitted because this method is used for knowing how many
    // materials to iterate over in a sparse representation
    // index_t get_full_nmats_for_zone(const index_t zone_idx) const;
    // omitted because this method is used for knowing how many
    // zones to iterate over in a sparse representation
    // index_t get_full_nzones_for_mat(const index_t mat_idx) const;
    index_t get_full_nspec_for_mat(const index_t zone_idx, const index_t mat_idx) const;

    // multi-buffer by material (sparse by material)
    // 0 <= zone_idx < num zones for material mat_idx
    // 0 <= mat_idx < num mats
    // 0 <= spec_idx < num species for material mat_idx
    index_t get_sbm_mat_id(const index_t zone_idx, const index_t mat_idx) const;
    index_t get_sbm_elem_id(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbm_vol_frac(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbm_mset_val(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbm_mass_frac(const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t spec_idx) const;
    index_t get_sbm_nzones_for_mat(const index_t mat_idx) const;
    // omitted because this method is used for knowing how many
    // materials to iterate over in a sparse representation
    // index_t get_sbm_nmats_for_zone(const index_t zone_idx) const;
    index_t get_sbm_nspec_for_mat(const index_t zone_idx, const index_t mat_idx) const;

    // uni-buffer by element (sparse by element)
    // 0 <= zone_idx < num zones
    // 0 <= mat_idx < num mats for zone zone_idx
    // 0 <= spec_idx < num species for material mat_idx in zone zone_idx
    index_t get_sbe_mat_id(const index_t zone_idx, const index_t mat_idx) const;
    index_t get_sbe_elem_id(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbe_vol_frac(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbe_mset_val(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_sbe_mass_frac(const index_t zone_idx,
                              const index_t mat_idx,
                              const index_t spec_idx) const;
    // omitted because this method is used for knowing how many
    // zones to iterate over in a sparse representation
    // index_t get_sbe_nzones_for_mat(const index_t mat_idx) const;
    index_t get_sbe_nmats_for_zone(const index_t zone_idx) const;
    index_t get_sbe_nspec_for_mat(const index_t zone_idx, const index_t mat_idx) const;

    // uni-buffer by material
    // not implemented; error in constructor

    // The following methods are guard rails; they just throw errors. If the 
    // accessor is used improperly users will get a helpful error instead
    // of just a segfault. The field and specset access methods will only be
    // turned on if a field or a specset is provided.
    index_t get_error_mat_id(const index_t zone_idx, const index_t mat_idx) const;
    index_t get_error_elem_id(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_error_vol_frac(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_error_mset_val(const index_t zone_idx, const index_t mat_idx) const;
    float64 get_error_mass_frac(const index_t zone_idx, 
                                const index_t mat_idx,
                                const index_t spec_idx) const;
    index_t get_error_nmats_for_zone(const index_t zone_idx) const;
    index_t get_error_nzones_for_mat(const index_t mat_idx) const;
    index_t get_error_nspec_for_mat(const index_t zone_idx, const index_t mat_idx) const;

//-----------------------------------------------------------------------------
//
// -- private data members --
//
//-----------------------------------------------------------------------------

    // function pointer members
    // these take us to implementations for each layout type
    GetMatIdPtr        m_get_mat_id;
    GetElemIdPtr       m_get_elem_id;
    GetVolFracPtr      m_get_vol_frac;
    GetMsetValPtr      m_get_mset_val;
    GetMassFracPtr     m_get_mass_frac;
    GetNMatsForZonePtr m_get_nmats_for_zone;
    GetNZonesForMatPtr m_get_nzones_for_mat;
    GetNMatSpecPtr     m_get_nspec_for_mat;

    // information members
    bool m_is_uni_buffer;
    bool m_is_element_dominant;

    // universal members
    // these are members that are useful for all layout types
    Node m_nmatspec;
    index_t_accessor m_nmatspec_acc;

    // multi-buffer (full AND sparse by material) members
    std::vector<float64_accessor> m_multi_vol_fracs;
    std::vector<float64_accessor> m_multi_mset_vals;
    Node m_multi_mat_idx_map; // multi-buffer material index map
    index_t_accessor m_multi_mat_idx_map_acc;
    std::vector<float64_accessor> m_multi_mass_fracs;
    index_t_accessor m_multi_nmatspec_offsets_acc;
    
    // multi-buffer material dominant (sparse by material) members
    std::vector<index_t_accessor> m_sbm_elem_ids;

    // uni-buffer element-dominant (sparse by element) members
    index_t_accessor m_sbe_material_ids;
    float64_accessor m_sbe_vol_fracs;
    float64_accessor m_sbe_mset_vals;
    o2mrelation::O2MIndex m_sbe_o2m_idx;
    float64_accessor m_sbe_mass_fracs;
    o2mrelation::O2MIndex m_sbe_specset_o2m_idx;

    // uni-buffer material-dominant (???) members
    // not implemented; error case in constructor
    
};
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset::MatsetAccessor --
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


#endif
