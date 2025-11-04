// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_venn.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_EXAMPLES_VENN_HPP
#define CONDUIT_BLUEPRINT_MESH_EXAMPLES_VENN_HPP

//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit.hpp"
#include "conduit_blueprint.hpp"
#include "conduit_blueprint_exports.h"

//-----------------------------------------------------------------------------
// -- begin conduit::--
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
/// Methods that generate example meshes.
//-----------------------------------------------------------------------------
namespace examples
{
    /// Generates a rectilinear grid with fields that
    /// are computed from 3 overlapping circles.
    ///
    /// matset_type options:
    ///   full -> non sparse volume fractions and matset values
    ///   sparse_by_material ->  sparse (material dominant) volume fractions
    ///                          and matset values
    ///   sparse_by_element  ->  sparse (element dominant)
    ///                          volume fractions and matset values
    void CONDUIT_BLUEPRINT_API venn(const std::string &matset_type,
                                    index_t nx,
                                    index_t ny,
                                    float64 radius,
                                    Node &res);

    /// Generates a rectilinear grid with fields that
    /// are computed from 3 overlapping circles.
    ///
    /// matset_type options:
    ///   full -> non sparse volume fractions and matset values
    ///   sparse_by_material ->  sparse (material dominant) volume fractions
    ///                          and matset values
    ///   sparse_by_element  ->  sparse (element dominant)
    ///                          volume fractions and matset values
    void CONDUIT_BLUEPRINT_API venn_specsets(const std::string &matset_type,
                                             index_t nx,
                                             index_t ny,
                                             float64 radius,
                                             Node &res);

}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::mesh::examples --
//-----------------------------------------------------------------------------


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
// -- end conduit --
//-----------------------------------------------------------------------------


#endif



