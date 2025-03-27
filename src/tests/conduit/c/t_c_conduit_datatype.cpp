// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_c_conduit_datatype.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.h"

#include <stdio.h>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(c_conduit_datatype, sizeof_index_t)
{
    int sz_it = conduit_datatype_sizeof_index_t();

#ifdef CONDUIT_INDEX_32
    EXPECT_EQ(sz_it,4);
#else
    EXPECT_EQ(sz_it,8);
#endif

}

//-----------------------------------------------------------------------------
TEST(c_conduit_datatype, stride_methods)
{
    conduit_node *n = conduit_node_create();
    
    // Create a simple array with stride = element_bytes
    conduit_float64 arr_vals[] = {10.0, 20.0, 30.0, 40.0, 50.0};
    conduit_node_set_external_float64_ptr(n, arr_vals, 5);
    
    // Get the datatype
    const conduit_datatype *dt = conduit_node_dtype(n);
    
    // Check stride-related properties
    conduit_index_t stride = conduit_datatype_stride(dt);
    conduit_index_t element_bytes = conduit_datatype_element_bytes(dt);
    conduit_index_t element_stride = conduit_datatype_element_stride(dt);
    
    // Verify that stride / element_bytes = element_stride
    EXPECT_EQ(stride / element_bytes, element_stride);
    
    // Since this is an array with standard layout, stride should be aligned with element bytes
    EXPECT_EQ(conduit_datatype_is_stride_element_aligned(dt), 1);
    
    // Test stride aligned with various byte sizes
    EXPECT_EQ(conduit_datatype_is_stride_aligned(dt, 8), 1);  // Stride should be aligned with 8 bytes
    EXPECT_EQ(conduit_datatype_is_stride_aligned(dt, 4), 1);  // Stride should be aligned with 4 bytes
    EXPECT_EQ(conduit_datatype_is_stride_aligned(dt, 2), 1);  // Stride should be aligned with 2 bytes
    
    // Clean up
    conduit_node_destroy(n);
}

