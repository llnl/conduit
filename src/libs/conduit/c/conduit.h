// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit.h
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_H
#define CONDUIT_H

//-----------------------------------------------------------------------------
// -- includes for the public conduit c interface -- 
//-----------------------------------------------------------------------------

#include "conduit_node.h"
#include "conduit_datatype.h"
#include "conduit_schema.h"
#include "conduit_utils.h"

//-----------------------------------------------------------------------------
// -- begin extern C
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// -- general conduit c interface methods --
//-----------------------------------------------------------------------------

CONDUIT_API void conduit_about(conduit_node *cnode);

#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------
// -- end extern C
//-----------------------------------------------------------------------------


#endif
