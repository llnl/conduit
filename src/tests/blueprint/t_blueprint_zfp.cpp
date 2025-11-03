// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_blueprint_zfp.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_blueprint.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;

// some CI do not have zfp available
// these tests look at dtypes and Npde entries, so contents don't matter

void set_zfparray_node_entries(Node& result,
                               uint8* header,
                               size_t n_header,
                               uint8* compressed_data,
                               size_t n_compressed_data)
{
    result[blueprint::zfparray::ZFP_HEADER_FIELD].set(header, n_header);
    result[blueprint::zfparray::ZFP_HEADER_DIM_FIELD] = 2;
    result[blueprint::zfparray::ZFP_HEADER_SCALAR_TYPE_FIELD] = 4; // zfp_type_double
    result[blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD].set(compressed_data, n_compressed_data);
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_zfp, zfp_verify_valid_zfparray)
{
    size_t n_header = 4;
    uint8 * header = new uint8[n_header]();

    size_t n_compressed_data = 4;
    uint8 * compressed_data = new uint8[n_compressed_data]();

    Node result, info;
    set_zfparray_node_entries(result, header, n_header, compressed_data, n_compressed_data);

    EXPECT_TRUE(blueprint::zfparray::verify(result, info));

    delete [] header;
    delete [] compressed_data;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_zfp, zfp_verify_invalid_zfparray_without_header)
{
    size_t n_header = 4;
    uint8 * header = new uint8[n_header]();

    size_t n_compressed_data = 4;
    uint8 * compressed_data = new uint8[n_compressed_data]();

    Node result, info;
    set_zfparray_node_entries(result, header, n_header, compressed_data, n_compressed_data);

    EXPECT_TRUE(blueprint::zfparray::verify(result, info));

    // remove header node
    EXPECT_TRUE(result.has_child(blueprint::zfparray::ZFP_HEADER_FIELD));
    result.remove(blueprint::zfparray::ZFP_HEADER_FIELD);

    EXPECT_FALSE(blueprint::zfparray::verify(result, info));

    delete [] header;
    delete [] compressed_data;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_zfp, zfp_verify_invalid_zfparray_without_compressed_data)
{
    size_t n_header = 4;
    uint8 * header = new uint8[n_header]();

    size_t n_compressed_data = 4;
    uint8 * compressed_data = new uint8[n_compressed_data]();

    Node result, info;
    set_zfparray_node_entries(result, header, n_header, compressed_data, n_compressed_data);

    EXPECT_TRUE(blueprint::zfparray::verify(result, info));

    // remove compressed-data node
    EXPECT_TRUE(result.has_child(blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD));
    result.remove(blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD);

    EXPECT_FALSE(blueprint::zfparray::verify(result, info));

    delete [] header;
    delete [] compressed_data;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_zfp, zfp_verify_invalid_zfparray_with_incorrect_header_dtype)
{
    size_t n_header = 4;
    uint8 * header = new uint8[n_header]();

    size_t n_compressed_data = 4;
    uint8 * compressed_data = new uint8[n_compressed_data]();

    Node result, info;
    set_zfparray_node_entries(result, header, n_header, compressed_data, n_compressed_data);

    EXPECT_TRUE(blueprint::zfparray::verify(result, info));

    // remove header node
    EXPECT_TRUE(result.has_child(blueprint::zfparray::ZFP_HEADER_FIELD));
    result.remove(blueprint::zfparray::ZFP_HEADER_FIELD);

    // re-add header node as a double
    double dummy = 4.4;
    result[blueprint::zfparray::ZFP_HEADER_FIELD] = dummy;

    EXPECT_FALSE(blueprint::zfparray::verify(result, info));

    delete [] header;
    delete [] compressed_data;
}

//-----------------------------------------------------------------------------
TEST(conduit_blueprint_zfp, zfp_verify_invalid_zfparray_with_incorrect_compressed_data_dtype)
{
    size_t n_header = 4;
    uint8 * header = new uint8[n_header]();

    size_t n_compressed_data = 4;
    uint8 * compressed_data = new uint8[n_compressed_data]();

    Node result, info;
    set_zfparray_node_entries(result, header, n_header, compressed_data, n_compressed_data);

    EXPECT_TRUE(blueprint::zfparray::verify(result, info));

    // remove compressed-data node
    EXPECT_TRUE(result.has_child(blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD));
    result.remove(blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD);

    // re-add compressed-data node as a double
    double dummy = 4.4;
    result[blueprint::zfparray::ZFP_COMPRESSED_DATA_FIELD] = dummy;

    EXPECT_FALSE(blueprint::zfparray::verify(result, info));

    delete [] header;
    delete [] compressed_data;
}

