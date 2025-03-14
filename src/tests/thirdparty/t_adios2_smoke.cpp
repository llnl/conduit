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
#include <iostream>
#include "gtest/gtest.h"

// Adapted from ADIOS2' global aggregate by color test.

//-----------------------------------------------------------------------------
TEST(adios2_smoke, basic_use)
{
    int rank = 0;
    int comm = 0;
    int NX = 5;
    const char *filename = "adios2_smoke.bp";
    int64_t m_adios2_group;
    int64_t m_adios2_file;

    int status = adios2_init_noxml(comm);
    EXPECT_TRUE(status >= 0 );

    adios2_set_max_buffer_size (10);

    status = adios2_declare_group(&m_adios2_group, "restart", "iter", adios2_stat_default);
    EXPECT_TRUE(status >= 0 );

    status = adios2_select_method(m_adios2_group, "POSIX", "", "");
    EXPECT_TRUE(status >= 0 );

    adios2_define_var(m_adios2_group, "NX",
                     "", adios2_integer,
                     0, 0, 0);

    status = adios2_open(&m_adios2_file, "restart", filename, "w", comm);

    status = adios2_write(m_adios2_file, "NX", (void *) &NX);
    EXPECT_TRUE(status >= 0 );

    status = adios2_close(m_adios2_file);
    EXPECT_TRUE(status >= 0 );

    status = adios2_finalize(rank);
    EXPECT_TRUE(status >= 0 );
}
