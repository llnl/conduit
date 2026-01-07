// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_smoke.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"

#include <iostream>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(conduit_smoke, basic_use)
{
    EXPECT_EQ(sizeof(conduit::uint32),4);
    EXPECT_EQ(sizeof(conduit::uint64),8);
    EXPECT_EQ(sizeof(conduit::float64),8);

    std::cout << conduit::about() << std::endl;
}

//-----------------------------------------------------------------------------
TEST(conduit_smoke, version_macro)
{
    EXPECT_EQ(CONDUIT_VERSION_VALUE, CONDUIT_MAKE_VERSION_VALUE(CONDUIT_VERSION_MAJOR, CONDUIT_VERSION_MINOR, CONDUIT_VERSION_PATCH));

    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(0,9,5) < CONDUIT_MAKE_VERSION_VALUE(0,9,6));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(0,9,50) < CONDUIT_MAKE_VERSION_VALUE(0,9,51));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(0,9,50) < CONDUIT_MAKE_VERSION_VALUE(1,0,0));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(0,10,0) < CONDUIT_MAKE_VERSION_VALUE(1,0,0));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(1,0,0) < CONDUIT_MAKE_VERSION_VALUE(1,0,1));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(1,0,0) < CONDUIT_MAKE_VERSION_VALUE(1,1,0));
    EXPECT_TRUE(CONDUIT_MAKE_VERSION_VALUE(1,0,1) < CONDUIT_MAKE_VERSION_VALUE(1,1,0));

#if CONDUIT_MAKE_VERSION_VALUE(0, 9, 6) >= CONDUIT_MAKE_VERSION_VALUE(0, 9, 5)
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(false);
#endif
}
