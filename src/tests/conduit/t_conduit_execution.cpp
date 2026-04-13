// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: t_conduit_execution.cpp
///
//-----------------------------------------------------------------------------

#include "conduit.hpp"
#include "conduit_execution.hpp"
#include "conduit_memory_manager.hpp"

#include <iostream>
#include <type_traits>
#include "gtest/gtest.h"

using namespace conduit;
using conduit::execution::ExecutionPolicy;

//-----------------------------------------------------------------------------
void *
allocate_for_policy(ExecutionPolicy policy, index_t bytes)
{
    if (policy.is_device_policy())
    {
        return execution::DeviceMemory::allocate(bytes);
    }

    return execution::HostMemory::allocate(bytes);
}

//-----------------------------------------------------------------------------
void
free_for_policy(ExecutionPolicy policy, void *ptr)
{
    if (policy.is_device_policy())
    {
        execution::DeviceMemory::deallocate(ptr);
        return;
    }

    execution::HostMemory::deallocate(ptr);
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

    if (ExecutionPolicy::is_openmp_enabled())
    {
        ExecutionPolicy openmp = ExecutionPolicy::openmp();
        func(openmp);
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        ExecutionPolicy device = ExecutionPolicy::device();
        func(device);
    }
}

//-----------------------------------------------------------------------------
void
conduit_device_prepare()
{
    execution::init_device_memory_handlers();
}

// TODO someday we want allocator to make sense for nodes when we are done with them

//---------------------------------------------------------------------------//
// example functor 
//---------------------------------------------------------------------------//
struct MyFunctor
{
    int res;
    int size;
    template<typename ComboPolicyTag>
    void operator()(ComboPolicyTag &exec)
    {
        (void)exec;
        res = 0;
        conduit::execution::forall<ComboPolicyTag>(0, size, [] EXEC_LAMBDA (int i)
        {
            (void)i;
        });
        res = size;
    }
};

//---------------------------------------------------------------------------//
// Mock of a class templated on a concrete tag
// (like a RAJA Reduction Object)
//---------------------------------------------------------------------------//
template <typename ExecPolicy>
class MySpecialClass
{
public:
    using policy = ExecPolicy;
    int val;

    MySpecialClass(int _val)
    :val(_val)
    {}
    
    EXEC_LAMBDA int exec(int i) const
    {
        return val + i;
    }
};

//---------------------------------------------------------------------------//
// example functor using MySpecialClass
//---------------------------------------------------------------------------//
struct MySpecialFunctor
{
    int res;
    int size;
    template<typename ComboPolicyTag>
    void operator()(ComboPolicyTag &exec)
    {
        (void)exec;
        // in this case we use an object
        // that is templated on a concrete tag
        // (like a RAJA Reduction Object)
        using for_policy = typename ComboPolicyTag::for_policy;
        res = 0;
        MySpecialClass<for_policy> s(10);
        conduit::execution::forall<ComboPolicyTag>(0, size, [=] EXEC_LAMBDA (int i)
        {
            const int value = s.exec(i);
            (void)value;
        });
        res = size;
    }
};

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_forall)
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        const index_t size = 10;

        index_t host_vals[size];
        index_t *vals_ptr =
            static_cast<index_t*>(allocate_for_policy(policy,
                                                      sizeof(index_t) * size));

        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
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

        free_for_policy(policy, vals_ptr);
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_reductions)
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        const index_t size = 4;
        index_t host_vals[size] = {0, -10, 10, 5};
        index_t *vals_ptr =
            static_cast<index_t*>(allocate_for_policy(policy,
                                                      sizeof(index_t) * size));
        conduit::execution::MagicMemory::copy(vals_ptr,
                                              &host_vals[0],
                                              sizeof(index_t) * size);

        conduit::execution::ReduceSum<index_t> sum_reducer(policy);
        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            sum_reducer += vals_ptr[i];
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        EXPECT_EQ(sum_reducer.get(), 5);

        conduit::execution::ReduceMin<index_t> min_reducer(policy);
        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            min_reducer.min(vals_ptr[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        EXPECT_EQ(min_reducer.get(), -10);

        conduit::execution::ReduceMinLoc<index_t> minloc_reducer(policy);
        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            minloc_reducer.minloc(vals_ptr[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        EXPECT_EQ(minloc_reducer.get(), -10);
        EXPECT_EQ(minloc_reducer.getLoc(), 1);

        conduit::execution::ReduceMax<index_t> max_reducer(policy);
        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            max_reducer.max(vals_ptr[i]);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        EXPECT_EQ(max_reducer.get(), 10);

        conduit::execution::ReduceMaxLoc<index_t> maxloc_reducer(policy);
        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            maxloc_reducer.maxloc(vals_ptr[i], i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);
        EXPECT_EQ(maxloc_reducer.get(), 10);
        EXPECT_EQ(maxloc_reducer.getLoc(), 2);

        free_for_policy(policy, vals_ptr);
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, test_atomics)
{
    conduit_device_prepare();
    for_each_enabled_policy([](ExecutionPolicy policy)
    {
        const index_t size = 4;
        index_t host_vals[size] = {0, -1, -2, -3};
        index_t *vals_ptr =
            static_cast<index_t*>(allocate_for_policy(policy,
                                                      sizeof(index_t) * size));

        conduit::execution::MagicMemory::copy(vals_ptr,
                                              &host_vals[0],
                                              sizeof(index_t) * size);

        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            conduit::execution::atomic_add(policy, vals_ptr + i, i);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        conduit::execution::MagicMemory::copy(&host_vals[0],
                                              vals_ptr,
                                              sizeof(index_t) * size);
        for (index_t i = 0; i < size; i++)
        {
            EXPECT_EQ(host_vals[i], 0);
        }

        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            conduit::execution::atomic_min(policy,
                                           vals_ptr + i,
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

        conduit::execution::forall(policy, 0, size, [=] EXEC_LAMBDA(index_t i)
        {
            conduit::execution::atomic_max(policy,
                                           vals_ptr + i,
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

        free_for_policy(policy, vals_ptr);
    });
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, for_all_and_dispatch)
{
    std::cout << "forall cases!" << std::endl;

    const int size = 4;
    MyFunctor func;
    func.size = size;
    MySpecialFunctor sfunc;
    sfunc.size = 4;

    auto test_exec_policy = [&](ExecutionPolicy policy)
    {
        int *vals_ptr = nullptr;
        if (policy.is_device_policy())
        {
            vals_ptr = static_cast<int*>(execution::DeviceMemory::allocate(sizeof(int) * size));
        }
        else
        {
            vals_ptr = static_cast<int*>(execution::HostMemory::allocate(sizeof(int) * size));
        }

        // Use runtime forall(policy, ...) when one portable kernel works for
        // every backend that may be selected in this translation unit.
        auto portable_kernel = [=] EXEC_LAMBDA (int i)
        {
            vals_ptr[i] = i;
        };
        conduit::execution::forall(policy, 0, size, portable_kernel);
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        int host_vals[size];
        conduit::execution::MagicMemory::copy(host_vals, vals_ptr, sizeof(int) * size);

        for (int i = 0; i < size; i++)
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

        conduit::execution::dispatch(policy, func);
        EXPECT_EQ(func.res, size);

        conduit::execution::dispatch(policy, sfunc);
        EXPECT_EQ(sfunc.res, size);

        // Use dispatch(policy, ...) when the concrete execution tag is needed
        // to instantiate backend-specific helper types or call forall<Tag>(...).
        int res = 0;
        auto concrete_kernel = [&](auto &exec)
        {
            using combo_policy = typename std::decay<decltype(exec)>::type;
            using for_policy = typename combo_policy::for_policy;
            MySpecialClass<for_policy> s(10);
            conduit::execution::forall<combo_policy>(0, size, [=] EXEC_LAMBDA (int i)
            {
                const int value = s.exec(i);
                (void)value;
            });
            res = 10;
        };
        conduit::execution::dispatch(policy, concrete_kernel);

        EXPECT_EQ(res, 10);
    };

    for_each_enabled_policy(test_exec_policy);
}

//-----------------------------------------------------------------------------
float64 *
strawman_make_device_buffer(const float64 *host_vals, index_t num_vals)
{
    float64 *device_ptr = static_cast<float64*>(
        execution::DeviceMemory::allocate(sizeof(float64) * num_vals));
    conduit::execution::MagicMemory::copy(device_ptr,
                                          host_vals,
                                          sizeof(float64) * num_vals);
    return device_ptr;
}

//-----------------------------------------------------------------------------
void
strawman_run_policy_and_sync(Node &node,
                             ExecutionPolicy policy,
                             const bool expect_src_device_backed_result,
                             const bool expect_des_device_backed_result)
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
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] EXEC_LAMBDA(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    acc_des.sync();

    if (expect_src_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }

    if (expect_des_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
}

//-----------------------------------------------------------------------------
void
strawman_run_policy_and_assume(Node &node,
                               ExecutionPolicy policy,
                               const bool expect_src_device_backed_result,
                               const bool expect_des_device_backed_result)
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
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] EXEC_LAMBDA(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // node["des"] takes ownership of the data in the active execution
    // space. This is a no op if node["des"] was already in that space.
    acc_des.assume();

    if (expect_src_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }

    if (expect_des_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
}

//-----------------------------------------------------------------------------
void
strawman_run_where_src_is(Node &node,
                          const bool expect_device_policy,
                          const bool expect_src_device_backed_result,
                          const bool expect_des_device_backed_result)
{
    // DataAccessors wrap node leaf data.
    float64_accessor acc_src(node["src"]);
    float64_accessor acc_des(node["des"]);

    // Use the location of the source data.
    ExecutionPolicy policy = acc_src.active_space();
    if (expect_device_policy)
    {
        EXPECT_TRUE(policy.is_device_policy());
    }
    else
    {
        EXPECT_TRUE(policy.is_host_policy());
    }

    // Ask the accessors to move their data to the memory space occupied
    // by node["src"] if their data is not already there.
    acc_src.use_with(policy);
    acc_des.use_with(policy);

    // Our forall will execute on the memory space occupied by node["src"]
    // because it was passed an ExecutionPolicy for that space.
    index_t size = acc_src.number_of_elements();
    conduit::execution::forall(policy, 0, size, [acc_src, acc_des] EXEC_LAMBDA(index_t idx)
    {
        const float64 val = 2.0 * acc_src[idx];
        acc_des.set(idx, val);
    });
    CONDUIT_DEVICE_ERROR_CHECK(policy);

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as node["src"].
    acc_des.sync();

    if (expect_src_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["src"].data_ptr()));
    }

    if (expect_des_device_backed_result)
    {
        EXPECT_TRUE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
    else
    {
        EXPECT_FALSE(execution::DeviceMemory::is_device_ptr(node["des"].data_ptr()));
    }
}

//-----------------------------------------------------------------------------
void
strawman_expect_doubled_des(Node &node)
{
    float64_accessor result_acc(node["des"]);
    // Verification runs on the host, so use a host execution policy
    // in case node["des"] still owns device-backed data here.
    result_acc.use_with(ExecutionPolicy::host());
    EXPECT_EQ(result_acc.number_of_elements(), 4);
    EXPECT_EQ(result_acc[0], 2.0);
    EXPECT_EQ(result_acc[1], 4.0);
    EXPECT_EQ(result_acc[2], 6.0);
    EXPECT_EQ(result_acc[3], 8.0);
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_src_host_des_host)
{
    conduit_device_prepare();
    const float64 src_vals[4] = {1.0, 2.0, 3.0, 4.0};
    const float64 des_vals[4] = {0.0, 0.0, 0.0, 0.0};

    // Run with an explicit host execution policy.
    // node["des"] is synced back to host memory.
    {
        std::cout << "sync policy=host src_start=host des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        node["src"].set(src_vals, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::host(), false, false);
        strawman_expect_doubled_des(node);
        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        // Run with an explicit device execution policy.
        // node["des"] is synced back to host memory.
        std::cout << "sync policy=device src_start=host des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        node["src"].set(src_vals, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::device(), false, false);
        strawman_expect_doubled_des(node);
        node.reset();
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        std::cout << "assume policy=host src_start=host des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        node["src"].set(src_vals, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::host(), false, false);
        strawman_expect_doubled_des(node);
        node.reset();
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        // Run with an explicit device execution policy and call assume().
        // node["des"] keeps the device-backed result buffer.
        std::cout << "assume policy=device src_start=host des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        node["src"].set(src_vals, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::device(), false, true);
        strawman_expect_doubled_des(node);
        node.reset();
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to host memory.
    {
        std::cout << "active_space src_start=host des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        node["src"].set(src_vals, 4);
        node["des"].set(des_vals, 4);
        strawman_run_where_src_is(node, false, false, false);
        strawman_expect_doubled_des(node);
        node.reset();
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_src_device_des_device)
{
    conduit_device_prepare();
    const float64 src_vals[4] = {1.0, 2.0, 3.0, 4.0};
    const float64 des_vals[4] = {0.0, 0.0, 0.0, 0.0};

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to device memory.
    {
        std::cout << "sync policy=host src_start=device des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::host(), true, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to device memory.
    {
        std::cout << "sync policy=device src_start=device des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::device(), true, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        std::cout << "assume policy=host src_start=device des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::host(), true, false);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        std::cout << "assume policy=device src_start=device des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::device(), true, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to device memory.
    {
        std::cout << "active_space src_start=device des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_where_src_is(node, true, true, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
        execution::DeviceMemory::deallocate(des_device_ptr);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_src_host_des_device)
{
    conduit_device_prepare();
    const float64 src_vals[4] = {1.0, 2.0, 3.0, 4.0};
    const float64 des_vals[4] = {0.0, 0.0, 0.0, 0.0};

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to device memory.
    {
        std::cout << "sync policy=host src_start=host des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set(src_vals, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::host(), false, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to device memory.
    {
        std::cout << "sync policy=device src_start=host des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set(src_vals, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::device(), false, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        std::cout << "assume policy=host src_start=host des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set(src_vals, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::host(), false, false);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        std::cout << "assume policy=device src_start=host des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set(src_vals, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::device(), false, true);
        strawman_expect_doubled_des(node);
        node.reset();
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to device memory.
    {
        std::cout << "active_space src_start=host des_start=device" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        des_device_ptr = strawman_make_device_buffer(des_vals, 4);
        node["src"].set(src_vals, 4);
        node["des"].set_external(des_device_ptr, 4);
        strawman_run_where_src_is(node, false, false, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(des_device_ptr);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman_src_device_des_host)
{
    conduit_device_prepare();
    const float64 src_vals[4] = {1.0, 2.0, 3.0, 4.0};
    const float64 des_vals[4] = {0.0, 0.0, 0.0, 0.0};

    if (!ExecutionPolicy::is_device_enabled())
    {
        return;
    }

    // Run with an explicit host execution policy.
    // node["des"] is synced back to host memory.
    {
        std::cout << "sync policy=host src_start=device des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::host(), true, false);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit device execution policy.
    // node["des"] is synced back to host memory.
    {
        std::cout << "sync policy=device src_start=device des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_sync(node, ExecutionPolicy::device(), true, false);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit host execution policy and call assume().
    // node["des"] keeps the host-backed result buffer.
    {
        std::cout << "assume policy=host src_start=device des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::host(), true, false);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Run with an explicit device execution policy and call assume().
    // node["des"] keeps the device-backed result buffer.
    {
        std::cout << "assume policy=device src_start=device des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set(des_vals, 4);
        strawman_run_policy_and_assume(node, ExecutionPolicy::device(), true, true);
        strawman_expect_doubled_des(node);
        node.reset();
        execution::DeviceMemory::deallocate(src_device_ptr);
    }

    // Use the location of node["src"] to choose where to execute.
    // node["des"] is then synced back to host memory.
    {
        std::cout << "active_space src_start=device des_start=host" << std::endl;

        float64 *src_device_ptr = nullptr;
        float64 *des_device_ptr = nullptr;
        Node node;
        src_device_ptr = strawman_make_device_buffer(src_vals, 4);
        node["src"].set_external(src_device_ptr, 4);
        node["des"].set(des_vals, 4);
        strawman_run_where_src_is(node, true, true, false);
        strawman_expect_doubled_des(node);
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

    //     forall(policy, 0, size, [=] EXEC_LAMBDA(index_t idx)
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

    //     forall(policy, 0, size, [=] EXEC_LAMBDA(index_t idx)
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

    //     forall(policy, 0, size, [=] EXEC_LAMBDA(index_t idx)
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

    //         forall<for_policy>(0, size, [=] EXEC_LAMBDA (int i)
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
