// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution_dispatch.cpp
///
//-----------------------------------------------------------------------------

#include "conduit_execution.hpp"
#include "conduit_execution_dispatch.hpp"
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
// Verifies that the input is a typed accessor (not a DataAccessor or DataArray)
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
// Verifies that the input is a DataArray (not a typed accessor)
template <typename T>
bool
is_direct_array(const conduit::DataArray<T> &)
{
    return false;
}

//-----------------------------------------------------------------------------
// Generates a vector of float32 values that trigger rounding errors when
// converted to float64 (for the purpose of validating DataAccessor conversions)
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
std::vector<float64>
make_float64_vals(index_t size)
{
    std::vector<float64> vals(static_cast<size_t>(size));
    for (index_t i = 0; i < size; i++)
    {
        vals[static_cast<size_t>(i)] = static_cast<float64>(0.1 * static_cast<float64>(i + 1));
    }

    return vals;
}

//-----------------------------------------------------------------------------
// Builds a float64 DataAccessor over strided data. The input buffer must already
// hold 2 * size elements. Overwrites every other element.
float64_accessor
make_strided_float64_accessor(std::vector<float64> &buf, index_t size)
{
    for (index_t i = 0; i < size; i++)
    {
        buf[static_cast<size_t>(2 * i)] = static_cast<float64>(i + 1);
    }

    const index_t elem = static_cast<index_t>(sizeof(float64));

    return float64_accessor(buf.data(), DataType::float64(size, 0, 2 * elem));
}

//-----------------------------------------------------------------------------
// Builds a float64 DataArray over strided data. The input buffer must already
// hold 2 * size elements. Overwrites every other element.
float64_array
make_strided_float64_array(std::vector<float64> &buf, index_t size)
{
    for (index_t i = 0; i < size; i++)
    {
        buf[static_cast<size_t>(2 * i)] = static_cast<float64>(i + 1);
    }

    const index_t elem = static_cast<index_t>(sizeof(float64));

    return float64_array(buf.data(), DataType::float64(size, 0, 2 * elem));
}

//-----------------------------------------------------------------------------
// Note that src and dst may be the same accessor, or different accessors. This
// kernel doubles the values in src and writes them to dst.
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
// All 3 accessors are assumed to have the same dtype for this kernel (it's
// possible to make this work with accessors of different dtypes if we find
// a good reason to do so in the future). This kernel sums the values of the 3
// accessors and writes them to dst.
template <typename Accessor>
void
run_group_sum_kernel(ExecutionPolicy &policy,
                     index_t size,
                     const Accessor x,
                     const Accessor y,
                     const Accessor z,
                     float64 *dst)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        dst[idx] = x[idx] + y[idx] + z[idx];
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
// Doubles acc in place through the single accessor dispatch, returning whether
// the dispatch upgraded it to a typed accessor.
template <typename Accessor>
bool
run_inplace_scale(ExecutionPolicy &policy, const Accessor &acc)
{
    const index_t size = acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::dispatch(acc, [&](auto vals)
    {
        is_direct = is_direct_array(vals);
        run_scale_kernel(policy, size, vals, vals);
    });

    return is_direct;
}

//-----------------------------------------------------------------------------
// Scales src into dst, reporting whether each side was upgraded to a typed
// accessor.
template <typename Src, typename Dst>
void
run_pair_scale(ExecutionPolicy &policy,
               const Src &src_acc,
               const Dst &dst_acc,
               bool &src_is_direct,
               bool &dst_is_direct)
{
    const index_t size = src_acc.number_of_elements();
    conduit::execution::dispatch(src_acc,
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
template <typename Accessor>
void
check_group_sum(ExecutionPolicy &policy,
                const Accessor &x_acc,
                const Accessor &y_acc,
                const Accessor &z_acc,
                bool expect_direct)
{
    const index_t size = x_acc.number_of_elements();

    std::vector<float64> typed_out(static_cast<size_t>(size), 0.0);
    float64 *out = typed_out.data();
    bool is_direct = false;
    conduit::execution::dispatch(x_acc,
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
TEST(conduit_execution_dispatch, single_accessor)
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

    // A compact buffer with a nonzero offset
    {
        std::vector<float64> buffer(static_cast<size_t>(size) + 1, 1.5);
        buffer[0] = -1.0;
        float64_accessor acc(buffer.data(), DataType::float64(size, sizeof(float64)));

        EXPECT_TRUE(run_inplace_scale(policy, acc));

        // The first element was not part of the DataAccessor, so it should not
        // have been modified.
        EXPECT_EQ(buffer[0], -1.0);

        for (index_t i = 1; i < size + 1; i++)
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
        float64_accessor acc = make_strided_float64_accessor(buffer, size);

        EXPECT_FALSE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i)], static_cast<float64>(2 * (i + 1)));
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i + 1)], -1.0);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, single_array)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    // A compact buffer whose dtype already matches the accessor type
    {
        std::vector<float64> buffer(static_cast<size_t>(size), 1.5);
        float64_array acc(buffer.data(), DataType::float64(size));

        EXPECT_TRUE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(i)], 3.0);
        }
    }

    // A compact buffer with a nonzero offset
    {
        std::vector<float64> buffer(static_cast<size_t>(size) + 1, 1.5);
        buffer[0] = -1.0;
        float64_array acc(buffer.data(), DataType::float64(size, sizeof(float64)));

        EXPECT_TRUE(run_inplace_scale(policy, acc));

        // The first element was not part of the DataArray, so it should not
        // have been modified.
        EXPECT_EQ(buffer[0], -1.0);

        for (index_t i = 1; i < size + 1; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(i)], 3.0);
        }
    }

    // A compact float32 buffer read through a float32_array. DataArray
    // doesn't do type conversion, so using a float64_array here will
    // essentially read garbage.
    {
        const std::vector<float32> src_vals = make_float32_roundoff_vals(size);

        Node node;
        node["typed"].set(src_vals);
        node["acc"].set(src_vals);

        float32_array typed_acc(node["typed"]);
        float32_array acc(node["acc"]);

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

    // A strided buffer keeps the DataArray, and is not upgraded to a
    // typed accessor.
    {
        std::vector<float64> buffer(static_cast<size_t>(2 * size), -1.0);
        float64_array acc = make_strided_float64_array(buffer, size);

        EXPECT_FALSE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i)], static_cast<float64>(2 * (i + 1)));
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i + 1)], -1.0);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, accessor_pair)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> src_vals = make_float32_roundoff_vals(size);

    // A float32 src and a float64 dst both get upgraded to a typed
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

    // A strided src does not get upgraded, while the compact dst
    // does get upgraded, because upgrading is per-accessor in the 2
    // accessor case.
    {
        std::vector<float64> src_buffer(static_cast<size_t>(2 * size), -1.0);
        std::vector<float64> dst_buffer(static_cast<size_t>(size), 0.0);
        float64_accessor src_acc = make_strided_float64_accessor(src_buffer, size);
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
TEST(conduit_execution_dispatch, array_pair)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> src_vals = make_float32_roundoff_vals(size);

    // A float32 src and a float64 dst both get upgraded to a typed
    // accessors even though the dtypes differ.
    {
        Node node;
        node["src"].set(src_vals);
        node["des"].set(make_execution_des_vals(size));

        float32_array src_acc(node["src"]);
        float64_array dst_acc(node["des"]);

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

    // A strided src does not get upgraded, while the compact dst
    // does get upgraded, because upgrading is per-accessor in the 2
    // accessor case.
    {
        std::vector<float64> src_buffer(static_cast<size_t>(2 * size), -1.0);
        std::vector<float64> dst_buffer(static_cast<size_t>(size), 0.0);
        float64_array src_acc = make_strided_float64_array(src_buffer, size);
        float64_array dst_acc(dst_buffer.data(), DataType::float64(size));

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
TEST(conduit_execution_dispatch, accessor_group)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> vals = make_float32_roundoff_vals(size);

    Node node;
    node["x"].set(vals);
    node["y"].set(vals);
    node["z"].set(vals);


    const float64_accessor x_acc(node["x"]);
    const float64_accessor y_acc(node["y"]);
    const float64_accessor z_acc(node["z"]);

    // The group shares one compact dtype, so it is upgraded. Swapping in
    // a member of another dtype causes the whole group to use plain
    // DataAccessors.
    check_group_sum(policy, x_acc, y_acc, z_acc, true);

    // Compact, but not the dtype the other members share, so this doesn't
    // get upgraded.
    node["odd_y"].set(std::vector<float64>(vals.begin(), vals.end()));
    const float64_accessor odd_y_acc(node["odd_y"]);
    check_group_sum(policy, x_acc, odd_y_acc, z_acc, false);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, array_group)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float64> vals = make_float64_vals(size);

    Node node;
    node["x"].set(vals);
    node["y"].set(vals);
    node["z"].set(vals);
    
    const float64_array x_acc(node["x"]);
    const float64_array y_acc(node["y"]);
    const float64_array z_acc(node["z"]);

    // The group shares one compact dtype, so it is upgraded. Swapping in
    // a member of another dtype causes the whole group to use plain
    // DataAccessors.
    check_group_sum(policy, x_acc, float64_array(node["y"]), z_acc, true);

    // Compact, but not the dtype the other members share (strided), so this
    // doesn't get upgraded.
    std::vector<float64> buffer(static_cast<size_t>(2 * size), -1.0);
    const float64_array odd_y_acc = make_strided_float64_array(buffer, size);
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

        // Demonstrates that the dispatch can mix and match DataAccessor
        // and DataArray, and that the dispatch upgrades both to typed accessors
        // when possible.
        float32_array src_acc(node["src"]);
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
TEST(conduit_execution_dispatch, policies)
{
    // Because nvcc doesn't support extended generic lambdas
    run_test_policies();
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
