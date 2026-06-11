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
// Standalone kernel to bypass NVCC / GTest private access restrictions
//-----------------------------------------------------------------------------
void run_test_forall_kernel(ExecutionPolicy policy, index_t size, index_t* vals_ptr)
{
    conduit::execution::forall(policy, 0, size, [=] CONDUIT_EXEC(index_t i)
    {
        vals_ptr[i] = i;
    });
}

//-----------------------------------------------------------------------------
// Functors to bypass NVCC's generic lambda ("auto") restrictions inside dispatch
//-----------------------------------------------------------------------------

struct RunReduceSum {
    ExecutionPolicy policy;
    index_t size;
    index_t* vals_ptr;
    index_t result = 0;
    template<typename Exec> void operator()(Exec&) {
        conduit::execution::ReduceSum<Exec, index_t> reducer(0);
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            reducer += d_vals[i];
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        result = reducer.get();
    }
};

struct RunReduceMin {
    ExecutionPolicy policy;
    index_t size;
    index_t* vals_ptr;
    index_t result = 0;
    template<typename Exec> void operator()(Exec&) {
        conduit::execution::ReduceMin<Exec, index_t> reducer(std::numeric_limits<index_t>::max());
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            reducer.min(d_vals[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        result = reducer.get();
    }
};

struct RunReduceMinLoc {
    ExecutionPolicy policy;
    index_t size;
    index_t* vals_ptr;
    index_t result = 0;
    index_t loc = -1;
    template<typename Exec> void operator()(Exec&) {
        conduit::execution::ReduceMinLoc<Exec, index_t> reducer(std::numeric_limits<index_t>::max(), -1);
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            reducer.minloc(d_vals[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        result = reducer.get();
        loc = reducer.getLoc();
    }
};

struct RunReduceMax {
    ExecutionPolicy policy;
    index_t size;
    index_t* vals_ptr;
    index_t result = 0;
    template<typename Exec> void operator()(Exec&) {
        conduit::execution::ReduceMax<Exec, index_t> reducer(std::numeric_limits<index_t>::lowest());
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            reducer.max(d_vals[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        result = reducer.get();
    }
};

struct RunReduceMaxLoc {
    ExecutionPolicy policy;
    index_t size;
    index_t* vals_ptr;
    index_t result = 0;
    index_t loc = -1;
    template<typename Exec> void operator()(Exec&) {
        conduit::execution::ReduceMaxLoc<Exec, index_t> reducer(std::numeric_limits<index_t>::lowest(), -1);
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            reducer.maxloc(d_vals[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        result = reducer.get();
        loc = reducer.getLoc();
    }
};

struct RunAtomicAdd {
    index_t size;
    index_t* vals_ptr;
    template<typename Exec> void operator()(Exec&) {
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            conduit::execution::atomic_add<Exec>(d_vals + i, i);
        });
    }
};

struct RunAtomicMin {
    index_t size;
    index_t* vals_ptr;
    template<typename Exec> void operator()(Exec&) {
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            conduit::execution::atomic_min<Exec>(d_vals + i, static_cast<index_t>(-10));
        });
    }
};

struct RunAtomicMax {
    index_t size;
    index_t* vals_ptr;
    template<typename Exec> void operator()(Exec&) {
        index_t* d_vals = vals_ptr;
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t i) {
            conduit::execution::atomic_max<Exec>(d_vals + i, static_cast<index_t>(10));
        });
    }
};

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

        run_test_forall_kernel(policy, size, vals_ptr);
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

        RunReduceSum run_sum{policy, size, vals_ptr, 0};
        conduit::execution::dispatch(policy, run_sum);
        EXPECT_EQ(run_sum.result, 5);

        RunReduceMin run_min{policy, size, vals_ptr, 0};
        conduit::execution::dispatch(policy, run_min);
        EXPECT_EQ(run_min.result, -10);

        RunReduceMinLoc run_minloc{policy, size, vals_ptr, 0, -1};
        conduit::execution::dispatch(policy, run_minloc);
        EXPECT_EQ(run_minloc.result, -10);
        EXPECT_EQ(run_minloc.loc, 1);

        RunReduceMax run_max{policy, size, vals_ptr, 0};
        conduit::execution::dispatch(policy, run_max);
        EXPECT_EQ(run_max.result, 10);

        RunReduceMaxLoc run_maxloc{policy, size, vals_ptr, 0, -1};
        conduit::execution::dispatch(policy, run_maxloc);
        EXPECT_EQ(run_maxloc.result, 10);
        EXPECT_EQ(run_maxloc.loc, 2);

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

        RunAtomicAdd run_add{size, vals_ptr};
        conduit::execution::dispatch(policy, run_add);
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], 0);
        }

        RunAtomicMin run_min{size, vals_ptr};
        conduit::execution::dispatch(policy, run_min);
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], -10);
        }

        RunAtomicMax run_max{size, vals_ptr};
        conduit::execution::dispatch(policy, run_max);
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
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
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
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
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
                (ExecutionPolicy::is_device_enabled()))
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
            // DataAccessor dispatch lambda
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataAccessor dispatch() lambda:\n" <<
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
                EXPECT_EQ(result_acc.number_of_elements(), EXECUTION_TEST_ARRAY_SIZE);
                for (index_t i = 0; i < EXECUTION_TEST_ARRAY_SIZE; i ++)
                {
                    EXPECT_EQ(result_acc[i], 2.0 * static_cast<float64>(i + 1));
                }

                node.reset();
            }

            //----------------------------------------------------------
            // DataAccessor dispatch functor
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataAccessor dispatch() functor:\n" <<
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

    for (const std::string &src_start : src_locations)
    {
        for (const std::string &des_start : des_locations)
        {
            //----------------------------------------------------------
            // DataArray sync
            // Run with an explicit execution policy and call sync().
            // node["des"] is synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
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
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
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
                (ExecutionPolicy::is_device_enabled()))
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
            // DataArray dispatch lambda
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataArray dispatch() lambda:\n" <<
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
                run_data_array_dispatch_and_sync(node, policy, min_val, min_loc);

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

            //----------------------------------------------------------
            // DataArray dispatch functor
            // Run with an explicit execution policy and call sync().
            // node["des"] is then synced back to where it started.
            //----------------------------------------------------------
            for (const std::string &policy_str : policies)
            {
                if (policy_str == "device" && ! ExecutionPolicy::is_device_enabled())
                {
                    continue;
                }

                Node exec_opts;
                exec_opts["execution_policy"].set(policy_str);
                conduit::execution::execution_set_options(exec_opts);

                CONDUIT_INFO("DataArray dispatch() functor:\n" <<
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
                run_data_array_dispatch_and_sync(node, policy, min_val, min_loc);

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
