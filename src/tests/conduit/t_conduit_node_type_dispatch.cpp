// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_node_type_dispatch.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"
using namespace conduit;
using namespace conduit::utils;


index_t ARRAY_SIZE = 1000000;
index_t NUM_ITERS  = 1;


//-----------------------------------------------------------------------------
// -- type specific template imp 
template<typename T>
float64
pointer_dispatch_detail(Node &n)
{
    float64 res = 0;
    index_t nele = n.dtype().number_of_elements();
    T *t_ptr = n.value();
    for(index_t i=0; i < nele;i++)
    {
        res += t_ptr[i];
    }

    return res;
}

//-----------------------------------------------------------------------------
// -- array type specific template imp 
template<typename T>
float64
array_dispatch_detail(Node &n)
{
    float64 res = 0;
    T t_array = n.value();
    index_t nele = n.dtype().number_of_elements();
    // T *t_ptr = n.value();
    for(index_t i=0; i < nele;i++)
    {
        res += t_array[i];
    }

    return res;
}

//-----------------------------------------------------------------------------
// -- use template dispatch (FASTEST but does not handle strange strides)
float64 
pointer_dispatch(Node &n)
{
    if(n.dtype().is_float64())
    {
        return pointer_dispatch_detail<float64>(n);
    }
    else if(n.dtype().is_float32())
    {
        return pointer_dispatch_detail<float32>(n);
    }

    return 0.0;
}

//-----------------------------------------------------------------------------
// -- use template data array dispatch (this handles all strides!)
float64 
array_dispatch(Node &n)
{
    if(n.dtype().is_float64())
    {
        return array_dispatch_detail<float64_array>(n);
    }
    else if(n.dtype().is_float32())
    {
        return array_dispatch_detail<float32_array>(n);
    }

    return 0.0;
}


//-----------------------------------------------------------------------------
// -- convert each element to desired type in the loop (this handles all strides)
float64 
on_the_fly_convert(Node &n)
{
    float64 res = 0;
    index_t nele = n.dtype().number_of_elements();

    Node temp;
    for(index_t i=0; i < nele;i++)
    {
        temp.set_external(n.dtype(),n.element_ptr(i));
        res += temp.to_float64();
    }

    return res;
}

//-----------------------------------------------------------------------------
// -- convert the array to desired type, then loop (this handles all strides)
float64 
array_convert(Node &n)
{
    float64 res = 0;
    index_t nele = n.dtype().number_of_elements();

    Node temp;
    n.to_float64_array(temp);
    float64 *f64_ptr = temp.value();

    for(index_t i=0; i < nele;i++)
    {
        res += f64_ptr[i];
    }

    return res;
}


//-----------------------------------------------------------------------------
// -- use accessor
float64 
data_accessor(Node &n)
{
    float64_accessor vals = n.value();
    float64 res = 0;
    index_t nele = vals.number_of_elements();

    for(index_t i=0; i < nele;i++)
    {
        res += vals[i];
    }

    return res;
}


//-----------------------------------------------------------------------------
TEST(conduit_node, test_dispatch)
{
    Node n;
    
    n.set(DataType::float32(ARRAY_SIZE));
    float32 *f32_ptr = n.value();
    for(index_t i=0;i<ARRAY_SIZE;i++)
    {
        f32_ptr[i] = 1.0;
    }

    std::cout << "Number of elements: " <<  ARRAY_SIZE << std::endl;
    std::cout << "Iterations: " << NUM_ITERS << std::endl;

    float64 res1,res2,res3,res4,res5;

    Timer t1;
    for(index_t i=0;i<NUM_ITERS;i++)
    {
        res1 = on_the_fly_convert(n);
    }
    float t1_tval = t1.elapsed();

    Timer t2;
    for(index_t i=0;i<NUM_ITERS;i++)
    {
        res2 = array_convert(n);
    }
    float t2_tval = t2.elapsed();

    Timer t3;
    for(index_t i=0;i<NUM_ITERS;i++)
    {
        res3 = pointer_dispatch(n);
    }
    float t3_tval = t3.elapsed();

    Timer t4;
    for(index_t i=0;i<NUM_ITERS;i++)
    {
        res4 = array_dispatch(n);
    }
    float t4_tval = t4.elapsed();


    Timer t5;
    for(index_t i=0;i<NUM_ITERS;i++)
    {
        res5 = data_accessor(n);
    }
    float t5_tval = t5.elapsed();


    EXPECT_EQ(res1,float64(ARRAY_SIZE));
    EXPECT_EQ(res2,float64(ARRAY_SIZE));
    EXPECT_EQ(res3,float64(ARRAY_SIZE));
    EXPECT_EQ(res4,float64(ARRAY_SIZE));
    EXPECT_EQ(res5,float64(ARRAY_SIZE));


    std::cout << "on_the_fly_convert: " << t1_tval << std::endl;
    std::cout << "array_convert:      " << t2_tval << std::endl;
    std::cout << "pointer_dispatch:   " << t3_tval << std::endl;
    std::cout << "array_dispatch:     " << t4_tval << std::endl;
    std::cout << "data_accessor:      " << t5_tval << std::endl;

}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);

    // allow override of the data size via the command line
    if(argc > 1)
    {
        ARRAY_SIZE = atoi(argv[1]);
    }

    if(argc > 2)
    {
        NUM_ITERS = atoi(argv[2]);
    }


    result = RUN_ALL_TESTS();
    return result;
}




