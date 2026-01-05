// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_array.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"

using namespace conduit;

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
    std::vector<int64>   v_int64 ={-1,0,1};
    std::vector<uint64>  v_uint64 = {1,2,3};
    std::vector<float64> v_float64= {-1.0,0.0,1.0};

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
        EXPECT_EQ(arr[0],-1);
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

