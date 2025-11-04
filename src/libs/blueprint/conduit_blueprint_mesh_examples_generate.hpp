// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_mesh_examples_generate.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_MESH_EXAMPLES_GENERATE_HPP
#define CONDUIT_BLUEPRINT_MESH_EXAMPLES_GENERATE_HPP

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
    // driver functions that allows you to generate any of conduit's
    // mesh examples

    /// generates a named example mesh w/o options (defaults options implied)
    void CONDUIT_BLUEPRINT_API generate(const std::string &example_name,
                                        conduit::Node &res);

    /// generates a named example mesh using options
    /// expects opts node to contains relevant args for each example
    void CONDUIT_BLUEPRINT_API generate(const std::string &example_name,
                                        const conduit::Node &opts,
                                        conduit::Node &res);

    /// creates the default options for a all examples
    void CONDUIT_BLUEPRINT_API generate_default_options(conduit::Node &opts);

    /// creates the default options for a given example
    void CONDUIT_BLUEPRINT_API generate_default_options(const std::string &example_name,
                                                        conduit::Node &opts);

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



