// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_geometry_vector.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_geometry_vector.hpp"

#include <iostream>
#include <limits>
#include "gtest/gtest.h"

using vector2d = conduit::geometry::vector<double, 2>;
using vector3d = conduit::geometry::vector<double, 3>;

// 2D Tests

TEST(Vector2D, ConstructionAndAccess) {
    vector2d v{};
    v[0] = 1.5;
    v[1] = -2.5;
    EXPECT_DOUBLE_EQ(v[0], 1.5);
    EXPECT_DOUBLE_EQ(v[1], -2.5);

    v.x = 3.0;
    v.y = 4.0;
    EXPECT_DOUBLE_EQ(v[0], 3.0);
    EXPECT_DOUBLE_EQ(v[1], 4.0);
}

TEST(Vector2D, ZeroAndSetAll) {
    vector2d v;
    v.set_all(7.0);
    EXPECT_DOUBLE_EQ(v[0], 7.0);
    EXPECT_DOUBLE_EQ(v[1], 7.0);

    v.zero();
    EXPECT_DOUBLE_EQ(v[0], 0.0);
    EXPECT_DOUBLE_EQ(v[1], 0.0);
}

TEST(Vector2D, ArithmeticOperators) {
    vector2d a; a[0] = 1.0; a[1] = 2.0;
    vector2d b; b[0] = 3.0; b[1] = 4.0;

    auto sum = a + b;
    EXPECT_DOUBLE_EQ(sum[0], 4.0);
    EXPECT_DOUBLE_EQ(sum[1], 6.0);

    auto diff = a - b;
    EXPECT_DOUBLE_EQ(diff[0], -2.0);
    EXPECT_DOUBLE_EQ(diff[1], -2.0);

    auto neg = -a;
    EXPECT_DOUBLE_EQ(neg[0], -1.0);
    EXPECT_DOUBLE_EQ(neg[1], -2.0);

    auto scalar_add = a + 5.0;
    EXPECT_DOUBLE_EQ(scalar_add[0], 6.0);
    EXPECT_DOUBLE_EQ(scalar_add[1], 7.0);

    auto scalar_sub = a - 1.0;
    EXPECT_DOUBLE_EQ(scalar_sub[0], 0.0);
    EXPECT_DOUBLE_EQ(scalar_sub[1], 1.0);

    auto scalar_mul = a * 2.0;
    EXPECT_DOUBLE_EQ(scalar_mul[0], 2.0);
    EXPECT_DOUBLE_EQ(scalar_mul[1], 4.0);

    auto scalar_div = b / 2.0;
    EXPECT_DOUBLE_EQ(scalar_div[0], 1.5);
    EXPECT_DOUBLE_EQ(scalar_div[1], 2.0);
}

TEST(Vector2D, CompoundAssignment) {
    vector2d v; v[0] = 1.0; v[1] = 2.0;
    vector2d w; w[0] = 3.0; w[1] = 4.0;

    v += w;
    EXPECT_DOUBLE_EQ(v[0], 4.0);
    EXPECT_DOUBLE_EQ(v[1], 6.0);

    v -= w;
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);

    v *= 2.0;
    EXPECT_DOUBLE_EQ(v[0], 2.0);
    EXPECT_DOUBLE_EQ(v[1], 4.0);

    v /= 2.0;
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
}

TEST(Vector2D, ComparisonOperators) {
    vector2d a; a[0] = 1.0; a[1] = 2.0;
    vector2d b; b[0] = 1.5; b[1] = 2.0;
    vector2d c; c[0] = 1.0; c[1] = 2.0;

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a >= c);
}

TEST(Vector2D, DotNormDistance) {
    vector2d a; a[0] = 3.0; a[1] = 4.0;
    vector2d b; b[0] = 0.0; b[1] = 0.0;

    EXPECT_DOUBLE_EQ(a.dot(a), 25.0);
    EXPECT_DOUBLE_EQ(a.norm(), 5.0);

    EXPECT_DOUBLE_EQ(a.distance2(b), 25.0);
    EXPECT_DOUBLE_EQ(a.distance(b), 5.0);
}

TEST(Vector2D, Normalize) {
    vector2d a; a[0] = 3.0; a[1] = 4.0;
    a.normalize();
    EXPECT_NEAR(a[0], 0.6, 1e-10);
    EXPECT_NEAR(a[1], 0.8, 1e-10);
    EXPECT_NEAR(a.norm(), 1.0, 1e-10);
}

TEST(Vector2D, CrossProduct) {
    vector2d a; a[0] = 1.0; a[1] = 2.0;
    vector2d b; b[0] = 3.0; b[1] = 4.0;
    double cross = a.cross(b);
    EXPECT_DOUBLE_EQ(cross, -2.0);
}

// 3D Tests

TEST(Vector3D, ConstructionAndAccess) {
    vector3d v{};
    v[0] = 1.0; v[1] = 2.0; v[2] = 3.0;
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
    EXPECT_DOUBLE_EQ(v[2], 3.0);

    v.x = 4.0;
    v.y = 5.0;
    v.z = 6.0;
    EXPECT_DOUBLE_EQ(v[0], 4.0);
    EXPECT_DOUBLE_EQ(v[1], 5.0);
    EXPECT_DOUBLE_EQ(v[2], 6.0);
}

TEST(Vector3D, ZeroAndSetAll) {
    vector3d v;
    v.set_all(-3.0);
    EXPECT_DOUBLE_EQ(v[0], -3.0);
    EXPECT_DOUBLE_EQ(v[1], -3.0);
    EXPECT_DOUBLE_EQ(v[2], -3.0);

    v.zero();
    EXPECT_DOUBLE_EQ(v[0], 0.0);
    EXPECT_DOUBLE_EQ(v[1], 0.0);
    EXPECT_DOUBLE_EQ(v[2], 0.0);
}

TEST(Vector3D, ArithmeticOperators) {
    vector3d a; a[0] = 1.0; a[1] = 2.0; a[2] = 3.0;
    vector3d b; b[0] = 4.0; b[1] = 5.0; b[2] = 6.0;

    auto sum = a + b;
    EXPECT_DOUBLE_EQ(sum[0], 5.0);
    EXPECT_DOUBLE_EQ(sum[1], 7.0);
    EXPECT_DOUBLE_EQ(sum[2], 9.0);

    auto diff = a - b;
    EXPECT_DOUBLE_EQ(diff[0], -3.0);
    EXPECT_DOUBLE_EQ(diff[1], -3.0);
    EXPECT_DOUBLE_EQ(diff[2], -3.0);

    auto neg = -a;
    EXPECT_DOUBLE_EQ(neg[0], -1.0);
    EXPECT_DOUBLE_EQ(neg[1], -2.0);
    EXPECT_DOUBLE_EQ(neg[2], -3.0);

    auto scalar_add = a + 1.5;
    EXPECT_DOUBLE_EQ(scalar_add[0], 2.5);
    EXPECT_DOUBLE_EQ(scalar_add[1], 3.5);
    EXPECT_DOUBLE_EQ(scalar_add[2], 4.5);

    auto scalar_sub = a - 0.5;
    EXPECT_DOUBLE_EQ(scalar_sub[0], 0.5);
    EXPECT_DOUBLE_EQ(scalar_sub[1], 1.5);
    EXPECT_DOUBLE_EQ(scalar_sub[2], 2.5);

    auto scalar_mul = a * 2.0;
    EXPECT_DOUBLE_EQ(scalar_mul[0], 2.0);
    EXPECT_DOUBLE_EQ(scalar_mul[1], 4.0);
    EXPECT_DOUBLE_EQ(scalar_mul[2], 6.0);

    auto scalar_div = b / 2.0;
    EXPECT_DOUBLE_EQ(scalar_div[0], 2.0);
    EXPECT_DOUBLE_EQ(scalar_div[1], 2.5);
    EXPECT_DOUBLE_EQ(scalar_div[2], 3.0);
}

TEST(Vector3D, CompoundAssignment) {
    vector3d v; v[0] = 1.0; v[1] = 2.0; v[2] = 3.0;
    vector3d w; w[0] = 4.0; w[1] = 5.0; w[2] = 6.0;

    v += w;
    EXPECT_DOUBLE_EQ(v[0], 5.0);
    EXPECT_DOUBLE_EQ(v[1], 7.0);
    EXPECT_DOUBLE_EQ(v[2], 9.0);

    v -= w;
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
    EXPECT_DOUBLE_EQ(v[2], 3.0);

    v *= 2.0;
    EXPECT_DOUBLE_EQ(v[0], 2.0);
    EXPECT_DOUBLE_EQ(v[1], 4.0);
    EXPECT_DOUBLE_EQ(v[2], 6.0);

    v /= 2.0;
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
    EXPECT_DOUBLE_EQ(v[2], 3.0);
}

TEST(Vector3D, ComparisonOperators) {
    vector3d a; a[0] = 1.0; a[1] = 2.0; a[2] = 3.0;
    vector3d b; b[0] = 2.0; b[1] = 2.0; b[2] = 4.0;
    vector3d c; c[0] = 1.0; c[1] = 2.0; c[2] = 3.0;

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a >= c);
}

TEST(Vector3D, DotNormDistance) {
    vector3d a; a[0] = 1.0; a[1] = 2.0; a[2] = 2.0;
    vector3d b; b[0] = 0.0; b[1] = 0.0; b[2] = 0.0;

    EXPECT_DOUBLE_EQ(a.dot(a), 9.0);
    EXPECT_DOUBLE_EQ(a.norm(), 3.0);

    EXPECT_DOUBLE_EQ(a.distance2(b), 9.0);
    EXPECT_DOUBLE_EQ(a.distance(b), 3.0);
}

TEST(Vector3D, Normalize) {
    vector3d a; a[0] = 0.0; a[1] = 3.0; a[2] = 4.0;
    a.normalize();
    EXPECT_NEAR(a[0], 0.0, 1e-10);
    EXPECT_NEAR(a[1], 0.6, 1e-10);
    EXPECT_NEAR(a[2], 0.8, 1e-10);
    EXPECT_NEAR(a.norm(), 1.0, 1e-10);
}

TEST(Vector3D, CrossProduct) {
    vector3d a; a[0] = 1.0; a[1] = 2.0; a[2] = 3.0;
    vector3d b; b[0] = 4.0; b[1] = 5.0; b[2] = 6.0;
    vector3d cross = a.cross(b);
    EXPECT_DOUBLE_EQ(cross[0], -3.0);
    EXPECT_DOUBLE_EQ(cross[1], 6.0);
    EXPECT_DOUBLE_EQ(cross[2], -3.0);
}

// Edge Cases

TEST(Vector2D, NormalizeZeroVector) {
    vector2d v; v[0] = 0.0; v[1] = 0.0;
    v.normalize();
    EXPECT_DOUBLE_EQ(v[0], 0.0);
    EXPECT_DOUBLE_EQ(v[1], 0.0);
}

TEST(Vector3D, NormalizeZeroVector) {
    vector3d v; v[0] = 0.0; v[1] = 0.0; v[2] = 0.0;
    v.normalize();
    EXPECT_DOUBLE_EQ(v[0], 0.0);
    EXPECT_DOUBLE_EQ(v[1], 0.0);
    EXPECT_DOUBLE_EQ(v[2], 0.0);
}

// Other features

TEST(Vector2D, Constructor) {
    vector2d a;
    EXPECT_DOUBLE_EQ(a.x, 0.);
    EXPECT_DOUBLE_EQ(a.y, 0.);

    vector2d v(1., 2.);
    EXPECT_DOUBLE_EQ(v.x, 1.);
    EXPECT_DOUBLE_EQ(v.y, 2.);
}

TEST(Vector3D, Constructor) {
    vector3d a;
    EXPECT_DOUBLE_EQ(a.x, 0.);
    EXPECT_DOUBLE_EQ(a.y, 0.);
    EXPECT_DOUBLE_EQ(a.z, 0.);

    vector3d v(1., 2., 3.);
    EXPECT_DOUBLE_EQ(v.x, 1.);
    EXPECT_DOUBLE_EQ(v.y, 2.);
    EXPECT_DOUBLE_EQ(v.z, 3.);
}

TEST(Vector3D, Accessor) {
    vector3d v(3., 6., 9.);

    EXPECT_DOUBLE_EQ(v.x, 3.);
    EXPECT_DOUBLE_EQ(v.y, 6.);
    EXPECT_DOUBLE_EQ(v.z, 9.);

    v.x = 1.;
    v.y = 2.;
    v.z = 3.;
    EXPECT_DOUBLE_EQ(v.x, 1.);
    EXPECT_DOUBLE_EQ(v.y, 2.);
    EXPECT_DOUBLE_EQ(v.z, 3.);

    v.x += 1.;
    v.y += 1.;
    v.z += 1.;
    EXPECT_DOUBLE_EQ(v.x, 2.);
    EXPECT_DOUBLE_EQ(v.y, 3.);
    EXPECT_DOUBLE_EQ(v.z, 4.);

    v.x -= 1.;
    v.y -= 1.;
    v.z -= 1.;
    EXPECT_DOUBLE_EQ(v.x, 1.);
    EXPECT_DOUBLE_EQ(v.y, 2.);
    EXPECT_DOUBLE_EQ(v.z, 3.);

    v.x *= 2.;
    v.y *= 2.;
    v.z *= 2.;
    EXPECT_DOUBLE_EQ(v.x, 2.);
    EXPECT_DOUBLE_EQ(v.y, 4.);
    EXPECT_DOUBLE_EQ(v.z, 6.);

    v.x /= 2.;
    v.y /= 2.;
    v.z /= 2.;
    EXPECT_DOUBLE_EQ(v.x, 1.);
    EXPECT_DOUBLE_EQ(v.y, 2.);
    EXPECT_DOUBLE_EQ(v.z, 3.);
}
