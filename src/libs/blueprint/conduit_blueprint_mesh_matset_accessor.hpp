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
// -- begin conduit::blueprint::mesh::matset::matset_accessor --
//-----------------------------------------------------------------------------
///
/// class: conduit::blueprint::mesh::matset_accessor
///
/// description:
///  Generic accessor for material sets, material fields, and species sets
///
//-----------------------------------------------------------------------------
class CONDUIT_BLUEPRINT_API matset_accessor
{
public:
//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::mesh::matset::matset_accessor public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// matset_accessor Construction and Destruction
//-----------------------------------------------------------------------------
    /// Default constructor.
    matset_accessor();

    /// Copy constructor.
    matset_accessor(const matset_accessor &idx);

    /// Construct with just a matset
    matset_accessor(const Node &matset);

    /// Construct with a matset and either a specset or a field
    matset_accessor(const Node &matset, const Node &either_specset_or_field);

    /// Construct with a matset, specset, and field
    matset_accessor(const Node &matset, const Node &field, const Node &specset);

    /// Destructor
    ~matset_accessor() { };

    /// Assignment operator.
    matset_accessor &operator=(const matset_accessor &itr);

//-----------------------------------------------------------------------------
/// Retrieve a flat-index.
//-----------------------------------------------------------------------------
    // use this for elem-dom matsets
    index_t     num_mats_for_zone(const index_t zone_idx) const;

    // use this for mat-dom matsets
    index_t     num_zones_for_mat(const index_t mat_idx) const;

//-----------------------------------------------------------------------------
/// Retrieve a flat-index.
//-----------------------------------------------------------------------------
    index_t     get_material_id(const index_t zone_idx,
                                const index_t mat_idx) const;

    float64     get_vol_frac(const index_t zone_idx,
                             const index_t mat_idx) const;

    float64     get_mset_val(const index_t zone_idx,
                             const index_t mat_idx) const;

private:

//-----------------------------------------------------------------------------
//
// -- conduit::blueprint::mesh::matset::matset_accessor private members --
//
//-----------------------------------------------------------------------------
    bool is_uni_buffer;
    bool is_element_dominant;

    // multi-buffer element dominant (full) members
    std::vector<float64_accessor> full_vol_fracs;
    std::vector<float64_accessor> full_mset_vals;
    
    // multi-buffer material dominant (sparse by material) members
    std::vector<float64_accessor> sbm_vol_fracs;
    std::vector<float64_accessor> sbm_mset_vals;
    std::vector<index_t_accessor> sbm_elem_ids;

    // uni-buffer element-dominant (sparse by element) members
    index_t_accessor sbe_material_ids;
    float64_accessor sbe_vol_fracs;
    float64_accessor sbe_mset_vals;
    o2mrelation::O2MIndex sbe_o2m_idx;

    // uni-buffer material-dominant (???) members
    // not implemented; error case in constructor
    
};
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::matset::matset_accessor --
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
