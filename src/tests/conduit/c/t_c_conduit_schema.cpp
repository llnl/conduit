// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_c_conduit_schema.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.h"

#include "gtest/gtest.h"

//-----------------------------------------------------------------------------
TEST(c_conduit_schema, basic_use)
{
    conduit_schema *s1 = conduit_schema_create();
    conduit_schema_set_json(s1,
                            "{ a: {dtype:int64, length:10},"
                            "  b: {dtype:float64, length:20} }");

    EXPECT_EQ(conduit_schema_number_of_children(s1), 2);
    EXPECT_EQ(conduit_schema_has_child(s1, "a"), 1);
    EXPECT_EQ(conduit_schema_has_path(s1, "b"), 1);

    conduit_schema *a = conduit_schema_child_by_name(s1, "a");
    const conduit_datatype *a_dtype = conduit_schema_dtype(a);
    EXPECT_EQ(conduit_datatype_is_int64(a_dtype), 1);

    char *json = conduit_schema_to_json(s1);
    EXPECT_TRUE(json != NULL);
    conduit_schema_string_destroy(json);

    conduit_schema *s2 = conduit_schema_create();
    conduit_schema_set(s2, s1);
    EXPECT_EQ(conduit_schema_equals(s2, s1), 1);

    conduit_schema_destroy(s2);
    conduit_schema_destroy(s1);
}

//-----------------------------------------------------------------------------
TEST(c_conduit_schema, node_schema_access)
{
    conduit_node *n = conduit_node_create();
    conduit_node_set_path_int64(n, "a", 10);

    const conduit_schema *s = conduit_node_schema(n);
    EXPECT_TRUE(s != NULL);
    EXPECT_EQ(conduit_schema_has_child(s, "a"), 1);

    char *json = conduit_schema_to_json(s);
    EXPECT_TRUE(json != NULL);
    conduit_schema_string_destroy(json);

    conduit_node_destroy(n);
}

