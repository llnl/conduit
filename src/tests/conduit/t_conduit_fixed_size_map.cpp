// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_fixed_size_map.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_fixed_size_map.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include "gtest/gtest.h"

using int_map3 = conduit::fixed_size_map<int, int, 3>;
using str_map2 = conduit::fixed_size_map<std::string, std::string, 2>;

TEST(FixedSizeMap, DefaultConstruction) {
    int_map3 m;
    EXPECT_EQ(m.size(), 0);
}

TEST(FixedSizeMap, InsertAndRetrieve) {
    int_map3 m;
    m[1] = 10;
    m[2] = 20;
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
}

TEST(FixedSizeMap, OverwriteValue) {
    int_map3 m;
    m[1] = 10;
    m[1] = 99;
    EXPECT_EQ(m.size(), 1);
    EXPECT_EQ(m[1], 99);
}

TEST(FixedSizeMap, InsertUpToCapacity) {
    int_map3 m;
    m[1] = 1;
    m[2] = 2;
    m[3] = 3;
    EXPECT_EQ(m.size(), 3);
    EXPECT_EQ(m[1], 1);
    EXPECT_EQ(m[2], 2);
    EXPECT_EQ(m[3], 3);
}

TEST(FixedSizeMap, InsertBeyondCapacityThrows) {
    int_map3 m;
    m[1] = 1;
    m[2] = 2;
    m[3] = 3;
    EXPECT_THROW(m[4] = 4, conduit::Error);
}

TEST(FixedSizeMap, ConstAccessExistingKey) {
    int_map3 m;
    m[5] = 42;
    const int_map3 &cm = m;
    EXPECT_EQ(cm[5], 42);
}

TEST(FixedSizeMap, ConstAccessNonExistingKeyThrows) {
    int_map3 m;
    m[1] = 10;
    const int_map3 &cm = m;
    EXPECT_THROW((void)cm[2], conduit::Error);
}

TEST(FixedSizeMap, GetByIndex) {
    int_map3 m;
    m[1] = 11;
    m[2] = 22;
    EXPECT_EQ(m.get(0), 11);
    EXPECT_EQ(m.get(1), 22);
}

TEST(FixedSizeMap, StringKeyValue) {
    str_map2 m;
    m["a"] = "apple";
    m["b"] = "banana";
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m["a"], "apple");
    EXPECT_EQ(m["b"], "banana");
}

TEST(FixedSizeMap, FindReturnsCorrectPointer) {
    int_map3 m;
    m[7] = 77;
    m[8] = 88;
    // Find is private, but we can test via operator[]
    m[7] = 777;
    EXPECT_EQ(m[7], 777);
    EXPECT_EQ(m.size(), 2);
}

TEST(FixedSizeMap, InsertDuplicateKeyDoesNotIncreaseSize) {
    int_map3 m;
    m[1] = 10;
    m[1] = 20;
    EXPECT_EQ(m.size(), 1);
}

TEST(FixedSizeMap, InsertMultipleTypes) {
    conduit::fixed_size_map<char, double, 2> m;
    m['x'] = 3.14;
    m['y'] = 2.71;
    EXPECT_EQ(m.size(), 2);
    EXPECT_DOUBLE_EQ(m['x'], 3.14);
    EXPECT_DOUBLE_EQ(m['y'], 2.71);
}

TEST(FixedSizeMap, GetOutOfBoundsIndex) {
    int_map3 m;
    m[1] = 10;
    EXPECT_NO_THROW((void)m.get(0));
    // No bounds checking, so this may be UB, but should not crash
    (void)m.get(2);
}
