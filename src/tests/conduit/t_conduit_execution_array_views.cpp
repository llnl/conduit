// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution_array_views.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_execution.hpp"
#include "conduit_execution_array_views.hpp"
#include "conduit_memory_manager.hpp"
#include "execution_test_utils.hpp"

#include <cstdlib>
#include <string>
#include <vector>
#include "gtest/gtest.h"

using namespace conduit;
using conduit::execution::ExecutionPolicy;

index_t EXECUTION_TEST_ARRAY_SIZE = 4;

//
// Test helpers
//

//-----------------------------------------------------------------------------
// Verifies that the given value type is a direct array (not a DataAccessor)
template <typename Vals>
bool
is_direct_array(const Vals &)
{
    return true;
}

//-----------------------------------------------------------------------------
template <typename T>
bool
is_direct_array(const conduit::DataAccessor<T> &)
{
    return false;
}

//-----------------------------------------------------------------------------
template <typename T>
std::vector<T>
make_dispatch_src_vals(index_t array_size)
{
    std::vector<T> vals(static_cast<size_t>(array_size));
    for (index_t i = 0; i < array_size; i++)
    {
        vals[static_cast<size_t>(i)] = static_cast<T>(i + 1);
    }

    return vals;
}

//-----------------------------------------------------------------------------
// Creates a strided float64 accessor with the given parameters, and fills
// the buffer with -1.0 values.
float64_accessor
make_strided_float64(std::vector<float64> &buf,
                     index_t size,
                     index_t step,
                     index_t skip,
                     bool fill_vals)
{
    buf.assign(static_cast<size_t>(skip + step * size), -1.0);
    if (fill_vals)
    {
        for (index_t i = 0; i < size; i++)
        {
            buf[static_cast<size_t>(skip + step * i)] = static_cast<float64>(i + 1);
        }
    }

    const index_t elem = static_cast<index_t>(sizeof(float64));

    return float64_accessor(buf.data(), DataType::float64(size, skip * elem, step * elem));
}

//-----------------------------------------------------------------------------
// Values that are not exactly representable as float32
std::vector<float32>
make_fractional_vals(index_t size)
{
    std::vector<float32> vals(static_cast<size_t>(size));
    for (index_t i = 0; i < size; i++)
    {
        vals[static_cast<size_t>(i)] = static_cast<float32>(0.1 * static_cast<float64>(i + 1));
    }

    return vals;
}

//-----------------------------------------------------------------------------
void
expect_compact_ptr(const float64_accessor &acc,
                   const std::vector<float64> &buf,
                   index_t step,
                   index_t skip)
{
    if (step == 1)
    {
        EXPECT_EQ(acc.compact_ptr(), buf.data() + skip);
    }
    else // if (step != 1)
    {
        EXPECT_TRUE(acc.compact_ptr() == nullptr);
    }
}

//-----------------------------------------------------------------------------
// Reads node[path] back through a host accessor and checks it holds 2*(i+1)
template <typename T>
void
expect_scaled(Node &node,
              const std::string &path,
              index_t size)
{
    DataAccessor<T> res_acc(node[path]);
    res_acc.use_with(ExecutionPolicy::host());

    EXPECT_EQ(res_acc.number_of_elements(), size);
    for (index_t i = 0; i < size; i++)
    {
        EXPECT_EQ(res_acc[i], static_cast<T>(2 * (i + 1)));
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void
expect_scaled_strided(const std::vector<T> &buf,
                      index_t size,
                      index_t step,
                      index_t skip)
{
    for (index_t i = 0; i < skip; i++)
    {
        EXPECT_EQ(buf[static_cast<size_t>(i)], static_cast<T>(-1));
    }

    for (index_t i = 0; i < size; i++)
    {
        EXPECT_EQ(buf[static_cast<size_t>(skip + step * i)], static_cast<T>(2 * (i + 1)));

        for (index_t j = 1; j < step; j++)
        {
            EXPECT_EQ(buf[static_cast<size_t>(skip + step * i + j)], static_cast<T>(-1));
        }
    }
}

//-----------------------------------------------------------------------------
// Note that src and dst may be the same read-write view, which covers the
// in-place case
template <typename SrcVals, typename DstVals>
void
run_dispatch_scale_kernel(ExecutionPolicy &policy,
                          index_t size,
                          const SrcVals src,
                          const DstVals dst)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        dst.set(idx, 2.0 * src[idx]);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename Vals>
void
run_dispatch_int_fill_kernel(ExecutionPolicy &policy,
                             index_t size,
                             const Vals vals)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        vals.set(idx, static_cast<conduit::int64>(2 * (idx + 1)));
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename Vals, typename T>
void
run_dispatch_read_copy_kernel(ExecutionPolicy &policy,
                              index_t size,
                              const Vals vals,
                              T *out)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        out[idx] = static_cast<T>(2 * vals[idx]);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
// A single Vals parameter serves all three members, since the grouped
// dispatch hands the kernel one shared view type
template <typename Vals>
void
run_dispatch_group_sum_kernel(ExecutionPolicy &policy,
                              index_t size,
                              const Vals x,
                              const Vals y,
                              const Vals z,
                              conduit::float64 *out)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        out[idx] = x[idx] + y[idx] + z[idx];
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
void
run_dispatch_scale(ExecutionPolicy &policy,
                   const float64_accessor &src_acc,
                   const float64_accessor &dst_acc,
                   bool &src_is_direct,
                   bool &dst_is_direct)
{
    const index_t size = src_acc.number_of_elements();
    conduit::execution::with_values(src_acc,
                                    dst_acc,
                                    [&](auto src, auto dst)
    {
        src_is_direct = is_direct_array(src);
        dst_is_direct = is_direct_array(dst);
        run_dispatch_scale_kernel(policy, size, src, dst);
    });
}

//-----------------------------------------------------------------------------
template <typename T>
bool
run_dispatch_int_fill(ExecutionPolicy &policy,
                      const DataAccessor<T> &acc)
{
    const index_t size = acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::with_write_values(acc, [&](auto vals)
    {
        is_direct = is_direct_array(vals);
        run_dispatch_int_fill_kernel(policy, size, vals);
    });

    return is_direct;
}

//-----------------------------------------------------------------------------
bool
run_dispatch_inplace(ExecutionPolicy &policy,
                     const float64_accessor &acc)
{
    const index_t size = acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::with_read_write_values(acc, [&](auto vals)
    {
        is_direct = is_direct_array(vals);
        run_dispatch_scale_kernel(policy, size, vals, vals);
    });

    return is_direct;
}

//-----------------------------------------------------------------------------
bool
run_dispatch_group_sum(ExecutionPolicy &policy,
                       const float64_accessor &x_acc,
                       const float64_accessor &y_acc,
                       const float64_accessor &z_acc,
                       float64 *out)
{
    const index_t size = x_acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::with_read_values(x_acc,
                                         y_acc,
                                         z_acc,
                                         [&](auto x, auto y, auto z)
    {
        is_direct = is_direct_array(x);
        run_dispatch_group_sum_kernel(policy, size, x, y, z, out);
    });

    return is_direct;
}

//-----------------------------------------------------------------------------
template <typename T>
bool
run_dispatch_read_copy(ExecutionPolicy &policy,
                       const DataAccessor<T> &acc,
                       T *out)
{
    const index_t size = acc.number_of_elements();
    bool is_direct = false;
    conduit::execution::with_read_values(acc, [&](auto vals)
    {
        is_direct = is_direct_array(vals);
        run_dispatch_read_copy_kernel(policy, size, vals, out);
    });

    return is_direct;
}

//
// Tests
//

//-----------------------------------------------------------------------------
// Checks which layouts reach the typed views and which fall back to the
// accessors when dispatching with with_values()
TEST(conduit_execution_array_views, with_values)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    // Test all direct/fallback combinations of a nested src + dst dispatch,
    // with and without a leading offset
    for (int c = 0; c < 8; c++)
    {
        const index_t skip = (c & 4) ? 3 : 0;
        const index_t src_step = (c & 2) ? 2 : 1;
        const index_t dst_step = (c & 1) ? 2 : 1;

        std::vector<float64> src_buffer;
        std::vector<float64> dst_buffer;
        float64_accessor src_acc = make_strided_float64(src_buffer, size, src_step, skip, true);
        float64_accessor dst_acc = make_strided_float64(dst_buffer, size, dst_step, skip, false);

        expect_compact_ptr(src_acc, src_buffer, src_step, skip);
        expect_compact_ptr(dst_acc, dst_buffer, dst_step, skip);

        bool src_direct = false;
        bool dst_direct = false;
        run_dispatch_scale(policy, src_acc, dst_acc, src_direct, dst_direct);

        EXPECT_EQ(src_direct, src_step == 1);
        EXPECT_EQ(dst_direct, dst_step == 1);

        expect_scaled_strided(dst_buffer, size, dst_step, skip);
    }

    // Compact buffers whose dtypes differ from the accessor type take
    // converting views, which must match the accessors. This is
    // checked with fractional values, which float32 cannot hold exactly.
    {
        const std::vector<float32> src_vals = make_fractional_vals(size);

        Node node;
        node["src"].set(src_vals);
        node["view_des"].set(std::vector<float32>(static_cast<size_t>(size), 0.0f));
        node["acc_des"].set(std::vector<float32>(static_cast<size_t>(size), 0.0f));

        float64_accessor src_acc(node["src"]);
        float64_accessor view_dst_acc(node["view_des"]);
        float64_accessor acc_dst_acc(node["acc_des"]);

        // Neither dtype matches float64 exactly, so there is no exact-match
        // pointer
        EXPECT_TRUE(src_acc.compact_ptr() == nullptr);
        EXPECT_TRUE(view_dst_acc.compact_ptr() == nullptr);

        bool src_is_direct = false;
        bool dst_is_direct = false;
        run_dispatch_scale(policy, src_acc, view_dst_acc, src_is_direct, dst_is_direct);

        EXPECT_TRUE(src_is_direct);
        EXPECT_TRUE(dst_is_direct);

        // Run the same kernel with the accessors directly
        run_dispatch_scale_kernel(policy, size, src_acc, acc_dst_acc);

        float32_array view_res(node["view_des"]);
        float32_array acc_res(node["acc_des"]);
        for (index_t i = 0; i < size; i++)
        {
            const float64 src_val = src_vals[static_cast<size_t>(i)];
            EXPECT_EQ(view_res[i], acc_res[i]);
            EXPECT_EQ(view_res[i], static_cast<float32>(2.0 * src_val));
        }
    }
}

//-----------------------------------------------------------------------------
// Fills a compact buffer of dtype U through a DataAccessor<T>
template <typename T, typename U>
void
check_int_fill(index_t size, bool expect_compact)
{
    Node node;
    node["conn"].set(std::vector<U>(static_cast<size_t>(size), 0));

    DataAccessor<T> conn_acc(node["conn"]);
    EXPECT_EQ(conn_acc.compact_ptr() != nullptr, expect_compact);

    ExecutionPolicy policy = ExecutionPolicy::serial();
    EXPECT_TRUE(run_dispatch_int_fill(policy, conn_acc));

    expect_scaled<T>(node, "conn", size);
}

//-----------------------------------------------------------------------------
// Runs with_write_values() over a variety of integer buffer dtypes
TEST(conduit_execution_array_views, with_write_values_integer_dtypes)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    // The direct path, then converting views over narrower buffers, then the
    // index_t alias, which must also resolve to a direct branch
    check_int_fill<int64, int64>(size, true);
    check_int_fill<int64, int32>(size, false);
    check_int_fill<int64, int16>(size, false);
    check_int_fill<index_t, index_t>(size, true);

    // Fallback: a strided int64 buffer
    {
        std::vector<int64> buffer(static_cast<size_t>(2 * size), -1);
        const index_t stride = 2 * static_cast<index_t>(sizeof(int64));

        int64_accessor conn_acc(buffer.data(), DataType::int64(size, 0, stride));
        EXPECT_TRUE(conn_acc.compact_ptr() == nullptr);

        ExecutionPolicy policy = ExecutionPolicy::serial();
        EXPECT_FALSE(run_dispatch_int_fill(policy, conn_acc));

        expect_scaled_strided(buffer, size, 2, 0);
    }
}

//-----------------------------------------------------------------------------
// Doubles a buffer of dtype U in place through a float64_accessor
template <typename U>
void
check_inplace(index_t size, bool expect_compact)
{
    Node node;
    node["vals"].set(make_dispatch_src_vals<U>(size));

    float64_accessor vals_acc(node["vals"]);
    EXPECT_EQ(vals_acc.compact_ptr() != nullptr, expect_compact);

    ExecutionPolicy policy = ExecutionPolicy::serial();
    EXPECT_TRUE(run_dispatch_inplace(policy, vals_acc));

    expect_scaled<float64>(node, "vals", size);
}

//-----------------------------------------------------------------------------
// Runs with_read_write_values() over a single array, direct and converting
TEST(conduit_execution_array_views, with_read_write_values_in_place)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    check_inplace<float64>(size, true);
    check_inplace<float32>(size, false);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_array_views, grouped_read)
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;
    ExecutionPolicy policy = ExecutionPolicy::serial();

    // A group can use a shared array view when every member shares one compact
    // dtype, so a single odd dtype or a strided member prevents this. Every
    // case is also compared against the same kernel run directly over
    // the accessors to verify we get the same result.
    Node node;
    node["x"].set(make_dispatch_src_vals<float64>(size));
    node["y"].set(make_dispatch_src_vals<float64>(size));
    node["z"].set(make_dispatch_src_vals<float64>(size));
    // Compact, but not the dtype the other two share
    node["odd_y"].set(make_dispatch_src_vals<float32>(size));
    // Values that a float32 buffer cannot hold exactly
    node["fractional"].set(make_fractional_vals(size));

    // The values interleaved with junk, so this member is not compact
    std::vector<float64> y_buffer;
    float64_accessor strided_y_acc = make_strided_float64(y_buffer, size, 2, 0, true);

    const float64_accessor x_accs[4] = {
        float64_accessor(node["x"]),
        float64_accessor(node["x"]),
        float64_accessor(node["x"]),
        float64_accessor(node["fractional"])};
    const float64_accessor y_accs[4] = {
        float64_accessor(node["y"]),
        float64_accessor(node["odd_y"]),
        strided_y_acc,
        float64_accessor(node["fractional"])};
    const float64_accessor z_accs[4] = {
        float64_accessor(node["z"]),
        float64_accessor(node["z"]),
        float64_accessor(node["z"]),
        float64_accessor(node["fractional"])};
    const bool expect_direct[4] = {true, false, false, true};

    for (int c = 0; c < 4; c++)
    {
        std::vector<float64> view_out(static_cast<size_t>(size), 0.0);
        EXPECT_EQ(run_dispatch_group_sum(policy,
                                         x_accs[c],
                                         y_accs[c],
                                         z_accs[c],
                                         view_out.data()),
                  expect_direct[c]);

        // Run the same kernel with the accessors directly
        std::vector<float64> acc_out(static_cast<size_t>(size), 0.0);
        run_dispatch_group_sum_kernel(policy,
                                      size,
                                      x_accs[c],
                                      y_accs[c],
                                      z_accs[c],
                                      acc_out.data());

        for (index_t i = 0; i < size; i++)
        {
            const size_t idx = static_cast<size_t>(i);
            // Verify that we get the same result when running the kernel directly
            EXPECT_EQ(view_out[idx], acc_out[idx]);
            EXPECT_EQ(view_out[idx], 3.0 * x_accs[c][i]);
        }
    }
}

//-----------------------------------------------------------------------------
// Reading a U buffer through a DataAccessor<T> must give the same values
// with and without the dispatch.
template <typename T, typename U>
void
check_read_dtype_parity(index_t size)
{
    Node node;
    node["vals"].set(make_dispatch_src_vals<U>(size));

    DataAccessor<T> acc(node["vals"]);

    ExecutionPolicy policy = ExecutionPolicy::serial();

    std::vector<T> view_out(static_cast<size_t>(size), static_cast<T>(0));
    EXPECT_TRUE(run_dispatch_read_copy(policy, acc, view_out.data()));

    // Run the same kernel with the accessors directly
    std::vector<T> acc_out(static_cast<size_t>(size), static_cast<T>(0));
    run_dispatch_read_copy_kernel(policy, size, acc, acc_out.data());

    // The same values in a strided buffer of the same dtype get no view,
    // but the results must still match
    const index_t elem = static_cast<index_t>(sizeof(U));
    std::vector<U> strided_buffer(static_cast<size_t>(2 * size), static_cast<U>(0));
    for (index_t i = 0; i < size; i++)
    {
        strided_buffer[static_cast<size_t>(2 * i)] = static_cast<U>(i + 1);
    }

    DataAccessor<T> strided_acc(strided_buffer.data(),
                                DataType(node["vals"].dtype().id(),
                                         size,
                                         0,
                                         2 * elem,
                                         elem,
                                         Endianness::DEFAULT_ID));

    std::vector<T> strided_out(static_cast<size_t>(size), static_cast<T>(0));
    EXPECT_FALSE(run_dispatch_read_copy(policy, strided_acc, strided_out.data()));

    for (index_t i = 0; i < size; i++)
    {
        const size_t idx = static_cast<size_t>(i);
        EXPECT_EQ(view_out[idx], acc_out[idx]);
        EXPECT_EQ(view_out[idx], strided_out[idx]);
        EXPECT_EQ(view_out[idx], static_cast<T>(2 * (i + 1)));
    }
}

//-----------------------------------------------------------------------------
// Checks parity for every numeric leaf dtype as the buffer type U
template <typename T>
void
check_read_dtype_parity_all(index_t size)
{
    check_read_dtype_parity<T, int8>(size);
    check_read_dtype_parity<T, int16>(size);
    check_read_dtype_parity<T, int32>(size);
    check_read_dtype_parity<T, int64>(size);
    check_read_dtype_parity<T, uint8>(size);
    check_read_dtype_parity<T, uint16>(size);
    check_read_dtype_parity<T, uint32>(size);
    check_read_dtype_parity<T, uint64>(size);
    check_read_dtype_parity<T, float32>(size);
    check_read_dtype_parity<T, float64>(size);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_array_views, with_read_values_dtype_parity_sweep)
{
    conduit_device_prepare();

    // Small enough that the values still fit in an int8 buffer
    const index_t size = 16;

    check_read_dtype_parity_all<float64>(size);
    check_read_dtype_parity_all<int64>(size);
    check_read_dtype_parity_all<index_t>(size);
}

//-----------------------------------------------------------------------------
// Scales node["src"] into node["des"] under the given policy, checking that
// an array view is used for both source and destination.
void
check_policy_scale(ExecutionPolicy &policy, Node &node, index_t size)
{
    float64_accessor src_acc(node["src"]);
    float64_accessor dst_acc(node["des"]);

    src_acc.use_with(policy);
    dst_acc.use_with(policy);

    bool src_is_direct = false;
    bool dst_is_direct = false;
    run_dispatch_scale(policy, src_acc, dst_acc, src_is_direct, dst_is_direct);

    EXPECT_TRUE(src_is_direct);
    EXPECT_TRUE(dst_is_direct);

    dst_acc.sync();

    expect_scaled<float64>(node, "des", size);
}

//-----------------------------------------------------------------------------
// Runs direct and converting dispatches under every enabled execution policy
void
run_test_with_values_policies()
{
    conduit_device_prepare();

    const index_t size = EXECUTION_TEST_ARRAY_SIZE;

    for_each_enabled_policy([&](ExecutionPolicy policy)
    {
        // Direct path: float64 data
        {
            Node node;
            node["src"].set(make_dispatch_src_vals<float64>(size));
            node["des"].set(make_execution_des_vals(size));

            check_policy_scale(policy, node, size);
        }

        // Converting views on both sides: float32 src, int32 dst
        {
            Node node;
            node["src"].set(make_dispatch_src_vals<float32>(size));
            node["des"].set(std::vector<int32>(static_cast<size_t>(size), 0));

            check_policy_scale(policy, node, size);
        }
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution_array_views, with_values_policies)
{
    // This is a separate function to avoid an issue between lambdas and the
    // gtest macro
    run_test_with_values_policies();
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // Allow overriding the data size via the command line
    if (argc == 2)
    {
        EXECUTION_TEST_ARRAY_SIZE = atoi(argv[1]);
    }

    return RUN_ALL_TESTS();
}
