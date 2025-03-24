// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_relay_zfp.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_RELAY_ZFP_HPP
#define CONDUIT_RELAY_ZFP_HPP

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include "zfp/array1.hpp"
#include "zfp/array2.hpp"
#include "zfp/array3.hpp"

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_relay_io.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay --
//-----------------------------------------------------------------------------
namespace relay
{

//-----------------------------------------------------------------------------
// -- begin conduit::relay::io --
//-----------------------------------------------------------------------------
namespace io
{

static const std::string ZFP_HEADER_FIELD = "zfp_header";
static const std::string ZFP_HEADER_DIM_FIELD = "zfp_header_dim";
static const std::string ZFP_HEADER_SCALAR_TYPE_FIELD = "zfp_header_scalar_type";
static const std::string ZFP_COMPRESSED_DATA_FIELD = "zfp_compressed_data";


CONDUIT_RELAY_API zfp::array* unwrap_zfparray(const Node &node);

void CONDUIT_RELAY_API        wrap_zfparray(const zfp::array &arr,
                                            Node &node);

void CONDUIT_RELAY_API        wrap_zfparray(const zfp::array *arr,
                                            Node &node);

}
//-----------------------------------------------------------------------------
// -- end conduit::relay::io --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit::relay --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
