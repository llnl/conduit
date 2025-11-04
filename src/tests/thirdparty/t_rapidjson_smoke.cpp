// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_rapidjson_smoke.cpp
///
//-----------------------------------------------------------------------------

#include "rapidjson/document.h"
#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(rapidjson_smoke, basic_use)
{
    const char json[] = "{ \"hello\" : \"world\" }";

    conduit_rapidjson::Document d;
    d.Parse<0>(json);

    ASSERT_STREQ(d["hello"].GetString(),"world");
}