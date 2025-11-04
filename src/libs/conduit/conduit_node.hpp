// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.


//-----------------------------------------------------------------------------
///
/// file: conduit_node.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_NODE_HPP
#define CONDUIT_NODE_HPP

//-----------------------------------------------------------------------------
// -- standard lib includes --
//-----------------------------------------------------------------------------
#include <vector>
#include <string>
#include <fstream>
#include <fstream>
#include <sstream>
#include <initializer_list>


//-----------------------------------------------------------------------------
// -- conduit includes --
//-----------------------------------------------------------------------------
#include "conduit_core.hpp"
#include "conduit_endianness.hpp"
#include "conduit_data_type.hpp"
#include "conduit_data_array.hpp"
#include "conduit_data_accessor.hpp"
#include "conduit_schema.hpp"
#include "conduit_generator.hpp"
#include "conduit_node_iterator.hpp"
#include "conduit_utils.hpp"

#if defined(CONDUIT_USE_TOTALVIEW)
// forward declaration of debugger visualizer function
int TV_ttf_display_type ( const conduit::Node *n );
#endif


//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- forward declarations required for conduit::Node --
//-----------------------------------------------------------------------------
class Generator;
class NodeIterator;
class NodeConstIterator;

//-----------------------------------------------------------------------------
// -- begin conduit::Node --
//-----------------------------------------------------------------------------
///
/// class: conduit::Node
///
/// description:
///  Node is the primary class in conduit.
///
//-----------------------------------------------------------------------------
class CONDUIT_API Node
{

//=============================================================================
//-----------------------------------------------------------------------------
//
// -- public methods --
//
//-----------------------------------------------------------------------------
//=============================================================================
public:

//-----------------------------------------------------------------------------
// -- friends of Node --
//-----------------------------------------------------------------------------
    /// Note on use of `friend`:
    ///  NodeIterator needs access to Node internals to create
    ///   an efficient iterator
    friend class NodeIterator;
    friend class NodeConstIterator;
    friend class Generator;

#if defined(CONDUIT_USE_TOTALVIEW)
    friend int ::TV_ttf_display_type ( const conduit::Node *n );
#endif

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node construction and destruction --
//
//-----------------------------------------------------------------------------
///@name Construction and Destruction
///@{
//-----------------------------------------------------------------------------
/// description:
///  Standard construction and destruction methods.
///
/// notes:
///  TODO:
///  Constructors currently use a mix of copy and pointer (external) semantics
///
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- basic constructor and destruction --
//-----------------------------------------------------------------------------
    Node();
    Node(const Node &node);
    ~Node();

    // returns any node to the empty state
    void reset();

//-----------------------------------------------------------------------------
// -- constructors for generic types --
//-----------------------------------------------------------------------------
    explicit Node(const DataType &dtype);
    explicit Node(const Schema &schema);

    /// in these methods the `external` param controls if we use copy or
    /// external semantics.
    Node(const Generator &gen,
         bool external);

    Node(const std::string &json_schema,
         void *data,
         bool external);

    Node(const Schema &schema,
         void *data,
         bool external);

    Node(const DataType &dtype,
         void *data,
         bool external);

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node construction and destruction --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node generate methods --
//
//-----------------------------------------------------------------------------
///@name Generation from JSON or YAML Schemas
///@{
//-----------------------------------------------------------------------------
/// description:
///  These methods use a Generator to parse a schema into a Node hierarchy.
///
/// * The non external variant with a NULL data parameter will allocate memory
///   for the Node hierarchy and populate with inline values from the json schema
///   (if they are provided).
///
/// * The `external' variants build a Node hierarchy that points to the input
///   data, they do not copy the data into the Node hierarchy.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Simplifed parsing w/o direct use of a generator instance
///
/// valid protocols:
///   json
///   conduit_json
///   conduit_base64_json
///   yaml
///
//-----------------------------------------------------------------------------
    void parse(const std::string &text,
               const std::string &protocol = "yaml");

//-----------------------------------------------------------------------------
// -- direct use of a generator --
//-----------------------------------------------------------------------------
    void generate(const Generator &gen);

    void generate_external(const Generator &gen);


//-----------------------------------------------------------------------------
// -- json schema optionally coupled with in-core data --
//-----------------------------------------------------------------------------
    void generate(const std::string &schema,
                  const std::string &protocol = std::string("conduit_json"),
                  void *data = NULL);

    void generate_external(const std::string &schema,
                           const std::string &protocol,
                           void *data);

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node generate methods --
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// -- begin declaration of Node basic i/o methods --
//
//-----------------------------------------------------------------------------
///@name Text, Binary and Memory-Mapped I/O
///@{
//-----------------------------------------------------------------------------
/// description:
///
//-----------------------------------------------------------------------------
    void load(const std::string &stream_path,
              const std::string &protocol="");

    void load(const std::string &stream_path,
              const Schema &schema);

    void save(const std::string &stream_path,
              const std::string &protocol="") const;

    void mmap(const std::string &stream_path);

    void mmap(const std::string &stream_path,
              const Schema &schema);


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node basic i/o methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node allocator selection methods --
//
//-----------------------------------------------------------------------------
///@name Allocator Selection
///@{
//-----------------------------------------------------------------------------
/// description:
///
//-----------------------------------------------------------------------------
    void    set_allocator(index_t allocator_id);
    index_t allocator() const;
    void    reset_allocator();
//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node allocator selection methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node set methods --
//
//-----------------------------------------------------------------------------
///@name Node::set(...)
///@{
//-----------------------------------------------------------------------------
/// description:
///   set(...) methods follow copy semantics.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set for generic types --
//-----------------------------------------------------------------------------
    void set_node(const Node &data);
    void set(const Node &data);

    void set_dtype(const DataType &dtype);
    void set(const DataType &dtype);

    void set_schema(const Schema &schema);
    void set(const Schema &schema);

    void set_data_using_schema(const Schema &schema, void *data);
    void set(const Schema &schema, void *data);

    void set_data_using_dtype(const DataType &dtype, void *data);
    void set(const DataType &dtype, void *data);

//-----------------------------------------------------------------------------
// -- set for bitwidth style scalar types ---
//-----------------------------------------------------------------------------
    // signed integer scalar types
    void set_int8(int8 data);
    void set(int8 data);

    void set_int16(int16 data);
    void set(int16 data);

    void set_int32(int32 data);
    void set(int32 data);

    void set_int64(int64 data);
    void set(int64 data);

    // unsigned integer scalar types
    void set_uint8(uint8 data);
    void set(uint8 data);

    void set_uint16(uint16 data);
    void set(uint16 data);

    void set_uint32(uint32 data);
    void set(uint32 data);

    void set_uint64(uint64 data);
    void set(uint64 data);

    // floating point scalar types
    void set_float32(float32 data);
    void set(float32 data);

    void set_float64(float64 data);
    void set(float64 data);

//-----------------------------------------------------------------------------
//  set scalar gap methods for c-native types
//-----------------------------------------------------------------------------
//  These set methods are used to fill out the interface for cases where
//  any of the native c types are not mapped 1-1 to our to bit width style
//  types.
//
//  Windows is one important case where this happens. Both long and int
//  represent 32-bit integers, and long long is used as the type for 64-bit
//  integers. In this case the long and int are aliased types -- we want
//  to support both via overloaded "set" functions, however one of the types
//  is already used as the underlying type for set(int32 ). When int is
//  selected as int32, Visual Studio needs an explicit method to disambiguate
//  the long case.
//-----------------------------------------------------------------------------
    void set(char data);

    #ifndef CONDUIT_USE_CHAR
        void set(signed char data);
        void set(unsigned char data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set(short data);
        void set(unsigned short data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set(int data);
        void set(unsigned int data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set(long data);
        void set(unsigned long data);
    #endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set(long long data);
        void set(unsigned long long data);
#endif

    #ifndef CONDUIT_USE_FLOAT
        void set(float data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set(double data);
    #endif


//-----------------------------------------------------------------------------
// -- set for conduit::DataArray types ---
//-----------------------------------------------------------------------------
    // signed integer array types via conduit::DataArray
    void set_int8_array(const int8_array  &data);
    void set(const int8_array  &data);

    void set_int16_array(const int16_array &data);
    void set(const int16_array &data);

    void set_int32_array(const int32_array &data);
    void set(const int32_array &data);

    void set_int64_array(const int64_array &data);
    void set(const int64_array &data);

    // unsigned integer array types via conduit::DataArray
    void set_uint8_array(const uint8_array  &data);
    void set(const uint8_array  &data);

    void set_uint16_array(const uint16_array &data);
    void set(const uint16_array &data);

    void set_uint32_array(const uint32_array &data);
    void set(const uint32_array &data);

    void set_uint64_array(const uint64_array &data);
    void set(const uint64_array &data);

    // floating point array types via conduit::DataArray
    void set_float32_array(const float32_array &data);
    void set(const float32_array &data);

    void set_float64_array(const float64_array &data);
    void set(const float64_array &data);


//-----------------------------------------------------------------------------
//  set array gap methods for c-native types
//-----------------------------------------------------------------------------
    // we never use char directly, so we always need this
    void set(const char_array &data);

    #ifndef CONDUIT_USE_CHAR
        void set(const signed_char_array &data);
        void set(const unsigned_char_array &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set(const short_array &data);
        void set(const unsigned_unsigned_array &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set(const int_array &data);
        void set(const unsigned_int_array &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set(const long_array &data);
        void set(const unsigned_long_array &data);
    #endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set(const long_long_array &data);
        void set(const unsigned_long_long_array &data);
#endif

    #ifndef CONDUIT_USE_FLOAT
        void set(const float_array &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set(const double_array &data);
    #endif


//-----------------------------------------------------------------------------
// -- set for string types --
//-----------------------------------------------------------------------------
    // char8_str use cases
    void set_string(const std::string &data);
    void set(const std::string &data);
    // special explicit case for string to avoid any overloading ambiguity
    void set_char8_str(const char *data);

//-----------------------------------------------------------------------------
// -- set for bitwidth style std::vector types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_int8_vector(const std::vector<int8>   &data);
    void set(const std::vector<int8>   &data);

    //-------------------------------------------------------------------------
    void set_int16_vector(const std::vector<int16>  &data);
    void set(const std::vector<int16>  &data);

    //-------------------------------------------------------------------------
    void set_int32_vector(const std::vector<int32>  &data);
    void set(const std::vector<int32>  &data);

    //-------------------------------------------------------------------------
    void set_int64_vector(const std::vector<int64>  &data);
    void set(const std::vector<int64>  &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_uint8_vector(const std::vector<uint8>   &data);
    void set(const std::vector<uint8>   &data);

    //-------------------------------------------------------------------------
    void set_uint16_vector(const std::vector<uint16>  &data);
    void set(const std::vector<uint16>  &data);

    //-------------------------------------------------------------------------
    void set_uint32_vector(const std::vector<uint32>  &data);
    void set(const std::vector<uint32>  &data);

    //-------------------------------------------------------------------------
    void set_uint64_vector(const std::vector<uint64>  &data);
    void set(const std::vector<uint64>  &data);

    //-------------------------------------------------------------------------
    // floating point array types via std::vector
    //-------------------------------------------------------------------------
    void set_float32_vector(const std::vector<float32> &data);
    void set(const std::vector<float32> &data);

    //-------------------------------------------------------------------------
    void set_float64_vector(const std::vector<float64> &data);
    void set(const std::vector<float64> &data);

//-----------------------------------------------------------------------------
//  set vector gap methods for c-native types
//-----------------------------------------------------------------------------
    void set(const std::vector<char> &data);

    #ifndef CONDUIT_USE_CHAR
        void set(const std::vector<signed char> &data);
        void set(const std::vector<unsigned char> &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set(const std::vector<short> &data);
        void set(const std::vector<unsigned short> &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set(const std::vector<int> &data);
        void set(const std::vector<unsigned int> &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set(const std::vector<long> &data);
        void set(const std::vector<unsigned long> &data);
    #endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set(const std::vector<long long> &data);
        void set(const std::vector<unsigned long long> &data);
#endif

    #ifndef CONDUIT_USE_FLOAT
        void set(const std::vector<float> &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set(const std::vector<double> &data);
    #endif


//-----------------------------------------------------------------------------
// -- std::initializer_list support --
//-----------------------------------------------------------------------------
//
// Support std::initializer_lists
//
// Example:
//   Node n;
//   n.set({1,2,3,4,5,6});
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set for bitwidth style std::initializer_list types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer array types via std::initializer_list
    //-------------------------------------------------------------------------
    void set_int8_initializer_list(const std::initializer_list<int8>   &data);
    void set(const std::initializer_list<int8>   &data);

    //-------------------------------------------------------------------------
    void set_int16_initializer_list(const std::initializer_list<int16>  &data);
    void set(const std::initializer_list<int16>  &data);

    //-------------------------------------------------------------------------
    void set_int32_initializer_list(const std::initializer_list<int32>  &data);
    void set(const std::initializer_list<int32>  &data);

    //-------------------------------------------------------------------------
    void set_int64_initializer_list(const std::initializer_list<int64>  &data);
    void set(const std::initializer_list<int64>  &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via std::initializer_list
    //-------------------------------------------------------------------------
    void set_uint8_initializer_list(const std::initializer_list<uint8>   &data);
    void set(const std::initializer_list<uint8>   &data);

    //-------------------------------------------------------------------------
    void set_uint16_initializer_list(const std::initializer_list<uint16>  &data);
    void set(const std::initializer_list<uint16>  &data);

    //-------------------------------------------------------------------------
    void set_uint32_initializer_list(const std::initializer_list<uint32>  &data);
    void set(const std::initializer_list<uint32>  &data);

    //-------------------------------------------------------------------------
    void set_uint64_initializer_list(const std::initializer_list<uint64>  &data);
    void set(const std::initializer_list<uint64>  &data);

    //-------------------------------------------------------------------------
    // floating point array types via std::initializer_list
    //-------------------------------------------------------------------------
    void set_float32_initializer_list(const std::initializer_list<float32> &data);
    void set(const std::initializer_list<float32> &data);

    //-------------------------------------------------------------------------
    void set_float64_initializer_list(const std::initializer_list<float64> &data);
    void set(const std::initializer_list<float64> &data);

//-----------------------------------------------------------------------------
//  set initializer_list gap methods for c-native types
//-----------------------------------------------------------------------------
    void set(const std::initializer_list<char> &data);

    #ifndef CONDUIT_USE_CHAR
        void set(const std::initializer_list<signed char> &data);
        void set(const std::initializer_list<unsigned char> &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set(const std::initializer_list<short> &data);
        void set(const std::initializer_list<unsigned short> &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set(const std::initializer_list<int> &data);
        void set(const std::initializer_list<unsigned int> &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set(const std::initializer_list<long> &data);
        void set(const std::initializer_list<unsigned long> &data);
    #endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set(const std::initializer_list<long long> &data);
        void set(const std::initializer_list<unsigned long long> &data);
#endif

    #ifndef CONDUIT_USE_FLOAT
        void set(const std::initializer_list<float> &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set(const std::initializer_list<double> &data);
    #endif

//-----------------------------------------------------------------------------
// -- set via bitwidth style pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer pointer cases
    //-------------------------------------------------------------------------
    void set_int8_ptr(const int8 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::int8),
                      index_t element_bytes = sizeof(conduit::int8),
                      index_t endianness = Endianness::DEFAULT_ID);

    void set(const int8 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::int8),
             index_t element_bytes = sizeof(conduit::int8),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_int16_ptr(const int16 *data,
                       index_t num_elements = 1,
                       index_t offset = 0,
                       index_t stride = sizeof(conduit::int16),
                       index_t element_bytes = sizeof(conduit::int16),
                       index_t endianness = Endianness::DEFAULT_ID);

    void set(const int16 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::int16),
             index_t element_bytes = sizeof(conduit::int16),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_int32_ptr(const int32 *data,
                       index_t num_elements = 1,
                       index_t offset = 0,
                       index_t stride = sizeof(conduit::int32),
                       index_t element_bytes = sizeof(conduit::int32),
                       index_t endianness = Endianness::DEFAULT_ID);

    void set(const int32 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::int32),
             index_t element_bytes = sizeof(conduit::int32),
             index_t endianness = Endianness::DEFAULT_ID);


    //-------------------------------------------------------------------------
    void set_int64_ptr(const int64 *data,
                       index_t num_elements = 1,
                       index_t offset = 0,
                       index_t stride = sizeof(conduit::int64),
                       index_t element_bytes = sizeof(conduit::int64),
                       index_t endianness = Endianness::DEFAULT_ID);

    void set(const int64 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::int64),
             index_t element_bytes = sizeof(conduit::int64),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // unsigned integer pointer cases
    //-------------------------------------------------------------------------
    void set_uint8_ptr(const uint8 *data,
                       index_t num_elements = 1,
                       index_t offset = 0,
                       index_t stride = sizeof(conduit::uint8),
                       index_t element_bytes = sizeof(conduit::uint8),
                       index_t endianness = Endianness::DEFAULT_ID);

    void set(const uint8 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::uint8),
             index_t element_bytes = sizeof(conduit::uint8),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_uint16_ptr(const uint16 *data,
                       index_t num_elements = 1,
                       index_t offset = 0,
                       index_t stride = sizeof(conduit::uint16),
                       index_t element_bytes = sizeof(conduit::uint16),
                       index_t endianness = Endianness::DEFAULT_ID);

    void set(const uint16 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::uint16),
             index_t element_bytes = sizeof(conduit::uint16),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_uint32_ptr(const uint32 *data,
                        index_t num_elements = 1,
                        index_t offset = 0,
                        index_t stride = sizeof(conduit::uint32),
                        index_t element_bytes = sizeof(conduit::uint32),
                        index_t endianness = Endianness::DEFAULT_ID);

    void set(const uint32 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::uint32),
             index_t element_bytes = sizeof(conduit::uint32),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_uint64_ptr(const uint64 *data,
                        index_t num_elements = 1,
                        index_t offset = 0,
                        index_t stride = sizeof(conduit::uint64),
                        index_t element_bytes = sizeof(conduit::uint64),
                        index_t endianness = Endianness::DEFAULT_ID);

    void set(const uint64 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::uint64),
             index_t element_bytes = sizeof(conduit::uint64),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // floating point pointer cases
    //-------------------------------------------------------------------------
    void set_float32_ptr(const float32 *data,
                         index_t num_elements = 1,
                         index_t offset = 0,
                         index_t stride = sizeof(conduit::float32),
                         index_t element_bytes = sizeof(conduit::float32),
                         index_t endianness = Endianness::DEFAULT_ID);

    void set(const float32 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::float32),
             index_t element_bytes = sizeof(conduit::float32),
             index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_float64_ptr(const float64 *data,
                         index_t num_elements = 1,
                         index_t offset = 0,
                         index_t stride = sizeof(conduit::float64),
                         index_t element_bytes = sizeof(conduit::float64),
                         index_t endianness = Endianness::DEFAULT_ID);

    void set(const float64 *data,
             index_t num_elements = 1,
             index_t offset = 0,
             index_t stride = sizeof(conduit::float64),
             index_t element_bytes = sizeof(conduit::float64),
             index_t endianness = Endianness::DEFAULT_ID);


//-----------------------------------------------------------------------------
//  set via pointer gap methods for c-native types
//-----------------------------------------------------------------------------
   //-------------------------------------------------------------------------
   // Char is never used in the interface, and set(char* ) is reserved
   // for strings, so we provide a set_char_ptr if folks want to
   //
   //-------------------------------------------------------------------------
   void set_char_ptr(const char *data,
                     index_t num_elements = 1,
                     index_t offset = 0,
                     index_t stride = sizeof(CONDUIT_NATIVE_CHAR),
                     index_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR),
                     index_t endianness = Endianness::DEFAULT_ID);

    #ifndef CONDUIT_USE_CHAR
        void set(const signed char *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                 index_t endianness = Endianness::DEFAULT_ID);

        void set(const unsigned char *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set(const short *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_SHORT),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT),
                 index_t endianness = Endianness::DEFAULT_ID);

        void set(const unsigned short *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_INT
        void set(const int *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_INT),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_INT),
                 index_t endianness = Endianness::DEFAULT_ID);

        void set(const unsigned int *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set(const long *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_LONG),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG),
                 index_t endianness = Endianness::DEFAULT_ID);

        void set(const unsigned long *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set(const long long *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_LONG_LONG),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_LONG),
                 index_t endianness = Endianness::DEFAULT_ID);

        void set(const unsigned long long *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set(const float *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_FLOAT),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_FLOAT),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set(const double *data,
                 index_t num_elements = 1,
                 index_t offset = 0,
                 index_t stride = sizeof(CONDUIT_NATIVE_DOUBLE),
                 index_t element_bytes = sizeof(CONDUIT_NATIVE_DOUBLE),
                 index_t endianness = Endianness::DEFAULT_ID);
    #endif

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node set methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node set_path methods --
//
//-----------------------------------------------------------------------------
///@name Node::set_path(...)
///@{
//-----------------------------------------------------------------------------
/// description:
///   set_path(...) methods methods follow copy semantics and allow you to use
///    an explicit path for the destination.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set_path for generic types --
//-----------------------------------------------------------------------------
    void set_path_node(const std::string &path,
                       const Node& data);
    void set_path(const std::string &path,
                  const Node& data);

    //-------------------------------------------------------------------------
    void set_path_dtype(const std::string &path,
                        const DataType& dtype);

    void set_path(const std::string &path,
                  const DataType& dtype);

    //-------------------------------------------------------------------------
    void set_path_schema(const std::string &path,
                         const Schema &schema);

    void set_path(const std::string &path,
                  const Schema &schema);

    //-------------------------------------------------------------------------
    void set_path_data_using_schema(const std::string &path,
                                    const Schema &schema,
                                    void *data);

    void set_path(const std::string &path,
                  const Schema &schema,
                  void *data);

    //-------------------------------------------------------------------------
    void set_path_data_using_dtype(const std::string &path,
                                   const DataType &dtype,
                                   void *data);

    void set_path(const std::string &path,
                  const DataType &dtype,
                  void *data);

//-----------------------------------------------------------------------------
// -- set_path for bitwidth style scalar types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer scalar types
    //-------------------------------------------------------------------------
    void set_path_int8(const std::string &path, int8 data);
    void set_path(const std::string &path, int8 data);

    //-------------------------------------------------------------------------
    void set_path_int16(const std::string &path, int16 data);
    void set_path(const std::string &path, int16 data);

    //-------------------------------------------------------------------------
    void set_path_int32(const std::string &path, int32 data);
    void set_path(const std::string &path, int32 data);

    //-------------------------------------------------------------------------
    void set_path_int64(const std::string &path, int64 data);
    void set_path(const std::string &path, int64 data);

    //-------------------------------------------------------------------------
    // unsigned integer scalar types
    //-------------------------------------------------------------------------
    void set_path_uint8(const std::string &path, uint8 data);
    void set_path(const std::string &path, uint8 data);

    //-------------------------------------------------------------------------
    void set_path_uint16(const std::string &path, uint16 data);
    void set_path(const std::string &path, uint16 data);

    //-------------------------------------------------------------------------
    void set_path_uint32(const std::string &path, uint32 data);
    void set_path(const std::string &path, uint32 data);

    //-------------------------------------------------------------------------
    void set_path_uint64(const std::string &path, uint64 data);
    void set_path(const std::string &path, uint64 data);

    //-------------------------------------------------------------------------
    // floating point scalar types
    //-------------------------------------------------------------------------
    void set_path_float32(const std::string &path, float32 data);
    void set_path(const std::string &path, float32 data);

    //-------------------------------------------------------------------------
    void set_path_float64(const std::string &path, float64 data);
    void set_path(const std::string &path, float64 data);

//-----------------------------------------------------------------------------
//  set_path scalar gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path(const std::string &path, char data);

    #ifndef CONDUIT_USE_CHAR
        void set_path(const std::string &path, signed char data);
        void set_path(const std::string &path, unsigned char data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path(const std::string &path, short data);
        void set_path(const std::string &path, unsigned short data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path(const std::string &path, int data);
        void set_path(const std::string &path, unsigned int data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path(const std::string &path, long data);
        void set_path(const std::string &path, unsigned long data);
    #endif
    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path(const std::string &path, long long data);
        void set_path(const std::string &path, unsigned long long data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path(const std::string &path, float data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path(const std::string &path, double data);
    #endif


//-----------------------------------------------------------------------------
// -- set_path for conduit::DataArray types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_int8_array(const std::string &path,
                             const int8_array  &data);

    void set_path(const std::string &path, const int8_array &data);

    //-------------------------------------------------------------------------
    void set_path_int16_array(const std::string &path,
                              const int16_array &data);

    void set_path(const std::string &path, const int16_array &data);

    //-------------------------------------------------------------------------
    void set_path_int32_array(const std::string &path,
                              const int32_array &data);

    void set_path(const std::string &path, const int32_array &data);

    //-------------------------------------------------------------------------
    void set_path_int64_array(const std::string &path,
                              const int64_array &data);

    void set_path(const std::string &path, const int64_array &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_uint8_array(const std::string &path,
                              const uint8_array  &data);

    void set_path(const std::string &path, const uint8_array  &data);

    //-------------------------------------------------------------------------
    void set_path_uint16_array(const std::string &path,
                               const uint16_array &data);

    void set_path(const std::string &path, const uint16_array &data);

    //-------------------------------------------------------------------------
    void set_path_uint32_array(const std::string &path,
                               const uint32_array &data);

    void set_path(const std::string &path, const uint32_array &data);

    //-------------------------------------------------------------------------
    void set_path_uint64_array(const std::string &path,
                               const uint64_array &data);

    void set_path(const std::string &path, const uint64_array &data);

    //-------------------------------------------------------------------------
    // floating point array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_float32_array(const std::string &path,
                                const float32_array &data);

    void set_path(const std::string &path, const float32_array &data);

    //-------------------------------------------------------------------------
    void set_path_float64_array(const std::string &path,
                                const float64_array &data);

    void set_path(const std::string &path, const float64_array &data);

//-----------------------------------------------------------------------------
//  set_path array gap methods for c-native types
//-----------------------------------------------------------------------------

    void set_path(const std::string &path,
                  const char_array &data);

    #ifndef CONDUIT_USE_CHAR
        void set_path(const std::string &path,
                      const signed_char_array &data);

        void set_path(const std::string &path,
                      const unsigned_char_array &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path(const std::string &path,
                      const short_array &data);

        void set_path(const std::string &path,
                      const unsigned_short_array &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path(const std::string &path,
                      const int_array &data);

        void set_path(const std::string &path,
                      const unsigned_int_array &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path(const std::string &path,
                      const long_array &data);

        void set_path(const std::string &path,
                      const unsigned_long_array &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path(const std::string &path,
                      const long_long_array &data);

        void set_path(const std::string &path,
                      const unsigned_long_long_array &data);

    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path(const std::string &path,
                      const float_array &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path(const std::string &path,
                      const double_array &data);
    #endif

//-----------------------------------------------------------------------------
// -- set_path for string types --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // char8_str use cases
    //-------------------------------------------------------------------------
    void set_path_string(const std::string &path,
                         const std::string &data);

    void set_path(const std::string &path,
                  const std::string &data);

    //-------------------------------------------------------------------------
    // special explicit case for string to avoid any overloading ambiguity
    //-------------------------------------------------------------------------
    void set_path_char8_str(const std::string &path,
                            const char* data);


//-----------------------------------------------------------------------------
// -- set_path for bitwidth style std::vector types ---
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_int8_vector(const std::string &path,
                              const std::vector<int8> &data);

    void set_path(const std::string &path, const std::vector<int8> &data);

    //-------------------------------------------------------------------------
    void set_path_int16_vector(const std::string &path,
                               const std::vector<int16> &data);

    void set_path(const std::string &path, const std::vector<int16> &data);

    //-------------------------------------------------------------------------
    void set_path_int32_vector(const std::string &path,
                               const std::vector<int32> &data);

    void set_path(const std::string &path, const std::vector<int32> &data);

    //-------------------------------------------------------------------------
    void set_path_int64_vector(const std::string &path,
                               const std::vector<int64> &data);

    void set_path(const std::string &path, const std::vector<int64> &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_uint8_vector(const std::string &path,
                               const std::vector<uint8> &data);

    void set_path(const std::string &path, const std::vector<uint8> &data);

    //-------------------------------------------------------------------------
    void set_path_uint16_vector(const std::string &path,
                                const std::vector<uint16> &data);

    void set_path(const std::string &path, const std::vector<uint16> &data);

    //-------------------------------------------------------------------------
    void set_path_uint32_vector(const std::string &path,
                                const std::vector<uint32> &data);
    void set_path(const std::string &path, const std::vector<uint32> &data);

    //-------------------------------------------------------------------------
    void set_path_uint64_vector(const std::string &path,
                                const std::vector<uint64> &data);

    void set_path(const std::string &path, const std::vector<uint64> &data);

    //-------------------------------------------------------------------------
    // floating point array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_float32_vector(const std::string &path,
                                 const std::vector<float32> &data);

    void set_path(const std::string &path, const std::vector<float32> &data);


    //-------------------------------------------------------------------------
    void set_path_float64_vector(const std::string &path,
                                 const std::vector<float64> &data);

    void set_path(const std::string &path, const std::vector<float64> &data);


//-----------------------------------------------------------------------------
//  set_path vector gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path(const std::string &path,
                  const std::vector<char> &data);

    #ifndef CONDUIT_USE_CHAR
        void set_path(const std::string &path,
                      const std::vector<signed char> &data);

        void set_path(const std::string &path,
                      const std::vector<unsigned char> &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path(const std::string &path,
                      const std::vector<short> &data);

        void set_path(const std::string &path,
                      const std::vector<unsigned short> &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path(const std::string &path,
                      const std::vector<int> &data);

        void set_path(const std::string &path,
                      const std::vector<unsigned int> &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path(const std::string &path,
                      const std::vector<long> &data);

        void set_path(const std::string &path,
                      const std::vector<unsigned long> &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path(const std::string &path,
                      const std::vector<long long> &data);

        void set_path(const std::string &path,
                      const std::vector<unsigned long long> &data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path(const std::string &path,
                      const std::vector<float> &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path(const std::string &path,
                      const std::vector<double> &data);
    #endif


//-----------------------------------------------------------------------------
// -- set_path via bitwidth style pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer pointer cases
    //-------------------------------------------------------------------------
    void set_path_int8_ptr(const std::string &path,
                           const int8 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::int8),
                           index_t element_bytes = sizeof(conduit::int8),
                           index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const int8 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::int8),
                  index_t element_bytes = sizeof(conduit::int8),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_int16_ptr(const std::string &path,
                            const int16 *data,
                            index_t num_elements = 1,
                            index_t offset = 0,
                            index_t stride = sizeof(conduit::int16),
                            index_t element_bytes = sizeof(conduit::int16),
                            index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const int16 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::int16),
                  index_t element_bytes = sizeof(conduit::int16),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_int32_ptr(const std::string &path,
                            const int32 *data,
                            index_t num_elements = 1,
                            index_t offset = 0,
                            index_t stride = sizeof(conduit::int32),
                            index_t element_bytes = sizeof(conduit::int32),
                            index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const int32 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::int32),
                  index_t element_bytes = sizeof(conduit::int32),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_int64_ptr(const std::string &path,
                            const int64 *data,
                            index_t num_elements = 1,
                            index_t offset = 0,
                            index_t stride = sizeof(conduit::int64),
                            index_t element_bytes = sizeof(conduit::int64),
                            index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const int64 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::int64),
                  index_t element_bytes = sizeof(conduit::int64),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // unsigned integer pointer cases
    //-------------------------------------------------------------------------
    void set_path_uint8_ptr(const std::string &path,
                            const uint8 *data,
                            index_t num_elements = 1,
                            index_t offset = 0,
                            index_t stride = sizeof(conduit::uint8),
                            index_t element_bytes = sizeof(conduit::uint8),
                            index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const uint8 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::uint8),
                  index_t element_bytes = sizeof(conduit::uint8),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_uint16_ptr(const std::string &path,
                             const uint16 *data,
                             index_t num_elements = 1,
                             index_t offset = 0,
                             index_t stride = sizeof(conduit::uint16),
                             index_t element_bytes = sizeof(conduit::uint16),
                             index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const uint16 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::uint16),
                  index_t element_bytes = sizeof(conduit::uint16),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_uint32_ptr(const std::string &path,
                             const uint32 *data,
                             index_t num_elements = 1,
                             index_t offset = 0,
                             index_t stride = sizeof(conduit::uint32),
                             index_t element_bytes = sizeof(conduit::uint32),
                             index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const uint32 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::uint32),
                  index_t element_bytes = sizeof(conduit::uint32),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_uint64_ptr(const std::string &path,
                             const uint64 *data,
                             index_t num_elements = 1,
                             index_t offset = 0,
                             index_t stride = sizeof(conduit::uint64),
                             index_t element_bytes = sizeof(conduit::uint64),
                             index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const uint64 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::uint64),
                  index_t element_bytes = sizeof(conduit::uint64),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // floating point integer pointer cases
    //-------------------------------------------------------------------------
    void set_path_float32_ptr(const std::string &path,
                              const float32 *data,
                              index_t num_elements = 1,
                              index_t offset = 0,
                              index_t stride = sizeof(conduit::float32),
                              index_t element_bytes = sizeof(conduit::float32),
                              index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const float32 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::float32),
                  index_t element_bytes = sizeof(conduit::float32),
                  index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_float64_ptr(const std::string &path,
                              const float64 *data,
                              index_t num_elements = 1,
                              index_t offset = 0,
                              index_t stride = sizeof(conduit::float64),
                              index_t element_bytes = sizeof(conduit::float64),
                              index_t endianness = Endianness::DEFAULT_ID);

    void set_path(const std::string &path,
                  const float64 *data,
                  index_t num_elements = 1,
                  index_t offset = 0,
                  index_t stride = sizeof(conduit::float64),
                  index_t element_bytes = sizeof(conduit::float64),
                  index_t endianness = Endianness::DEFAULT_ID);


//-----------------------------------------------------------------------------
//  set via pointer gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path_char_ptr(const std::string &path,
                           const char *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(CONDUIT_NATIVE_CHAR),
                           index_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR),
                           index_t endianness = Endianness::DEFAULT_ID);

    #ifndef CONDUIT_USE_CHAR
        void set_path(const std::string &path,
                      const signed char *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                      index_t endianness = Endianness::DEFAULT_ID);

        void set_path(const std::string &path,
                      const unsigned char *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path(const std::string &path,
                      const short *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_SHORT),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT),
                      index_t endianness = Endianness::DEFAULT_ID);

        void set_path(const std::string &path,
                      const unsigned short *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path(const std::string &path,
                      const int *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_INT),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_INT),
                      index_t endianness = Endianness::DEFAULT_ID);

        void set_path(const std::string &path,
                      const unsigned int *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path(const std::string &path,
                      const long *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_LONG),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG),
                      index_t endianness = Endianness::DEFAULT_ID);

        void set_path(const std::string &path,
                      const unsigned long *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path(const std::string &path,
                      const long long *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_LONG_LONG),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_LONG),
                      index_t endianness = Endianness::DEFAULT_ID);

        void set_path(const std::string &path,
                      const unsigned long long *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                      index_t endianness = Endianness::DEFAULT_ID);

    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path(const std::string &path,
                      const float *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_FLOAT),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_FLOAT),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path(const std::string &path,
                      const double *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(CONDUIT_NATIVE_DOUBLE),
                      index_t element_bytes = sizeof(CONDUIT_NATIVE_DOUBLE),
                      index_t endianness = Endianness::DEFAULT_ID);
    #endif

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node set_path methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node set_external methods --
//
//-----------------------------------------------------------------------------
///@name Node::set_external(...)
///@{
//-----------------------------------------------------------------------------
/// description:
///   set_external(...) methods methods follow pointer semantics.
///   (they do not copy data into the node, but point to the data passed)
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// -- set external for generic types --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    void set_external_node(const Node &n);
    void set_external(const Node &n);

    //-------------------------------------------------------------------------
    void set_external_data_using_schema(const Schema &schema,
                                        void *data);

    void set_external(const Schema &schema,
                      void *data);

    //-------------------------------------------------------------------------
    void set_external_data_using_dtype(const DataType &dtype,
                                       void *data);

    void set_external(const DataType &dtype,
                      void *data);

//-----------------------------------------------------------------------------
// -- set_external via bitwidth style pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer pointer cases
    //-------------------------------------------------------------------------
    void set_external_int8_ptr(int8  *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(conduit::int8),
                               index_t element_bytes = sizeof(conduit::int8),
                               index_t endianness = Endianness::DEFAULT_ID);

    void set_external(int8  *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::int8),
                      index_t element_bytes = sizeof(conduit::int8),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_int16_ptr(int16 *data,
                                index_t num_elements = 1,
                                index_t offset = 0,
                                index_t stride = sizeof(conduit::int16),
                                index_t element_bytes = sizeof(conduit::int16),
                                index_t endianness = Endianness::DEFAULT_ID);

    void set_external(int16 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::int16),
                      index_t element_bytes = sizeof(conduit::int16),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_int32_ptr(int32 *data,
                                index_t num_elements = 1,
                                index_t offset = 0,
                                index_t stride = sizeof(conduit::int32),
                                index_t element_bytes = sizeof(conduit::int32),
                                index_t endianness = Endianness::DEFAULT_ID);

    void set_external(int32 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::int32),
                      index_t element_bytes = sizeof(conduit::int32),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_int64_ptr(int64 *data,
                                index_t num_elements = 1,
                                index_t offset = 0,
                                index_t stride = sizeof(conduit::int64),
                                index_t element_bytes = sizeof(conduit::int64),
                                index_t endianness = Endianness::DEFAULT_ID);

    void set_external(int64 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::int64),
                      index_t element_bytes = sizeof(conduit::int64),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // unsigned integer pointer cases
    //-------------------------------------------------------------------------
    void set_external_uint8_ptr(uint8  *data,
                                index_t num_elements = 1,
                                index_t offset = 0,
                                index_t stride = sizeof(conduit::uint8),
                                index_t element_bytes = sizeof(conduit::uint8),
                                index_t endianness = Endianness::DEFAULT_ID);

    void set_external(uint8  *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::uint8),
                      index_t element_bytes = sizeof(conduit::uint8),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_uint16_ptr(uint16 *data,
                                 index_t num_elements = 1,
                                 index_t offset = 0,
                                 index_t stride = sizeof(conduit::uint16),
                                 index_t element_bytes = sizeof(conduit::uint16),
                                 index_t endianness = Endianness::DEFAULT_ID);

    void set_external(uint16 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::uint16),
                      index_t element_bytes = sizeof(conduit::uint16),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_uint32_ptr(uint32 *data,
                                 index_t num_elements = 1,
                                 index_t offset = 0,
                                 index_t stride = sizeof(conduit::uint32),
                                 index_t element_bytes = sizeof(conduit::uint32),
                                 index_t endianness = Endianness::DEFAULT_ID);

    void set_external(uint32 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::uint32),
                      index_t element_bytes = sizeof(conduit::uint32),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_uint64_ptr(uint64 *data,
                                 index_t num_elements = 1,
                                 index_t offset = 0,
                                 index_t stride = sizeof(conduit::uint64),
                                 index_t element_bytes = sizeof(conduit::uint64),
                                 index_t endianness = Endianness::DEFAULT_ID);

    void set_external(uint64 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::uint64),
                      index_t element_bytes = sizeof(conduit::uint64),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // floating point pointer cases
    //-------------------------------------------------------------------------
    void set_external_float32_ptr(float32 *data,
                                  index_t num_elements = 1,
                                  index_t offset = 0,
                                  index_t stride = sizeof(conduit::float32),
                                  index_t element_bytes = sizeof(conduit::float32),
                                  index_t endianness = Endianness::DEFAULT_ID);

    void set_external(float32 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::float32),
                      index_t element_bytes = sizeof(conduit::float32),
                      index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_external_float64_ptr(float64 *data,
                                  index_t num_elements = 1,
                                  index_t offset = 0,
                                  index_t stride = sizeof(conduit::float64),
                                  index_t element_bytes = sizeof(conduit::float64),
                                  index_t endianness = Endianness::DEFAULT_ID);

    void set_external(float64 *data,
                      index_t num_elements = 1,
                      index_t offset = 0,
                      index_t stride = sizeof(conduit::float64),
                      index_t element_bytes = sizeof(conduit::float64),
                      index_t endianness = Endianness::DEFAULT_ID);

//-----------------------------------------------------------------------------
//  set via pointer gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_external_char_ptr(char *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_CHAR),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR),
                               index_t endianness = Endianness::DEFAULT_ID);

    #ifndef CONDUIT_USE_CHAR
        void set_external(signed char *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                          index_t endianness = Endianness::DEFAULT_ID);

        void set_external(unsigned char *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_external(short *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_SHORT),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT),
                          index_t endianness = Endianness::DEFAULT_ID);

        void set_external(unsigned short *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_external(int *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_INT),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_INT),
                          index_t endianness = Endianness::DEFAULT_ID);

        void set_external(unsigned int *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_external(long *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_LONG),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG),
                          index_t endianness = Endianness::DEFAULT_ID);

        void set_external(unsigned long *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_external(long long *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_LONG_LONG),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_LONG),
                          index_t endianness = Endianness::DEFAULT_ID);

        void set_external(unsigned long long *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_external(float *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_FLOAT),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_FLOAT),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_external(double *data,
                          index_t num_elements = 1,
                          index_t offset = 0,
                          index_t stride = sizeof(CONDUIT_NATIVE_DOUBLE),
                          index_t element_bytes = sizeof(CONDUIT_NATIVE_DOUBLE),
                          index_t endianness = Endianness::DEFAULT_ID);
    #endif


//-----------------------------------------------------------------------------
// -- set_external for conduit::DataArray types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // signed integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_external_int8_array(const int8_array &data);
    void set_external(const int8_array &data);

    //-------------------------------------------------------------------------
    void set_external_int16_array(const int16_array &data);
    void set_external(const int16_array &data);

    //-------------------------------------------------------------------------
    void set_external_int32_array(const int32_array &data);
    void set_external(const int32_array &data);

    //-------------------------------------------------------------------------
    void set_external_int64_array(const int64_array &data);
    void set_external(const int64_array &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_external_uint8_array(const uint8_array  &data);
    void set_external(const uint8_array  &data);

    //-------------------------------------------------------------------------
    void set_external_uint16_array(const uint16_array &data);
    void set_external(const uint16_array &data);

    //-------------------------------------------------------------------------
    void set_external_uint32_array(const uint32_array &data);
    void set_external(const uint32_array &data);

    //-------------------------------------------------------------------------
    void set_external_uint64_array(const uint64_array &data);
    void set_external(const uint64_array &data);

    //-------------------------------------------------------------------------
    // floating point array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_external_float32_array(const float32_array &data);
    void set_external(const float32_array &data);

    //-------------------------------------------------------------------------
    void set_external_float64_array(const float64_array &data);
    void set_external(const float64_array &data);

//-----------------------------------------------------------------------------
//  set_external array gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_external(const char_array &data);

    #ifndef CONDUIT_USE_CHAR
        void set_external(const signed_char_array &data);
        void set_external(const unsigned_char_array &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_external(const short_array &data);
        void set_external(const unsigned_unsigned_array &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_external(const int_array &data);
        void set_external(const unsigned_int_array &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_external(const long_array &data);
        void set_external(const unsigned_long_array &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_external(const long_long_array &data);
        void set_external(const unsigned_long_long_array &data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_external(const float_array &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_external(const double_array &data);
    #endif


//-----------------------------------------------------------------------------
// -- set_external for string types ---
//-----------------------------------------------------------------------------
    void set_external_char8_str(char *data);

//-----------------------------------------------------------------------------
// -- set_external for bitwidth style std::vector types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // signed integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_external_int8_vector(std::vector<int8> &data);
    void set_external(std::vector<int8> &data);

    //-------------------------------------------------------------------------
    void set_external_int16_vector(std::vector<int16> &data);
    void set_external(std::vector<int16> &data);

    //-------------------------------------------------------------------------
    void set_external_int32_vector(std::vector<int32> &data);
    void set_external(std::vector<int32> &data);

    //-------------------------------------------------------------------------
    void set_external_int64_vector(std::vector<int64> &data);
    void set_external(std::vector<int64> &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_external_uint8_vector(std::vector<uint8> &data);
    void set_external(std::vector<uint8> &data);

    //-------------------------------------------------------------------------
    void set_external_uint16_vector(std::vector<uint16> &data);
    void set_external(std::vector<uint16> &data);

    //-------------------------------------------------------------------------
    void set_external_uint32_vector(std::vector<uint32> &data);
    void set_external(std::vector<uint32> &data);

    //-------------------------------------------------------------------------
    void set_external_uint64_vector(std::vector<uint64> &data);
    void set_external(std::vector<uint64> &data);

    //-------------------------------------------------------------------------
    // floating point array types via std::vector
    //-------------------------------------------------------------------------
    void set_external_float32_vector(std::vector<float32> &data);
    void set_external(std::vector<float32> &data);

    //-------------------------------------------------------------------------
    void set_external_float64_vector(std::vector<float64> &data);
    void set_external(std::vector<float64> &data);

//-----------------------------------------------------------------------------
//  set_external vector gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_external(const std::vector<char> &data);

    #ifndef CONDUIT_USE_CHAR
        void set_external(const std::vector<signed char> &data);
        void set_external(const std::vector<unsigned char> &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_external(const std::vector<short> &data);
        void set_external(const std::vector<unsigned short> &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_external(const std::vector<int> &data);
        void set_external(const std::vector<unsigned int> &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_external(const std::vector<long> &data);
        void set_external(const std::vector<unsigned long> &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_external(const std::vector<long long> &data);
        void set_external(const std::vector<unsigned long long> &data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_external(const std::vector<float> &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_external(const std::vector<double> &data);
    #endif


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end  declaration of Node set_external methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node set_path_external methods --
//
//-----------------------------------------------------------------------------
///@name Node::set_path_external(...)
///@{
//-----------------------------------------------------------------------------
/// description:
///   set_path_external(...) methods allow the node to point to external
///   memory, and allow you to use an explicit path for the destination node.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- set path external for generic types --
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    void    set_path_external_node(const std::string &path,
                                   Node &node);

    void    set_path_external(const std::string &path,
                              Node &node);

    //-------------------------------------------------------------------------
    void    set_path_external_data_using_schema(const std::string &path,
                                                const Schema &schema,
                                                void *data);

    void    set_path_external(const std::string &path,
                              const Schema &schema,
                              void *data);

    //-------------------------------------------------------------------------
    void    set_path_external_data_using_dtype(const std::string &path,
                                               const DataType &dtype,
                                               void *data);

    void    set_path_external(const std::string &path,
                              const DataType &dtype,
                              void *data);

//-----------------------------------------------------------------------------
// -- set_path_external via bitwidth style pointers (scalar and array types) --
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // signed integer pointer cases
    //-------------------------------------------------------------------------
    void set_path_external_int8_ptr(const std::string &path,
                                    int8 *data,
                                    index_t num_elements = 1,
                                    index_t offset = 0,
                                    index_t stride = sizeof(conduit::int8),
                                    index_t element_bytes = sizeof(conduit::int8),
                                    index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           int8 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::int8),
                           index_t element_bytes = sizeof(conduit::int8),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_int16_ptr(const std::string &path,
                                     int16 *data,
                                     index_t num_elements = 1,
                                     index_t offset = 0,
                                     index_t stride = sizeof(conduit::int16),
                                     index_t element_bytes = sizeof(conduit::int16),
                                     index_t endianness = Endianness::DEFAULT_ID);


    void set_path_external(const std::string &path,
                           int16 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::int16),
                           index_t element_bytes = sizeof(conduit::int16),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_int32_ptr(const std::string &path,
                                     int32 *data,
                                     index_t num_elements = 1,
                                     index_t offset = 0,
                                     index_t stride = sizeof(conduit::int32),
                                     index_t element_bytes = sizeof(conduit::int32),
                                     index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           int32 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::int32),
                           index_t element_bytes = sizeof(conduit::int32),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_int64_ptr(const std::string &path,
                                     int64 *data,
                                     index_t num_elements = 1,
                                     index_t offset = 0,
                                     index_t stride = sizeof(conduit::int64),
                                     index_t element_bytes = sizeof(conduit::int64),
                                     index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           int64 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::int64),
                           index_t element_bytes = sizeof(conduit::int64),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // unsigned integer pointer cases
    //-------------------------------------------------------------------------
    void set_path_external_uint8_ptr(const std::string &path,
                                     uint8  *data,
                                     index_t num_elements = 1,
                                     index_t offset = 0,
                                     index_t stride = sizeof(conduit::uint8),
                                     index_t element_bytes = sizeof(conduit::uint8),
                                     index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           uint8  *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::uint8),
                           index_t element_bytes = sizeof(conduit::uint8),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_uint16_ptr(const std::string &path,
                                      uint16 *data,
                                      index_t num_elements = 1,
                                      index_t offset = 0,
                                      index_t stride = sizeof(conduit::uint16),
                                      index_t element_bytes = sizeof(conduit::uint16),
                                      index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           uint16 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::uint16),
                           index_t element_bytes = sizeof(conduit::uint16),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_uint32_ptr(const std::string &path,
                                      uint32 *data,
                                      index_t num_elements = 1,
                                      index_t offset = 0,
                                      index_t stride = sizeof(conduit::uint32),
                                      index_t element_bytes = sizeof(conduit::uint32),
                                      index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           uint32 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::uint32),
                           index_t element_bytes = sizeof(conduit::uint32),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_uint64_ptr(const std::string &path,
                                      uint64 *data,
                                      index_t num_elements = 1,
                                      index_t offset = 0,
                                      index_t stride = sizeof(conduit::uint64),
                                      index_t element_bytes = sizeof(conduit::uint64),
                                      index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           uint64 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::uint64),
                           index_t element_bytes = sizeof(conduit::uint64),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    // floating point pointer cases
    //-------------------------------------------------------------------------
    void set_path_external_float32_ptr(const std::string &path,
                                       float32 *data,
                                       index_t num_elements = 1,
                                       index_t offset = 0,
                                       index_t stride = sizeof(conduit::float32),
                                       index_t element_bytes = sizeof(conduit::float32),
                                       index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           float32 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::float32),
                           index_t element_bytes = sizeof(conduit::float32),
                           index_t endianness = Endianness::DEFAULT_ID);

    //-------------------------------------------------------------------------
    void set_path_external_float64_ptr(const std::string &path,
                                       float64 *data,
                                       index_t num_elements = 1,
                                       index_t offset = 0,
                                       index_t stride = sizeof(conduit::float64),
                                       index_t element_bytes = sizeof(conduit::float64),
                                       index_t endianness = Endianness::DEFAULT_ID);

    void set_path_external(const std::string &path,
                           float64 *data,
                           index_t num_elements = 1,
                           index_t offset = 0,
                           index_t stride = sizeof(conduit::float64),
                           index_t element_bytes = sizeof(conduit::float64),
                           index_t endianness = Endianness::DEFAULT_ID);

//-----------------------------------------------------------------------------
//  set via pointer gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path_external_char_ptr(const std::string &path,
                                    char *data,
                                    index_t num_elements = 1,
                                    index_t offset = 0,
                                    index_t stride = sizeof(CONDUIT_NATIVE_CHAR),
                                    index_t element_bytes = sizeof(CONDUIT_NATIVE_CHAR),
                                    index_t endianness = Endianness::DEFAULT_ID);

    #ifndef CONDUIT_USE_CHAR
        void set_path_external(const std::string &path,
                               signed char *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_SIGNED_CHAR),
                               index_t endianness = Endianness::DEFAULT_ID);

        void set_path_external(const std::string &path,
                               unsigned char *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_CHAR),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path_external(const std::string &path,
                               short *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_SHORT),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_SHORT),
                               index_t endianness = Endianness::DEFAULT_ID);

        void set_path_external(const std::string &path,
                               unsigned short *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_SHORT),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path_external(const std::string &path,
                               int *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_INT),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_INT),
                               index_t endianness = Endianness::DEFAULT_ID);

        void set_path_external(const std::string &path,
                               unsigned int *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_INT),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path_external(const std::string &path,
                               long *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_LONG),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG),
                               index_t endianness = Endianness::DEFAULT_ID);

        void set_path_external(const std::string &path,
                               unsigned long *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path_external(const std::string &path,
                               long long *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_LONG_LONG),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_LONG_LONG),
                               index_t endianness = Endianness::DEFAULT_ID);

        void set_path_external(const std::string &path,
                               unsigned long long *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_UNSIGNED_LONG_LONG),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path_external(const std::string &path,
                               float *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_FLOAT),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_FLOAT),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path_external(const std::string &path,
                               double *data,
                               index_t num_elements = 1,
                               index_t offset = 0,
                               index_t stride = sizeof(CONDUIT_NATIVE_DOUBLE),
                               index_t element_bytes = sizeof(CONDUIT_NATIVE_DOUBLE),
                               index_t endianness = Endianness::DEFAULT_ID);
    #endif


//-----------------------------------------------------------------------------
// -- set_path_external for conduit::DataArray types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // signed integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_external_int8_array(const std::string &path,
                                      const int8_array &data);

    void set_path_external(const std::string &path,
                           const int8_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_int16_array(const std::string &path,
                                       const int16_array &data);

    void set_path_external(const std::string &path,
                           const int16_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_int32_array(const std::string &path,
                                       const int32_array &data);

    void set_path_external(const std::string &path,
                           const int32_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_int64_array(const std::string &path,
                                       const int64_array &data);

    void set_path_external(const std::string &path,
                           const int64_array &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_external_uint8_array(const std::string &path,
                                       const uint8_array &data);

    void set_path_external(const std::string &path,
                           const uint8_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint16_array(const std::string &path,
                                        const uint16_array &data);

    void set_path_external(const std::string &path,
                           const uint16_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint32_array(const std::string &path,
                                        const uint32_array &data);

    void set_path_external(const std::string &path,
                           const uint32_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint64_array(const std::string &path,
                                        const uint64_array &data);

    void set_path_external(const std::string &path,
                           const uint64_array &data);

    //-------------------------------------------------------------------------
    // floating point array types via conduit::DataArray
    //-------------------------------------------------------------------------
    void set_path_external_float32_array(const std::string &path,
                                         const float32_array &data);

    void set_path_external(const std::string &path,
                           const float32_array &data);

    //-------------------------------------------------------------------------
    void set_path_external_float64_array(const std::string &path,
                                         const float64_array &data);

    void set_path_external(const std::string &path,
                          const float64_array &data);

//-----------------------------------------------------------------------------
//  set_path_external array gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path_external(const std::string &path,
                           const char_array &data);

    #ifndef CONDUIT_USE_CHAR
        void set_path_external(const std::string &path,
                               const signed_char_array &data);

        void set_path_external(const std::string &path,
                              const unsigned_char_array &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path_external(const std::string &path,
                               const short_array &data);

        void set_path_external(const std::string &path,
                               const unsigned_short_array &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path_external(const std::string &path,
                               const int_array &data);

        void set_path_external(const std::string &path,
                               const unsigned_int_array &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path_external(const std::string &path,
                               const long_array &data);

        void set_path_external(const std::string &path,
                               const unsigned_long_array &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path_external(const std::string &path,
                               const long_long_array &data);

        void set_path_external(const std::string &path,
                               const unsigned_long_long_array &data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path_external(const std::string &path,
                               const float_array &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path_external(const std::string &path,
                               const double_array &data);
    #endif



//-----------------------------------------------------------------------------
// -- set_external for string types ---
//-----------------------------------------------------------------------------
    void set_path_external_char8_str(const std::string &path, char *data);

//-----------------------------------------------------------------------------
// -- set_path_external for bitwidth style std::vector types ---
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // signed integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_external_int8_vector(const std::string &path,
                                       std::vector<int8> &data);

    void set_path_external(const std::string &path,
                           std::vector<int8> &data);

    //-------------------------------------------------------------------------
    void set_path_external_int16_vector(const std::string &path,
                                        std::vector<int16> &data);

    void set_path_external(const std::string &path,
                           std::vector<int16> &data);

    //-------------------------------------------------------------------------
    void set_path_external_int32_vector(const std::string &path,
                                        std::vector<int32> &data);

    void set_path_external(const std::string &path,
                           std::vector<int32> &data);

    //-------------------------------------------------------------------------
    void set_path_external_int64_vector(const std::string &path,
                                        std::vector<int64> &data);

    void set_path_external(const std::string &path,
                           std::vector<int64> &data);

    //-------------------------------------------------------------------------
    // unsigned integer array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_external_uint8_vector(const std::string &path,
                                        std::vector<uint8> &data);

    void set_path_external(const std::string &path,
                           std::vector<uint8> &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint16_vector(const std::string &path,
                                         std::vector<uint16> &data);

    void set_path_external(const std::string &path,
                           std::vector<uint16> &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint32_vector(const std::string &path,
                                         std::vector<uint32> &data);

    void set_path_external(const std::string &path,
                           std::vector<uint32> &data);

    //-------------------------------------------------------------------------
    void set_path_external_uint64_vector(const std::string &path,
                                         std::vector<uint64> &data);

    void set_path_external(const std::string &path,
                           std::vector<uint64> &data);

    //-------------------------------------------------------------------------
    // floating point array types via std::vector
    //-------------------------------------------------------------------------
    void set_path_external_float32_vector(const std::string &path,
                                          std::vector<float32> &data);

    void set_path_external(const std::string &path,
                           std::vector<float32> &data);

    //-------------------------------------------------------------------------
    void set_path_external_float64_vector(const std::string &path,
                                          std::vector<float64> &data);

    void set_path_external(const std::string &path,
                           std::vector<float64> &data);

//-----------------------------------------------------------------------------
//  set_path_external vector gap methods for c-native types
//-----------------------------------------------------------------------------
    void set_path_external(const std::string &path,
                           const std::vector<char> &data);

    #ifndef CONDUIT_USE_CHAR
        void set_path_external(const std::string &path,
                               const std::vector<signed char> &data);

        void set_path_external(const std::string &path,
                               const std::vector<unsigned char> &data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        void set_path_external(const std::string &path,
                               const std::vector<short> &data);

        void set_path_external(const std::string &path,
                               const std::vector<unsigned short> &data);
    #endif

    #ifndef CONDUIT_USE_INT
        void set_path_external(const std::string &path,
                               const std::vector<int> &data);

        void set_path_external(const std::string &path,
                               const std::vector<unsigned int> &data);
    #endif

    #ifndef CONDUIT_USE_LONG
        void set_path_external(const std::string &path,
                               const std::vector<long> &data);

        void set_path_external(const std::string &path,
                               const std::vector<unsigned long> &data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        void set_path_external(const std::string &path,
                               const std::vector<long long> &data);

        void set_path_external(const std::string &path,
                               const std::vector<unsigned long long> &data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        void set_path_external(const std::string &path,
                               const std::vector<float> &data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        void set_path_external(const std::string &path,
                               const std::vector<double> &data);
    #endif


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node set_path_external methods --
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// -- begin declaration of Node assignment operators --
//
//-----------------------------------------------------------------------------
///@name Node Assignment Operators
///@{
//-----------------------------------------------------------------------------
/// description:
/// &operator=(...) methods use set(...) (copy) semantics
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- assignment operators for generic types --
//-----------------------------------------------------------------------------
    Node &operator=(const Node &node);
    Node &operator=(const DataType &dtype);
    Node &operator=(const Schema &schema);

//-----------------------------------------------------------------------------
// --  assignment operators for scalar types ---
//-----------------------------------------------------------------------------
     // signed integer scalar types
    Node &operator=(int8 data);
    Node &operator=(int16 data);
    Node &operator=(int32 data);
    Node &operator=(int64 data);

     // unsigned integer scalar types
    Node &operator=(uint8 data);
    Node &operator=(uint16 data);
    Node &operator=(uint32 data);
    Node &operator=(uint64 data);

    // floating point scalar types
    Node &operator=(float32 data);
    Node &operator=(float64 data);


//-----------------------------------------------------------------------------
// --  assignment c-native gap operators for scalar types ---
//-----------------------------------------------------------------------------

    Node &operator=(char data);

    #ifndef CONDUIT_USE_CHAR
        Node &operator=(signed char data);
        Node &operator=(unsigned char data);
    #endif

    #ifndef CONDUIT_USE_SHORT
        Node &operator=(short data);
        Node &operator=(unsigned short data);
    #endif

    #ifndef CONDUIT_USE_INT
        Node &operator=(int data);
        Node &operator=(unsigned int data);
    #endif

    #ifndef CONDUIT_USE_LONG
        Node &operator=(long data);
        Node &operator=(unsigned long data);
    #endif

    #if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
        Node &operator=(long long data);
        Node &operator=(unsigned long long data);
    #endif

    #ifndef CONDUIT_USE_FLOAT
        Node &operator=(float data);
    #endif

    #ifndef CONDUIT_USE_DOUBLE
        Node &operator=(double data);
    #endif
//-----------------------------------------------------------------------------
// -- assignment operators for conduit::DataArray types ---
//-----------------------------------------------------------------------------
    // signed integer array types via conduit::DataArray
    Node &operator=(const int8_array  &data);
    Node &operator=(const int16_array &data);
    Node &operator=(const int32_array &data);
    Node &operator=(const int64_array &data);

    // unsigned integer array ttypes via conduit::DataArray
    Node &operator=(const uint8_array  &data);
    Node &operator=(const uint16_array &data);
    Node &operator=(const uint32_array &data);
    Node &operator=(const uint64_array &data);

    // floating point array types via conduit::DataArray
    Node &operator=(const float32_array &data);
    Node &operator=(const float64_array &data);

//-----------------------------------------------------------------------------
// --  assignment c-native gap operators for data array  types ---
//-----------------------------------------------------------------------------
    Node &operator=(const char_array &data);

#ifndef CONDUIT_USE_CHAR
    Node &operator=(const signed_char_array &data);
    Node &operator=(const unsigned_char_array &data);
#endif

#ifndef CONDUIT_USE_SHORT
    Node &operator=(const short_array &data);
    Node &operator=(const unsigned_short_array &data);
#endif

#ifndef CONDUIT_USE_INT
    Node &operator=(const int_array &data);
    Node &operator=(const unsigned_int_array &data);
#endif

#ifndef CONDUIT_USE_LONG
    Node &operator=(const long_array &data);
    Node &operator=(const unsigned_long_array &data);
#endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
    Node &operator=(const long_long_array &data);
    Node &operator=(const unsigned_long_long_array &data);
#endif

#ifndef CONDUIT_USE_FLOAT
    Node &operator=(const float_array &data);
#endif

#ifndef CONDUIT_USE_DOUBLE
    Node &operator=(const double_array &data);
#endif


//-----------------------------------------------------------------------------
// -- assignment operators for std::vector types ---
//-----------------------------------------------------------------------------

    // signed integer array types via std::vector
    Node &operator=(const std::vector<int8>   &data);
    Node &operator=(const std::vector<int16>  &data);
    Node &operator=(const std::vector<int32>  &data);
    Node &operator=(const std::vector<int64>  &data);

    // unsigned integer array types via std::vector
    Node &operator=(const std::vector<uint8>   &data);
    Node &operator=(const std::vector<uint16>  &data);
    Node &operator=(const std::vector<uint32>  &data);
    Node &operator=(const std::vector<uint64>  &data);

    // floating point array types via std::vector
    Node &operator=(const std::vector<float32> &data);
    Node &operator=(const std::vector<float64> &data);


//-----------------------------------------------------------------------------
// --  assignment c-native gap operators for vector types ---
//-----------------------------------------------------------------------------
    Node &operator=(const std::vector<char> &data);

#ifndef CONDUIT_USE_CHAR
    Node &operator=(const std::vector<signed char> &data);
    Node &operator=(const std::vector<unsigned char> &data);
#endif

#ifndef CONDUIT_USE_SHORT
    Node &operator=(const std::vector<short> &data);
    Node &operator=(const std::vector<unsigned short> &data);
#endif

#ifndef CONDUIT_USE_INT
    Node &operator=(const std::vector<int> &data);
    Node &operator=(const std::vector<unsigned int> &data);
#endif

#ifndef CONDUIT_USE_LONG
    Node &operator=(const std::vector<long> &data);
    Node &operator=(const std::vector<unsigned long> &data);
#endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
    Node &operator=(const std::vector<long long> &data);
    Node &operator=(const std::vector<unsigned long long> &data);
#endif

#ifndef CONDUIT_USE_FLOAT
    Node &operator=(const std::vector<float> &data);
#endif

#ifndef CONDUIT_USE_DOUBLE
    Node &operator=(const std::vector<double> &data);
#endif

//-----------------------------------------------------------------------------
// -- std::initializer_list support --
//-----------------------------------------------------------------------------
//
// Support std::initializer_lists
//
// Example:
//   Node n;
//   n = {1,2,3,4,5,6};
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- assignment operators for std::initializer_list types ---
//-----------------------------------------------------------------------------
    // signed integer array types via std::initializer_list
    Node &operator=(const std::initializer_list<int8>   &data);
    Node &operator=(const std::initializer_list<int16>  &data);
    Node &operator=(const std::initializer_list<int32>  &data);
    Node &operator=(const std::initializer_list<int64>  &data);

    // unsigned integer array types via std::initialize_list
    Node &operator=(const std::initializer_list<uint8>   &data);
    Node &operator=(const std::initializer_list<uint16>  &data);
    Node &operator=(const std::initializer_list<uint32>  &data);
    Node &operator=(const std::initializer_list<uint64>  &data);

    // floating point array types via std::initializer_list
    Node &operator=(const std::initializer_list<float32> &data);
    Node &operator=(const std::initializer_list<float64> &data);

//-----------------------------------------------------------------------------
// --  assignment c-native gap operators for initializer_list types ---
//-----------------------------------------------------------------------------

    Node &operator=(const std::initializer_list<char> &data);

#ifndef CONDUIT_USE_CHAR
    Node &operator=(const std::initializer_list<signed char> &data);
    Node &operator=(const std::initializer_list<unsigned char> &data);
#endif

#ifndef CONDUIT_USE_SHORT
    Node &operator=(const std::initializer_list<short> &data);
    Node &operator=(const std::initializer_list<unsigned short> &data);
#endif

#ifndef CONDUIT_USE_INT
    Node &operator=(const std::initializer_list<int> &data);
    Node &operator=(const std::initializer_list<unsigned int> &data);
#endif

#ifndef CONDUIT_USE_LONG
    Node &operator=(const std::initializer_list<long> &data);
    Node &operator=(const std::initializer_list<unsigned long> &data);
#endif

#if defined(CONDUIT_HAS_LONG_LONG) && !defined(CONDUIT_USE_LONG_LONG)
    Node &operator=(const std::initializer_list<long long> &data);
    Node &operator=(const std::initializer_list<unsigned long long> &data);
#endif

#ifndef CONDUIT_USE_FLOAT
    Node &operator=(const std::initializer_list<float> &data);
#endif

#ifndef CONDUIT_USE_DOUBLE
    Node &operator=(const std::initializer_list<double> &data);
#endif

//-----------------------------------------------------------------------------
// -- assignment operators for string types --
//-----------------------------------------------------------------------------
    // char8_str use cases
    Node &operator=(const char *data);
    Node &operator=(const std::string &data);

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node assignment operators --
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// -- begin declaration of Node transforms --
//
//-----------------------------------------------------------------------------
///@name Node Transforms
///@{
//-----------------------------------------------------------------------------
/// description:
///  TODO
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- serialization methods ---
//-----------------------------------------------------------------------------
    /// serialize to a byte vector
    void        serialize(std::vector<uint8> &data) const;
    /// serialize to a file identified by stream_path
    void        serialize(const std::string &stream_path) const;
    /// serialize to an output stream
    void        serialize(std::ofstream &ofs) const;

//-----------------------------------------------------------------------------
// -- compaction methods ---
//-----------------------------------------------------------------------------
    /// compact into a new node
    void        compact_to(Node &n_dest) const;

//-----------------------------------------------------------------------------
// -- update methods ---
//-----------------------------------------------------------------------------
    /// update() adds children from n_src to current Node (analogous to a
    /// python dictionary update)
    ///
    void        update(const Node &n_src);

    /// update_compatible() copies data from the children in n_src that match
    ///  the current Nodes children.
    void        update_compatible(const Node &n_src);

    /// update_external() sets this node to describe the data from the children
    //   in n_src.
    void        update_external(Node &n_src);
    
//-----------------------------------------------------------------------------
// -- move and swap ---
//-----------------------------------------------------------------------------
    /// move() assume ownership of data and tree structure of the passed
    /// Node, passed node is empty afterward
    void        move(Node &n);

    /// swap() swap the ownership ownership of data and tree structure 
    /// between two nodes.
    void        swap(Node &n);
    
//-----------------------------------------------------------------------------
// -- endian related --
//-----------------------------------------------------------------------------
    void endian_swap(index_t endianness);

    void endian_swap_to_machine_default()
        {endian_swap(Endianness::DEFAULT_ID);}

    void endian_swap_to_little()
        {endian_swap(Endianness::LITTLE_ID);}

    void endian_swap_to_big()
        {endian_swap(Endianness::BIG_ID);}


//-----------------------------------------------------------------------------
// -- leaf coercion methods ---
//-----------------------------------------------------------------------------
    ///
    /// These methods allow you to coerce a leaf type to another type.
    ///

    /// scalar coercion

    /// convert to a signed integer types
    int8             to_int8()   const;
    int16            to_int16()  const;
    int32            to_int32()  const;
    int64            to_int64()  const;

    /// convert to a unsigned integer types
    uint8            to_uint8()   const;
    uint16           to_uint16()  const;
    uint32           to_uint32()  const;
    uint64           to_uint64()  const;

    /// convert to a floating point type
    float32          to_float32() const;
    float64          to_float64() const;

    /// convert to the index type
    index_t          to_index_t() const;

    /// convert to c integer types
    char             to_char() const;
    short            to_short() const;
    int              to_int()   const;
    long             to_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    long long        to_long_long()  const;
#endif

    /// convert to c signed integer types
    signed char      to_signed_char() const;
    signed short     to_signed_short() const;
    signed int       to_signed_int()   const;
    signed long      to_signed_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    long long        to_signed_long_long()  const;
#endif

    /// convert to c unsigned integer types
    unsigned char    to_unsigned_char()  const;
    unsigned short   to_unsigned_short() const;
    unsigned int     to_unsigned_int()   const;
    unsigned long    to_unsigned_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned long long to_unsigned_long_long()  const;
#endif

    /// convert to c floating point types
    float            to_float() const;
    double           to_double() const;

#ifdef CONDUIT_USE_LONG_DOUBLE
    long double      to_long_double() const;
#endif


//-----------------------------------------------------------------------------
// -- array conversion methods --
//
/// These methods convert an array to a specific array type.
/// The result is stored in the passed node.
//-----------------------------------------------------------------------------

    /// convert to a signed integer types
    void    to_int8_array(Node &res)  const;
    void    to_int16_array(Node &res) const;
    void    to_int32_array(Node &res) const;
    void    to_int64_array(Node &res) const;

    /// convert to a unsigned integer types
    void    to_uint8_array(Node &res)  const;
    void    to_uint16_array(Node &res) const;
    void    to_uint32_array(Node &res) const;
    void    to_uint64_array(Node &res) const;

    /// convert to a floating point type
    void    to_float32_array(Node &res) const;
    void    to_float64_array(Node &res) const;

    /// convert to the index type
    void    to_index_t_array(Node &res) const;

    /// convert to c types
    void    to_char_array(Node &res)  const;
    void    to_short_array(Node &res) const;
    void    to_int_array(Node &res)   const;
    void    to_long_array(Node &res)  const;

    /// convert to c signed integer types
    void    to_signed_char_array(Node &res) const;
    void    to_signed_short_array(Node &res) const;
    void    to_signed_int_array(Node &res)   const;
    void    to_signed_long_array(Node &res)  const;

#ifdef CONDUIT_HAS_LONG_LONG
    void    to_signed_long_long_array(Node &res)  const;
#endif

    /// convert to c unsigned integer types
    void    to_unsigned_char_array(Node &res)  const;
    void    to_unsigned_short_array(Node &res) const;
    void    to_unsigned_int_array(Node &res)   const;
    void    to_unsigned_long_array(Node &res)  const;

#ifdef CONDUIT_HAS_LONG_LONG
    void    to_unsigned_long_long_array(Node &res)  const;
#endif

    /// convert to c floating point types
    void    to_float_array(Node &res) const;
    void    to_double_array(Node &res) const;

//-----------------------------------------------------------------------------
// -- dynamic conversion methods --
//
/// These methods convert any data to a given type.
/// The result is stored in the passed node.
//-----------------------------------------------------------------------------

    void    to_data_type(index_t dtype_id, Node &res) const;

//-----------------------------------------------------------------------------
// -- Node::Value Helper class --
//
// This class allows us to support casting return semantics.
// we can't support these methods directly in conduit::Node because doing so
// undermines our operator=() overloads.
//-----------------------------------------------------------------------------
    class CONDUIT_API Value
    {
        friend class Node;
        public:
            ~Value();
            Value(const Value &rhs);

            operator char()  const;

            // cast operators for signed integers
            operator signed char()  const;
            operator signed short() const;
            operator signed int()   const;
            operator signed long()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator signed long long() const;
            #endif


            // cast operators for unsigned integers
            operator unsigned char()   const;
            operator unsigned short()  const;
            operator unsigned int()    const;
            operator unsigned long()   const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator unsigned long long() const;
            #endif

            // cast operators for floating point types
            operator float()  const;
            operator double() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long double() const;
            #endif

            // -- as pointer -- //
            // char is special
            // we need a char operator to support char8_str case
            operator char*()  const;

            // as signed int ptr
            operator signed char*()  const;
            operator signed short*() const;
            operator signed int*()   const;
            operator signed long*()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator signed long long *() const;
            #endif

            // as unsigned int ptr
            operator unsigned char*()  const;
            operator unsigned short*() const;
            operator unsigned int*()   const;
            operator unsigned long*()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator unsigned long long *() const;
            #endif


            // as floating point ptr
            operator float*()  const;
            operator double*() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long double *() const;
            #endif


            // -- as array -- //
            operator char_array()  const;

            // as signed array
            operator signed_char_array()  const;
            operator signed_short_array() const;
            operator signed_int_array()   const;
            operator signed_long_array()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator signed_long_long_array() const;
            #endif

            // as unsigned array
            operator unsigned_char_array()  const;
            operator unsigned_short_array() const;
            operator unsigned_int_array()   const;
            operator unsigned_long_array()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator unsigned_long_long_array() const;
            #endif

            // as floating point array
            operator float_array()  const;
            operator double_array() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long_double_array() const;
            #endif

            /// native c types accessors
            operator char_accessor() const;

            /// signed integer accessors
            operator signed_char_accessor()  const;
            operator signed_short_accessor() const;
            operator signed_int_accessor()   const;
            operator signed_long_accessor()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator  signed_long_long_accessor() const;
            #endif

            /// unsigned integer accessors
            operator unsigned_char_accessor()  const;
            operator unsigned_short_accessor() const;
            operator unsigned_int_accessor()   const;
            operator unsigned_long_accessor()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator  unsigned_long_long_accessor() const;
            #endif

            /// floating point accessors
            operator float_accessor() const;
            operator double_accessor() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long_double_accessor() const;
            #endif


        private:
            // This is private we only want conduit::Node to create a
            // conduit::Node::Value instance
            Value(Node *node, bool coerse);
            // holds the node with the actually data
            Node    *m_node;
            // coercion flag, note - only scalars types can be coerced
            bool     m_coerse;
    };

//-----------------------------------------------------------------------------
// -- Node::ConstValue Helper class --
//
// This class allows us to support casting return semantics.
// we can't support these methods directly in conduit::Node  because doing so
// undermines our operator=() overloads.
//-----------------------------------------------------------------------------
    class CONDUIT_API ConstValue
    {
        friend class Node;
        public:
            ~ConstValue();
            ConstValue(const Value &rhs);
            ConstValue(const ConstValue &rhs);

            operator char()  const;

            // cast operators for signed integers
            operator signed char()  const;
            operator signed short() const;
            operator signed int()   const;
            operator signed long()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator signed long long() const;
            #endif

            // cast operators for unsigned integers
            operator unsigned char()   const;
            operator unsigned short()  const;
            operator unsigned int()    const;
            operator unsigned long()   const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator unsigned long long() const;
            #endif

            // cast operators for floating point types
            operator float()  const;
            operator double() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long double() const;
            #endif

            // -- as pointer -- //
            // char is special
            // we need a char operator to support char8_str case
            operator const char*()  const;


            // as signed int ptr
            operator const signed char*()  const;
            operator const signed short*() const;
            operator const signed int*()   const;
            operator const signed long*()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator const signed long long *() const;
            #endif

            // as unsigned int ptr
            operator const unsigned char*()  const;
            operator const unsigned short*() const;
            operator const unsigned int*()   const;
            operator const unsigned long*()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator const unsigned long long *() const;
            #endif

            // as floating point ptr
            operator const float*()  const;
            operator const double*() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long double *() const;
            #endif


            // -- as array -- //
            operator const char_array() const;

            // as signed array
            operator const signed_char_array() const;
            operator const signed_short_array() const;
            operator const signed_int_array()   const;
            operator const signed_long_array()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator const signed_long_long_array() const;
            #endif

            // as unsigned array
            operator const unsigned_char_array()  const;
            operator const unsigned_short_array() const;
            operator const unsigned_int_array()   const;
            operator const unsigned_long_array()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator const unsigned_long_long_array() const;
            #endif

            // as floating point array
            operator const float_array()  const;
            operator const double_array() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator const long_double_array() const;
            #endif

            // -- as accessor -- //
            operator char_accessor() const;

            /// signed integer arrays
            operator signed_char_accessor()  const;
            operator signed_short_accessor() const;
            operator signed_int_accessor()   const;
            operator signed_long_accessor()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator  signed_long_long_accessor() const;
            #endif

            /// unsigned integer arrays
            operator unsigned_char_accessor()  const;
            operator unsigned_short_accessor() const;
            operator unsigned_int_accessor()   const;
            operator unsigned_long_accessor()  const;
            #ifdef CONDUIT_HAS_LONG_LONG
                operator  unsigned_long_long_accessor() const;
            #endif

            /// floating point arrays
            operator float_accessor() const;
            operator double_accessor() const;
            #ifdef CONDUIT_USE_LONG_DOUBLE
                operator long_double_accessor() const;
            #endif


        private:
            // This is private we only want conduit::Node to create a
            // conduit::Node::ConstValue instance
            ConstValue(const Node *node, bool coerse);
            // holds the node with the actually data
            const Node *m_node;
            // coercion flag, note - only scalars types can be coerced
            bool        m_coerse;
    };

//-----------------------------------------------------------------------------
// -- Node methods that use the Node::Value class as a casting vehicle.
//-----------------------------------------------------------------------------

    Value value()  // works for all leaf types, but no coercion
        { return  Value(this,false); }

    Value to_value() // only works for scalar leaf types
        { return  Value(this,true); }

    ConstValue value() const // works for all leaf types, but no coercion
        { return  ConstValue(this,false); }

    ConstValue to_value() const // only works for scalar leaf types
        { return  ConstValue(this,true); }


//-----------------------------------------------------------------------------
// -- String construction methods ---
//-----------------------------------------------------------------------------
    /// Creates a string representation of a node.
    /// accepted protocols:
    ///   "json"
    ///   "conduit_json"
    ///   "conduit_base64_json"
    ///   "yaml"
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_string(const std::string &protocol="yaml",
                                  index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    /// accept formatting args via conduit node
    std::string         to_string(const conduit::Node &opts) const;

    void                to_string_stream(std::ostream &os,
                                         const conduit::Node &opts) const;

    void                to_string_stream(const std::string &stream_path,
                                         const conduit::Node &opts) const;

    void                to_string_stream(std::ostream &os,
                                         const std::string &protocol="yaml",
                                         index_t indent=2,
                                         index_t depth=0,
                                         const std::string &pad=" ",
                                         const std::string &eoe="\n") const;

    void                to_string_stream(const std::string &stream_path,
                                         const std::string &protocol="yaml",
                                         index_t indent=2,
                                         index_t depth=0,
                                         const std::string &pad=" ",
                                         const std::string &eoe="\n") const;

    // NOTE(cyrush): The primary reason this function exists is to enable easier
    // compatibility with debugging tools (e.g. totalview, gdb) that have
    // difficulty allocating default string parameters.
    std::string         to_string_default() const;

//-----------------------------------------------------------------------------
// -- Summary string construction methods ---
//-----------------------------------------------------------------------------
    /// Creates a summary string representation of a node.
    ///
    /// `opts` Node is used to pass formatting params.
    ///
    /// Supported `opts` entries: 
    ///  
    ///  num_children_threshold: max number of children to print for a list or object
    ///    (default: 7)
    ///  num_elements_threshold: max number of elements to print for a leaf
    ///    (default: 5)
    ///  indent: indention amount (default: 2 instances of pad)
    ///  depth: indention level (default: 0)
    ///  pad:  padding string (default: " ")
    //   eoe:  end of element string (default: "\n")
    ///
    /// formatting details:
    ///   these methods prefix entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.

    std::string         to_summary_string() const;
    std::string         to_summary_string(const conduit::Node &opts) const;
    void                to_summary_string_stream(std::ostream &os) const;
    void                to_summary_string_stream(std::ostream &os,
                                                 const conduit::Node &opts) const;
    void                to_summary_string_stream(const std::string &stream_path,
                                                 const conduit::Node &opts) const;

    // NOTE(cyrush): In this case to_summary_string() is easily callable
    // in debugging tools, but still provide the to_summary_string_default()
    // variant to present a similar interface to other `to_` methods.
    std::string         to_summary_string_default() const;


//-----------------------------------------------------------------------------
// -- JSON construction methods ---
//-----------------------------------------------------------------------------
    /// Creates a JSON string representation of a node.
    /// accepted protocols:
    ///  "json"
    ///  "conduit_json"
    ///  "conduit_base64_json"
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_json(const std::string &protocol="json",
                                index_t indent=2,
                                index_t depth=0,
                                const std::string &pad=" ",
                                const std::string &eoe="\n") const;

    /// accept formatting args via conduit node
    std::string         to_json(const conduit::Node &opts) const;

    void                to_json_stream(std::ostream &os,
                                       const conduit::Node &opts) const;

    void                to_json_stream(const std::string &stream_path,
                                       const conduit::Node &opts) const;


    void                to_json_stream(std::ostream &os,
                                       const std::string &protocol="json",
                                       index_t indent=2,
                                       index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    void                to_json_stream(const std::string &stream_path,
                                       const std::string &protocol="json",
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
    /// Creates a YAML string representation of a node.
    /// accepted protocols:
    ///  "yaml"
    ///
    /// formatting details:
    ///   this method prefixes entries with indent strings created using
    ///      utils::indent(...,indent, depth, pad)
    ///   adds the `eoe` (end-of-entry) suffix where necessary.
    ///
    std::string         to_yaml(const std::string &protocol="yaml",
                                index_t indent=2,
                                index_t depth=0,
                                const std::string &pad=" ",
                                const std::string &eoe="\n") const;

    /// accept formatting args via conduit node
    std::string         to_yaml(const conduit::Node &opts) const;

    void                to_yaml_stream(std::ostream &os,
                                       const conduit::Node &opts) const;

    void                to_yaml_stream(const std::string &stream_path,
                                       const conduit::Node &opts) const;


    void                to_yaml_stream(std::ostream &os,
                                       const std::string &protocol="yaml",
                                       index_t indent=2,
                                       index_t depth=0,
                                       const std::string &pad=" ",
                                       const std::string &eoe="\n") const;

    void                to_yaml_stream(const std::string &stream_path,
                                       const std::string &protocol="yaml",
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
// -- end declaration of Node transforms --
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// -- begin declaration of Node information methods --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
///@name Node Information
///@{
//-----------------------------------------------------------------------------
/// description:
///  These methods provide general info about the node hierarchy, and memory
///  layout.
//-----------------------------------------------------------------------------
    // schema access
    const Schema     &schema() const
                        { return *m_schema;}

    const DataType   &dtype() const
                        { return m_schema->dtype();}

    // TODO we'd like to have this function be const
    // just like the reference version of it is.
    // Then we also would have a private version of this
    // that is not const. But that cause causes
    // problems for us.
    // https://github.com/LLNL/conduit/issues/1320
    Schema          *schema_ptr()
                        {return m_schema;}

    // check if data owned by this node is externally
    // allocated.
    bool             is_data_external() const
                        {return !m_alloced;}

    // check if this node is the root of a tree nodes.
    bool             is_root() const
                        {return m_parent == NULL;}

    // parent access
    Node            *parent()
                        {return m_parent;}

    const Node      *parent() const
                        {return m_parent;}

    //memory space info

    /// stride() * (num_elements()-1) + element_bytes() summed over all
    /// leaves
    index_t          total_strided_bytes() const
                        { return m_schema->total_strided_bytes();}

    /// num_elements() * element_bytes() summed over all leaves
    index_t          total_bytes_compact() const
                        { return m_schema->total_bytes_compact();}


    /// total number of bytes allocated in this node hierarchy
    index_t           total_bytes_allocated() const;

    /// total number of bytes memory mapped in this node hierarchy
    index_t           total_bytes_mmaped() const;

    /// Is this node using a compact data layout?
    bool              is_compact() const
                         {return m_schema->is_compact();}

    //-------------------------------------------------------------------------
    /// contiguous checks
    //-------------------------------------------------------------------------
    /// A node is contiguous if the leaves of it children (traversed in a depth
    /// first order) cover a contiguous chunk of the address space.
    ///
    /// The direct address checks are only done for leaves with data,
    /// nodes in the objects, lists, or empty roles don't directly
    /// advance the pointer.
    ///
    /// Checks use each leaf's offset and the total strided bytes
    /// If leaves do not abut in address space, or if any leaf points to NULL
    /// the Node is not contiguous.
    ///
    /// This check is agnostic to if the Node owns the data.

    /// Does this node has a contiguous data layout?
    bool             is_contiguous() const;


    /// true if node hierarchy's memory contiguously follows
    /// the given node's memory
    bool             contiguous_with(const Node &n) const;

    /// true if node hierarchy's memory contiguously follows
    /// the given address. Note: contiguous with NULL is false.
    bool             contiguous_with(void *address) const;


    /// if this node has a contiguous data layout, returns
    /// the start address of its memory, otherwise returns NULL

    void            *contiguous_data_ptr();
    const void      *contiguous_data_ptr() const;

    /// is this node compatible with given node
    bool             compatible(const Node &n) const
                        {return m_schema->compatible(n.schema());}

    /// check for differences between this node and the given node, storing
    //  the results digest in the provided data node
    bool             diff(const Node &n,
                          Node &info,
                          const float64 epsilon = CONDUIT_EPSILON,
                          bool relaxint = false) const;

    /// diff this node to the given node for compatibility (i.e. validate it
    //  has everything that the instance node has), storing the results
    //  digest in the provided data node
    bool             diff_compatible(const Node &n,
                                     Node &info,
                                     const float64 epsilon = CONDUIT_EPSILON,
                                     bool relaxint = false) const;

    ///
    /// info() creates a node that contains metadata about the current
    /// node's memory properties
    void             info(Node &nres) const;

    /// TODO: this is inefficient w/o move semantics, but is very
    /// convenient for testing and example programs.
    Node             info() const;

    ///
    /// describe() creates a node that replaces each leaf with
    ///  descriptive statistics (count, mean, min, max) and a string repd 
    ///  values summary
    void             describe(Node &nres) const;
    void             describe(const Node &opts, Node &nres) const;

//-----------------------------------------------------------------------------
// -- stdout print methods ---
//-----------------------------------------------------------------------------
    /// print a simplified json representation of the this node to std out
    void            print() const;

    /// print a detailed json representation of the this node to std out.
    /// json output includes conduit schema constructs
    void            print_detailed() const;

//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node information methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node entry access methods --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
///@name Node Entry Access Methods
///@{
//-----------------------------------------------------------------------------
/// description:
///  Node traversal (iterators), child access (for list or object types)
//-----------------------------------------------------------------------------
    /// return a iterator that give access to this nodes children
    NodeIterator        children();
    NodeConstIterator   children() const;

    // When fetching, there is no absolute path construct, all paths are
    /// fetched relative to the current node (a leading "/" is ignored when
    /// fetching). Empty path names are also ignored, fetching "a///b" is
    /// equalvalent to fetching "a/b".

    /// fetch the node at the given path
    /// non-const `fetch' methods do modify map structure if a path
    /// does not exist
    Node             &fetch(const std::string &path);
    const Node       &fetch(const std::string &path) const;

    /// the `fetch_existing' methods don't modify map structure, if a path
    /// doesn't exist they will throw an exception
    Node             &fetch_existing(const std::string &path);
    const Node       &fetch_existing(const std::string &path) const;

    // add_child will not try to parse the name as a path. "foo/bar.png" is
    // a legal name.
    Node             &add_child(const std::string &name);

    /// fetch the node at the given index
    Node             &child(index_t idx);
    const Node       &child(index_t idx) const;

    /// fetch direct child by name
    /// the `child' methods don't modify map structure, and also
    /// do not try to resolve names as paths
    Node             &child(const std::string &name);
    const Node       &child(const std::string &name) const;

    /// fetch a pointer to the node  at the given path
    Node             *fetch_ptr(const std::string &path);
    const Node       *fetch_ptr(const std::string &path) const;

    /// fetch a pointer to the node at the given index
    Node             *child_ptr(index_t idx);
    const Node       *child_ptr(index_t idx) const;

    /// access child node via a path (equivalent to fetch via path)
    Node             &operator[](const std::string &path);
    const Node       &operator[](const std::string &path) const;

    /// access child node via index (equivalent to fetch via index)
    Node             &operator[](index_t idx);
    const Node       &operator[](index_t idx) const;

    /// returns the number of children (list and object interfaces)
    index_t number_of_children() const;

    /// returns a string with the path of this node
    /// relative to its immediate parent
    std::string name() const;
    /// returns a string with the path of this node up
    /// the tree, following the parent chain
    std::string path() const;

    /// checks if a node has a direct child with given name
    bool        has_child(const std::string &name) const;
    /// checks if given path exists in the Node hierarchy
    bool        has_path(const std::string &path) const;
    /// returns the direct child names for this node
    const std::vector<std::string> &child_names() const;

    /// adds an empty unnamed node to a list (list interface)
    /// TODO `append` is a strange name here, we want this interface
    /// but we may be abusing the common concept folks think of
    //  for the term `append`.
    Node   &append();

    /// remove child at index (list and object interfaces)
    void    remove(index_t idx);
    /// remove child at given path (object interface)
    void    remove(const std::string &path);
    /// remove child with given name, will not parse name as path
    void    remove_child(const std::string &name);
    /// rename a child (object interface)
    void    rename_child(const std::string &current_name,
                         const std::string &new_name);


    /// helpers to create a list of a homogenous types
    ///
    /// these allocates contiguous chunk of data to
    /// hold num_entries copies of the given schema or
    /// dtype, and change the node into a list with
    /// children pointing into this chunk of data
    ///
    /// the node owns the data, and the children
    /// are "set_external" to the proper location.
    ///
    void list_of(const Schema &schema,
                 index_t num_entries);


    void list_of(const DataType &dtype,
                 index_t num_entries);

    void list_of_external(void *data,
                          const Schema &schema,
                          index_t num_entries);


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node entry access methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- begin declaration of Node value access methods --
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
///@name Node Value Access Methods
///@{
//-----------------------------------------------------------------------------
/// description:
///  Direct access to data at leaf types.
//-----------------------------------------------------------------------------

     // signed integer scalars
    int8             as_int8()   const;
    int16            as_int16()  const;
    int32            as_int32()  const;
    int64            as_int64()  const;

    // unsigned integer scalars
    uint8            as_uint8()   const;
    uint16           as_uint16()  const;
    uint32           as_uint32()  const;
    uint64           as_uint64()  const;

    // floating point scalars
    float32          as_float32() const;
    float64          as_float64() const;

    // signed integers via pointers
    int8            *as_int8_ptr();
    int16           *as_int16_ptr();
    int32           *as_int32_ptr();
    int64           *as_int64_ptr();
    index_t         *as_index_t_ptr();

    // unsigned integers via pointers
    uint8           *as_uint8_ptr();
    uint16          *as_uint16_ptr();
    uint32          *as_uint32_ptr();
    uint64          *as_uint64_ptr();

    // floating point via pointers
    float32         *as_float32_ptr();
    float64         *as_float64_ptr();

    // signed integers via pointers
    const int8      *as_int8_ptr()   const;
    const int16     *as_int16_ptr()  const;
    const int32     *as_int32_ptr()  const;
    const int64     *as_int64_ptr()  const;

    // unsigned integers via pointers
    const uint8     *as_uint8_ptr()  const;
    const uint16    *as_uint16_ptr() const;
    const uint32    *as_uint32_ptr() const;
    const uint64    *as_uint64_ptr() const;
    const index_t   *as_index_t_ptr() const;

    // floating point via pointers
    const float32   *as_float32_ptr() const;
    const float64   *as_float64_ptr() const;

    // signed integer array types via conduit::DataArray
    int8_array       as_int8_array();
    int16_array      as_int16_array();
    int32_array      as_int32_array();
    int64_array      as_int64_array();

    // unsigned integer array types via conduit::DataArray
    uint8_array      as_uint8_array();
    uint16_array     as_uint16_array();
    uint32_array     as_uint32_array();
    uint64_array     as_uint64_array();

    // floating point array types via conduit::DataArray
    float32_array    as_float32_array();
    float64_array    as_float64_array();

    // index type array types via conduit::DataArray
    index_t_array    as_index_t_array() const;

    // signed integer array types via conduit::DataArray (const variants)
    const int8_array       as_int8_array()  const;
    const int16_array      as_int16_array() const;
    const int32_array      as_int32_array() const;
    const int64_array      as_int64_array() const;

    // unsigned integer array types via conduit::DataArray (const variants)
    const uint8_array      as_uint8_array()  const;
    const uint16_array     as_uint16_array() const;
    const uint32_array     as_uint32_array() const;
    const uint64_array     as_uint64_array() const;

    // floating point array value via conduit::DataArray (const variants)
    const float32_array    as_float32_array() const;
    const float64_array    as_float64_array() const;

    // signed integer accessors
    int8_accessor       as_int8_accessor()  const;
    int16_accessor      as_int16_accessor() const;
    int32_accessor      as_int32_accessor() const;
    int64_accessor      as_int64_accessor() const;

    // unsigned integer accessors
    uint8_accessor      as_uint8_accessor()  const;
    uint16_accessor     as_uint16_accessor() const;
    uint32_accessor     as_uint32_accessor() const;
    uint64_accessor     as_uint64_accessor() const;

    // floating point accessors
    float32_accessor    as_float32_accessor() const;
    float64_accessor    as_float64_accessor() const;

    // index type array accessors
    index_t_accessor    as_index_t_accessor() const;


    // char8_str cases
    char            *as_char8_str();
    const char      *as_char8_str() const;
    std::string      as_string()    const;

    // direct data pointer access
    void            *data_ptr();
    const void      *data_ptr() const;

    /// returns the number of bytes allocated by this node
    index_t          allocated_bytes() const
                        {return !m_mmaped ? m_data_size : 0;}

    /// returns the number of bytes mmaped by this node
    index_t          mmaped_bytes() const
                        {return m_mmaped ? m_data_size : 0;}

    void  *element_ptr(index_t idx)
        {return static_cast<char*>(m_data) + dtype().element_index(idx);};
    const void  *element_ptr(index_t idx) const
        {return static_cast<char*>(m_data) + dtype().element_index(idx);};

//-----------------------------------------------------------------------------
/// description:
///  Direct access to data at leaf types (native c++ types)
//-----------------------------------------------------------------------------`

    // c style scalar
    char           as_char()  const;
    short          as_short() const;
    int            as_int()   const;
    long           as_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    long long      as_long_long() const;
#endif

    // signed integer scalars
    signed char    as_signed_char() const;
    signed short   as_signed_short() const;
    signed int     as_signed_int()   const;
    signed long    as_signed_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    signed long long  as_signed_long_long() const;
#endif

    // unsigned integer scalars
    unsigned char    as_unsigned_char()  const;
    unsigned short   as_unsigned_short() const;
    unsigned int     as_unsigned_int()   const;
    unsigned long    as_unsigned_long()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned long long  as_unsigned_long_long() const;
#endif

    // floating point scalars
    float            as_float() const;
    double           as_double() const;

#ifdef CONDUIT_USE_LONG_DOUBLE
    long double      as_long_double() const;
#endif

    // c style via pointer
    char            *as_char_ptr();
    short           *as_short_ptr();
    int             *as_int_ptr();
    long            *as_long_ptr();

#ifdef CONDUIT_HAS_LONG_LONG
    long long       *as_long_long_ptr();
#endif

    // signed integers via pointers
    signed char      *as_signed_char_ptr();
    signed short     *as_signed_short_ptr();
    signed int       *as_signed_int_ptr();
    signed long      *as_signed_long_ptr();

#ifdef CONDUIT_HAS_LONG_LONG
    signed long long *as_signed_long_long_ptr();
#endif

    // unsigned integers via pointers
    unsigned char   *as_unsigned_char_ptr();
    unsigned short  *as_unsigned_short_ptr();
    unsigned int    *as_unsigned_int_ptr();
    unsigned long   *as_unsigned_long_ptr();

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned long long *as_unsigned_long_long_ptr();
#endif

    // floating point via pointers
    float           *as_float_ptr();
    double          *as_double_ptr();

#ifdef CONDUIT_USE_LONG_DOUBLE
    long double         *as_long_double_ptr();
#endif

    // char via pointer (const variant)
    const char       *as_char_ptr()  const;
    const short      *as_short_ptr() const;
    const int        *as_int_ptr()   const;
    const long       *as_long_ptr()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const long long  *as_long_long_ptr() const;
#endif

    // signed integers via pointers (const variants)
    const signed char       *as_signed_char_ptr()  const;
    const signed short      *as_signed_short_ptr() const;
    const signed int        *as_signed_int_ptr()   const;
    const signed long       *as_signed_long_ptr()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const signed long long  *as_signed_long_long_ptr() const;
#endif

    // unsigned integers via pointers (const variants)
    const unsigned char   *as_unsigned_char_ptr()  const;
    const unsigned short  *as_unsigned_short_ptr() const;
    const unsigned int    *as_unsigned_int_ptr()   const;
    const unsigned long   *as_unsigned_long_ptr()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const unsigned long long *as_unsigned_long_long_ptr() const;
#endif

    // floating point via pointers (const variants)
    const float           *as_float_ptr()  const;

    const double          *as_double_ptr() const;
#ifdef CONDUIT_USE_LONG_DOUBLE
    const long double     *as_long_double_ptr() const;
#endif

    // c style array via conduit::DataArray
    char_array        as_char_array();
    short_array       as_short_array();
    int_array         as_int_array();
    long_array        as_long_array();

#ifdef CONDUIT_HAS_LONG_LONG
    long_long_array  as_long_long_array();
#endif

    // signed integer array types via conduit::DataArray
    signed_char_array  as_signed_char_array();
    signed_short_array as_signed_short_array();
    signed_int_array   as_signed_int_array();
    signed_long_array  as_signed_long_array();

#ifdef CONDUIT_HAS_LONG_LONG
    signed_long_long_array  as_signed_long_long_array();
#endif

    // unsigned integer array types via conduit::DataArray
    unsigned_char_array    as_unsigned_char_array();
    unsigned_short_array   as_unsigned_short_array();
    unsigned_int_array     as_unsigned_int_array();
    unsigned_long_array    as_unsigned_long_array();

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned_long_long_array  as_unsigned_long_long_array();
#endif

    // floating point array types via conduit::DataArray
    float_array     as_float_array();
    double_array    as_double_array();

#ifdef CONDUIT_USE_LONG_DOUBLE
    long_double_array as_long_double_array();
#endif

    // c array type via conduit::DataArray (const variant)
    const char_array       as_char_array()  const;
    const short_array      as_short_array() const;
    const int_array        as_int_array()   const;
    const long_array       as_long_array()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const long_long_array  as_long_long_array() const;
#endif

    // signed integer array types via conduit::DataArray (const variants)
    const signed_char_array       as_signed_char_array()  const;
    const signed_short_array      as_signed_short_array() const;
    const signed_int_array        as_signed_int_array()   const;
    const signed_long_array       as_signed_long_array()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const signed_long_long_array  as_signed_long_long_array() const;
#endif


    // unsigned integer array types via conduit::DataArray (const variants)
    const unsigned_char_array    as_unsigned_char_array()  const;
    const unsigned_short_array   as_unsigned_short_array() const;
    const unsigned_int_array     as_unsigned_int_array()   const;
    const unsigned_long_array    as_unsigned_long_array()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    const unsigned_long_long_array  as_unsigned_long_long_array() const;
#endif

    // floating point array value via conduit::DataArray (const variants)
    const float_array     as_float_array()  const;
    const double_array    as_double_array() const;

#ifdef CONDUIT_USE_LONG_DOUBLE
    const long_double_array  as_long_double_array() const;
#endif

    // accessors

    // c array accessors
    char_accessor       as_char_accessor()  const;
    short_accessor      as_short_accessor() const;
    int_accessor        as_int_accessor()   const;
    long_accessor       as_long_accessor()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    long_long_accessor  as_long_long_accessor() const;
#endif

    // signed integer accessors
    signed_char_accessor       as_signed_char_accessor()  const;
    signed_short_accessor      as_signed_short_accessor() const;
    signed_int_accessor        as_signed_int_accessor()   const;
    signed_long_accessor       as_signed_long_accessor()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    signed_long_long_accessor  as_signed_long_long_accessor() const;
#endif


    // unsigned integer accessors
    unsigned_char_accessor    as_unsigned_char_accessor()  const;
    unsigned_short_accessor   as_unsigned_short_accessor() const;
    unsigned_int_accessor     as_unsigned_int_accessor()   const;
    unsigned_long_accessor    as_unsigned_long_accessor()  const;

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned_long_long_accessor  as_unsigned_long_long_accessor() const;
#endif

    // floating point accessors
    float_accessor     as_float_accessor()  const;
    double_accessor    as_double_accessor() const;

#ifdef CONDUIT_USE_LONG_DOUBLE
    long_double_accessor  as_long_double_accessor() const;
#endif


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Node value access methods --
//
//-----------------------------------------------------------------------------

private:
//-----------------------------------------------------------------------------
//
// -- begin declaration of  Private Construction Helpers --
//
//-----------------------------------------------------------------------------
///@name Private Construction Helpers
///@{
//-----------------------------------------------------------------------------
/// description:
/// these methods are used for construction by the Node & Generator classes.
//-----------------------------------------------------------------------------
    void             set_data_ptr(void *data_ptr);
    ///
    /// Note: set_schema_ptr is *only* used in the case were we have
    /// a schema pointer that is owned by a parent schema. Using it to set a
    /// pointer that should be owned by a node unleashes chaos.
    ///
    void             set_schema_ptr(Schema *schema_ptr);
    void             append_node_ptr(Node *node)
                        {m_children.push_back(node);}

    void             set_parent(Node *new_parent)
                        { m_parent = new_parent;}


//-----------------------------------------------------------------------------
///@}
//-----------------------------------------------------------------------------
//
// -- end declaration of Private Construction Helpers --
//
//-----------------------------------------------------------------------------


//=============================================================================
//-----------------------------------------------------------------------------
//
// -- private methods and members --
//
//-----------------------------------------------------------------------------
//=============================================================================

//-----------------------------------------------------------------------------
// value access related to conditional long long and long double support
//-----------------------------------------------------------------------------
// We provide connivence methods for native c types, but we don't want to
// provide them in the public api for long long and long double.
// Why? These types are ambiguous, if folks want a 64-bit integer, they should
// explicitly use conduit::int64, conduit::uint64, etc
// The only place where long long and long double will appear in the public
// interface is in the Node::Value() class, where it is needed for casting magic
// to work for uint64, etc types.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
// -- private methods that help with init, memory allocation, and cleanup --
//
//-----------------------------------------------------------------------------
    // setup a node to at as a given type
    void             init(const DataType &dtype);
    // memory allocation and mapping routines
    void             allocate(index_t dsize);
    void             allocate(const DataType &dtype);
    void             mmap(const std::string &stream_path,
                          index_t dsize);
    // release any alloced or memory mapped data
    void             release();
    // clean up everything (used by destructor)
    void             cleanup();

    // set defaults (used by constructors)
    void              init_defaults();
    // setup node to act as a list
    void              init_list();
    // setup node to act as an object
    void              init_object();

//-----------------------------------------------------------------------------
//
// -- private methods that help with protocol detection for load and save  --
//
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    static void  identify_protocol(const std::string &path,
                                   std::string &io_type);

//-----------------------------------------------------------------------------
//
// -- private methods that help with hierarchical construction --
//
//-----------------------------------------------------------------------------
    // work horse for complex node hierarchical setup
    static void      walk_schema(Node   *node,
                                 Schema *schema,
                                 void   *data,
                                 index_t allocator_id);

    static void      mirror_node(Node *node,
                                 Schema *schema,
                                 const Node *src);

//-----------------------------------------------------------------------------
//
// -- private methods that help with compaction, serialization, and info  --
//
//-----------------------------------------------------------------------------
    void              compact_to(uint8 *data,
                                 index_t curr_offset) const;
    /// compact helper for leaf types
    void              compact_elements_to(uint8 *data) const;


    void              serialize(uint8 *data,
                                index_t curr_offset) const;

    /// Implements recursive check for if node is contiguous to the
    /// passed start address. If contiguous, returns true and the
    /// last address of the contiguous block.
    ///
    /// this method recursively traverses a node hierarchy
    ///
    /// At each traversal step, it checks if the current Node is contiguous
    /// to the given address.
    ///
    /// If contiguous: it returns true and the last address of the
    /// contiguous block the ref pointer "end_addy"
    ///
    /// If NOT contiguous:  it returns false, and end_addy is set to NULL.
    ///
    /// to start the traversal, we use NULL input as a special case.
    ///
    /// The direct address checks are only done for leaves with data,
    /// nodes in the objects, lists, or empty roles don't directly
    /// advance the pointer.
    bool              contiguous_with(uint8  *start_addy,
                                      uint8 *&end_addy) const;

    void              info(Node &res,
                           const std::string &curr_path) const;

    /// helper that finds the first non null data pointer, used by
    /// contiguous_data_ptr()
    const void       *find_first_data_ptr() const;

//-----------------------------------------------------------------------------
//
// -- private to_json helpers --
//
//-----------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // the generic to_json methods are used by the specialized cases
    //-------------------------------------------------------------------------
    std::string         to_json_generic(bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;

    void                to_json_generic(const std::string &stream_path,
                                        bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;

    void                to_json_generic(std::ostream &os,
                                        bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to json without any conduit schema constructs
    //-------------------------------------------------------------------------
    std::string      to_pure_json(index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    void             to_pure_json(const std::string &stream_path,
                                  index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    void             to_pure_json(std::ostream &os,
                                  index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to json that contains conduit schema constructs
    //-------------------------------------------------------------------------
    std::string      to_detailed_json(index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    void             to_detailed_json(const std::string &stream_path,
                                      index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    void             to_detailed_json(std::ostream &os,
                                      index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to detailed json with address entry
    //-------------------------------------------------------------------------
    std::string      to_detailed_json_external(index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    void             to_detailed_json_external(const std::string &stream_path,
                                               index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    void             to_detailed_json_external(std::ostream &os,
                                               index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to json with data payload encoded using base64
    //-------------------------------------------------------------------------
    std::string      to_base64_json(index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

    void             to_base64_json(const std::string &stream_path,
                                    index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

    void             to_base64_json(std::ostream &os,
                                    index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

//-----------------------------------------------------------------------------
//
// -- private to_yaml helpers --
//
//-----------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // the generic to_yaml methods are used by the specialized cases
    //-------------------------------------------------------------------------
    std::string         to_yaml_generic(bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;

    void                to_yaml_generic(const std::string &stream_path,
                                        bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;

    void                to_yaml_generic(std::ostream &os,
                                        bool detailed,
                                        bool address,
                                        index_t indent=2,
                                        index_t depth=0,
                                        const std::string &pad=" ",
                                        const std::string &eoe="\n") const;
    //-------------------------------------------------------------------------
    // transforms the node to yaml without any conduit schema constructs
    //-------------------------------------------------------------------------
    std::string      to_pure_yaml(index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    void             to_pure_yaml(const std::string &stream_path,
                                  index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    void             to_pure_yaml(std::ostream &os,
                                  index_t indent=2,
                                  index_t depth=0,
                                  const std::string &pad=" ",
                                  const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to yaml that contains conduit schema constructs
    //-------------------------------------------------------------------------
    std::string      to_detailed_yaml(index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    void             to_detailed_yaml(const std::string &stream_path,
                                      index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    void             to_detailed_yaml(std::ostream &os,
                                      index_t indent=2,
                                      index_t depth=0,
                                      const std::string &pad=" ",
                                      const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to detailed yaml with address entry
    //-------------------------------------------------------------------------
    std::string      to_detailed_yaml_external(index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    void             to_detailed_yaml_external(const std::string &stream_path,
                                               index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    void             to_detailed_yaml_external(std::ostream &os,
                                               index_t indent=2,
                                               index_t depth=0,
                                               const std::string &pad=" ",
                                               const std::string &eoe="\n") const;

    //-------------------------------------------------------------------------
    // transforms the node to yaml with data payload encoded using base64
    //-------------------------------------------------------------------------
    std::string      to_base64_yaml(index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

    void             to_base64_yaml(const std::string &stream_path,
                                    index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

    void             to_base64_yaml(std::ostream &os,
                                    index_t indent=2,
                                    index_t depth=0,
                                    const std::string &pad=" ",
                                    const std::string &eoe="\n") const;

//-------------------------------------------------------------------------
// private base64 helper used by both yaml and json
//-------------------------------------------------------------------------
    void             to_base64(Node &n,
                               Node &bb64_data) const;

//-------------------------------------------------------------------------
// private summary string helper
// (public interface methods bundle options in a node)
//-------------------------------------------------------------------------
    void             to_summary_string_stream(std::ostream &os,
                                              index_t num_children_threshold,
                                              index_t num_elements_threshold,
                                              index_t indent,
                                              index_t depth,
                                              const std::string &pad,
                                              const std::string &eoe) const;

//-----------------------------------------------------------------------------
//
// -- conduit::Node private data members --
//
//-----------------------------------------------------------------------------
    /// pointer to this node's parent (if it exists)
    Node                *m_parent;
    /// pointer to this node's schema
    Schema              *m_schema;
    /// we need to know if *this* node created the schema
    bool                 m_owns_schema;

    /// collection of children
    std::vector<Node*>   m_children;

    // TODO: DataContainer?
    // pointer to the node's data
    void     *m_data;
    // size of the allocated or mmaped data point
    index_t   m_data_size;

    // flag that indicates this node allocated m_data
    bool      m_alloced;
    // flag that indicates if m_data is memory-mapped
    bool      m_mmaped;

    // private class that implements a cross platform memory map interface
    class MMap;

    // memory-map helper instance
    // This is only allocated if a memory map is active (m_mmaped is true)
    // Note: m_mmaped is used for bookkeeping during cases were we are
    // initializing nodes using memory maps, so it is still needed apart from
    // simply knowing if this pointer is valid.
    MMap     *m_mmap;

    // allocator id for memory
    index_t m_allocator_id;
};
//-----------------------------------------------------------------------------
// -- end conduit::Node --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
