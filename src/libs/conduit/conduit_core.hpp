// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_core.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_CORE_HPP
#define CONDUIT_CORE_HPP

//-----------------------------------------------------------------------------
// -- standard cpp lib includes -- 
//-----------------------------------------------------------------------------
#include <string>
#include <iostream>

//-----------------------------------------------------------------------------
// -- configure time defines -- 
//-----------------------------------------------------------------------------
#include "conduit_config.hpp"

//-----------------------------------------------------------------------------
// -- define proper lib exports for various platforms -- 
//-----------------------------------------------------------------------------
#include "conduit_exports.h"

//-----------------------------------------------------------------------------
// -- include bit width style types mapping header  -- 
//-----------------------------------------------------------------------------
#include "conduit_bitwidth_style_types.h"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

class Node;

//-----------------------------------------------------------------------------
/// typedefs that map bit width style types into conduit::
//-----------------------------------------------------------------------------

/// unsigned integer typedefs
typedef conduit_uint8   uint8;
typedef conduit_uint16  uint16;
typedef conduit_uint32  uint32;
typedef conduit_uint64  uint64;

/// signed integer typedefs
typedef conduit_int8    int8;
typedef conduit_int16   int16;
typedef conduit_int32   int32;
typedef conduit_int64   int64;

/// floating point typedefs
typedef conduit_float32 float32;
typedef conduit_float64 float64;

/// index typedefs
typedef conduit_index32_t index32_t;
typedef conduit_index64_t index64_t;
// conduit_index_t is defined in Bitwidth_Style_Types.h
// it will be index64_t, unless CONDUIT_INDEX_32 is defined
typedef conduit_index_t   index_t;

//-----------------------------------------------------------------------------
/// The about methods construct human readable info about how conduit was
/// configured.
//-----------------------------------------------------------------------------
std::string CONDUIT_API about();
void        CONDUIT_API about(Node &);

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif

