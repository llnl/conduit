// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_fixed_size_vector.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_fixed_size_vector.hpp"

#include <algorithm>
#include <array>
#include <string>
#include "gtest/gtest.h"

using int_vec5 = conduit::fixed_size_vector<int, 5>;
using str_vec3 = conduit::fixed_size_vector<std::string, 3>;

TEST(FixedSizeVector, DefaultConstruction) {
    int_vec5 v;
    EXPECT_EQ(v.size(), 0);
}

TEST(FixedSizeVector, PushBackAndSize) {
    int_vec5 v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], 3);
}

TEST(FixedSizeVector, PushBackBeyondCapacity) {
    int_vec5 v;
    for(int i = 0; i < 10; ++i) v.push_back(i);
    EXPECT_EQ(v.size(), 5);
    for(int i = 0; i < 5; ++i) {
        EXPECT_EQ(v[i], i);
    }
}

TEST(FixedSizeVector, Clear) {
    int_vec5 v;
    v.push_back(42);
    v.push_back(43);
    v.clear();
    EXPECT_EQ(v.size(), 0);
    // push_back should work after clear
    v.push_back(99);
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v[0], 99);
}

TEST(FixedSizeVector, ResizeSmaller) {
    int_vec5 v;
    for(int i = 0; i < 5; ++i) v.push_back(i);
    v.resize(2);
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[1], 1);
}

TEST(FixedSizeVector, ResizeLarger) {
    int_vec5 v;
    v.push_back(1);
    v.resize(4);
    // Only size should change, uninitialized elements may have garbage values
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[0], 1);
}

TEST(FixedSizeVector, ResizeBeyondCapacity) {
    int_vec5 v;
    v.resize(10);
    EXPECT_EQ(v.size(), 5);
}

TEST(FixedSizeVector, ElementAccess) {
    int_vec5 v;
    v.push_back(7);
    v.push_back(8);
    v[1] = 42;
    EXPECT_EQ(v[1], 42);
    const int_vec5 &cv = v;
    EXPECT_EQ(cv[1], 42);
}

TEST(FixedSizeVector, Iteration) {
    int_vec5 v;
    for(int i = 0; i < 5; ++i) v.push_back(i * 2);
    int sum = 0;
    for(auto it = v.begin(); it != v.begin() + v.size(); ++it)
        sum += *it;
    EXPECT_EQ(sum, 0 + 2 + 4 + 6 + 8);
}

TEST(FixedSizeVector, ConstIteration) {
    int_vec5 v;
    for(int i = 0; i < 3; ++i) v.push_back(i + 1);
    const int_vec5 &cv = v;
    int product = 1;
    for(auto it = cv.cbegin(); it != cv.cbegin() + cv.size(); ++it)
        product *= *it;
    EXPECT_EQ(product, 6); // 1*2*3
}

TEST(FixedSizeVector, StringType) {
    str_vec3 v;
    v.push_back("a");
    v.push_back("b");
    v.push_back("c");
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

TEST(FixedSizeVector, PushBackAfterResize) {
    int_vec5 v;
    v.resize(3);
    v.push_back(10);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v[3], 10);
}

TEST(FixedSizeVector, EndAndCend) {
    int_vec5 v;
    for(int i = 0; i < 5; ++i) v.push_back(i);
    EXPECT_EQ(*(v.end()-1), 4);
    const int_vec5 &cv = v;
    EXPECT_EQ(*(cv.cend()-1), 4);
}

// Edge case: push_back on full vector
TEST(FixedSizeVector, PushBackFull) {
    int_vec5 v;
    for(int i = 0; i < 5; ++i) v.push_back(i);
    v.push_back(99); // Should not change vector
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v[4], 4);
}

// Edge case: operator[] out of bounds (undefined, but test for no crash)
TEST(FixedSizeVector, OutOfBoundsAccess) {
    int_vec5 v;
    v.push_back(1);
    // No bounds checking, but should not crash
    (void)v[10];
}

// Edge case: clear then push_back
TEST(FixedSizeVector, ClearThenPushBack) {
    int_vec5 v;
    for(int i = 0; i < 3; ++i) v.push_back(i);
    v.clear();
    v.push_back(7);
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v[0], 7);
}
