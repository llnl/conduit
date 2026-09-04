// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_array.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_execution.hpp"
#include "conduit_memory_manager.hpp"
#include "execution_test_utils.hpp"

#include <iostream>
#include <vector>
#include "gtest/gtest.h"

using namespace conduit;
using conduit::execution::ExecutionPolicy;

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, value)
{

    Node n;
    n.set((int8)10);

    int8_accessor  i8_acc  = n.value();
    int16_accessor i16_acc = n.value();
    int32_accessor i32_acc = n.value();
    int64_accessor i64_acc = n.value();

    uint8_accessor  ui8_acc  = n.value();
    uint16_accessor ui16_acc = n.value();
    uint32_accessor ui32_acc = n.value();
    uint64_accessor ui64_acc = n.value();
    
    
    float32_accessor f32_acc = n.value();
    float64_accessor f64_acc = n.value();
    
    EXPECT_EQ(i8_acc[0],(int8)(10));
    EXPECT_EQ(i16_acc[0],(int16)(10));
    EXPECT_EQ(i32_acc[0],(int32)(10));
    EXPECT_EQ(i64_acc[0],(int64)(10));
    
    
    EXPECT_EQ(ui8_acc[0],(uint8)(10));
    EXPECT_EQ(ui16_acc[0],(uint16)(10));
    EXPECT_EQ(ui32_acc[0],(uint32)(10));
    EXPECT_EQ(ui64_acc[0],(uint64)(10));

    EXPECT_EQ(f32_acc[0],(float32)(10));
    EXPECT_EQ(f64_acc[0],(float64)(10));

}


//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, as_bitwidth_style)
{

    Node n;
    n.set((int8)10);

    int8_accessor  i8_acc  = n.as_int8_accessor();
    int16_accessor i16_acc = n.as_int16_accessor();
    int32_accessor i32_acc = n.as_int32_accessor();
    int64_accessor i64_acc = n.as_int64_accessor();

    uint8_accessor  ui8_acc  = n.as_uint8_accessor();
    uint16_accessor ui16_acc = n.as_uint16_accessor();
    uint32_accessor ui32_acc = n.as_uint32_accessor();
    uint64_accessor ui64_acc = n.as_uint64_accessor();
    
    
    float32_accessor f32_acc = n.as_float32_accessor();
    float64_accessor f64_acc = n.as_float64_accessor();
    
    EXPECT_EQ(i8_acc[0],(int8)(10));
    EXPECT_EQ(i16_acc[0],(int16)(10));
    EXPECT_EQ(i32_acc[0],(int32)(10));
    EXPECT_EQ(i64_acc[0],(int64)(10));
    
    
    EXPECT_EQ(ui8_acc[0],(uint8)(10));
    EXPECT_EQ(ui16_acc[0],(uint16)(10));
    EXPECT_EQ(ui32_acc[0],(uint32)(10));
    EXPECT_EQ(ui64_acc[0],(uint64)(10));

    EXPECT_EQ(f32_acc[0],(float32)(10));
    EXPECT_EQ(f64_acc[0],(float64)(10));

}


//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, summary_stats)
{
    std::vector<int64>   v_int64   = {-1,0,1};
    std::vector<uint64>  v_uint64  = {1,2,3};
    std::vector<float64> v_float64 = {-1.0,0.0,1.0};

    // void* / DataType constructor variants
    {
        int64_accessor   va_int64(&v_int64[0],DataType::int64(3));
        uint64_accessor  va_uint64(&v_uint64[0],DataType::uint64(3));
        float64_accessor va_float64(&v_float64[0],DataType::float64(3));

        EXPECT_EQ(va_int64.min(),-1);
        EXPECT_EQ(va_int64.max(),1);
        EXPECT_EQ(va_int64.mean(),0);
        EXPECT_EQ(va_int64.sum(),0);
        EXPECT_EQ(va_int64.count(-1),1);

        EXPECT_EQ(va_uint64.min(),1);
        EXPECT_EQ(va_uint64.max(),3);
        EXPECT_EQ(va_uint64.mean(),2);
        EXPECT_EQ(va_uint64.sum(),6);
        EXPECT_EQ(va_uint64.count(2),1);

        EXPECT_EQ(va_float64.min(),-1.0);
        EXPECT_EQ(va_float64.max(),1.0);
        EXPECT_EQ(va_float64.mean(),0.0);
        EXPECT_EQ(va_float64.sum(),0.0);
        EXPECT_EQ(va_float64.count(0.0),1);
    }

    // Node-backed constructor variants
    Node n;
    n["int64"].set(std::vector<int64>{-1,0,1});
    n["uint64"].set(std::vector<uint64>{1,2,3});
    n["float64"].set(std::vector<float64>{-1.0,0.0,1.0});

    // Node ref variant
    {
        int64_accessor   va_int64(n["int64"]);
        uint64_accessor  va_uint64(n["uint64"]);
        float64_accessor va_float64(n["float64"]);

        EXPECT_EQ(va_int64.min(),-1);
        EXPECT_EQ(va_int64.max(),1);
        EXPECT_EQ(va_int64.mean(),0);
        EXPECT_EQ(va_int64.sum(),0);
        EXPECT_EQ(va_int64.count(-1),1);

        EXPECT_EQ(va_uint64.min(),1);
        EXPECT_EQ(va_uint64.max(),3);
        EXPECT_EQ(va_uint64.mean(),2);
        EXPECT_EQ(va_uint64.sum(),6);
        EXPECT_EQ(va_uint64.count(2),1);

        EXPECT_EQ(va_float64.min(),-1.0);
        EXPECT_EQ(va_float64.max(),1.0);
        EXPECT_EQ(va_float64.mean(),0.0);
        EXPECT_EQ(va_float64.sum(),0.0);
        EXPECT_EQ(va_float64.count(0.0),1);
    }

    // const Node ref variant
    {
        const Node &cn = n;
        int64_accessor va_int64_c(cn["int64"]);
        EXPECT_EQ(va_int64_c.min(),-1);
        EXPECT_EQ(va_int64_c.max(),1);
        EXPECT_EQ(va_int64_c.mean(),0);
        EXPECT_EQ(va_int64_c.sum(),0);
        EXPECT_EQ(va_int64_c.count(-1),1);
    }

    // Node ptr variant
    {
        int64_accessor va_uint64_p(&n["uint64"]);
        EXPECT_EQ(va_uint64_p.min(),1);
        EXPECT_EQ(va_uint64_p.max(),3);
        EXPECT_EQ(va_uint64_p.mean(),2);
        EXPECT_EQ(va_uint64_p.sum(),6);
        EXPECT_EQ(va_uint64_p.count(2),1);
    }

    // const Node ptr variant
    {
        const Node *cnp = &n["float64"];
        int64_accessor va_float64_cnp(cnp);
        EXPECT_EQ(va_float64_cnp.min(),-1.0);
        EXPECT_EQ(va_float64_cnp.max(),1.0);
        EXPECT_EQ(va_float64_cnp.mean(),0.0);
        EXPECT_EQ(va_float64_cnp.sum(),0.0);
        EXPECT_EQ(va_float64_cnp.count(0.0),1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, as_cstyle)
{

    Node n;
    n.set((int8)10);

    char_accessor         c_acc  = n.as_char_accessor();
    signed_char_accessor  sc_acc = n.as_signed_char_accessor();
    signed_short_accessor ss_acc = n.as_signed_short_accessor();
    signed_int_accessor   si_acc = n.as_signed_int_accessor();
    signed_long_accessor  sl_acc = n.as_signed_long_accessor();

#ifdef CONDUIT_HAS_LONG_LONG
    signed_long_long_accessor  sll_acc = n.as_signed_long_long_accessor();
#endif

    unsigned_char_accessor  usc_acc = n.as_unsigned_char_accessor();
    unsigned_short_accessor uss_acc = n.as_unsigned_short_accessor();
    unsigned_int_accessor   usi_acc = n.as_unsigned_int_accessor();
    unsigned_long_accessor  usl_acc = n.as_unsigned_long_accessor();

#ifdef CONDUIT_HAS_LONG_LONG
    unsigned_long_long_accessor  usll_acc = n.as_unsigned_long_long_accessor();
#endif

    float_accessor  f_acc = n.as_float_accessor();
    double_accessor d_acc = n.as_double_accessor();

#ifdef CONDUIT_USE_LONG_DOUBLE
    long_double_accessor  ld_acc = n.as_long_double_accessor();
#endif

    EXPECT_EQ(c_acc[0],(char)(10));
    EXPECT_EQ(sc_acc[0],(signed char)(10));
    EXPECT_EQ(ss_acc[0],(signed short)(10));
    EXPECT_EQ(si_acc[0],(signed int)(10));
    EXPECT_EQ(sl_acc[0],(signed long)(10));

#ifdef CONDUIT_HAS_LONG_LONG
    EXPECT_EQ(sll_acc[0],(signed long long)(10));
#endif

    EXPECT_EQ(usc_acc[0],(unsigned char)(10));
    EXPECT_EQ(uss_acc[0],(unsigned short)(10));
    EXPECT_EQ(usi_acc[0],(unsigned int)(10));
    EXPECT_EQ(usl_acc[0],(unsigned long)(10));

#ifdef CONDUIT_HAS_LONG_LONG
    EXPECT_EQ(usll_acc[0],(unsigned long long)(10));
#endif

    EXPECT_EQ(f_acc[0],(float)(10));
    EXPECT_EQ(d_acc[0],(double)(10));

#ifdef CONDUIT_USE_LONG_DOUBLE
    EXPECT_EQ(ld_acc[0],(long double)(10));
#endif 

}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, default_construct)
{
    index_t_accessor n_acc;
    Node n;
    n.set({-1,2,-3,4,-5});

    n_acc = n.value();
    EXPECT_EQ(n_acc[0],(index_t)(-1));
    EXPECT_EQ(n_acc[1],(index_t)( 2));
    EXPECT_EQ(n_acc[2],(index_t)(-3));
    EXPECT_EQ(n_acc[3],(index_t)( 4));
    EXPECT_EQ(n_acc[4],(index_t)(-5));
}


//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, set)
{
    // Value variant
    {
        Node n;
        n.set(DataType::int8(10));

        int8_accessor  i8_acc  = n.value();
        int16_accessor i16_acc = n.value();
        int32_accessor i32_acc = n.value();
        int64_accessor i64_acc = n.value();

        uint8_accessor  ui8_acc  = n.value();
        uint16_accessor ui16_acc = n.value();
        uint32_accessor ui32_acc = n.value();
        uint64_accessor ui64_acc = n.value();

        float32_accessor f32_acc = n.value();
        float64_accessor f64_acc = n.value();

        i8_acc.set(0,-4);
        i16_acc.set(1,-8);
        i32_acc.set(2,-16);
        i64_acc.set(3,-32);

        ui8_acc.set(4, 4);
        ui16_acc.set(5,8);
        ui32_acc.set(6,16);
        ui64_acc.set(7,32);

        f32_acc.set(8,16);
        f64_acc.set(9,32);

        EXPECT_EQ(i32_acc[0],-4);
        EXPECT_EQ(i32_acc[1],-8);
        EXPECT_EQ(i32_acc[2],-16);
        EXPECT_EQ(i32_acc[3],-32);

        EXPECT_EQ(i32_acc[4],4);
        EXPECT_EQ(i32_acc[5],8);
        EXPECT_EQ(i32_acc[6],16);
        EXPECT_EQ(i32_acc[7],32);

        EXPECT_EQ(i32_acc[8],16);
        EXPECT_EQ(i32_acc[9],32);
    }

    // Node ref variant
    {
        Node n;
        n.set(DataType::int8(10));
    
        int8_accessor  i8_acc(n);
        int16_accessor i16_acc(n);
        int32_accessor i32_acc(n);
        int64_accessor i64_acc(n);
    
        uint8_accessor  ui8_acc(n);
        uint16_accessor ui16_acc(n);
        uint32_accessor ui32_acc(n);
        uint64_accessor ui64_acc(n);
    
        float32_accessor f32_acc(n);
        float64_accessor f64_acc(n);
    
        i8_acc.set(0,-4);
        i16_acc.set(1,-8);
        i32_acc.set(2,-16);
        i64_acc.set(3,-32);
    
        ui8_acc.set(4, 4);
        ui16_acc.set(5,8);
        ui32_acc.set(6,16);
        ui64_acc.set(7,32);
    
        f32_acc.set(8,16);
        f64_acc.set(9,32);
    
        EXPECT_EQ(i32_acc[0],-4);
        EXPECT_EQ(i32_acc[1],-8);
        EXPECT_EQ(i32_acc[2],-16);
        EXPECT_EQ(i32_acc[3],-32);
    
        EXPECT_EQ(i32_acc[4],4);
        EXPECT_EQ(i32_acc[5],8);
        EXPECT_EQ(i32_acc[6],16);
        EXPECT_EQ(i32_acc[7],32);
    
        EXPECT_EQ(i32_acc[8],16);
        EXPECT_EQ(i32_acc[9],32);
    }

    // const Node ref variant
    {
        Node n;
        n.set(DataType::int8(10));
        const Node &cn = n;

        int8_accessor  i8_acc(cn);
        int16_accessor i16_acc(cn);
        int32_accessor i32_acc(cn);
        int64_accessor i64_acc(cn);

        uint8_accessor  ui8_acc(cn);
        uint16_accessor ui16_acc(cn);
        uint32_accessor ui32_acc(cn);
        uint64_accessor ui64_acc(cn);

        float32_accessor f32_acc(cn);
        float64_accessor f64_acc(cn);

        i8_acc.set(0,-4);
        i16_acc.set(1,-8);
        i32_acc.set(2,-16);
        i64_acc.set(3,-32);

        ui8_acc.set(4, 4);
        ui16_acc.set(5,8);
        ui32_acc.set(6,16);
        ui64_acc.set(7,32);

        f32_acc.set(8,16);
        f64_acc.set(9,32);

        EXPECT_EQ(i32_acc[0],-4);
        EXPECT_EQ(i32_acc[1],-8);
        EXPECT_EQ(i32_acc[2],-16);
        EXPECT_EQ(i32_acc[3],-32);

        EXPECT_EQ(i32_acc[4],4);
        EXPECT_EQ(i32_acc[5],8);
        EXPECT_EQ(i32_acc[6],16);
        EXPECT_EQ(i32_acc[7],32);

        EXPECT_EQ(i32_acc[8],16);
        EXPECT_EQ(i32_acc[9],32);
    }

    // Node ptr variant
    {
        Node n;
        n.set(DataType::int8(10));

        int8_accessor  i8_acc(&n);
        int16_accessor i16_acc(&n);
        int32_accessor i32_acc(&n);
        int64_accessor i64_acc(&n);

        uint8_accessor  ui8_acc(&n);
        uint16_accessor ui16_acc(&n);
        uint32_accessor ui32_acc(&n);
        uint64_accessor ui64_acc(&n);

        float32_accessor f32_acc(&n);
        float64_accessor f64_acc(&n);

        i8_acc.set(0,-4);
        i16_acc.set(1,-8);
        i32_acc.set(2,-16);
        i64_acc.set(3,-32);

        ui8_acc.set(4, 4);
        ui16_acc.set(5,8);
        ui32_acc.set(6,16);
        ui64_acc.set(7,32);

        f32_acc.set(8,16);
        f64_acc.set(9,32);

        EXPECT_EQ(i32_acc[0],-4);
        EXPECT_EQ(i32_acc[1],-8);
        EXPECT_EQ(i32_acc[2],-16);
        EXPECT_EQ(i32_acc[3],-32);

        EXPECT_EQ(i32_acc[4],4);
        EXPECT_EQ(i32_acc[5],8);
        EXPECT_EQ(i32_acc[6],16);
        EXPECT_EQ(i32_acc[7],32);

        EXPECT_EQ(i32_acc[8],16);
        EXPECT_EQ(i32_acc[9],32);
    }

    // const Node ptr variant
    {
        Node n;
        n.set(DataType::int8(10));
        const Node *cnp = &n;

        int8_accessor  i8_acc(cnp);
        int16_accessor i16_acc(cnp);
        int32_accessor i32_acc(cnp);
        int64_accessor i64_acc(cnp);

        uint8_accessor  ui8_acc(cnp);
        uint16_accessor ui16_acc(cnp);
        uint32_accessor ui32_acc(cnp);
        uint64_accessor ui64_acc(cnp);

        float32_accessor f32_acc(cnp);
        float64_accessor f64_acc(cnp);

        i8_acc.set(0,-4);
        i16_acc.set(1,-8);
        i32_acc.set(2,-16);
        i64_acc.set(3,-32);

        ui8_acc.set(4, 4);
        ui16_acc.set(5,8);
        ui32_acc.set(6,16);
        ui64_acc.set(7,32);

        f32_acc.set(8,16);
        f64_acc.set(9,32);

        EXPECT_EQ(i32_acc[0],-4);
        EXPECT_EQ(i32_acc[1],-8);
        EXPECT_EQ(i32_acc[2],-16);
        EXPECT_EQ(i32_acc[3],-32);

        EXPECT_EQ(i32_acc[4],4);
        EXPECT_EQ(i32_acc[5],8);
        EXPECT_EQ(i32_acc[6],16);
        EXPECT_EQ(i32_acc[7],32);

        EXPECT_EQ(i32_acc[8],16);
        EXPECT_EQ(i32_acc[9],32);
    }
}


//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, fill)
{
    Node n;
    n.set(DataType::int32(10));

    int32_array    arr = n.value();
    int64_accessor acc = n.value();

    acc.fill(-1);

    for(int i=0;i<10;i++)
    {
        EXPECT_EQ(arr[i],-1);
    }
}




//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, to_string)
{
    Node n;
    n.set(DataType::int32(6));

    int32_accessor acc = n.value();

    std::string res = acc.to_string();
    std::cout << res << std::endl;
    EXPECT_EQ(res,"[0, 0, 0, 0, 0, 0]");

    res = acc.to_summary_string();
    std::cout << res << std::endl;
    EXPECT_EQ(res,"[0, 0, 0, ..., 0, 0]");
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, set_using_ptrs)
{
    //in this case we are  using std vectors to init data conveniently
    // we are actually testing the pointer set cases
    // we test std vector set cases directly in "set_using_std_vectors"
    std::vector<int8>  v_int8(10,-8);
    std::vector<int16> v_int16(10,-16);
    std::vector<int32> v_int32(10,-32);
    std::vector<int64> v_int64(10,-64);

    std::vector<uint8>  v_uint8(10,8);
    std::vector<uint16> v_uint16(10,16);
    std::vector<uint32> v_uint32(10,32);
    std::vector<uint64> v_uint64(10,64);

    std::vector<float32> v_float32(10,32.0);
    std::vector<float64> v_float64(10,64.0);



    Node n;

    // int8_array
    n["vint8"].set(DataType::int8(10));
    n["vint8"].as_int8_accessor().set(&v_int8[0],10);
    int8 *n_int8_ptr = n["vint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],v_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_accessor().set(&v_int16[0],10);
    int16 *n_int16_ptr = n["vint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],v_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_accessor().set(&v_int32[0],10);
    int32 *n_int32_ptr = n["vint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],v_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_accessor().set(&v_int64[0],10);
    int64 *n_int64_ptr = n["vint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],v_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_accessor().set(&v_uint8[0],10);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],v_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_accessor().set(&v_uint16[0],10);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],v_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_accessor().set(&v_uint32[0],10);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],v_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_accessor().set(&v_uint64[0],10);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],v_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_accessor().set(&v_float32[0],10);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],v_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_accessor().set(&v_float64[0],10);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],v_float64[i]);
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, set_using_data_array)
{
    std::vector<int8>  v_int8(10,-8);
    std::vector<int16> v_int16(10,-16);
    std::vector<int32> v_int32(10,-32);
    std::vector<int64> v_int64(10,-64);

    std::vector<uint8>  v_uint8(10,8);
    std::vector<uint16> v_uint16(10,16);
    std::vector<uint32> v_uint32(10,32);
    std::vector<uint64> v_uint64(10,64);

    std::vector<float32>  v_float32(10,32.0);
    std::vector<float64>  v_float64(10,64.0);

    int8_array    va_int8(&v_int8[0],DataType::int8(10));
    int16_array   va_int16(&v_int16[0],DataType::int16(10));
    int32_array   va_int32(&v_int32[0],DataType::int32(10));
    int64_array   va_int64(&v_int64[0],DataType::int64(10));

    uint8_array   va_uint8(&v_uint8[0],DataType::uint8(10));
    uint16_array  va_uint16(&v_uint16[0],DataType::uint16(10));
    uint32_array  va_uint32(&v_uint32[0],DataType::uint32(10));
    uint64_array  va_uint64(&v_uint64[0],DataType::uint64(10));

    float32_array  va_float32(&v_float32[0],DataType::float32(10));
    float64_array  va_float64(&v_float64[0],DataType::float64(10));


    Node n;

    // int8_array
    n["vint8"].set(DataType::int8(10));
    n["vint8"].as_int8_accessor().set(va_int8);
    int8 *n_int8_ptr = n["vint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],va_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_accessor().set(va_int16);
    int16 *n_int16_ptr = n["vint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],va_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_accessor().set(va_int32);
    int32 *n_int32_ptr = n["vint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],va_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_accessor().set(va_int64);
    int64 *n_int64_ptr = n["vint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],va_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_accessor().set(va_uint8);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],va_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_accessor().set(va_uint16);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],va_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_accessor().set(va_uint32);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],va_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_accessor().set(va_uint64);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],va_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_accessor().set(va_float32);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],va_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_accessor().set(va_float64);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],va_float64[i]);
    }

}



//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, set_using_data_accessor)
{
    std::vector<int8>  v_int8(10,-8);
    std::vector<int16> v_int16(10,-16);
    std::vector<int32> v_int32(10,-32);
    std::vector<int64> v_int64(10,-64);

    std::vector<uint8>  v_uint8(10,8);
    std::vector<uint16> v_uint16(10,16);
    std::vector<uint32> v_uint32(10,32);
    std::vector<uint64> v_uint64(10,64);

    std::vector<float32>  v_float32(10,32.0);
    std::vector<float64>  v_float64(10,64.0);

    int8_accessor  vacc_int8(&v_int8[0],DataType::int8(10));
    int16_accessor vacc_int16(&v_int16[0],DataType::int16(10));
    int32_accessor vacc_int32(&v_int32[0],DataType::int32(10));
    int64_accessor vacc_int64(&v_int64[0],DataType::int64(10));

    uint8_accessor  vacc_uint8(&v_uint8[0],DataType::uint8(10));
    uint16_accessor vacc_uint16(&v_uint16[0],DataType::uint16(10));
    uint32_accessor vacc_uint32(&v_uint32[0],DataType::uint32(10));
    uint64_accessor vacc_uint64(&v_uint64[0],DataType::uint64(10));

    float32_accessor vacc_float32(&v_float32[0],DataType::float32(10));
    float64_accessor vacc_float64(&v_float64[0],DataType::float64(10));


    Node n;

    // int8_array
    n["vint8"].set(DataType::int8(10));
    n["vint8"].as_int8_accessor().set(vacc_int8);
    int8 *n_int8_ptr = n["vint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],v_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_accessor().set(vacc_int16);
    int16 *n_int16_ptr = n["vint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],v_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_accessor().set(vacc_int32);
    int32 *n_int32_ptr = n["vint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],v_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_accessor().set(vacc_int64);
    int64 *n_int64_ptr = n["vint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],v_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_accessor().set(vacc_uint8);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],v_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_accessor().set(vacc_uint16);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],v_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_accessor().set(vacc_uint32);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],v_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_accessor().set(vacc_uint64);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],v_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_accessor().set(vacc_float32);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],v_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_accessor().set(vacc_float64);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],v_float64[i]);
    }

}
//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, bulk_strided_and_offset)
{
    const index_t num_elements = 5;

    // A 1-byte underlying dtype viewing the odd entries of an interleaved buffer
    {
        std::vector<int8> buff(2 * num_elements, (int8)-7);

        Node n;
        n.set_external(DataType::int8(num_elements,
                                      sizeof(int8),      // offset
                                      2 * sizeof(int8)), // stride
                       &buff[0]);

        // An int64 accessor over the strided int8 data
        int64_accessor acc = n.value();

        acc.fill(42);
        for (index_t i = 0; i < num_elements; i++)
        {
            // The gap bytes between the viewed elements are untouched
            EXPECT_EQ(buff[2 * i], (int8)-7);
            // The viewed odd bytes hold the fill value, stored as int8
            EXPECT_EQ(buff[2 * i + 1], (int8)42);
            // Reading back through the accessor converts the int8 to int64
            EXPECT_EQ(acc[i], (int64)42);
        }

        // Bulk set from a pointer source with distinct per element values
        std::vector<int32> v_src;
        for (index_t i = 0; i < num_elements; i++)
        {
            v_src.push_back((int32)(10 + i));
        }

        acc.set(&v_src[0], num_elements);
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[2 * i], (int8)-7);
            EXPECT_EQ(buff[2 * i + 1], (int8)(10 + i));
        }

        // Bulk set from a DataArray source
        std::vector<int64> v_arr_src;
        for (index_t i = 0; i < num_elements; i++)
        {
            v_arr_src.push_back((int64)(20 + i));
        }
        int64_array va_src(&v_arr_src[0], DataType::int64(num_elements));

        acc.set(va_src);
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[2 * i], (int8)-7);
            EXPECT_EQ(buff[2 * i + 1], (int8)(20 + i));
        }

        // Bulk set from a DataAccessor source
        std::vector<float64> v_acc_src;
        for (index_t i = 0; i < num_elements; i++)
        {
            v_acc_src.push_back((float64)(30 + i));
        }
        float64_accessor vacc_src(&v_acc_src[0], DataType::float64(num_elements));

        acc.set(vacc_src);
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[2 * i], (int8)-7);
            EXPECT_EQ(buff[2 * i + 1], (int8)(30 + i));
        }
    }

    // An 8-byte underlying dtype viewed by a narrower (int32) accessor
    {
        std::vector<float64> buff(2 * num_elements, -7.5);

        Node n;
        n.set_external(DataType::float64(num_elements,
                                         sizeof(float64),      // offset
                                         2 * sizeof(float64)), // stride
                       &buff[0]);

        int32_accessor i32_acc = n.value();

        i32_acc.fill(-11);
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[2 * i], -7.5);
            EXPECT_EQ(buff[2 * i + 1], -11.0);
        }

        std::vector<int64> v_i64_src;
        for (index_t i = 0; i < num_elements; i++)
        {
            v_i64_src.push_back((int64)(500 + i));
        }

        i32_acc.set(&v_i64_src[0], num_elements);
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[2 * i], -7.5);
            EXPECT_EQ(buff[2 * i + 1], (float64)(500 + i));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, bulk_set_double_conversion)
{
    // Bulk set must convert source -> T -> underlying dtype

    // A float64 source through an int64 accessor into float64 underlying data
    {
        Node n;
        n.set(DataType::float64(4));

        int64_accessor acc = n.value();

        std::vector<float64> v_src = {1.5, -2.75, 3.99, -0.5};
        acc.set(&v_src[0], 4);

        float64 *ptr = n.value();
        EXPECT_EQ(ptr[0], 1.0);
        EXPECT_EQ(ptr[1], -2.0);
        EXPECT_EQ(ptr[2], 3.0);
        EXPECT_EQ(ptr[3], 0.0);
    }

    // The same case, but on strided data
    {
        std::vector<float64> buff(8, -7.5);

        Node n;
        n.set_external(DataType::float64(4,
                                         sizeof(float64),
                                         2 * sizeof(float64)),
                       &buff[0]);

        int64_accessor acc = n.value();

        std::vector<float64> v_src = {1.5, -2.75, 3.99, -0.5};
        acc.set(&v_src[0], 4);

        EXPECT_EQ(buff[1], 1.0);
        EXPECT_EQ(buff[3], -2.0);
        EXPECT_EQ(buff[5], 3.0);
        EXPECT_EQ(buff[7], 0.0);

        for (index_t i = 0; i < 4; i++)
        {
            EXPECT_EQ(buff[2 * i], -7.5);
        }
    }

    // An int32 source through a uint8 accessor into int32 underlying data
    {
        Node n;
        n.set(DataType::int32(4));

        uint8_accessor acc = n.value();

        std::vector<int32> v_src = {300, -1, 255, 256};
        acc.set(&v_src[0], 4);

        int32 *ptr = n.value();
        EXPECT_EQ(ptr[0], (int32)44);  // 300 % 256
        EXPECT_EQ(ptr[1], (int32)255); // (uint8)-1
        EXPECT_EQ(ptr[2], (int32)255);
        EXPECT_EQ(ptr[3], (int32)0);   // 256 % 256
    }

    // Fill an int64 accessor over float32 data
    {
        Node n;
        n.set(DataType::float32(3));

        int64_accessor acc = n.value();
        acc.fill(-9);

        float32 *ptr = n.value();
        for (index_t i = 0; i < 3; i++)
        {
            EXPECT_EQ(ptr[i], (float32)-9.0);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, device_fill)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // The same fill runs in each policy's memory space, host or device
        Node n;
        n["vals"].set(DataType::int32(num_elements));

        // A float64 view of int32 data, moved to the policy's space if needed
        float64_accessor acc(n["vals"]);
        acc.use_with(policy);
        EXPECT_EQ(acc.active_policy().is_device_policy(), policy.is_device_policy());

        acc.fill(static_cast<float64>(-7.0));

        // Sync values back to n["vals"]
        acc.sync();

        int32 *vals_ptr = n["vals"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(vals_ptr[i], static_cast<int32>(-7));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, device_set_from_host_ptr)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    // Host-based source
    std::vector<float64> src(num_elements);
    for (index_t i = 0; i < num_elements; i++)
    {
        src[static_cast<size_t>(i)] = static_cast<float64>(i + 1);
    }

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // The destination lives in each policy's space, the source stays on host
        Node n;
        n["vals"].set(DataType::int32(num_elements));

        // Move the data if necessary
        float64_accessor acc(n["vals"]);
        acc.use_with(policy);

        // The host source is copied across memory spaces when needed
        acc.set(&src[0], num_elements);

        acc.sync();

        int32 *vals_ptr = n["vals"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(vals_ptr[i], static_cast<int32>(i + 1));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, device_set_from_device_ptr_conversion)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // Both the source and the destination live in the policy's space
        Node n;
        n["src"].set(DataType::int32(num_elements));
        n["des"].set(DataType::float64(num_elements));

        int32 *src_ptr = n["src"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            src_ptr[i] = static_cast<int32>(i + 1);
        }

        // Move the source, then grab its pointer in the policy's space
        int32_accessor src(n["src"]);
        src.use_with(policy);
        const int32 *src_vals = static_cast<const int32*>(src.element_ptr(0));

        float64_accessor des(n["des"]);
        des.use_with(policy);

        // The int32 source is converted to float64 per element, on device
        des.set(src_vals, num_elements);

        des.sync();

        float64 *des_ptr = n["des"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(des_ptr[i], static_cast<float64>(i + 1));
        }

        // A host destination can also be set from the device source, with
        // the same int32 to float64 conversion
        if (policy.is_device_policy())
        {
            Node n_host;
            n_host["des"].set(DataType::float64(num_elements));
            float64_accessor host_des(n_host["des"]);
            host_des.set(src_vals, num_elements);

            float64 *host_ptr = n_host["des"].value();
            for (index_t i = 0; i < num_elements; i++)
            {
                EXPECT_EQ(host_ptr[i], static_cast<float64>(i + 1));
            }
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, device_summary_stats)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    std::vector<int32> src(num_elements);
    float64 expected_sum = 0.0;
    for (index_t i = 0; i < num_elements; i++)
    {
        src[static_cast<size_t>(i)] = static_cast<int32>(i - num_elements / 2);
        expected_sum += static_cast<float64>(src[static_cast<size_t>(i)]);
    }

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // The same stats are computed in each policy's memory space
        Node n;
        n["vals"].set(src);

        // The accessor is a float64 view of int32 data.
        float64_accessor acc(n["vals"]);
        acc.use_with(policy);

        EXPECT_EQ(acc.min(), static_cast<float64>(src[0]));
        EXPECT_EQ(acc.max(), static_cast<float64>(src[static_cast<size_t>(num_elements - 1)]));
        EXPECT_EQ(acc.sum(), expected_sum);
        EXPECT_EQ(acc.mean(), expected_sum / static_cast<float64>(num_elements));
        EXPECT_EQ(acc.count(0.0), 1);
        EXPECT_EQ(acc.count(1000.0), 0);
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, device_strided)
{
    conduit_device_prepare();

    // View the odd entries of an interleaved buffer
    const index_t num_elements = 8;
    const index_t num_buff = 2 * num_elements;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // The same strided fill runs in each policy's memory space
        Node n;
        n["vals"].set(DataType::int32(num_elements,
                                      sizeof(int32),        // offset
                                      2 * sizeof(int32)));  // stride

        int32 *buff = static_cast<int32*>(n["vals"].data_ptr());
        for (index_t i = 0; i < num_buff; i++)
        {
            buff[i] = static_cast<int32>(1000 + i);
        }

        float64_accessor acc(n["vals"]);
        acc.use_with(policy);

        acc.fill(static_cast<float64>(-7.0));

        acc.sync();

        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[static_cast<size_t>(2 * i + 1)], static_cast<int32>(-7));
            // The interleaved gaps must not have changed
            EXPECT_EQ(buff[static_cast<size_t>(2 * i)], static_cast<int32>(1000 + 2 * i));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, mixed_space_set)
{
    conduit_device_prepare();

    const index_t num_ele = 8;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // Only device policies can form a mixed host/device pair, skip the rest
        if (!policy.is_device_policy())
        {
            return;
        }

        Node n;
        n["src"].set(DataType::float64(num_ele));
        n["des"].set(DataType::float64(num_ele));

        float64 *src_ptr = n["src"].value();
        for (index_t i = 0; i < num_ele; i++)
        {
            src_ptr[i] = static_cast<float64>(i + 1);
        }

        float64_accessor dev_src(n["src"]);
        dev_src.use_with(policy);

        // A host destination set from a device source
        float64_accessor host_des(n["des"]);
        host_des.set(dev_src);

        float64 *des_ptr = n["des"].value();
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(des_ptr[i], static_cast<float64>(i + 1));
        }

        // A device destination set from a host source
        Node n_dev;
        n_dev["des"].set(DataType::float64(num_ele));
        float64_accessor dev_des(n_dev["des"]);
        dev_des.use_with(policy);
        dev_des.set(host_des);
        dev_des.sync();

        float64 *dev_des_ptr = n_dev["des"].value();
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(dev_des_ptr[i], static_cast<float64>(i + 1));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, mixed_space_set_narrowing)
{
    conduit_device_prepare();

    const index_t num_ele = 4;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // Only device policies can form a mixed host/device pair, skip the rest
        if (!policy.is_device_policy())
        {
            return;
        }

        Node n;
        n["src"].set(DataType::int64(num_ele));
        n["des"].set(DataType::int64(num_ele));

        int64 *src_ptr = n["src"].value();
        for (index_t i = 0; i < num_ele; i++)
        {
            // Values that do not fit in an int8
            src_ptr[i] = 1000 + i;
        }

        // The source accessor narrows its int64 data to int8 on read, and
        // the cross-space set must preserve that narrowing rather than
        // copying the raw int64 bytes
        int8_accessor host_src(n["src"]);

        int64_accessor dev_des(n["des"]);
        dev_des.use_with(policy);
        dev_des.set(host_src);
        dev_des.sync();

        int64 *des_ptr = n["des"].value();
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(des_ptr[i], static_cast<int64>(static_cast<int8>(1000 + i)));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, accessor_large_n_bulk_ops)
{
    // A newly constructed view must resolve its execution policy lazily. We
    // need the number of elements to be above the small-N threshold so that
    // these operations use foralls.
    const index_t num_elements = CONDUIT_SMALL_N_THRESHOLD + 8;

    Node n;
    n["vals"].set(DataType::int32(num_elements));

    float64_accessor acc(n["vals"]);
    acc.fill(1.0);

    EXPECT_EQ(acc.min(), 1.0);
    EXPECT_EQ(acc.max(), 1.0);
    EXPECT_EQ(acc.sum(), static_cast<float64>(num_elements));
    EXPECT_EQ(acc.mean(), 1.0);
    EXPECT_EQ(acc.count(1.0), num_elements);
}

//-----------------------------------------------------------------------------
TEST(conduit_data_accessor, large_n_bulk_ops)
{
    conduit_device_prepare();

    // We need the number of elements to be above the small-N threshold so that
    // these operations use foralls.
    const index_t num_elements = CONDUIT_SMALL_N_THRESHOLD + 8;

    // Host-based source
    std::vector<float64> src(num_elements);
    float64 expected_src_sum = 0.0;
    for (index_t i = 0; i < num_elements; i++)
    {
        src[static_cast<size_t>(i)] = static_cast<float64>(i + 1);
        expected_src_sum += src[static_cast<size_t>(i)];
    }

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // The same bulk ops run in each policy's memory space, host or device
        Node n;
        n["vals"].set(DataType::float64(num_elements));

        // Move the data if necessary
        float64_accessor acc(n["vals"]);
        acc.use_with(policy);

        acc.fill(static_cast<float64>(2.5));
        EXPECT_EQ(acc.sum(), 2.5 * static_cast<float64>(num_elements));

        // Every element holds the fill value
        EXPECT_EQ(acc.min(), 2.5);
        EXPECT_EQ(acc.max(), 2.5);
        EXPECT_EQ(acc.count(2.5), num_elements);

        // The host source is copied across memory spaces when needed
        acc.set(&src[0], num_elements);

        EXPECT_EQ(acc.sum(), expected_src_sum);
        EXPECT_EQ(acc.min(), 1.0);
        EXPECT_EQ(acc.max(), static_cast<float64>(num_elements));
        EXPECT_EQ(acc.mean(), expected_src_sum / static_cast<float64>(num_elements));
        EXPECT_EQ(acc.count(1.0), 1);
        EXPECT_EQ(acc.count(0.0), 0);

        // Sync values back to n["vals"]
        acc.sync();

        // Check a few elements
        float64 *vals_ptr = n["vals"].value();
        EXPECT_EQ(vals_ptr[0], 1.0);
        EXPECT_EQ(vals_ptr[num_elements / 2], static_cast<float64>(num_elements / 2 + 1));
        EXPECT_EQ(vals_ptr[num_elements - 1], static_cast<float64>(num_elements));
    });
}
