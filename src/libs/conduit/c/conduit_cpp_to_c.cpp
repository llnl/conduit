// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_cpp_to_c.cpp
///
//-----------------------------------------------------------------------------
#include "conduit.h"
#include "conduit.hpp"

#include "conduit_cpp_to_c.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

struct conduit_node_impl {};

//---------------------------------------------------------------------------//
Node *
cpp_node(conduit_node *cnode)
{
    return reinterpret_cast<Node*>(cnode);
}

//---------------------------------------------------------------------------//
conduit_node *
c_node(Node *node)
{
    return reinterpret_cast<conduit_node*>(node);
}


//---------------------------------------------------------------------------//
const Node *
cpp_node(const conduit_node *cnode)
{
    return reinterpret_cast<const Node*>(cnode);
}

//---------------------------------------------------------------------------//
const conduit_node *
c_node(const Node *node)
{
    return reinterpret_cast<const conduit_node*>(node);
}

//---------------------------------------------------------------------------//
Node &
cpp_node_ref(conduit_node *cnode)
{
    return *reinterpret_cast<Node*>(cnode);
}

//---------------------------------------------------------------------------//
const Node &
cpp_node_ref(const conduit_node *cnode)
{
    return *reinterpret_cast<const Node*>(cnode);
}

struct conduit_datatype_impl {};

//---------------------------------------------------------------------------//
DataType *
cpp_datatype(conduit_datatype *cdatatype)
{
    return reinterpret_cast<DataType*>(cdatatype);
}

//---------------------------------------------------------------------------//
conduit_datatype *
c_datatype(DataType *datatype)
{
    return reinterpret_cast<conduit_datatype*>(datatype);
}


//---------------------------------------------------------------------------//
const DataType *
cpp_datatype(const conduit_datatype *cdatatype)
{
    return reinterpret_cast<const DataType*>(cdatatype);
}

//---------------------------------------------------------------------------//
const conduit_datatype *
c_datatype(const DataType *datatype)
{
    return reinterpret_cast<const conduit_datatype*>(datatype);
}

//---------------------------------------------------------------------------//
DataType &
cpp_datatype_ref(conduit_datatype *cdatatype)
{
    return *reinterpret_cast<DataType*>(cdatatype);
}

//---------------------------------------------------------------------------//
const DataType &
cpp_datatype_ref(const conduit_datatype *cdatatype)
{
    return *reinterpret_cast<const DataType*>(cdatatype);
}

struct conduit_schema_impl {};

//---------------------------------------------------------------------------//
Schema *
cpp_schema(conduit_schema *cschema)
{
    return reinterpret_cast<Schema*>(cschema);
}

//---------------------------------------------------------------------------//
conduit_schema *
c_schema(Schema *schema)
{
    return reinterpret_cast<conduit_schema*>(schema);
}

//---------------------------------------------------------------------------//
const Schema *
cpp_schema(const conduit_schema *cschema)
{
    return reinterpret_cast<const Schema*>(cschema);
}

//---------------------------------------------------------------------------//
const conduit_schema *
c_schema(const Schema *schema)
{
    return reinterpret_cast<const conduit_schema*>(schema);
}

//---------------------------------------------------------------------------//
Schema &
cpp_schema_ref(conduit_schema *cschema)
{
    return *reinterpret_cast<Schema*>(cschema);
}

//---------------------------------------------------------------------------//
const Schema &
cpp_schema_ref(const conduit_schema *cschema)
{
    return *reinterpret_cast<const Schema*>(cschema);
}

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

