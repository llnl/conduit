// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_node_set.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;

//-----------------------------------------------------------------------------
// set bitwidth unsigned int scalar
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_uint_scalar)
{
    uint8    u8v = 8;
    uint16  u16v = 16;
    uint32  u32v = 32;
    uint64  u64v = 64;

    Node n;
    // uint8
    n.set(u8v);
    n.schema().print();
    EXPECT_EQ(n.as_uint8(),u8v);
    EXPECT_EQ(n.total_strided_bytes(),1);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_uint64(),8);
    
    // uint16
    n.set(u16v);
    n.schema().print();
    EXPECT_EQ(n.as_uint16(),u16v);
    EXPECT_EQ(n.total_strided_bytes(),2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_uint64(),16);

    
    // uint32
    n.set(u32v);
    n.schema().print();
    EXPECT_EQ(n.as_uint32(),u32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_uint64(),32);
    
    // uint64
    n.set(u64v);
    n.schema().print();
    EXPECT_EQ(n.as_uint64(),u64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_uint64(),64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_uint_scalar)
{
    uint8    u8v = 8;
    uint16  u16v = 16;
    uint32  u32v = 32;
    uint64  u64v = 64;

    Node n;
    // uint8
    n.set_path("one/two/three",u8v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));    
    Node &nc = n["one/two/three"];
    EXPECT_EQ(nc.as_uint8(),u8v);
    EXPECT_EQ(nc.total_strided_bytes(),1);
    EXPECT_EQ(nc.dtype().element_bytes(),1);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_uint64(),8);
    
    // uint16
    n.set_path("one/two/three",u16v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_uint16(),u16v);
    EXPECT_EQ(nc.total_strided_bytes(),2);
    EXPECT_EQ(nc.dtype().element_bytes(),2);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_uint64(),16);

    
    // uint32
    n.set_path("one/two/three",u32v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_uint32(),u32v);
    EXPECT_EQ(nc.total_strided_bytes(),4);
    EXPECT_EQ(nc.dtype().element_bytes(),4);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_uint64(),32);
    
    // uint64
    n.set_path("one/two/three",u64v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_uint64(),u64v);
    EXPECT_EQ(nc.total_strided_bytes(),8);
    EXPECT_EQ(nc.dtype().element_bytes(),8);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_uint64(),64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_uint_scalar)
{
    uint8    u8v = 8;
    uint16  u16v = 16;
    uint32  u32v = 32;
    uint64  u64v = 64;

    Node n;
    // uint8
    n.set_external(&u8v);
    n.schema().print();
    EXPECT_EQ(n.as_uint8(),u8v);
    EXPECT_EQ(n.total_strided_bytes(),1);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),8);
    n.set((uint8)1);
    EXPECT_EQ(u8v,1);
    
    // uint16
    n.set_external(&u16v);
    n.schema().print();
    EXPECT_EQ(n.as_uint16(),u16v);
    EXPECT_EQ(n.total_strided_bytes(),2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),16);
    n.set((uint16)1);
    EXPECT_EQ(u16v,1);

    
    // uint32
    n.set_external(&u32v);
    n.schema().print();
    EXPECT_EQ(n.as_uint32(),u32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),32);
    n.set((uint32)1);
    EXPECT_EQ(u32v,1);

    
    // uint64
    n.set_external(&u64v);
    n.schema().print();
    EXPECT_EQ(n.as_uint64(),u64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),true);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),64);
    n.set((uint64)1);
    EXPECT_EQ(u64v,1);


}

//-----------------------------------------------------------------------------
// set bitwidth signed int scalar
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_int_scalar)
{
    int8    i8v = -8;
    int16  i16v = -16;
    int32  i32v = -32;
    int64  i64v = -64;

    Node n;
    // int8
    n.set(i8v);
    n.schema().print();
    EXPECT_EQ(n.as_int8(),i8v);
    EXPECT_EQ(n.total_strided_bytes(),1);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-8);
    
    // int16
    n.set(i16v);
    n.schema().print();
    EXPECT_EQ(n.as_int16(),i16v);
    EXPECT_EQ(n.total_strided_bytes(),2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-16);

    
    // int32
    n.set(i32v);
    n.schema().print();
    EXPECT_EQ(n.as_int32(),i32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-32);
    
    // int64
    n.set(i64v);
    n.schema().print();
    EXPECT_EQ(n.as_int64(),i64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_int_scalar)
{
    int8    i8v = -8;
    int16  i16v = -16;
    int32  i32v = -32;
    int64  i64v = -64;

    Node n;
    // int8
    n.set_path("one/two/three",i8v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node &nc = n["one/two/three"];
    EXPECT_EQ(nc.as_int8(),i8v);
    EXPECT_EQ(nc.total_strided_bytes(),1);
    EXPECT_EQ(nc.dtype().element_bytes(),1);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_int64(),-8);
    
    // int16
    n.set_path("one/two/three",i16v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_int16(),i16v);
    EXPECT_EQ(nc.total_strided_bytes(),2);
    EXPECT_EQ(nc.dtype().element_bytes(),2);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_int64(),-16);

    
    // int32
    n.set_path("one/two/three",i32v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_int32(),i32v);
    EXPECT_EQ(nc.total_strided_bytes(),4);
    EXPECT_EQ(nc.dtype().element_bytes(),4);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_int64(),-32);
    
    // int64
    n.set_path("one/two/three",i64v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_int64(),i64v);
    EXPECT_EQ(nc.total_strided_bytes(),8);
    EXPECT_EQ(nc.dtype().element_bytes(),8);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),true);
    EXPECT_EQ(nc.dtype().is_signed_integer(),true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),false);
    EXPECT_EQ(nc.to_int64(),-64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_int_scalar)
{
    int8    i8v = -8;
    int16  i16v = -16;
    int32  i32v = -32;
    int64  i64v = -64;

    Node n;
    // int8
    n.set_external(&i8v);
    n.schema().print();
    EXPECT_EQ(n.as_int8(),i8v);
    EXPECT_EQ(n.total_strided_bytes(),1);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    n.set((int8)-1);
    EXPECT_EQ(i8v,-1);
    
    // int16
    n.set_external(&i16v);
    n.schema().print();
    EXPECT_EQ(n.as_int16(),i16v);
    EXPECT_EQ(n.total_strided_bytes(),2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-16);
    n.set((int16)-1);
    EXPECT_EQ(i16v,-1);

    
    // int32
    n.set_external(&i32v);
    n.schema().print();
    EXPECT_EQ(n.as_int32(),i32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-32);
    n.set((int32)-1);
    EXPECT_EQ(i32v,-1);
        
    // int64
    n.set_external(&i64v);
    n.schema().print();
    EXPECT_EQ(n.as_int64(),i64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),true);
    EXPECT_EQ(n.dtype().is_signed_integer(),true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),false);
    EXPECT_EQ(n.to_int64(),-64);
    n.set((int64)-1);
    EXPECT_EQ(i64v,-1);

}

//-----------------------------------------------------------------------------
// set bitwidth float scalar
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_float_scalar)
{
    float32  f32v = -3.2f;
    float64  f64v = -6.4;

    Node n;

    // float32
    n.set(f32v);
    n.schema().print();
    EXPECT_EQ(n.as_float32(),f32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),false);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),true);
    EXPECT_NEAR(n.to_float64(),-3.2,0.001);
    
    // float64
    n.set(f64v);
    n.schema().print();
    EXPECT_EQ(n.as_float64(),f64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),false);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),true);
    EXPECT_NEAR(n.to_float64(),-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_float_scalar)
{
    float32  f32v = -3.2f;
    float64  f64v = -6.4;

    // float32
    Node n;
    n.set_path("one/two/three",f32v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node &nc = n["one/two/three"];
    EXPECT_EQ(nc.as_float32(),f32v);
    EXPECT_EQ(nc.total_strided_bytes(),4);
    EXPECT_EQ(nc.dtype().element_bytes(),4);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),false);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),true);
    EXPECT_NEAR(nc.to_float64(),-3.2,0.001);
    
    // float64
    n.set_path("one/two/three",f64v);
    n.schema().print();
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_float64(),f64v);
    EXPECT_EQ(nc.total_strided_bytes(),8);
    EXPECT_EQ(nc.dtype().element_bytes(),8);
    EXPECT_EQ(nc.dtype().is_number(),true);
    EXPECT_EQ(nc.dtype().is_integer(),false);
    EXPECT_EQ(nc.dtype().is_signed_integer(),false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(nc.dtype().is_floating_point(),true);
    EXPECT_NEAR(nc.to_float64(),-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_float_scalar)
{
    float32  f32v = -3.2f;
    float64  f64v = -6.4;

    Node n;

    // float32
    n.set_external(&f32v);
    n.schema().print();
    EXPECT_EQ(n.as_float32(),f32v);
    EXPECT_EQ(n.total_strided_bytes(),4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),false);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),true);
    EXPECT_NEAR(n.to_float64(),-3.2,0.001);
    n.set((float32)-1.1);
    EXPECT_NEAR(f32v,-1.1,0.001);
    
    // float64
    n.set_external(&f64v);
    n.schema().print();
    EXPECT_EQ(n.as_float64(),f64v);
    EXPECT_EQ(n.total_strided_bytes(),8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    EXPECT_EQ(n.dtype().is_number(),true);
    EXPECT_EQ(n.dtype().is_integer(),false);
    EXPECT_EQ(n.dtype().is_signed_integer(),false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(),false);
    EXPECT_EQ(n.dtype().is_floating_point(),true);
    EXPECT_NEAR(n.to_float64(),-6.4,0.001);
    n.set((float64)-1.1);
    EXPECT_NEAR(f64v,-1.1,0.001);
}

//-----------------------------------------------------------------------------
// set bitwidth unsigned int ptr
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_uint_ptr)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};
        
    Node n;
    // using uint8* interface
    n.set(u8av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    uint8 *u8av_ptr = n.as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u8av_ptr[i],&u8av[i]);
    }
    EXPECT_EQ(u8av_ptr[5],64);
    
    // using uint16* interface
    n.set(u16av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    uint16 *u16av_ptr = n.as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u16av_ptr[i],&u16av[i]);
    }
    EXPECT_EQ(u16av_ptr[5],64);
    
    // using uint32 * interface
    n.set(u32av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    uint32 *u32av_ptr = n.as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u32av_ptr[i],&u32av[i]);
    }
    EXPECT_EQ(u32av_ptr[5],64);
    
    // using uint64 * interface
    n.set(u64av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    uint64 *u64av_ptr = n.as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_uint_array)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};
    
    uint8_array  u8av_a(u8av,DataType::uint8(6));
    uint16_array u16av_a(u16av,DataType::uint16(6));
    uint32_array u32av_a(u32av,DataType::uint32(6));
    uint64_array u64av_a(u64av,DataType::uint64(6));
    
    Node n;
    // uint8
    n.set(u8av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    uint8 *u8av_ptr = n.as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u8av_ptr[i],&u8av[i]);
    }
    EXPECT_EQ(u8av_ptr[5],64);
    
    // uint16    
    n.set(u16av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    uint16 *u16av_ptr = n.as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u16av_ptr[i],&u16av[i]);
    }
    EXPECT_EQ(u16av_ptr[5],64);
    
    // uint32    
    n.set(u32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    uint32 *u32av_ptr = n.as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u32av_ptr[i],&u32av[i]);
    }
    EXPECT_EQ(u32av_ptr[5],64);
    
    // uint64
    n.set(u64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    uint64 *u64av_ptr = n.as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_uint_array)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};
    
    uint8_array  u8av_a(u8av,DataType::uint8(6));
    uint16_array u16av_a(u16av,DataType::uint16(6));
    uint32_array u32av_a(u32av,DataType::uint32(6));
    uint64_array u64av_a(u64av,DataType::uint64(6));
    
    Node n;
    // uint8
    n.set_path("two/lvl",u8av_a);
    n.schema().print();    
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    uint8 *u8av_ptr = n["two/lvl"].as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u8av_ptr[i],&u8av[i]);
    }
    EXPECT_EQ(u8av_ptr[5],64);
    
    // uint16    
    n.set_path("two/lvl",u16av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    uint16 *u16av_ptr = n["two/lvl"].as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u16av_ptr[i],&u16av[i]);
    }
    EXPECT_EQ(u16av_ptr[5],64);    
    
    // uint32    
    n.set_path("two/lvl",u32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    uint32 *u32av_ptr = n["two/lvl"].as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u32av_ptr[i],&u32av[i]);
    }
    EXPECT_EQ(u32av_ptr[5],64);
    
    // uint64
    n.set_path("two/lvl",u64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    uint64 *u64av_ptr =  n["two/lvl"].as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_uint_ptr)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};

    Node n;    
    // using uint8* interface
    n.set_path("two/lvl",u8av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    uint8 *u8av_ptr = n["two/lvl"].as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u8av_ptr[i],&u8av[i]);
    }
    EXPECT_EQ(u8av_ptr[5],64);

    
    // using uint16* interface
    n.set_path("two/lvl",u16av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    uint16 *u16av_ptr = n["two/lvl"].as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u16av_ptr[i],&u16av[i]);
    }
    EXPECT_EQ(u16av_ptr[5],64);
    
    // using uint32 * interface
    n.set_path("two/lvl",u32av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ( n["two"]["lvl"].dtype().element_bytes(),4);
    uint32 *u32av_ptr = n["two"]["lvl"].as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u32av_ptr[i],&u32av[i]);
    }
    EXPECT_EQ(u32av_ptr[5],64);
    
    // using uint64 * interface
    n.set_path("two/lvl",u64av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ( n["two"]["lvl"].dtype().element_bytes(),8);
    uint64 *u64av_ptr =  n["two/lvl"].as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_uint_array)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};
    
    uint8_array u8av_a(u8av,DataType::uint8(6));
    uint16_array u16av_a(u16av,DataType::uint16(6));
    uint32_array u32av_a(u32av,DataType::uint32(6));
    uint64_array u64av_a(u64av,DataType::uint64(6));
    
    Node n;
    // uint8
    n.set_external(u8av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    uint8 *u8av_ptr = n.as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
         // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u8av_ptr[i],&u8av[i]);
    }
    u8av_ptr[1] = 100;
    EXPECT_EQ(u8av[1],100);
    n.print();
    
    // uint16    
    n.set_external(u16av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    uint16 *u16av_ptr = n.as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u16av_ptr[i],&u16av[i]);
    }
    u16av_ptr[1] = 100;
    EXPECT_EQ(u16av[1],100);
    n.print();

    // uint32    
    n.set_external(u32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    uint32 *u32av_ptr = n.as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u32av_ptr[i],&u32av[i]);
    }
    u32av_ptr[1] = 100;
    EXPECT_EQ(u32av[1],100);
    n.print();

    
    // uint64
    n.set_external(u64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    uint64 *u64av_ptr = n.as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);
    u64av_ptr[1] = 100;
    EXPECT_EQ(u64av[1],100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_uint_ptr)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};

    Node n;
    // uint8
    n.set_external(u8av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    uint8 *u8av_ptr = n.as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
         // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u8av_ptr[i],&u8av[i]);
    }
    u8av_ptr[1] = 100;
    EXPECT_EQ(u8av[1],100);
    n.print();
    
    // uint16    
    n.set_external(u16av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    uint16 *u16av_ptr = n.as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u16av_ptr[i],&u16av[i]);
    }
    u16av_ptr[1] = 100;
    EXPECT_EQ(u16av[1],100);
    n.print();

    // uint32    
    n.set_external(u32av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    uint32 *u32av_ptr = n.as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u32av_ptr[i],&u32av[i]);
    }
    u32av_ptr[1] = 100;
    EXPECT_EQ(u32av[1],100);
    n.print();

    
    // uint64
    n.set_external(u64av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    uint64 *u64av_ptr = n.as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);
    u64av_ptr[1] = 100;
    EXPECT_EQ(u64av[1],100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_uint_array)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};
    
    uint8_array u8av_a(u8av,DataType::uint8(6));
    uint16_array u16av_a(u16av,DataType::uint16(6));
    uint32_array u32av_a(u32av,DataType::uint32(6));
    uint64_array u64av_a(u64av,DataType::uint64(6));
    
    Node n;
    // uint8
    n.set_path_external("two/lvl",u8av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    uint8 *u8av_ptr = n["two/lvl"].as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
         // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u8av_ptr[i],&u8av[i]);
    }
    u8av_ptr[1] = 100;
    EXPECT_EQ(u8av[1],100);
    n.print();
    
    // uint16    
    n.set_path_external("two/lvl",u16av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    uint16 *u16av_ptr = n["two/lvl"].as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u16av_ptr[i],&u16av[i]);
    }
    u16av_ptr[1] = 100;
    EXPECT_EQ(u16av[1],100);
    n.print();

    // uint32    
    n.set_path_external("two/lvl",u32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    uint32 *u32av_ptr = n["two/lvl"].as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u32av_ptr[i],&u32av[i]);
    }
    u32av_ptr[1] = 100;
    EXPECT_EQ(u32av[1],100);
    n.print();

    
    // uint64
    n.set_path_external("two/lvl",u64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    uint64 *u64av_ptr = n["two/lvl"].as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);
    u64av_ptr[1] = 100;
    EXPECT_EQ(u64av[1],100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_uint_ptr)
{
    uint8    u8av[6] = {2,4,8,16,32,64};
    uint16  u16av[6] = {2,4,8,16,32,64};
    uint32  u32av[6] = {2,4,8,16,32,64};
    uint64  u64av[6] = {2,4,8,16,32,64};

    Node n;
    // uint8
    n.set_path_external("two/lvl",u8av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    uint8 *u8av_ptr = n["two/lvl"].as_uint8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u8av_ptr[i],u8av[i]);
         // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u8av_ptr[i],&u8av[i]);
    }
    u8av_ptr[1] = 100;
    EXPECT_EQ(u8av[1],100);
    n.print();
    
    // uint16    
    n.set_path_external("two/lvl",u16av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    uint16 *u16av_ptr = n["two/lvl"].as_uint16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u16av_ptr[i],u16av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u16av_ptr[i],&u16av[i]);
    }
    u16av_ptr[1] = 100;
    EXPECT_EQ(u16av[1],100);
    n.print();

    // uint32    
    n.set_path_external("two/lvl",u32av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    uint32 *u32av_ptr = n["two/lvl"].as_uint32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u32av_ptr[i],u32av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u32av_ptr[i],&u32av[i]);
    }
    u32av_ptr[1] = 100;
    EXPECT_EQ(u32av[1],100);
    n.print();

    
    // uint64
    n.set_path_external("two/lvl",u64av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    uint64 *u64av_ptr = n["two/lvl"].as_uint64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(u64av_ptr[i],u64av[i]);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&u64av_ptr[i],&u64av[i]);
    }
    EXPECT_EQ(u64av_ptr[5],64);
    u64av_ptr[1] = 100;
    EXPECT_EQ(u64av[1],100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_int_array)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    int8_array  i8av_a(i8av,DataType::int8(6));
    int16_array i16av_a(i16av,DataType::int16(6));
    int32_array i32av_a(i32av,DataType::int32(6));
    int64_array i64av_a(i64av,DataType::int64(6));
    
    Node n;
    // int8
    n.set(i8av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    int8 *i8av_ptr = n.as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    // int16    
    n.set(i16av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    int16 *i16av_ptr = n.as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    // int32
    n.set(i32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    int32 *i32av_ptr = n.as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    // int64
    n.set(i64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    int64 *i64av_ptr = n.as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);

}


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_int_ptr)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
        
    Node n;
    // int8
    n.set(i8av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    int8 *i8av_ptr = n.as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    // int16    
    n.set(i16av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    int16 *i16av_ptr = n.as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    // int32
    n.set(i32av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    int32 *i32av_ptr = n.as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    // int64
    n.set(i64av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    int64 *i64av_ptr = n.as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_int_array)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    int8_array  i8av_a(i8av,DataType::int8(6));
    int16_array i16av_a(i16av,DataType::int16(6));
    int32_array i32av_a(i32av,DataType::int32(6));
    int64_array i64av_a(i64av,DataType::int64(6));
    
    Node n;
    // int8
    n.set_path("two/lvl",i8av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    int8 *i8av_ptr = n["two/lvl"].as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    // int16    
    n.set_path("two/lvl",i16av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    int16 *i16av_ptr = n["two/lvl"].as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    // int32
    n.set_path("two/lvl",i32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    int32 *i32av_ptr = n["two/lvl"].as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    // int64
    n.set_path("two/lvl",i64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    int64 *i64av_ptr = n["two/lvl"].as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set_, set_path_bitwidth_int_ptr)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
        
    Node n;
    // int8
    n.set_path("two/lvl",i8av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    int8 *i8av_ptr = n["two/lvl"].as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    // int16    
    n.set_path("two/lvl",i16av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    int16 *i16av_ptr = n["two/lvl"].as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    // int32
    n.set_path("two/lvl",i32av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    int32 *i32av_ptr = n["two/lvl"].as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    // int64
    n.set_path("two/lvl",i64av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    int64 *i64av_ptr = n["two/lvl"].as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_int_array)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    int8_array  i8av_a(i8av,DataType::int8(6));
    int16_array i16av_a(i16av,DataType::int16(6));
    int32_array i32av_a(i32av,DataType::int32(6));
    int64_array i64av_a(i64av,DataType::int64(6));
    
    Node n;
    // int8
    n.set_external(i8av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    int8 *i8av_ptr = n.as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    i8av_ptr[1] = -100;
    EXPECT_EQ(i8av[1],-100);
    n.print();
    
    // int16    
    n.set_external(i16av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    int16 *i16av_ptr = n.as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    i16av_ptr[1] = -100;
    EXPECT_EQ(i16av[1],-100);
    n.print();
    
    // int32
    n.set_external(i32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    int32 *i32av_ptr = n.as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    i32av_ptr[1] = -100;
    EXPECT_EQ(i32av[1],-100);
    n.print();
    
    // int64
    n.set_external(i64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    int64 *i64av_ptr = n.as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);
    i64av_ptr[1] = -100;
    EXPECT_EQ(i64av[1],-100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set_, set_external_bitwidth_int_ptr)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    Node n;
    // int8
    n.set_external(i8av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6);
    EXPECT_EQ(n.dtype().element_bytes(),1);
    int8 *i8av_ptr = n.as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    i8av_ptr[1] = -100;
    EXPECT_EQ(i8av[1],-100);
    n.print();
    
    // int16    
    n.set_external(i16av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*2);
    EXPECT_EQ(n.dtype().element_bytes(),2);
    int16 *i16av_ptr = n.as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    i16av_ptr[1] = -100;
    EXPECT_EQ(i16av[1],-100);
    n.print();
    
    // int32
    n.set_external(i32av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    int32 *i32av_ptr = n.as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    i32av_ptr[1] = -100;
    EXPECT_EQ(i32av[1],-100);
    n.print();
    
    // int64
    n.set_external(i64av,6);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),6*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    int64 *i64av_ptr = n.as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);
    i64av_ptr[1] = -100;
    EXPECT_EQ(i64av[1],-100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_int_array)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    int8_array  i8av_a(i8av,DataType::int8(6));
    int16_array i16av_a(i16av,DataType::int16(6));
    int32_array i32av_a(i32av,DataType::int32(6));
    int64_array i64av_a(i64av,DataType::int64(6));
    
    Node n;
    // int8
    n.set_path_external("two/lvl",i8av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    int8 *i8av_ptr = n["two/lvl"].as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    i8av_ptr[1] = -100;
    EXPECT_EQ(i8av[1],-100);
    n.print();
    
    // int16    
    n.set_path_external("two/lvl",i16av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    int16 *i16av_ptr = n["two/lvl"].as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    i16av_ptr[1] = -100;
    EXPECT_EQ(i16av[1],-100);
    n.print();
    
    // int32
    n.set_path_external("two/lvl",i32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    int32 *i32av_ptr = n["two/lvl"].as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    i32av_ptr[1] = -100;
    EXPECT_EQ(i32av[1],-100);
    n.print();
    
    // int64
    n.set_path_external("two/lvl",i64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    int64 *i64av_ptr = n["two/lvl"].as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);
    i64av_ptr[1] = -100;
    EXPECT_EQ(i64av[1],-100);
    n.print();

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_int_ptr)
{
    int8    i8av[6] = {-2,-4,-8,-16,-32,-64};
    int16  i16av[6] = {-2,-4,-8,-16,-32,-64};
    int32  i32av[6] = {-2,-4,-8,-16,-32,-64};
    int64  i64av[6] = {-2,-4,-8,-16,-32,-64};
    
    Node n;
    // int8
    n.set_path_external("two/lvl",i8av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),1);
    int8 *i8av_ptr = n["two/lvl"].as_int8_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i8av_ptr[i],i8av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i8av_ptr[i],&i8av[i]);
    }
    EXPECT_EQ(i8av_ptr[5],-64);
    i8av_ptr[1] = -100;
    EXPECT_EQ(i8av[1],-100);
    n.print();
    
    // int16    
    n.set_path_external("two/lvl",i16av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*2);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),2);
    int16 *i16av_ptr = n["two/lvl"].as_int16_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i16av_ptr[i],i16av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i16av_ptr[i],&i16av[i]);
    }
    EXPECT_EQ(i16av_ptr[5],-64);
    i16av_ptr[1] = -100;
    EXPECT_EQ(i16av[1],-100);
    n.print();
    
    // int32
    n.set_path_external("two/lvl",i32av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    int32 *i32av_ptr = n["two/lvl"].as_int32_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i32av_ptr[i],i32av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i32av_ptr[i],&i32av[i]);
    }
    EXPECT_EQ(i32av_ptr[5],-64);
    i32av_ptr[1] = -100;
    EXPECT_EQ(i32av[1],-100);
    n.print();
    
    // int64
    n.set_path_external("two/lvl",i64av,6);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),6*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    int64 *i64av_ptr = n["two/lvl"].as_int64_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(i64av_ptr[i],i64av[i]);
       // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&i64av_ptr[i],&i64av[i]);
    }
    EXPECT_EQ(i64av_ptr[5],-64);
    i64av_ptr[1] = -100;
    EXPECT_EQ(i64av[1],-100);
    n.print();

}

/// 
/// set float array cases
///

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_float_array)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    float32_array f32av_a(f32av,DataType::float32(4));
    float64_array f64av_a(f64av,DataType::float64(4));

    Node n;
    // float32
    n.set(f32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    float32 *f32av_ptr = n.as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f32av_ptr[i],&f32av[i]); 
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    
    // float64
    n.set(f64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    float64 *f64av_ptr = n.as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_bitwidth_float_ptr)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    Node n;
    // float32
    n.set(f32av,4);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    float32 *f32av_ptr = n.as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f32av_ptr[i],&f32av[i]); 
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    
    // float64
    n.set(f64av,4);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    float64 *f64av_ptr = n.as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_float_array)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    float32_array f32av_a(f32av,DataType::float32(4));
    float64_array f64av_a(f64av,DataType::float64(4));

    Node n;
    // float32
    n.set_path("two/lvl",f32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    float32 *f32av_ptr = n["two/lvl"].as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f32av_ptr[i],&f32av[i]); 
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    
    // float64
    n.set_path("two/lvl",f64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    float64 *f64av_ptr = n["two/lvl"].as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_bitwidth_float_ptr)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    Node n;
    // float32
    n.set_path("two/lvl",f32av,4);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    float32 *f32av_ptr = n["two/lvl"].as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f32av_ptr[i],&f32av[i]); 
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    
    // float64
    n.set_path("two/lvl",f64av,4);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    float64 *f64av_ptr = n["two/lvl"].as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);

}

/// 
/// set float array external cases
///
//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_float_array)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    float32_array f32av_a(f32av,DataType::float32(4));
    float64_array f64av_a(f64av,DataType::float64(4));

    Node n;
    // float32
    n.set_external(f32av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    float32 *f32av_ptr = n.as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f32av_ptr[i],&f32av[i]);
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    f32av_ptr[1] = -110.1f;
    EXPECT_NEAR(f32av[1],-110.1,0.001);
    n.print();
    
    // float64
    n.set_external(f64av_a);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    float64 *f64av_ptr = n.as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    f64av_ptr[1] = -110.1;
    EXPECT_NEAR(f64av[1],-110.1,0.001);
    n.print();
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_external_bitwidth_float_ptr)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    Node n;
    // float32
    n.set_external(f32av,4);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*4);
    EXPECT_EQ(n.dtype().element_bytes(),4);
    float32 *f32av_ptr = n.as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f32av_ptr[i],&f32av[i]);
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    f32av_ptr[1] = -110.1f;
    EXPECT_NEAR(f32av[1],-110.1,0.001);
    n.print();
    
    // float64
    n.set_external(f64av,4);
    n.schema().print();
    EXPECT_EQ(n.total_strided_bytes(),4*8);
    EXPECT_EQ(n.dtype().element_bytes(),8);
    float64 *f64av_ptr = n.as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    f64av_ptr[1] = -110.1;
    EXPECT_NEAR(f64av[1],-110.1,0.001);
    n.print();
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_float_array)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    float32_array f32av_a(f32av,DataType::float32(4));
    float64_array f64av_a(f64av,DataType::float64(4));

    Node n;
    // float32
    n.set_path_external("two/lvl",f32av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    float32 *f32av_ptr = n["two/lvl"].as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f32av_ptr[i],&f32av[i]);
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    f32av_ptr[1] = -110.1f;
    EXPECT_NEAR(f32av[1],-110.1,0.001);
    n.print();
    
    // float64
    n.set_path_external("two/lvl",f64av_a);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    float64 *f64av_ptr = n["two/lvl"].as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    f64av_ptr[1] = -110.1;
    EXPECT_NEAR(f64av[1],-110.1,0.001);
    n.print();
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_bitwidth_float_ptr)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    Node n;
    // float32
    n.set_path_external("two/lvl",f32av,4);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    float32 *f32av_ptr = n["two/lvl"].as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f32av_ptr[i],&f32av[i]);
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    f32av_ptr[1] = -110.1f;
    EXPECT_NEAR(f32av[1],-110.1,0.001);
    n.print();
    
    // float64
    n.set_path_external("two/lvl",f64av,4);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    float64 *f64av_ptr = n["two/lvl"].as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    f64av_ptr[1] = -110.1;
    EXPECT_NEAR(f64av[1],-110.1,0.001);
    n.print();
}



//-----------------------------------------------------------------------------
// C Style Checks
//-----------------------------------------------------------------------------
//  -- set & set_external -- 
//-----------------------------------------------------------------------------
//
//  set_cstyle_native_int
//  set_cstyle_native_int_ptr
//  set_cstyle_native_int_array
//  set_cstyle_native_int_vec
//
//  set_cstyle_unsigned_int
//  set_cstyle_unsigned_int_ptr
//  set_cstyle_unsigned_int_array
//  set_cstyle_unsigned_int_vec
//
//  set_cstyle_signed_int
//  set_cstyle_signed_int_ptr
//  set_cstyle_signed_int_array
//  set_cstyle_signed_int_vec
//
//  set_cstyle_float
//  set_cstyle_float_ptr
//  set_cstyle_float_array
//  set_cstyle_float_vec
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// set -- c style native scalar, ptr, array, and vec
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_native_int)
{
    char char_v = 2;
    short short_v = 4;
    int int_v = 8;
    long long_v = 16;

    Node n;
    // char
    n.set(char_v);
    EXPECT_EQ(n.as_char(), char_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
#ifdef CONDUIT_CHAR_IS_SIGNED
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_char(), 2);

    // short
    n.set(short_v);
    EXPECT_EQ(n.as_short(), short_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
#ifdef CONDUIT_SHORT_IS_SIGNED
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_short(), 4);

    // int
    n.set(int_v);
    EXPECT_EQ(n.as_int(), int_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
#ifdef CONDUIT_INT_IS_SIGNED
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_int(), 8);

    // long
    n.set(long_v);
    EXPECT_EQ(n.as_long(), long_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
#ifdef CONDUIT_LONG_IS_SIGNED
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_long(), 16);

#ifdef CONDUIT_HAS_LONG_LONG
    long long long_long_v = 16;
    // long long
    n.set(long_long_v);
    EXPECT_EQ(n.as_long_long(), long_long_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
#ifdef CONDUIT_LONG_LONG_IS_SIGNED
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_long_long(), 16);
#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_native_int_ptr)
{
    char        char_av[6]  = {2,4,8,16,32,64};
    short       short_av[6] = {2,4,8,16,32,64};
    int         int_av[6]   = {2,4,8,16,32,64};
    long        long_av[6]  = {2,4,8,16,32,64};
#ifdef CONDUIT_HAS_LONG_LONG
    long long   longlong_av[6]  = {2,4,8,16,32,64};
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // char
    n.set_char_ptr(char_av,6);
    n.schema().print();
    char *char_ptr = n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 =  n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set(short_av,6);
    n.schema().print();
    short *short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set(int_av,6);
    n.schema().print();
    int *int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set(long_av,6);
    n.schema().print();
    long *long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);
   
    // also check access via value()
    long *long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set(longlong_av,6);
    n.schema().print();
    long long *longlong_ptr = n.as_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // char 
    n.set_external_char_ptr(char_av,6);
    n.schema().print();
    char_ptr = n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 = n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_external(short_av,6);
    n.schema().print();
    short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_external(int_av,6);
    n.schema().print();
    int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_external(long_av,6);
    n.schema().print();
    long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_external(longlong_av,6);
    n.schema().print();
    longlong_ptr = n.as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_native_int_array)
{
    // note: char may not be signed, we check
    // signed values in other tests
    char        char_av[6]  = {2,4,8,16,32,64};
    short       short_av[6] = {2,4,8,16,32,64};
    int         int_av[6]   = {2,4,8,16,32,64};
    long        long_av[6]  = {2,4,8,16,32,64};
#ifdef CONDUIT_HAS_LONG_LONG
    long long   longlong_av[6]  = {2,4,8,16,32,64};
#endif
    
    char_array  char_av_a(char_av,DataType::c_char(6));
    short_array short_av_a(short_av,DataType::c_short(6));
    int_array   int_av_a(int_av,DataType::c_int(6));
    long_array  long_av_a(long_av,DataType::c_long(6));
    
#ifdef CONDUIT_HAS_LONG_LONG
    long_long_array  longlong_av_a(longlong_av,DataType::c_long_long(6));
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // char
    n.set(char_av_a);
    n.schema().print();
    char *char_ptr = n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 =  n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set(short_av_a);
    n.schema().print();
    short *short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set(int_av_a);
    n.schema().print();
    int *int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set(long_av_a);
    n.schema().print();
    long *long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);
   
    // also check access via value()
    long *long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set(longlong_av_a);
    n.schema().print();
    long long *longlong_ptr = n.as_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // char
    n.set_external(char_av_a);
    n.schema().print();
    char_ptr = n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 =  n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_external(short_av_a);
    n.schema().print();
    short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_external(int_av_a);
    n.schema().print();
    int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_external(long_av_a);
    n.schema().print();
    long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_external(longlong_av_a);
    n.schema().print();
    longlong_ptr = n.as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_native_int_vec)
{
    // note: char may not be signed, we check
    // signed values in other tests
    char        char_av[6]  = {2,4,8,16,32,64};
    short       short_av[6] = {2,4,8,16,32,64};
    int         int_av[6]   = {2,4,8,16,32,64};
    long        long_av[6]  = {2,4,8,16,32,64};
#ifdef CONDUIT_HAS_LONG_LONG
    long long longlong_av[6]  = {2,4,8,16,32,64};
#endif
    
    std::vector<char>        char_v(char_av,char_av+6);
    std::vector<short>       short_v(short_av,short_av+6);
    std::vector<int>         int_v(int_av,int_av+6);
    std::vector<long>        long_v(long_av,long_av+6);
    
#ifdef CONDUIT_HAS_LONG_LONG
    std::vector<long long>   longlong_v(longlong_av,longlong_av+6);
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // char
    n.set(char_v);
    n.schema().print();
    char *char_ptr = n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_v[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 = n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set(short_v);
    n.schema().print();
    short *short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_v[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set(int_v);
    n.schema().print();
    int *int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_v[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set(long_v);
    n.schema().print();
    long *long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_v[i]);
    }

    EXPECT_EQ(long_ptr[5],64);
   
    // also check access via value()
    long *long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set(longlong_v);
    n.schema().print();
    long long *longlong_ptr = n.as_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_v[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_v[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // char
    n.set_external(char_v);
    n.schema().print();
    char_ptr =  n.as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_v[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 =  n.value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_external(short_v);
    n.schema().print();
    short_ptr = n.as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_v[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n.value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_external(int_v);
    n.schema().print();
    int_ptr = n.as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_v[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n.value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_external(long_v);
    n.schema().print();
    long_ptr = n.as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_v[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n.value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_external(longlong_v);
    n.schema().print();
    longlong_ptr = n.as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_v[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n.value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
// set -- c style unsigned scalar, ptr, array, and vec
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_unsigned_int)
{
    unsigned char uchar_v = 2;
    unsigned short ushort_v = 4;
    unsigned int uint_v = 8;
    unsigned long ulong_v = 16;

    Node n;
    // char
    n.set(uchar_v);
    EXPECT_EQ(n.as_unsigned_char(), uchar_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_unsigned_char(), 2);

    // short
    n.set(ushort_v);
    EXPECT_EQ(n.as_unsigned_short(), ushort_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_unsigned_short(), 4);

    // int
    n.set(uint_v);
    EXPECT_EQ(n.as_unsigned_int(), uint_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_unsigned_int(), 8);

    // long
    n.set(ulong_v);
    EXPECT_EQ(n.as_unsigned_long(), ulong_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_unsigned_long(), 16);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    unsigned long long ulong_long_v = 16;
    n.set(ulong_long_v);
    EXPECT_EQ(n.as_unsigned_long_long(), ulong_long_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_unsigned_long_long(), 16);
#endif
}


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_unsigned_int_ptr)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // unsigned char
    n.set(uchar_av,6);
    n.schema().print();
    unsigned char *uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set(ushort_av,6);
    n.schema().print();
    unsigned short *ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);
    
    // also check access via value()
    unsigned short *ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);
    
    // unsigned int
    n.set(uint_av,6);
    n.schema().print();
    unsigned int *uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);
    
    // unsigned long
    n.set(ulong_av,6);
    n.schema().print();
    unsigned long *ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_av[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set(ulonglong_av,6);
    n.schema().print();
    unsigned long long *ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_av[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // unsigned char
    n.set_external(uchar_av,6);
    n.schema().print();
    uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_external(ushort_av,6);
    n.schema().print();
    ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_external(uint_av,6);
    n.schema().print();
    uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_external(ulong_av,6);
    n.schema().print();
    ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_av[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_external(ulonglong_av);
    n.schema().print();
    ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_av[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_unsigned_int_array)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif
    
    unsigned_char_array  uchar_av_a(uchar_av,DataType::c_unsigned_char(6));
    unsigned_short_array ushort_av_a(ushort_av,DataType::c_unsigned_short(6));
    unsigned_int_array   uint_av_a(uint_av,DataType::c_unsigned_int(6));
    unsigned_long_array  ulong_av_a(ulong_av,DataType::c_unsigned_long(6));
    
#if defined CONDUIT_HAS_LONG_LONG
    unsigned_long_long_array  ulonglong_av_a(ulonglong_av,DataType::c_unsigned_long_long(6));
#endif 
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // unsigned char
    n.set(uchar_av_a);
    n.schema().print();
    unsigned char *uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_av_a[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set(ushort_av_a);
    n.schema().print();
    unsigned short *ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_av_a[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);
    
    // also check access via value()
    unsigned short *ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);
    
    // unsigned int
    n.set(uint_av_a);
    n.schema().print();
    unsigned int *uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_av_a[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);
    
    // unsigned long
    n.set(ulong_av_a);
    n.schema().print();
    unsigned long *ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_av_a[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set(ulonglong_av_a);
    n.schema().print();
    unsigned long long *ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_av_a[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // unsigned char
    n.set_external(uchar_av_a);
    n.schema().print();
    uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_av_a[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_external(ushort_av_a);
    n.schema().print();
    ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_av_a[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_external(uint_av_a);
    n.schema().print();
    uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_av_a[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_external(ulong_av_a);
    n.schema().print();
    ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_av_a[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_external(ulonglong_av_a);
    n.schema().print();
    ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_av_a[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_unsigned_int_vec)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif
    
    std::vector<unsigned char>  uchar_v(uchar_av,uchar_av+6);
    std::vector<unsigned short> ushort_v(ushort_av,ushort_av+6);
    std::vector<unsigned int>   uint_v(uint_av,uint_av+6);
    std::vector<unsigned long>  ulong_v(ulong_av,ulong_av+6);
    
#if defined CONDUIT_HAS_LONG_LONG
    std::vector<unsigned long long> ulonglong_v(ulonglong_av,ulonglong_av+6);
#endif 
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // unsigned char
    n.set(uchar_v);
    n.schema().print();
    unsigned char *uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_v[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set(ushort_v);
    n.schema().print();
    unsigned short *ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_v[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);
    
    // also check access via value()
    unsigned short *ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);
    
    // unsigned int
    n.set(uint_v);
    n.schema().print();
    unsigned int *uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_v[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);
    
    // unsigned long
    n.set(ulong_v);
    n.schema().print();
    unsigned long *ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_v[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set(ulonglong_v);
    n.schema().print();
    unsigned long long *ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_v[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // unsigned char
    n.set_external(uchar_v);
    n.schema().print();
    uchar_ptr = n.as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_v[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n.value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_external(ushort_v);
    n.schema().print();
    ushort_ptr = n.as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_v[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n.value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_external(uint_v);
    n.schema().print();
    uint_ptr = n.as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_v[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n.value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_external(ulong_v);
    n.schema().print();
    ulong_ptr = n.as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_v[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n.value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_external(ulonglong_v);
    n.schema().print();
    ulonglong_ptr = n.as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_v[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n.value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif
    
}

//-----------------------------------------------------------------------------
// set -- c style signed scalar, ptr, array, and vec
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_signed_int)
{
    signed char schar_v = 2;
    signed short sshort_v = 4;
    signed int sint_v = 8;
    signed long slong_v = 16;

    Node n;
    // char
    n.set(schar_v);
    EXPECT_EQ(n.as_signed_char(), schar_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_signed_char(), 2);

    // short
    n.set(sshort_v);
    EXPECT_EQ(n.as_signed_short(), sshort_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_signed_short(), 4);

    // int
    n.set(sint_v);
    EXPECT_EQ(n.as_signed_int(), sint_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_signed_int(), 8);

    // long
    n.set(slong_v);
    EXPECT_EQ(n.as_signed_long(), slong_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_signed_long(), 16);

#if defined CONDUIT_HAS_LONG_LONG
    // long long
    signed long long slong_long_v =16;
    n.set(slong_long_v);
    EXPECT_EQ(n.as_signed_long_long(), slong_long_v);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), true);
    EXPECT_EQ(n.dtype().is_signed_integer(), true);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), false);
    EXPECT_EQ(n.to_signed_long_long(), 16);
#endif
}


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_signed_int_ptr)
{
    signed char   schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short  sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int    sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long   slong_av[6]  = {-2,-4,-8,-16,-32,-64};

#if defined CONDUIT_HAS_LONG_LONG
    signed long long   slonglong_av[6]  = {-2,-4,-8,-16,-32,-64};
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // unsigned char
    n.set(schar_av,6);
    n.schema().print();
    signed char *schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 = n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // unsigned short
    n.set(sshort_av,6);
    n.schema().print();
    signed short *sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);
    
    // also check access via value()
    signed short *sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);
    
    // unsigned int
    n.set(sint_av,6);
    n.schema().print();
    signed int *sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    signed int *sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);
    
    // unsigned long
    n.set(slong_av,6);
    n.schema().print();
    signed long *slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    signed long *slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set(slonglong_av,6);
    n.schema().print();
    signed long long *slonglong_ptr = n.as_signed_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    signed long long *slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);
#endif

    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // unsigned char
    n.set_external(schar_av,6);
    n.schema().print();
    schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 = n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // unsigned short
    n.set_external(sshort_av,6);
    n.schema().print();
    sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // unsigned int
    n.set_external(sint_av,6);
    n.schema().print();
    sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // unsigned long
    n.set_external(slong_av,6);
    n.schema().print();
    slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_external(slonglong_av);
    n.schema().print();
    slonglong_ptr = n.as_signed_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);
#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_signed_int_array)
{
    signed char   schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short  sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int    sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long   slong_av[6]  = {-2,-4,-8,-16,-32,-64};

#if defined CONDUIT_HAS_LONG_LONG
    signed long long   slonglong_av[6] = {-2,-4,-8,-16,-32,-64};
#endif
    
    signed_char_array  schar_av_a(schar_av,DataType::c_signed_char(6));
    signed_short_array sshort_av_a(sshort_av,DataType::c_signed_short(6));
    signed_int_array   sint_av_a(sint_av,DataType::c_signed_int(6));
    signed_long_array  slong_av_a(slong_av,DataType::c_signed_long(6));
    
#if defined CONDUIT_HAS_LONG_LONG
    signed_long_long_array  slonglong_av_a(slonglong_av,
                                           DataType::c_signed_long_long(6));
#endif 
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // signed char
    n.set(schar_av_a);
    n.schema().print();
    signed char *schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_av_a[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 = n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // signed short
    n.set(sshort_av_a);
    n.schema().print();
    signed short *sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_av_a[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);
    
    // also check access via value()
    signed short *sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);
    
    // signed int
    n.set(sint_av_a);
    n.schema().print();
    signed int *sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_av_a[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    signed int *sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);
    
    // signed long
    n.set(slong_av_a);
    n.schema().print();
    signed long *slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_av_a[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    signed long *slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // signed long long
    n.set(slonglong_av_a);
    n.schema().print();
    signed long long *slonglong_ptr = n.as_signed_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_av_a[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    signed long long *slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);
#endif

    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // signed char
    n.set_external(schar_av_a);
    n.schema().print();
    schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_av_a[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 = n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // signed short
    n.set_external(sshort_av_a);
    n.schema().print();
    sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_av_a[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // signed int
    n.set_external(sint_av_a);
    n.schema().print();
    sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_av_a[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // signed long
    n.set_external(slong_av_a);
    n.schema().print();
    slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_av_a[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_external(slonglong_av_a);
    n.schema().print();
    slonglong_ptr = n.as_signed_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_av_a[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);
#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_signed_int_vec)
{
    signed char  schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int   sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long  slong_av[6]  = {-2,-4,-8,-16,-32,-64};
#ifdef CONDUIT_HAS_LONG_LONG
    signed long long slonglong_av[6]  = {-2,-4,-8,-16,-32,-64};
#endif
    
    std::vector<signed char>  schar_v(schar_av,schar_av+6);
    std::vector<signed short> sshort_v(sshort_av,sshort_av+6);
    std::vector<signed int>   sint_v(sint_av,sint_av+6);
    std::vector<signed long>  slong_v(slong_av,slong_av+6);
    
#ifdef CONDUIT_HAS_LONG_LONG
    std::vector<signed long long>   slonglong_v(slonglong_av,slonglong_av+6);
#endif
    
    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////
    
    // char
    n.set(schar_v);
    n.schema().print();
    signed char *schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_v[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 =  n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set(sshort_v);
    n.schema().print();
    short *sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_v[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    short *sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set(sint_v);
    n.schema().print();
    int *sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_v[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    int *sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set(slong_v);
    n.schema().print();
    long *slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_v[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);
   
    // also check access via value()
    long *slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set(slonglong_v);
    n.schema().print();
    long long *slonglong_ptr = n.as_signed_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_v[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_v[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    long long *slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif
    
    ////////////////////////////
    // set external 
    ////////////////////////////

    // char
    n.set_external(schar_v);
    n.schema().print();
    schar_ptr = n.as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_v[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 =  n.value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set_external(sshort_v);
    n.schema().print();
    sshort_ptr = n.as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_v[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n.value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_external(sint_v);
    n.schema().print();
    sint_ptr = n.as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_v[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n.value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_external(slong_v);
    n.schema().print();
    slong_ptr = n.as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_v[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n.value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_external(slonglong_v);
    n.schema().print();
    slonglong_ptr = n.as_signed_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_v[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n.value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
// set -- c style float scalar, ptr, array, and vec
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_float)
{
    float fv = -0.8f;
    double dv = -1.6;

    Node n;
    // float
    n.set(fv);
    EXPECT_EQ(n.as_float(), fv);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), false);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), true);
    EXPECT_NEAR(n.to_float(), -0.8, 0.001);

    // double
    n.set(dv);
    EXPECT_EQ(n.as_double(), dv);
    EXPECT_EQ(n.dtype().is_number(), true);
    EXPECT_EQ(n.dtype().is_integer(), false);
    EXPECT_EQ(n.dtype().is_signed_integer(), false);
    EXPECT_EQ(n.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(n.dtype().is_floating_point(), true);
    EXPECT_NEAR(n.to_double(), -1.6, 0.001);
}


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_float_ptr)
{
    float   fav[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    double  dav[4] = {-0.8, -1.6, -3.2, -6.4};

#ifdef CONDUIT_USE_LONG_DOUBLE
    long double  ldav[4] = {-0.8, -1.6, -3.2, -6.4};
#endif

    Node n;
    if(sizeof(float) == 4)
    {
        // float32
        n.set(fav,4);
        n.schema().print();
        EXPECT_EQ(n.total_strided_bytes(),4*4);
        EXPECT_EQ(n.dtype().element_bytes(),4);
        float32 *f32av_ptr = n.as_float32_ptr();
        for(index_t i=0;i<4;i++)
        {
            EXPECT_NEAR(f32av_ptr[i],fav[i],0.001);
            // set(...) semantics imply a copy -- mem addys should differ
            EXPECT_NE(&f32av_ptr[i],&fav[i]); 
        }
        EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    }
    
    if(sizeof(double)== 8)
    {
        // float64
        n.set(dav,4);
        n.schema().print();
        EXPECT_EQ(n.total_strided_bytes(),4*8);
        EXPECT_EQ(n.dtype().element_bytes(),8);
        float64 *f64av_ptr = n.as_float64_ptr();
        for(index_t i=0;i<4;i++)
        {
            EXPECT_NEAR(f64av_ptr[i],dav[i],0.001);
            // set(...) semantics imply a copy -- mem addys should differ
            EXPECT_NE(&f64av_ptr[i],&dav[i]);
        }
        EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    }

    ////////////////////////////
    // set 
    ////////////////////////////

    // float
    n.set(fav,4);
    n.schema().print();
    float *f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav[i]); 
    }

    EXPECT_NEAR(f_ptr[3],-6.4,0.001);

    // also check access via value()
    float *f_ptr_2 = n.value();
    EXPECT_EQ(f_ptr,f_ptr_2);
    
    // double
    n.set(dav,4);
    n.schema().print();
    double *d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav[i]);
    }

    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    // also check access via value()
    double *d_ptr_2 = n.value();
    EXPECT_EQ(d_ptr,d_ptr_2);


#ifdef CONDIT_USE_LONG_DOUBLE

    // long_double
    n.set(ldav,4);
    n.schema().print();
    long double *ld_ptr = n.as_long_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(ld_ptr[i],ldav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ld_ptr[i],&ldav[i]);
    }
    EXPECT_NEAR(ld_ptr[3],-6.4,0.001);
    
    // also check access via value()
    long double *ld_ptr_2 = n.value();
    EXPECT_EQ(ld_ptr,ld_ptr_2);

#endif
    
    ////////////////////////////
    // set external 
    ////////////////////////////
    
    // float
    n.set_external(fav,4);
    n.schema().print();
    f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_EQ(&f_ptr[i],&fav[i]); 
    }

    EXPECT_NEAR(f_ptr[3],-6.4,0.001);

    // also check access via value()
    f_ptr_2 = n.value();
    EXPECT_EQ(f_ptr,f_ptr_2);

    // double
    n.set_external(dav,4);
    n.schema().print();
    d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_EQ(&d_ptr[i],&dav[i]);
    }

    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    // also check access via value()
    d_ptr_2 = n.value();
    EXPECT_EQ(d_ptr,d_ptr_2);


#ifdef CONDIT_USE_LONG_DOUBLE

    // long_double
    n.set_external(ldav,4);
    n.schema().print();
    ld_ptr = n.as_long_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(ld_ptr[i],ldav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_EQ(&ld_ptr[i],&ldav[i]);
    }
    EXPECT_NEAR(ld_ptr[3],-6.4,0.001);

    // also check access via value()
    ld_ptr_2 = n.value();
    EXPECT_EQ(ld_ptr,ld_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_float_array)
{
    float   fav[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    double  dav[4] = {-0.8, -1.6, -3.2, -6.4};

    float_array  fav_a(fav,DataType::c_float(4));
    double_array dav_a(dav,DataType::c_double(4));

    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////

    // float
    n.set(fav_a);
    n.schema().print();
    float *f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set(dav_a);
    n.schema().print();
    double *d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);
    
    
    ////////////////////////////
    // set external 
    ////////////////////////////
    
    // float
    n.set_external(fav_a);
    n.schema().print();
    f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_external(dav_a);
    n.schema().print();
    d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_cstyle_float_vec)
{
    float   fav[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    double  dav[4] = {-0.8, -1.6, -3.2, -6.4};

    std::vector<float>  fav_v(fav,fav+4);
    std::vector<double> dav_v(dav,dav+4);

    Node n;
    
    ////////////////////////////
    // set 
    ////////////////////////////

    // float
    n.set(fav_v);
    n.schema().print();
    float *f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav_v[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav_v[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set(dav_v);
    n.schema().print();
    double *d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav_v[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav_v[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);
    
    
    ////////////////////////////
    // set external 
    ////////////////////////////
    
    // float
    n.set_external(fav_v);
    n.schema().print();
    f_ptr = n.as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav_v[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&f_ptr[i],&fav_v[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_external(dav_v);
    n.schema().print();
    d_ptr = n.as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav_v[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&d_ptr[i],&dav_v[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
// C Style Checks
//-----------------------------------------------------------------------------
//  -- set_path & set_path_external -- 
//-----------------------------------------------------------------------------
//
//. set_path_cstyle_native_int
//  set_path_cstyle_native_int_ptr
//  set_path_cstyle_native_int_array
//  set_path_cstyle_native_int_vec
//
//  set_path_cstyle_unsigned_int
//  set_path_cstyle_unsigned_int_ptr
//  set_path_cstyle_unsigned_int_array
//  set_path_cstyle_unsigned_int_vec
//
//  set_path_cstyle_signed_int
//  set_path_cstyle_signed_int_ptr
//  set_path_cstyle_signed_int_array
//  set_path_cstyle_signed_int_vec
//
//  set_path_cstyle_float_ptr
//  set_path_cstyle_float_ptr
//  set_path_cstyle_float_array
//  set_path_cstyle_float_vec
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// set_path -- c style native scalar, ptr, array, and vec
//-----------------------------------------------------------------------------

// add and expand cases that were added by kitware for their vendored conduit
// https://gitlab.kitware.com/paraview/catalyst/-/commit/dbb5c297b5a05b98979912a4944af43a2ddd3818

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_native_int)
{
    char char_v = 2;
    short short_v = 4;
    int int_v = 8;
    long long_v = 16;

    Node n;
    // char
    n.set_path("one/two/three", char_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node nc = n["one/two/three"];
    EXPECT_EQ(nc.as_char(), char_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
#ifdef CONDUIT_CHAR_IS_SIGNED
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_char(), 2);

    // short
    n.set_path("one/two/three", short_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_short(), short_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
#ifdef CONDUIT_SHORT_IS_SIGNED
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_short(), 4);

    // int
    n.set_path("one/two/three", int_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_int(), int_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
#ifdef CONDUIT_INT_IS_SIGNED
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_int(), 8);

    // long
    n.set_path("one/two/three", long_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_long(), long_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
#ifdef CONDUIT_LONG_IS_SIGNED
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_long(), 16);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    long long long_long_v = 16;
    n.set_path("one/two/three", long_long_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_long_long(), long_long_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
#ifdef CONDUIT_LONG_LONG_IS_SIGNED
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
#else
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
#endif
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_long_long(), 16);
#endif

}


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_native_int_ptr)
{
    char        char_av[6]  = {2,4,8,16,32,64};
    short       short_av[6] = {2,4,8,16,32,64};
    int         int_av[6]   = {2,4,8,16,32,64};
    long        long_av[6]  = {2,4,8,16,32,64};
#ifdef CONDUIT_HAS_LONG_LONG
    long long   longlong_av[6]  = {2,4,8,16,32,64};
#endif
    
    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////

    // char
    n.set_path_char_ptr("c",char_av,6);
    n["c"].schema().print();
    char *char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_path("s",short_av,6);
    n["s"].schema().print();
    short *short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path("i",int_av,6);
    n["i"].schema().print();
    int *int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path("l",long_av,6);
    n["l"].schema().print();
    long *long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);
   
    // also check access via value()
    long *long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",longlong_av,6);
    n["ll"].schema().print();
    long long *longlong_ptr = n["ll"].as_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external_char_ptr("c",char_av,6);
    n["c"].schema().print();
    char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_path_external("s",short_av,6);
    n["s"].schema().print();
    short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path_external("i",int_av,6);
    n["i"].schema().print();
    int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path_external("l",long_av,6);
    n["l"].schema().print();
    long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",longlong_av,6);
    n["ll"].schema().print();
    longlong_ptr = n["ll"].as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_native_int_array)
{
    char        char_av[6]  = {2,4,8,16,32,64};
    short       short_av[6] = {2,4,8,16,32,64};
    int         int_av[6]   = {2,4,8,16,32,64};
    long        long_av[6]  = {2,4,8,16,32,64};
#ifdef CONDUIT_HAS_LONG_LONG
    long long   longlong_av[6]  = {2,4,8,16,32,64};
#endif
    
    char_array  char_av_a(char_av,DataType::c_char(6));
    short_array short_av_a(short_av,DataType::c_short(6));
    int_array   int_av_a(int_av,DataType::c_int(6));
    long_array  long_av_a(long_av,DataType::c_long(6));
    
#ifdef CONDUIT_HAS_LONG_LONG
    long_long_array  longlong_av_a(longlong_av,DataType::c_long_long(6));
#endif
    
    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////
    
    // char
    n.set_path("c",char_av_a);
    n["c"].schema().print();
    char *char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_path("s",short_av_a);
    n["s"].schema().print();
    short *short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path("i",int_av_a);
    n["i"].schema().print();
    int *int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path("l",long_av_a);
    n["l"].schema().print();
    long *long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);
   
    // also check access via value()
    long *long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",longlong_av_a);
    n["ll"].schema().print();
    long long *longlong_ptr = n["ll"].as_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external("c",char_av_a);
    n["c"].schema().print();
    char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_av[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short 
    n.set_path_external("s",short_av_a);
    n["s"].schema().print();
    short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_av[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path_external("i",int_av_a);
    n["i"].schema().print();
    int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_av[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path_external("l",long_av_a);
    n["l"].schema().print();
    long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_av[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",longlong_av_a);
    n["ll"].schema().print();
    longlong_ptr = n["ll"].as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_av[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_native_int_vec)
{
    char         char_av[6]  = {2,4,8,16,32,64};
    short        short_av[6] = {2,4,8,16,32,64};
    int          int_av[6]   = {2,4,8,16,32,64};
    long         long_av[6]  = {2,4,8,16,32,64};

#ifdef CONDUIT_HAS_LONG_LONG
    long long longlong_av[6]  = {2,4,8,16,32,64};
#endif

    std::vector<char>          char_v(char_av,char_av + 6);
    std::vector<short>         short_v(short_av,short_av+6);
    std::vector<int>           int_v(int_av,int_av+6);
    std::vector<long>          long_v(long_av,long_av+6);

#ifdef CONDUIT_HAS_LONG_LONG
    std::vector<long long>     longlong_v(longlong_av,longlong_av+6);
#endif

    Node n;

    ////////////////////////////
    // set path
    ////////////////////////////

    // char
    n.set_path("c",char_v);
    n["c"].schema().print();
    char *char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&char_ptr[i],&char_v[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char *char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short
    n.set_path("s",short_v);
    n["s"].schema().print();
    short *short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&short_ptr[i],&short_v[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short *short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path("i",int_v);
    n["i"].schema().print();
    int *int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&int_ptr[i],&int_v[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int *int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path("l",long_v);
    n["l"].schema().print();
    long *long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&long_ptr[i],&long_v[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long *long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",longlong_v);
    n["ll"].schema().print();
    long long *longlong_ptr = n["ll"].as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_v[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&longlong_ptr[i],&longlong_v[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    long long *longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external("c",char_v);
    n["c"].schema().print();
    char_ptr = n["c"].as_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(char_ptr[i],char_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&char_ptr[i],&char_v[i]);
    }

    EXPECT_EQ(char_ptr[5],64);

    // also check access via value()
    char_ptr_2 =  n["c"].value();
    EXPECT_EQ(char_ptr,char_ptr_2);

    // short
    n.set_path_external("s",short_v);
    n["s"].schema().print();
    short_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(short_ptr[i],short_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&short_ptr[i],&short_v[i]);
    }

    EXPECT_EQ(short_ptr[5],64);

    // also check access via value()
    short_ptr_2 = n["s"].value();
    EXPECT_EQ(short_ptr,short_ptr_2);

    // int
    n.set_path_external("i",int_v);
    n["i"].schema().print();
    int_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(int_ptr[i],int_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&int_ptr[i],&int_v[i]);
    }

    EXPECT_EQ(int_ptr[5],64);

    // also check access via value()
    int_ptr_2 = n["i"].value();
    EXPECT_EQ(int_ptr,int_ptr_2);

    // long
    n.set_path_external("l",long_v);
    n["l"].schema().print();
    long_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(long_ptr[i],long_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&long_ptr[i],&long_v[i]);
    }

    EXPECT_EQ(long_ptr[5],64);

    // also check access via value()
    long_ptr_2 = n["l"].value();
    EXPECT_EQ(long_ptr,long_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",longlong_v);
    n["ll"].schema().print();
    longlong_ptr = n["ll"].as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(longlong_ptr[i],longlong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&longlong_ptr[i],&longlong_v[i]);
    }

    EXPECT_EQ(longlong_ptr[5],64);

    // also check access via value()
    longlong_ptr_2 = n["ll"].value();
    EXPECT_EQ(longlong_ptr,longlong_ptr_2);

#endif

}

//-----------------------------------------------------------------------------
// set_path -- c style unsigned ptr, scalar, array, and vec
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_unsigned_int)
{
    unsigned char uchar_v = 2;
    unsigned short ushort_v = 4;
    unsigned int uint_v = 8;
    unsigned long ulong_v = 16;

    Node n;
    // char
    n.set_path("one/two/three", uchar_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node nc = n["one/two/three"];
    EXPECT_EQ(nc.as_unsigned_char(), uchar_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_unsigned_char(), 2);

    // short
    n.set_path("one/two/three", ushort_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_unsigned_short(), ushort_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_unsigned_short(), 4);

    // int
    n.set_path("one/two/three", uint_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_unsigned_int(), uint_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_unsigned_int(), 8);

    // long
    n.set_path("one/two/three", ulong_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_unsigned_long(), ulong_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_unsigned_long(), 16);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    unsigned long long ulong_long_v = 16;
    n.set_path("one/two/three", ulong_long_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_unsigned_long_long(), ulong_long_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), true);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_unsigned_long_long(), 16);
#endif

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_unsigned_int_ptr)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif

    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////
    
    // unsigned char
    n.set_path("uc",uchar_av,6);
    n["uc"].schema().print();
    unsigned char *uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path("us",ushort_av,6);
    n["us"].schema().print();
    unsigned short *ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);
    
    // also check access via value()
    unsigned short *ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);
    
    // unsigned int
    n.set_path("ui",uint_av,6);
    n["ui"].schema().print();
    unsigned int *uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);
    
    // unsigned long
    n.set_path("ul",ulong_av,6);
    n["ul"].schema().print();
    unsigned long *ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_av[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path("ull",ulonglong_av,6);
    n["ull"].schema().print();
    unsigned long long *ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_av[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // unsigned char
    n.set_path_external("uc",uchar_av,6);
    n["uc"].schema().print();
    uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path_external("us",ushort_av,6);
    n["us"].schema().print();
    ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_path_external("ui",uint_av,6);
    n["ui"].schema().print();
    uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_path_external("ul",ulong_av,6);
    n["ul"].schema().print();
    ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_av[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path_external("ull",ulonglong_av,6);
    n["ull"].schema().print();
    ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_av[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_unsigned_int_array)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif
    
    unsigned_char_array  uchar_av_a(uchar_av,DataType::c_unsigned_char(6));
    unsigned_short_array ushort_av_a(ushort_av,DataType::c_unsigned_short(6));
    unsigned_int_array   uint_av_a(uint_av,DataType::c_unsigned_int(6));
    unsigned_long_array  ulong_av_a(ulong_av,DataType::c_unsigned_long(6));
    
#if defined CONDUIT_HAS_LONG_LONG
    unsigned_long_long_array  ulonglong_av_a(ulonglong_av,DataType::c_unsigned_long_long(6));
#endif 
    
    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////
    
    // unsigned char
    n.set_path("uc",uchar_av_a);
    n["uc"].schema().print();
    unsigned char *uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path("us",ushort_av_a);
    n["us"].schema().print();
    unsigned short *ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);
    
    // also check access via value()
    unsigned short *ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);
    
    // unsigned int
    n.set_path("ui",uint_av_a);
    n["ui"].schema().print();
    unsigned int *uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);
    
    // unsigned long
    n.set_path("ul",ulong_av_a);
    n["ul"].schema().print();
    unsigned long *ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_av_a[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path("ull",ulonglong_av_a);
    n["ull"].schema().print();
    unsigned long long *ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av_a[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_av_a[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // unsigned char
    n.set_path_external("uc",uchar_av_a);
    n["uc"].schema().print();
    uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_av[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path_external("us",ushort_av_a);
    n["us"].schema().print();
    ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_av[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_path_external("ui",uint_av_a);
    n["ui"].schema().print();
    uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_av[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_path_external("ul",ulong_av_a);
    n["ul"].schema().print();
    ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_av_a[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path_external("ull",ulonglong_av_a);
    n["ull"].schema().print();
    ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_av_a[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_av_a[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_unsigned_int_vec)
{
    unsigned char   uchar_av[6]  = {2,4,8,16,32,64};
    unsigned short  ushort_av[6] = {2,4,8,16,32,64};
    unsigned int    uint_av[6]   = {2,4,8,16,32,64};
    unsigned long   ulong_av[6]  = {2,4,8,16,32,64};

#if defined CONDUIT_HAS_LONG_LONG
    unsigned long long   ulonglong_av[6]  = {2,4,8,16,32,64};
#endif


    std::vector<unsigned char>   uchar_v(uchar_av,uchar_av + 6);
    std::vector<unsigned short>  ushort_v(ushort_av,ushort_av+6);
    std::vector<unsigned int>    uint_v(uint_av,uint_av+6);
    std::vector<unsigned long>   ulong_v(ulong_av,ulong_av+6);

#ifdef CONDUIT_HAS_LONG_LONG
    std::vector<unsigned long long>     ulonglong_v(ulonglong_av,ulonglong_av+6);
#endif


    Node n;

    ////////////////////////////
    // set path
    ////////////////////////////

    // unsigned char
    n.set_path("uc",uchar_v);
    n["uc"].schema().print();
    unsigned char *uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uchar_ptr[i],&uchar_v[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    unsigned char *uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path("us",ushort_v);
    n["us"].schema().print();
    unsigned short *ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ushort_ptr[i],&ushort_v[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    unsigned short *ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_path("ui",uint_v);
    n["ui"].schema().print();
    unsigned int *uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&uint_ptr[i],&uint_v[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    unsigned int *uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_path("ul",ulong_v);
    n["ul"].schema().print();
    unsigned long *ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulong_ptr[i],&ulong_v[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    unsigned long *ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path("ull",ulonglong_v);
    n["ull"].schema().print();
    unsigned long long *ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ulonglong_ptr[i],&ulonglong_v[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    unsigned long long *ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // unsigned char
    n.set_path_external("uc",uchar_v);
    n["uc"].schema().print();
    uchar_ptr = n["uc"].as_unsigned_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uchar_ptr[i],uchar_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uchar_ptr[i],&uchar_v[i]);
    }

    EXPECT_EQ(uchar_ptr[5],64);

    // also check access via value()
    uchar_ptr_2 = n["uc"].value();
    EXPECT_EQ(uchar_ptr,uchar_ptr_2);

    // unsigned short
    n.set_path_external("us",ushort_v);
    n["us"].schema().print();
    ushort_ptr = n["us"].as_unsigned_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ushort_ptr[i],ushort_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ushort_ptr[i],&ushort_v[i]);
    }

    EXPECT_EQ(ushort_ptr[5],64);

    // also check access via value()
    ushort_ptr_2 = n["us"].value();
    EXPECT_EQ(ushort_ptr,ushort_ptr_2);

    // unsigned int
    n.set_path_external("ui",uint_v);
    n["ui"].schema().print();
    uint_ptr = n["ui"].as_unsigned_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(uint_ptr[i],uint_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&uint_ptr[i],&uint_v[i]);
    }

    EXPECT_EQ(uint_ptr[5],64);

    // also check access via value()
    uint_ptr_2 = n["ui"].value();
    EXPECT_EQ(uint_ptr,uint_ptr_2);

    // unsigned long
    n.set_path_external("ul",ulong_v);
    n["ul"].schema().print();
    ulong_ptr = n["ul"].as_unsigned_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulong_ptr[i],ulong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulong_ptr[i],&ulong_v[i]);
    }

    EXPECT_EQ(ulong_ptr[5],64);

    // also check access via value()
    ulong_ptr_2 = n["ul"].value();
    EXPECT_EQ(ulong_ptr,ulong_ptr_2);


#ifdef CONDUIT_HAS_LONG_LONG
    // unsigned long long
    n.set_path_external("ull",ulonglong_v);
    n["ull"].schema().print();
    ulonglong_ptr = n["ull"].as_unsigned_long_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(ulonglong_ptr[i],ulonglong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ulonglong_ptr[i],&ulonglong_v[i]);
    }

    EXPECT_EQ(ulonglong_ptr[5],64);

    // also check access via value()
    ulonglong_ptr_2 = n["ull"].value();
    EXPECT_EQ(ulonglong_ptr,ulonglong_ptr_2);
#endif

}

//-----------------------------------------------------------------------------
// set_path -- c style scalar, signed ptr, array, and vec
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_signed_int)
{
    signed char schar_v = 2;
    signed short sshort_v = 4;
    signed int sint_v = 8;
    signed long slong_v = 16;

    Node n;
    // char
    n.set_path("one/two/three", schar_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node nc = n["one/two/three"];
    EXPECT_EQ(nc.as_signed_char(), schar_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_signed_char(), 2);

    // short
    n.set_path("one/two/three", sshort_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_signed_short(), sshort_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_signed_short(), 4);

    // int
    n.set_path("one/two/three", sint_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_signed_int(), sint_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_signed_int(), 8);

    // long
    n.set_path("one/two/three", slong_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_signed_long(), slong_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_signed_long(), 16);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    signed long long slong_long_v = 16;
    n.set_path("one/two/three", slong_long_v);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_signed_long_long(), slong_long_v);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), true);
    EXPECT_EQ(nc.dtype().is_signed_integer(), true);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), false);
    EXPECT_EQ(nc.to_signed_long_long(), 16);
#endif

}



//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_signed_int_ptr)
{
    signed char  schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int   sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long  slong_av[6]  = {-2,-4,-8,-16,-32,-64};
#ifdef CONDUIT_HAS_LONG_LONG
    signed long long   slonglong_av[6]  = {-2,-4,-8,-16,-32,-64};
#endif

    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////
    
    // char
    n.set_path("c",schar_av,6);
    n["c"].schema().print();
    signed char *schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set_path("s",sshort_av,6);
    n["s"].schema().print();
    signed short *sshort_ptr = n["s"].as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    short *sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path("i",sint_av,6);
    n["i"].schema().print();
    signed int *sint_ptr = n["i"].as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    int *sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path("l",slong_av,6);
    n["l"].schema().print();
    signed long *slong_ptr = n["l"].as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);
   
    // also check access via value()
    signed long *slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",slonglong_av,6);
    n["ll"].schema().print();
    signed long long *slonglong_ptr = n["ll"].as_signed_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    signed long long *slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external("c",schar_av,6);
    n["c"].schema().print();
    schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set_path_external("s",sshort_av,6);
    n["s"].schema().print();
    sshort_ptr = n["s"].as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path_external("i",sint_av,6);
    n["i"].schema().print();
    sint_ptr = n["i"].as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path_external("l",slong_av,6);
    n["l"].schema().print();
    slong_ptr = n["l"].as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",slonglong_av,6);
    n["ll"].schema().print();
    slonglong_ptr = n["ll"].as_signed_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_signed_int_array)
{
    signed char  schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int   sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long  slong_av[6]  = {-2,-4,-8,-16,-32,-64};
#ifdef CONDUIT_HAS_LONG_LONG
    signed long long   slonglong_av[6]  = {-2,-4,-8,-16,-32,-64};
#endif
    
    signed_char_array  schar_av_a(schar_av,DataType::c_signed_char(6));
    signed_short_array sshort_av_a(sshort_av,DataType::c_signed_short(6));
    signed_int_array   sint_av_a(sint_av,DataType::c_signed_int(6));
    signed_long_array  slong_av_a(slong_av,DataType::c_signed_long(6));
    
#ifdef CONDUIT_HAS_LONG_LONG
   signed_long_long_array   slonglong_av_a(slonglong_av,
                                          DataType::c_signed_long_long(6));
#endif
    
    Node n;
    
    ////////////////////////////
    // set path 
    ////////////////////////////
    
    // char
    n.set_path("c",schar_av_a);
    n["c"].schema().print();
    signed char *schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set_path("s",sshort_av_a);
    n["s"].schema().print();
    signed short *sshort_ptr = n["s"].as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    short *sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path("i",sint_av_a);
    n["i"].schema().print();
    signed int *sint_ptr = n["i"].as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    int *sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path("l",slong_av_a);
    n["l"].schema().print();
    signed long *slong_ptr = n["l"].as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);
   
    // also check access via value()
    signed long *slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",slonglong_av_a);
    n["ll"].schema().print();
    signed long long *slonglong_ptr = n["ll"].as_signed_long_long_ptr();
    
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    signed long long *slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external("c",schar_av_a);
    n["c"].schema().print();
    schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_av[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short 
    n.set_path_external("s",sshort_av_a);
    n["s"].schema().print();
    sshort_ptr = n["s"].as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_av[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path_external("i",sint_av_a);
    n["i"].schema().print();
    sint_ptr = n["i"].as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_av[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path_external("l",slong_av_a);
    n["l"].schema().print();
    slong_ptr = n["l"].as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_av[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",slonglong_av_a);
    n["ll"].schema().print();
    slonglong_ptr = n["ll"].as_signed_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_av[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_av[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif
    
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_signed_int_vec)
{
    signed char  schar_av[6]  = {-2,-4,-8,-16,-32,-64};
    signed short sshort_av[6] = {-2,-4,-8,-16,-32,-64};
    signed int   sint_av[6]   = {-2,-4,-8,-16,-32,-64};
    signed long  slong_av[6]  = {-2,-4,-8,-16,-32,-64};

#ifdef CONDUIT_HAS_LONG_LONG
    signed long long slonglong_av[6]  = {-2,-4,-8,-16,-32,-64};
#endif

    std::vector<signed char>    schar_v(schar_av,schar_av + 6);
    std::vector<signed short>   sshort_v(sshort_av,sshort_av+6);
    std::vector<signed int>     sint_v(sint_av,sint_av+6);
    std::vector<signed long>    slong_v(slong_av,slong_av+6);

#ifdef CONDUIT_HAS_LONG_LONG
    std::vector<signed long long>  slonglong_v(slonglong_av,
                                               slonglong_av+6);
#endif

    Node n;

    ////////////////////////////
    // set path
    ////////////////////////////

    // char
    n.set_path("c",schar_v);
    n["c"].schema().print();
    signed char *schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&schar_ptr[i],&schar_v[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    signed char *schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short
    n.set_path("s",sshort_v);
    n["s"].schema().print();
    signed short *sshort_ptr = n["s"].as_signed_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sshort_ptr[i],&sshort_v[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    signed short *sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path("i",sint_v);
    n["i"].schema().print();
    signed int *sint_ptr = n["i"].as_signed_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&sint_ptr[i],&sint_v[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    signed int *sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path("l",slong_v);
    n["l"].schema().print();
    signed long *slong_ptr = n["l"].as_signed_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_v[i]);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&slong_ptr[i],&slong_v[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    signed long *slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path("ll",slonglong_v);
    n["ll"].schema().print();
    signed long long *slonglong_ptr = n["ll"].as_signed_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_v[i]);
        // set(...) semantics imply a copy -- mem addy should differ
        EXPECT_NE(&slonglong_ptr[i],&slonglong_v[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    signed long long *slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////

    // char
    n.set_path_external("c",schar_v);
    n["c"].schema().print();
    schar_ptr = n["c"].as_signed_char_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(schar_ptr[i],schar_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&schar_ptr[i],&schar_v[i]);
    }

    EXPECT_EQ(schar_ptr[5],-64);

    // also check access via value()
    schar_ptr_2 =  n["c"].value();
    EXPECT_EQ(schar_ptr,schar_ptr_2);

    // short
    n.set_path_external("s",sshort_v);
    n["s"].schema().print();
    sshort_ptr = n["s"].as_short_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sshort_ptr[i],sshort_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sshort_ptr[i],&sshort_v[i]);
    }

    EXPECT_EQ(sshort_ptr[5],-64);

    // also check access via value()
    sshort_ptr_2 = n["s"].value();
    EXPECT_EQ(sshort_ptr,sshort_ptr_2);

    // int
    n.set_path_external("i",sint_v);
    n["i"].schema().print();
    sint_ptr = n["i"].as_int_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(sint_ptr[i],sint_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&sint_ptr[i],&sint_v[i]);
    }

    EXPECT_EQ(sint_ptr[5],-64);

    // also check access via value()
    sint_ptr_2 = n["i"].value();
    EXPECT_EQ(sint_ptr,sint_ptr_2);

    // long
    n.set_path_external("l",slong_v);
    n["l"].schema().print();
    slong_ptr = n["l"].as_long_ptr();
    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slong_ptr[i],slong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slong_ptr[i],&slong_v[i]);
    }

    EXPECT_EQ(slong_ptr[5],-64);

    // also check access via value()
    slong_ptr_2 = n["l"].value();
    EXPECT_EQ(slong_ptr,slong_ptr_2);

#ifdef CONDUIT_HAS_LONG_LONG
    // long long
    n.set_path_external("ll",slonglong_v);
    n["ll"].schema().print();
    slonglong_ptr = n["ll"].as_long_long_ptr();

    for(index_t i=0;i<6;i++)
    {
        EXPECT_EQ(slonglong_ptr[i],slonglong_v[i]);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&slonglong_ptr[i],&slonglong_v[i]);
    }

    EXPECT_EQ(slonglong_ptr[5],-64);

    // also check access via value()
    slonglong_ptr_2 = n["ll"].value();
    EXPECT_EQ(slonglong_ptr,slonglong_ptr_2);

#endif

}

//-----------------------------------------------------------------------------
// set_path -- c style float scalar, ptr, array, and vec
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_float)
{
    float fv = -0.8f;
    double dv = -1.6;

    Node n;
    // float
    n.set_path("one/two/three", fv);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    Node nc = n["one/two/three"];
    EXPECT_EQ(nc.as_float(), fv);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), false);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), true);
    EXPECT_NEAR(nc.to_float(), -0.8, 0.001);

    // double
    n.set_path("one/two/three", dv);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_double(), dv);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), false);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), true);
    EXPECT_NEAR(nc.to_double(), -1.6, 0.001);

#ifdef CONDUIT_USE_LONG_DOUBLE
    // long double
    long double  ldav  -1.6;
    n.set_path("one/two/three", dv);
    EXPECT_TRUE(n.has_path("one"));
    EXPECT_TRUE(n["one"].has_path("two/three"));
    nc = n["one/two/three"];
    EXPECT_EQ(nc.as_double(), dv);
    EXPECT_EQ(nc.dtype().is_number(), true);
    EXPECT_EQ(nc.dtype().is_integer(), false);
    EXPECT_EQ(nc.dtype().is_signed_integer(), false);
    EXPECT_EQ(nc.dtype().is_unsigned_integer(), false);
    EXPECT_EQ(nc.dtype().is_floating_point(), true);
    EXPECT_NEAR(nc.to_long_double(), -1.6, 0.001);
#endif

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_float_ptr)
{
    float   fav[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    double  dav[4] = {-0.8, -1.6, -3.2, -6.4};

#ifdef CONDUIT_USE_LONG_DOUBLE
    long double  ldav[4] = {-0.8, -1.6, -3.2, -6.4};
#endif

    Node n;

    ////////////////////////////
    // set path 
    ////////////////////////////

    // float
    n.set_path("f",fav,4);
    n["f"].schema().print();
    float *f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav[i]); 
    }

    EXPECT_NEAR(f_ptr[3],-6.4,0.001);

    // also check access via value()
    float *f_ptr_2 = n["f"].value();
    EXPECT_EQ(f_ptr,f_ptr_2);
    
    // double
    n.set_path("d",dav,4);
    n["d"].schema().print();
    double *d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav[i]);
    }

    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    // also check access via value()
    double *d_ptr_2 = n["d"].value();
    EXPECT_EQ(d_ptr,d_ptr_2);


#ifdef CONDIT_USE_LONG_DOUBLE

    // long_double
    n.set_path("ld",ldav,4);
    n["ld"].schema().print();
    long double *ld_ptr = n["ld"].as_long_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(ld_ptr[i],ldav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&ld_ptr[i],&ldav[i]);
    }
    EXPECT_NEAR(ld_ptr[3],-6.4,0.001);
    
    // also check access via value()
    long double *ld_ptr_2 = n["ld"].value();
    EXPECT_EQ(ld_ptr,ld_ptr_2);

#endif

    ////////////////////////////
    // set path external
    ////////////////////////////
    
    // float
    n.set_path_external("f",fav,4);
    n["f"].schema().print();
    f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&f_ptr[i],&fav[i]); 
    }

    EXPECT_NEAR(f_ptr[3],-6.4,0.001);

    // also check access via value()
    f_ptr_2 = n["f"].value();
    EXPECT_EQ(f_ptr,f_ptr_2);

    // double
    n.set_path_external("d",dav,4);
    n["d"].schema().print();
    d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&d_ptr[i],&dav[i]);
    }

    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    // also check access via value()
    d_ptr_2 = n["d"].value();
    EXPECT_EQ(d_ptr,d_ptr_2);


#ifdef CONDIT_USE_LONG_DOUBLE

    // long_double
    n.set_path_extenral("ld",ldav,4);
    n["ld"].schema().print();
    ld_ptr = n["ld"].as_long_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(ld_ptr[i],ldav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&ld_ptr[i],&ldav[i]);
    }
    EXPECT_NEAR(ld_ptr[3],-6.4,0.001);

    // also check access via value()
    ld_ptr_2 = n["ld"].value();
    EXPECT_EQ(ld_ptr,ld_ptr_2);

#endif
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_float_array)
{
    float   fav[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    double  dav[4] = {-0.8, -1.6, -3.2, -6.4};

    float_array  fav_a(fav,DataType::c_float(4));
    double_array dav_a(dav,DataType::c_double(4));

    ////////////////////////////
    // set path 
    ////////////////////////////

    Node n;
    // float
    n.set_path("f",fav_a);
    n["f"].schema().print();
    float *f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_path("d",dav_a);
    n["d"].schema().print();
    double *d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    ////////////////////////////
    // set path external
    ////////////////////////////

    // float
    n.set_path_external("f",fav_a);
    n["f"].schema().print();
    f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_path_external("d",dav_a);
    n["d"].schema().print();
    d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_cstyle_float_vec)
{
    std::vector<float>  fav;
    fav.push_back(-0.8f);
    fav.push_back(-1.6f);
    fav.push_back(-3.2f);
    fav.push_back(-6.4f);
    
    std::vector<double> dav;
    dav.push_back(-0.8);
    dav.push_back(-1.6);
    dav.push_back(-3.2);
    dav.push_back(-6.4);

    ////////////////////////////
    // set path 
    ////////////////////////////

    Node n;
    // float
    n.set_path("f",fav);
    n["f"].schema().print();
    float *f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_path("d",dav);
    n["d"].schema().print();
    double *d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set(...) semantics imply a copy -- mem addys should differ
        EXPECT_NE(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

    ////////////////////////////
    // set path external
    ////////////////////////////

    // float
    n.set_path_external("f",fav);
    n["f"].schema().print();
    f_ptr = n["f"].as_float_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f_ptr[i],fav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&f_ptr[i],&fav[i]); 
    }
    EXPECT_NEAR(f_ptr[3],-6.4,0.001);
    
    // double
    n.set_path_external("d",dav);
    n["d"].schema().print();
    d_ptr = n["d"].as_double_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(d_ptr[i],dav[i],0.001);
        // set_external(...) semantics implies zero-copy -- mem addys should equal
        EXPECT_EQ(&d_ptr[i],&dav[i]);
    }
    EXPECT_NEAR(d_ptr[3],-6.4,0.001);

}

//-----------------------------------------------------------------------------
// end c style set and set path checks
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_path_external_node)
{
    float32  f32av[4] = {-0.8f, -1.6f, -3.2f, -6.4f};
    float64  f64av[4] = {-0.8, -1.6, -3.2, -6.4};

    Node nsrc;
    // float32
    nsrc.set_path_external("two/lvl",f32av,4);
    nsrc.schema().print();
    nsrc.print();

    
    Node n;
    n.set_external(nsrc);
    
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*4);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),4);
    float32 *f32av_ptr = n["two/lvl"].as_float32_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f32av_ptr[i],f32av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f32av_ptr[i],&f32av[i]);
    }
    EXPECT_NEAR(f32av_ptr[3],-6.4,0.001);
    f32av_ptr[1] = -110.1f;
    EXPECT_NEAR(f32av[1],-110.1,0.001);
    n.print();
    
    
    // float64
    nsrc.set_path_external("two/lvl",f64av,4);
    nsrc.schema().print();
    nsrc.print();
    
    n.set_external(nsrc);
    n.schema().print();
    EXPECT_TRUE(n.has_path("two"));
    EXPECT_TRUE(n["two"].has_path("lvl"));
    EXPECT_EQ(n["two"]["lvl"].total_strided_bytes(),4*8);
    EXPECT_EQ(n["two"]["lvl"].dtype().element_bytes(),8);
    float64 *f64av_ptr = n["two/lvl"].as_float64_ptr();
    for(index_t i=0;i<4;i++)
    {
        EXPECT_NEAR(f64av_ptr[i],f64av[i],0.001);
        // set_external(...) semantics imply a ref -- mem addys should match
        EXPECT_EQ(&f64av_ptr[i],&f64av[i]);
    }
    EXPECT_NEAR(f64av_ptr[3],-6.4,0.001);
    f64av_ptr[1] = -110.1;
    EXPECT_NEAR(f64av[1],-110.1,0.001);
    n.print();
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_string_multiple)
{
    Node n;
    n.set_string("I am a long string");
    EXPECT_EQ(n.as_string(),"I am a long string");
    n.set_string("I am a longer string than before");  // OK
    EXPECT_EQ(n.as_string(),"I am a longer string than before");
    n.set_string("shorter");    // was crashing ..
    EXPECT_EQ(n.as_string(),"shorter");
}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_vector)
{
    
    std::vector<int8>  i8_vec(1);
    std::vector<int16> i16_vec(1);
    std::vector<int32> i32_vec(1);
    std::vector<int64> i64_vec(1);

    i8_vec[0]  = -8;
    i16_vec[0] = -16;
    i32_vec[0] = -32;
    i64_vec[0] = -64;
    
    std::vector<uint8>  ui8_vec(1);
    std::vector<uint16> ui16_vec(1);
    std::vector<uint32> ui32_vec(1);
    std::vector<uint64> ui64_vec(1);
    
    ui8_vec[0]  = 8;
    ui16_vec[0] = 16;
    ui32_vec[0] = 32;
    ui64_vec[0] = 64;
    
    std::vector<float32> f32_vec(1);
    std::vector<float64> f64_vec(1);
    f32_vec[0] = 2.71828f;
    f64_vec[0] = 3.1415;
    

    // set 
    Node n1;
    n1["i8"].set(i8_vec);
    n1["i16"].set(i16_vec);
    n1["i32"].set(i32_vec);
    n1["i64"].set(i64_vec);

    n1["ui8"].set(ui8_vec);
    n1["ui16"].set(ui16_vec);
    n1["ui32"].set(ui32_vec);
    n1["ui64"].set(ui64_vec);

    n1["f32"].set(f32_vec);
    n1["f64"].set(f64_vec);

    n1.print();

    // set_path

    Node n2;
    n2.set_path("i8",i8_vec);
    n2.set_path("i16",i16_vec);
    n2.set_path("i32",i32_vec);
    n2.set_path("i64",i64_vec);

    n2.set_path("ui8",ui8_vec);
    n2.set_path("ui16",ui16_vec);
    n2.set_path("ui32",ui32_vec);
    n2.set_path("ui64",ui64_vec);

    n2.set_path("f32",f32_vec);
    n2.set_path("f64",f64_vec);


    n2.print();

    EXPECT_TRUE(n1.schema().equals(n2.schema()));

}

//-----------------------------------------------------------------------------
TEST(conduit_node_set, set_vector_external)
{
    
    std::vector<int8>  i8_vec(1);
    std::vector<int16> i16_vec(1);
    std::vector<int32> i32_vec(1);
    std::vector<int64> i64_vec(1);

    std::vector<uint8>  ui8_vec(1);
    std::vector<uint16> ui16_vec(1);
    std::vector<uint32> ui32_vec(1);
    std::vector<uint64> ui64_vec(1);

    std::vector<float32> f32_vec(1);
    std::vector<float64> f64_vec(1);

    // set 
    Node n1;
    n1["i8"].set_external(i8_vec);
    n1["i16"].set_external(i16_vec);
    n1["i32"].set_external(i32_vec);
    n1["i64"].set_external(i64_vec);

    n1["ui8"].set_external(ui8_vec);
    n1["ui16"].set_external(ui16_vec);
    n1["ui32"].set_external(ui32_vec);
    n1["ui64"].set_external(ui64_vec);

    n1["f32"].set_external(f32_vec);
    n1["f64"].set_external(f64_vec);

    n1.print();

    // set_path

    Node n2;
    n2.set_path_external("i8",i8_vec);
    n2.set_path_external("i16",i16_vec);
    n2.set_path_external("i32",i32_vec);
    n2.set_path_external("i64",i64_vec);

    n2.set_path_external("ui8",ui8_vec);
    n2.set_path_external("ui16",ui16_vec);
    n2.set_path_external("ui32",ui32_vec);
    n2.set_path_external("ui64",ui64_vec);

    n2.set_path_external("f32",f32_vec);
    n2.set_path_external("f64",f64_vec);


    i8_vec[0]  = -8;
    i16_vec[0] = -16;
    i32_vec[0] = -32;
    i64_vec[0] = -64;

    
    ui8_vec[0]  = 8;
    ui16_vec[0] = 16;
    ui32_vec[0] = 32;
    ui64_vec[0] = 64;
    
    f32_vec[0] = 2.71828f;
    f64_vec[0] = 3.1415;
    
    n1.print();
    n2.print();

    EXPECT_TRUE(n1.schema().equals(n2.schema()));
    
    int8  *i8_ptr  = n1["i8"].value();
    int16 *i16_ptr = n1["i16"].value();
    int32 *i32_ptr = n1["i32"].value();
    int64 *i64_ptr = n1["i64"].value();
    
    EXPECT_EQ(i8_ptr,&i8_vec[0]);
    EXPECT_EQ(i16_ptr,&i16_vec[0]);
    EXPECT_EQ(i32_ptr,&i32_vec[0]);
    EXPECT_EQ(i64_ptr,&i64_vec[0]);
    
    uint8  *ui8_ptr  = n1["ui8"].value();
    uint16 *ui16_ptr = n1["ui16"].value();
    uint32 *ui32_ptr = n1["ui32"].value();
    uint64 *ui64_ptr = n1["ui64"].value();
    
    EXPECT_EQ(ui8_ptr,&ui8_vec[0]);
    EXPECT_EQ(ui16_ptr,&ui16_vec[0]);
    EXPECT_EQ(ui32_ptr,&ui32_vec[0]);
    EXPECT_EQ(ui64_ptr,&ui64_vec[0]);
    
    float32 *f32_ptr = n1["f32"].value();
    float64 *f64_ptr = n1["f64"].value();
    
    EXPECT_EQ(f32_ptr,&f32_vec[0]);
    EXPECT_EQ(f64_ptr,&f64_vec[0]);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, node_set_existing_char8)
{
    Schema s;
    
    std::string value = "my value";
    
    s["a"].set(DataType::int64());
    s["b"].set(DataType::char8_str(value.length()+1));
    
    Schema s_compact;
    s.compact_to(s_compact);
    
    
    Node n2;
    n2.set_external_char8_str(const_cast<char*>(value.c_str()));
    
    Node n(s_compact);
    
    n["a"].set_int64(10);
    n["b"].update(n2);
    
    EXPECT_EQ(n["a"].as_int64(),10);
    EXPECT_EQ(n["b"].as_string(),value);
    
    n.print();
    


    n["b"].set(value);

    n.print();

    EXPECT_EQ(n["b"].as_string(),value);
}

//-----------------------------------------------------------------------------
TEST(conduit_node, node_set_existing_obj)
{
    Node n_init;

    n_init["a"] = DataType::list();
    
    CONDUIT_INFO("INITIAL");
    CONDUIT_INFO(n_init.to_json());

    Node n_des;
    n_des["a"].append().set("value");

    
    CONDUIT_INFO("DES");
    CONDUIT_INFO(n_des.to_json());
    
    n_init = n_des;
    
    EXPECT_EQ(n_init.number_of_children(),1);
    
    CONDUIT_INFO("POST SET");
    CONDUIT_INFO(n_init.to_json());

}

//-----------------------------------------------------------------------------
TEST(conduit_node, node_set_non_compact_dtype)
{
    // These checks node set for a non-compact datatype
    // matches the results of node set for a schema with
    // the same data type.
    
    Node n1;

    DataType dt = DataType::c_int(2,0,sizeof(int)*2);
    Schema s(dt);
    n1.set(s);
    
    n1.info().print();
    
    int_array n1_vals = n1.value();
    
    n1_vals[0] = 10;
    n1_vals[1] = 20;
    
    Node n2;
    n2.set(dt);

    n2.info().print();
    
    int_array n2_vals = n1.value();
    
    n2_vals[0] = 10;
    n2_vals[1] = 20;
    
    
    EXPECT_EQ(n1.dtype().strided_bytes(),n2.dtype().strided_bytes());
    EXPECT_EQ(n1.total_bytes_allocated(),n2.total_bytes_allocated());
    
    
}


//-----------------------------------------------------------------------------
TEST(conduit_array, cxx_11_init_lists)
{
    Node n, n_check, n_info;

    // int 
    {
        std::vector<int> v = {-1,2,-3,4,-5};
        n_check = v;

        n.set({-1,2,-3,4,-5});
        n.print();
        EXPECT_TRUE(n.dtype().is_int());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {-1,2,-3,4,-5};
        n.print();
        EXPECT_TRUE(n.dtype().is_int());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }

    // uint
    {
        std::vector<unsigned int> v = {1,2,3,4,5};
        n_check = v;

        n.set({1u,2u,3u,4u,5u});
        n.print();
        EXPECT_TRUE(n.dtype().is_unsigned_int());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {1u,2u,3u,4u,5u};
        n.print();
        EXPECT_TRUE(n.dtype().is_unsigned_int());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }

    // long
    {
        std::vector<long> v = {-1,2,-3,4,-5};
        n_check = v;

        n.set({-1l,2l,-3l,4l,-5l});
        n.print();
        EXPECT_TRUE(n.dtype().is_long());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {-1l,2l,-3l,4l,-5l};
        n.print();
        EXPECT_TRUE(n.dtype().is_long());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }

    // ulong
    {
        std::vector<unsigned long> v = {1,2,3,4,5};
        n_check = v;

        n.set({1ul,2ul,3ul,4ul,5ul});
        n.print();
        EXPECT_TRUE(n.dtype().is_unsigned_long());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {1ul,2ul,3ul,4ul,5ul};
        n.print();
        EXPECT_TRUE(n.dtype().is_unsigned_long());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }

    // float
    {
        std::vector<float> v = {1.0,2.0,3.0,4.0,5.0};
        n_check = v;

        n.set({1.0f,2.0f,3.0f,4.0f,5.0f});
        n.print();
        EXPECT_TRUE(n.dtype().is_float());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {1.0f,2.0f,3.0f,4.0f,5.0f};
        n.print();
        EXPECT_TRUE(n.dtype().is_float());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }
    

    // double
    {
        std::vector<double> v = {1.0,2.0,3.0,4.0,5.0};
        n_check = v;

        n.set({1.0,2.0,3.0,4.0,5.0});
        n.print();
        EXPECT_TRUE(n.dtype().is_double());
        EXPECT_FALSE(n.diff(n_check,n_info));

        n = {1.0,2.0,3.0,4.0,5.0};
        n.print();
        EXPECT_TRUE(n.dtype().is_double());
        EXPECT_FALSE(n.diff(n_check,n_info));
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_node, node_set_with_offset)
{
    Node n;

    conduit::float64  vals[6] = { 1.0, 2.0,
                                  1.0, 2.0,
                                  1.0, 2.0,};

    n["v1"].set(vals,
                3,
                0,
                2 * sizeof(conduit::float64));
    n["v2"].set(vals,
                3,
                sizeof(conduit::float64),
                2 * sizeof(conduit::float64));
    n.print();

    float64_array av1 = n["v1"].value();
    float64_array av2 = n["v2"].value();
    
    EXPECT_EQ(av1[0],1.0);
    EXPECT_EQ(av1[1],1.0);
    EXPECT_EQ(av1[2],1.0);

    EXPECT_EQ(av2[0],2.0);
    EXPECT_EQ(av2[1],2.0);
    EXPECT_EQ(av2[2],2.0);

}
