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

void *device_alloc(index_t bytes)
{
#if defined(CONDUIT_USE_RAJA)
    return execution::DeviceMemory::allocate(bytes);
#else
    return execution::HostMemory::allocate(bytes);
#endif
}

void device_free(void *ptr)
{
#if defined(CONDUIT_USE_RAJA)
    return execution::DeviceMemory::deallocate(ptr);
#else
    return execution::HostMemory::deallocate(ptr);
#endif
}

void conduit_device_prepare()
{
    execution::init_device_memory_handlers();
}

// TODO someday we want allocator to make sense for nodes when we are done with them

// TODO turn the strawman into tests?

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
    const index_t size = 10;

    index_t host_vals[size];
    index_t *dev_vals_ptr = static_cast<index_t*>(device_alloc(sizeof(index_t) * size));

    ExecutionPolicy serial = ExecutionPolicy::serial();
    conduit::execution::forall(serial, 0, size, [=](index_t i)
    {
        dev_vals_ptr[i] = i;
    });
    CONDUIT_DEVICE_ERROR_CHECK(serial);
    
    conduit::execution::MagicMemory::copy(&host_vals[0], dev_vals_ptr, sizeof(index_t) * size);

    for (index_t i = 0; i < size; i ++)
    {
        EXPECT_EQ(host_vals[i],i);
    }

    device_free(dev_vals_ptr);
}

// //-----------------------------------------------------------------------------
// TEST(conduit_execution, test_reductions)
// {
//     Conduit::execution::ExecPolicy SerialPolicy(conduit::execution::policy::Serial);
//     const index_t size = 4;
//     index_t host_vals[size] = {0,-10,10, 5};
//     index_t *dev_vals_ptr = static_cast<index_t*>(device_alloc(sizeof(index_t) * size));
//     MagicMemory::copy(dev_vals_ptr, &host_vals[0], sizeof(index_t) * size);


//     // sum
//     // ascent::ReduceSum<reduce_policy,index_t> sum_reducer;
//     using reduce_policy = typename conduit::execution::policy::Serial::reduce_policy;
//     conduit::execution::forall<reduce_policy>(0, size, [=](index_t i)
//     {
//         sum_reducer += dev_vals_ptr[i];
//     });
//     // CONDUIT_DEVICE_ERROR_CHECK();

//     EXPECT_EQ(sum_reducer.get(),5);


//     // // min
//     // ascent::ReduceMin<reduce_policy,index_t> min_reducer;
//     // conduit::execution::forall<reduce_policy>(0, size, [=](index_t i)
//     // {
//     //     min_reducer.min(dev_vals_ptr[i]);
//     // });
//     // // CONDUIT_DEVICE_ERROR_CHECK();

//     // EXPECT_EQ(min_reducer.get(),-10);

//     // // minloc
//     // ascent::ReduceMinLoc<reduce_policy,index_t> minloc_reducer;
//     // conduit::execution::forall<reduce_policy>(0, size, [=](index_t i)
//     // {
//     //     minloc_reducer.minloc(dev_vals_ptr[i],i);
//     // });
//     // // CONDUIT_DEVICE_ERROR_CHECK();

//     // EXPECT_EQ(minloc_reducer.get(),-10);
//     // EXPECT_EQ(minloc_reducer.getLoc(),1);


//     // // max
//     // ascent::ReduceMax<reduce_policy,index_t> max_reducer;
//     // conduit::execution::forall<reduce_policy>(0, size, [=](index_t i)
//     // {
//     //     max_reducer.max(dev_vals_ptr[i]);
//     // });
//     // // CONDUIT_DEVICE_ERROR_CHECK();

//     // EXPECT_EQ(max_reducer.get(),10);

//     // // maxloc
//     // ascent::ReduceMaxLoc<reduce_policy,index_t> maxloc_reducer;
//     // conduit::execution::forall<reduce_policy>(0, size, [=](index_t i)
//     // {
//     //     maxloc_reducer.maxloc(dev_vals_ptr[i],i);
//     // });
//     // // CONDUIT_DEVICE_ERROR_CHECK();

//     // EXPECT_EQ(maxloc_reducer.get(),10);
//     // EXPECT_EQ(maxloc_reducer.getLoc(),2);

//     device_free(dev_vals_ptr);
// }

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

    if (ExecutionPolicy::is_serial_enabled())
    {
        ExecutionPolicy serial = ExecutionPolicy::serial();
        test_exec_policy(serial);
    }

    if (ExecutionPolicy::is_openmp_enabled())
    {
        ExecutionPolicy openmp = ExecutionPolicy::openmp();
        test_exec_policy(openmp);
    }

    if (ExecutionPolicy::is_device_enabled())
    {
        ExecutionPolicy device = ExecutionPolicy::device();
        test_exec_policy(device);
    }
}

//-----------------------------------------------------------------------------
TEST(conduit_execution, strawman)
{
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
}
