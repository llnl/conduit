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
float64 *
make_float64_device_buffer(const float64 *host_vals, index_t num_vals)
{
    float64 *device_ptr = static_cast<float64*>(
        execution::DeviceMemory::allocate(sizeof(float64) * num_vals));
    conduit::execution::MagicMemory::copy(device_ptr,
                                          host_vals,
                                          sizeof(float64) * num_vals);
    return device_ptr;
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
run_data_accessor_dispatch_and_sync(Node &node,
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

    // Dispatch maps a runtime policy choice to a compile-time concrete
    // execution tag object.
    conduit::execution::dispatch(policy, [&](auto exec_tag)
    {
        // recover a concrete execution tag type Exec
        using Exec = decltype(exec_tag);

        // Instantiate a reducer using compile-time Exec::reduce_policy.
        // Reducers require a compile-time concrete tag object.
        conduit::execution::ReduceMinLoc<Exec, float64>
            reducer(std::numeric_limits<float64>::max(), -1);

        // Our forall will execute in the memory space selected by the
        // requested Exec concrete tag object.
        index_t size = acc_src.number_of_elements();
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t idx)
        {
            const float64 val = 2.0 * acc_src[idx];
            reducer.minloc(val, idx);
            acc_des.set(idx, val);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        // Collect the results.
        min_val = reducer.get();
        min_loc = reducer.getLoc();
    });

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    acc_des.sync();
}

//-----------------------------------------------------------------------------
void
run_data_array_dispatch_and_sync(Node &node,
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

    // Dispatch maps a runtime policy choice to a compile-time concrete
    // execution tag object.
    conduit::execution::dispatch(policy, [&](auto exec_tag)
    {
        // recover a concrete execution tag type Exec
        using Exec = decltype(exec_tag);

        // Instantiate a reducer using compile-time Exec::reduce_policy.
        // Reducers require a compile-time concrete tag object.
        conduit::execution::ReduceMinLoc<Exec, float64>
            reducer(std::numeric_limits<float64>::max(), -1);

        // Our forall will execute in the memory space selected by the
        // requested Exec concrete tag object.
        index_t size = arr_src.number_of_elements();
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t idx)
        {
            const float64 val = 2.0 * arr_src[idx];
            reducer.minloc(val, idx);
            arr_des.set(idx, val);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        // Collect the results.
        min_val = reducer.get();
        min_loc = reducer.getLoc();
    });

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    arr_des.sync();
}

//-----------------------------------------------------------------------------
struct DataAccessorDispatchFunctor
{
    // We must provide a policy (for device error-checking).
    ExecutionPolicy policy;
    
    // We must provide DataAccessors that wrap node leaf data.
    float64_accessor acc_src;
    float64_accessor acc_des;

    // Initialize values.
    float64 min_val = 0.0;
    index_t min_loc = -1;

    template<typename Exec>
    void operator()(Exec &)
    {
        // Instantiate a reducer using compile-time Exec::reduce_policy.
        // Reducers require a compile-time concrete tag object.
        conduit::execution::ReduceMinLoc<Exec, float64>
            reducer(std::numeric_limits<float64>::max(), -1);

        // Capture device-usable view copies directly so the kernel does not
        // implicitly depend on the host-side functor object via `this`.
        // Local variables can be captured by value directly, while data members
        // are accessed through `this`. We want the kernel to depend only on the
        // small device-usable views, not the enclosing host-side functor object.
        const float64_accessor src = acc_src;
        const float64_accessor des = acc_des;

        // Our forall will execute in the memory space selected by the
        // requested Exec concrete tag object.
        index_t size = acc_src.number_of_elements();
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t idx)
        {
            const float64 val = 2.0 * src[idx];
            reducer.minloc(val, idx);
            des.set(idx, val);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        // Collect the results.
        min_val = reducer.get();
        min_loc = reducer.getLoc();
    }
};

//-----------------------------------------------------------------------------
void
run_data_accessor_dispatch_functor_and_sync(Node &node,
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

    // The functor packages the runtime-policy-dispatched execution work
    // so a concrete Exec tag can be used to instantiate a reducer and
    // launch the corresponding forall<Exec> kernel over the accessor data.
    DataAccessorDispatchFunctor func{policy, acc_src, acc_des};

    // Dispatch maps a runtime policy choice to a compile-time concrete
    // execution tag object.
    conduit::execution::dispatch(policy, func);

    // Collect the results.
    min_val = func.min_val;
    min_loc = func.min_loc;

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    acc_des.sync();
}

//-----------------------------------------------------------------------------
struct DataArrayDispatchFunctor
{
    // We must provide a policy (for device error-checking).
    ExecutionPolicy policy;

    // We must provide DataArrays that wrap node leaf data.
    float64_array arr_src;
    float64_array arr_des;

    // Initialize values.
    float64 min_val = 0.0;
    index_t min_loc = -1;

    template<typename Exec>
    void operator()(Exec &)
    {
        // Instantiate a reducer using compile-time Exec::reduce_policy.
        // Reducers require a compile-time concrete tag object.
        conduit::execution::ReduceMinLoc<Exec, float64>
            reducer(std::numeric_limits<float64>::max(), -1);

        // Capture device-usable view copies directly so the kernel does not
        // implicitly depend on the host-side functor object via `this`.
        // Local variables can be captured by value directly, while data members
        // are accessed through `this`. We want the kernel to depend only on the
        // small device-usable views, not the enclosing host-side functor object.
        const float64_array src = arr_src;
        const float64_array des = arr_des;
        
        // Our forall will execute in the memory space selected by the
        // requested Exec concrete tag object.
        index_t size = arr_src.number_of_elements();
        conduit::execution::forall<Exec>(0, size, [=] CONDUIT_EXEC(index_t idx)
        {
            const float64 val = 2.0 * src[idx];
            reducer.minloc(val, idx);
            des.set(idx, val);
        });
        CONDUIT_DEVICE_ERROR_CHECK(policy);

        // Collect the results.
        min_val = reducer.get();
        min_loc = reducer.getLoc();
    }
};

//-----------------------------------------------------------------------------
void
run_data_array_dispatch_functor_and_sync(Node &node,
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

    // The functor packages the runtime-policy-dispatched execution work
    // so a concrete Exec tag can be used to instantiate a reducer and
    // launch the corresponding forall<Exec> kernel over the array data.
    DataArrayDispatchFunctor func{policy, arr_src, arr_des};

    // Dispatch maps a runtime policy choice to a compile-time concrete
    // execution tag object.
    conduit::execution::dispatch(policy, func);

    // Collect the results.
    min_val = func.min_val;
    min_loc = func.min_loc;

    // Sync values to node["des"].
    // This is a no op if node["des"] was originally in the same memory
    // space as the requested execution policy.
    arr_des.sync();
}

#endif
