// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_relay_io_hdf5_prec.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_relay.hpp"
#include "conduit_relay_io_hdf5.hpp"
#include "hdf5.h"
#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;
using namespace conduit::relay;

//-----------------------------------------------------------------------------
TEST(conduit_relay_io_hdf5, hdf5_prec_write)
{
    io::hdf5_reset_options();

    Node n, nload, info;
    n["i8"].set_int8(-8);
    n["i16"].set_int16(-16);
    n["i32"].set_int32(-32);
    n["i64"].set_int64(-64);

    n["u8"].set_uint8(80);
    n["u16"].set_uint16(160);
    n["u32"].set_uint32(3200);
    n["u64"].set_uint64(6400);

    n["f32"].set_float32(1.141);
    n["f64"].set_float64(3.14159);

    CONDUIT_INFO("Example Tree")
    std::cout << n.to_yaml() << std::endl;
    n.schema().print();

    io::hdf5_write(n,"tout_hdf5_prec_opts_base.hdf5");

    // shuffle the deck on read!
    Node opts;
    opts["precision/int8/read"]  = "int16";
    opts["precision/int16/read"] = "int8";
    opts["precision/int32/read"] = "int64";
    opts["precision/int64/read"] = "int32";

    opts["precision/uint8/read"]  = "uint16";
    opts["precision/uint16/read"] = "uint8";
    opts["precision/uint32/read"] = "uint64";
    opts["precision/uint64/read"] = "uint32";

    opts["precision/float64/read"] = "float32";
    opts["precision/float32/read"] = "float64";

    io::hdf5_set_options(opts);
    CONDUIT_INFO("Options for Read Shuffle")
    std::cout << opts.to_yaml() << std::endl;
    io::hdf5_read("tout_hdf5_prec_opts_base.hdf5",nload);
    nload.schema().print();

    // check types and values
    EXPECT_TRUE(nload["i8"].dtype().is_int16());
    EXPECT_EQ(nload["i8"].as_int16(),-8);

    EXPECT_TRUE(nload["i16"].dtype().is_int8());
    EXPECT_EQ(nload["i16"].as_int8(),-16);

    EXPECT_TRUE(nload["i32"].dtype().is_int64());
    EXPECT_EQ(nload["i32"].as_int64(),-32);

    EXPECT_TRUE(nload["i64"].dtype().is_int32());
    EXPECT_EQ(nload["i64"].as_int32(),-64);

    EXPECT_TRUE(nload["u8"].dtype().is_uint16());
    EXPECT_EQ(nload["u8"].as_uint16(),80);

    EXPECT_TRUE(nload["u16"].dtype().is_uint8());
    EXPECT_EQ(nload["u16"].as_uint8(),160);

    EXPECT_TRUE(nload["u32"].dtype().is_uint64());
    EXPECT_EQ(nload["u32"].as_uint64(),3200);

    EXPECT_TRUE(nload["u64"].dtype().is_uint32());
    EXPECT_EQ(nload["u64"].as_uint32(),6400);

    EXPECT_TRUE(nload["f32"].dtype().is_float64());
    EXPECT_NEAR(nload["f32"].as_float64(),1.141,1e-5);

    EXPECT_TRUE(nload["f64"].dtype().is_float32());
    EXPECT_NEAR(nload["f64"].as_float32(),3.14159,1e-5);


    io::hdf5_reset_options();
    opts.reset();

    // shuffle the deck on write!
    opts["precision/int8/write"]  = "int16";
    opts["precision/int16/write"] = "int8";
    opts["precision/int32/write"] = "int64";
    opts["precision/int64/write"] = "int32";

    opts["precision/uint8/write"]  = "uint16";
    opts["precision/uint16/write"] = "uint8";
    opts["precision/uint32/write"] = "uint64";
    opts["precision/uint64/write"] = "uint32";

    opts["precision/float64/write"] = "float32";
    opts["precision/float32/write"] = "float64";

    io::hdf5_set_options(opts);

    CONDUIT_INFO("Options for Write Shuffle")
    std::cout << opts.to_yaml() << std::endl;
    io::hdf5_write(n,"tout_hdf5_prec_opts_write_shuffle.hdf5");
    io::hdf5_read("tout_hdf5_prec_opts_write_shuffle.hdf5",nload);
    nload.schema().print();

    // check types and values
    EXPECT_TRUE(nload["i8"].dtype().is_int16());
    EXPECT_EQ(nload["i8"].as_int16(),-8);

    EXPECT_TRUE(nload["i16"].dtype().is_int8());
    EXPECT_EQ(nload["i16"].as_int8(),-16);

    EXPECT_TRUE(nload["i32"].dtype().is_int64());
    EXPECT_EQ(nload["i32"].as_int64(),-32);

    EXPECT_TRUE(nload["i64"].dtype().is_int32());
    EXPECT_EQ(nload["i64"].as_int32(),-64);

    EXPECT_TRUE(nload["u8"].dtype().is_uint16());
    EXPECT_EQ(nload["u8"].as_uint16(),80);

    EXPECT_TRUE(nload["u16"].dtype().is_uint8());
    EXPECT_EQ(nload["u16"].as_uint8(),160);

    EXPECT_TRUE(nload["u32"].dtype().is_uint64());
    EXPECT_EQ(nload["u32"].as_uint64(),3200);

    EXPECT_TRUE(nload["u64"].dtype().is_uint32());
    EXPECT_EQ(nload["u64"].as_uint32(),6400);

    EXPECT_TRUE(nload["f32"].dtype().is_float64());
    EXPECT_NEAR(nload["f32"].as_float64(),1.141,1e-5);

    EXPECT_TRUE(nload["f64"].dtype().is_float32());
    EXPECT_NEAR(nload["f64"].as_float32(),3.14159,1e-5);
    io::hdf5_reset_options();

}
