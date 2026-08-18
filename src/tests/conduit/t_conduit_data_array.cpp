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
TEST(conduit_data_array, basic_construction)
{
    std::vector<int8> data1(10,8);
    std::vector<int8> data2(10,-8);

    void *data1_ptr = &data1[0];
    const void *cdata2_ptr = &data2[0];

    // void* / DataType constructor variants
    {
        // void* variant
        DataArray<int8> da_1(data1_ptr,DataType::int8(10));

        std::cout << da_1.to_string() << std::endl;

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_1[i]);
        }

        // const void* variant
        DataArray<int8> da_2(cdata2_ptr,DataType::int8(10));

        std::cout << da_2.to_string() << std::endl;

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(-8,da_2[i]);
        }

        // copy constructor
        DataArray<int8> da_3(da_1);
        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_3[i]);
        }

        da_3[0] = 16;

        // assignment operator
        da_3 = da_2;

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(-8,da_2[i]);
        }

        da_3[0] = -16;

        std::cout << da_3.to_string() << std::endl;

        // test other variants of to_string and to stream, etc
        da_3.to_string_stream(std::cout);
        da_3.to_json_stream(std::cout);

        EXPECT_EQ(16,data1[0]);
        EXPECT_EQ(-16,data2[0]);
    }

    // Node-backed constructor variants
    {
        Node n1;
        n1.set(std::vector<int8>(10,8));

        // Node ref variant
        DataArray<int8> da_1(n1);

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_1[i]);
        }

        // const Node ref variant
        const Node cn1(n1);
        DataArray<int8> da_2(cn1);

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_2[i]);
        }

        // Node ptr variant
        DataArray<int8> da_3(&n1);

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_3[i]);
        }

        // const Node ptr variant
        const Node *cn1_ptr = &n1;
        DataArray<int8> da_4(cn1_ptr);

        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_4[i]);
        }

        // copy construction from a Node-backed DataArray
        DataArray<int8> da_5(da_1);
        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(8,da_5[i]);
        }

        // DataArray assignment: rebinds da_5 to point at n2's data
        Node n2;
        n2.set(std::vector<int8>(10,-8));
        DataArray<int8> da_6(n2);

        da_5 = da_6;
        for(index_t i=0;i<10;i++)
        {
            EXPECT_EQ(-8,da_5[i]);
        }

        // write-through: element writes via DataArray should modify the Node's data
        da_1[0] = 42;
        EXPECT_EQ(42, n1.as_int8_array()[0]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, array_stride_int8)
{
    std::vector<int8> data(20,0);

    for(int i=0;i<20;i+=2)
    {
        data[i] = i/2;
    }

    for(int i=1;i<20;i+=2)
    {
        data[i] = -i/2;
    }
    
    std::cout << "Full Data" << std::endl;
    for(int i=0;i<20;i++)
    {
        std::cout << (int64) data[i] << " ";
    }
    std::cout << std::endl;

    DataType arr_t(DataType::INT8_ID,
                   10,
                   0,
                   sizeof(int8)*2, // stride
                   sizeof(int8),
                   Endianness::DEFAULT_ID);
    Node n;
    n["value"].set_external(arr_t,&data[0]);

    int8_array arr = n["value"].as_int8_array();

    for(int i=0;i<10;i++)
    {
        // note: the cast is for proper printing to std::out
        std::cout << "value[" << i << "] = " << ((int64)arr[i] ) << std::endl;
    }
    std::cout << std::endl;

    EXPECT_EQ(arr[5],5);
    EXPECT_EQ(arr[9],9);

    arr[1] = 100;
    EXPECT_EQ(data[2],100);

    std::cout << "Full Data" << std::endl;
    for(int i=0;i<20;i++)
    {
        std::cout << (int64) data[i] << " ";
    }
    std::cout << std::endl;

    Node n2(DataType::int8(10,sizeof(int8),sizeof(int8)*2),
            &data[0],
            true); /// true for external

    int8_array arr_2 = n2.as_int8_array();

    for(int i=0;i<10;i++)
    {
        // note: the cast is for proper printing to std::out
        std::cout << "value[" << i << "] = " <<  ((int64)arr_2[i] ) << std::endl;
    }
    std::cout << std::endl;

    EXPECT_EQ(arr_2[0],0);
    EXPECT_EQ(arr_2[9],-9);

    // Node-backed constructor variants (reusing the existing n["value"] with stride=2, offset=0)
    // Note: arr[1] = 100 was written above, so data[2] == 100 at this point

    // Node ref variant
    int8_array arr_nb(n["value"]);
    EXPECT_EQ(arr_nb[5],5);
    EXPECT_EQ(arr_nb[9],9);

    // const Node ref variant
    const Node &cn_val = n["value"];
    int8_array arr_c(cn_val);
    EXPECT_EQ(arr_c[5],5);
    EXPECT_EQ(arr_c[9],9);

    // Node ptr variant
    int8_array arr_p(&n["value"]);
    EXPECT_EQ(arr_p[5],5);
    EXPECT_EQ(arr_p[9],9);

    // const Node ptr variant
    const Node *cnp = &n["value"];
    int8_array arr_cnp(cnp);
    EXPECT_EQ(arr_cnp[5],5);
    EXPECT_EQ(arr_cnp[9],9);

}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, array_stride_int8_external)
{
    std::vector<int64> data(20,0);

    for(int i=0;i<20;i+=2)
    {
        data[i] = i/2;
    }

    for(int i=1;i<20;i+=2)
    {
        data[i] = -i/2;
    }
    std::cout << "Full Data" << std::endl;

    for(int i=0;i<20;i++)
    {
        std::cout << (int64) data[i] << " ";
    }
    std::cout << std::endl;

    Node n;
    n["value"].set_external(data);

    int64_array arr = n["value"].as_int64_array();

    for(int i=0;i<20;i++)
    {
        // note: the cast is for proper printing to std::out
        std::cout << "value[" << i << "] = " << arr[i] << std::endl;
    }
    std::cout << std::endl;

    data[2]*=10;
    data[3]*=10;

    EXPECT_EQ(arr[2],10);
    EXPECT_EQ(arr[3],-10);

}


//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_using_ptrs)
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
    n["vint8"].as_int8_array().set(&v_int8[0],10);
    int8 *n_int8_ptr = n["vint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],v_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_array().set(&v_int16[0],10);
    int16 *n_int16_ptr = n["vint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],v_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_array().set(&v_int32[0],10);
    int32 *n_int32_ptr = n["vint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],v_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_array().set(&v_int64[0],10);
    int64 *n_int64_ptr = n["vint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],v_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_array().set(&v_uint8[0],10);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],v_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_array().set(&v_uint16[0],10);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],v_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_array().set(&v_uint32[0],10);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],v_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_array().set(&v_uint64[0],10);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],v_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_array().set(&v_float32[0],10);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],v_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_array().set(&v_float64[0],10);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],v_float64[i]);
    }

}


//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_using_data_array)
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
    n["vint8"].as_int8_array().set(va_int8);
    int8 *n_int8_ptr = n["vint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],va_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_array().set(va_int16);
    int16 *n_int16_ptr = n["vint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],va_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_array().set(va_int32);
    int32 *n_int32_ptr = n["vint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],va_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_array().set(va_int64);
    int64 *n_int64_ptr = n["vint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],va_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_array().set(va_uint8);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],va_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_array().set(va_uint16);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],va_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_array().set(va_uint32);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],va_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_array().set(va_uint64);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],va_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_array().set(va_float32);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],va_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_array().set(va_float64);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],va_float64[i]);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_single_element)
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

    // change the second element in each array
    va_int8.set(1,(int8)-4);
    va_int16.set(1,(int16)-8);
    va_int32.set(1,(int32)-16);
    va_int64.set(1,(int64)-32);

    va_uint8.set(1,(uint8) 4);
    va_uint16.set(1,(uint16)8);
    va_uint32.set(1,(uint32)16);
    va_uint64.set(1,(uint64)32);

    va_float32.set(1,(float32)16.0);
    va_float64.set(1,(float64)32.0);

    va_int8.print();
    va_int16.print();
    va_int32.print();
    va_int64.print();

    va_uint8.print();
    va_uint16.print();
    va_uint32.print();
    va_uint64.print();

    va_float32.print();
    va_float64.print();

    EXPECT_EQ(va_int8[1],(int8)-4);
    EXPECT_EQ(va_int16[1],(int16)-8);
    EXPECT_EQ(va_int32[1],(int32)-16);
    EXPECT_EQ(va_int64[1],(int64)-32);

    EXPECT_EQ(va_uint8[1],(uint8)4);
    EXPECT_EQ(va_uint16[1],(uint16)8);
    EXPECT_EQ(va_uint32[1],(uint32)16);
    EXPECT_EQ(va_uint64[1],(uint64)32);

    EXPECT_EQ(va_float32[1],(float32) 16.0);
    EXPECT_EQ(va_float64[1],(float64) 32.0);
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_single_element_node_backed)
{
    Node n;
    n["int8"].set(std::vector<int8>(10,-8));
    n["int16"].set(std::vector<int16>(10,-16));
    n["int32"].set(std::vector<int32>(10,-32));
    n["int64"].set(std::vector<int64>(10,-64));

    n["uint8"].set(std::vector<uint8>(10,8));
    n["uint16"].set(std::vector<uint16>(10,16));
    n["uint32"].set(std::vector<uint32>(10,32));
    n["uint64"].set(std::vector<uint64>(10,64));

    n["float32"].set(std::vector<float32>(10,32.0));
    n["float64"].set(std::vector<float64>(10,64.0));

    int8_array    va_int8(n["int8"]);
    int16_array   va_int16(n["int16"]);
    int32_array   va_int32(n["int32"]);
    int64_array   va_int64(n["int64"]);

    uint8_array   va_uint8(n["uint8"]);
    uint16_array  va_uint16(n["uint16"]);
    uint32_array  va_uint32(n["uint32"]);
    uint64_array  va_uint64(n["uint64"]);

    float32_array va_float32(n["float32"]);
    float64_array va_float64(n["float64"]);

    // change the second element in each array
    va_int8.set(1,(int8)-4);
    va_int16.set(1,(int16)-8);
    va_int32.set(1,(int32)-16);
    va_int64.set(1,(int64)-32);

    va_uint8.set(1,(uint8) 4);
    va_uint16.set(1,(uint16)8);
    va_uint32.set(1,(uint32)16);
    va_uint64.set(1,(uint64)32);

    va_float32.set(1,(float32)16.0);
    va_float64.set(1,(float64)32.0);

    va_int8.print();
    va_int16.print();
    va_int32.print();
    va_int64.print();

    va_uint8.print();
    va_uint16.print();
    va_uint32.print();
    va_uint64.print();

    va_float32.print();
    va_float64.print();

    EXPECT_EQ(va_int8[1],(int8)-4);
    EXPECT_EQ(va_int16[1],(int16)-8);
    EXPECT_EQ(va_int32[1],(int32)-16);
    EXPECT_EQ(va_int64[1],(int64)-32);

    EXPECT_EQ(va_uint8[1],(uint8)4);
    EXPECT_EQ(va_uint16[1],(uint16)8);
    EXPECT_EQ(va_uint32[1],(uint32)16);
    EXPECT_EQ(va_uint64[1],(uint64)32);

    EXPECT_EQ(va_float32[1],(float32) 16.0);
    EXPECT_EQ(va_float64[1],(float64) 32.0);

    // verify write-through: changes made via DataArray should appear in the Node
    EXPECT_EQ(n["int8"].as_int8_array()[1],    (int8)-4);
    EXPECT_EQ(n["int16"].as_int16_array()[1],  (int16)-8);
    EXPECT_EQ(n["int32"].as_int32_array()[1],  (int32)-16);
    EXPECT_EQ(n["int64"].as_int64_array()[1],  (int64)-32);

    EXPECT_EQ(n["uint8"].as_uint8_array()[1],   (uint8)4);
    EXPECT_EQ(n["uint16"].as_uint16_array()[1], (uint16)8);
    EXPECT_EQ(n["uint32"].as_uint32_array()[1], (uint32)16);
    EXPECT_EQ(n["uint64"].as_uint64_array()[1], (uint64)32);

    EXPECT_EQ(n["float32"].as_float32_array()[1], (float32)16.0);
    EXPECT_EQ(n["float64"].as_float64_array()[1], (float64)32.0);
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_using_data_accessor)
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
    n["vint8"].as_int8_array().set(vacc_int8);
    int8 *n_int8_ptr = n["vint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],v_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_array().set(vacc_int16);
    int16 *n_int16_ptr = n["vint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],v_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_array().set(vacc_int32);
    int32 *n_int32_ptr = n["vint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],v_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_array().set(vacc_int64);
    int64 *n_int64_ptr = n["vint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],v_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_array().set(vacc_uint8);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],v_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_array().set(vacc_uint16);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],v_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_array().set(vacc_uint32);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],v_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_array().set(vacc_uint64);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],v_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_array().set(vacc_float32);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],v_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_array().set(vacc_float64);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(index_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],v_float64[i]);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_using_std_vectors)
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


    Node n;

    // int8_array
    n["vint8"].set(DataType::int8(10));
    n["vint8"].as_int8_array().set(v_int8);
    int8 *n_int8_ptr = n["vint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int8_ptr[i],v_int8[i]);
    }

    // int16_array
    n["vint16"].set(DataType::int16(10));
    n["vint16"].as_int16_array().set(v_int16);
    int16 *n_int16_ptr = n["vint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int16_ptr[i],v_int16[i]);
    }

    // int32_array
    n["vint32"].set(DataType::int32(10));
    n["vint32"].as_int32_array().set(v_int32);
    int32 *n_int32_ptr = n["vint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int32_ptr[i],v_int32[i]);
    }

    // int64_array
    n["vint64"].set(DataType::int64(10));
    n["vint64"].as_int64_array().set(v_int64);
    int64 *n_int64_ptr = n["vint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_int64_ptr[i],v_int64[i]);
    }

    // uint8_array
    n["vuint8"].set(DataType::uint8(10));
    n["vuint8"].as_uint8_array().set(v_uint8);
    uint8 *n_uint8_ptr = n["vuint8"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint8_ptr[i],v_uint8[i]);
    }

    // uint16_array
    n["vuint16"].set(DataType::uint16(10));
    n["vuint16"].as_uint16_array().set(v_uint16);
    uint16 *n_uint16_ptr = n["vuint16"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint16_ptr[i],v_uint16[i]);
    }

    // uint32_array
    n["vuint32"].set(DataType::uint32(10));
    n["vuint32"].as_uint32_array().set(v_uint32);
    uint32 *n_uint32_ptr = n["vuint32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint32_ptr[i],v_uint32[i]);
    }

    // uint64_array
    n["vuint64"].set(DataType::uint64(10));
    n["vuint64"].as_uint64_array().set(v_uint64);
    uint64 *n_uint64_ptr = n["vuint64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_uint64_ptr[i],v_uint64[i]);
    }


    // float32_array
    n["vfloat32"].set(DataType::float32(10));
    n["vfloat32"].as_float32_array().set(v_float32);
    float32 *n_float32_ptr = n["vfloat32"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float32_ptr[i],v_float32[i]);
    }

    // float64_array
    n["vfloat64"].set(DataType::float64(10));
    n["vfloat64"].as_float64_array().set(v_float64);
    float64 *n_float64_ptr = n["vfloat64"].value();
    for(size_t i=0;i<10;i++)
    {
        EXPECT_EQ(n_float64_ptr[i],v_float64[i]);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, print_bells_and_whistles)
{
    Node n;

    n["int32_1"].set(DataType::int32(1));
    n["int32_2"].set(DataType::int32(2));

    int32_array va_int32_1 = n["int32_1"].value();
    int32_array va_int32_2 = n["int32_2"].value();

    va_int32_1[0] = 1;

    va_int32_2[0] = 1;
    va_int32_2[1] = 2;

    std::string s_json_int32_1 = va_int32_1.to_json();
    std::string s_json_int32_2 = va_int32_2.to_json();

    std::string s_yaml_int32_1 = va_int32_1.to_yaml();
    std::string s_yaml_int32_2 = va_int32_2.to_yaml();

    std::cout << "int32_1: " << s_json_int32_1 << std::endl;
    std::cout << "int32_2: " << s_json_int32_2 << std::endl;

    EXPECT_EQ(s_json_int32_1,"1");
    EXPECT_EQ(s_json_int32_2,"[1, 2]");

    EXPECT_EQ(s_json_int32_1,s_yaml_int32_1);
    EXPECT_EQ(s_json_int32_2,s_yaml_int32_2);

    std::vector<float64>  v_float64(10,64.0);
    float64_array    va_float64(&v_float64[0],DataType::float64(10));

    std::cout << "to_string(\"yaml\")" << std::endl;
    std::cout << va_float64.to_string("yaml") << std::endl;
    std::cout << "to_string(\"json\")" << std::endl;
    std::cout << va_float64.to_string("json") << std::endl;

    std::cout << "to_json()" << std::endl;
    std::cout << va_float64.to_json() << std::endl;

    std::cout << "to_yaml()" << std::endl;
    std::cout << va_float64.to_yaml() << std::endl;

    std::cout << "to_string_stream(..., yaml)" << std::endl;
    va_float64.to_string_stream(std::cout,"yaml");
    std::cout << std::endl;

    std::cout << "to_string_stream(..., json)" << std::endl;
    va_float64.to_string_stream(std::cout,"json");
    std::cout << std::endl;

    std::cout << "to_json_stream()" << std::endl;
    va_float64.to_json_stream(std::cout);
    std::cout << std::endl;

    std::cout << "to_yaml_stream()" << std::endl;
    va_float64.to_yaml_stream(std::cout);
    std::cout << std::endl;
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, fill)
{
    int num_ele = 5;

    std::vector<int8>  v_int8(num_ele,-8);
    std::vector<int16> v_int16(num_ele,-16);
    std::vector<int32> v_int32(num_ele,-32);
    std::vector<int64> v_int64(num_ele,-64);

    std::vector<uint8>  v_uint8(num_ele,8);
    std::vector<uint16> v_uint16(num_ele,16);
    std::vector<uint32> v_uint32(num_ele,32);
    std::vector<uint64> v_uint64(num_ele,64);

    std::vector<float32>  v_float32(num_ele,32.0);
    std::vector<float64>  v_float64(num_ele,64.0);

    Node n;
    n["v_int8"].set(v_int8);
    n["v_int16"].set(v_int16);
    n["v_int32"].set(v_int32);
    n["v_int64"].set(v_int64);

    n["v_uint8"].set(v_uint8);
    n["v_uint16"].set(v_uint16);
    n["v_uint32"].set(v_uint32);
    n["v_uint64"].set(v_uint64);

    n["v_float32"].set(v_float32);
    n["v_float64"].set(v_float64);

    n.print();

    int8_array   va_int8  = n["v_int8"].value();
    int16_array  va_int16 = n["v_int16"].value();
    int32_array  va_int32 = n["v_int32"].value();
    int64_array  va_int64 = n["v_int64"].value();

    uint8_array  va_uint8  = n["v_uint8"].value();
    uint16_array va_uint16 = n["v_uint16"].value();
    uint32_array va_uint32 = n["v_uint32"].value();
    uint64_array va_uint64 = n["v_uint64"].value();

    float32_array va_float32 = n["v_float32"].value();
    float64_array va_float64 = n["v_float64"].value();

    for(int i=0;i<num_ele; i++)
    {
        EXPECT_NE(va_int8[i],-1);
        EXPECT_NE(va_int16[i],-1);
        EXPECT_NE(va_int32[i],-1);
        EXPECT_NE(va_int64[i],-1);

        EXPECT_NE(va_uint8[i],1);
        EXPECT_NE(va_uint16[i],1);
        EXPECT_NE(va_uint32[i],1);
        EXPECT_NE(va_uint64[i],1);

        EXPECT_NE(va_float32[i],1.0);
        EXPECT_NE(va_float64[i],1.0);
    }


    // not all combos of src to dest, but all combos of src.
    va_int8.fill((int8)   -1);
    va_int16.fill((int16) -1);
    va_int32.fill((int32) -1);
    va_int64.fill((int64) -1);

    va_uint8.fill((uint8)   1);
    va_uint16.fill((uint16) 1);
    va_uint32.fill((uint32) 1);
    va_uint64.fill((uint64) 1);

    va_float32.fill((float32) 1.0);
    va_float64.fill((float64) 1.0);

    n.print();

    for(int i=0;i<num_ele; i++)
    {
        EXPECT_EQ(va_int8[i],-1);
        EXPECT_EQ(va_int16[i],-1);
        EXPECT_EQ(va_int32[i],-1);
        EXPECT_EQ(va_int64[i],-1);

        EXPECT_EQ(va_uint8[i],1);
        EXPECT_EQ(va_uint16[i],1);
        EXPECT_EQ(va_uint32[i],1);
        EXPECT_EQ(va_uint64[i],1);

        EXPECT_EQ(va_float32[i],1.0);
        EXPECT_EQ(va_float64[i],1.0);
    }

}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, compact_to_bytes)
{
    std::vector<int64> vals(8,0);

    vals[0] = 3;
    vals[2] = 7;
    vals[4] = 9;
    vals[6] = 11;

    // void* / DataType constructor variant
    {
        // stride every 16 bytes (2 int64s)
        int64_array varray(&vals[0],DataType::int64(4,0,16));

        std::cout << varray.to_string() << std::endl;

        uint8 buff[64*4];

        varray.compact_elements_to(buff);

        int64 *vals_cpt = (int64*)buff;

        EXPECT_EQ(vals_cpt[0],3);
        EXPECT_EQ(vals_cpt[1],7);
        EXPECT_EQ(vals_cpt[2],9);
        EXPECT_EQ(vals_cpt[3],11);
    }

    // Node-backed constructor variants
    {
        // set a strided view on a Node: 4 elements, stride every 16 bytes (2 int64s)
        DataType strided_dt = DataType::int64(4,0,16);
        Node n;
        n.set_external(strided_dt,&vals[0]);

        // Node ref variant
        int64_array varray(n);

        std::cout << varray.to_string() << std::endl;

        uint8 buff[64*4];

        varray.compact_elements_to(buff);

        int64 *vals_cpt = (int64*)buff;

        EXPECT_EQ(vals_cpt[0],3);
        EXPECT_EQ(vals_cpt[1],7);
        EXPECT_EQ(vals_cpt[2],9);
        EXPECT_EQ(vals_cpt[3],11);

        // const Node ref variant
        const Node &cn = n;
        int64_array varray_c(cn);
        uint8 buff_c[64*4];
        varray_c.compact_elements_to(buff_c);
        int64 *vals_cpt_c = (int64*)buff_c;
        EXPECT_EQ(vals_cpt_c[0],3);
        EXPECT_EQ(vals_cpt_c[1],7);
        EXPECT_EQ(vals_cpt_c[2],9);
        EXPECT_EQ(vals_cpt_c[3],11);

        // Node ptr variant
        int64_array varray_p(&n);
        uint8 buff_p[64*4];
        varray_p.compact_elements_to(buff_p);
        int64 *vals_cpt_p = (int64*)buff_p;
        EXPECT_EQ(vals_cpt_p[0],3);
        EXPECT_EQ(vals_cpt_p[1],7);
        EXPECT_EQ(vals_cpt_p[2],9);
        EXPECT_EQ(vals_cpt_p[3],11);

        // const Node ptr variant
        const Node *cnp = &n;
        int64_array varray_cnp(cnp);
        uint8 buff_cnp[64*4];
        varray_cnp.compact_elements_to(buff_cnp);
        int64 *vals_cpt_cnp = (int64*)buff_cnp;
        EXPECT_EQ(vals_cpt_cnp[0],3);
        EXPECT_EQ(vals_cpt_cnp[1],7);
        EXPECT_EQ(vals_cpt_cnp[2],9);
        EXPECT_EQ(vals_cpt_cnp[3],11);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, summary_stats)
{
    std::vector<int64>    v_int64(3,-64);
    std::vector<uint64>   v_uint64(3,64);
    std::vector<float64>  v_float64(3,64.0);

    int64_array   va_int64(&v_int64[0],DataType::int64(3));
    uint64_array  va_uint64(&v_uint64[0],DataType::uint64(3));
    float64_array va_float64(&v_float64[0],DataType::float64(3));

    // void* / DataType constructor variant
    {
        va_int64.set({-1,0,1});
        va_uint64.set({1,2,3});
        va_float64.set({-1.0,0.0,1.0});

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
    {
        Node n;
        n["int64"].set(std::vector<int64>{-1,0,1});
        n["uint64"].set(std::vector<uint64>{1,2,3});
        n["float64"].set(std::vector<float64>{-1.0,0.0,1.0});

        // Node ref variant
        int64_array   va_int64(n["int64"]);
        uint64_array  va_uint64(n["uint64"]);
        float64_array va_float64(n["float64"]);

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

        // const Node ref variant
        const Node &cn = n;
        int64_array va_int64_c(cn["int64"]);
        EXPECT_EQ(va_int64_c.min(),-1);
        EXPECT_EQ(va_int64_c.max(),1);
        EXPECT_EQ(va_int64_c.mean(),0);
        EXPECT_EQ(va_int64_c.sum(),0);
        EXPECT_EQ(va_int64_c.count(-1),1);

        // Node ptr variant
        int64_array va_int64_p(&n["int64"]);
        EXPECT_EQ(va_int64_p.min(),-1);
        EXPECT_EQ(va_int64_p.max(),1);
        EXPECT_EQ(va_int64_p.mean(),0);
        EXPECT_EQ(va_int64_p.sum(),0);
        EXPECT_EQ(va_int64_p.count(-1),1);

        // const Node ptr variant
        const Node *cnp = &n["int64"];
        int64_array va_int64_cnp(cnp);
        EXPECT_EQ(va_int64_cnp.min(),-1);
        EXPECT_EQ(va_int64_cnp.max(),1);
        EXPECT_EQ(va_int64_cnp.mean(),0);
        EXPECT_EQ(va_int64_cnp.sum(),0);
        EXPECT_EQ(va_int64_cnp.count(-1),1);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, summary_print)
{
    std::vector<int64> v_int64(5,-64);
    int64_array   va_int64(&v_int64[0],DataType::int64(5));

    va_int64.set({1,2,3,4,5});

    // default (thresh is 5)
    std::string v = va_int64.to_summary_string();
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[1, 2, 3, 4, 5]");

    // above threshold, even # to display
    v = va_int64.to_summary_string(2);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[1, ..., 5]");

    // above threshold, od # to display
    v = va_int64.to_summary_string(3);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[1, 2, ..., 5]");

    // above threshold, threshold is much larger than # of eles
    v = va_int64.to_summary_string(64);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[1, 2, 3, 4, 5]");

    // above threshold, threshold is negative
    v = va_int64.to_summary_string(-1);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[1, 2, 3, 4, 5]");


    // single ele array
    std::vector<int64> v_int64_single(1,-1);
    int64_array   va_int64_single(&v_int64[0],DataType::int64(1));

    va_int64_single.set({1});

    v = va_int64_single.to_summary_string();
    std::cout << v << std::endl;
    EXPECT_EQ(v,"1");

    v = va_int64_single.to_summary_string(64);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"1");

    v = va_int64_single.to_summary_string(-1);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"1");


    // single empty array
    std::vector<int64> v_int64_empty(0,0);
    int64_array   va_int64_empty(&v_int64[0],DataType::int64(0));

    v = va_int64_empty.to_summary_string();
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[]");

    v = va_int64_empty.to_summary_string(64);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[]");

    v = va_int64_empty.to_summary_string(-1);
    std::cout << v << std::endl;
    EXPECT_EQ(v,"[]");

}


//-----------------------------------------------------------------------------
TEST(conduit_data_array, cxx_11_init_lists)
{
    std::vector<int8>  v_int8(3,-8);
    std::vector<int16> v_int16(3,-16);
    std::vector<int32> v_int32(3,-32);
    std::vector<int64> v_int64(3,-64);

    std::vector<uint8>  v_uint8(3,8);
    std::vector<uint16> v_uint16(3,16);
    std::vector<uint32> v_uint32(3,32);
    std::vector<uint64> v_uint64(3,64);

    std::vector<float32>  v_float32(3,32.0);
    std::vector<float64>  v_float64(3,64.0);

    int8_array    va_int8(&v_int8[0],DataType::int8(3));
    int16_array   va_int16(&v_int16[0],DataType::int16(3));
    int32_array   va_int32(&v_int32[0],DataType::int32(3));
    int64_array   va_int64(&v_int64[0],DataType::int64(3));

    uint8_array   va_uint8(&v_uint8[0],DataType::uint8(3));
    uint16_array  va_uint16(&v_uint16[0],DataType::uint16(3));
    uint32_array  va_uint32(&v_uint32[0],DataType::uint32(3));
    uint64_array  va_uint64(&v_uint64[0],DataType::uint64(3));

    float32_array  va_float32(&v_float32[0],DataType::float32(3));
    float64_array  va_float64(&v_float64[0],DataType::float64(3));

    // int 8
    {
        va_int8.set({-1,2,-3});
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8 = {-1,2,-3};
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);
        va_int8.print();

        va_int8.set({1u,2u,3u});
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8 = {1u,2u,3u};
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8.set({-1l,2l,-3l});
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8 = {-1l,2l,-3l};
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8.set({1ul,2ul,3ul});
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);
        va_int8.print();

        va_int8 = {1ul,2ul,3ul};
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8.set({1.0f,2.0f,3.0f});
        va_int8.print();
        va_int8 = {1.0f,2.0f,3.0f};
        va_int8.print();

        va_int8.set({1.0,2.0,3.0});
        va_int8.print();
        va_int8 = {1.0,2.0,3.0};
        va_int8.print();
    }

    // int 16
    {
        va_int16.set({-1,2,-3});
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16 = {-1,2,-3};
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16.set({1u,2u,3u});
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16 = {1u,2u,3u};
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16.set({-1l,2l,-3l});
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);
        va_int16.print();

        va_int16 = {-1l,2l,-3l};
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16.set({1ul,2ul,3ul});
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16 = {1ul,2ul,3ul};
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16.set({1.0f,2.0f,3.0f});
        va_int16.print();
        va_int16 = {1.0f,2.0f,3.0f};
        va_int16.print();

        va_int16.set({1.0,2.0,3.0});
        va_int16.print();
        va_int16 = {1.0,2.0,3.0};
        va_int16.print();
    }

    // int 32
    {
        va_int32.set({-1,2,-3});
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32 = {-1,2,-3};
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32.set({1u,2u,3u});
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32 = {1u,2u,3u};
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32.set({-1l,2l,-3l});
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32 = {-1l,2l,-3l};
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32.set({1ul,2ul,3ul});
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32 = {1ul,2ul,3ul};
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32.set({1.0f,2.0f,3.0f});
        va_int32.print();
        va_int32 = {1.0f,2.0f,3.0f};
        va_int32.print();

        va_int32.set({1.0,2.0,3.0});
        va_int32.print();
        va_int32 = {1.0,2.0,3.0};
        va_int32.print();
    }

    // int 64
    {
        va_int64.set({-1,2,-3});
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64 = {-1,2,-3};
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);


        va_int64.set({1u,2u,3u});
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64 = {1u,2u,3u};
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64.set({-1l,2l,-3l});
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64 = {-1l,2l,-3l};
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64.set({1ul,2ul,3ul});
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64 = {1ul,2ul,3ul};
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64.set({1.0f,2.0f,3.0f});
        va_int64.print();
        va_int64 = {1.0f,2.0f,3.0f};
        va_int64.print();

        va_int64.set({1.0,2.0,3.0});
        va_int64.print();
        va_int64 = {1.0,2.0,3.0};
        va_int64.print();
    }

    // uint 8
    {
        va_uint8.set({1,2,3});
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);
        va_uint8.print();

        va_uint8 = {1,2,3};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1u,2u,3u});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1u,2u,3u};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1l,2l,3l});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1l,2l,3l};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);


        va_uint8.set({1ul,2ul,3ul});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1ul,2ul,3ul};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1.0f,2.0f,3.0f});
        va_uint8.print();
        va_uint8 = {1.0f,2.0f,3.0f};
        va_uint8.print();

        va_uint8.set({1.0,2.0,3.0});
        va_uint8.print();
        va_uint8 = {1.0,2.0,3.0};
        va_uint8.print();
    }

    // uint 16
    {
        va_uint16.set({1,2,3});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1,2,3};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1u,2u,3u});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1u,2u,3u};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1l,2l,3l});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1l,2l,3l};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1ul,2ul,3ul});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1ul,2ul,3ul};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1.0f,2.0f,3.0f});
        va_uint16.print();
        va_uint16 = {1.0f,2.0f,3.0f};
        va_uint16.print();

        va_uint16.set({1.0,2.0,3.0});
        va_uint16.print();
        va_uint16 = {1.0,2.0,3.0};
        va_uint16.print();
    }

    // uint 32
    {
        va_uint32.set({1,2,3});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1,2,3};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);


        va_uint32.set({1u,2u,3u});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1u,2u,3u};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);


        va_uint32.set({1l,2l,3l});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1l,2l,3l};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1ul,2ul,3ul});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1ul,2ul,3ul};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1.0f,2.0f,3.0f});
        va_uint32.print();
        va_uint32 = {1.0f,2.0f,3.0f};
        va_uint32.print();

        va_uint32.set({1.0,2.0,3.0});
        va_uint32.print();
        va_uint32 = {1.0,2.0,3.0};
        va_uint32.print();
    }

    // uint 64
    {
        va_uint64.set({1,2,3});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1,2,3};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1u,2u,3u});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1u,2u,3u};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1l,2l,3l});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1l,2l,3l};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1ul,2ul,3ul});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1ul,2ul,3ul};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1.0f,2.0f,3.0f});
        va_uint64.print();
        va_uint64 = {1.0f,2.0f,3.0f};
        va_uint64.print();

        va_uint64.set({1.0,2.0,3.0});
        va_uint64.print();
        va_uint64 = {1.0,2.0,3.0};
        va_uint64.print();
    }

    // float 32
    {
        va_float32.set({-1,-2,-3});
        va_float32.print();
        va_float32 = {-1,2,-3};
        va_float32.print();

        va_float32.set({1u,2u,3u});
        va_float32.print();
        va_float32 = {1u,2u,3u};
        va_float32.print();

        va_float32.set({1l,2l,3l});
        va_float32.print();
        va_float32 = {-1l,2l,-3l};
        va_float32.print();

        va_float32.set({1ul,2ul,3ul});
        va_float32.print();
        va_float32 = {1ul,2ul,3ul};
        va_float32.print();

        va_float32.set({1.0f,2.0f,3.0f});
        va_float32.print();
        EXPECT_EQ(va_float32[0],1.0f);
        EXPECT_EQ(va_float32[1],2.0f);
        EXPECT_EQ(va_float32[2],3.0f);

        va_float32 = {1.0f,2.0f,3.0f};
        va_float32.print();
        EXPECT_EQ(va_float32[0],1.0f);
        EXPECT_EQ(va_float32[1],2.0f);
        EXPECT_EQ(va_float32[2],3.0f);

        va_float32.set({1.0,2.0,3.0});
        va_float32.print();
        va_float32 = {1.0,2.0,3.0};
        va_float32.print();
    }

    // float 64
    {
        va_float64.set({-1,-2,-3});
        va_float64.print();
        va_float64 = {-1,2,-3};
        va_float64.print();

        va_float64.set({1u,2u,3u});
        va_float64.print();
        va_float64 = {1u,2u,3u};
        va_float64.print();

        va_float64.set({1l,2l,3l});
        va_float64.print();
        va_float64 = {-1l,2l,-3l};
        va_float64.print();

        va_float64.set({1ul,2ul,3ul});
        va_float64.print();
        va_float64 = {1ul,2ul,3ul};
        va_float64.print();

        va_float64.set({1.0f,2.0f,3.0f});
        va_float64.print();
        va_float64 = {1.0f,2.0f,3.0f};
        va_float64.print();

        va_float64.set({1.0,2.0,3.0});
        va_float64.print();
        EXPECT_EQ(va_float64[0],1.0);
        EXPECT_EQ(va_float64[1],2.0);
        EXPECT_EQ(va_float64[2],3.0);

        va_float64 = {1.0,2.0,3.0};
        va_float64.print();
        EXPECT_EQ(va_float64[0],1.0);
        EXPECT_EQ(va_float64[1],2.0);
        EXPECT_EQ(va_float64[2],3.0);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, cxx_11_init_lists_node_backed)
{
    Node n;
    n["int8"].set(std::vector<int8>(3,-8));
    n["int16"].set(std::vector<int16>(3,-16));
    n["int32"].set(std::vector<int32>(3,-32));
    n["int64"].set(std::vector<int64>(3,-64));

    n["uint8"].set(std::vector<uint8>(3,8));
    n["uint16"].set(std::vector<uint16>(3,16));
    n["uint32"].set(std::vector<uint32>(3,32));
    n["uint64"].set(std::vector<uint64>(3,64));

    n["float32"].set(std::vector<float32>(3,32.0));
    n["float64"].set(std::vector<float64>(3,64.0));

    int8_array    va_int8(n["int8"]);
    int16_array   va_int16(n["int16"]);
    int32_array   va_int32(n["int32"]);
    int64_array   va_int64(n["int64"]);

    uint8_array   va_uint8(n["uint8"]);
    uint16_array  va_uint16(n["uint16"]);
    uint32_array  va_uint32(n["uint32"]);
    uint64_array  va_uint64(n["uint64"]);

    float32_array  va_float32(n["float32"]);
    float64_array  va_float64(n["float64"]);
    
    // int 8
    {
        va_int8.set({-1,2,-3});
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8 = {-1,2,-3};
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);
        va_int8.print();

        va_int8.set({1u,2u,3u});
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8 = {1u,2u,3u};
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8.set({-1l,2l,-3l});
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8 = {-1l,2l,-3l};
        va_int8.print();
        EXPECT_EQ(va_int8[0],-1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],-3);

        va_int8.set({1ul,2ul,3ul});
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);
        va_int8.print();

        va_int8 = {1ul,2ul,3ul};
        va_int8.print();
        EXPECT_EQ(va_int8[0],1);
        EXPECT_EQ(va_int8[1],2);
        EXPECT_EQ(va_int8[2],3);

        va_int8.set({1.0f,2.0f,3.0f});
        va_int8.print();
        va_int8 = {1.0f,2.0f,3.0f};
        va_int8.print();

        va_int8.set({1.0,2.0,3.0});
        va_int8.print();
        va_int8 = {1.0,2.0,3.0};
        va_int8.print();
    }

    // int 16
    {
        va_int16.set({-1,2,-3});
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16 = {-1,2,-3};
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16.set({1u,2u,3u});
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16 = {1u,2u,3u};
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16.set({-1l,2l,-3l});
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);
        va_int16.print();

        va_int16 = {-1l,2l,-3l};
        va_int16.print();
        EXPECT_EQ(va_int16[0],-1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],-3);

        va_int16.set({1ul,2ul,3ul});
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16 = {1ul,2ul,3ul};
        va_int16.print();
        EXPECT_EQ(va_int16[0],1);
        EXPECT_EQ(va_int16[1],2);
        EXPECT_EQ(va_int16[2],3);

        va_int16.set({1.0f,2.0f,3.0f});
        va_int16.print();
        va_int16 = {1.0f,2.0f,3.0f};
        va_int16.print();

        va_int16.set({1.0,2.0,3.0});
        va_int16.print();
        va_int16 = {1.0,2.0,3.0};
        va_int16.print();
    }

    // int 32
    {
        va_int32.set({-1,2,-3});
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32 = {-1,2,-3};
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32.set({1u,2u,3u});
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32 = {1u,2u,3u};
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32.set({-1l,2l,-3l});
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32 = {-1l,2l,-3l};
        va_int32.print();
        EXPECT_EQ(va_int32[0],-1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],-3);

        va_int32.set({1ul,2ul,3ul});
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32 = {1ul,2ul,3ul};
        va_int32.print();
        EXPECT_EQ(va_int32[0],1);
        EXPECT_EQ(va_int32[1],2);
        EXPECT_EQ(va_int32[2],3);

        va_int32.set({1.0f,2.0f,3.0f});
        va_int32.print();
        va_int32 = {1.0f,2.0f,3.0f};
        va_int32.print();

        va_int32.set({1.0,2.0,3.0});
        va_int32.print();
        va_int32 = {1.0,2.0,3.0};
        va_int32.print();
    }

    // int 64
    {
        va_int64.set({-1,2,-3});
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64 = {-1,2,-3};
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64.set({1u,2u,3u});
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64 = {1u,2u,3u};
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64.set({-1l,2l,-3l});
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64 = {-1l,2l,-3l};
        va_int64.print();
        EXPECT_EQ(va_int64[0],-1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],-3);

        va_int64.set({1ul,2ul,3ul});
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64 = {1ul,2ul,3ul};
        va_int64.print();
        EXPECT_EQ(va_int64[0],1);
        EXPECT_EQ(va_int64[1],2);
        EXPECT_EQ(va_int64[2],3);

        va_int64.set({1.0f,2.0f,3.0f});
        va_int64.print();
        va_int64 = {1.0f,2.0f,3.0f};
        va_int64.print();

        va_int64.set({1.0,2.0,3.0});
        va_int64.print();
        va_int64 = {1.0,2.0,3.0};
        va_int64.print();
    }

    // uint 8
    {
        va_uint8.set({1,2,3});
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);
        va_uint8.print();

        va_uint8 = {1,2,3};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1u,2u,3u});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1u,2u,3u};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1l,2l,3l});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1l,2l,3l};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1ul,2ul,3ul});
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8 = {1ul,2ul,3ul};
        va_uint8.print();
        EXPECT_EQ(va_uint8[0],1);
        EXPECT_EQ(va_uint8[1],2);
        EXPECT_EQ(va_uint8[2],3);

        va_uint8.set({1.0f,2.0f,3.0f});
        va_uint8.print();
        va_uint8 = {1.0f,2.0f,3.0f};
        va_uint8.print();

        va_uint8.set({1.0,2.0,3.0});
        va_uint8.print();
        va_uint8 = {1.0,2.0,3.0};
        va_uint8.print();
    }

    // uint 16
    {
        va_uint16.set({1,2,3});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1,2,3};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1u,2u,3u});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1u,2u,3u};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1l,2l,3l});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1l,2l,3l};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1ul,2ul,3ul});
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16 = {1ul,2ul,3ul};
        va_uint16.print();
        EXPECT_EQ(va_uint16[0],1);
        EXPECT_EQ(va_uint16[1],2);
        EXPECT_EQ(va_uint16[2],3);

        va_uint16.set({1.0f,2.0f,3.0f});
        va_uint16.print();
        va_uint16 = {1.0f,2.0f,3.0f};
        va_uint16.print();

        va_uint16.set({1.0,2.0,3.0});
        va_uint16.print();
        va_uint16 = {1.0,2.0,3.0};
        va_uint16.print();
    }

    // uint 32
    {
        va_uint32.set({1,2,3});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1,2,3};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1u,2u,3u});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1u,2u,3u};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1l,2l,3l});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1l,2l,3l};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1ul,2ul,3ul});
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32 = {1ul,2ul,3ul};
        va_uint32.print();
        EXPECT_EQ(va_uint32[0],1);
        EXPECT_EQ(va_uint32[1],2);
        EXPECT_EQ(va_uint32[2],3);

        va_uint32.set({1.0f,2.0f,3.0f});
        va_uint32.print();
        va_uint32 = {1.0f,2.0f,3.0f};
        va_uint32.print();

        va_uint32.set({1.0,2.0,3.0});
        va_uint32.print();
        va_uint32 = {1.0,2.0,3.0};
        va_uint32.print();
    }

    // uint 64
    {
        va_uint64.set({1,2,3});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1,2,3};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1u,2u,3u});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1u,2u,3u};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1l,2l,3l});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1l,2l,3l};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1ul,2ul,3ul});
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64 = {1ul,2ul,3ul};
        va_uint64.print();
        EXPECT_EQ(va_uint64[0],1);
        EXPECT_EQ(va_uint64[1],2);
        EXPECT_EQ(va_uint64[2],3);

        va_uint64.set({1.0f,2.0f,3.0f});
        va_uint64.print();
        va_uint64 = {1.0f,2.0f,3.0f};
        va_uint64.print();

        va_uint64.set({1.0,2.0,3.0});
        va_uint64.print();
        va_uint64 = {1.0,2.0,3.0};
        va_uint64.print();
    }

    // float 32
    {
        va_float32.set({-1,-2,-3});
        va_float32.print();
        va_float32 = {-1,2,-3};
        va_float32.print();

        va_float32.set({1u,2u,3u});
        va_float32.print();
        va_float32 = {1u,2u,3u};
        va_float32.print();

        va_float32.set({1l,2l,3l});
        va_float32.print();
        va_float32 = {-1l,2l,-3l};
        va_float32.print();

        va_float32.set({1ul,2ul,3ul});
        va_float32.print();
        va_float32 = {1ul,2ul,3ul};
        va_float32.print();

        va_float32.set({1.0f,2.0f,3.0f});
        va_float32.print();
        EXPECT_EQ(va_float32[0],1.0f);
        EXPECT_EQ(va_float32[1],2.0f);
        EXPECT_EQ(va_float32[2],3.0f);

        va_float32 = {1.0f,2.0f,3.0f};
        va_float32.print();
        EXPECT_EQ(va_float32[0],1.0f);
        EXPECT_EQ(va_float32[1],2.0f);
        EXPECT_EQ(va_float32[2],3.0f);

        va_float32.set({1.0,2.0,3.0});
        va_float32.print();
        va_float32 = {1.0,2.0,3.0};
        va_float32.print();
    }

    // float 64
    {
        va_float64.set({-1,-2,-3});
        va_float64.print();
        va_float64 = {-1,2,-3};
        va_float64.print();

        va_float64.set({1u,2u,3u});
        va_float64.print();
        va_float64 = {1u,2u,3u};
        va_float64.print();

        va_float64.set({1l,2l,3l});
        va_float64.print();
        va_float64 = {-1l,2l,-3l};
        va_float64.print();

        va_float64.set({1ul,2ul,3ul});
        va_float64.print();
        va_float64 = {1ul,2ul,3ul};
        va_float64.print();

        va_float64.set({1.0f,2.0f,3.0f});
        va_float64.print();
        va_float64 = {1.0f,2.0f,3.0f};
        va_float64.print();

        va_float64.set({1.0,2.0,3.0});
        va_float64.print();
        EXPECT_EQ(va_float64[0],1.0);
        EXPECT_EQ(va_float64[1],2.0);
        EXPECT_EQ(va_float64[2],3.0);

        va_float64 = {1.0,2.0,3.0};
        va_float64.print();
        EXPECT_EQ(va_float64[0],1.0);
        EXPECT_EQ(va_float64[1],2.0);
        EXPECT_EQ(va_float64[2],3.0);
    }
}
//-----------------------------------------------------------------------------
TEST(conduit_data_array, bulk_strided_and_offset)
{
    // An int8 destination viewing the odd entries of an interleaved buffer
    // (offset = 1 byte, stride = 2 bytes, 6 elements).
    {
        const index_t num_ele = 6;
        std::vector<int8> buff(12);
        for (index_t i = 0; i < 12; i++)
        {
            buff[(size_t)i] = (int8)(100 + i);
        }

        DataType dt = DataType::int8(num_ele,
                                     sizeof(int8),      // offset
                                     2 * sizeof(int8)); // stride

        int8_array va(&buff[0], dt);

        EXPECT_EQ(va[0], (int8)101);
        EXPECT_EQ(va[5], (int8)111);

        // Fill writes every viewed element and nothing else
        va.fill((int8)-7);
        for (index_t i = 0; i < num_ele; i++)
        {
            // The viewed element lives at buff[2 * i + 1], while the
            // interleaved gap next to it must be untouched
            EXPECT_EQ(buff[(size_t)(2 * i + 1)], (int8)-7);
            EXPECT_EQ(buff[(size_t)(2 * i)], (int8)(100 + 2 * i));
        }

        // Bulk set from a pointer source
        int8 src[6] = {-1, 2, -3, 4, -5, 6};
        va.set(src, num_ele);
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(buff[(size_t)(2 * i + 1)], src[i]);
            EXPECT_EQ(buff[(size_t)(2 * i)], (int8)(100 + 2 * i));
        }

        // Bulk set from an init list source
        va.set({10, -20, 30, -40, 50, -60});
        int8 expected[6] = {10, -20, 30, -40, 50, -60};
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(buff[(size_t)(2 * i + 1)], expected[i]);
            EXPECT_EQ(buff[(size_t)(2 * i)], (int8)(100 + 2 * i));
        }
    }

    // A float64 destination (8 byte elements), node backed via set_external
    // (offset = 8 bytes, stride = 16 bytes, 6 elements)
    {
        const index_t num_ele = 6;
        std::vector<float64> buff(12);
        for (index_t i = 0; i < 12; i++)
        {
            buff[(size_t)i] = 1000.0 + (float64)i;
        }

        DataType dt = DataType::float64(num_ele,
                                        sizeof(float64),      // offset
                                        2 * sizeof(float64)); // stride

        Node n;
        n["vals"].set_external(dt, &buff[0]);
        float64_array va = n["vals"].value();

        va.fill((float64)-3.5);
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(buff[(size_t)(2 * i + 1)], -3.5);
            EXPECT_EQ(buff[(size_t)(2 * i)], 1000.0 + (float64)(2 * i));
        }

        float64 src[6] = {-1.25, 2.5, -3.75, 4.5, -5.25, 6.125};
        va.set(src, num_ele);
        for (index_t i = 0; i < num_ele; i++)
        {
            EXPECT_EQ(buff[(size_t)(2 * i + 1)], src[i]);
            EXPECT_EQ(buff[(size_t)(2 * i)], 1000.0 + (float64)(2 * i));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, set_cross_type_sources)
{
    const index_t num_ele = 5;

    std::vector<int32> v_int32(5);
    v_int32[0] = -1;
    v_int32[1] =  2;
    v_int32[2] = -300;
    v_int32[3] =  4000;
    v_int32[4] = -50000;
    int32_array va_int32(&v_int32[0], DataType::int32(num_ele));

    // The float64 sources include values that truncate when cast to an int
    std::vector<float64> v_float64(5);
    v_float64[0] =  1.5;
    v_float64[1] = -2.5;
    v_float64[2] =  3.99;
    v_float64[3] = -4.01;
    v_float64[4] =  1000.75;
    float64_array    va_float64(&v_float64[0], DataType::float64(num_ele));
    float64_accessor vacc_float64(&v_float64[0], DataType::float64(num_ele));

    Node n;

    // A float64 destination set from an int32 DataArray source (widening)
    n["dest_f64"].set(DataType::float64(num_ele));
    float64_array dest_f64 = n["dest_f64"].value();
    dest_f64.set(va_int32);
    for (index_t i = 0; i < num_ele; i++)
    {
        EXPECT_EQ(dest_f64[i], (float64)v_int32[(size_t)i]);
    }

    // An int32 destination set from a float64 DataArray source (truncating)
    n["dest_i32"].set(DataType::int32(num_ele));
    int32_array dest_i32 = n["dest_i32"].value();
    dest_i32.set(va_float64);
    int32 expected[5] = {1, -2, 3, -4, 1000};
    for (index_t i = 0; i < num_ele; i++)
    {
        EXPECT_EQ(dest_i32[i], expected[i]);
    }

    // An int32 destination set from a float64 DataAccessor source (truncating)
    dest_i32.fill((int32)0);
    dest_i32.set(vacc_float64);
    for (index_t i = 0; i < num_ele; i++)
    {
        EXPECT_EQ(dest_i32[i], expected[i]);
    }

    // A strided + offset int32 destination set from a float64 DataArray source
    std::vector<int32> buff(10);
    for (index_t i = 0; i < 10; i++)
    {
        buff[(size_t)i] = (int32)(1000 + i);
    }

    DataType dt = DataType::int32(num_ele,
                                  sizeof(int32),      // offset
                                  2 * sizeof(int32)); // stride
    int32_array dest_strided(&buff[0], dt);

    dest_strided.set(va_float64);
    for (index_t i = 0; i < num_ele; i++)
    {
        EXPECT_EQ(buff[(size_t)(2 * i + 1)], expected[i]);
        // The interleaved gaps must be untouched
        EXPECT_EQ(buff[(size_t)(2 * i)], (int32)(1000 + 2 * i));
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, device_fill)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        Node n;
        n["vals"].set(DataType::float64(num_elements));

        // Ask the array to move its data to the memory space occupied by the
        // requested execution policy if its data is not already there.
        float64_array va(n["vals"]);
        va.use_with(policy);
        EXPECT_EQ(va.active_space().is_device_policy(), policy.is_device_policy());

        va.fill(static_cast<float64>(-3.5));

        // Sync values to n["vals"].
        va.sync();

        float64 *vals_ptr = n["vals"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(vals_ptr[i], -3.5);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, device_set_from_host_ptr)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    // Host-based source
    std::vector<float64> src(num_elements);
    for (index_t i = 0; i < num_elements; i++)
    {
        src[static_cast<size_t>(i)] = 1.5 * static_cast<float64>(i + 1);
    }

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        Node n;
        n["vals"].set(DataType::float64(num_elements));

        // Move the data if necessary
        float64_array va(n["vals"]);
        va.use_with(policy);

        // set() requires the source and destination to share a memory space
        if (policy.is_device_policy())
        {
            EXPECT_THROW(va.set(&src[0], num_elements), conduit::Error);
            va.use_with(ExecutionPolicy::host());
        }

        va.set(&src[0], num_elements);

        va.use_with(policy);
        va.sync();

        float64 *vals_ptr = n["vals"].value();
        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(vals_ptr[i], src[static_cast<size_t>(i)]);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, device_summary_stats)
{
    conduit_device_prepare();

    const index_t num_elements = 16;

    std::vector<float64> src(num_elements);
    float64 expected_sum = 0.0;
    for (index_t i = 0; i < num_elements; i++)
    {
        src[static_cast<size_t>(i)] = static_cast<float64>(i - num_elements / 2);
        expected_sum += src[static_cast<size_t>(i)];
    }

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        Node n;
        n["vals"].set(src);

        float64_array va(n["vals"]);
        va.use_with(policy);

        EXPECT_EQ(va.min(), src[0]);
        EXPECT_EQ(va.max(), src[static_cast<size_t>(num_elements - 1)]);
        EXPECT_EQ(va.sum(), expected_sum);
        EXPECT_EQ(va.mean(), expected_sum / static_cast<float64>(num_elements));
        EXPECT_EQ(va.count(0.0), 1);
        EXPECT_EQ(va.count(1000.0), 0);
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, device_strided)
{
    conduit_device_prepare();

    // View the odd entries of an interleaved buffer
    const index_t num_elements = 8;
    const index_t num_buff = 2 * num_elements;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        Node n;
        n["vals"].set(DataType::float64(num_elements,
                                        sizeof(float64),        // offset
                                        2 * sizeof(float64)));  // stride

        float64 *buff = static_cast<float64*>(n["vals"].data_ptr());
        for (index_t i = 0; i < num_buff; i++)
        {
            buff[i] = 1000.0 + static_cast<float64>(i);
        }

        float64_array va(n["vals"]);
        va.use_with(policy);

        va.fill(static_cast<float64>(-3.5));

        va.sync();

        for (index_t i = 0; i < num_elements; i++)
        {
            EXPECT_EQ(buff[static_cast<size_t>(2 * i + 1)], -3.5);
            // The interleaved gaps must not have changed
            EXPECT_EQ(buff[static_cast<size_t>(2 * i)], 1000.0 + static_cast<float64>(2 * i));
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_data_array, mixed_space_set_errors)
{
    conduit_device_prepare();

    const index_t num_ele = 8;

    for_each_enabled_policy([&](ExecutionPolicy &policy)
    {
        // set() between different memory spaces throws an error
        if (!policy.is_device_policy())
        {
            return;
        }

        Node n;
        n["src"].set(DataType::float64(num_ele));
        n["des"].set(DataType::float64(num_ele));

        float64_array dev_src(n["src"]);
        dev_src.use_with(policy);

        float64_array host_des(n["des"]);

        EXPECT_THROW(host_des.set(dev_src), conduit::Error);

        float64_array host_src(n["des"]);
        EXPECT_THROW(dev_src.set(host_src), conduit::Error);
    });
}
