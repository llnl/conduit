// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_schema.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_SCHEMA_HPP
#define CONDUIT_SCHEMA_HPP

//-----------------------------------------------------------------------------
// -- standard lib includes -- 
//-----------------------------------------------------------------------------
#include <map>
#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------
// -- conduit includes -- 
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"
#include "conduit_endianness.hpp"
#include "conduit_data_type.hpp"


//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::Schema --
//-----------------------------------------------------------------------------
///
/// class: conduit::Schema
///
/// description:
///  TODO
///
//-----------------------------------------------------------------------------
class CONDUIT_API Schema
{
public:
//-----------------------------------------------------------------------------
// -- friends of Schema --
//-----------------------------------------------------------------------------
    friend class Node;
    friend class NodeIterator;
    friend class NodeConstIterator;

//----------------------------------------------------------------------------
//
/// Schema construction and destruction.
//
//----------------------------------------------------------------------------
/// description:
///  Standard construction and destruction methods.
///
/// notes:
///  TODO:
///
//-----------------------------------------------------------------------------
    /// create an empty schema
    Schema(); 
    /// schema copy constructor
    explicit Schema(const Schema &schema);
    /// create a schema for a leaf type given a data type id
    explicit Schema(index_t dtype_id);
    /// create a schema from a DataType
    explicit Schema(const DataType &dtype);
    /// create a schema from a json description (std::string case)
    explicit Schema(const std::string &json_schema);
    /// create a schema from a json description (c string case)
    explicit Schema(const char *json_schema);

    /// Schema Destructor
    ~Schema();
    /// return a schema to the default (empty) state
    void  reset();

//-----------------------------------------------------------------------------
//
// Schema set methods
//
//-----------------------------------------------------------------------------
    void set(const Schema &schema); 

    void set(index_t dtype_id);
    void set(const DataType &dtype);
    void set(const std::string &json_schema);


//-----------------------------------------------------------------------------
//
/// Schema assignment operators
//
//-----------------------------------------------------------------------------
    Schema &operator=(const Schema &schema);
    Schema &operator=(index_t dtype_id);
    Schema &operator=(const DataType &dtype);
    Schema &operator=(const std::string &json_schema);


//-----------------------------------------------------------------------------
//
/// Information Methods
//
//-----------------------------------------------------------------------------
    const DataType &dtype() const 
                        {return m_dtype;}

    DataType       &dtype() 
                        {return m_dtype;}

    index_t         element_index(index_t idx) const 
                        {return m_dtype.element_index(idx);}

    bool            is_root() const
                        { return m_parent == NULL;}

    /// returns if this schema represents are compact layout
    bool            is_compact() const;

    /// is this schema compatible with given schema
    bool            compatible(const Schema &s) const;

    /// is this schema equal to given schema
    bool            equals(const Schema &s) const;

    /// sum of the strided bytes of all leaves
    index_t         total_strided_bytes() const;
    /// sum of the bytes of the compact form of all leaves
    index_t         total_bytes_compact() const;


    /// parent access
    const Schema   *parent() const
                       { return m_parent;}

    Schema         *parent()
                        { return m_parent;}

    void            print() const
                        {std::cout << to_json(false,2) << std::endl;}

//-----------------------------------------------------------------------------
//
/// Transformation Methods
//
//-----------------------------------------------------------------------------
    void            compact_to(Schema &s_dest) const;

    //-----------------------------------------------------------------------------
    // -- String construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a string representation of a schema.
    /// accepted protocols:
    ///   "json"
    ///   "yaml"
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string     to_string(const std::string &protocol="json",
                              index_t indent=2, 
                              index_t depth=0,
                              const std::string &pad=" ",
                              const std::string &eoe="\n") const;

    void            to_string_stream(std::ostream &os,
                                     const std::string &protocol="json", 
                                     index_t indent=2, 
                                     index_t depth=0,
                                     const std::string &pad=" ",
                                     const std::string &eoe="\n") const;

    void            to_string_stream(const std::string &stream_path,
                                     const std::string &protocol="json",
                                     index_t indent=2, 
                                     index_t depth=0,
                                     const std::string &pad=" ",
                                     const std::string &eoe="\n") const;

    // NOTE(cyrush): The primary reason this function exists is to enable easier
    // compatibility with debugging tools (e.g. totalview, gdb) that have
    // difficulty allocating default string parameters.
    std::string     to_string_default() const;

    //-----------------------------------------------------------------------------
    // -- JSON construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a JSON string representation of a schema.
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string     to_json(index_t indent=2,
                            index_t depth=0,
                            const std::string &pad=" ",
                            const std::string &eoe="\n") const;

    void            to_json_stream(std::ostream &os,
                                   index_t indent=2,
                                   index_t depth=0,
                                   const std::string &pad=" ",
                                   const std::string &eoe="\n") const;

    void            to_json_stream(const std::string &stream_path, 
                                   index_t indent=2,
                                   index_t depth=0,
                                   const std::string &pad=" ",
                                   const std::string &eoe="\n") const;

    // NOTE(JRC): The primary reason this function exists is to enable easier
    // compatibility with debugging tools (e.g. totalview, gdb) that have
    // difficulty allocating default string parameters.
    std::string         to_json_default() const;

    //-----------------------------------------------------------------------------
    // -- YAML construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a YAML string representation of a schema.
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_yaml(index_t indent=2, 
                                index_t depth=0,
                                const std::string &pad=" ",
                                const std::string &eoe="\n") const;

    void                to_yaml_stream(std::ostream &os,
                                       index_t indent=2, 
                                       index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    void                to_yaml_stream(const std::string &stream_path,
                                       index_t indent=2, 
                                       index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    // NOTE(JRC): The primary reason this function exists is to enable easier
    // compatibility with debugging tools (e.g. totalview, gdb) that have
    // difficulty allocating default string parameters.
    std::string         to_yaml_default() const;

//-----------------------------------------------------------------------------
//
/// Basic I/O methods
//
//-----------------------------------------------------------------------------

    void            save(const std::string &ofname,
                         const std::string &protocol="json",
                         index_t indent=2,
                         index_t depth=0,
                         const std::string &pad=" ",
                         const std::string &eoe="\n") const;

    void            load(const std::string &stream_path);


//-----------------------------------------------------------------------------
//
/// Access to children (object and list interface)
//
//-----------------------------------------------------------------------------
    /// returns the number of children for a list of object type
    index_t number_of_children() const;

    // access a child schema by index
    Schema           &child(index_t idx);
    const Schema     &child(index_t idx) const;
    /// access to child schema pointer by index
    Schema           *child_ptr(index_t idx);
    const Schema     *child_ptr(index_t idx) const;

    /// remove a child by index
    void             remove(index_t idx);

    /// these use the "child" methods
    Schema           &operator[](const index_t idx);
    const Schema     &operator[](const index_t idx) const;

//-----------------------------------------------------------------------------
//
/// Object interface methods
//
//-----------------------------------------------------------------------------

    /// the `fetch_existing' methods don't modify map structure, if a path
    /// doesn't exist they will throw an exception
    Schema           &fetch_existing(const std::string &path);
    const Schema     &fetch_existing(const std::string &path) const;

    // the 'child' methods also don't modify map structure. Additionally,
    // they do not search parent/child schemas, and thus allow getting children
    // whose names contain slashes.
    Schema           &child(const std::string &name);
    const Schema     &child(const std::string &name) const;

    // the 'add_child' method will not parse the name arg as a path, allowing
    // for addition of literally-named children. Returns either the existing
    // Schema with the name or a new Schema
    Schema           &add_child(const std::string &name);

    /// non-const fetch with a path arg methods do modify map 
    // structure if a path doesn't exist
    Schema           &fetch(const std::string &path);
    const Schema     &fetch(const std::string &path) const;
    
    Schema           *fetch_ptr(const std::string &path);
    const Schema     *fetch_ptr(const std::string &path) const;

    /// child name to index map
    index_t          child_index(const std::string &name) const;

    /// index to name map
    /// returns an empty string when passed index is invalid, or 
    /// this schema does not describe an object.
    std::string      child_name(index_t idx) const;

    /// rename existing child
    ///
    /// throws an error if child does not exist, new name is invalid
    /// or if this schema does not describe an object.
    void             rename_child(const std::string &current_name,
                                  const std::string &new_name);

    /// this uses the fetch method
    Schema           &operator[](const std::string &path);
    /// the const variant uses the "fetch_child" method
    const Schema     &operator[](const std::string &path) const;
    
    std::string       name() const;
    std::string       path() const;
    
    bool              has_child(const std::string &name) const;
    bool              has_path(const std::string &path) const;
    const std::vector<std::string> &child_names() const;
    void              remove(const std::string &path);
    /// remove_child removes a direct child only (allows pathlike names)
    void              remove_child(const std::string &name); 
    
//-----------------------------------------------------------------------------
//
/// List Append Interface Methods
//
//-----------------------------------------------------------------------------
    Schema &append();

private:
//-----------------------------------------------------------------------------
//
// -- private methods that help with init, memory allocation, and cleanup --
//
//-----------------------------------------------------------------------------
    // set defaults (used by constructors)
    void        init_defaults();
    // setup schema to represent a list
    void        init_list();
    // setup schema to represent an object
    void        init_object();
    // cleanup any allocated memory.
    void        release();

    /// helps with proper alloc size for:
    /// Node::set_using_schema()and Node::set_data_using_schema
    ///
    /// this is private b/c, while it makes sense for a schema stand alone
    /// the values don't always make sense for a Node with several allocs
    ///
    /// We could try to make this clear to folks, but we think there will
    /// still be confusion, so we are just using it internally.
    index_t     spanned_bytes() const;

//-----------------------------------------------------------------------------
//
/// -- Private transform helpers -- 
//
//-----------------------------------------------------------------------------
    void        compact_to(Schema &s_dest, index_t curr_offset) const ;
    void        walk_schema(const std::string &json_schema);
//-----------------------------------------------------------------------------
//
// -- conduit::Schema::Schema_Object_Hierarchy --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// Holds hierarchy data for schemas that describe an object.
//-----------------------------------------------------------------------------

    struct Schema_Object_Hierarchy 
    {
        std::vector<Schema*>            children;
        std::vector<std::string>        object_order;
        std::map<std::string, index_t>  object_map;
    };

    // this is used to return a ref to an empty list of strings as 
    // child names when the schema is not in the object role.
    static std::vector<std::string>     m_empty_child_names;

//-----------------------------------------------------------------------------
//
// -- conduit::Schema::Schema_List_Hierarchy --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// Holds hierarchy data for schemas that describe a list.
//-----------------------------------------------------------------------------
    struct Schema_List_Hierarchy 
    {
        std::vector<Schema*> children;
    };

//-----------------------------------------------------------------------------
//
/// -- Private methods that help with access book keeping data structures. --
//
//-----------------------------------------------------------------------------
    // for obj and list interfaces
    std::vector<Schema*>                   &children();
    std::map<std::string, index_t>         &object_map();
    std::vector<std::string>               &object_order();

    const std::vector<Schema*>             &children()  const;    
    const std::map<std::string, index_t>   &object_map()   const;
    const std::vector<std::string>         &object_order() const;

    void                                   object_map_print()   const;
    void                                   object_order_print() const;
//-----------------------------------------------------------------------------
/// Cast helpers for hierarchy data.
//-----------------------------------------------------------------------------
    Schema_Object_Hierarchy               *object_hierarchy();
    Schema_List_Hierarchy                 *list_hierarchy();

    const Schema_Object_Hierarchy         *object_hierarchy() const;
    const Schema_List_Hierarchy           *list_hierarchy()   const;

    /// helper to find the index of a given schema pointer
    index_t child_index(Schema *schema_ptr) const;


//-----------------------------------------------------------------------------
//
// -- conduit::Schema private data members --
//
//-----------------------------------------------------------------------------
    /// holds the description of this schema instance
    DataType    m_dtype;
    /// holds the schema hierarchy data.
    /// Instead of accessing this directly, use the private methods:
    ///   children(), object_map(), object_order
    /// concretely, this will be:
    /// - NULL for leaf type
    /// - A Schema_Object_Hierarchy instance for schemas describing an object
    /// - A Schema_List_Hierarchy instance for schemas describing a list
    void       *m_hierarchy_data;
    /// if this schema instance has a parent, this holds the pointer to that
    /// parent
    Schema     *m_parent;


};
//-----------------------------------------------------------------------------
// -- end conduit::Schema --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
