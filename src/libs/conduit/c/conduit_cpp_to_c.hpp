// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_cpp_to_c.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_CPP_TO_C_HPP
#define CONDUIT_CPP_TO_C_HPP

#include "conduit.hpp"

#include "conduit.h"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//---------------------------------------------------------------------------//
CONDUIT_API conduit::Node *cpp_node(conduit_node *cnode);
//---------------------------------------------------------------------------//
CONDUIT_API conduit_node *c_node(conduit::Node *node);

//---------------------------------------------------------------------------//
CONDUIT_API const conduit::Node *cpp_node(const conduit_node *cnode);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit_node *c_node(const conduit::Node *node);

//---------------------------------------------------------------------------//
CONDUIT_API conduit::Node &cpp_node_ref(conduit_node *cnode);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit::Node &cpp_node_ref(const conduit_node *cnode);


//---------------------------------------------------------------------------//
CONDUIT_API conduit::DataType *cpp_datatype(conduit_datatype *cdatatype);
//---------------------------------------------------------------------------//
CONDUIT_API conduit_datatype  *c_datatype(conduit::DataType  *datatype);

//---------------------------------------------------------------------------//
CONDUIT_API const conduit::DataType *cpp_datatype(const conduit_datatype *cdatatype);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit_datatype  *c_datatype(const conduit::DataType *datatype);

//---------------------------------------------------------------------------//
CONDUIT_API conduit::DataType &cpp_datatype_ref(conduit_datatype *cdatatype);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit::DataType &cpp_datatype_ref(const conduit_datatype *datatype);

//---------------------------------------------------------------------------//
CONDUIT_API conduit::Schema *cpp_schema(conduit_schema *cschema);
//---------------------------------------------------------------------------//
CONDUIT_API conduit_schema  *c_schema(conduit::Schema *schema, bool owns);

//---------------------------------------------------------------------------//
CONDUIT_API const conduit::Schema *cpp_schema(const conduit_schema *cschema);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit_schema  *c_schema(const conduit::Schema *schema, bool owns);

//---------------------------------------------------------------------------//
CONDUIT_API conduit::Schema &cpp_schema_ref(conduit_schema *cschema);
//---------------------------------------------------------------------------//
CONDUIT_API const conduit::Schema &cpp_schema_ref(const conduit_schema *cschema);

//---------------------------------------------------------------------------//
/// Destroys a schema handle created by c_schema(). If the handle owns the schema,
/// also deletes the underlying schema object.
CONDUIT_API void destroy_cschema(conduit_schema *cschema);

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
