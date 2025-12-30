// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_generator.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_GENERATOR_HPP
#define CONDUIT_GENERATOR_HPP

//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"
#include "conduit_schema.hpp"
#include "conduit_node.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{
//-----------------------------------------------------------------------------
// -- begin conduit::Generator --
//-----------------------------------------------------------------------------
///
/// class: conduit::Generator
///
/// description:
///  The Generator class implements parsing logic for yaml and json schemas.
///
//-----------------------------------------------------------------------------
class CONDUIT_API Generator
{
public:
    
//-----------------------------------------------------------------------------
// -- friends of Generator --
//-----------------------------------------------------------------------------
    friend class Node;
    friend class Schema;
    
//-----------------------------------------------------------------------------
//
// -- conduit::Generator public members --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Generator Construction and Destruction
//-----------------------------------------------------------------------------
    Generator();

    /// create a generator from json or yaml using a given protocol name, 
    //  which can optionally be applied to a data pointer
    ///
    /// protocols:
    ///   "json"
    ///   "conduit_json"
    ///   "conduit_json_external"
    ///   "conduit_base64_json"
    ///   "yaml"
    ///   "conduit_yaml"
    ///   "conduit_yaml_external"
    ///   "conduit_base64_yaml"
    ///
    Generator(const std::string &schema,
              const std::string &protocol = std::string("conduit_json"),
              void *data = NULL);


    void set_schema(const std::string &schema);
    void set_protocol(const std::string &protocol);
    void set_data_ptr(void *);

    const std::string &schema() const;
    const std::string &protocol()   const;
    void *data_ptr() const;


//-----------------------------------------------------------------------------
// JSON Parsing interface
//-----------------------------------------------------------------------------
    /// parse a json schema to a Schema object.
    void walk(Schema &sdest) const;

    /// parse a json schema to a Node object.
    void walk(Node &ndest) const;
    void walk_external(Node &ndest) const;

    // private class used to encapsulate RapidJSON logic. 
    class Parser;


private:
//-----------------------------------------------------------------------------
//
// -- conduit::Generator private data members --
//
//-----------------------------------------------------------------------------
    
    
    /// holds the schema text
    std::string  m_schema;
    /// holds the parsing protocol
    std::string  m_protocol;
    /// optional external data pointer
    void        *m_data;

};
//-----------------------------------------------------------------------------
// -- end conduit::Generator --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------


#endif
