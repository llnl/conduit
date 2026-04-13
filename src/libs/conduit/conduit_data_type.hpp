// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_type.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_DATA_TYPE_HPP
#define CONDUIT_DATA_TYPE_HPP

//-----------------------------------------------------------------------------
// -- standard lib includes -- 
//-----------------------------------------------------------------------------
#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------
// -- conduit library includes -- 
//-----------------------------------------------------------------------------
#include "conduit_execution_qualifiers.hpp"
#include "conduit_core.hpp"
#include "conduit_endianness.hpp"
#include "conduit_utils.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- forward declarations required for conduit::Node --
//-----------------------------------------------------------------------------
class Schema;
class Node;

//-----------------------------------------------------------------------------
// -- begin conduit::DataType --
//-----------------------------------------------------------------------------
///
/// class: conduit::DataType
///
/// description:
///  DataType is used to describe a single entry in a Schema or Node Hierarchy.
///
//-----------------------------------------------------------------------------
class CONDUIT_API DataType
{
public:
//-----------------------------------------------------------------------------
/// TypeID is an Enumeration used to describe the type cases supported
///  by conduit:
//-----------------------------------------------------------------------------
    typedef enum
    {
        EMPTY_ID     = CONDUIT_EMPTY_ID,     // empty (default type)
        OBJECT_ID    = CONDUIT_OBJECT_ID,    // object
        LIST_ID      = CONDUIT_LIST_ID,      // list
        INT8_ID      = CONDUIT_INT8_ID,      // int8 and int8_array
        INT16_ID     = CONDUIT_INT16_ID,     // int16 and int16_array
        INT32_ID     = CONDUIT_INT32_ID,     // int32 and int32_array
        INT64_ID     = CONDUIT_INT64_ID,     // int64 and int64_array
        UINT8_ID     = CONDUIT_UINT8_ID,     // int8 and int8_array
        UINT16_ID    = CONDUIT_UINT16_ID,    // uint16 and uint16_array
        UINT32_ID    = CONDUIT_UINT32_ID,    // uint32 and uint32_array
        UINT64_ID    = CONDUIT_UINT64_ID,    // uint64 and uint64_array
        FLOAT32_ID   = CONDUIT_FLOAT32_ID,   // float32 and float32_array
        FLOAT64_ID   = CONDUIT_FLOAT64_ID,   // float64 and float64_array
        CHAR8_STR_ID = CONDUIT_CHAR8_STR_ID  // char8 string (incore c-string)
    } TypeID;

//-----------------------------------------------------------------------------
/// DataType::sizeof_index_t() provides an ABI stable way to probe the size
/// of index_t.
///
/// index_t is fundamental to sizes in Conduit's interface,
/// it is usually 64-bits but there is still a 32-bit option.
//-----------------------------------------------------------------------------
    static int sizeof_index_t();

//-----------------------------------------------------------------------------
// -- begin conduit::DataType Objects Constructor Helpers --
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
///
/// class: conduit::DataType:Objects
///
/// description:
///  Reference DataType instances for "object" types.
///
//-----------------------------------------------------------------------------

    static DataType empty();
    static DataType object();
    static DataType list();

//-----------------------------------------------------------------------------
// -- end conduit::DataType Objects Constructor Helpers --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::DataType Leaf Constructor Helpers --
//-----------------------------------------------------------------------------
    /// signed integer arrays
    static DataType int8(conduit::index_t num_elements=1,
                         conduit::index_t offset = 0,
                         conduit::index_t stride = sizeof(conduit::int8),
                         conduit::index_t element_bytes = sizeof(conduit::int8),
                         conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType int16(conduit::index_t num_elements=1,
                          conduit::index_t offset = 0,
                          conduit::index_t stride = sizeof(conduit::int16),
                          conduit::index_t element_bytes = sizeof(conduit::int16),
                          conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType int32(conduit::index_t num_elements=1,
                          conduit::index_t offset = 0,
                          conduit::index_t stride = sizeof(conduit::int32),
                          conduit::index_t element_bytes = sizeof(conduit::int32),
                          conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType int64(conduit::index_t num_elements=1,
                          conduit::index_t offset = 0,
                          conduit::index_t stride = sizeof(conduit::int64),
                          conduit::index_t element_bytes = sizeof(conduit::int64),
                          conduit::index_t endianness = Endianness::DEFAULT_ID);

    /// unsigned integer arrays
    static DataType uint8(conduit::index_t num_elements=1,
                          conduit::index_t offset = 0,
                          conduit::index_t stride = sizeof(conduit::uint8),
                          conduit::index_t element_bytes = sizeof(conduit::uint8),
                          conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType uint16(conduit::index_t num_elements=1,
                           conduit::index_t offset = 0,
                           conduit::index_t stride = sizeof(conduit::uint16),
                           conduit::index_t element_bytes = sizeof(conduit::uint16),
                           conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType uint32(conduit::index_t num_elements=1,
                           conduit::index_t offset = 0,
                           conduit::index_t stride = sizeof(conduit::uint32),
                           conduit::index_t element_bytes = sizeof(conduit::uint32),
                           conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType uint64(conduit::index_t num_elements=1,
                           conduit::index_t offset = 0,
                           conduit::index_t stride = sizeof(conduit::uint64),
                           conduit::index_t element_bytes = sizeof(conduit::uint64),
                           conduit::index_t endianness = Endianness::DEFAULT_ID);

    /// floating point arrays
    static DataType float32(conduit::index_t num_elements=1,
                            conduit::index_t offset = 0,
                            conduit::index_t stride = sizeof(conduit::float32),
                            conduit::index_t element_bytes=sizeof(conduit::float32),
                            conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType float64(conduit::index_t num_elements=1,
                            conduit::index_t offset = 0,
                            conduit::index_t stride = sizeof(conduit::float64),
                            conduit::index_t element_bytes=sizeof(conduit::float64),
                            conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType char8_str(conduit::index_t num_elements=1,
                              conduit::index_t offset = 0,
                              conduit::index_t stride = 1,
                              conduit::index_t element_bytes=1,
                              conduit::index_t endianness = Endianness::DEFAULT_ID);

    // Note: this is an alias to either int32, or int 64
    //       controlled by CONDUIT_INDEX_32 compile time option
    static DataType index_t(conduit::index_t num_elements=1,
                            conduit::index_t offset = 0,
                            conduit::index_t stride = sizeof(conduit_index_t),
                            conduit::index_t element_bytes=sizeof(conduit_index_t),
                            conduit::index_t endianness = Endianness::DEFAULT_ID);

//-----------------------------------------------------------------------------
// -- end conduit::DataType Leaf Constructor Helpers --
//-----------------------------------------------------------------------------
                            
//-----------------------------------------------------------------------------
// -- begin conduit::DataType C Native Leaf Constructor Helpers --
//-----------------------------------------------------------------------------
    /// signed integer arrays
    static DataType c_char(conduit::index_t num_elements=1,
                           conduit::index_t offset = 0,
                           conduit::index_t stride = sizeof(CONDUIT_NATIVE_CHAR),
                           conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR),
                           conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_short(conduit::index_t num_elements=1,
                            conduit::index_t offset = 0,
                            conduit::index_t stride = sizeof(CONDUIT_NATIVE_SHORT),
                            conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT),
                            conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_int(conduit::index_t num_elements=1,
                          conduit::index_t offset = 0,
                          conduit::index_t stride = sizeof(CONDUIT_NATIVE_INT),
                          conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_INT),
                          conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_long(conduit::index_t num_elements=1,
                           conduit::index_t offset = 0,
                           conduit::index_t stride = sizeof(CONDUIT_NATIVE_LONG),
                           conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG),
                           conduit::index_t endianness = Endianness::DEFAULT_ID);

#ifdef CONDUIT_HAS_LONG_LONG
    static DataType c_long_long(conduit::index_t num_elements=1,
                                conduit::index_t offset = 0,
                                conduit::index_t stride = sizeof(CONDUIT_NATIVE_LONG_LONG),
                                conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_LONG),
                                conduit::index_t endianness = Endianness::DEFAULT_ID);
#endif

    /// signed integer arrays
    static DataType c_signed_char(conduit::index_t num_elements=1,
                                  conduit::index_t offset = 0,
                                  conduit::index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                                  conduit::index_t element_bytes =  sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                                  conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_signed_short(conduit::index_t num_elements=1,
                                   conduit::index_t offset = 0,
                                   conduit::index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_SHORT),
                                   conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_SHORT),
                                   conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_signed_int(conduit::index_t num_elements=1,
                                 conduit::index_t offset = 0,
                                 conduit::index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_INT),
                                 conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_INT),
                                 conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_signed_long(conduit::index_t num_elements=1,
                                  conduit::index_t offset = 0,
                                  conduit::index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_LONG),
                                  conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_LONG),
                                  conduit::index_t endianness = Endianness::DEFAULT_ID);

#ifdef CONDUIT_HAS_LONG_LONG
    static DataType c_signed_long_long(conduit::index_t num_elements=1,
                                       conduit::index_t offset = 0,
                                       conduit::index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_LONG_LONG),
                                       conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_LONG_LONG),
                                       conduit::index_t endianness = Endianness::DEFAULT_ID);
#endif

    /// unsigned integer arrays
    static DataType c_unsigned_char(conduit::index_t num_elements=1,
                                    conduit::index_t offset = 0,
                                    conduit::index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                                    conduit::index_t element_bytes =  sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                                    conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_unsigned_short(conduit::index_t num_elements=1,
                                     conduit::index_t offset = 0,
                                     conduit::index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                                     conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                                     conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_unsigned_int(conduit::index_t num_elements=1,
                                   conduit::index_t offset = 0,
                                   conduit::index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                                   conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                                   conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_unsigned_long(conduit::index_t num_elements=1,
                                    conduit::index_t offset = 0,
                                    conduit::index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                                    conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                                    conduit::index_t endianness = Endianness::DEFAULT_ID);

#ifdef CONDUIT_HAS_LONG_LONG
    static DataType c_unsigned_long_long(conduit::index_t num_elements=1,
                                         conduit::index_t offset = 0,
                                         conduit::index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                                         conduit::index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                                         conduit::index_t endianness = Endianness::DEFAULT_ID);
#endif

    /// floating point arrays
    static DataType c_float(conduit::index_t num_elements=1,
                            conduit::index_t offset = 0,
                            conduit::index_t stride = sizeof(CONDUIT_NATIVE_FLOAT),
                            conduit::index_t element_bytes=sizeof(CONDUIT_NATIVE_FLOAT),
                            conduit::index_t endianness = Endianness::DEFAULT_ID);

    static DataType c_double(conduit::index_t num_elements=1,
                             conduit::index_t offset = 0,
                             conduit::index_t stride = sizeof(CONDUIT_NATIVE_DOUBLE),
                             conduit::index_t element_bytes=sizeof(CONDUIT_NATIVE_DOUBLE),
                             conduit::index_t endianness = Endianness::DEFAULT_ID);

#ifdef CONDUIT_USE_LONG_DOUBLE
    static DataType c_long_double(conduit::index_t num_elements=1,
                                  conduit::index_t offset = 0,
                                  conduit::index_t stride = sizeof(CONDUIT_NATIVE_LONG_DOUBLE),
                                  conduit::index_t element_bytes=sizeof(CONDUIT_NATIVE_LONG_DOUBLE),
                                  conduit::index_t endianness = Endianness::DEFAULT_ID);
#endif


//-----------------------------------------------------------------------------
// -- begin conduit::DataType C Native Leaf Constructor Helpers 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- conduit::DataType public methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------

    /// standard constructor
    CONDUIT_EXEC_HOST_DEVICE DataType()
    : m_id(DataType::EMPTY_ID),
      m_num_ele(0),
      m_offset(0),
      m_stride(0),
      m_ele_bytes(0),
      m_endianness(Endianness::DEFAULT_ID)
    {}
    /// copy constructor
    CONDUIT_EXEC_HOST_DEVICE DataType(const DataType& type)
    : m_id(type.m_id),
      m_num_ele(type.m_num_ele),
      m_offset(type.m_offset),
      m_stride(type.m_stride),
      m_ele_bytes(type.m_ele_bytes),
      m_endianness(type.m_endianness)
    {}
    /// Assignment operator
    CONDUIT_EXEC_HOST_DEVICE DataType& operator=(const DataType& type)
    {
        m_id = type.m_id;
        m_num_ele = type.m_num_ele;
        m_offset = type.m_offset;
        m_stride = type.m_stride;
        m_ele_bytes = type.m_ele_bytes;
        m_endianness = type.m_endianness;

        return *this;
    }
    /// construct simplest dtype for given type id
    explicit DataType(conduit::index_t id,
                      conduit::index_t num_elements=0);

    /// construct from full details, given a data type name
    DataType(const std::string &dtype_name,
             conduit::index_t num_elements,
             conduit::index_t offset,
             conduit::index_t stride,
             conduit::index_t element_bytes,
             conduit::index_t endianness);

    /// construct from full details, given a data type id
    CONDUIT_EXEC_HOST_DEVICE DataType(conduit::index_t dtype_id,
                                      conduit::index_t num_elements,
                                      conduit::index_t offset,
                                      conduit::index_t stride,
                                      conduit::index_t element_bytes,
                                      conduit::index_t endianness)
    : m_id(dtype_id),
      m_num_ele(num_elements),
      m_offset(offset),
      m_stride(stride),
      m_ele_bytes(element_bytes),
      m_endianness(endianness)
    {}

    /// destructor
   CONDUIT_EXEC_HOST_DEVICE ~DataType() = default;

   /// return a data type to the default (empty) state
   void  reset();
   
//-----------------------------------------------------------------------------
// Setters
//-----------------------------------------------------------------------------
    void       set(const DataType& type);
    
    void       set(const std::string &dtype_name,
                   conduit::index_t num_elements,
                   conduit::index_t offset,
                   conduit::index_t stride,
                   conduit::index_t element_bytes,
                   conduit::index_t endianness);    

    void       set(conduit::index_t dtype_id,
                   conduit::index_t num_elements,
                   conduit::index_t offset,
                   conduit::index_t stride,
                   conduit::index_t element_bytes,
                   conduit::index_t endianness);
    
    void       set_id(conduit::index_t dtype_id)
                    { m_id = dtype_id;}
                   
    void       set_number_of_elements(conduit::index_t v)
                    { m_num_ele = v;}
    void       set_offset(conduit::index_t v)
                    { m_offset = v;}
    void       set_stride(conduit::index_t v)
                    { m_stride = v;}
    void       set_element_bytes(conduit::index_t v)
                    { m_ele_bytes = v;}
    void       set_endianness(conduit::index_t v)
                    { m_endianness = v;}

//-----------------------------------------------------------------------------
// Getters and info methods.
//-----------------------------------------------------------------------------
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t id() const { return m_id;}
    std::string name()  const { return id_to_name(m_id);}

    CONDUIT_EXEC_HOST_DEVICE conduit::index_t number_of_elements() const
                    { return m_num_ele; }
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t offset() const
                    { return m_offset; }
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t stride() const
                    { return m_stride; }
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t element_bytes() const
                    { return m_ele_bytes; }
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t endianness() const
                    { return m_endianness; }
    CONDUIT_EXEC_HOST_DEVICE conduit::index_t element_index(conduit::index_t idx) const
                    {
#if !defined(CONDUIT_EXEC_DEVICE_COMPILE)
                        if(idx > 0 && m_stride == 0)
                        {
                            CONDUIT_WARN("Node index calculation with with stride = 0");
                        }
#endif
                        return m_offset + m_stride * idx;
                    }

    /// strided bytes = stride() * (number_of_elements() -1) + element_bytes()
    conduit::index_t     strided_bytes() const;
    // bytes compact = number_of_elements() * element_bytes()
    conduit::index_t     bytes_compact() const;
    /// spanned bytes = strided_bytes() + offet()
    conduit::index_t     spanned_bytes() const;

    bool        is_compact() const;
    bool        compatible(const DataType& type) const;
    bool        equals(const DataType& type) const;

    bool        is_empty()            const;
    bool        is_object()           const;
    bool        is_list()             const;

    bool        is_number()           const;
    bool        is_floating_point()   const;
    bool        is_integer()          const;
    bool        is_signed_integer()   const;
    bool        is_unsigned_integer() const;
    
    bool        is_int8()             const;
    bool        is_int16()            const;
    bool        is_int32()            const;
    bool        is_int64()            const;

    bool        is_uint8()            const;
    bool        is_uint16()           const;
    bool        is_uint32()           const;
    bool        is_uint64()           const;

    bool        is_float32()          const;
    bool        is_float64()          const;
    bool        is_index_t()          const;

    // native c types
    bool        is_char()             const;
    bool        is_short()            const;
    bool        is_int()              const;
    bool        is_long()             const;
    /// note: is_long_long() always returns false if conduit is not
    /// using long long to fill its support for bitwidth style types
    bool        is_long_long()      const;

    // signed c types
    bool        is_signed_char()    const;
    bool        is_signed_short()   const;
    bool        is_signed_int()     const;
    bool        is_signed_long()    const;
    /// note: is_signed_long_long() always returns false if conduit is not
    /// using long long to fill its support for bitwidth style types
    bool        is_signed_long_long() const;

    // unsigned c types
    bool        is_unsigned_char()    const;
    bool        is_unsigned_short()   const;
    bool        is_unsigned_int()     const;
    bool        is_unsigned_long()    const;
    /// note: is_unsigned_long_long() always returns false if conduit is not
    /// using long long to fill its support for bitwidth style types
    bool        is_unsigned_long_long() const;

    // floating point c types
    bool        is_float()            const;
    bool        is_double()           const;
    /// note: is_long_double() always returns false if conduit is not using 
    /// long double to fill its support for bitwidth style types
    bool        is_long_double()      const;

    // strings
    bool        is_string()           const;
    bool        is_char8_str()        const;

    // endianness
    bool        is_little_endian()    const;
    bool        is_big_endian()       const;
    bool        endianness_matches_machine() const;

//-----------------------------------------------------------------------------
// Helpers to convert TypeID Enum Values to human readable strings and 
// vice versa.
//-----------------------------------------------------------------------------
    static conduit::index_t name_to_id(const std::string &name);
    static std::string      id_to_name(conduit::index_t dtype);
    static conduit::index_t c_type_name_to_id(const std::string &name);

//-----------------------------------------------------------------------------
// Access to simple reference data types by id or name.
//-----------------------------------------------------------------------------

    static DataType default_dtype(conduit::index_t dtype_id);
    static DataType default_dtype(const std::string &name);

//-----------------------------------------------------------------------------
// Return the default number of bytes used in a given type (from a type id, or
// string)
//-----------------------------------------------------------------------------
    static conduit::index_t  default_bytes(conduit::index_t dtype_id);
    static conduit::index_t  default_bytes(const std::string &name);


//-----------------------------------------------------------------------------
// Transforms
//-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    // -- String construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a string representation of a data type.
    /// accepted protocols:
    ///   "json"
    ///   "yaml"
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_string(const std::string &protocol="json",
                                  conduit::index_t indent=2,
                                  conduit::index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;
    void                to_string_stream(std::ostream &os, 
                                         const std::string &protocol="json",
                                         conduit::index_t indent=2,
                                         conduit::index_t depth=0,
                                         const std::string &pad=" ",
                                         const std::string &eoe="\n") const;

    // NOTE(cyrush): The primary reason this function exists is to enable 
    // easier compatibility with debugging tools (e.g. totalview, gdb) that
    // have difficulty allocating default string parameters.
    std::string         to_string_default() const;

    //-----------------------------------------------------------------------------
    // -- JSON construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a JSON string representation of a data type.
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_json(conduit::index_t indent=2,
                                conduit::index_t depth=0,
                                const std::string &pad=" ",
                                const std::string &eoe="\n") const;

    void                to_json_stream(std::ostream &os,
                                       conduit::index_t indent=2,
                                       conduit::index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    // NOTE(cyrush): The primary reason this function exists is to enable 
    // easier compatibility with debugging tools (e.g. totalview, gdb) that
    // have difficulty allocating default string parameters.
    std::string         to_json_default() const;


    //-----------------------------------------------------------------------------
    // -- YAML construction methods ---
    //-----------------------------------------------------------------------------
    /// Creates a YAML string representation of a data type.
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_yaml(conduit::index_t indent=2,
                                conduit::index_t depth=0,
                                const std::string &pad=" ",
                                const std::string &eoe="\n") const;

    void                to_yaml_stream(std::ostream &os,
                                       conduit::index_t indent=2,
                                       conduit::index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    // NOTE(cyrush): The primary reason this function exists is to enable 
    // easier compatibility with debugging tools (e.g. totalview, gdb) that
    // have difficulty allocating default string parameters.
    std::string         to_yaml_default() const;

    void                compact_to(DataType &dtype) const;


private:
//-----------------------------------------------------------------------------
//
// -- conduit::DataType private data members --
//
//-----------------------------------------------------------------------------
    conduit::index_t  m_id;         /// for dtype enum value
    conduit::index_t  m_num_ele;    /// number of elements
    conduit::index_t  m_offset;     /// bytes to start of array
    conduit::index_t  m_stride;     /// bytes between start of current and start of next
    conduit::index_t  m_ele_bytes;  /// bytes per element
    conduit::index_t  m_endianness; /// endianness of elements

};
//-----------------------------------------------------------------------------
// -- end conduit::DataType --
//-----------------------------------------------------------------------------


}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
