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
#include <type_traits>
#include <vector>
#include "gtest/gtest.h"

using namespace conduit;
using conduit::execution::ExecutionPolicy;

index_t EXECUTION_TEST_ARRAY_SIZE = 4;

//-----------------------------------------------------------------------------
template <typename ArrayType>
void
expect_doubled_execution_values(const ArrayType &values, index_t array_size)
{
    // Shared by the data array and data accessor execution tests.
    EXPECT_EQ(values.number_of_elements(), array_size);
    for(index_t i = 0; i < array_size; i++)
    {
        EXPECT_EQ(values[i], 2.0 * static_cast<float64>(i + 1));
    }
}

//-----------------------------------------------------------------------------
// Tests
//-----------------------------------------------------------------------------

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
TEST(conduit_execution, test_forall)
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        CONDUIT_INFO("test_forall policy=" << policy.policy_name());
        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);

        const index_t size = 10;

        index_t host_vals[size];
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

        conduit::execution::MagicMemory::copy(&host_vals[0],
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
TEST(conduit_execution, test_reductions)
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
        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::ReduceSum<Exec, index_t> sum_reducer(0);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                sum_reducer += vals_ptr[i];
            });
            CONDUIT_DEVICE_ERROR_CHECK(policy);
            sum_result = sum_reducer.get();
        });
        EXPECT_EQ(sum_result, 5);

        index_t min_result = 0;
        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::ReduceMin<Exec, index_t>
                min_reducer(std::numeric_limits<index_t>::max());
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                min_reducer.min(vals_ptr[i]);
            });
            CONDUIT_DEVICE_ERROR_CHECK(policy);
            min_result = min_reducer.get();
        });
        EXPECT_EQ(min_result, -10);

        index_t minloc_result = 0;
        index_t minloc_index = -1;
        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::ReduceMinLoc<Exec, index_t>
                minloc_reducer(std::numeric_limits<index_t>::max(), -1);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                minloc_reducer.minloc(vals_ptr[i], i);
            });
            CONDUIT_DEVICE_ERROR_CHECK(policy);
            minloc_result = minloc_reducer.get();
            minloc_index = minloc_reducer.getLoc();
        });
        EXPECT_EQ(minloc_result, -10);
        EXPECT_EQ(minloc_index, 1);

        index_t max_result = 0;
        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::ReduceMax<Exec, index_t>
                max_reducer(std::numeric_limits<index_t>::lowest());
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                max_reducer.max(vals_ptr[i]);
            });
            CONDUIT_DEVICE_ERROR_CHECK(policy);
            max_result = max_reducer.get();
        });
        EXPECT_EQ(max_result, 10);

        index_t maxloc_result = 0;
        index_t maxloc_index = -1;
        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::ReduceMaxLoc<Exec, index_t>
                maxloc_reducer(std::numeric_limits<index_t>::lowest(), -1);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                maxloc_reducer.maxloc(vals_ptr[i], i);
            });
            CONDUIT_DEVICE_ERROR_CHECK(policy);
            maxloc_result = maxloc_reducer.get();
            maxloc_index = maxloc_reducer.getLoc();
        });
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
TEST(conduit_execution, test_atomics)
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

        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                conduit::execution::atomic_add<Exec>(vals_ptr + i, i);
            });
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], 0);
        }

        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                conduit::execution::atomic_min<Exec>(vals_ptr + i,
                                                     static_cast<index_t>(-10));
            });
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], -10);
        }

        conduit::execution::dispatch(policy, [&](auto exec)
        {
            using Exec = decltype(exec);
            conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i)
            {
                conduit::execution::atomic_max<Exec>(vals_ptr + i,
                                                     static_cast<index_t>(10));
            });
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

    for (const std::string &src_start : src_locations)
    {
        for (const std::string &des_start : des_locations)
        {
            //----------------------------------------------------------
            // DataAccessor sync
            // Run with an explicit execution policy and call sync().
            // node["des"] is synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                ExecutionPolicy policy;
                if (policy_str == "host")
                {
                    policy = ExecutionPolicy::host();
                }
                else
                {
                    if (ExecutionPolicy::is_device_enabled())
                    {
                        policy = ExecutionPolicy::device();
                    }
                    else
                    {
                        continue;
                    }
                }

                CONDUIT_INFO("DataAccessor sync():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                float64 *src_device_ptr = nullptr;
                float64 *des_device_ptr = nullptr;
                if (src_start == "host")
                {
                    node["src"].set(src_vals);
                }
                else
                {
                    src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
                    node["src"].set_external(src_device_ptr, src_vals.size());
                }
                if (des_start == "host")
                {
                    node["des"].set(des_vals);
                }
                else
                {
                    des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
                    node["des"].set_external(des_device_ptr, des_vals.size());
                }

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
                expect_doubled_execution_values(result_acc, EXECUTION_TEST_ARRAY_SIZE);

                node.reset();

                if (src_start == "device")
                {
                    execution::DeviceMemory::deallocate(src_device_ptr);
                }
                if (des_start == "device")
                {
                    execution::DeviceMemory::deallocate(des_device_ptr);
                }
            }

            //----------------------------------------------------------
            // DataAccessor assume
            // Run with an explicit execution policy and call assume().
            // node["des"] keeps the result buffer where it executed.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                ExecutionPolicy policy;
                if (policy_str == "host")
                {
                    policy = ExecutionPolicy::host();
                }
                else
                {
                    if (ExecutionPolicy::is_device_enabled())
                    {
                        policy = ExecutionPolicy::device();
                    }
                    else
                    {
                        continue;
                    }
                }

                CONDUIT_INFO("DataAccessor assume():\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                float64 *src_device_ptr = nullptr;
                float64 *des_device_ptr = nullptr;
                if (src_start == "host")
                {
                    node["src"].set(src_vals);
                }
                else
                {
                    src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
                    node["src"].set_external(src_device_ptr, src_vals.size());
                }
                if (des_start == "host")
                {
                    node["des"].set(des_vals);
                }
                else
                {
                    des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
                    node["des"].set_external(des_device_ptr, des_vals.size());
                }

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
                expect_doubled_execution_values(result_acc, EXECUTION_TEST_ARRAY_SIZE);

                node.reset();

                if (src_start == "device")
                {
                    execution::DeviceMemory::deallocate(src_device_ptr);
                }
                // we only need to free if node now points somewhere else
                if (des_start == "device" && policy_str == "host")
                {
                    execution::DeviceMemory::deallocate(des_device_ptr);
                }
            }

            //----------------------------------------------------------
            // DataAccessor active_space
            // Use the location of node["src"] to choose where to execute.
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            if ((src_start == "host" && des_start == "host") ||
                (ExecutionPolicy::is_device_enabled()))
            {
                CONDUIT_INFO("DataAccessor active_space():\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                float64 *src_device_ptr = nullptr;
                float64 *des_device_ptr = nullptr;
                if (src_start == "host")
                {
                    node["src"].set(src_vals);
                }
                else
                {
                    src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
                    node["src"].set_external(src_device_ptr, src_vals.size());
                }
                if (des_start == "host")
                {
                    node["des"].set(des_vals);
                }
                else
                {
                    des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
                    node["des"].set_external(des_device_ptr, des_vals.size());
                }

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
                expect_doubled_execution_values(result_acc, EXECUTION_TEST_ARRAY_SIZE);

                node.reset();

                if (src_start == "device")
                {
                    execution::DeviceMemory::deallocate(src_device_ptr);
                }
                if (des_start == "device")
                {
                    execution::DeviceMemory::deallocate(des_device_ptr);
                }
            }

            //----------------------------------------------------------
            // DataAccessor dispatch lambda
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                ExecutionPolicy policy;
                if (policy_str == "host")
                {
                    policy = ExecutionPolicy::host();
                }
                else
                {
                    if (ExecutionPolicy::is_device_enabled())
                    {
                        policy = ExecutionPolicy::device();
                    }
                    else
                    {
                        continue;
                    }
                }

                CONDUIT_INFO("DataAccessor dispatch() lambda:\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                float64 *src_device_ptr = nullptr;
                float64 *des_device_ptr = nullptr;
                if (src_start == "host")
                {
                    node["src"].set(src_vals);
                }
                else
                {
                    src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
                    node["src"].set_external(src_device_ptr, src_vals.size());
                }
                if (des_start == "host")
                {
                    node["des"].set(des_vals);
                }
                else
                {
                    des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
                    node["des"].set_external(des_device_ptr, des_vals.size());
                }

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                float64 min_val; index_t min_loc;
                run_data_accessor_dispatch_and_sync(node, policy, min_val, min_loc);

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
                expect_doubled_execution_values(result_acc, EXECUTION_TEST_ARRAY_SIZE);

                node.reset();

                if (src_start == "device")
                {
                    execution::DeviceMemory::deallocate(src_device_ptr);
                }
                if (des_start == "device")
                {
                    execution::DeviceMemory::deallocate(des_device_ptr);
                }
            }

            //----------------------------------------------------------
            // DataAccessor dispatch functor
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                ExecutionPolicy policy;
                if (policy_str == "host")
                {
                    policy = ExecutionPolicy::host();
                }
                else
                {
                    if (ExecutionPolicy::is_device_enabled())
                    {
                        policy = ExecutionPolicy::device();
                    }
                    else
                    {
                        continue;
                    }
                }

                CONDUIT_INFO("DataAccessor dispatch() functor:\n" <<
                             "    policy="    << policy_str << "\n" <<
                             "    src_start=" << src_start << "\n" <<
                             "    des_start=" << des_start);

                Node node;
                float64 *src_device_ptr = nullptr;
                float64 *des_device_ptr = nullptr;
                if (src_start == "host")
                {
                    node["src"].set(src_vals);
                }
                else
                {
                    src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
                    node["src"].set_external(src_device_ptr, src_vals.size());
                }
                if (des_start == "host")
                {
                    node["des"].set(des_vals);
                }
                else
                {
                    des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
                    node["des"].set_external(des_device_ptr, des_vals.size());
                }

                Node cali_opts;
                cali_opts["config"] = "runtime-report";
                annotations::initialize(cali_opts);

                float64 min_val; index_t min_loc;
                run_data_accessor_dispatch_and_sync(node, policy, min_val, min_loc);

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
                expect_doubled_execution_values(result_acc, EXECUTION_TEST_ARRAY_SIZE);

                node.reset();

                if (src_start == "device")
                {
                    execution::DeviceMemory::deallocate(src_device_ptr);
                }
                if (des_start == "device")
                {
                    execution::DeviceMemory::deallocate(des_device_ptr);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Test the DataArray strawman execution paths when both node["src"] and
// node["des"] start in host memory. The covered cases are explicit host
// `sync`, explicit device `sync`, explicit host `assume`, explicit device
// `assume`, and `active_space` dispatch from host-backed source data.
TEST(conduit_execution, strawman_data_array_src_host_des_host)
{
    conduit_device_prepare();
    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    // Run with an explicit host execution policy.
    // node["des"] is synced back to host memory.
    {
        CONDUIT_INFO("data_array sync policy=host src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        // Run with an explicit device execution policy.
        // node["des"] is synced back to host memory.
        CONDUIT_INFO("data_array sync policy=device src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=host src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        // Run with an explicit device execution policy and call assume().
        // node["des"] keeps the device-backed result buffer.
        CONDUIT_INFO("data_array assume policy=device src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to host memory.
    {
        CONDUIT_INFO("data_array active_space src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        ExecutionPolicy policy;
        run_data_array_using_active_space(node, policy);
        EXPECT_TRUE(policy.is_host_policy());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        CONDUIT_INFO("data_array dispatch lambda policy=host src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::host(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=host src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::host(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=device src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::device(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        CONDUIT_INFO("data_array dispatch functor policy=device src_start=host des_start=host");

        Node node;
        node["src"].set(src_vals);
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::device(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }
}

//-----------------------------------------------------------------------------
// Test the DataArray strawman execution paths when both node["src"] and
// node["des"] start in device memory. The covered cases are explicit host
// `sync`, explicit device `sync`, explicit host `assume`, explicit device
// `assume`, and `active_space` dispatch from device-backed source data.
TEST(conduit_execution, strawman_data_array_src_device_des_device)
{
    conduit_device_prepare();
    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to device memory.
    {
        CONDUIT_INFO("data_array sync policy=host src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to device memory.
    {
        CONDUIT_INFO("data_array sync policy=device src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=host src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=device src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to device memory.
    {
        CONDUIT_INFO("data_array active_space src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        ExecutionPolicy policy;
        run_data_array_using_active_space(node, policy);
        EXPECT_TRUE(policy.is_device_policy());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=host src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::host(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=host src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::host(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=device src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::device(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=device src_start=device des_start=device");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::device(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }
}

//-----------------------------------------------------------------------------
// Test the DataArray strawman execution paths when node["src"] starts in host
// memory and node["des"] starts in device memory. The covered cases are
// explicit host `sync`, explicit device `sync`, explicit host `assume`,
// explicit device `assume`, and `active_space` dispatch from host-backed
// source data.
TEST(conduit_execution, strawman_data_array_src_host_des_device)
{
    conduit_device_prepare();
    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to device memory.
    {
        CONDUIT_INFO("data_array sync policy=host src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to device memory.
    {
        CONDUIT_INFO("data_array sync policy=device src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=host src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=device src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to device memory.
    {
        CONDUIT_INFO("data_array active_space src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        ExecutionPolicy policy;
        run_data_array_using_active_space(node, policy);
        EXPECT_TRUE(policy.is_host_policy());
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=host src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::host(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=host src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::host(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=device src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::device(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=device src_start=host des_start=device");

        Node node;
        float64 *des_device_ptr = make_float64_device_buffer(des_vals.data(), des_vals.size());
        node["src"].set(src_vals);
        node["des"].set_external(des_device_ptr, des_vals.size());

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::device(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }
}

//-----------------------------------------------------------------------------
// Test the DataArray strawman execution paths when node["src"] starts in
// device memory and node["des"] starts in host memory. The covered cases are
// explicit host `sync`, explicit device `sync`, explicit host `assume`,
// explicit device `assume`, and `active_space` dispatch from device-backed
// source data.
TEST(conduit_execution, strawman_data_array_src_device_des_host)
{
    conduit_device_prepare();
    const std::vector<float64> src_vals = make_execution_src_vals(EXECUTION_TEST_ARRAY_SIZE);
    const std::vector<float64> des_vals = make_execution_des_vals(EXECUTION_TEST_ARRAY_SIZE);

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to host memory.
    {
        CONDUIT_INFO("data_array sync policy=host src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to host memory.
    {
        CONDUIT_INFO("data_array sync policy=device src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_sync(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=host src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::host());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        CONDUIT_INFO("data_array assume policy=device src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        run_data_array_policy_and_assume(node, ExecutionPolicy::device());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to host memory.
    {
        CONDUIT_INFO("data_array active_space src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        ExecutionPolicy policy;
        run_data_array_using_active_space(node, policy);
        EXPECT_TRUE(policy.is_device_policy());
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));

        float64_array result_array(node["des"]);
        // Verification runs on the host, so use a host execution policy
        // in case node["des"] still owns device-backed data here.
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=host src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::host(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=host src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::host(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch lambda policy=device src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_and_sync(node,
                                         ExecutionPolicy::device(),
                                         min_val,
                                         min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    {
        CONDUIT_INFO("data_array dispatch functor policy=device src_start=device des_start=host");

        Node node;
        float64 *src_device_ptr = make_float64_device_buffer(src_vals.data(), src_vals.size());
        node["src"].set_external(src_device_ptr, src_vals.size());
        node["des"].set(des_vals);

        Node cali_opts;
        cali_opts["config"] = "runtime-report";
        annotations::initialize(cali_opts);
        float64 min_val; index_t min_loc;
        run_data_array_dispatch_functor_and_sync(node,
                                                 ExecutionPolicy::device(),
                                                 min_val,
                                                 min_loc);
        annotations::finalize();
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
        EXPECT_EQ(min_val, 2.0);
        EXPECT_EQ(min_loc, 0);

        float64_array result_array(node["des"]);
        result_array.use_with(ExecutionPolicy::host());
        expect_doubled_execution_values(result_array, EXECUTION_TEST_ARRAY_SIZE);

        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }
}

    // // TODO are there other cases in the notes?
    // //------------------------------------------------------
    // // forall cases
    // //------------------------------------------------------

    // //------------------------------------------------------
    // // run on device
    // //------------------------------------------------------
    // if (ExecutionPolicy::is_device_enabled())
    // {
    //     Node node;
    //     std::vector<int64> data_src = {0, 1, 2, 3};
    //     node["src"].set(data_src);
    //     std::vector<int64> data_des = {0, 0, 0, 0};
    //     node["src"].set(data_des);
    //     ExecutionAccessor<float64> acc_src(node["src"]);
    //     ExecutionAccessor<float64> acc_des(node["des"]);

    //     ExecutionPolicy policy = ExecutionPolicy::device();

    //     acc_src.use_with(policy);
    //     acc_des.use_with(policy);

    //     index_t size = acc_src.number_of_elements();

    //     forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    //     {
    //         const float64 val = 2.0 * acc_src[idx];
    //         acc_des.set(idx,val);
    //     });
    //     CONDUIT_DEVICE_ERROR_CHECK();

    //     // sync values to node["des"]
    //     // (no op if node["des"] was originally device memory)
    //     acc_des.sync();
    // }

    // //------------------------------------------------------
    // // run on device, 
    // // result stays on device and is owned by node["des"],
    // // even if not on the device before hand
    // //------------------------------------------------------
    // {
    //     Node node;
    //     ExecutionAccessor<float64> acc_src(node["src"]);
    //     ExecutionAccessor<float64> acc_des(node["des"]);

    //     ExecutionPolicy policy = ExecutionPolicy::device();

    //     acc_src.use_with(policy);
    //     acc_des.use_with(policy);

    //     index_t size = acc_src.number_of_elements();

    //     forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    //     {
    //         const float64 val = 2.0 * acc_src[idx];
    //         acc_des.set(idx,val);
    //     });
    //     CONDUIT_DEVICE_ERROR_CHECK();

    //     // move results to be owned by node["des"]
    //     // (no op if node["des"] was originally device memory)
    //     acc_des.move(node["des"]); 
    // }

    // //------------------------------------------------------
    // // run where the src data is
    // //------------------------------------------------------
    // {
    //     Node node;
    //     ExecutionAccessor<float64> acc_src(node["src"]);
    //     ExecutionAccessor<float64> acc_des(node["des"]);

    //     ExecutionPolicy policy = acc_src.active_space().execution_policy();
    //     acc_des.use_with(policy);
    //     acc_des.use_with(policy);

    //     index_t size = acc_src.number_of_elements();

    //     forall(policy, 0, size, [=] CONDUIT_EXEC(index_t idx)
    //     {
    //         const float64 val = 2.0 * acc_src[idx];
    //         acc_des.set(idx,val);
    //     });
    //     CONDUIT_DEVICE_ERROR_CHECK();

    //     // sync values to node["des"], 
    //     // (no op if node["des"] was originally in 
    //     //  same memory space as node["src"] )
    //     acc_des.sync(node["des"]); 
    // }

    // //------------------------------------------------------
    // // more complex cases
    // //------------------------------------------------------

    // //------------------------------------------------------
    // // complex run on device 
    // // double lambda forwarding concrete template tag
    // // for use in lambda
    // //
    // // ( requires c++ 20 b/c of templated lambda)
    // //------------------------------------------------------
    // {
    //     Node node;
    //     ExecutionAccessor<float64> acc_src(node["src"]);
    //     ExecutionAccessor<float64> acc_des(node["des"]);

    //     ExecutionPolicy policy = ExecutionPolicy::device();
    //     acc_des.use_with(policy);
    //     acc_des.use_with(policy);

    //     index_t size = acc_src.number_of_elements();

    //     index_t min_loc = -1;
    //     float64 min_val = 0;

    //     dispatch(policy, [&] <typename Exec>(Exec &exec)
    //     {
    //         float64 identity = std::numeric_limits<float64>::max();
    //         using for_policy    = typename Exec::for_policy;
    //         using reduce_policy = typename Exec::reduce_policy;

    //         ReduceMinLoc<reduce_policy,float64> reducer(identity,-1);

    //         forall<for_policy>(0, size, [=] CONDUIT_EXEC (int i)
    //         {
    //             const float64 val = 2.0 * acc_src[idx];
    //             reducer.minloc(val,i);
    //             acc_des.set(idx,val);
    //         });
    //         CONDUIT_DEVICE_ERROR_CHECK();

    //         min_val = reducer.get();
    //         min_loc = reducer.getLoc();
    //     });

    //     // sync values to node["des"], 
    //     // (no op if node["des"] was originally in
    //     //  same memory space as node["src"] )
    //     acc_des.sync(node["des"]); 
    // }

    // //------------------------------------------------------
    // // complex run on device using functor
    // // (functor implementation)
    // //------------------------------------------------------
    // struct ExecFunctor
    // {
    //     float64 min_val;
    //     index_t min_loc;

    //     ExecutionAccessor<float64> acc_src;
    //     ExecutionAccessor<float64> acc_des;

    //     template<typename Exec>
    //     void operator()(Exec &exec)
    //     {
    //         float64 identity = std::numeric_limits<float64>::max();
    //         using for_policy    = typename Exec::for_policy;
    //         using reduce_policy = typename Exec::reduce_policy;

    //         ReduceMinLoc<reduce_policy,float64> reducer(identity, -1);

    //         forall<for_policy>(0, size, [=] (int i)
    //         {
    //             const float64 val = 2.0 * acc_src[idx];
    //             reducer.minloc(val,i);
    //             acc_des.set(idx,val);
    //         });
    //         CONDUIT_DEVICE_ERROR_CHECK();

    //         min_val = reducer.get();
    //         min_loc = reducer.getLoc();
    //     }
    // };

    // //------------------------------------------------------
    // // complex run on device using functor 
    // // (functor dispatch)
    // //------------------------------------------------------
    // {
    //     Node node;
    //     ExecutionAccessor<float64> acc_src(node["src"]);
    //     ExecutionAccessor<float64> acc_des(node["des"]);

    //     ExecutionPolicy policy = ExecutionPolicy::device();
    //     acc_des.use_with(policy);
    //     acc_des.use_with(policy);

    //     index_t size = acc_src.number_of_elements();

    //     ExecFunctor f();

    //     // init functor
    //     f.acc_src = acc_src;
    //     f.acc_des = acc_des;

    //     dispatch(policy,f);

    //     // get results stored in functor
    //     float64 min_val = f.min_val;
    //     index_t min_loc = f.min_loc;

    //     // sync values to node["des"], 
    //     // (no op if node["des"] was originally in
    //     //  same memory space as node["src"])
    //     acc_des.sync(node["des"]); 
    // }

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
