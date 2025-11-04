// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_node_to_array.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;

// TODO(JRC): Figure out if there's any better way to facilitate these conversions
// from C++ object functions to C-Style functions for better procedural testing.

typedef void (*NodeConvertFun)(const Node&, Node&);

void convert_to_int8_array(const Node &n, Node &res) { n.to_int8_array(res); }
void convert_to_int16_array(const Node &n, Node &res) { n.to_int16_array(res); }
void convert_to_int32_array(const Node &n, Node &res) { n.to_int32_array(res); }
void convert_to_int64_array(const Node &n, Node &res) { n.to_int64_array(res); }
void convert_to_uint8_array(const Node &n, Node &res) { n.to_uint8_array(res); }
void convert_to_uint16_array(const Node &n, Node &res) { n.to_uint16_array(res); }
void convert_to_uint32_array(const Node &n, Node &res) { n.to_uint32_array(res); }
void convert_to_uint64_array(const Node &n, Node &res) { n.to_uint64_array(res); }
void convert_to_float32_array(const Node &n, Node &res) { n.to_float32_array(res); }
void convert_to_float64_array(const Node &n, Node &res) { n.to_float64_array(res); }
void convert_to_char_array(const Node &n, Node &res) { n.to_char_array(res); }
void convert_to_short_array(const Node &n, Node &res) { n.to_short_array(res); }
void convert_to_int_array(const Node &n, Node &res) { n.to_int_array(res); }
void convert_to_long_array(const Node &n, Node &res) { n.to_long_array(res); }
void convert_to_unsigned_char_array(const Node &n, Node &res) { n.to_unsigned_char_array(res); }
void convert_to_unsigned_short_array(const Node &n, Node &res) { n.to_unsigned_short_array(res); }
void convert_to_unsigned_int_array(const Node &n, Node &res) { n.to_unsigned_int_array(res); }
void convert_to_unsigned_long_array(const Node &n, Node &res) { n.to_unsigned_long_array(res); }
void convert_to_float_array(const Node &n, Node &res) { n.to_float_array(res); }
void convert_to_double_array(const Node &n, Node &res) { n.to_double_array(res); }

const int CONVERT_TYPES[20] = {
    DataType::INT8_ID, DataType::INT16_ID, DataType::INT32_ID, DataType::INT64_ID,
    DataType::UINT8_ID, DataType::UINT16_ID, DataType::UINT32_ID, DataType::UINT64_ID,
    DataType::FLOAT32_ID, DataType::FLOAT64_ID,
    CONDUIT_NATIVE_CHAR_ID, CONDUIT_NATIVE_SHORT_ID, CONDUIT_NATIVE_INT_ID, CONDUIT_NATIVE_LONG_ID,
    CONDUIT_NATIVE_UNSIGNED_CHAR_ID, CONDUIT_NATIVE_UNSIGNED_SHORT_ID, CONDUIT_NATIVE_UNSIGNED_INT_ID, CONDUIT_NATIVE_UNSIGNED_LONG_ID,
    CONDUIT_NATIVE_FLOAT_ID, CONDUIT_NATIVE_DOUBLE_ID
};

const NodeConvertFun CONVERT_TO_FUNS[20] = {
    convert_to_int8_array, convert_to_int16_array, convert_to_int32_array, convert_to_int64_array,
    convert_to_uint8_array, convert_to_uint16_array, convert_to_uint32_array, convert_to_uint64_array,
    convert_to_float32_array, convert_to_float64_array,
    convert_to_char_array, convert_to_short_array, convert_to_int_array, convert_to_long_array,
    convert_to_unsigned_char_array, convert_to_unsigned_short_array, convert_to_unsigned_int_array, convert_to_unsigned_long_array,
    convert_to_float_array, convert_to_double_array
};

//-----------------------------------------------------------------------------
TEST(conduit_node_to_array, static_type_to_vector)
{
    uint8 data_vals[3] = {1,2,3};
    Node data_node;
    data_node.set(data_vals,3);

    for(index_t ti = 0; ti < 20; ti++)
    {
        int node_tid = CONVERT_TYPES[ti];
        NodeConvertFun convert_fun = CONVERT_TO_FUNS[ti];

        Node type_node;
        convert_fun(data_node, type_node);

        EXPECT_EQ(type_node.dtype().id(), node_tid);
        EXPECT_EQ(type_node.dtype().number_of_elements(),
            data_node.dtype().number_of_elements());

        Node temp_node;
        convert_to_uint8_array(type_node, temp_node);
        EXPECT_FALSE(memcmp(temp_node.data_ptr(), data_node.data_ptr(),
            data_node.total_bytes_allocated()));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_node_to_array, dynamic_type_to_vector)
{
    uint8 data_vals[3] = {1,2,3};
    Node data_node;
    data_node.set(data_vals,3);

    for(index_t ti = 0; ti < 10; ti++)
    {
        Node base_node;
        const DataType::TypeID base_tid = (DataType::TypeID)CONVERT_TYPES[ti];
        NodeConvertFun base_convert_fun = CONVERT_TO_FUNS[ti];
        base_convert_fun(data_node, base_node);

        for(index_t tj = 0; tj < 10; tj++)
        {
            Node to_node;
            const DataType::TypeID to_tid = (DataType::TypeID)CONVERT_TYPES[tj];
            base_node.to_data_type(to_tid, to_node);

            EXPECT_EQ(to_node.dtype().id(), to_tid);
            EXPECT_EQ(to_node.dtype().number_of_elements(),
                data_node.dtype().number_of_elements());

            Node temp_node;
            convert_to_uint8_array(to_node, temp_node);
            EXPECT_FALSE(memcmp(temp_node.data_ptr(), data_node.data_ptr(),
                data_node.total_bytes_allocated()));
        }
    }
}
