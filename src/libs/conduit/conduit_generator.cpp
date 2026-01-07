// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_generator.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_generator.hpp"
#include "conduit_config.hpp"

//-----------------------------------------------------------------------------
// -- standard lib includes --
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <cstdlib>

//-----------------------------------------------------------------------------
// -- json includes and namespace --
//-----------------------------------------------------------------------------

#ifdef CONDUIT_USE_YYJSON
  #include "conduit_yyjson_interface.h"
  namespace conduit_json = conduit_yyjson;
#else
  #include "rapidjson/document.h"
  #include "rapidjson/error/en.h"
  namespace conduit_json = conduit_rapidjson;
#endif


//-----------------------------------------------------------------------------
// -- libyaml includes --
//-----------------------------------------------------------------------------
#include "yaml.h"


//-----------------------------------------------------------------------------
// -- conduit library includes --
//-----------------------------------------------------------------------------
#include "conduit_error.hpp"
#include "conduit_utils.hpp"


//-----------------------------------------------------------------------------
//
/// The CONDUIT_JSON_PARSE_ERROR macro use as a single place for handling
/// errors related to rapidjson parsing.
//
//-----------------------------------------------------------------------------
#define CONDUIT_JSON_PARSE_ERROR(json_str, document )                        \
{                                                                            \
    std::ostringstream __json_parse_oss;                                     \
    Generator::Parser::JSON::parse_error_details( json_str,                  \
                                                 document,                   \
                                                __json_parse_oss);           \
    CONDUIT_ERROR("JSON parse error: \n"                                     \
                  << __json_parse_oss.str()                                  \
                  << "\n");                                                  \
}


//-----------------------------------------------------------------------------
//
/// The CONDUIT_YAML_PARSE_ERROR macro use as a single place for handling
/// errors related to libyaml parsing.
//
//-----------------------------------------------------------------------------
#define CONDUIT_YAML_PARSE_ERROR( yaml_doc, yaml_parser )                    \
{                                                                            \
    std::ostringstream __yaml_parse_oss;                                     \
    Generator::Parser::YAML::parse_error_details( yaml_parser,               \
                                                __yaml_parse_oss);           \
    CONDUIT_ERROR("YAML parse error: \n"                                     \
                  << __yaml_parse_oss.str()                                  \
                  << "\n");                                                  \
}

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::Generator::Parser --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Generator::Parser -- concrete parsing implementations
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class Generator::Parser
{
public:

//-----------------------------------------------------------------------------
// Shared string parsing helpers
//-----------------------------------------------------------------------------
    // checks if c-string is a null pointer or empty
    static bool string_is_empty(const char *txt_value);

    // checks if input c-string is an integer or a double
    static bool string_is_number(const char *txt_value);

    // checks if input string holds something that converts
    // to a double (integer c-string will pass this check )
    static bool string_is_double(const char *txt_value);

    // checks if input c-string holds something that converts
    // to an integer
    static bool string_is_integer(const char *txt_value);

    // converts c-string to double
    static double string_to_double(const char *txt_value);
    // converts c-string to long
    static long int string_to_long(const char *txt_value);
    // converts c-string to unsigned long
    static unsigned long int string_to_unsigned_long(const char *txt_value);

    static index_t parse_leaf_dtype_name(const std::string &dtype_name);

//-----------------------------------------------------------------------------
// Generator::Parser::JSON handles parsing via rapidjson.
// We want to isolate the conduit API from the rapidjson headers
// so any methods using rapidjson types are defined in here.
// "friend Generator" in Node, allows Generator::Parser to construct complex
// nodes.
//-----------------------------------------------------------------------------
  class JSON
  {
  public:

    static const conduit_json::ParseFlag JSON_PARSE_OPTS = conduit_json::kParseNoFlags;

    static index_t json_to_numeric_dtype(const conduit_json::Value &jvalue);

    static index_t check_homogenous_json_array(const conduit_json::Value &jvalue);

    static void    parse_json_int64_array(const conduit_json::Value &jvalue,
                                          std::vector<int64> &res);

    // for efficiency - assumes res is already alloced to proper size
    static void    parse_json_int64_array(const conduit_json::Value &jvalue,
                                          int64_array &res);

    static void    parse_json_int64_array(const conduit_json::Value &jvalue,
                                          Node &node);

    static void    parse_json_uint64_array(const conduit_json::Value &jvalue,
                                           std::vector<uint64> &res);

    // for efficiency - assumes res is already alloced to proper size
    static void    parse_json_uint64_array(const conduit_json::Value &jvalue,
                                          uint64_array &res);


    static void    parse_json_uint64_array(const conduit_json::Value &jvalue,
                                           Node &node);

    static void    parse_json_float64_array(const conduit_json::Value &jvalue,
                                            std::vector<float64> &res);

    // for efficiency - assumes res is already alloced to proper size
    static void    parse_json_float64_array(const conduit_json::Value &jvalue,
                                            float64_array &res);

    static void    parse_json_float64_array(const conduit_json::Value &jvalue,
                                            Node &node);

    static void    parse_leaf_dtype(const conduit_json::Value &jvalue,
                                    index_t offset,
                                    DataType &dtype_res);

    static void    parse_inline_leaf(const conduit_json::Value &jvalue,
                                     Node &node);
    static void*   parse_inline_address(const conduit_json::Value& jvalue);

    static void    parse_inline_value(const conduit_json::Value &jvalue,
                                      Node &node);

    static void    walk_json_schema(Schema *schema,
                                    const   conduit_json::Value &jvalue,
                                    index_t curr_offset);

    static void    walk_pure_json_schema(Node  *node,
                                         Schema *schema,
                                         const conduit_json::Value &jvalue);

    // if data pointer is provided, data is copied into dest node
    static void    walk_json_schema(Node   *node,
                                    Schema *schema,
                                    void   *data,
                                    const conduit_json::Value &jvalue,
                                    index_t curr_offset);

    // if data pointer is provided, data is set_external into dest node
    static void    walk_json_schema_external(Node   *node,
                                             Schema *schema,
                                             void   *data,
                                             const conduit_json::Value &jvalue,
                                             index_t curr_offset);

    static void    parse_base64(Node *node,
                                const conduit_json::Value &jvalue);

    static void    parse_error_details(const std::string &json,
                                       const conduit_json::Document &document,
                                       std::ostream &os);
  };
//-----------------------------------------------------------------------------
// Generator::Parser::YAML handles parsing via libyaml.
// We want to isolate the conduit API from the libyaml headers
// so any methods using libyaml types are defined in here.
// "friend Generator" in Node, allows Generator::Parser to construct complex
// nodes.
//-----------------------------------------------------------------------------
  class YAML
  {
  public:


    //-----------------------------------------------------------------------------
    // Wrappers around libyaml c API that help us parse
    //-----------------------------------------------------------------------------

    // YAMLParserWrapper class helps with libyaml cleanup when
    // exceptions are thrown during parsing
    class YAMLParserWrapper
    {
    public:
        YAMLParserWrapper();
       ~YAMLParserWrapper();

       // parses and creates doc tree, throws exception
       // when things go wrong
       void         parse(const char *yaml_txt);

       yaml_document_t *yaml_doc_ptr();
       yaml_node_t     *yaml_doc_root_ptr();

    private:
        yaml_document_t m_yaml_doc;
        yaml_parser_t   m_yaml_parser;

        bool m_yaml_parser_is_valid;
        bool m_yaml_doc_is_valid;

    };

    //
    // yaml scalar (aka leaf) values are always strings, however that is
    // not a very useful way to parse into Conduit tree. We apply json
    // rules to the yaml leaves to get more useful types in Conduit
    //
    // excluded from the JSON-like rules are:
    //  boolean literals (true, false)
    //  the null literal (null)
    //
    // This is b/c we can't distinguish between string values like
    //    "true"
    // vs non-quoted literals like
    //    true
    // with the yaml parser
    //

    static bool check_yaml_is_number(const yaml_node_t *yaml_node);

    static bool check_yaml_is_int(const yaml_node_t *yaml_node);

    static long get_yaml_long(const yaml_node_t *yaml_node);

    static unsigned long get_yaml_unsigned_long(const yaml_node_t *yaml_node);

    static bool check_yaml_is_scalar_node(const yaml_node_t *yaml_node);

    static const char *get_yaml_string(const yaml_node_t *yaml_node);

    static bool check_yaml_is_sequence(const yaml_node_t *yaml_node);

    static int get_yaml_sequence_length(const yaml_node_t *yaml_node);

    static int get_yaml_num_members(const yaml_node_t *yaml_node);

    static bool check_yaml_is_mapping_node(const yaml_node_t *yaml_node);

    static yaml_node_t* fetch_yaml_node_from_list(yaml_document_t *yaml_doc,
                                                  const yaml_node_t *yaml_node,
                                                  const int index);

    static yaml_node_t* fetch_yaml_node_from_object_by_name(yaml_document_t *yaml_doc,
                                                            const yaml_node_t *yaml_node,
                                                            const std::string member_name);

    static void* parse_inline_address(const yaml_node_t *yaml_node);

    template <typename T>
    static void parse_yaml_array(yaml_document_t *yaml_doc,
                                 const yaml_node_t *yaml_node,
                                 std::vector<T> &res,
                                 const int seq_size);

    // for efficiency - assumes res is already alloced to proper size
    template <typename T>
    static void parse_yaml_array(yaml_document_t *yaml_doc,
                                 const yaml_node_t *yaml_node,
                                 DataArray<T> &res,
                                 const int seq_size);

    // parses generic leaf and places value in res
    static void parse_yaml_inline_leaf(const char *yaml_txt,
                                       Node &res);

    static void parse_inline_leaf(const char *yaml_txt,
                                  Node &node);

    static void parse_leaf_dtype(yaml_document_t *yaml_doc,
                                 const yaml_node_t *yaml_node,
                                 index_t offset,
                                 DataType &dtype_res);

    static void parse_inline_value(yaml_document_t *yaml_doc,
                                   const yaml_node_t *yaml_node,
                                   Node &node);

    // finds if leaf string is int64, float64, or neither (DataType::EMPTY_T)
    static index_t yaml_leaf_to_numeric_dtype(const char *txt_value);

    // checks if the input yaml node is a homogenous numeric sequence
    //
    // if not: returns DataType::EMPTY_T and seq_size = -1
    //
    // if so:
    //  seq_size contains the sequence length and:
    //  if homogenous integer sequence returns DataType::INT64_T
    //  if homogenous floating point sequence returns DataType::FLOAT64_T
    static index_t check_homogenous_yaml_numeric_sequence(const Node &node,
                                                          yaml_document_t *yaml_doc,
                                                          const yaml_node_t *yaml_node,
                                                          index_t &seq_size);

    static void    walk_yaml_schema(Schema *schema,
                                    yaml_document_t *yaml_doc,
                                    const yaml_node_t *yaml_node,
                                    index_t curr_offset);

    // if data pointer is provided, data is copied into dest node
    static void    walk_yaml_schema(Node   *node,
                                    Schema *schema,
                                    void   *data,
                                    yaml_document_t *yaml_doc,
                                    const yaml_node_t *yaml_node,
                                    index_t curr_offset,
                                    const bool external = false);

    // workhorse for parsing a pure yaml tree
    static void    walk_pure_yaml_schema(Node  *node,
                                         Schema *schema,
                                         yaml_document_t *yaml_doc,
                                         yaml_node_t *yaml_node);

    static void    parse_base64(Node *node,
                                yaml_document_t *yaml_doc,
                                yaml_node_t *yaml_node);

    // extract human readable parser errors
    static void    parse_error_details(yaml_parser_t *yaml_parser,
                                       std::ostream &os);

  };

};

//-----------------------------------------------------------------------------
// -- begin conduit::Generator::Parser:: --
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
// checks if input string is a null pointer or empty
bool
Generator::Parser::string_is_empty(const char *txt_value)
{
    if(txt_value == NULL)
        return true;
    return strlen(txt_value) == 0;
}

//---------------------------------------------------------------------------//
// checks if input string holds something that converts
// to a double (integer strings will pass this check )
bool
Generator::Parser::string_is_number(const char *txt_value)
{
    return string_is_integer(txt_value) || string_is_double(txt_value);
}

//---------------------------------------------------------------------------//
// checks if input string holds something that converts
// to a double (integer strings will pass this check )
bool
Generator::Parser::string_is_double(const char *txt_value)
{
    if(string_is_empty(txt_value))
        return false;
    char *val_end = NULL;
    strtod(txt_value,&val_end);
    return *val_end == 0;
}

//---------------------------------------------------------------------------//
// checks if input string holds something that converts
// to an integer
bool
Generator::Parser::string_is_integer(const char *txt_value)
{
    if(string_is_empty(txt_value))
        return false;
    char *val_end = NULL;
    strtol(txt_value,&val_end,10);
    return *val_end == 0;
}

//---------------------------------------------------------------------------//
double
Generator::Parser::string_to_double(const char *txt_value)
{
    char *val_end = NULL;
    return strtod(txt_value,&val_end);
}

//---------------------------------------------------------------------------//
long int
Generator::Parser::string_to_long(const char *txt_value)
{
    char *val_end = NULL;
    return strtol(txt_value,&val_end,10);
}

//---------------------------------------------------------------------------//
unsigned long int
Generator::Parser::string_to_unsigned_long(const char *txt_value)
{
    char *val_end = NULL;
    return strtoul(txt_value,&val_end,10);
}

//---------------------------------------------------------------------------//
index_t
Generator::Parser::parse_leaf_dtype_name(const std::string &dtype_name)
{
    index_t dtype_id = DataType::name_to_id(dtype_name);
    if (dtype_id == DataType::EMPTY_ID)
    {
        // also try native type names
        dtype_id = DataType::c_type_name_to_id(dtype_name);
    }

    // do an explicit check for empty
    if(dtype_id == DataType::EMPTY_ID && dtype_name != "empty")
    {
        CONDUIT_ERROR("Generator error:\n"
                       << "invalid leaf type "
                       << "\""  <<  dtype_name << "\"");
    }
    return dtype_id;
}


//-----------------------------------------------------------------------------
// -- begin conduit::Generator::Parser::JSON --
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
index_t
Generator::Parser::JSON::json_to_numeric_dtype(const conduit_json::Value &jvalue)
{
    index_t res = DataType::EMPTY_ID;
    if(jvalue.IsNumber())
    {
        // TODO: We could have better logic for dealing with int vs uint
        if(jvalue.IsUint64() ||
           jvalue.IsInt64()  ||
           jvalue.IsUint()   ||
           jvalue.IsInt())
        {
            res  = DataType::INT64_ID; // for int
        }
        else if(jvalue.IsDouble())
        {
            res  = DataType::FLOAT64_ID; // for float
        }
        // else -- value already inited to EMPTY_ID
    }
    else if(jvalue.IsString()) // we may have strings that are nan, inf, etc
    {
        if(string_is_double(jvalue.GetString()))
        {
            res  = DataType::FLOAT64_ID;
        }
    }

    return res;
}

//---------------------------------------------------------------------------//
index_t
Generator::Parser::JSON::check_homogenous_json_array(const conduit_json::Value &jvalue)
{
    // check for homogenous array of ints or floats
    // promote to float64 as the most wide type
    // (this is a heuristic decision)

    if(jvalue.Size() == 0)
        return DataType::EMPTY_ID;

    // we could also have string reps of nan, infinity, etc.
    // json_to_numeric_dtype handles that case fo us

    index_t val_type = json_to_numeric_dtype(jvalue[(conduit_json::SizeType)0]);
    bool homogenous  = (val_type != DataType::EMPTY_ID);

    for (conduit_json::SizeType i = 1; i < jvalue.Size() && homogenous; i++)
    {
        index_t curr_val_type = json_to_numeric_dtype(jvalue[i]);
        if(val_type == DataType::INT64_ID  &&
           curr_val_type ==  DataType::FLOAT64_ID)
        {
            // promote to a double (may be lossy in some cases)
            val_type = DataType::FLOAT64_ID;
        }
        else if(curr_val_type == DataType::EMPTY_ID)
        {
            // non homogenous inline
            homogenous = false;
            val_type = DataType::EMPTY_ID;
        }
    }

    return val_type;
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_int64_array(const conduit_json::Value &jvalue,
                                                std::vector<int64> &res)
{
   res.resize(jvalue.Size(),0);
   for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
   {
       res[i] = jvalue[i].GetInt64();
   }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_int64_array(const conduit_json::Value &jvalue,
                                                int64_array &res)
{
    // for efficiency - assumes res is already alloced to proper size
    for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
    {
        res[i] = jvalue[i].GetInt64();
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_int64_array(const conduit_json::Value &jvalue,
                                                Node &node)
{
    // TODO: we can make this more efficient
    std::vector<int64> vals;
    parse_json_int64_array(jvalue,vals);

    switch(node.dtype().id())
    {
        // signed ints
        case DataType::INT8_ID:
            node.as_int8_array().set(vals);
            break;
        case DataType::INT16_ID:
            node.as_int16_array().set(vals);
            break;
        case DataType::INT32_ID:
            node.as_int32_array().set(vals);
            break;
        case DataType::INT64_ID:
            node.as_int64_array().set(vals);
            break;
        // unsigned ints
        case DataType::UINT8_ID:
            node.as_uint8_array().set(vals);
            break;
        case DataType::UINT16_ID:
            node.as_uint16_array().set(vals);
            break;
        case DataType::UINT32_ID:
            node.as_uint32_array().set(vals);
            break;
        case DataType::UINT64_ID:
            node.as_uint64_array().set(vals);
            break;
        //floats
        case DataType::FLOAT32_ID:
            node.as_float32_array().set(vals);
            break;
        case DataType::FLOAT64_ID:
            node.as_float64_array().set(vals);
            break;
        default:
            CONDUIT_ERROR("JSON Generator error:\n"
                           << "attempting to set non-numeric Node with"
                           << " int64 array");
            break;
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_uint64_array(const conduit_json::Value &jvalue,
                                                 std::vector<uint64> &res)
{
    res.resize(jvalue.Size(),0);
    for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
    {
        res[i] = jvalue[i].GetUint64();
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_uint64_array(const conduit_json::Value &jvalue,
                                                 uint64_array &res)
{
    // for efficiency - assumes res is already alloced to proper size
    for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
    {
       res[i] = jvalue[i].GetUint64();
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_uint64_array(const conduit_json::Value &jvalue,
                                                 Node &node)
{
    // TODO: we can make this more efficient
    std::vector<uint64> vals;
    parse_json_uint64_array(jvalue,vals);

    switch(node.dtype().id())
    {
        // signed ints
        case DataType::INT8_ID:
            node.as_int8_array().set(vals);
            break;
        case DataType::INT16_ID:
            node.as_int16_array().set(vals);
            break;
        case DataType::INT32_ID:
            node.as_int32_array().set(vals);
            break;
        case DataType::INT64_ID:
            node.as_int64_array().set(vals);
            break;
        // unsigned ints
        case DataType::UINT8_ID:
            node.as_uint8_array().set(vals);
            break;
        case DataType::UINT16_ID:
            node.as_uint16_array().set(vals);
            break;
        case DataType::UINT32_ID:
            node.as_uint32_array().set(vals);
            break;
        case DataType::UINT64_ID:
            node.as_uint64_array().set(vals);
            break;
        //floats
        case DataType::FLOAT32_ID:
            node.as_float32_array().set(vals);
            break;
        case DataType::FLOAT64_ID:
            node.as_float64_array().set(vals);
            break;
        default:
            CONDUIT_ERROR("JSON Generator error:\n"
                           << "attempting to set non-numeric Node with"
                           << " uint64 array");
            break;
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_float64_array(const conduit_json::Value &jvalue,
                                                  std::vector<float64> &res)
{
    res.resize(jvalue.Size(),0);
    for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
    {
        if(jvalue[i].IsNumber())
        {
            res[i] = jvalue[i].GetDouble();
        }
        else if(jvalue[i].IsString()) // could be an inline string with nan,inf,etc
        {
            res[i] = string_to_double(jvalue[i].GetString());
        }
        else
        {
            CONDUIT_ERROR("JSON Generator error:\n"
                           << "parse_json_float64_array: unexpected JSON value type "
                           << "at index" << i);
        }
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_float64_array(const conduit_json::Value &jvalue,
                                                  float64_array &res)
{
    // for efficiency - assumes res is already alloced to proper size
    for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
    {
        if(jvalue[i].IsNumber())
        {
            res[i] = jvalue[i].GetDouble();
        }
        else if(jvalue[i].IsString()) // could be an inline string with nan,inf,etc
        {
            res[i] = string_to_double(jvalue[i].GetString());
        }
        else
        {
            CONDUIT_ERROR("JSON Generator error:\n"
                           << "parse_json_float64_array: unexpected JSON value type "
                           << "at index" << i);
        }
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_json_float64_array(const conduit_json::Value &jvalue,
                                                  Node &node)
{
    // TODO: we can make this more efficient
    std::vector<float64> vals;
    parse_json_float64_array(jvalue,vals);

    switch(node.dtype().id())
    {
        case DataType::INT8_ID:
            node.as_int8_array().set(vals);
            break;
        case DataType::INT16_ID:
            node.as_int16_array().set(vals);
            break;
        case DataType::INT32_ID:
            node.as_int32_array().set(vals);
            break;
        case DataType::INT64_ID:
            node.as_int64_array().set(vals);
            break;
        // unsigned ints
        case DataType::UINT8_ID:
            node.as_uint8_array().set(vals);
            break;
        case DataType::UINT16_ID:
            node.as_uint16_array().set(vals);
            break;
        case DataType::UINT32_ID:
            node.as_uint32_array().set(vals);
            break;
        case DataType::UINT64_ID:
            node.as_uint64_array().set(vals);
            break;
        //floats
        case DataType::FLOAT32_ID:
            node.as_float32_array().set(vals);
            break;
        case DataType::FLOAT64_ID:
            node.as_float64_array().set(vals);
            break;
        default:
            CONDUIT_ERROR("JSON Generator error:\n"
                           << "attempting to set non-numeric Node with"
                           << " float64 array");
            break;
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_leaf_dtype(const conduit_json::Value &jvalue,
                                          index_t offset,
                                          DataType &dtype_res)
{

    if(jvalue.IsString())
    {
        std::string dtype_name(jvalue.GetString());
        index_t dtype_id = parse_leaf_dtype_name(dtype_name);
        index_t ele_size = DataType::default_bytes(dtype_id);
        dtype_res.set(dtype_id,
                      1,
                      offset,
                      ele_size,
                      ele_size,
                      Endianness::DEFAULT_ID);
    }
    else if(jvalue.IsObject())
    {
        CONDUIT_ASSERT( ( jvalue.HasMember("dtype") && jvalue["dtype"].IsString() ),
                        "JSON Generator error:\n"
                         << "'dtype' must be a JSON string.");

        std::string dtype_name(jvalue["dtype"].GetString());

        index_t length = 0;

        auto extract_uint64_member = [&](const char *member_name,
                                         index_t &value_to_change)
        {
            if (jvalue.HasMember(member_name))
            {
                const conduit_json::Value &json_value = jvalue[member_name];
                CONDUIT_ASSERT(json_value.IsNumber(),
                               "JSON Generator error:\n"
                               << "'" << member_name << "' must be a number ");
                value_to_change = json_value.GetUint64();
            }
        };

        if (jvalue.HasMember("number_of_elements"))
        {
            const conduit_json::Value &json_num_eles = jvalue["number_of_elements"];
            if(json_num_eles.IsNumber())
            {
                length = json_num_eles.GetUint64();
            }
            else
            {
                CONDUIT_ERROR("JSON Generator error:\n"
                               << "'number_of_elements' must be a number ");
            }
        }
        //
        // TODO DEPRECATE and replace with the lambda up above
        //
        // length is the old schema style, we should deprecate this path
        else if (jvalue.HasMember("length"))
        {
            const conduit_json::Value &json_len = jvalue["length"];
            if(json_len.IsNumber())
            {
                length = json_len.GetUint64();
            }
            else
            {
                CONDUIT_ERROR("JSON Generator error:\n"
                               << "'length' must be a number ");
            }
        }

        index_t dtype_id  = parse_leaf_dtype_name(dtype_name);
        index_t ele_size  = DataType::default_bytes(dtype_id);
        index_t stride    = ele_size;

        //  parse offset (override default if passed)
        extract_uint64_member("offset", offset);

        // parse stride (override default if passed)
        extract_uint64_member("stride", stride);

        // parse element_bytes (override default if passed)
        extract_uint64_member("element_bytes", ele_size);

        // parse endianness (override default if passed)
        index_t endianness = Endianness::DEFAULT_ID;
        if(jvalue.HasMember("endianness"))
        {
            const conduit_json::Value &json_endianness = jvalue["endianness"];
            if(json_endianness.IsString())
            {
                std::string end_val(json_endianness.GetString());
                if(end_val == "big")
                {
                    endianness = Endianness::BIG_ID;
                }
                else if(end_val == "little")
                {
                    endianness = Endianness::LITTLE_ID;
                }
                else
                {
                    CONDUIT_ERROR("JSON Generator error:\n"
                              << "'endianness' must be a string"
                              << " (\"big\" or \"little\")"
                              << " parsed value: " << end_val);
                }
            }
            else
            {
                CONDUIT_ERROR("JSON Generator error:\n"
                          << "'endianness' must be a string"
                          << " (\"big\" or \"little\")");
            }
        }

        if (0 == length)
        {
            if(jvalue.HasMember("value") &&
               jvalue["value"].IsArray())
            {
                length = jvalue["value"].Size();
            }
            // support explicit length 0 in a schema
            else if(!jvalue.HasMember("number_of_elements"))
            {
                length = 1;
            }
        }

        dtype_res.set(dtype_id,
                      length,
                      offset,
                      stride,
                      ele_size,
                      endianness);
    }
    else
    {
        CONDUIT_ERROR("JSON Generator error:\n"
                       << "a leaf dtype entry must be a JSON string or"
                       <<  " JSON object.");
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_inline_leaf(const conduit_json::Value &jvalue,
                                           Node &node)
{
    if(jvalue.IsString())
    {
        if(node.dtype().id() == DataType::CHAR8_STR_ID)
        {
            std::string sval(jvalue.GetString());
            node.set(utils::unescape_special_chars(sval));
        }
        else
        {
             // JSON type incompatible with char8_str
             // only allow strings to be assigned to a char8_str type
             // throw parsing error if our inline values
             // don't match what we expected

            CONDUIT_ERROR("JSON Generator error:\n"
                           << "a JSON string can only be used as an inline"
                           << " value for a Conduit CHAR8_STR Node.");
        }
    }
    else if(jvalue.IsBool())
    {
        //
        if(node.dtype().id() == DataType::UINT8_ID)
        {
            node.set((uint8)jvalue.GetBool());
        }
        else
        {
             // JSON type incompatible with uint8
             // only allow JSON bools to be assigned to a uint8 type
             // throw parsing error if our inline values
             // don't match what we expected

            CONDUIT_ERROR("JSON Generator error:\n"
                           << "a JSON bool can only be used as an inline"
                           << " value for a Conduit UINT8 Node.");

        }
    }
    else if(jvalue.IsNumber())
    {
        switch(node.dtype().id())
        {
            // signed ints
            case DataType::INT8_ID:
                node.set((int8)jvalue.GetInt64());
                break;
            case DataType::INT16_ID:
                node.set((int16)jvalue.GetInt64());
                break;
            case DataType::INT32_ID:
                node.set((int32)jvalue.GetInt64());
                break;
            case DataType::INT64_ID:
                node.set((int64)jvalue.GetInt64());
                break;
            // unsigned ints
            case DataType::UINT8_ID:
                node.set((uint8)jvalue.GetUint64());
                break;
            case DataType::UINT16_ID:
                node.set((uint16)jvalue.GetUint64());
                break;
            case DataType::UINT32_ID:
                node.set((uint32)jvalue.GetUint64());
                break;
            case DataType::UINT64_ID:
                node.set((uint64)jvalue.GetUint64());
                break;
            //floats
            case DataType::FLOAT32_ID:
                node.set((float32)jvalue.GetDouble());
                break;
            case DataType::FLOAT64_ID:
                node.set((float64)jvalue.GetDouble());
                break;
            default:
                // JSON type incompatible with numeric
                // only allow numeric to be assigned to a numeric type
                // throw parsing error if our inline values
                // don't match what we expected
                CONDUIT_ERROR("JSON Generator error:\n"
                              << "a JSON number can only be used as an inline"
                              << " value for a Conduit Numeric Node.");
                break;
        }
    }
    else if(jvalue.IsNull())
    {
        // empty data type
        node.reset();
    }
}

//---------------------------------------------------------------------------//
void *
Generator::Parser::JSON::parse_inline_address(const conduit_json::Value &jvalue)
{
    void * res = nullptr;
    if(jvalue.IsString())
    {
        std::string sval(jvalue.GetString());
        res = utils::hex_string_to_value<void*>(sval);
    }
    // else if(jvalue.IsNumber())
    // {
    //     // TODO: FUTURE? ...
    // }
    else
    {
         CONDUIT_ERROR("JSON Generator error:\n"
                              << "inline address should be a string");
    }
    return res;
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_inline_value(const conduit_json::Value &jvalue,
                                            Node &node)
{
    if(jvalue.IsArray())
    {
        // we assume a "value" is a leaf or list of compatible leaves
        index_t hval_type = check_homogenous_json_array(jvalue);

        CONDUIT_ASSERT( (node.dtype().number_of_elements() >= static_cast<index_t>(jvalue.Size()) ),
                       "JSON Generator error:\n"
                        << "number of elements in JSON array is more"
                        << "than dtype can hold");

        if(hval_type == DataType::INT64_ID)
        {
            if(node.dtype().is_unsigned_integer())
            {
                parse_json_uint64_array(jvalue,node);
            }
            else
            {
                parse_json_int64_array(jvalue,node);
            }
        }
        else if(hval_type == DataType::FLOAT64_ID)
        {
            parse_json_float64_array(jvalue,node);
        }
        else if(hval_type == DataType::EMPTY_ID)
        {
            // we need to allow this case but do nothing.
            // for conduit_json cases, the node will
            // have the right data type
        }
        else
        {
            // Parsing Error, not homogenous
            CONDUIT_ERROR("JSON Generator error:\n"
                        << "a JSON array for value initialization"
                        << " is not homogenous");
        }
    }
    else
    {
        parse_inline_leaf(jvalue,node);
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::walk_json_schema(Schema *schema,
                                          const   conduit_json::Value &jvalue,
                                          index_t curr_offset)
{
    // object cases
    if(jvalue.IsObject())
    {
        if (jvalue.HasMember("dtype"))
        {
            // if dtype is an object, we have a "list_of" case
            const conduit_json::Value &dt_value = jvalue["dtype"];
            if(dt_value.IsObject())
            {
                int length =1;
                if(jvalue.HasMember("length"))
                {
                    const conduit_json::Value &len_value = jvalue["length"];
                    if(len_value.IsObject() &&
                       len_value.HasMember("reference"))
                    {
                        CONDUIT_ERROR("JSON Generator error:\n"
                                      << "'reference' option is not supported"
                                      << " when parsing to a Schema because"
                                      << " reference data does not exist.");
                    }
                    else if(len_value.IsNumber())
                    {
                        length = len_value.GetInt();
                    }
                    else
                    {
                        CONDUIT_ERROR("JSON Generator error:\n"
                                      << "'length' must be a JSON Object or"
                                      << " JSON number");
                    }
                }
                // we will create `length' # of objects of obj des by dt_value

                // TODO: we only need to parse this once, not leng # of times
                // but this is the easiest way to start.
                for (int i = 0; i < length; i ++)
                {
                    Schema &curr_schema = schema->append();
                    curr_schema.set(DataType::list());
                    walk_json_schema(&curr_schema, dt_value, curr_offset);
                    curr_offset += curr_schema.total_strided_bytes();
                }
            }
            else
            {
                // handle leaf node with explicit props
                DataType dtype;
                parse_leaf_dtype(jvalue,curr_offset,dtype);
                schema->set(dtype);
            }
        }
        else
        {
            // if we make it here and have an empty json object
            // we still want the conduit schema to take on the
            // object role
            schema->set(DataType::object());

            // loop over all entries
            for (conduit_json::Value::ConstMemberIterator itr =
                 jvalue.MemberBegin();
                 itr != jvalue.MemberEnd(); ++itr)
            {
                const std::string entry_name(itr->name.GetString());
                Schema &curr_schema = schema->add_child(entry_name);
                curr_schema.set(DataType::object());
                walk_json_schema(&curr_schema,itr->value, curr_offset);
                curr_offset += curr_schema.total_strided_bytes();
            }
        }
    }
    // List case
    else if(jvalue.IsArray())
    {
        // if we make it here and have an empty json list
        // we still want the conduit schema to take on the
        // list role
        schema->set(DataType::list());

        for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
        {
            Schema &curr_schema = schema->append();
            curr_schema.set(DataType::list());
            walk_json_schema(&curr_schema,jvalue[i], curr_offset);
            curr_offset += curr_schema.total_strided_bytes();
        }
    }
    // Simplest case, handles "uint32", "float64", etc
    else if(jvalue.IsString())
    {
        DataType dtype;
        parse_leaf_dtype(jvalue,curr_offset,dtype);
        schema->set(dtype);
    }
    else
    {
        CONDUIT_ERROR("JSON Generator error:\n"
                      << "Invalid JSON type for parsing Schema."
                      << "Expected: JSON Object, Array, or String");
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::walk_pure_json_schema(Node *node,
                                               Schema *schema,
                                               const conduit_json::Value &jvalue)
{
    // object cases
    if(jvalue.IsObject())
    {
        // if we make it here and have an empty json object
        // we still want the conduit node to take on the
        // object role
        schema->set(DataType::object());
        // loop over all entries
        for (conduit_json::Value::ConstMemberIterator itr = jvalue.MemberBegin();
             itr != jvalue.MemberEnd(); ++itr)
        {
            std::string entry_name(itr->name.GetString());

            // json files may have duplicate object names
            // we could provide some clear semantics, such as:
            //   always use first instance, or always use last instance
            // however duplicate object names are most likely a
            // typo, so it's best to throw an error

            if(schema->has_child(entry_name))
            {
                CONDUIT_ERROR("JSON Generator error:\n"
                              << "Duplicate JSON object name: "
                              << utils::join_path(node->path(),entry_name));
            }

            Schema *curr_schema = &schema->add_child(entry_name);

            Node *curr_node = new Node();
            curr_node->set_schema_ptr(curr_schema);
            curr_node->set_parent(node);
            node->append_node_ptr(curr_node);

            walk_pure_json_schema(curr_node,
                                  curr_schema,
                                  itr->value);
        }
    }
    // List case
    else if (jvalue.IsArray())
    {
        index_t hval_type = check_homogenous_json_array(jvalue);
        if(hval_type == DataType::INT64_ID)
        {
            node->set(DataType::int64(jvalue.Size()));
            int64_array vals = node->value();
            parse_json_int64_array(jvalue,vals);
        }
        else if(hval_type == DataType::FLOAT64_ID)
        {
            node->set(DataType::float64(jvalue.Size()));
            float64_array vals = node->value();
            parse_json_float64_array(jvalue,vals);
        }
        else // not numeric array
        {
            // if we make it here and have an empty json list
            // we still want the conduit node to take on the
            // list role
            schema->set(DataType::list());

            for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
            {
                schema->append();
                Schema *curr_schema = schema->child_ptr(i);
                Node * curr_node = new Node();
                curr_node->set_schema_ptr(curr_schema);
                curr_node->set_parent(node);
                node->append_node_ptr(curr_node);
                walk_pure_json_schema(curr_node,curr_schema,jvalue[i]);
            }
        }
    }
    // Simplest case, handles "uint32", "float64", with extended type info
    else if(jvalue.IsString()) // bytestr case
    {
        std::string sval(jvalue.GetString());
        node->set(sval);
    }
    else if(jvalue.IsNull())
    {
        node->reset();
    }
    else if(jvalue.IsBool())
    {
        // we store bools as uint8s
        if(jvalue.IsTrue())
        {
            node->set((uint8)1);
        }
        else
        {
            node->set((uint8)0);
        }
    }
    else if(jvalue.IsNumber())
    {
        // use 64bit types by default ...
        if(jvalue.IsInt() || jvalue.IsInt64())
        {
            node->set((int64)jvalue.GetInt64());
        }
        else if(jvalue.IsUint() || jvalue.IsUint64())
        {
            node->set((uint64)jvalue.GetUint64());
        }
        else  // double case
        {
            node->set((float64)jvalue.GetDouble());
        }
    }
    else
    {
        // not sure if can an even land here, but catch error just in case.
        CONDUIT_ERROR("JSON Generator error:\n"
                      << "Invalid JSON type for parsing Node from pure JSON."
                      << " Expected: JSON Object, Array, String, Null,"
                      << " Boolean, or Number");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::walk_json_schema(Node   *node,
                                          Schema *schema,
                                          void   *data,
                                          const conduit_json::Value &jvalue,
                                          index_t curr_offset)
{
    // object cases
    if(jvalue.IsObject())
    {
        if (jvalue.HasMember("dtype"))
        {
            // if dtype is an object, we have a "list_of" case
            const conduit_json::Value &dt_value = jvalue["dtype"];
            if(dt_value.IsObject())
            {
                index_t length =1;
                if(jvalue.HasMember("length"))
                {
                    if(jvalue["length"].IsNumber())
                    {
                        length = jvalue["length"].GetInt();
                    }
                    else if(jvalue["length"].IsObject() &&
                            jvalue["length"].HasMember("reference"))
                    {
                        const std::string ref_path =
                          jvalue["length"]["reference"].GetString();
                        length = node->fetch(ref_path).to_index_t();
                    }
                    else
                    {
                        CONDUIT_ERROR("JSON Parsing error:\n"
                                      << "'length' must be a number "
                                      << "or reference.");
                    }

                }
                // we will create `length' # of objects of obj des by dt_value

                // TODO: we only need to parse this once, not leng # of times
                // but this is the easiest way to start.
                for(index_t i=0;i< length;i++)
                {
                    schema->append();
                    Schema *curr_schema = schema->child_ptr(i);
                    Node *curr_node = new Node();
                    curr_node->set_schema_ptr(curr_schema);
                    curr_node->set_parent(node);
                    node->append_node_ptr(curr_node);
                    walk_json_schema(curr_node,
                                     curr_schema,
                                     data,
                                     dt_value,
                                     curr_offset);
                    // auto offset only makes sense when we have data
                    if(data != NULL)
                        curr_offset += curr_schema->total_strided_bytes();
                }

            }
            else
            {
                // handle leaf node with explicit props
                DataType src_dtype;
                parse_leaf_dtype(jvalue,curr_offset,src_dtype);

                DataType des_dtype;
                src_dtype.compact_to(des_dtype);

                // check for explciit address
                if(jvalue.HasMember("address"))
                {
                    void *data_ptr = parse_inline_address(jvalue["address"]);
                    node->set_external(src_dtype,data_ptr);
                }
                else
                {
                    if(data != NULL)
                    {
                        uint8 *src_data_ptr = ((uint8*)data) + src_dtype.offset();
                        // node is already linked to the schema pointer
                        // we need to dynamically alloc, use compact dtype
                        node->set(des_dtype); // causes an init
                        // copy bytes from src data to node's memory
                        utils::conduit_memcpy_strided_elements(node->data_ptr(),               // dest data
                                                               des_dtype.number_of_elements(), // num ele
                                                               des_dtype.element_bytes(),      // ele bytes
                                                               des_dtype.stride(),             // dest stride
                                                               src_data_ptr,                   // src data
                                                               src_dtype.stride());            // src stride
                    }
                    else
                    {
                        // node is already linked to the schema pointer
                        // we need to dynamically alloc, use compact dtype
                        node->set(des_dtype); // causes an init
                    }

                    // check for inline json values
                    if(jvalue.HasMember("value"))
                    {
                        parse_inline_value(jvalue["value"],*node);
                    }
                }
            }
        }
        else // object case
        {
            schema->set(DataType::object());
            // standard object case - loop over all entries
            for (conduit_json::Value::ConstMemberIterator itr =
                 jvalue.MemberBegin();
                 itr != jvalue.MemberEnd(); ++itr)
            {
                std::string entry_name(itr->name.GetString());

                // json files may have duplicate object names
                // we could provide some clear semantics, such as:
                //   always use first instance, or always use last instance.
                // however duplicate object names are most likely a
                // typo, so it's best to throw an error
                //
                // also its highly unlikely that the auto offset case
                // could safely deal with offsets for the
                // duplicate key case
                if(schema->has_child(entry_name))
                {
                    CONDUIT_ERROR("JSON Generator error:\n"
                                  << "Duplicate JSON object name: "
                                  << utils::join_path(node->path(),entry_name));
                }

                Schema *curr_schema = &schema->add_child(entry_name);
                Node *curr_node = new Node();
                curr_node->set_schema_ptr(curr_schema);
                curr_node->set_parent(node);
                node->append_node_ptr(curr_node);
                walk_json_schema(curr_node,
                                 curr_schema,
                                 data,
                                 itr->value,
                                 curr_offset);

                // auto offset only makes sense when we have data
                if(data != NULL)
                    curr_offset += curr_schema->total_strided_bytes();
            }

        }
    }
    // List case
    else if (jvalue.IsArray())
    {
        schema->set(DataType::list());

        for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
        {
            schema->append();
            Schema *curr_schema = schema->child_ptr(i);
            Node *curr_node = new Node();
            curr_node->set_schema_ptr(curr_schema);
            curr_node->set_parent(node);
            node->append_node_ptr(curr_node);
            walk_json_schema(curr_node,
                             curr_schema,
                             data,
                             jvalue[i],
                             curr_offset);
            // auto offset only makes sense when we have data
            if(data != NULL)
                curr_offset += curr_schema->total_strided_bytes();
        }
    }
    // Simplest case, handles "uint32", "float64", with extended type info
    else if(jvalue.IsString())
    {
        DataType dtype;
        parse_leaf_dtype(jvalue,curr_offset,dtype);
        schema->set(dtype);

        if(data != NULL)
        {
            // node is already linked to the schema pointer
            node->set_data_ptr(data);
        }
        else
        {
            // node is already linked to the schema pointer
            // we need to dynamically alloc
            node->set(dtype);  // causes an init
        }
    }
    else
    {
        CONDUIT_ERROR("JSON Generator error:\n"
                      << "Invalid JSON type for parsing Node."
                      << " Expected: JSON Object, Array, or String");
    }
}

//---------------------------------------------------------------------------//
// TODO can this be creatively combined with walk_json_schema? The functions
// are nearly identical
void
Generator::Parser::JSON::walk_json_schema_external(Node   *node,
                                                   Schema *schema,
                                                   void   *data,
                                                   const conduit_json::Value &jvalue,
                                                   index_t curr_offset)
{
    // object cases
    if(jvalue.IsObject())
    {

        if (jvalue.HasMember("dtype"))
        {
            // if dtype is an object, we have a "list_of" case
            const conduit_json::Value &dt_value = jvalue["dtype"];
            if(dt_value.IsObject())
            {
                index_t length =1;
                if(jvalue.HasMember("length"))
                {
                    if(jvalue["length"].IsNumber())
                    {
                        length = jvalue["length"].GetInt();
                    }
                    else if(jvalue["length"].IsObject() &&
                            jvalue["length"].HasMember("reference"))
                    {
                        std::string ref_path =
                          jvalue["length"]["reference"].GetString();
                        length = node->fetch(ref_path).to_index_t();
                    }
                    else
                    {
                        CONDUIT_ERROR("JSON Parsing error:\n"
                                      << "'length' must be a number "
                                      << "or reference.");
                    }

                }
                // we will create `length' # of objects of obj des by dt_value

                // TODO: we only need to parse this once, not leng # of times
                // but this is the easiest way to start.
                for(index_t i=0;i< length;i++)
                {
                    schema->append();
                    Schema *curr_schema = schema->child_ptr(i);
                    Node *curr_node = new Node();
                    curr_node->set_schema_ptr(curr_schema);
                    curr_node->set_parent(node);
                    node->append_node_ptr(curr_node);
                    walk_json_schema_external(curr_node,
                                              curr_schema,
                                              data,
                                              dt_value,
                                              curr_offset);
                    // auto offset only makes sense when we have data
                    if(data != NULL)
                        curr_offset += curr_schema->total_strided_bytes();
                }
            }
            else
            {
                // handle leaf node with explicit props
                DataType dtype;

                parse_leaf_dtype(jvalue,curr_offset,dtype);

                // check for explciit address
                if(jvalue.HasMember("address"))
                {
                    void *data_ptr = parse_inline_address(jvalue["address"]);
                    node->set_external(dtype,data_ptr);
                }
                else
                {

                    if(data != NULL)
                    {
                        // node is already linked to the schema pointer
                        schema->set(dtype);
                        node->set_data_ptr(data);
                    }
                    else
                    {
                        // node is already linked to the schema pointer
                        // we need to dynamically alloc
                        node->set(dtype);  // causes an init
                    }

                    // check for inline json values
                    if(jvalue.HasMember("value"))
                    {
                        parse_inline_value(jvalue["value"],*node);
                    }
                }
            }
        }
        else // object case
        {
            schema->set(DataType::object());
            // standard object case - loop over all entries
            for (conduit_json::Value::ConstMemberIterator itr =
                 jvalue.MemberBegin();
                 itr != jvalue.MemberEnd(); ++itr)
            {
                std::string entry_name(itr->name.GetString());

                // json files may have duplicate object names
                // we could provide some clear semantics, such as:
                //   always use first instance, or always use last instance.
                // however duplicate object names are most likely a
                // typo, so it's best to throw an error
                //
                // also its highly unlikely that the auto offset case
                // could safely deal with offsets for the
                // duplicate key case
                if(schema->has_child(entry_name))
                {
                    CONDUIT_ERROR("JSON Generator error:\n"
                                  << "Duplicate JSON object name: "
                                  << utils::join_path(node->path(),entry_name));
                }

                Schema *curr_schema = &schema->add_child(entry_name);

                Node *curr_node = new Node();
                curr_node->set_schema_ptr(curr_schema);
                curr_node->set_parent(node);
                node->append_node_ptr(curr_node);
                walk_json_schema_external(curr_node,
                                          curr_schema,
                                          data,
                                          itr->value,
                                          curr_offset);

                // auto offset only makes sense when we have data
                if(data != NULL)
                    curr_offset += curr_schema->total_strided_bytes();
            }

        }
    }
    // List case
    else if (jvalue.IsArray())
    {
        schema->set(DataType::list());

        for (conduit_json::SizeType i = 0; i < jvalue.Size(); i++)
        {
            schema->append();
            Schema *curr_schema = schema->child_ptr(i);
            Node *curr_node = new Node();
            curr_node->set_schema_ptr(curr_schema);
            curr_node->set_parent(node);
            node->append_node_ptr(curr_node);
            walk_json_schema_external(curr_node,
                                      curr_schema,
                                      data,
                                      jvalue[i],
                                      curr_offset);
            // auto offset only makes sense when we have data
            if(data != NULL)
                curr_offset += curr_schema->total_strided_bytes();
        }
    }
    // Simplest case, handles "uint32", "float64", with extended type info
    else if(jvalue.IsString())
    {
        DataType dtype;
        parse_leaf_dtype(jvalue,curr_offset,dtype);
        schema->set(dtype);

        if(data != NULL)
        {
             // node is already linked to the schema pointer
             node->set_data_ptr(data);
        }
        else
        {
             // node is already linked to the schema pointer
             // we need to dynamically alloc
             node->set(dtype);  // causes an init
        }
    }
    else
    {
        CONDUIT_ERROR("JSON Generator error:\n"
                      << "Invalid JSON type for parsing Node."
                      << " Expected: JSON Object, Array, or String");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_base64(Node *node,
                                      const conduit_json::Value &jvalue)
{
    // object case

    std::string base64_str = "";

    if(jvalue.IsObject())
    {
        Schema s;
        if (jvalue.HasMember("data") && jvalue["data"].HasMember("base64"))
        {
            base64_str = jvalue["data"]["base64"].GetString();
        }
        else
        {
            CONDUIT_ERROR("conduit_base64_json protocol error: missing data/base64");
        }

        if (jvalue.HasMember("schema"))
        {
            // parse schema
            index_t curr_offset = 0;
            walk_json_schema(&s,jvalue["schema"],curr_offset);
        }
        else
        {
            CONDUIT_ERROR("conduit_base64_json protocol error: missing schema");
        }

        const char *src_ptr = base64_str.c_str();
        index_t encoded_len = (index_t) base64_str.length();
        index_t dec_buff_size = utils::base64_decode_buffer_size(encoded_len);

        // decode buffer
        Node bb64_decode;
        bb64_decode.set(DataType::char8_str(dec_buff_size));
        char *decode_ptr = (char*)bb64_decode.data_ptr();
        memset(decode_ptr,0,dec_buff_size);

        utils::base64_decode(src_ptr,
                             encoded_len,
                             decode_ptr);

        node->set(s,decode_ptr);

    }
    else
    {
        CONDUIT_ERROR("conduit_base64_json protocol error: missing schema and data/base64");
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::JSON::parse_error_details(const std::string &json,
                                             const conduit_json::Document &document,
                                             std::ostream &os)
{
    // provide message with line + char from rapidjson parse error offset
    index_t doc_offset = (index_t)document.GetErrorOffset();
    std::string json_curr = json.substr(0,doc_offset);

    std::string curr = "";
    std::string next = " ";

    index_t doc_line   = 0;
    index_t doc_char   = 0;

    while(!next.empty())
    {
        utils::split_string(json_curr, "\n", curr, next);
        doc_char = curr.size();
        json_curr = next;
        if(!next.empty())
        {
            doc_line++;
        }
    }

    os << " parse error message:\n"
       << GetParseError_En(document.GetParseError()) << "\n"
       << " offset: "    << doc_offset << "\n"
       << " line: "      << doc_line << "\n"
       << " character: " << doc_char << "\n"
       << " json:\n"     << json << "\n";
}


//-----------------------------------------------------------------------------
// -- end conduit::Generator::Parser::JSON --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// -- begin conduit::Generator::YAML --
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// -- begin conduit::Generator::YAML::YAMLParserWrapper --
//-----------------------------------------------------------------------------


//---------------------------------------------------------------------------//
Generator::Parser::YAML::YAMLParserWrapper::YAMLParserWrapper()
: m_yaml_parser_is_valid(false),
  m_yaml_doc_is_valid(false)
{

}

//---------------------------------------------------------------------------//
Generator::Parser::YAML::YAMLParserWrapper::~YAMLParserWrapper()
{
    // cleanup!
    if(m_yaml_parser_is_valid)
    {
        yaml_parser_delete(&m_yaml_parser);
    }

    if(m_yaml_doc_is_valid)
    {
        yaml_document_delete(&m_yaml_doc);
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::YAMLParserWrapper::parse(const char *yaml_txt)
{
    // Initialize parser
    if(yaml_parser_initialize(&m_yaml_parser) == 0)
    {
        // error!
        CONDUIT_ERROR("yaml_parser_initialize failed");
    }
    else
    {
        m_yaml_parser_is_valid = true;
    }

    // set input
    yaml_parser_set_input_string(&m_yaml_parser,
                                 (const unsigned char*)yaml_txt,
                                 strlen(yaml_txt));

    // use parser to construct document
    if( yaml_parser_load(&m_yaml_parser, &m_yaml_doc) == 0 )
    {
        CONDUIT_YAML_PARSE_ERROR(&m_yaml_doc,
                                 &m_yaml_parser);
    }
    else
    {
        m_yaml_doc_is_valid = true;
    }
}

//---------------------------------------------------------------------------//
yaml_document_t *
Generator::Parser::YAML::YAMLParserWrapper::yaml_doc_ptr()
{
    yaml_document_t *res = NULL;

    if(m_yaml_doc_is_valid)
    {
        res = &m_yaml_doc;
    }

    return res;
}

//---------------------------------------------------------------------------//
yaml_node_t *
Generator::Parser::YAML::YAMLParserWrapper::yaml_doc_root_ptr()
{
    yaml_node_t *res = NULL;

    if(m_yaml_doc_is_valid)
    {
        res = yaml_document_get_root_node(&m_yaml_doc);
    }

    return res;
}

//-----------------------------------------------------------------------------
// -- end conduit::Generator::YAML::YAMLParserWrapper --
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
index_t
Generator::Parser::YAML::yaml_leaf_to_numeric_dtype(const char *txt_value)
{
    index_t res = DataType::EMPTY_ID;
    if(string_is_integer(txt_value))
    {
        res = DataType::INT64_ID;
    }
    else if(string_is_double(txt_value))
    {
        res = DataType::FLOAT64_ID;
    }
    //else, already inited to DataType::EMPTY_ID

    return res;
}

//---------------------------------------------------------------------------//
bool
Generator::Parser::YAML::check_yaml_is_number(const yaml_node_t *yaml_node)
{
    if (check_yaml_is_scalar_node(yaml_node))
    {
        const char *yaml_value_str = get_yaml_string(yaml_node);
        return string_is_integer(yaml_value_str) || string_is_double(yaml_value_str);
    }

    return false;
}

//---------------------------------------------------------------------------//
bool
Generator::Parser::YAML::check_yaml_is_int(const yaml_node_t *yaml_node)
{
    if (check_yaml_is_scalar_node(yaml_node))
    {
        return string_is_integer(get_yaml_string(yaml_node));
    }
    return false;
}

//---------------------------------------------------------------------------//
long
Generator::Parser::YAML::get_yaml_long(const yaml_node_t *yaml_node)
{
    return string_to_long(get_yaml_string(yaml_node));
}

//---------------------------------------------------------------------------//
unsigned long
Generator::Parser::YAML::get_yaml_unsigned_long(const yaml_node_t *yaml_node)
{
    return string_to_unsigned_long(get_yaml_string(yaml_node));
}

//---------------------------------------------------------------------------//
bool
Generator::Parser::YAML::check_yaml_is_scalar_node(const yaml_node_t *yaml_node)
{
    return yaml_node->type == YAML_SCALAR_NODE;
}

//---------------------------------------------------------------------------//
bool
Generator::Parser::YAML::check_yaml_is_sequence(const yaml_node_t *yaml_node)
{
    return yaml_node->type == YAML_SEQUENCE_NODE;
}

//---------------------------------------------------------------------------//
bool
Generator::Parser::YAML::check_yaml_is_mapping_node(const yaml_node_t *yaml_node)
{
    return yaml_node->type == YAML_MAPPING_NODE;
}

//---------------------------------------------------------------------------//
const char *
Generator::Parser::YAML::get_yaml_string(const yaml_node_t *yaml_node)
{
    const char *yaml_value_str = (const char*) yaml_node->data.scalar.value;
    CONDUIT_ASSERT(yaml_value_str, "YAML Generator error:\nInvalid yaml scalar value.");
    return yaml_value_str;
}

//---------------------------------------------------------------------------//
int
Generator::Parser::YAML::get_yaml_sequence_length(const yaml_node_t *yaml_node)
{
    // assumes yaml_node->type == YAML_SEQUENCE_NODE
    return yaml_node->data.sequence.items.top - yaml_node->data.sequence.items.start;
}

//---------------------------------------------------------------------------//
int
Generator::Parser::YAML::get_yaml_num_members(const yaml_node_t *yaml_node)
{
    // assumes yaml_node->type == YAML_MAPPING_NODE
    return yaml_node->data.mapping.pairs.top - yaml_node->data.mapping.pairs.start;
}

//---------------------------------------------------------------------------//
yaml_node_t*
Generator::Parser::YAML::fetch_yaml_node_from_list(yaml_document_t *yaml_doc,
                                                   const yaml_node_t *yaml_node,
                                                   const int index)
{
    return yaml_document_get_node(yaml_doc,
                                  yaml_node->data.sequence.items.start[index]);
}

//---------------------------------------------------------------------------//
yaml_node_t*
Generator::Parser::YAML::fetch_yaml_node_from_object_by_name(yaml_document_t *yaml_doc,
                                                             const yaml_node_t *yaml_node,
                                                             const std::string member_name)
{
    for (index_t cld_idx = 0; cld_idx < get_yaml_num_members(yaml_node); cld_idx ++)
    {
        yaml_node_pair_t *yaml_pair = yaml_node->data.mapping.pairs.start + cld_idx;
        CONDUIT_ASSERT(yaml_pair, "YAML Generator error:\nfailed to fetch mapping pair.");
        yaml_node_t *yaml_key = yaml_document_get_node(yaml_doc, yaml_pair->key);
        CONDUIT_ASSERT(yaml_key, "YAML Generator error:\nfailed to fetch mapping key.");
        CONDUIT_ASSERT(check_yaml_is_scalar_node(yaml_key),
                       "YAML Generator error:\nInvalid mapping key type.");
        const std::string entry_name(get_yaml_string(yaml_key));
        if (entry_name == member_name)
        {
            yaml_node_t *yaml_child = yaml_document_get_node(yaml_doc, yaml_pair->value);
            CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");
            return yaml_child;
        }
    }

    return nullptr;
}


//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::parse_inline_value(yaml_document_t *yaml_doc,
                                            const yaml_node_t *yaml_node,
                                            Node &node)
{
    if (check_yaml_is_sequence(yaml_node))
    {
        // we assume a "value" is a leaf or list of compatible leaves
        index_t seq_size  = -1;
        index_t hval_type = check_homogenous_yaml_numeric_sequence(node,
                                                                   yaml_doc,
                                                                   yaml_node,
                                                                   seq_size);

        CONDUIT_ASSERT((node.dtype().number_of_elements() >= get_yaml_sequence_length(yaml_node)),
                       "YAML Generator error:\n"
                        << "number of elements in YAML array is more"
                        << "than dtype can hold");

        if (hval_type == DataType::INT64_ID)
        {
            if (node.dtype().is_unsigned_integer())
            {
                // TODO: we can make this more efficient
                std::vector<uint64> vals;
                parse_yaml_array<uint64>(yaml_doc, yaml_node, vals, seq_size);
                switch (node.dtype().id())
                {
                    case DataType::UINT8_ID:
                        node.as_uint8_array().set(vals);
                        break;
                    case DataType::UINT16_ID:
                        node.as_uint16_array().set(vals);
                        break;
                    case DataType::UINT32_ID:
                        node.as_uint32_array().set(vals);
                        break;
                    case DataType::UINT64_ID:
                        node.as_uint64_array().set(vals);
                        break;
                    default:
                        CONDUIT_ERROR("YAML Generator error:\n"
                                       << "attempting to set non-numeric Node with"
                                       << " uint64 array");
                        break;
                }
            }
            else
            {
                // TODO: we can make this more efficient
                std::vector<int64> vals;
                parse_yaml_array<int64>(yaml_doc, yaml_node, vals, seq_size);
                switch (node.dtype().id())
                {
                    case DataType::INT8_ID:
                        node.as_int8_array().set(vals);
                        break;
                    case DataType::INT16_ID:
                        node.as_int16_array().set(vals);
                        break;
                    case DataType::INT32_ID:
                        node.as_int32_array().set(vals);
                        break;
                    case DataType::INT64_ID:
                        node.as_int64_array().set(vals);
                        break;
                    default:
                        CONDUIT_ERROR("YAML Generator error:\n"
                                       << "attempting to set non-numeric Node with"
                                       << " int64 array");
                        break;
                }
            }
        }
        else if(hval_type == DataType::FLOAT64_ID)
        {
            // TODO: we can make this more efficient
            std::vector<float64> vals;
            parse_yaml_array<float64>(yaml_doc, yaml_node, vals, seq_size);
            switch (node.dtype().id())
            {
                case DataType::FLOAT32_ID:
                    node.as_float32_array().set(vals);
                    break;
                case DataType::FLOAT64_ID:
                    node.as_float64_array().set(vals);
                    break;
                default:
                    CONDUIT_ERROR("YAML Generator error:\n"
                                   << "attempting to set non-numeric Node with"
                                   << " float64 array");
                    break;
            }
        }
        else if(hval_type == DataType::EMPTY_ID)
        {
            // we need to allow this case but do nothing.
            // for conduit_yaml cases, the node will
            // have the right data type
        }
        else
        {
            // Parsing Error, not homogenous
            CONDUIT_ERROR("YAML Generator error:\n"
                        << "a YAML array for value initialization"
                        << " is not homogenous");
        }
    }
    else
    {
        parse_inline_leaf(get_yaml_string(yaml_node), node);
    }
}

//---------------------------------------------------------------------------//
void *
Generator::Parser::YAML::parse_inline_address(const yaml_node_t *yaml_node)
{
    void * res = nullptr;
    if (check_yaml_is_scalar_node(yaml_node))
    {
        const std::string sval(get_yaml_string(yaml_node));
        res = utils::hex_string_to_value<void*>(sval);
    }
    // else if(jvalue.IsNumber())
    // {
    //     // TODO: FUTURE? ...
    // }
    else
    {
         CONDUIT_ERROR("YAML Generator error:\n"
                       << "inline address should be a string");
    }
    return res;
}

//---------------------------------------------------------------------------//
template <typename T>
void
Generator::Parser::YAML::parse_yaml_array(yaml_document_t *yaml_doc,
                                          const yaml_node_t *yaml_node,
                                          std::vector<T> &res,
                                          const int seq_size)
{
    static_assert(std::is_same<T, int64>::value ||
                  std::is_same<T, uint64>::value ||
                  std::is_same<T, float64>::value,
                  "Bad type provided.");

    res.resize(seq_size, 0);
    for (index_t cld_idx = 0; cld_idx < seq_size; cld_idx ++)
    {
        yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
        CONDUIT_ASSERT(yaml_child && check_yaml_is_scalar_node(yaml_child),
                       "YAML Generator error:\nInvalid array value.");
        const char *yaml_value_str = get_yaml_string(yaml_child);
        if (std::is_same<T, int64>::value)
        {
            res[cld_idx] = (T) string_to_long(yaml_value_str);
        }
        else if (std::is_same<T, uint64>::value)
        {
            res[cld_idx] = (T) string_to_unsigned_long(yaml_value_str);
        }
        else // b/c of our static assertion at the beginning we can assume float64
        {
            res[cld_idx] = (T) string_to_double(yaml_value_str);
        }
    }
}

//---------------------------------------------------------------------------//
template <typename T>
void
Generator::Parser::YAML::parse_yaml_array(yaml_document_t *yaml_doc,
                                          const yaml_node_t *yaml_node,
                                          DataArray<T> &res,
                                          const int seq_size)
{
    static_assert(std::is_same<T, int64>::value ||
                  std::is_same<T, uint64>::value ||
                  std::is_same<T, float64>::value,
                  "Bad type provided.");

    for (index_t cld_idx = 0; cld_idx < seq_size; cld_idx ++)
    {
        yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
        CONDUIT_ASSERT(yaml_child && check_yaml_is_scalar_node(yaml_child),
                       "YAML Generator error:\nInvalid array value.");
        const char *yaml_value_str = get_yaml_string(yaml_child);
        if (std::is_same<T, int64>::value)
        {
            res[cld_idx] = (T) string_to_long(yaml_value_str);
        }
        else if (std::is_same<T, uint64>::value)
        {
            res[cld_idx] = (T) string_to_unsigned_long(yaml_value_str);
        }
        else // b/c of our static assertion at the beginning we can assume float64
        {
            res[cld_idx] = (T) string_to_double(yaml_value_str);
        }
    }
}

//---------------------------------------------------------------------------//
index_t
Generator::Parser::YAML::check_homogenous_yaml_numeric_sequence(const Node &node,
                                                                yaml_document_t *yaml_doc,
                                                                const yaml_node_t *yaml_node,
                                                                index_t &seq_size)
{
    index_t res = DataType::EMPTY_ID;
    seq_size = -1;
    bool ok = true;
    index_t cld_idx;
    for (cld_idx = 0; cld_idx < get_yaml_sequence_length(yaml_node); cld_idx ++)
    {
        yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
        CONDUIT_ASSERT(yaml_child,
                       "YAML Generator error:\n"
                       << "Invalid sequence child at path: "
                       << node.path() << "[" << cld_idx << "]");

        // first make sure we only have yaml scalars
        if (check_yaml_is_scalar_node(yaml_child))
        {
            // check for integers, then widen to floats
            index_t child_dtype_id = yaml_leaf_to_numeric_dtype(get_yaml_string(yaml_child));
            if (child_dtype_id == DataType::EMPTY_ID)
            {
                ok = false;
                break;
            }
            else if (res == DataType::EMPTY_ID)
            {
                // good so far, promote to child's dtype
                res = child_dtype_id;
            }
            else if (res == DataType::INT64_ID && child_dtype_id == DataType::FLOAT64_ID)
            {
                // promote to float64
                res = DataType::FLOAT64_ID;
            }
        }
        else
        {
            ok = false;
            break;
        }
    }

    // if we are ok, seq_size is the final cld_idx
    if (ok)
    {
        seq_size = cld_idx;
    }
    else
    {
        res = DataType::EMPTY_ID;
    }

    return res;
}

//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::parse_inline_leaf(const char *yaml_txt,
                                           Node &node)
{
    if (string_is_integer(yaml_txt) || string_is_double(yaml_txt))
    {
        switch(node.dtype().id())
        {
            // signed ints
            case DataType::INT8_ID:
                node.set(static_cast<int8>(string_to_long(yaml_txt)));
                break;
            case DataType::INT16_ID:
                node.set(static_cast<int16>(string_to_long(yaml_txt)));
                break;
            case DataType::INT32_ID:
                node.set(static_cast<int32>(string_to_long(yaml_txt)));
                break;
            case DataType::INT64_ID:
                node.set(static_cast<int64>(string_to_long(yaml_txt)));
                break;
            // unsigned ints
            case DataType::UINT8_ID:
                node.set(static_cast<uint8>(string_to_unsigned_long(yaml_txt)));
                break;
            case DataType::UINT16_ID:
                node.set(static_cast<uint16>(string_to_unsigned_long(yaml_txt)));
                break;
            case DataType::UINT32_ID:
                node.set(static_cast<uint32>(string_to_unsigned_long(yaml_txt)));
                break;
            case DataType::UINT64_ID:
                node.set(static_cast<uint64>(string_to_unsigned_long(yaml_txt)));
                break;
            //floats
            case DataType::FLOAT32_ID:
                node.set(static_cast<float32>(string_to_double(yaml_txt)));
                break;
            case DataType::FLOAT64_ID:
                node.set(static_cast<float64>(string_to_double(yaml_txt)));
                break;
            default:
                // YAML type incompatible with numeric
                // only allow numeric to be assigned to a numeric type
                // throw parsing error if our inline values
                // don't match what we expected
                CONDUIT_ERROR("YAML Generator error:\n"
                              << "a YAML number can only be used as an inline"
                              << " value for a Conduit Numeric Node.");
                break;
        }
    }
    else if (string_is_empty(yaml_txt))
    {
        // empty data type
        node.reset();
    }
    else // general string case
    {
        if (node.dtype().id() == DataType::CHAR8_STR_ID)
        {
            node.set_char8_str(yaml_txt);
        }
        else
        {
             // YAML type incompatible with char8_str
             // only allow strings to be assigned to a char8_str type
             // throw parsing error if our inline values
             // don't match what we expected

            CONDUIT_ERROR("YAML Generator error:\n"
                           << "a YAML string can only be used as an inline"
                           << " value for a Conduit CHAR8_STR Node.");
        }
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::parse_yaml_inline_leaf(const char *yaml_txt,
                                                Node &node)
{
    if (string_is_integer(yaml_txt))
    {
        node.set(static_cast<int64>(string_to_long(yaml_txt)));
    }
    else if (string_is_double(yaml_txt))
    {
        node.set(static_cast<float64>(string_to_double(yaml_txt)));
    }
    else if (string_is_empty(yaml_txt))
    {
        node.reset();
    }
    else // general string case
    {
        node.set_char8_str(yaml_txt);
    }
}

//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::parse_leaf_dtype(yaml_document_t *yaml_doc,
                                          const yaml_node_t *yaml_node,
                                          index_t offset,
                                          DataType &dtype_res)
{
    if (check_yaml_is_scalar_node(yaml_node))
    {
        std::string dtype_name(get_yaml_string(yaml_node));
        index_t dtype_id = parse_leaf_dtype_name(dtype_name);
        index_t ele_size = DataType::default_bytes(dtype_id);
        dtype_res.set(dtype_id,
                      1,
                      offset,
                      ele_size,
                      ele_size,
                      Endianness::DEFAULT_ID);
    }
    else if (check_yaml_is_mapping_node(yaml_node))
    {
        yaml_node_t* dtype_node = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "dtype");
        CONDUIT_ASSERT(dtype_node && check_yaml_is_scalar_node(dtype_node),
                       "YAML Generator error:\n'dtype' must be a YAML string.")
        const std::string dtype_name(get_yaml_string(dtype_node));

        index_t length = 0;

        auto extract_uint64_member = [&](const std::string &member_name,
                                         index_t &value_to_change)
        {
            const yaml_node_t* value_node = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, member_name);
            if (value_node)
            {
                CONDUIT_ASSERT(check_yaml_is_number(value_node),
                               "YAML Generator error:\n"
                               << "'" << member_name << "' must be a number ");
                value_to_change = static_cast<uint64>(get_yaml_unsigned_long(value_node));
            }
        };

        const yaml_node_t* num_ele_node = fetch_yaml_node_from_object_by_name(yaml_doc,
                                                                              yaml_node,
                                                                              "number_of_elements");
        if (num_ele_node)
        {
            CONDUIT_ASSERT(check_yaml_is_number(num_ele_node),
                           "YAML Generator error:\n"
                           << "'number_of_elements' must be a number ");
            length = static_cast<uint64>(get_yaml_unsigned_long(num_ele_node));
        }
        //
        // TODO DEPRECATE and replace with the lambda up above
        // I don't think that here and in the JSON equivalent are the only
        // two places where length needs to be deprecated.
        //
        // length is the old schema style, we should deprecate this path
        else
        {
            const yaml_node_t* length_node = fetch_yaml_node_from_object_by_name(yaml_doc,
                                                                                 yaml_node,
                                                                                 "length");
            if (length_node)
            {
                CONDUIT_ASSERT(check_yaml_is_number(length_node),
                               "YAML Generator error:\n"
                               << "'length' must be a number ");
                length = static_cast<uint64>(get_yaml_unsigned_long(length_node));
            }
        }

        index_t dtype_id = parse_leaf_dtype_name(dtype_name);
        index_t ele_size = DataType::default_bytes(dtype_id);
        index_t stride = ele_size;

        //  parse offset (override default if passed)
        extract_uint64_member("offset", offset);

        // parse stride (override default if passed)
        extract_uint64_member("stride", stride);

        // parse element_bytes (override default if passed)
        extract_uint64_member("element_bytes", ele_size);

        // parse endianness (override default if passed)
        index_t endianness = Endianness::DEFAULT_ID;
        const yaml_node_t* endianness_node = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "endianness");
        if (endianness_node)
        {
            if (check_yaml_is_scalar_node(endianness_node))
            {
                const std::string end_val(get_yaml_string(endianness_node));
                if ("big" == end_val)
                {
                    endianness = Endianness::BIG_ID;
                }
                else if ("little" == end_val)
                {
                    endianness = Endianness::LITTLE_ID;
                }
                else
                {
                    CONDUIT_ERROR("YAML Generator error:\n"
                              << "'endianness' must be a string"
                              << " (\"big\" or \"little\")"
                              << " parsed value: " << end_val);
                }
            }
            else
            {
                CONDUIT_ERROR("YAML Generator error:\n"
                          << "'endianness' must be a string"
                          << " (\"big\" or \"little\")");
            }
        }

        if (0 == length)
        {
            const yaml_node_t* value_node = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "value");
            if (value_node && check_yaml_is_sequence(value_node))
            {
                length = get_yaml_sequence_length(value_node);
            }
            // support explicit length 0 in a schema
            else if (! fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "number_of_elements"))
            {
                length = 1;
            }
        }

        dtype_res.set(dtype_id,
                      length,
                      offset,
                      stride,
                      ele_size,
                      endianness);
    }
    else
    {
        CONDUIT_ERROR("YAML Generator error:\n"
                       << "a leaf dtype entry must be a YAML string or"
                       <<  " YAML object.");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::walk_yaml_schema(Node *node,
                                          Schema *schema,
                                          void *data,
                                          yaml_document_t *yaml_doc,
                                          const yaml_node_t *yaml_node,
                                          index_t curr_offset,
                                          const bool external)
{
    // object cases
    if (check_yaml_is_mapping_node(yaml_node))
    {
        // if dtype is an object, we have a "list_of" case
        const yaml_node_t *dt_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "dtype");
        if (dt_value) // if yaml has dtype
        {
            // if dtype yaml is object type
            if (check_yaml_is_mapping_node(dt_value))
            {
                index_t length = 1;
                const yaml_node_t *len_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "length");
                if (len_value) // if dtype has member length
                {
                    if (check_yaml_is_number(len_value))
                    {
                        length = static_cast<index_t>(get_yaml_long(len_value));
                    }
                    else if (check_yaml_is_mapping_node(len_value))
                    {
                        const yaml_node_t *ref_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "reference");
                        if (ref_value)
                        {
                            const std::string ref_path = get_yaml_string(ref_value);
                            length = node->fetch(ref_path).to_index_t();
                        }
                        else
                        {
                            CONDUIT_ERROR("YAML Generator error:\n"
                                          << "'length' must be a number or"
                                          << " reference");
                        }
                    }
                    else
                    {
                        CONDUIT_ERROR("YAML Generator error:\n"
                                      << "'length' must be a number or"
                                      << " reference");
                    }
                }
                // we will create `length' # of objects of obj des by dt_value

                // TODO: we only need to parse this once, not leng # of times
                // but this is the easiest way to start.
                for (index_t i = 0; i < length; i++)
                {
                    schema->append();
                    Schema *curr_schema = schema->child_ptr(i);
                    Node *curr_node = new Node();
                    curr_node->set_schema_ptr(curr_schema);
                    curr_node->set_parent(node);
                    node->append_node_ptr(curr_node);
                    walk_yaml_schema(curr_node,
                                     curr_schema,
                                     data,
                                     yaml_doc,
                                     dt_value,
                                     curr_offset,
                                     external);
                    // auto offset only makes sense when we have data
                    if (data)
                    {
                        curr_offset += curr_schema->total_strided_bytes();
                    }
                }
            }
            else
            {
                // handle leaf node with explicit props
                DataType src_dtype;
                parse_leaf_dtype(yaml_doc, yaml_node, curr_offset, src_dtype);

                DataType des_dtype;
                src_dtype.compact_to(des_dtype);

                // check for explicit address
                const yaml_node_t* address_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "address");
                if (address_value)
                {
                    void *data_ptr = parse_inline_address(address_value);
                    node->set_external(src_dtype, data_ptr);
                }
                else
                {
                    if (data)
                    {
                        if (external) // handle conduit_yaml_external case
                        {
                            // node is already linked to the schema pointer
                            schema->set(des_dtype);
                            node->set_data_ptr(data);
                        }
                        else
                        {
                            uint8 *src_data_ptr = ((uint8*)data) + src_dtype.offset();
                            // node is already linked to the schema pointer
                            // we need to dynamically alloc, use compact dtype
                            node->set(des_dtype); // causes an init
                            // copy bytes from src data to node's memory
                            utils::conduit_memcpy_strided_elements(node->data_ptr(),               // dest data
                                                                   des_dtype.number_of_elements(), // num ele
                                                                   des_dtype.element_bytes(),      // ele bytes
                                                                   des_dtype.stride(),             // dest stride
                                                                   src_data_ptr,                   // src data
                                                                   src_dtype.stride());            // src stride
                        }
                    }
                    else
                    {
                        // node is already linked to the schema pointer
                        // we need to dynamically alloc, use compact dtype
                        node->set(des_dtype); // causes an init
                    }

                    // check for inline yaml values
                    const yaml_node_t *value_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "value");
                    if (value_value)
                    {
                        parse_inline_value(yaml_doc, value_value, *node);
                    }
                }
            }
        }
        else // object case
        {
            schema->set(DataType::object());
            // standard object case - loop over all entries
            for (index_t cld_idx = 0; cld_idx < get_yaml_num_members(yaml_node); cld_idx ++)
            {
                yaml_node_pair_t *yaml_pair = yaml_node->data.mapping.pairs.start + cld_idx;
                CONDUIT_ASSERT(yaml_pair, "YAML Generator error:\nfailed to fetch mapping pair.");
                yaml_node_t *yaml_key = yaml_document_get_node(yaml_doc, yaml_pair->key);
                CONDUIT_ASSERT(yaml_key, "YAML Generator error:\nfailed to fetch mapping key.");
                CONDUIT_ASSERT(check_yaml_is_scalar_node(yaml_key),
                               "YAML Generator error:\nInvalid mapping key type.");
                const std::string entry_name(get_yaml_string(yaml_key));

                // yaml files may have duplicate object names
                // we could provide some clear semantics, such as:
                //   always use first instance, or always use last instance
                // however duplicate object names are most likely a
                // typo, so it's best to throw an error
                //
                // also its highly unlikely that the auto offset case
                // could safely deal with offsets for the
                // duplicate key case
                CONDUIT_ASSERT(! schema->has_child(entry_name),
                               "YAML Generator error:\n"
                               << "Duplicate YAML object name: "
                               << utils::join_path(node->path(),entry_name));

                yaml_node_t *yaml_child = yaml_document_get_node(yaml_doc, yaml_pair->value);
                CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

                Schema *curr_schema = &schema->add_child(entry_name);
                Node *curr_node = new Node();
                curr_node->set_schema_ptr(curr_schema);
                curr_node->set_parent(node);
                node->append_node_ptr(curr_node);
                walk_yaml_schema(curr_node,
                                 curr_schema,
                                 data,
                                 yaml_doc,
                                 yaml_child,
                                 curr_offset,
                                 external);

                // auto offset only makes sense when we have data
                if (data)
                {
                    curr_offset += curr_schema->total_strided_bytes();
                }
            }
        }
    }
    // List case
    else if (check_yaml_is_sequence(yaml_node))
    {
        schema->set(DataType::list());

        for (index_t cld_idx = 0; cld_idx < get_yaml_sequence_length(yaml_node); cld_idx ++)
        {
            yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
            CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

            schema->append();
            Schema *curr_schema = schema->child_ptr(cld_idx);
            Node *curr_node = new Node();
            curr_node->set_schema_ptr(curr_schema);
            curr_node->set_parent(node);
            node->append_node_ptr(curr_node);
            walk_yaml_schema(curr_node,
                             curr_schema,
                             data,
                             yaml_doc,
                             yaml_child,
                             curr_offset,
                             external);
            // auto offset only makes sense when we have data
            if (data)
            {
                curr_offset += curr_schema->total_strided_bytes();
            }
        }
    }
    // Simplest case, handles "uint32", "float64", with extended type info
    else if (check_yaml_is_scalar_node(yaml_node))
    {
        DataType dtype;
        parse_leaf_dtype(yaml_doc, yaml_node, curr_offset, dtype);
        schema->set(dtype);

        if (data)
        {
            // node is already linked to the schema pointer
            node->set_data_ptr(data);
        }
        else
        {
            // node is already linked to the schema pointer
            // we need to dynamically alloc
            node->set(dtype);  // causes an init
        }
    }
    else
    {
        CONDUIT_ERROR("YAML Generator error:\n"
                      << "Invalid YAML type for parsing Node."
                      << " Expected: YAML Object, Array, or String");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::walk_yaml_schema(Schema *schema,
                                          yaml_document_t *yaml_doc,
                                          const yaml_node_t *yaml_node,
                                          index_t curr_offset)
{
    // object cases
    if (check_yaml_is_mapping_node(yaml_node))
    {
        const yaml_node_t* dt_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "dtype");
        if (dt_value) // if yaml has dtype
        {
            // if dtype is an object, we have a "list_of" case
            if (check_yaml_is_mapping_node(dt_value)) // if dtype yaml is object type
            {
                int length = 1;
                const yaml_node_t *len_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "length");
                if (len_value) // if dtype has member length
                {
                    if (check_yaml_is_mapping_node(len_value) &&
                        fetch_yaml_node_from_object_by_name(yaml_doc, len_value, "reference"))
                    {
                        CONDUIT_ERROR("YAML Generator error:\n"
                                      << "'reference' option is not supported"
                                      << " when parsing to a Schema because"
                                      << " reference data does not exist.");
                    }
                    else if (check_yaml_is_int(len_value))
                    {
                        length = static_cast<int>(get_yaml_long(len_value));
                    }
                    else
                    {
                        CONDUIT_ERROR("YAML Generator error:\n"
                                      << "'length' must be a YAML Object or"
                                      << " YAML number");
                    }
                }
                // we will create `length' # of objects of obj des by dt_value

                // TODO: we only need to parse this once, not leng # of times
                // but this is the easiest way to start.
                for (int i = 0; i < length; i ++)
                {
                    Schema &curr_schema = schema->append();
                    curr_schema.set(DataType::list());
                    walk_yaml_schema(&curr_schema,
                                     yaml_doc,
                                     dt_value,
                                     curr_offset);
                    curr_offset += curr_schema.total_strided_bytes();
                }
            }
            else
            {
                // handle leaf node with explicit props
                DataType dtype;
                parse_leaf_dtype(yaml_doc, yaml_node, curr_offset, dtype);
                schema->set(dtype);
            }
        }
        else
        {
            // if we make it here and have an empty yaml object
            // we still want the conduit schema to take on the
            // object role
            schema->set(DataType::object());

            // loop over all entries
            for (index_t cld_idx = 0; cld_idx < get_yaml_num_members(yaml_node); cld_idx ++)
            {
                yaml_node_pair_t *yaml_pair = yaml_node->data.mapping.pairs.start + cld_idx;
                CONDUIT_ASSERT(yaml_pair, "YAML Generator error:\nfailed to fetch mapping pair.");
                yaml_node_t *yaml_key = yaml_document_get_node(yaml_doc, yaml_pair->key);
                CONDUIT_ASSERT(yaml_key, "YAML Generator error:\nfailed to fetch mapping key.");
                CONDUIT_ASSERT(check_yaml_is_scalar_node(yaml_key),
                               "YAML Generator error:\nInvalid mapping key type.");
                const std::string entry_name(get_yaml_string(yaml_key));

                // yaml files may have duplicate object names
                // we could provide some clear semantics, such as:
                //   always use first instance, or always use last instance
                // however duplicate object names are most likely a
                // typo, so it's best to throw an error
                CONDUIT_ASSERT(! schema->has_child(entry_name),
                               "YAML Generator error:\n"
                               << "Duplicate YAML object name: "
                               << entry_name);

                Schema &curr_schema = schema->add_child(entry_name);
                curr_schema.set(DataType::object());

                yaml_node_t *yaml_child = yaml_document_get_node(yaml_doc, yaml_pair->value);
                CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

                walk_yaml_schema(&curr_schema,
                                 yaml_doc,
                                 yaml_child,
                                 curr_offset);
                curr_offset += curr_schema.total_strided_bytes();
            }
        }
    }
    // List case
    else if (check_yaml_is_sequence(yaml_node))
    {
        // if we make it here and have an empty yaml list
        // we still want the conduit schema to take on the
        // list role
        schema->set(DataType::list());

        for (index_t cld_idx = 0; cld_idx < get_yaml_sequence_length(yaml_node); cld_idx ++)
        {
            yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
            CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

            Schema &curr_schema = schema->append();
            curr_schema.set(DataType::list());
            walk_yaml_schema(&curr_schema, yaml_doc, yaml_child, curr_offset);
            curr_offset += curr_schema.total_strided_bytes();
        }
    }
    // Simplest case, handles "uint32", "float64", etc
    else if (check_yaml_is_scalar_node(yaml_node))
    {
        DataType dtype;
        parse_leaf_dtype(yaml_doc, yaml_node, curr_offset, dtype);
        schema->set(dtype);
    }
    else
    {
        CONDUIT_ERROR("YAML Generator error:\n"
                      << "Invalid YAML type for parsing Schema."
                      << " Expected: YAML Map, Sequence, or String");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::walk_pure_yaml_schema(Node *node,
                                               Schema *schema,
                                               yaml_document_t *yaml_doc,
                                               yaml_node_t *yaml_node)
{

    // object cases
    if (check_yaml_is_mapping_node(yaml_node))
    {
        // if we make it here and have an empty yaml object
        // we still want the conduit node to take on the
        // object role
        schema->set(DataType::object());
        // loop over all entries
        for (index_t cld_idx = 0; cld_idx < get_yaml_num_members(yaml_node); cld_idx ++)
        {
            yaml_node_pair_t *yaml_pair = yaml_node->data.mapping.pairs.start + cld_idx;
            CONDUIT_ASSERT(yaml_pair, "YAML Generator error:\nfailed to fetch mapping pair.");
            yaml_node_t *yaml_key = yaml_document_get_node(yaml_doc, yaml_pair->key);
            CONDUIT_ASSERT(yaml_key, "YAML Generator error:\nfailed to fetch mapping key.");
            CONDUIT_ASSERT(check_yaml_is_scalar_node(yaml_key),
                           "YAML Generator error:\nInvalid mapping key type.");
            const std::string entry_name(get_yaml_string(yaml_key));
            yaml_node_t *yaml_child = yaml_document_get_node(yaml_doc, yaml_pair->value);
            CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

            // yaml files may have duplicate object names
            // we could provide some clear semantics, such as:
            //   always use first instance, or always use last instance
            // however duplicate object names are most likely a
            // typo, so it's best to throw an error

            CONDUIT_ASSERT(! schema->has_child(entry_name),
                           "YAML Generator error:\n"
                           << "Duplicate YAML object name: "
                           << utils::join_path(node->path(),entry_name));

            Schema *curr_schema = &schema->add_child(entry_name);

            Node *curr_node = new Node();
            curr_node->set_schema_ptr(curr_schema);
            curr_node->set_parent(node);
            node->append_node_ptr(curr_node);

            walk_pure_yaml_schema(curr_node,
                                  curr_schema,
                                  yaml_doc,
                                  yaml_child);
        }
    }
    // List case
    else if (check_yaml_is_sequence(yaml_node))
    {
        index_t seq_size  = -1;
        index_t hval_type = check_homogenous_yaml_numeric_sequence(*node,
                                                                   yaml_doc,
                                                                   yaml_node,
                                                                   seq_size);

        if (hval_type == DataType::INT64_ID)
        {
            if (node->dtype().is_unsigned_integer())
            {
                node->set(DataType::uint64(seq_size));
                uint64_array vals = node->value();
                parse_yaml_array<uint64>(yaml_doc, yaml_node, vals, seq_size);
            }
            else
            {
                node->set(DataType::int64(seq_size));
                int64_array vals = node->value();
                parse_yaml_array<int64>(yaml_doc, yaml_node, vals, seq_size);
            }
        }
        else if(hval_type == DataType::FLOAT64_ID)
        {
            node->set(DataType::float64(seq_size));
            float64_array vals = node->value();
            parse_yaml_array<float64>(yaml_doc, yaml_node, vals, seq_size);
        }
        else
        {
            // general case (not a numeric array)
            for (index_t cld_idx = 0; cld_idx < get_yaml_sequence_length(yaml_node); cld_idx ++)
            {
                yaml_node_t *yaml_child = fetch_yaml_node_from_list(yaml_doc, yaml_node, cld_idx);
                CONDUIT_ASSERT(yaml_child, "YAML Generator error:\nInvalid mapping child.");

                schema->append();
                Schema *curr_schema = schema->child_ptr(cld_idx);
                Node * curr_node = new Node();
                curr_node->set_schema_ptr(curr_schema);
                curr_node->set_parent(node);
                node->append_node_ptr(curr_node);
                walk_pure_yaml_schema(curr_node,
                                      curr_schema,
                                      yaml_doc,
                                      yaml_child);
            }
        }
    }
    else if (check_yaml_is_scalar_node(yaml_node))// bytestr case
    {
        parse_yaml_inline_leaf(get_yaml_string(yaml_node),*node);
    }
    else // this will include unknown enum vals and YAML_NO_NODE
    {
        // not sure if can an even land here, but catch error just in case.
        CONDUIT_ERROR("YAML Generator error:\n"
                      << "Invalid YAML type for parsing Node from pure YAML."
                      << " Expected: YAML Map, Sequence, String, Null,"
                      << " Boolean, or Number");
    }
}


//---------------------------------------------------------------------------//
void
Generator::Parser::YAML::parse_base64(Node *node,
                                      yaml_document_t *yaml_doc,
                                      yaml_node_t *yaml_node)
{
    // object case

    std::string base64_str = "";

    if(check_yaml_is_mapping_node(yaml_node))
    {
        Schema s;

        const yaml_node_t *data_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "data");
        if (data_value) // if yaml has data
        {
            const yaml_node_t *base64_value = fetch_yaml_node_from_object_by_name(yaml_doc, data_value, "base64");
            if (base64_value) // if yaml has base64
            {
                base64_str = get_yaml_string(base64_value);
            }
            else
            {
                CONDUIT_ERROR("conduit_base64_yaml protocol error: missing data/base64");
            }
        }
        else
        {
            CONDUIT_ERROR("conduit_base64_yaml protocol error: missing data/base64");
        }

        const yaml_node_t *schema_value = fetch_yaml_node_from_object_by_name(yaml_doc, yaml_node, "schema");
        if (schema_value) // if yaml has schema
        {
            // parse schema
            index_t curr_offset = 0;
            walk_yaml_schema(&s, yaml_doc, schema_value, curr_offset);
        }
        else
        {
            CONDUIT_ERROR("conduit_base64_yaml protocol error: missing schema");
        }

        const char *src_ptr = base64_str.c_str();
        index_t encoded_len = (index_t) base64_str.length();
        index_t dec_buff_size = utils::base64_decode_buffer_size(encoded_len);

        // decode buffer
        Node bb64_decode;
        bb64_decode.set(DataType::char8_str(dec_buff_size));
        char *decode_ptr = (char*)bb64_decode.data_ptr();
        memset(decode_ptr,0,dec_buff_size);

        utils::base64_decode(src_ptr,
                             encoded_len,
                             decode_ptr);

        node->set(s,decode_ptr);

    }
    else
    {
        CONDUIT_ERROR("conduit_base64_yaml protocol error: missing schema and data/base64");
    }
}


//-----------------------------------------------------------------------------
void
Generator::Parser::YAML::parse_error_details(yaml_parser_t *yaml_parser,
                                             std::ostream &os)
{
    os << "YAML Parsing Error (";
    switch (yaml_parser->error)
    {
        case YAML_NO_ERROR:
            os << "YAML_NO_ERROR";
            break;
        case YAML_MEMORY_ERROR:
            os << "YAML_MEMORY_ERROR";
            break;
        case YAML_READER_ERROR:
            os << "YAML_MEMORY_ERROR";
            break;
        case YAML_SCANNER_ERROR:
            os << "YAML_SCANNER_ERROR";
            break;
        case YAML_PARSER_ERROR:
            os << "YAML_PARSER_ERROR";
            break;
        case YAML_COMPOSER_ERROR:
            os << "YAML_COMPOSER_ERROR";
            break;
        case YAML_WRITER_ERROR:
            os << "YAML_WRITER_ERROR";
            break;
        case YAML_EMITTER_ERROR:
            os << "YAML_EMITTER_ERROR";
            break;
        default:
            os << "[Unknown Error!]";
            break;
    }

    // Q: Is yaml_parser->problem_mark.index useful here?
    //    that might be the only case where we need the yaml_doc
    //    otherwise using yaml_parser is sufficient

    if(yaml_parser->problem != NULL)
    {
        os << ")\n Problem:\n" << yaml_parser->problem << "\n"
           << "  Problem Line: "   << yaml_parser->problem_mark.line << "\n"
           << "  Problem Column: " << yaml_parser->problem_mark.column << "\n";
    }
    else
    {
        os << "unexpected: yaml_parser->problem is NULL (missing)\n";
    }
    if(yaml_parser->context != NULL)
    {
       os << " Context\n"         << yaml_parser->context << "\n"
          << "  Context Line: "   << yaml_parser->context_mark.line << "\n"
          << "  Context Column: " << yaml_parser->context_mark.column<< "\n";
    }
    os << std::endl;
}

//-----------------------------------------------------------------------------
// -- end conduit::Generator::Parser::YAML --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Generator Construction and Destruction
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
Generator::Generator()
:m_schema(""),
 m_protocol("conduit_json"),
 m_data(NULL)
{}


//---------------------------------------------------------------------------//
Generator::Generator(const std::string &schema,
                     const std::string &protocol,
                     void *data)
:m_schema(schema),
 m_protocol(protocol),
 m_data(data)
{}

//---------------------------------------------------------------------------//
void
Generator::set_schema(const std::string &schema)
{
    m_schema = schema;
}

//---------------------------------------------------------------------------//
void
Generator::set_protocol(const std::string &protocol)
{
    m_protocol = protocol;
}

//---------------------------------------------------------------------------//
void
Generator::set_data_ptr(void *data_ptr)
{
    m_data = data_ptr;
}

//---------------------------------------------------------------------------//
const std::string &
Generator::schema() const
{
    return m_schema;
}

//---------------------------------------------------------------------------//
const std::string &
Generator::protocol() const
{
    return m_protocol;
}

//---------------------------------------------------------------------------//
void *
Generator::data_ptr() const
{
    return m_data;
}


//-----------------------------------------------------------------------------
// JSON & YAML Parsing interface
//-----------------------------------------------------------------------------s


//---------------------------------------------------------------------------//
void
Generator::walk(Schema &schema) const
{
    schema.reset();
    index_t curr_offset = 0;

    auto json_case = [&]()
    {
        conduit_json::Document document;
        std::string res = utils::json_sanitize(m_schema);

        if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
        {
            CONDUIT_JSON_PARSE_ERROR(res, document);
        }

        Parser::JSON::walk_json_schema(&schema,document,curr_offset);
    };
    auto yaml_case = [&]()
    {
        Parser::YAML::YAMLParserWrapper parser;
        parser.parse(m_schema.c_str());

        yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
        yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

        CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

        Parser::YAML::walk_yaml_schema(&schema,
                                       yaml_doc,
                                       yaml_node,
                                       curr_offset);

        // YAMLParserWrapper cleans up for us
    };

    std::string error_message;
    bool schema_walked = false;

    // The protocol is default initialized to json. We choose to prefer
    // walking based on the protocol but we can fall back if possible.
    if (m_protocol.find("json") != std::string::npos)
    {
        try
        {
            json_case();
            schema_walked = true;
        }
        catch (conduit::Error &e_json)
        {
            error_message = "JSON schema walk failed: " + e_json.message();
            try
            {
                yaml_case();
                schema_walked = true;
            }
            catch(conduit::Error &e_yaml)
            {
                error_message += "\nYAML schema walk failed: " + e_yaml.message();
            }
        }

        if (! schema_walked)
        {
            CONDUIT_ERROR(error_message);
        }
    }
    else if (m_protocol.find("yaml") != std::string::npos)
    {
        try
        {
            yaml_case();
            schema_walked = true;
        }
        catch (conduit::Error &e_yaml)
        {
            error_message = "YAML schema walk failed: " + e_yaml.message();
            try
            {
                json_case();
                schema_walked = true;
            }
            catch(conduit::Error &e_json)
            {
                error_message += "\nJSON schema walk failed: " + e_json.message();
            }
        }

        if (! schema_walked)
        {
            CONDUIT_ERROR(error_message);
        }
    }
    else
    {
        CONDUIT_ERROR("Unknown protocol in " << m_protocol);
    }
}

//---------------------------------------------------------------------------//
void
Generator::walk(Node &node) const
{
    // try catch b/c:
    // if something goes wrong we will clear the node and re-throw
    // if exception is caught downstream
    // we want node to be empty instead of partially inited
    try
    {
        node.reset();
        // json, yaml, and conduit_base64_json don't leverage "data"
        if(m_protocol == "json")
        {
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }

            Parser::JSON::walk_pure_json_schema(&node,
                                                node.schema_ptr(),
                                                document);
        }
        else if(m_protocol == "yaml")
        {
            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            // errors will flow up from this call
            Parser::YAML::walk_pure_yaml_schema(&node,
                                                node.schema_ptr(),
                                                yaml_doc,
                                                yaml_node);
        }
        else if( m_protocol == "conduit_base64_json")
        {
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }

            Parser::JSON::parse_base64(&node,
                                       document);
        }

        else if( m_protocol == "conduit_base64_yaml")
        {
            // YAMLParserWrapper cleans up for us
            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            Parser::YAML::parse_base64(&node,
                                       yaml_doc,
                                       yaml_node);
        }
        else if( m_protocol == "conduit_json" || m_protocol == "conduit_json_external")
        {
            // Note: conduit_json_external if case here for symmetry with gen / read options
            // this case is fully handled by conduit_json logic
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }
            index_t curr_offset = 0;

            Parser::JSON::walk_json_schema(&node,
                                           node.schema_ptr(),
                                           m_data,
                                           document,
                                           curr_offset);
        }
        else if( m_protocol == "conduit_yaml" || m_protocol == "conduit_yaml_external")
        {
            index_t curr_offset = 0;
            const bool external = false;

            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            Parser::YAML::walk_yaml_schema(&node,
                                           node.schema_ptr(),
                                           m_data,
                                           yaml_doc,
                                           yaml_node,
                                           curr_offset,
                                           external);

            // YAMLParserWrapper cleans up for us
        }
        else
        {
            CONDUIT_ERROR("Generator unknown parsing protocol: " << m_protocol);
        }
    }
    catch(const conduit::Error& e)
    {
        node.reset();
        throw e;
    }
}

//---------------------------------------------------------------------------//
void
Generator::walk_external(Node &node) const
{
    // try catch b/c:
    // if something goes wrong we will clear the node and re-throw
    // if exception is caught downstream
    // we want node to be empty instead of partially inited
    try
    {
        node.reset();
        // if data is null, we can parse the schema via other 'walk' methods
        if(m_protocol == "json")
        {
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }

            Parser::JSON::walk_pure_json_schema(&node,
                                                node.schema_ptr(),
                                                document);
        }
        else if(m_protocol == "yaml")
        {
            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            // errors will flow up from this call
            Parser::YAML::walk_pure_yaml_schema(&node,
                                                node.schema_ptr(),
                                                yaml_doc,
                                                yaml_node);

            // YAMLParserWrapper cleans up for us
        }
        else if( m_protocol == "conduit_base64_json")
        {
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }

            Parser::JSON::parse_base64(&node,
                                       document);
        }
        else if( m_protocol == "conduit_base64_yaml")
        {
            // YAMLParserWrapper cleans up for us
            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            Parser::YAML::parse_base64(&node,
                                       yaml_doc,
                                       yaml_node);
        }
        else if( m_protocol == "conduit_json" || m_protocol == "conduit_json_external")
        {
            // Note: conduit_json_external if case here for symmetry with gen / read options
            // this case is fully handled by conduit_json logic
            conduit_json::Document document;
            std::string res = utils::json_sanitize(m_schema);

            if(document.Parse<Parser::JSON::JSON_PARSE_OPTS>(res.c_str()).HasParseError())
            {
                CONDUIT_JSON_PARSE_ERROR(res, document);
            }
            index_t curr_offset = 0;

            Parser::JSON::walk_json_schema_external(&node,
                                                    node.schema_ptr(),
                                                    m_data,
                                                    document,
                                                    curr_offset);
        }
        else if( m_protocol == "conduit_yaml" || m_protocol == "conduit_yaml_external")
        {
            index_t curr_offset = 0;
            const bool external = true;

            Parser::YAML::YAMLParserWrapper parser;
            parser.parse(m_schema.c_str());

            yaml_document_t *yaml_doc  = parser.yaml_doc_ptr();
            yaml_node_t     *yaml_node = parser.yaml_doc_root_ptr();

            CONDUIT_ASSERT(yaml_doc && yaml_node,"failed to fetch yaml document root");

            Parser::YAML::walk_yaml_schema(&node,
                                           node.schema_ptr(),
                                           m_data,
                                           yaml_doc,
                                           yaml_node,
                                           curr_offset,
                                           external);

            // YAMLParserWrapper cleans up for us
        }
        else
        {
            CONDUIT_ERROR("Generator unknown parsing protocol: " << m_protocol);
        }
    }
    catch(const conduit::Error& e)
    {
        node.reset();
        throw e;
    }
}


//-----------------------------------------------------------------------------
// -- end conduit::Generator --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

