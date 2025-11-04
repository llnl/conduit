// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_yyjson_smoke.cpp
///
//-----------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "conduit_yyjson.h"

using namespace conduit_yyjson;

//-----------------------------------------------------------------------------
TEST(yyjson_smoke, basic_use)
{
    const char json[] = "{ \"hello\" : \"world\" }";
    yyjson_doc *doc = yyjson_read(json, strlen(json), 0);
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *name = yyjson_obj_get(root, "hello");
    ASSERT_STREQ(yyjson_get_str(name),"world");
    yyjson_doc_free(doc);
}
