// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: adios2_smoke.cpp
///
//-----------------------------------------------------------------------------

// Serial test only.
#define _NOMPI

#include <adios2.h>
#include <cassert>
#include <iostream>
#include "gtest/gtest.h"

// Adapted from ADIOS2' global aggregate by color test.

//-----------------------------------------------------------------------------
TEST(adios2_smoke, basic_use)
{
    #warning "TODO"
    assert(false);
    //TODO int rank = 0;
    //TODO int comm = 0;
    //TODO int NX = 5;
    //TODO const char *filename = "adios2_smoke.bp";
    //TODO int64_t m_adios2_group;
    //TODO int64_t m_adios2_file;
    //TODO 
    //TODO int status = adios2_init_noxml(comm);
    //TODO EXPECT_TRUE(status >= 0 );
    //TODO 
    //TODO adios2_set_max_buffer_size (10);
    //TODO 
    //TODO status = adios2_declare_group(&m_adios2_group, "restart", "iter", adios2_stat_default);
    //TODO EXPECT_TRUE(status >= 0 );
    //TODO 
    //TODO status = adios2_select_method(m_adios2_group, "POSIX", "", "");
    //TODO EXPECT_TRUE(status >= 0 );
    //TODO 
    //TODO adios2_define_var(m_adios2_group, "NX",
    //TODO                  "", adios2_integer,
    //TODO                  0, 0, 0);
    //TODO 
    //TODO status = adios2_open(&m_adios2_file, "restart", filename, "w", comm);
    //TODO 
    //TODO status = adios2_write(m_adios2_file, "NX", (void *) &NX);
    //TODO EXPECT_TRUE(status >= 0 );
    //TODO 
    //TODO status = adios2_close(m_adios2_file);
    //TODO EXPECT_TRUE(status >= 0 );
    //TODO 
    //TODO status = adios2_finalize(rank);
    //TODO EXPECT_TRUE(status >= 0 );
}
