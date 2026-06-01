// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_device.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(conduit_device, about_check)
{
    conduit::Node n;
    conduit::about(n);
    std::cout << n.to_yaml() << std::endl;

    EXPECT_TRUE(n.has_child("device_support"));

#if defined(CONDUIT_USE_CUDA)
    EXPECT_EQ(n["device_support"].as_string(),"cuda");
#elif defined(CONDUIT_USE_HIP)
    EXPECT_EQ(n["device_support"].as_string(),"hip");
#else
    EXPECT_EQ(n["device_support"].as_string(),"disabled");
#endif

}

