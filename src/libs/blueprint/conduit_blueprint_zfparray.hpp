// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_blueprint_zfparray.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BLUEPRINT_ZFPARRAY_HPP
#define CONDUIT_BLUEPRINT_ZFPARRAY_HPP

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
// -- begin conduit::blueprint::zfparray --
//-----------------------------------------------------------------------------
namespace zfparray
{

static const std::string ZFP_HEADER_FIELD = "zfp_header";
static const std::string ZFP_HEADER_DIM_FIELD = "zfp_header_dim";
static const std::string ZFP_HEADER_SCALAR_TYPE_FIELD = "zfp_header_scalar_type";
static const std::string ZFP_COMPRESSED_DATA_FIELD = "zfp_compressed_data";

//-----------------------------------------------------------------------------
// blueprint protocol interface
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool CONDUIT_BLUEPRINT_API verify(const conduit::Node &n,
                                  conduit::Node &info);

//-----------------------------------------------------------------------------
bool CONDUIT_BLUEPRINT_API verify(const std::string &protocol,
                                  const conduit::Node &n,
                                  conduit::Node &info);


//-----------------------------------------------------------------------------
}
//-----------------------------------------------------------------------------
// -- end conduit::blueprint::zfparray --
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
