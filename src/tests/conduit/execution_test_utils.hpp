// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: execution_test_utils.hpp
///
//-----------------------------------------------------------------------------

#ifndef EXECUTION_TEST_UTILS_HPP
#define EXECUTION_TEST_UTILS_HPP

#include "conduit.hpp"
#include "conduit_annotations.hpp"
#include "conduit_execution.hpp"
#include "conduit_memory_manager.hpp"

#include <vector>
#include "gtest/gtest.h"

#if defined(CONDUIT_USE_DEVICE)
#include <umpire/ResourceManager.hpp>
#endif // defined(CONDUIT_USE_DEVICE)

using namespace conduit;
using conduit::execution::ExecutionPolicy;

// Some utility functions for execution tests.

//-----------------------------------------------------------------------------
void
conduit_device_prepare()
{
    execution::init_device_memory_handlers();
}

//-----------------------------------------------------------------------------
std::vector<float64>
make_execution_src_vals(index_t array_size)
{
    std::vector<float64> vals(array_size);
    for(index_t i = 0; i < array_size; i++)
    {
        vals[i] = static_cast<float64>(i + 1);
    }

    return vals;
}

//-----------------------------------------------------------------------------
std::vector<float64>
make_execution_des_vals(index_t array_size)
{
    return std::vector<float64>(array_size, 0.0);
}

//-----------------------------------------------------------------------------
template <typename Func>
void
for_each_enabled_policy(Func &&func)
{
    if (ExecutionPolicy::is_serial_enabled())
    {
        ExecutionPolicy serial = ExecutionPolicy::serial();
        func(serial);
    }

    if (ExecutionPolicy::is_cuda_enabled())
    {
        ExecutionPolicy cuda = ExecutionPolicy::cuda();
        func(cuda);
    }

    if (ExecutionPolicy::is_hip_enabled())
    {
        ExecutionPolicy hip = ExecutionPolicy::hip();
        func(hip);
    }

    if (ExecutionPolicy::is_openmp_enabled())
    {
        ExecutionPolicy openmp = ExecutionPolicy::openmp();
        func(openmp);
    }

    if (ExecutionPolicy::is_host_enabled())
    {
        ExecutionPolicy host = ExecutionPolicy::host();
        func(host);
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        ExecutionPolicy device = ExecutionPolicy::device();
        func(device);
    }

    if (ExecutionPolicy::is_parallel_enabled())
    {
        ExecutionPolicy parallel = ExecutionPolicy::parallel();
        func(parallel);
    }
}

//-----------------------------------------------------------------------------
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
run_dispatch_inplace_kernel(ExecutionPolicy &policy,
                            index_t size,
                            const Vals vals)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        vals.set(idx, 2.0 * vals[idx]);
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
template <typename T>
bool
is_direct_array(const conduit::execution::DirectArrayReader<T> &)
{
    return true;
}

//-----------------------------------------------------------------------------
template <typename T>
bool
is_direct_array(const conduit::execution::DirectArrayWriter<T> &)
{
    return true;
}

//-----------------------------------------------------------------------------
template <typename T>
bool
is_direct_array(const conduit::execution::DirectArrayReadWriter<T> &)
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
    for(index_t i = 0; i < array_size; i++)
    {
        vals[static_cast<size_t>(i)] = static_cast<T>(i + 1);
    }

    return vals;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// STRAWMAN FUNCTIONS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void
run_data_accessor_policy_and_sync(Node &node, ExecutionPolicy policy)
{
    // DataAccessors wrap node leaf data.
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    ExecutionPolicy as = acc_src.active_space();
    std::cout << as.policy_name() << std::endl;

    // Ask the accessors to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // Our forall will execute in the memory space selected by the
    // requested ExecutionPolicy.
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    acc_des.sync();
}

//-----------------------------------------------------------------------------
void
run_data_array_policy_and_sync(Node &node, ExecutionPolicy policy)
{
    // DataArrays wrap node leaf data.
    float64_array arr_src(node["src"]);
    float64_array arr_des(node["des"]);

    // Ask the arrays to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    arr_src.use_with(policy);
    arr_des.use_with(policy);

    // Our forall will execute in the memory space selected by the
    // requested ExecutionPolicy.
    index_t size = arr_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [arr_src, arr_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * arr_src[idx];
        arr_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    arr_des.sync();
}

//-----------------------------------------------------------------------------
void
run_data_accessor_policy_and_assume(Node &node, ExecutionPolicy policy)
{
    // DataAccessors wrap node leaf data.
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    // Ask the accessors to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // Our forall will execute in the memory space selected by the
    // requested ExecutionPolicy.
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // node["des"] takes ownership of the data in the active execution
    // space. This is a no op if node["des"] was already in that space.
    acc_des.assume();
}

//-----------------------------------------------------------------------------
void
run_data_array_policy_and_assume(Node &node, ExecutionPolicy policy)
{
    // DataArrays wrap node leaf data.
    float64_array arr_src(node["src"]);
    float64_array arr_des(node["des"]);

    // Ask the arrays to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    arr_src.use_with(policy);
    arr_des.use_with(policy);

    // Our forall will execute in the memory space selected by the
    // requested ExecutionPolicy.
    index_t size = arr_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [arr_src, arr_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * arr_src[idx];
        arr_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // node["des"] takes ownership of the data in the active execution
    // space. This is a no op if node["des"] was already in that space.
    arr_des.assume();
}

//-----------------------------------------------------------------------------
// the passed-in policy is set to the active space policy so we can test it for
// correctness after this function.
void
run_data_accessor_using_active_space(Node &node, ExecutionPolicy &exec_policy)
{
    // DataAccessors wrap node leaf data.
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    // Use the location of the source data.
    ExecutionPolicy policy = acc_src.active_space();

    // Ask the accessors to move their data to the memory space occupied
    // by node["src"] if their data is not already there.
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // Our forall will execute on the memory space occupied by node["src"]
    // because it was passed an ExecutionPolicy for that space.
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as node["src"].
    acc_des.sync();

    // save the execution policy for testing outside this function
    exec_policy = policy;
}

//-----------------------------------------------------------------------------
// the passed-in policy is set to the active space policy so we can test it for
// correctness after this function.
void
run_data_array_using_active_space(Node &node, ExecutionPolicy &exec_policy)
{
    // DataArrays wrap node leaf data.
    float64_array arr_src(node["src"]);
    float64_array arr_des(node["des"]);

    // Use the location of the source data.
    ExecutionPolicy policy = arr_src.active_space();

    // Ask the arrays to move their data to the memory space occupied
    // by node["src"] if their data is not already there.
    arr_src.use_with(policy);
    arr_des.use_with(policy);

    // Our forall will execute on the memory space occupied by node["src"]
    // because it was passed an ExecutionPolicy for that space.
    index_t size = arr_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [arr_src, arr_des] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * arr_src[idx];
        arr_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as node["src"].
    arr_des.sync();

    // save the execution policy for testing outside this function
    exec_policy = policy;
}

//-----------------------------------------------------------------------------
void
run_data_accessor_reduction_and_sync(Node &node,
                                     ExecutionPolicy policy,
                                     float64 &min_val,
                                     index_t &min_loc)
{
    // DataAccessors wrap node leaf data.
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    // Ask the accessors to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // Initialize values.
    min_val = 0.0;
    min_loc = -1;

    // Reducers are runtime-neutral at the API surface, so they can be used
    // directly inside forall(policy, ...) without recovering a concrete Exec.
    conduit::execution::ReduceMinLoc<float64>
        reducer(std::numeric_limits<float64>::max(), -1);

    // Our forall will execute in the memory space selected by the
    // requested runtime policy.
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        reducer.minloc(val, idx);
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Collect the results.
    min_val = reducer.get();
    min_loc = reducer.getLoc();

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    acc_des.sync();
}

//-----------------------------------------------------------------------------
void
run_data_array_reduction_and_sync(Node &node,
                                  ExecutionPolicy policy,
                                  float64 &min_val,
                                  index_t &min_loc)
{
    // DataArrays wrap node leaf data.
    float64_array arr_src(node["src"]);
    float64_array arr_des(node["des"]);

    // Ask the arrays to move their data to the memory space occupied
    // by the requested execution policy if their data is not already
    // there.
    arr_src.use_with(policy);
    arr_des.use_with(policy);

    // Initialize values.
    min_val = 0.0;
    min_loc = -1;

    // Reducers are runtime-neutral at the API surface, so they can be used
    // directly inside forall(policy, ...) without recovering a concrete Exec.
    conduit::execution::ReduceMinLoc<float64>
        reducer(std::numeric_limits<float64>::max(), -1);

    // Our forall will execute in the memory space selected by the
    // requested runtime policy.
    index_t size = arr_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    {
        const float64 val = 2.0 * arr_src[idx];
        reducer.minloc(val, idx);
        arr_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Collect the results.
    min_val = reducer.get();
    min_loc = reducer.getLoc();

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    arr_des.sync();
}

#if defined(CONDUIT_USE_DEVICE)
//-----------------------------------------------------------------------------
inline size_t
umpire_bytes_allocated(const char *pool_name)
{
    return umpire::ResourceManager::getInstance()
               .getAllocator(pool_name)
               .getCurrentSize();
}

//-----------------------------------------------------------------------------
inline void
expect_no_leak(void (*run_fn)(Node &, ExecutionPolicy),
               index_t node_alloc_id,
               ExecutionPolicy policy)
{
    const index_t n = 1024;
    const std::vector<float64> src_vals(n, 1.0);
    const std::vector<float64> des_vals(n, 0.0);

    // Warm up run to make sure that both the host/device memory pools are
    // created before we query their baseline sizes.
    {
        Node node;
        node["src"].set_allocator(node_alloc_id);
        node["des"].set_allocator(node_alloc_id);
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        // Calls sync/assume for us
        run_fn(node, policy);

        // This should free the memory allocated by sync/assume
        node.reset();
    }

    // Record the baseline sizes of both the host/device memory pools
    const size_t host_baseline = umpire_bytes_allocated("CONDUIT_HOST_POOL");
    const size_t device_baseline = umpire_bytes_allocated("CONDUIT_DEVICE_POOL");

    const int iterations = 4;
    for (int i = 0; i < iterations; i++)
    {
        Node node;
        node["src"].set_allocator(node_alloc_id);
        node["des"].set_allocator(node_alloc_id);
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        // Calls sync/assume for us
        run_fn(node, policy);

        // This should free the memory allocated by sync/assume
        node.reset();
    }

    // The memory allocated above should have been freed, so we expect both
    // the host/device memory pools to be back to where they started.
    EXPECT_EQ(umpire_bytes_allocated("CONDUIT_HOST_POOL"), host_baseline);
    EXPECT_EQ(umpire_bytes_allocated("CONDUIT_DEVICE_POOL"), device_baseline);
}
#endif // defined(CONDUIT_USE_DEVICE)

#endif
