// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_annotations.hpp"
#include "conduit_execution.hpp"
#include "conduit_memory_manager.hpp"
#include "execution_test_utils.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>
#include "gtest/gtest.h"

#if defined(CONDUIT_USE_DEVICE)
#include <umpire/ResourceManager.hpp>
#endif // defined(CONDUIT_USE_DEVICE)

using namespace conduit;
using conduit::execution::ExecutionPolicy;

index_t EXECUTION_TEST_ARRAY_SIZE = 4;

//-----------------------------------------------------------------------------
TEST(conduit_execution, policy_aliases)
{
    const ExecutionPolicy host = ExecutionPolicy::host();
    EXPECT_TRUE(host.is_host_policy());

    const ExecutionPolicy host_from_name("host");
    EXPECT_EQ(host_from_name.policy_id(), host.policy_id());

#if defined(CONDUIT_USE_OPENMP)
    EXPECT_TRUE(host.is_openmp());
    EXPECT_EQ(host.policy_name(), "openmp");
#else
    EXPECT_TRUE(host.is_serial());
    EXPECT_EQ(host.policy_name(), "serial");
#endif

    if (ExecutionPolicy::is_device_enabled())
    {
        const ExecutionPolicy device = ExecutionPolicy::device();
        EXPECT_TRUE(device.is_device_policy());

        const ExecutionPolicy device_from_name("device");
        EXPECT_EQ(device_from_name.policy_id(), device.policy_id());

#if defined(CONDUIT_USE_CUDA)
        EXPECT_TRUE(device.is_cuda());
        EXPECT_EQ(device.policy_name(), "cuda");
#elif defined(CONDUIT_USE_HIP)
        EXPECT_TRUE(device.is_hip());
        EXPECT_EQ(device.policy_name(), "hip");
#endif
    }

    const ExecutionPolicy parallel = ExecutionPolicy::parallel();

#if defined(CONDUIT_USE_CUDA)
    EXPECT_TRUE(ExecutionPolicy::is_parallel_enabled());
    EXPECT_TRUE(parallel.is_parallel_policy());
    EXPECT_TRUE(parallel.is_cuda());
    EXPECT_EQ(parallel.policy_name(), "cuda");
#elif defined(CONDUIT_USE_HIP)
    EXPECT_TRUE(ExecutionPolicy::is_parallel_enabled());
    EXPECT_TRUE(parallel.is_parallel_policy());
    EXPECT_TRUE(parallel.is_hip());
    EXPECT_EQ(parallel.policy_name(), "hip");
#elif defined(CONDUIT_USE_OPENMP)
    EXPECT_TRUE(ExecutionPolicy::is_parallel_enabled());
    EXPECT_TRUE(parallel.is_parallel_policy());
    EXPECT_TRUE(parallel.is_openmp());
    EXPECT_EQ(parallel.policy_name(), "openmp");
#else
    EXPECT_FALSE(ExecutionPolicy::is_parallel_enabled());
    EXPECT_FALSE(parallel.is_parallel_policy());
    EXPECT_TRUE(parallel.is_serial());
    EXPECT_EQ(parallel.policy_name(), "serial");
#endif
    const ExecutionPolicy parallel_from_name("parallel");
    EXPECT_EQ(parallel_from_name.policy_id(), parallel.policy_id());
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, execution_settings)
{
    conduit_device_prepare();
    
    // container for opts whenever we fetch them
    Node get_opts;

#if defined(CONDUIT_USE_DEVICE)
    // if new allocators are added earlier, then these values could change
    const index_t DEVICE_ALLOC_ID = 1;
    const index_t HOST_ALLOC_ID = 2;
#else
    const index_t DEVICE_ALLOC_ID = -1;
    const index_t HOST_ALLOC_ID = 0;
#endif

    // put nodes on host and device
    Node host_data, device_data;

    // ------------------------------
    //
    // Test host and device allocators
    //
    // ------------------------------
    {
        const index_t host_alloc_id = execution::get_host_allocator_id();
        const index_t device_alloc_id = execution::get_device_allocator_id();
        const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);

        host_data.set_allocator(host_alloc_id);
        host_data.set(src_vals);

        // test that allocators are what we expect
        EXPECT_EQ(device_alloc_id, DEVICE_ALLOC_ID);
        EXPECT_EQ(host_alloc_id, HOST_ALLOC_ID);

        // prove that host data is on host
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(host_data.data_ptr()));

#if defined(CONDUIT_USE_DEVICE)
        // with device support, allocate device_data on the device
        device_data.set_allocator(device_alloc_id);
        device_data.set(src_vals);
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(device_data.data_ptr()));
#else // !defined(CONDUIT_USE_DEVICE)
        // without device support, device_data stays on host
        device_data.set(src_vals);
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(device_data.data_ptr()));
#endif // !defined(CONDUIT_USE_DEVICE)
    }

    // ------------------------------
    //
    // test default settings
    //
    // ------------------------------
    {
        Node default_opts;
        execution::execution_options(default_opts);
        EXPECT_TRUE(default_opts.has_child("execution_location"));
        EXPECT_EQ(default_opts["execution_location"].as_string(), "input");
        EXPECT_TRUE(default_opts.has_child("output_location"));
        EXPECT_EQ(default_opts["output_location"].as_string(), "input");
        EXPECT_TRUE(default_opts.has_child("sync_strategy"));
        EXPECT_EQ(default_opts["sync_strategy"].as_string(), "assume");
        EXPECT_TRUE(default_opts.has_child("fallback_location"));
        EXPECT_EQ(default_opts["fallback_location"].as_string(), "host");
        EXPECT_TRUE(default_opts.has_child("device_allocator"));
        EXPECT_EQ(default_opts["device_allocator"].as_index_t(), DEVICE_ALLOC_ID);
        EXPECT_TRUE(default_opts.has_child("host_allocator"));
        EXPECT_EQ(default_opts["host_allocator"].as_index_t(), HOST_ALLOC_ID);
        EXPECT_TRUE(default_opts.has_child("user_provided_allocator"));
        EXPECT_EQ(default_opts["user_provided_allocator"].as_index_t(), -1);
        execution::reset_execution_options();
    }

    // ------------------------------
    //
    // Test Execution Policy settings
    //
    // ------------------------------

    // test bad execution policy
    {
        Node exec_opts;
        exec_opts["execution_location"] = "banana";
        EXPECT_THROW(execution::execution_set_options(exec_opts),
                     conduit::Error);
        execution::reset_execution_options();
    }

    // test host exec policy
    {
        Node exec_opts;
        exec_opts["execution_location"] = "host";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        execution::ExecutionPolicy default_policy = execution::get_execution_policy();
        execution::ExecutionPolicy host_supplied_policy = execution::get_execution_policy(host_data);
        execution::ExecutionPolicy device_supplied_policy = execution::get_execution_policy(device_data);
        EXPECT_TRUE(default_policy.is_host_policy());
        EXPECT_TRUE(host_supplied_policy.is_host_policy());
        EXPECT_TRUE(device_supplied_policy.is_host_policy());
        EXPECT_TRUE(get_opts.has_child("execution_location"));
        EXPECT_EQ(get_opts["execution_location"].as_string(), "host");
        execution::reset_execution_options();
    }

#if defined(CONDUIT_USE_DEVICE)
    // test device exec policy
    {
        Node exec_opts;
        exec_opts["execution_location"] = "device";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        execution::ExecutionPolicy default_policy = execution::get_execution_policy();
        execution::ExecutionPolicy host_supplied_policy = execution::get_execution_policy(host_data);
        execution::ExecutionPolicy device_supplied_policy = execution::get_execution_policy(device_data);
        EXPECT_TRUE(default_policy.is_device_policy());
        EXPECT_TRUE(host_supplied_policy.is_device_policy());
        EXPECT_TRUE(device_supplied_policy.is_device_policy());
        EXPECT_TRUE(get_opts.has_child("execution_location"));
        EXPECT_EQ(get_opts["execution_location"].as_string(), "device");
        execution::reset_execution_options();
    }
#endif // defined(CONDUIT_USE_DEVICE)

    // test input exec policy & w/ host fallback
    {
        Node exec_opts;
        exec_opts["execution_location"] = "input";
        exec_opts["fallback_location"] = "host";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        execution::ExecutionPolicy fallback_policy = execution::get_execution_policy();
        execution::ExecutionPolicy host_supplied_policy = execution::get_execution_policy(host_data);
        execution::ExecutionPolicy device_supplied_policy = execution::get_execution_policy(device_data);
        EXPECT_TRUE(fallback_policy.is_host_policy());
        EXPECT_TRUE(host_supplied_policy.is_host_policy());
#if defined(CONDUIT_USE_DEVICE)
        // with device, device_data is on the device, so input policy gives device policy
        EXPECT_TRUE(device_supplied_policy.is_device_policy());
#else // !defined(CONDUIT_USE_DEVICE)
        // without device, device_data is on host, so input policy gives host policy
        EXPECT_TRUE(device_supplied_policy.is_host_policy());
#endif // !defined(CONDUIT_USE_DEVICE)
        EXPECT_TRUE(get_opts.has_child("execution_location"));
        EXPECT_EQ(get_opts["execution_location"].as_string(), "input");
        EXPECT_TRUE(get_opts.has_child("fallback_location"));
        EXPECT_EQ(get_opts["fallback_location"].as_string(), "host");
        execution::reset_execution_options();
    }

#if defined(CONDUIT_USE_DEVICE)
    // test input exec policy & w/ device fallback
    {
        Node exec_opts;
        exec_opts["execution_location"] = "input";
        exec_opts["fallback_location"] = "device";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        execution::ExecutionPolicy fallback_policy = execution::get_execution_policy();
        execution::ExecutionPolicy host_supplied_policy = execution::get_execution_policy(host_data);
        execution::ExecutionPolicy device_supplied_policy = execution::get_execution_policy(device_data);
        EXPECT_TRUE(fallback_policy.is_device_policy());
        EXPECT_TRUE(host_supplied_policy.is_host_policy());
        EXPECT_TRUE(device_supplied_policy.is_device_policy());
        EXPECT_TRUE(get_opts.has_child("execution_location"));
        EXPECT_EQ(get_opts["execution_location"].as_string(), "input");
        EXPECT_TRUE(get_opts.has_child("fallback_location"));
        EXPECT_EQ(get_opts["fallback_location"].as_string(), "device");
        execution::reset_execution_options();
    }
#endif // defined(CONDUIT_USE_DEVICE)

    // ------------------------------
    //
    // Test Output Allocator settings
    //
    // ------------------------------

    // test bad output allocator
    {
        Node exec_opts;
        exec_opts["output_location"] = "banana";
        EXPECT_THROW(execution::execution_set_options(exec_opts),
                     conduit::Error);
        execution::reset_execution_options();
    }

    // test host output allocator
    {
        Node exec_opts;
        exec_opts["output_location"] = "host";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        index_t default_alloc_id = execution::get_output_allocator_id();
        index_t host_supplied_alloc_id = execution::get_output_allocator_id(host_data);
        index_t device_supplied_alloc_id = execution::get_output_allocator_id(device_data);
        EXPECT_EQ(default_alloc_id, HOST_ALLOC_ID);
        EXPECT_EQ(host_supplied_alloc_id, HOST_ALLOC_ID);
        EXPECT_EQ(device_supplied_alloc_id, HOST_ALLOC_ID);
        EXPECT_TRUE(get_opts.has_child("output_location"));
        EXPECT_EQ(get_opts["output_location"].as_string(), "host");
        execution::reset_execution_options();
    }

    // test device output allocator
    {
        Node exec_opts;
        exec_opts["output_location"] = "device";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        index_t default_alloc_id = execution::get_output_allocator_id();
        index_t host_supplied_alloc_id = execution::get_output_allocator_id(host_data);
        index_t device_supplied_alloc_id = execution::get_output_allocator_id(device_data);
        EXPECT_EQ(default_alloc_id, DEVICE_ALLOC_ID);
        EXPECT_EQ(host_supplied_alloc_id, DEVICE_ALLOC_ID);
        EXPECT_EQ(device_supplied_alloc_id, DEVICE_ALLOC_ID);
        EXPECT_TRUE(get_opts.has_child("output_location"));
        EXPECT_EQ(get_opts["output_location"].as_string(), "device");
        execution::reset_execution_options();
    }

    // test input output allocator & w/ host fallback
    {
        Node exec_opts;
        exec_opts["output_location"] = "input";
        exec_opts["fallback_location"] = "host";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        index_t fallback_alloc_id = execution::get_output_allocator_id();
        index_t host_supplied_alloc_id = execution::get_output_allocator_id(host_data);
        index_t device_supplied_alloc_id = execution::get_output_allocator_id(device_data);
        EXPECT_EQ(fallback_alloc_id, HOST_ALLOC_ID);
        EXPECT_EQ(host_supplied_alloc_id, HOST_ALLOC_ID);
#if defined(CONDUIT_USE_DEVICE)
        // with device, device_data is on the device and returns its device allocator
        EXPECT_EQ(device_supplied_alloc_id, DEVICE_ALLOC_ID);
#else // !defined(CONDUIT_USE_DEVICE)
        // without device, device_data is on host and returns the host allocator
        EXPECT_EQ(device_supplied_alloc_id, HOST_ALLOC_ID);
#endif // !defined(CONDUIT_USE_DEVICE)
        EXPECT_TRUE(get_opts.has_child("output_location"));
        EXPECT_EQ(get_opts["output_location"].as_string(), "input");
        EXPECT_TRUE(get_opts.has_child("fallback_location"));
        EXPECT_EQ(get_opts["fallback_location"].as_string(), "host");
        execution::reset_execution_options();
    }

    // test input output allocator & w/ device fallback
    {
        Node exec_opts;
        exec_opts["output_location"] = "input";
        exec_opts["fallback_location"] = "device";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        index_t fallback_alloc_id = execution::get_output_allocator_id();
        index_t host_supplied_alloc_id = execution::get_output_allocator_id(host_data);
        index_t device_supplied_alloc_id = execution::get_output_allocator_id(device_data);
        EXPECT_EQ(fallback_alloc_id, DEVICE_ALLOC_ID);
        EXPECT_EQ(host_supplied_alloc_id, HOST_ALLOC_ID);
#if defined(CONDUIT_USE_DEVICE)
        // with device, device_data is on the device and returns its device allocator
        EXPECT_EQ(device_supplied_alloc_id, DEVICE_ALLOC_ID);
#else // !defined(CONDUIT_USE_DEVICE)
        // without device, device_data is on host and returns the host allocator
        EXPECT_EQ(device_supplied_alloc_id, HOST_ALLOC_ID);
#endif // !defined(CONDUIT_USE_DEVICE)
        EXPECT_TRUE(get_opts.has_child("output_location"));
        EXPECT_EQ(get_opts["output_location"].as_string(), "input");
        EXPECT_TRUE(get_opts.has_child("fallback_location"));
        EXPECT_EQ(get_opts["fallback_location"].as_string(), "device");
        execution::reset_execution_options();
    }

    // test user_provided output allocator
    {
        Node exec_opts;
        exec_opts["output_location"] = 12345;
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        index_t default_alloc_id = execution::get_output_allocator_id();
        index_t host_supplied_alloc_id = execution::get_output_allocator_id(host_data);
        index_t device_supplied_alloc_id = execution::get_output_allocator_id(device_data);
        EXPECT_EQ(default_alloc_id, 12345);
        EXPECT_EQ(host_supplied_alloc_id, 12345);
        EXPECT_EQ(device_supplied_alloc_id, 12345);
        EXPECT_TRUE(get_opts.has_child("output_location"));
        EXPECT_EQ(get_opts["output_location"].as_string(), "user_provided");
        execution::reset_execution_options();
    }

    // ------------------------------
    //
    // Test Sync Strategy settings
    //
    // ------------------------------

    // test bad sync strategy
    {
        Node exec_opts;
        exec_opts["sync_strategy"] = "banana";
        EXPECT_THROW(execution::execution_set_options(exec_opts),
                     conduit::Error);
    }

    // test sync sync strategy
    {
        Node exec_opts;
        exec_opts["sync_strategy"] = "sync";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        const execution::SyncStrategy sync_strategy = execution::get_sync_strategy();
        EXPECT_EQ(sync_strategy, execution::SyncStrategy::Sync);
        EXPECT_TRUE(get_opts.has_child("sync_strategy"));
        EXPECT_EQ(get_opts["sync_strategy"].as_string(), "sync");
        execution::reset_execution_options();
    }

    // test assume sync strategy
    {
        Node exec_opts;
        exec_opts["sync_strategy"] = "assume";
        execution::execution_set_options(exec_opts);
        execution::execution_options(get_opts);
        const execution::SyncStrategy sync_strategy = execution::get_sync_strategy();
        EXPECT_EQ(sync_strategy, execution::SyncStrategy::Assume);
        EXPECT_TRUE(get_opts.has_child("sync_strategy"));
        EXPECT_EQ(get_opts["sync_strategy"].as_string(), "assume");
        execution::reset_execution_options();
    }
}

//-----------------------------------------------------------------------------
void
run_test_forall()
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_forall policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = EXECUTION_TEST_ARRAY_SIZE;

        std::vector<index_t> host_vals(size);
        index_t *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::DeviceMemory::allocate(sizeof(index_t) * size));
        }
        else
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::HostMemory::allocate(sizeof(index_t) * size));
        }

        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            vals_ptr[i] = i;
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(host_vals.data(),
                                              vals_ptr,
                                              sizeof(index_t) * size);

        for (index_t i = 0; i < size; i ++)
        {
            EXPECT_EQ(host_vals[i], i);
        }

        if (policy.is_device_policy())
        {
            execution::DeviceMemory::deallocate(vals_ptr);
        }
        else
        {
            execution::HostMemory::deallocate(vals_ptr);
        }

        annotations::finalize();
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_forall)
{
    // this is a separate func to avoid issue with lambda vs gtest macro
    run_test_forall();
}

//-----------------------------------------------------------------------------
void
run_test_reductions()
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_reductions policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = 4;
        index_t host_vals[size] = {0, -10, 10, 5};
        index_t *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::DeviceMemory::allocate(sizeof(index_t) * size));
        }
        else
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::HostMemory::allocate(sizeof(index_t) * size));
        }

        conduit::execution::MagicMemory::copy(vals_ptr,
                                              &host_vals[0],
                                              sizeof(index_t) * size);

        index_t sum_result = 0;
        conduit::execution::ReduceSum<index_t> sum_reducer(0);
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            sum_reducer += vals_ptr[i];
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        sum_result = sum_reducer.get();
        EXPECT_EQ(sum_result, 5);

        index_t min_result = 0;
        conduit::execution::ReduceMin<index_t>
            min_reducer(std::numeric_limits<index_t>::max());
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            min_reducer.min(vals_ptr[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        min_result = min_reducer.get();
        EXPECT_EQ(min_result, -10);

        index_t minloc_result = 0;
        index_t minloc_index = -1;
        conduit::execution::ReduceMinLoc<index_t>
            minloc_reducer(std::numeric_limits<index_t>::max(), -1);
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            minloc_reducer.minloc(vals_ptr[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        minloc_result = minloc_reducer.get();
        minloc_index = minloc_reducer.getLoc();
        EXPECT_EQ(minloc_result, -10);
        EXPECT_EQ(minloc_index, 1);

        index_t max_result = 0;
        conduit::execution::ReduceMax<index_t>
            max_reducer(std::numeric_limits<index_t>::lowest());
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            max_reducer.max(vals_ptr[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        max_result = max_reducer.get();
        EXPECT_EQ(max_result, 10);

        index_t maxloc_result = 0;
        index_t maxloc_index = -1;
        conduit::execution::ReduceMaxLoc<index_t>
            maxloc_reducer(std::numeric_limits<index_t>::lowest(), -1);
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            maxloc_reducer.maxloc(vals_ptr[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        maxloc_result = maxloc_reducer.get();
        maxloc_index = maxloc_reducer.getLoc();
        EXPECT_EQ(maxloc_result, 10);
        EXPECT_EQ(maxloc_index, 2);

        if (policy.is_device_policy())
        {
            execution::DeviceMemory::deallocate(vals_ptr);
        }
        else
        {
            execution::HostMemory::deallocate(vals_ptr);
        }

        annotations::finalize();
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_reductions)
{
    // this is a separate func to avoid issue with lambda vs gtest macro
    run_test_reductions();
}

//-----------------------------------------------------------------------------
void
run_test_atomics()
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_atomics policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = 4;
        index_t host_vals[size] = {0, -1, -2, -3};
        index_t *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::DeviceMemory::allocate(sizeof(index_t) * size));
        }
        else
        {
            vals_ptr =
                static_cast<index_t *>(
                    execution::HostMemory::allocate(sizeof(index_t) * size));
        }

        conduit::execution::MagicMemory::copy(vals_ptr,
                                              &host_vals[0],
                                              sizeof(index_t) * size);

        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            conduit::execution::atomic_add(vals_ptr + i, i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], 0);
        }

        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            conduit::execution::atomic_min(vals_ptr + i,
                                           static_cast<index_t>(-10));
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], -10);
        }

        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            conduit::execution::atomic_max(vals_ptr + i,
                                           static_cast<index_t>(10));
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], 10);
        }

        if (policy.is_device_policy())
        {
            execution::DeviceMemory::deallocate(vals_ptr);
        }
        else
        {
            execution::HostMemory::deallocate(vals_ptr);
        }

        annotations::finalize();
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_atomics)
{
    // this is a separate func to avoid issue with lambda vs gtest macro
    run_test_atomics();
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_data_accessor)
{
    conduit_device_prepare();

    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    // our main loop iterates over these items
    // comment out specific entries to test a specific combination
    const std::vector<std::string> src_locations = {"host", "device"};
    const std::vector<std::string> des_locations = {"host", "device"};
    const std::vector<std::string> policies      = {"host", "device"};

    // TODO: This loop boilerplate can be extracted into a helper function and reused
    for (const std::string &src_start : src_locations)
    {
        for (const std::string &des_start : des_locations)
        {
            // Compute this once since we'll use it in several places
            const bool is_src_or_dst_device = (src_start == "device" || des_start == "device");

            //----------------------------------------------------------
            // DataAccessor sync
            // Run with an explicit execution policy and call sync().
            // node["des"] is synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataAccessor sync():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                run_data_accessor_policy_and_sync(node, policy);

                annotations::finalize();

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_accessor result_acc(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_acc.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_acc.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_acc[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataAccessor assume
            // Run with an explicit execution policy and call assume().
            // node["des"] keeps the result buffer where it executed.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataAccessor assume():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                run_data_accessor_policy_and_assume(node, policy);

                annotations::finalize();

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // but destination memory will move to be where policy is
                if (policy_str == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_accessor result_acc(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_acc.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_acc.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_acc[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataAccessor active_space
            // Use the location of node["src"] to choose where to execute.
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            if ((src_start == "host" && des_start == "host") ||
                ExecutionPolicy::is_device_enabled())
            {
                CONDUIT_INFO("DataAccessor active_space():\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                ExecutionPolicy policy;
                run_data_accessor_using_active_space(node, policy);

                annotations::finalize();

                if (src_start == "host")
                {
                    EXPECT_TRUE(policy.is_host_policy());
                }
                else
                {
                    EXPECT_TRUE(policy.is_device_policy());
                }

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_accessor result_acc(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_acc.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_acc.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_acc[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataAccessor reduction
            // Run with an explicit execution policy, reduce directly inside
            // forall(policy, ...), and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataAccessor reduction:\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                float64 min_val; index_t min_loc;
                run_data_accessor_reduction_and_sync(node, policy, min_val, min_loc);

                annotations::finalize();

                EXPECT_EQ(min_val, 2.0);
                EXPECT_EQ(min_loc, 0);

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_accessor result_acc(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_acc.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_acc.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_acc[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }
        }
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_data_array)
{
    conduit_device_prepare();

    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    // our main loop iterates over these items
    // comment out specific entries to test a specific combination
    const std::vector<std::string> src_locations = {"host", "device"};
    const std::vector<std::string> des_locations = {"host", "device"};
    const std::vector<std::string> policies      = {"host", "device"};

    // TODO: This loop boilerplate can be extracted into a helper function and reused
    for (const std::string &src_start : src_locations)
    {
        for (const std::string &des_start : des_locations)
        {
            // Compute this once since we'll use it in several places
            const bool is_src_or_dst_device = (src_start == "device" || des_start == "device");

            //----------------------------------------------------------
            // DataArray sync
            // Run with an explicit execution policy and call sync().
            // node["des"] is synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataArray sync():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                run_data_array_policy_and_sync(node, policy);

                annotations::finalize();

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_array result_arr(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_arr.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_arr.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_arr[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataArray assume
            // Run with an explicit execution policy and call assume().
            // node["des"] keeps the result buffer where it executed.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataArray assume():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                run_data_array_policy_and_assume(node, policy);

                annotations::finalize();

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // but destination memory will move to be where policy is
                if (policy_str == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_array result_arr(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_arr.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_arr.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_arr[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataArray active_space
            // Use the location of node["src"] to choose where to execute.
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            if ((src_start == "host" && des_start == "host") ||
                ExecutionPolicy::is_device_enabled())
            {
                CONDUIT_INFO("DataArray active_space():\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                ExecutionPolicy policy;
                run_data_array_using_active_space(node, policy);

                annotations::finalize();

                if (src_start == "host")
                {
                    EXPECT_TRUE(policy.is_host_policy());
                }
                else
                {
                    EXPECT_TRUE(policy.is_device_policy());
                }

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }

                float64_array result_arr(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_arr.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_arr.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_arr[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataArray reduction
            // Run with an explicit execution policy, reduce directly inside
            // forall(policy, ...), and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                // Skip device-dependent combos if conduit was built without device support
                if ((is_src_or_dst_device || policy_str == "device") &&
                    !ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_location"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataArray reduction:\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                ExecutionPolicy policy = conduit::execution::get_execution_policy();
                Node node;
                if (src_start == "host")
                {
                    node["src"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["src"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                if (des_start == "host")
                {
                    node["des"].set_allocator(conduit::execution::get_host_allocator_id());
                }
                else
                {
                    node["des"].set_allocator(conduit::execution::get_device_allocator_id());
                }
                node["src"].set(src_vals);
                node["des"].set(des_vals);

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                float64 min_val; index_t min_loc;
                run_data_array_reduction_and_sync(node, policy, min_val, min_loc);

                annotations::finalize();

                EXPECT_EQ(min_val, 2.0);
                EXPECT_EQ(min_loc, 0);

                // src data will never change from where it started
                if (src_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
                }
                // des data is sync'd back to where it started
                if (des_start == "host")
                {
                    EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }
                else
                {
                    EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
                }


                float64_array result_arr(node["des"]);
                // Verification runs on the host, so use a host execution policy
                // in case node["des"] still owns device-backed data here.
                result_arr.use_with(ExecutionPolicy::host());
                EXPECT_EQ(result_arr.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_arr[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }
        }
    }
}

#if defined(CONDUIT_USE_DEVICE)
//-----------------------------------------------------------------------------
static size_t
umpire_bytes_allocated(const char *pool_name)
{
    return umpire::ResourceManager::getInstance()
               .getAllocator(pool_name)
               .getCurrentSize();
}

//-----------------------------------------------------------------------------
static void
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

//-----------------------------------------------------------------------------
TEST(conduit_execution, no_memory_leak_on_data_accessor_sync)
{
    conduit_device_prepare();

    // Data originates on the host but is executed on the device
    expect_no_leak(run_data_accessor_policy_and_sync,
                   conduit::execution::get_host_allocator_id(),
                   ExecutionPolicy::device());

    // Data originates on the device but is executed on the host
    expect_no_leak(run_data_accessor_policy_and_sync,
                   conduit::execution::get_device_allocator_id(),
                   ExecutionPolicy::host());
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, no_memory_leak_on_data_accessor_assume)
{
    conduit_device_prepare();

    // Data originates on the host but is executed on the device
    expect_no_leak(run_data_accessor_policy_and_assume,
                   conduit::execution::get_host_allocator_id(),
                   ExecutionPolicy::device());

    // Data originates on the device but is executed on the host
    expect_no_leak(run_data_accessor_policy_and_assume,
                   conduit::execution::get_device_allocator_id(),
                   ExecutionPolicy::host());
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, no_memory_leak_on_data_array_sync)
{
    conduit_device_prepare();

    // Data originates on the host but is executed on the device
    expect_no_leak(run_data_array_policy_and_sync,
                   conduit::execution::get_host_allocator_id(),
                   ExecutionPolicy::device());

    // Data originates on the device but is executed on the host
    expect_no_leak(run_data_array_policy_and_sync,
                   conduit::execution::get_device_allocator_id(),
                   ExecutionPolicy::host());
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, no_memory_leak_on_data_array_assume)
{
    conduit_device_prepare();

    // Data originates on the host but is executed on the device
    expect_no_leak(run_data_array_policy_and_assume,
                   conduit::execution::get_host_allocator_id(),
                   ExecutionPolicy::device());

    // Data originates on the device but is executed on the host
    expect_no_leak(run_data_array_policy_and_assume,
                   conduit::execution::get_device_allocator_id(),
                   ExecutionPolicy::host());
}
#endif // defined(CONDUIT_USE_DEVICE)

//-----------------------------------------------------------------------------
void
run_test_sort_ascending()
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_sort_ascending policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = EXECUTION_TEST_ARRAY_SIZE;

        index_t *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr = static_cast<index_t *>(execution::DeviceMemory::allocate(sizeof(index_t) * size));
        }
        else // if (!policy.is_device_policy())
        {
            vals_ptr = static_cast<index_t *>(execution::HostMemory::allocate(sizeof(index_t) * size));
        }

        // Fill the array with descending numbers
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            vals_ptr[i] = size - 1 - i;
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::sort_ascending(policy, vals_ptr, vals_ptr + size);
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        std::vector<index_t> host_vals(size);
        conduit::execution::MagicMemory::copy(host_vals.data(), vals_ptr, sizeof(index_t) * size);

        // Validate that the array is sorted in ascending order
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], i);
        }

        if (policy.is_device_policy())
        {
            execution::DeviceMemory::deallocate(vals_ptr);
        }
        else // if (!policy.is_device_policy())
        {
            execution::HostMemory::deallocate(vals_ptr);
        }

        annotations::finalize();
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_sort_ascending)
{
    // this is a separate func to avoid issue with lambda vs gtest macro
    run_test_sort_ascending();
}

//-----------------------------------------------------------------------------
void
run_test_sort_descending()
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_sort_descending policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = EXECUTION_TEST_ARRAY_SIZE;

        index_t *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr = static_cast<index_t *>(execution::DeviceMemory::allocate(sizeof(index_t) * size));
        }
        else // if (!policy.is_device_policy())
        {
            vals_ptr = static_cast<index_t *>(execution::HostMemory::allocate(sizeof(index_t) * size));
        }

        // Fill the array with ascending numbers
        conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
        {
            vals_ptr[i] = i;
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::sort_descending(policy, vals_ptr, vals_ptr + size);
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        std::vector<index_t> host_vals(size);
        conduit::execution::MagicMemory::copy(host_vals.data(), vals_ptr, sizeof(index_t) * size);

        // Validate that the array is sorted in descending order
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], size - 1 - i);
        }

        if (policy.is_device_policy())
        {
            execution::DeviceMemory::deallocate(vals_ptr);
        }
        else // if (!policy.is_device_policy())
        {
            execution::HostMemory::deallocate(vals_ptr);
        }

        annotations::finalize();
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_sort_descending)
{
    // this is a separate func to avoid issue with lambda vs gtest macro
    run_test_sort_descending();
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // allow override of the data size via the command line
    if(argc == 2)
    {
        EXECUTION_TEST_ARRAY_SIZE = atoi(argv[1]);
    }

    return RUN_ALL_TESTS();
}
