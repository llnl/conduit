// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution_typed_accessor.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_execution.hpp"
#include "conduit_execution_typed_accessor.hpp"
#include "execution_test_utils.hpp"

#include <cstdlib>
#include <vector>
#include "gtest/gtest.h"

using namespace conduit;
using conduit::execution::ExecutionPolicy;

index_t EXECUTION_TEST_ARRAY_SIZE = 4;

//
// Test helpers
//

//-----------------------------------------------------------------------------
// Verifies that the input is a typed accessor (not a DataAccessor)
template <typename TypedAccessor>
bool
is_direct_array(const TypedAccessor &)
{
    return true;
}

//-----------------------------------------------------------------------------
// Verifies that the input is a DataAccessor (not a typed accessor)
template <typename T>
bool
is_direct_array(const conduit::DataAccessor<T> &)
{
    return false;
}

//-----------------------------------------------------------------------------
// Generates a vector of float32 values that trigger roundoff errors when
// converted to float64 (for the purpose of validating accessor results)
std::vector<float32>
make_float32_roundoff_vals(index_t size)
{
    std::vector<float32> vals(static_cast<size_t>(size));
    for (index_t i = 0; i < size; i++)
    {
        vals[static_cast<size_t>(i)] = static_cast<float32>(0.1 * static_cast<float64>(i + 1));
    }

    return vals;
}

//-----------------------------------------------------------------------------
// Builds a float64 accessor over strided data. The buffer must already hold
// 2 * size elements; every other element is overwritten with 1..size and the
// rest keep their fill value.
float64_accessor
make_strided_float64(std::vector<float64> &buf, index_t size)
{
    for (index_t i = 0; i < size; i++)
    {
        buf[static_cast<size_t>(2 * i)] = static_cast<float64>(i + 1);
    }

    const index_t elem = static_cast<index_t>(sizeof(float64));

    return float64_accessor(buf.data(), DataType::float64(size, 0, 2 * elem));
}

//-----------------------------------------------------------------------------
// Note that src and dst may be the same accessor, which covers the in-place
// case.
template <typename Src, typename Dst>
void
run_scale_kernel(ExecutionPolicy &policy,
                 index_t size,
                 const Src src,
                 const Dst dst)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        dst.set(idx, 2.0 * src[idx]);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
// All 3 accessors are assumed to have the same dtype
template <typename Accessor>
void
run_group_sum_kernel(ExecutionPolicy &policy,
                     index_t size,
                     const Accessor x,
                     const Accessor y,
                     const Accessor z,
                     float64 *out)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        out[idx] = x[idx] + y[idx] + z[idx];
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
// Doubles acc in place through the single accessor dispatch, returning whether
// the dispatch upgraded it to a typed accessor.
bool
run_inplace_scale(ExecutionPolicy &policy, const float64_accessor &acc)
{
    const index_t size = acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::with_typed_accessor(acc, [&](auto vals)
    {
        is_direct = is_direct_array(vals);
        run_scale_kernel(policy, size, vals, vals);
    });

    return is_direct;
}

//-----------------------------------------------------------------------------
// Scales src into dst, reporting whether each side was upgraded to a typed
// accessor.
void
run_pair_scale(ExecutionPolicy &policy,
               const float64_accessor &src_acc,
               const float64_accessor &dst_acc,
               bool &src_is_direct,
               bool &dst_is_direct)
{
    const index_t size = src_acc.number_of_elements();
    conduit::execution::with_typed_accessor(src_acc,
                                            dst_acc,
                                            [&](auto src, auto dst)
    {
        src_is_direct = is_direct_array(src);
        dst_is_direct = is_direct_array(dst);
        run_scale_kernel(policy, size, src, dst);
    });
}

//-----------------------------------------------------------------------------
// Sums the values of three accessors, checking that the group was or was not
// upgraded as expected and that the same kernel run over plain DataAccessors
// gives the same values.
void
check_group_sum(ExecutionPolicy &policy,
                const float64_accessor &x_acc,
                const float64_accessor &y_acc,
                const float64_accessor &z_acc,
                bool expect_direct)
{
    const index_t size = x_acc.number_of_elements();

    std::vector<float64> typed_out(static_cast<size_t>(size), 0.0);
    float64 *out = typed_out.data();
    bool is_direct = false;
    conduit::execution::with_typed_accessor(x_acc,
                                            y_acc,
                                            z_acc,
                                            [&](auto x, auto y, auto z)
    {
        is_direct = is_direct_array(x);
        run_group_sum_kernel(policy, size, x, y, z, out);
    });
    EXPECT_EQ(is_direct, expect_direct);

    std::vector<float64> acc_out(static_cast<size_t>(size), 0.0);
    run_group_sum_kernel(policy, size, x_acc, y_acc, z_acc, acc_out.data());

    for (index_t i = 0; i < size; i++)
    {
        const size_t idx = static_cast<size_t>(i);
        EXPECT_EQ(typed_out[idx], acc_out[idx]);
        EXPECT_EQ(typed_out[idx], x_acc[i] + y_acc[i] + z_acc[i]);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_typed_accessor, single_accessor)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    // A compact buffer whose dtype already matches the accessor type
    {
        std::vector<float64> buffer(static_cast<size_t>(size), 1.5);
        float64_accessor acc(buffer.data(), DataType::float64(size));

        EXPECT_TRUE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(i)], 3.0);
        }
    }

    // A compact float32 buffer read through a float64_accessor should still
    // get upgraded. The same kernel run over a DataAccessor has to produce
    // the same values.
    {
        const std::vector<float32> src_vals = make_float32_roundoff_vals(size);

        Node node;
        node["typed"].set(src_vals);
        node["acc"].set(src_vals);

        float64_accessor typed_acc(node["typed"]);
        float64_accessor acc(node["acc"]);

        EXPECT_TRUE(run_inplace_scale(policy, typed_acc));
        run_scale_kernel(policy, size, acc, acc);

        float32_array typed_res(node["typed"]);
        float32_array acc_res(node["acc"]);
        for (index_t i = 0; i < size; i++)
        {
            const float64 src_val = src_vals[static_cast<size_t>(i)];
            EXPECT_EQ(typed_res[i], acc_res[i]);
            EXPECT_EQ(typed_res[i], static_cast<float32>(2.0 * src_val));
        }
    }

    // A strided buffer keeps the DataAccessor, and is not upgraded to a
    // typed accessor.
    {
        std::vector<float64> buffer(static_cast<size_t>(2 * size), -1.0);
        float64_accessor acc = make_strided_float64(buffer, size);

        EXPECT_FALSE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i)], static_cast<float64>(2 * (i + 1)));
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i + 1)], -1.0);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_typed_accessor, accessor_pair)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> src_vals = make_float32_roundoff_vals(size);

    // A float32 source and a float64 destination both get upgraded to a typed
    // accessor even though the dtypes differ.
    {
        Node node;
        node["src"].set(src_vals);
        node["des"].set(make_execution_des_vals(size));

        float64_accessor src_acc(node["src"]);
        float64_accessor dst_acc(node["des"]);

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy, src_acc, dst_acc, src_is_direct, dst_is_direct);

        EXPECT_TRUE(src_is_direct);
        EXPECT_TRUE(dst_is_direct);

        float64_array res(node["des"]);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(res[i], 2.0 * src_vals[static_cast<size_t>(i)]);
        }
    }

    // A strided source does not get upgraded, while the compact destination
    // does get upgraded.
    {
        std::vector<float64> src_buffer(static_cast<size_t>(2 * size), -1.0);
        std::vector<float64> dst_buffer(static_cast<size_t>(size), 0.0);
        float64_accessor src_acc = make_strided_float64(src_buffer, size);
        float64_accessor dst_acc(dst_buffer.data(), DataType::float64(size));

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy, src_acc, dst_acc, src_is_direct, dst_is_direct);

        EXPECT_FALSE(src_is_direct);
        EXPECT_TRUE(dst_is_direct);

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(dst_buffer[static_cast<size_t>(i)], static_cast<float64>(2 * (i + 1)));
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_typed_accessor, accessor_group)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> vals = make_float32_roundoff_vals(size);

    Node node;
    node["x"].set(vals);
    node["y"].set(vals);
    node["z"].set(vals);
    // Compact, but not the dtype the other members share
    node["odd_y"].set(std::vector<float64>(vals.begin(), vals.end()));

    const float64_accessor x_acc(node["x"]);
    const float64_accessor z_acc(node["z"]);

    // The group shares one compact dtype, so it is upgraded. Swapping in
    // a member of another dtype causes the whole group to use plain
    // DataAccessors.
    check_group_sum(policy, x_acc, float64_accessor(node["y"]), z_acc, true);
    const float64_accessor odd_y_acc(node["odd_y"]);
    check_group_sum(policy, x_acc, odd_y_acc, z_acc, false);
}

//-----------------------------------------------------------------------------
// Runs a converting pair dispatch under every enabled execution policy
void
run_test_policies()
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    for_each_enabled_policy([&](ExecutionPolicy policy)
    {
        // A float32 source and an int32 destination
        Node node;
        node["src"].set(std::vector<float32>(static_cast<size_t>(size), 1.0f));
        node["des"].set(std::vector<int32>(static_cast<size_t>(size), 0));

        float64_accessor src_acc(node["src"]);
        float64_accessor dst_acc(node["des"]);

        src_acc.use_with(policy);
        dst_acc.use_with(policy);

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy, src_acc, dst_acc, src_is_direct, dst_is_direct);

        EXPECT_TRUE(src_is_direct);
        EXPECT_TRUE(dst_is_direct);

        dst_acc.sync();

        int32_array res(node["des"]);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(res[i], 2);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_typed_accessor, policies)
{
    // Because nvcc is a bad compiler and won't allow extended generic lambdas
    run_test_policies();
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
