// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_smoke.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"


struct TestAllocator
{
    static size_t m_total_bytes_alloced;
    static size_t m_alloc_count;
    static size_t m_free_count;
    static size_t m_memcpy_count;
    static size_t m_memset_count;

    static void * banana_alloc(size_t items, size_t item_size)
    {
        std::cout<<"Bananas allocate\n";
        m_total_bytes_alloced += items * item_size;
        m_alloc_count++;
        return calloc(items, item_size);
    }

    static void free_bananas(void *data_ptr)
    {
        std::cout<<"free bananas\n";
        m_free_count++;
        free(data_ptr);
    }

    static void banana_memset(void * ptr, int value, size_t num )
    {
        m_memset_count++;
        std::cout<<"set bananas\n";
        memset(ptr,value,num);
    }

    static void banana_copy(void * destination, const void * source, size_t num)
    {
        m_memcpy_count++;
        std::cout<<"copy bananas\n";
        memcpy(destination,source,num);
    }

    static void all_about_bananas()
    {
        std::cout << "[total_bytes_alloced: " << m_total_bytes_alloced <<
                     " alloc_count: "  << m_alloc_count <<
                     " free_count: "   << m_free_count <<
                     " memcpy_count: " << m_memcpy_count <<
                     " memset_count: " << m_memset_count << std::endl;
    }
};


size_t TestAllocator::m_total_bytes_alloced = 0;
size_t TestAllocator::m_alloc_count  = 0;
size_t TestAllocator::m_free_count   = 0;
size_t TestAllocator::m_memcpy_count = 0;
size_t TestAllocator::m_memset_count = 0;

//-----------------------------------------------------------------------------
TEST(conduit_memory_allocator, test_custom_allocator)
{
    conduit::Node node_info;
    conduit::Node other_info;

    conduit::utils::set_memcpy_handler(TestAllocator::banana_copy);
    conduit::utils::set_memset_handler(TestAllocator::banana_memset);

    conduit::index_t allocator_id
     = conduit::utils::register_allocator(TestAllocator::banana_alloc,
                                          TestAllocator::free_bananas);

    EXPECT_EQ(TestAllocator::m_memset_count,0);

    conduit::Node node;
    node.set_allocator(allocator_id);
    // this should allocate 10 * 8 bytes
    node["array"].set(conduit::DataType::float64(10));

    TestAllocator::all_about_bananas();
    EXPECT_EQ(TestAllocator::m_total_bytes_alloced,80);
    EXPECT_EQ(TestAllocator::m_alloc_count,1);
    EXPECT_EQ(TestAllocator::m_free_count,0);

    // this should allocate 4 bytes
    node["int"].set_int32(1);

    TestAllocator::all_about_bananas();
    EXPECT_EQ(TestAllocator::m_total_bytes_alloced,84);
    EXPECT_EQ(TestAllocator::m_alloc_count,2);
    EXPECT_EQ(TestAllocator::m_free_count,0);

    // ----------------------------------------------
    conduit::Node other;
    other["path"].set_float32(1.0);

    // this should allocate 4 bytes
    node.update(other);

    EXPECT_EQ(TestAllocator::m_total_bytes_alloced,88);
    EXPECT_EQ(TestAllocator::m_alloc_count,3);
    EXPECT_EQ(TestAllocator::m_free_count,0);

    node.info(node_info);
    other.info(other_info);

    // this should free everything, free count == 3
    node.reset();
    TestAllocator::all_about_bananas();
    EXPECT_EQ(TestAllocator::m_alloc_count,3);
    EXPECT_EQ(TestAllocator::m_free_count,3);

    std::cout << "Main Node Info" << std::endl;
    node_info.print();
    // all pointers should be use alloc_id = 1
    conduit::NodeConstIterator ptr_itr = node_info["mem_spaces"].children();
    while(ptr_itr.has_next())
    {
        const conduit::Node &p_n = ptr_itr.next();
#if defined(CONDUIT_USE_DEVICE)
        EXPECT_EQ(p_n["allocator_id"].to_int64(),3);
#else
        EXPECT_EQ(p_n["allocator_id"].to_int64(),1);
#endif
    }
    
    std::cout << "Other Node Info" << std::endl;
    other_info.print();

    // all pointers should be use alloc_id = 0
    ptr_itr = other_info["mem_spaces"].children();
    while(ptr_itr.has_next())
    {
        const conduit::Node &p_n = ptr_itr.next();
        EXPECT_EQ(p_n["allocator_id"].to_int64(),0);
    }
}


struct StrangeAllocator
{
    static void * strange_alloc(size_t items, size_t item_size)
    {
        std::cout<<"Strange allocate\n";
        return calloc(items, item_size);
    }

    static void strange_free(void *data_ptr)
    {
        std::cout<<"strange free \n";
        free(data_ptr);
    }

    static void strange_memset(void * ptr, int value, size_t num )
    {
        // init with -1 bytes
        std::cout<<"strange set\n";
        memset(ptr,-1,num);
    }

    static void strange_copy(void * destination, const void * source, size_t num)
    {
        std::cout<<"strange copy\n";
        conduit::uint8 *des_ptr = (conduit::uint8*)destination;
        for(int i=0;i<num;i++)
        {
            des_ptr[i] = (conduit::uint8) (i%2 ? 0:1);
        }
    }

};


//-----------------------------------------------------------------------------
TEST(conduit_memory_allocator, test_strange)
{
    conduit::utils::set_memcpy_handler(StrangeAllocator::strange_copy);
    conduit::utils::set_memset_handler(StrangeAllocator::strange_memset);

    conduit::index_t allocator_id
     = conduit::utils::register_allocator(StrangeAllocator::strange_alloc,
                                          StrangeAllocator::strange_free);


    conduit::Node n1, n2;
    n1.set_allocator(allocator_id);
    n1.set(conduit::DataType::uint8(3));
    n1.print();

    std::cout << "set" << std::endl;
    n2.set(n1);
    n2.print();

    conduit::uint8_array n2_ptr = n2.value();
    EXPECT_EQ(n2_ptr[0],1);
    EXPECT_EQ(n2_ptr[1],0);
    EXPECT_EQ(n2_ptr[2],1);

    std::cout << "update (blank dest)" << std::endl;
    n2.reset();
    // update incompat case
    n2.update(n1);
    n2.print();
    conduit::uint8_array n2_vals = n2.value();

    EXPECT_EQ(n2_vals[0],1);
    EXPECT_EQ(n2_vals[1],0);
    EXPECT_EQ(n2_vals[2],1);

    std::cout << "update (compat dest)" << std::endl;
    n2.reset();
    // trigger update case we want to test (compat update)
    n2.set(conduit::DataType::uint8(3));
    n2.update(n1);
    n2.print();
    n2_vals = n2.value();

    EXPECT_EQ(n2_vals[0],1);
    EXPECT_EQ(n2_vals[1],0);
    EXPECT_EQ(n2_vals[2],1);


    std::cout << "update_compatible (compat dest)" << std::endl;
    n2.reset();
    // trigger update case we want to test (compat update)
    n2.set(conduit::DataType::uint8(3));
    n2.update_compatible(n1);
    n2.print();
    n2_vals = n2.value();

    EXPECT_EQ(n2_vals[0],1);
    EXPECT_EQ(n2_vals[1],0);
    EXPECT_EQ(n2_vals[2],1);

    n2.reset();
    n2.set(conduit::DataType::uint8(3));

    conduit::uint8 buff[3] = {0,0,0};
    n2_vals = n2.value();

    n2_vals.compact_elements_to(buff);

    EXPECT_EQ(buff[0],1);
    EXPECT_EQ(buff[1],0);
    EXPECT_EQ(buff[2],1);
}
