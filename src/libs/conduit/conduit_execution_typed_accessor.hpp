// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_typed_accessor.hpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// TLDR: Trade longer compile times (11 instantiations per kernel) for faster
// array accesses + compiler vectorization.
//
// DataAccessor<T> supports data allocated on host or device memory, and accepts
// any leaf dtype and any stride. This is necessary for supporting the full menu
// of choices that Conduit makes available to users. However, safely accessing
// the underlying data requires us to compute offset + stride * idx and then
// switch on the dtype id to cast the value to T. The switch is evaluated
// per-element, even though the dtype will never change mid-kernel. The switch
// also prevents the compiler from vectorizing/coalescing the loop.
//
// The with_typed_accessor() helpers in this header exist to avoid the overhead
// of evaluating the dtype switch statement per-element. The idea is that each
// DataAccessor's dtype switch is evaluated exactly once, after which the
// caller's kernel is invoked with either:
//
//  - A typed accessor (a wrapper around a raw pointer) when the data is
//    contiguous (not strided) and the dtype is numeric, or
//  - The DataAccessor itself, unchanged, for everything else (strided data,
//    non-numeric leaves).
//
// This allows the kernel to be written once, templated over the array-like
// argument, and guarantees that it will behave identically on both paths,
// since the typed accessors apply the same conversion to T that DataAccessor
// would. This enables us to handle kernel dispatches with any dtype, any stride,
// and any offset a user provides, while automatically optimizing array
// accesses (when possible). On the mesh transform benchmarks this improves
// performance by 2-6x depending on the transform and backend.
//
// The downside of this approach is that it requires compilation of
// additional kernel instantiations (i.e., we incur extra compile time for
// each typed accessor that could be passed to a given kernel). Every numeric
// dtype has a typed accessor compiled for it, so each dispatched array
// instantiates the kernel 11 times (10 typed accessors plus the DataAccessor
// fallback). This trades a one-time compilation cost for faster execution
// regardless of which dtype
// the user provides at runtime. The two-accessor overload dispatches each
// side independently (any dtype mix, up to 11 * 11 instantiations), while the
// three-accessor overload dispatches the group through one shared case
// (11 instantiations) and falls back to the accessors when the members
// hold different dtypes, since nesting three would instantiate 11^3.
// 
// Supported cases that can be upgraded:
//
//   data passed to a kernel                       | result
//   ----------------------------------------------|----------------------
//   compact, dtype matches the accessor type      | typed accessor (fast)
//   compact, any other numeric dtype              | typed accessor (fast)
//   compact pair, dtypes differ between the two   | typed accessors (fast)
//   compact triple, all sharing one dtype         | typed accessors (fast)
//   compact triple, mixed dtypes                  | accessors (unchanged)
//   strided data                                  | accessors (unchanged)
//   non-numeric leaf                              | accessors (unchanged)
//
// It would be possible to support strided typed accessors as well, but 1)
// since the stride is a runtime value, strided accesses could not be
// vectorized the way contiguous accesses are, so most of the benefit would
// not materialize, and 2) it would add another typed accessor per dtype (21
// kernel instantiations per array instead of 11).
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_TYPED_ACCESSOR_HPP
#define CONDUIT_EXECUTION_TYPED_ACCESSOR_HPP

//-----------------------------------------------------------------------------
// -- conduit  includes --
//-----------------------------------------------------------------------------
#include "conduit_execution.hpp"
#include "conduit_data_accessor.hpp"
#include "conduit_data_type.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit:: --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::execution --
//-----------------------------------------------------------------------------
namespace execution
{

//-----------------------------------------------------------------------------
// -- begin conduit::execution::detail --
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
// A wrapper around a typed array ptr that implements the same access interface
// as DataAccessor<T>. This allows kernels to be simultaneously templated over
// either a DataAccessor<T> or a RawDataAccessor. T is the dtype that the kernel
// consumes while U is the dtype of the underlying data.
template <typename T, typename U>
struct RawDataAccessor
{
    // A raw pointer to the array data
    U *ptr;

    // Returns the element at ptr[idx], cast to T
    CONDUIT_EXEC T operator[](index_t idx) const
    {
        return static_cast<T>(ptr[idx]);
    }

    // Sets the element at ptr[idx] to value, cast to U
    CONDUIT_EXEC void set(index_t idx, T value) const
    {
        ptr[idx] = static_cast<U>(value);
    }
};

//-----------------------------------------------------------------------------
// Creates a RawDataAccessor from a DataAccessor so that the kernel can access
// its underlying data without having to re-evaluate the dtype for each access.
template <typename U, typename T>
RawDataAccessor<T, U>
make_typed_accessor(const DataAccessor<T> &acc)
{
    return RawDataAccessor<T, U>{
        static_cast<U*>(const_cast<void*>(acc.element_ptr(0)))
    };
}

//-----------------------------------------------------------------------------
// A helper that returns true when the array's elements are contiguous, which
// implies that a typed accessor can be used instead of the DataAccessor.
inline bool
is_compact_layout(const DataType &dt)
{
    return dt.stride() == dt.element_bytes();
}

//-----------------------------------------------------------------------------
// Returns true when 1) every DataAccessor in a group is compact and 2) they
// all share one dtype, implying that a single typed kernel instantiation can
// serve them all.
//-----------------------------------------------------------------------------
template <typename T>
bool
group_is_uniform(const DataAccessor<T> &acc0,
                 const DataAccessor<T> &acc1,
                 const DataAccessor<T> &acc2)
{
    return is_compact_layout(acc0.dtype()) &&
           is_compact_layout(acc1.dtype()) &&
           is_compact_layout(acc2.dtype()) &&
           acc0.dtype().id() == acc1.dtype().id() &&
           acc0.dtype().id() == acc2.dtype().id();
}

//-----------------------------------------------------------------------------
// Invokes the kernel with typed accessors for a group of DataAccessors that
// group_is_uniform() accepted. One dtype case serves the whole group, so
// the number of kernel instantiations does not depend on the group size.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
bool
try_group_typed_accessor(const DataAccessor<T> &acc0,
                         const DataAccessor<T> &acc1,
                         const DataAccessor<T> &acc2,
                         Kernel &&kernel)
{
    switch(acc0.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(make_typed_accessor<int8>(acc0),
                   make_typed_accessor<int8>(acc1),
                   make_typed_accessor<int8>(acc2));
            return true;
        case DataType::INT16_ID:
            kernel(make_typed_accessor<int16>(acc0),
                   make_typed_accessor<int16>(acc1),
                   make_typed_accessor<int16>(acc2));
            return true;
        case DataType::INT32_ID:
            kernel(make_typed_accessor<int32>(acc0),
                   make_typed_accessor<int32>(acc1),
                   make_typed_accessor<int32>(acc2));
            return true;
        case DataType::INT64_ID:
            kernel(make_typed_accessor<int64>(acc0),
                   make_typed_accessor<int64>(acc1),
                   make_typed_accessor<int64>(acc2));
            return true;
        case DataType::UINT8_ID:
            kernel(make_typed_accessor<uint8>(acc0),
                   make_typed_accessor<uint8>(acc1),
                   make_typed_accessor<uint8>(acc2));
            return true;
        case DataType::UINT16_ID:
            kernel(make_typed_accessor<uint16>(acc0),
                   make_typed_accessor<uint16>(acc1),
                   make_typed_accessor<uint16>(acc2));
            return true;
        case DataType::UINT32_ID:
            kernel(make_typed_accessor<uint32>(acc0),
                   make_typed_accessor<uint32>(acc1),
                   make_typed_accessor<uint32>(acc2));
            return true;
        case DataType::UINT64_ID:
            kernel(make_typed_accessor<uint64>(acc0),
                   make_typed_accessor<uint64>(acc1),
                   make_typed_accessor<uint64>(acc2));
            return true;
        case DataType::FLOAT32_ID:
            kernel(make_typed_accessor<float32>(acc0),
                   make_typed_accessor<float32>(acc1),
                   make_typed_accessor<float32>(acc2));
            return true;
        case DataType::FLOAT64_ID:
            kernel(make_typed_accessor<float64>(acc0),
                   make_typed_accessor<float64>(acc1),
                   make_typed_accessor<float64>(acc2));
            return true;
        default:
            return false;
    }
}

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed accessor of the underlying data's dtype.
// Returns false when the dtype is not supported (e.g., non-numeric), in which
// case the caller falls back to invoking the kernel with the DataAccessor.
template <typename T, typename Kernel>
bool
try_typed_accessor(const DataAccessor<T> &acc, Kernel &&kernel)
{
    switch(acc.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(make_typed_accessor<int8>(acc));
            return true;
        case DataType::INT16_ID:
            kernel(make_typed_accessor<int16>(acc));
            return true;
        case DataType::INT32_ID:
            kernel(make_typed_accessor<int32>(acc));
            return true;
        case DataType::INT64_ID:
            kernel(make_typed_accessor<int64>(acc));
            return true;
        case DataType::UINT8_ID:
            kernel(make_typed_accessor<uint8>(acc));
            return true;
        case DataType::UINT16_ID:
            kernel(make_typed_accessor<uint16>(acc));
            return true;
        case DataType::UINT32_ID:
            kernel(make_typed_accessor<uint32>(acc));
            return true;
        case DataType::UINT64_ID:
            kernel(make_typed_accessor<uint64>(acc));
            return true;
        case DataType::FLOAT32_ID:
            kernel(make_typed_accessor<float32>(acc));
            return true;
        case DataType::FLOAT64_ID:
            kernel(make_typed_accessor<float64>(acc));
            return true;
        default:
            return false;
    }
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//
// The following helpers exist to accelerate existing use cases found
// throughout the codebase. Additional overloads (e.g., larger group sizes)
// could be added later if we find new use cases that aren't covered by the
// existing overloads.
//

//-----------------------------------------------------------------------------
// with_typed_accessor works by trying to invoke the kernel functor with a raw,
// typed accessor to the underlying data. If the necessary conditions are met,
// the kernel is invoked with a typed accessor, which 1) avoids the per-access
// overhead of DataAccessors and 2) gives the compiler an opportunity to
// vectorize. If the conditions are not met, the kernel is invoked with the
// DataAccessor itself, which will function identically but incur per-access
// overhead. Because both types of accessor provide the same interface for
// reading and writing data, existing kernel code can automatically be upgraded
// to use the faster accessor when the conditions are met. The downside is that
// each kernel has to be instantiated and compiled 11 times (once with the
// DataAccessor and once for each supported dtype).
template <typename T, typename Kernel>
void
with_typed_accessor(const DataAccessor<T> &acc, Kernel &&kernel)
{
    if (detail::is_compact_layout(acc.dtype()) &&
        detail::try_typed_accessor(acc, kernel))
    {
        // The conditions were met to invoke the kernel with a typed accessor
        return;
    }
    // The conditions were not met to invoke the kernel with a typed accessor,
    // so we directly invoke it with the provided DataAccessor instead.
    kernel(acc);
}

//-----------------------------------------------------------------------------
// A special case of with_typed_accessor that takes two DataAccessors. Each
// side is dispatched independently, so the kernel can be upgraded to use typed
// accessors even if the accessor types and underlying dtypes differ.
template <typename T0, typename T1, typename Kernel>
void
with_typed_accessor(const DataAccessor<T0> &acc0,
                    const DataAccessor<T1> &acc1,
                    Kernel &&kernel)
{
    // For future reference, one could nest these an arbitrary number of times
    // to support executing kernels with more than two accessors of different
    // dtypes.
    with_typed_accessor(acc0, [&](auto vals0)
    {
        with_typed_accessor(acc1, [&](auto vals1)
        {
            kernel(vals0, vals1);
        });
    });
}

//-----------------------------------------------------------------------------
// A special case of with_typed_accessor that takes three DataAccessors of the
// same dtype. It is possible to template this for 3 different dtypes like in
// the previous helper, but that would require 11^3 instantiations of the
// kernel (one for each combination of dtypes and fallbacks). That decision
// could be revisited if we later find that kernel dispatches with 3 accessors
// of mixed dtypes are more common than expected.
template <typename T, typename Kernel>
void
with_typed_accessor(const DataAccessor<T> &acc0,
                    const DataAccessor<T> &acc1,
                    const DataAccessor<T> &acc2,
                    Kernel &&kernel)
{
    if (detail::group_is_uniform(acc0, acc1, acc2) &&
        detail::try_group_typed_accessor(acc0, acc1, acc2, kernel))
    {
        // The conditions were met to invoke the kernel with typed accessors
        return;
    }
    // The conditions were not met to invoke the kernel with typed accessors,
    // so we directly invoke it with the provided DataAccessors instead.
    kernel(acc0, acc1, acc2);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif
