// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_zfp.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_relay_zfp.hpp"
#include "gtest/gtest.h"
#include <cstring>

using namespace conduit;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, zfparray_to_zfparray_no_conduit)
{
    // create compressed-array
    uint nx = 9;
    uint ny = 12;
    double rate = 32.0;
    std::vector<float> vals;
    vals.resize(nx * ny);

    for (uint j = 0; j < ny; j++)
    {
        for (uint i = 0; i < nx; i++)
        {
            vals[nx*j + i] = i * 10. + j*j;
        }
    }

    zfp::array2f arr(nx, ny, rate, vals.data());
    zfp::array2f::header header(arr);

    std::vector<uint8> hdr_bytes, data_bytes;

    hdr_bytes.resize(header.size_bytes());
    std::memcpy(hdr_bytes.data(), header.data(), header.size_bytes());

    data_bytes.resize(arr.compressed_size());
    std::memcpy(data_bytes.data(), arr.compressed_data(), arr.compressed_size());

    zfp::array1f::header r_header(hdr_bytes.data(), hdr_bytes.size());

    // NOTE: This no longer works with new zfp (https://github.com/LLNL/zfp/issues/253)
    // zfp::array *r_arr = zfp::array::construct(r_header);
    // // copy data into the array instance
    // std::memcpy(r_arr->compressed_data(),
    //             data_bytes.data(), data_bytes.size());

}

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, wrap_zfparray_and_verify_header)
{
    // initialize empty result Node
    Node result;
    EXPECT_FALSE(result.has_child(io::ZFP_HEADER_FIELD));

    // create compressed-array
    uint nx = 9;
    uint ny = 12;
    double rate = 8.0;
    zfp::array2f arr(nx, ny, rate);

    // write zfparray to Node
    io::wrap_zfparray(arr, result);

    // verify header entry was set
    EXPECT_TRUE(result.has_child(io::ZFP_HEADER_FIELD));
    Node n_header = result[io::ZFP_HEADER_FIELD];

    // assert header dtype
    EXPECT_TRUE(n_header.dtype().is_uint8());
    uint8_array header_as_arr = n_header.as_uint8_array();
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, wrap_zfparray_and_verify_compressed_data)
{
    // initialize empty result Node
    Node result;

    // create compressed-array
    uint nx = 9;
    uint ny = 12;
    uint ntotal = nx * ny;
    double rate = 8.0;
    std::vector<float> vals;
    vals.resize(ntotal);

    for (uint i = 0; i < ntotal; i++)
    {
        vals[i] = i*i;
    }


    zfp::array2f arr(nx, ny, rate, vals.data());

    // write zfparray to Node
    io::wrap_zfparray(arr, result);

    // verify compressed data entry was set
    EXPECT_TRUE(result.has_child(io::ZFP_COMPRESSED_DATA_FIELD));
    Node &n_data = result[io::ZFP_COMPRESSED_DATA_FIELD];

    EXPECT_TRUE(n_data.dtype().is_unsigned_integer());

    EXPECT_TRUE(0 == std::memcmp(arr.compressed_data(), n_data.data_ptr(), arr.compressed_size()));
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, wrap_zfparray_with_header_exception)
{
    // create compressed-array that does not support short header
    uint nx = 9;
    uint ny = 12;
    uint nz = 5;
    double rate = 64.0;
    zfp::array3d arr(nx, ny, nz, rate);

    // write zfparray to Node, but expect failure
    Node result;
    EXPECT_THROW(io::wrap_zfparray(arr, result),std::exception);
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, unwrap_zfparray)
{
    // create compressed-array
    const uint nx = 9;
    const uint ny = 12;

    float vals[nx * ny];
    uint i, j;
    for (j = 0; j < ny; j++) {
        for (i = 0; i < nx; i++) {
            vals[nx*j + i] = i * 10. + j*j;
        }
    }

    double rate = 32.0;
    zfp::array2f original_arr(nx, ny, rate, vals);

    // write zfparray to Node
    Node result;
    io::wrap_zfparray(original_arr, result);

    // fetch zfparray object from Node
    zfp::array* fetched_arr = io::unwrap_zfparray(result);

    // verify against original_arr
    ASSERT_TRUE(fetched_arr != 0);

    zfp::array2f* casted_arr = dynamic_cast<zfp::array2f*>(fetched_arr);
    ASSERT_TRUE(casted_arr != 0);

    EXPECT_EQ(nx, casted_arr->size_x());
    EXPECT_EQ(ny, casted_arr->size_y());
    EXPECT_EQ(rate, casted_arr->rate());

    // verify compressed data
    EXPECT_EQ(original_arr.compressed_size(), casted_arr->compressed_size());
    EXPECT_TRUE(0 == std::memcmp(original_arr.compressed_data(), casted_arr->compressed_data(), original_arr.compressed_size()));

    delete fetched_arr;
}

//-----------------------------------------------------------------------------
TEST(conduit_relay_zfp, unwrap_zfparray_with_exception)
{
    // create compressed-array
    uint nx = 9;
    uint ny = 12;

    double rate = 32.0;
    zfp::array2f original_arr(nx, ny, rate);

    // write zfparray to Node
    Node result;
    io::wrap_zfparray(original_arr, result);

    // corrupt the Node's data
    size_t n = 10;
    std::vector<float> vals;
    vals.resize(n);
    for (size_t i = 0; i < n; i++)
    {
        vals[i] = 0;
    }

    result[io::ZFP_HEADER_FIELD].set(vals.data(), vals.size());

    // fetch zfparray object from Node
    EXPECT_THROW(io::unwrap_zfparray(result);,std::exception);

}

