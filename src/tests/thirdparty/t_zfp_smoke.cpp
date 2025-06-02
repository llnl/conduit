// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_zfp_smoke.cpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// external lib includes
//-----------------------------------------------------------------------------
#include "zfp/array1.hpp"
#include "zfp/array2.hpp"
#include "zfp/array3.hpp"

#include <iostream>
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(zfp_smoke, basic_use)
{
    // create a compressed zfp array
    uint nx = 9;
    uint ny = 12;
    uint ntotal = nx * ny;
    float * vals = new float[ntotal];

    for (uint i = 0; i < ntotal; i++)
    {
        vals[i] = i*i;
    }

    double rate = 8.0;
    zfp::array2f arr(nx, ny, rate, vals);

    delete [] vals;
}
