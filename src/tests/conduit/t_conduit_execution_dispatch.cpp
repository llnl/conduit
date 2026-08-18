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

const index_t EXECUTION_TEST_ARRAY_SIZE = 4;

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
        const std::vector<float32> src_vals = make_roundoff_vals<float32>(size);

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
        float64_accessor acc = make_strided_float64<float64_accessor>(buffer, size);

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
        const std::vector<float32> src_vals = make_roundoff_vals<float32>(size);

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
        float64_array acc = make_strided_float64<float64_array>(buffer, size);

        EXPECT_FALSE(run_inplace_scale(policy, acc));

        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i)], static_cast<float64>(2 * (i + 1)));
            EXPECT_EQ(buffer[static_cast<size_t>(2 * i + 1)], -1.0);
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, all_dtypes)
{
    conduit_device_prepare();

    // signed integer types
    check_single_dtype_dispatch<int8>({-3, 5, -60, 12},
                                      DataType::INT8_ID);
    check_single_dtype_dispatch<int16>({-300, 400, -1000, 5000},
                                       DataType::INT16_ID);
    check_single_dtype_dispatch<int32>({-300, 70000, -1000, 5000},
                                       DataType::INT32_ID);
    check_single_dtype_dispatch<int64>({-300, (int64(1) << 40), -1000, 5000},
                                       DataType::INT64_ID);

    // unsigned integer types
    check_single_dtype_dispatch<uint8>({3, 5, 60, 100},
                                       DataType::UINT8_ID);
    check_single_dtype_dispatch<uint16>({300, 400, 1000, 5000},
                                        DataType::UINT16_ID);
    check_single_dtype_dispatch<uint32>({300, 70000, 1000, 5000},
                                        DataType::UINT32_ID);
    check_single_dtype_dispatch<uint64>({300, (uint64(1) << 40), 1000, 5000},
                                        DataType::UINT64_ID);

    // floating point types
    check_single_dtype_dispatch<float32>({-1.5f, 2.25f, -300.5f, 1024.0f},
                                         DataType::FLOAT32_ID);
    check_single_dtype_dispatch<float64>({-1.5, 2.25, -300.5, 1024.0},
                                         DataType::FLOAT64_ID);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, accessor_pair)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    const std::vector<float32> src_vals = make_roundoff_vals<float32>(size);

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
        run_pair_scale(policy,
                       src_acc,
                       dst_acc,
                       src_is_direct,
                       dst_is_direct);

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
        float64_accessor src_acc = make_strided_float64<float64_accessor>(src_buffer, size);
        float64_accessor dst_acc(dst_buffer.data(), DataType::float64(size));

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy,
                       src_acc,
                       dst_acc,
                       src_is_direct,
                       dst_is_direct);

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

    const std::vector<float32> src_vals = make_roundoff_vals<float32>(size);

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
        run_pair_scale(policy,
                       src_acc,
                       dst_acc,
                       src_is_direct,
                       dst_is_direct);

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
        float64_array src_acc = make_strided_float64<float64_array>(src_buffer, size);
        float64_array dst_acc(dst_buffer.data(), DataType::float64(size));

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy,
                       src_acc,
                       dst_acc,
                       src_is_direct,
                       dst_is_direct);

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

    const std::vector<float32> vals = make_roundoff_vals<float32>(size);

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

    const std::vector<float64> vals = make_roundoff_vals<float64>(size);

    Node node;
    node["x"].set(vals);
    node["y"].set(vals);
    node["z"].set(vals);

    const float64_array x_acc(node["x"]);
    const float64_array y_acc(node["y"]);
    const float64_array z_acc(node["z"]);

    // The group shares one compact dtype, so it is upgraded
    check_group_sum(policy, x_acc, y_acc, z_acc, true);

    // Swapping in a strided member causes the whole group to use plain DataArrays
    std::vector<float64> buffer(static_cast<size_t>(2 * size), -1.0);
    const float64_array odd_y_acc = make_strided_float64<float64_array>(buffer, size);
    check_group_sum(policy, x_acc, odd_y_acc, z_acc, false);
}

//-----------------------------------------------------------------------------
// Runs a single accessor dispatch under every enabled execution policy
void
run_test_single_policies()
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    for_each_enabled_policy([&](ExecutionPolicy policy)
    {
        Node node;
        node["vals"].set(std::vector<float64>(static_cast<size_t>(size), 1.5));

        float64_accessor acc(node["vals"]);

        acc.use_with(policy);

        EXPECT_TRUE(run_inplace_scale(policy, acc));

        acc.sync();

        float64_array res(node["vals"]);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(res[i], 3.0);
        }
    });
}

//-----------------------------------------------------------------------------
// Runs a pair dispatch under every enabled execution policy
void
run_test_pair_policies()
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    for_each_enabled_policy([&](ExecutionPolicy policy)
    {
        Node node;
        node["src"].set(std::vector<float32>(static_cast<size_t>(size), 1.0f));
        node["des"].set(std::vector<int32>(static_cast<size_t>(size), 0));

        float32_array src_acc(node["src"]);
        float64_accessor dst_acc(node["des"]);

        src_acc.use_with(policy);
        dst_acc.use_with(policy);

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_pair_scale(policy,
                       src_acc, dst_acc, src_is_direct, dst_is_direct);

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
// Runs a 3 accessor group dispatch under every enabled execution policy
void
run_test_group_policies()
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    for_each_enabled_policy([&](ExecutionPolicy policy)
    {
        const std::vector<float64> vals(static_cast<size_t>(size), 1.5);

        Node node;
        node["x"].set(vals);
        node["y"].set(vals);
        node["z"].set(vals);
        node["des"].set(make_execution_des_vals(size));

        float64_accessor x_acc(node["x"]);
        float64_accessor y_acc(node["y"]);
        float64_accessor z_acc(node["z"]);
        float64_accessor dst_acc(node["des"]);

        x_acc.use_with(policy);
        y_acc.use_with(policy);
        z_acc.use_with(policy);
        dst_acc.use_with(policy);

        EXPECT_TRUE(run_group_sum(policy,
                                  x_acc,
                                  y_acc,
                                  z_acc,
                                  dst_acc));

        dst_acc.sync();

        float64_array res(node["des"]);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(res[i], 4.5);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_dispatch, policies)
{
    // Because nvcc doesn't support extended generic lambdas
    run_test_single_policies();
    run_test_pair_policies();
    run_test_group_policies();
}
