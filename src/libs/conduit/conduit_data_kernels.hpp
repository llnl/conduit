// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_data_kernels.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_DATA_KERNELS_HPP
#define CONDUIT_DATA_KERNELS_HPP

//-----------------------------------------------------------------------------
// These are shared kernels that implement the DataArray and DataAccessor APIs.
//-----------------------------------------------------------------------------

#include "conduit_execution.hpp"
#include "conduit_data_accessor.hpp"
#include "conduit_data_array.hpp"

#include <limits>

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// The friend of DataAccessor that lets kernels write through the private
// set_value_helper()
struct DataAccessorKernelAccess
{
    template <typename T>
    static CONDUIT_EXEC void set(const DataAccessor<T> &vals,
                                 index_t i,
                                 T value)
    {
        vals.set_value_helper(i, value);
    }
};

//-----------------------------------------------------------------------------
// -- begin conduit::detail --
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
// Returns the fastest policy for a bulk operation of the given size.
// Parallel operations on host-resident data with fewer than
// CONDUIT_SMALL_N_THRESHOLD elements are faster with serial execution due to
// the overhead associated with warming up and managing OpenMP threads.
inline execution::ExecutionPolicy
select_policy(execution::ExecutionPolicy policy, index_t num_elements)
{
    if (!policy.is_device_policy() && num_elements < CONDUIT_SMALL_N_THRESHOLD)
    {
        return execution::ExecutionPolicy::serial();
    }
    return policy;
}

//-----------------------------------------------------------------------------
// These kernels use functor structs rather than lambdas for two reasons:
// First, the structs seem to compile a bit faster under nvcc. Second, this
// header is included by conduit_data_array and conduit_data_accessor, so that
// these kernel instantiations can be reused in both. That reuse is unsafe
// with lambdas because nvcc registers extended lambdas per translation unit,
// so a deduplicated lambda instantiation can end up referencing a discarded
// translation unit's registration table and crash at runtime.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct FillOp
{
    U vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, value);
    }
};

//-----------------------------------------------------------------------------
template <typename T>
struct FillArrayOp
{
    DataArray<T> vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.element(i) = value;
    }
};

//-----------------------------------------------------------------------------
template <typename T>
struct FillAccessorOp
{
    DataAccessor<T> vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        DataAccessorKernelAccess::set(vals, i, value);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromPtrOp
{
    T vals;
    const U *src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromPtrArrayOp
{
    DataArray<T> vals;
    const U *src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.element(i) = static_cast<T>(src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromPtrAccessorOp
{
    DataAccessor<T> vals;
    const U *src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        DataAccessorKernelAccess::set(vals, i, static_cast<T>(src[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromAccOp
{
    T vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.set(i, src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromAccArrayOp
{
    DataArray<T> vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        vals.element(i) = static_cast<T>(src[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CopyFromAccAccessorOp
{
    DataAccessor<T> vals;
    U src;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        DataAccessorKernelAccess::set(vals, i, static_cast<T>(src[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct MinOp
{
    execution::ReduceMin<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer.min(static_cast<T>(vals[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct MaxOp
{
    execution::ReduceMax<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer.max(static_cast<T>(vals[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct SumOp
{
    execution::ReduceSum<T> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer += static_cast<T>(vals[i]);
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct MeanOp
{
    execution::ReduceSum<float64> reducer;
    U vals;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer += static_cast<float64>(static_cast<T>(vals[i]));
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct CountOp
{
    execution::ReduceSum<index_t> reducer;
    U vals;
    T value;

    CONDUIT_EXEC void operator()(index_t i) const
    {
        reducer += (static_cast<T>(vals[i]) == value) ? 1 : 0;
    }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
fill_kernel(execution::ExecutionPolicy &policy,
            index_t num_elements,
            const U vals,
            const T value)
{
    execution::forall(policy, 0, num_elements, FillOp<T, U>{vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T>
void
fill_kernel(execution::ExecutionPolicy &policy,
            index_t num_elements,
            const DataArray<T> vals,
            const T value)
{
    execution::forall(policy, 0, num_elements, FillArrayOp<T>{vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T>
void
fill_kernel(execution::ExecutionPolicy &policy,
            index_t num_elements,
            const DataAccessor<T> vals,
            const T value)
{
    execution::forall(policy, 0, num_elements, FillAccessorOp<T>{vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_ptr_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U *src,
                     const T vals)
{
    execution::forall(policy, 0, num_elements, CopyFromPtrOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_ptr_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U *src,
                     const DataArray<T> vals)
{
    execution::forall(policy, 0, num_elements, CopyFromPtrArrayOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_ptr_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U *src,
                     const DataAccessor<T> vals)
{
    execution::forall(policy, 0, num_elements, CopyFromPtrAccessorOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_acc_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U src,
                     const T vals)
{
    execution::forall(policy, 0, num_elements, CopyFromAccOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_acc_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U src,
                     const DataArray<T> vals)
{
    execution::forall(policy, 0, num_elements, CopyFromAccArrayOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
void
copy_from_acc_kernel(execution::ExecutionPolicy &policy,
                     index_t num_elements,
                     const U src,
                     const DataAccessor<T> vals)
{
    execution::forall(policy, 0, num_elements, CopyFromAccAccessorOp<T, U>{vals, src});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
min_kernel(execution::ExecutionPolicy &policy,
           index_t num_elements,
           const U vals)
{
    execution::ReduceMin<T> reducer(std::numeric_limits<T>::max());
    execution::forall(policy, 0, num_elements, MinOp<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
max_kernel(execution::ExecutionPolicy &policy,
           index_t num_elements,
           const U vals)
{
    execution::ReduceMax<T> reducer(std::numeric_limits<T>::lowest());
    execution::forall(policy, 0, num_elements, MaxOp<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
T
sum_kernel(execution::ExecutionPolicy &policy,
           index_t num_elements,
           const U vals)
{
    execution::ReduceSum<T> reducer(static_cast<T>(0));
    execution::forall(policy, 0, num_elements, SumOp<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
float64
mean_kernel(execution::ExecutionPolicy &policy,
            index_t num_elements,
            const U vals)
{
    execution::ReduceSum<float64> reducer(0.0);
    execution::forall(policy, 0, num_elements, MeanOp<T, U>{reducer, vals});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

//-----------------------------------------------------------------------------
template <typename T, typename U>
index_t
count_kernel(execution::ExecutionPolicy &policy,
             index_t num_elements,
             const U vals,
             const T value)
{
    execution::ReduceSum<index_t> reducer(0);
    execution::forall(policy, 0, num_elements, CountOp<T, U>{reducer, vals, value});
    CONDUIT_DEVICE_ERROR_CHECK(policy);
    return reducer.get();
}

}
//-----------------------------------------------------------------------------
// -- end conduit::detail --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif // CONDUIT_DATA_KERNELS_HPP
