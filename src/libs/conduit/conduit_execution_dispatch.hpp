// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_dispatch.hpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// TLDR: Trade longer compile times (11 instantiations per kernel for
// DataAccessor and 2 instantiations per kernel for DataArray) for faster array
// accesses + better opportunity for the compiler to optimize accesses.
//
// DataAccessor<T> and DataArray<T> are designed to support data allocated on
// host or device memory. However, safely accessing data of generic sizes and
// strides requires these data structures to incur additional per-access
// overhead.
//
// The dispatch() helpers in this header exist to automatically avoid
// per-access overhead by evaluating the dtype and compactness exactly once,
// after which the caller's kernel is invoked with either:
//
//  - A typed accessor (a wrapper around a raw pointer) when the data is
//    contiguous (not strided) and the dtype is numeric (not a string), or
//  - The original accessor itself, unchanged, for everything else (strided or
//    non-compact data).
//
// This enables us to handle kernel dispatches with generic accessors of any
// dtype, any stride, and any offset a user provides, while automatically
// upgrading them to a typed accessor (when possible). On the mesh transform
// benchmarks, this optimization significantly improves performance across all
// of our backends.
//
// The downside of this approach is that it requires compiling additional
// kernel instantiations (i.e., we incur extra compile time for each
// instantiation). Each kernel is instantiated for each possible dtype that
// could be passed to it, so that users can enjoy improved accessor performance
// at runtime without having to worry about using specific accessor dtypes at
// compile time.
//
// Each kernel is instantiated 11 times for DataAccessor (10 possible dtypes
// and 1 fallback) and only 2 times for DataArray (DataArrays don't do type
// conversion, so 1 typed accessor and 1 fallback). The two-accessor overload
// dispatches each side independently (any accessor and dtype mix, up to
// 11^2 = 121 instantiations in the worst case), while the three-accessor
// overload dispatches a group through one shared case (a compromise between
// 11 vs. 11^3 = 1331 instantiations) and falls back to the accessors if any
// have a different dtype.
// 
// Supported cases that can be upgraded:
//
//   data passed to a kernel                       | result
//   ----------------------------------------------|----------------------
//   compact, dtype matches the accessor type      | typed accessor  (fast)
//   compact, any other numeric dtype              | typed accessor  (fast)
//   compact pair, dtypes differ between the two   | typed accessors (fast)
//   compact triple, all sharing one dtype         | typed accessors (fast)
//   compact triple, mixed dtypes*                 | DataAccessor    (slow)
//   strided data**                                | DataAccessor    (slow)
//
// *  Possible, but we should wait until we have a real use case for it.
// ** Also possible for some special cases, but left as future work.
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_DISPATCH_HPP
#define CONDUIT_EXECUTION_DISPATCH_HPP

//-----------------------------------------------------------------------------
// -- conduit  includes --
//-----------------------------------------------------------------------------
#include "conduit_data_type.hpp"
#include "conduit_data_accessor.hpp"
#include "conduit_data_array.hpp"

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
// A wrapper around a raw pointer that implements the same access interface as
// DataAccessor<T> and DataArray<T>. This allows kernels to be templated over
// any of the 3 accessor types. T is the dtype that the kernel consumes while U
// is the dtype of the underlying data, since Conduit allows the dtype of the
// underlying data to differ from the dtype of the accessor. Although in the
// DataArray case, T and U are always the same because DataArrays don't perform
// type conversion.
//
// NOTE: We verified via Godbolt (https://godbolt.org/) that the compiler optimizes the static_casts
// away when T and U are the same, so there is no further performance to be
// gained by specializing the RawDataAccessor for that case.
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
// Creates a RawDataAccessor from an input Accessor after determining that it
// is safe to substitute it with a typed accessor.
template <typename U,
          template <typename> class Accessor,
          typename T>
RawDataAccessor<T, U>
make_typed_accessor(const Accessor<T> &acc)
{
    // element_ptr(0) points at the first element with the dtype's byte offset
    // already applied (base + offset + stride * 0). If we've made it this
    // far, we have already verified that the data is upgradeable.
    //
    // The reason two casts are needed is because element_ptr()
    // const-qualifies its return value even though the underlying buffer is
    // not const. const_cast strips the const away and static_cast converts the
    // resulting void* to U*.
    //
    // In the case of DataArray<T>, U is always T because DataArrays don't
    // perform type conversion.
    return RawDataAccessor<T, U>{
        static_cast<U*>(const_cast<void*>(acc.element_ptr(0)))
    };
}

//-----------------------------------------------------------------------------
// A helper that returns true when an array's elements can be walked with a raw
// typed pointer, which implies a typed accessor can be used instead of the
// input Accessor.
//
// "Upgradeable" means the elements are spaced by the dtype's width
// (stride == sizeof(U) for the U that try_typed_accessor will select),
// starting from wherever the data begins. A non-zero offset is fine because
// element_ptr accounts for it on our behalf, and so we can treat the resulting
// pointer as a plain U array.
// 
// Note that this is deliberately NOT DataType::is_compact(), which includes
// the offset in spanned_bytes() and would therefore cause us to fall back in
// cases that can otherwise be upgraded.
inline bool
has_upgradeable_layout(const DataType &dt)
{
    return dt.stride() == DataType::default_bytes(dt.id());
}

//-----------------------------------------------------------------------------
// Returns true when every Accessor in the group is upgradeable and has the
// same dtype. When true, this implies that a single typed kernel
// instantiation can serve them all.
template <template <typename> class Accessor,
          typename T>
bool
is_upgradeable_group(const Accessor<T> &acc0,
                     const Accessor<T> &acc1,
                     const Accessor<T> &acc2)
{
    return has_upgradeable_layout(acc0.dtype()) &&
           has_upgradeable_layout(acc1.dtype()) &&
           has_upgradeable_layout(acc2.dtype()) &&
           acc0.dtype().id() == acc1.dtype().id() &&
           acc0.dtype().id() == acc2.dtype().id();
}

//-----------------------------------------------------------------------------
// A helper for dispatching a kernel based on the underlying dtype of a
// DataAccessor, so that we can avoid having to reproduce this switch statement
// in multiple places.
template <typename Func>
bool
dispatch_dtype(index_t dtype_id, Func &&func)
{
    // Similar to the switch used by DataAccessor set() and element()
    switch(dtype_id)
    {
        case DataType::INT8_ID:
            func(int8{});
            return true;
        case DataType::INT16_ID:
            func(int16{});
            return true;
        case DataType::INT32_ID:
            func(int32{});
            return true;
        case DataType::INT64_ID:
            func(int64{});
            return true;
        case DataType::UINT8_ID:
            func(uint8{});
            return true;
        case DataType::UINT16_ID:
            func(uint16{});
            return true;
        case DataType::UINT32_ID:
            func(uint32{});
            return true;
        case DataType::UINT64_ID:
            func(uint64{});
            return true;
        case DataType::FLOAT32_ID:
            func(float32{});
            return true;
        case DataType::FLOAT64_ID:
            func(float64{});
            return true;
        default:
            // A non-numeric dtype
            return false;
    }
}

//-----------------------------------------------------------------------------
// Invokes the kernel with typed accessors for a group of DataAccessors of a
// single dtype. This allows a single kernel instantiation to serve the group
// of accessors.
template <typename T, typename Kernel>
bool
try_typed_accessor(const DataAccessor<T> &acc0,
                   const DataAccessor<T> &acc1,
                   const DataAccessor<T> &acc2,
                   Kernel &&kernel)
{
    const index_t dtype_id = acc0.dtype().id();
    return dispatch_dtype(dtype_id, [&](auto dtype)
    {
        // decltype deduces the underlying dtype for us
        using U = decltype(dtype);
        kernel(make_typed_accessor<U>(acc0),
               make_typed_accessor<U>(acc1),
               make_typed_accessor<U>(acc2));
    });
}

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed accessor of the underlying dtype. Returns
// false when the dtype is not supported (e.g., non-numeric), in which case the
// caller falls back to invoking the kernel with the DataAccessor directly.
template <typename T, typename Kernel>
bool
try_typed_accessor(const DataAccessor<T> &acc, Kernel &&kernel)
{
    const index_t dtype_id = acc.dtype().id();
    return dispatch_dtype(dtype_id, [&](auto dtype)
    {
        // decltype deduces the underlying dtype for us
        using U = decltype(dtype);
        kernel(make_typed_accessor<U>(acc));
    });
}

//-----------------------------------------------------------------------------
// Invokes the kernel with typed accessors for a group of DataArrays of a
// single dtype. This allows a single kernel instantiation to serve the group
// of accessors. This never returns false because DataArrays don't perform type
// conversion, so the only way this can fail is if the input DataArray is
// strided, which should be caught before this function is called.
template <typename T, typename Kernel>
bool
try_typed_accessor(const DataArray<T> &acc0,
                   const DataArray<T> &acc1,
                   const DataArray<T> &acc2,
                   Kernel &&kernel)
{
    kernel(make_typed_accessor<T>(acc0),
           make_typed_accessor<T>(acc1),
           make_typed_accessor<T>(acc2));
    return true;
}

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed accessor of the underlying data's dtype.
// Never returns false because DataArrays don't do type conversion, so the only
// way this can fail is if the input DataArray is strided, which is caught
// before this function is called.
template <typename T, typename Kernel>
bool
try_typed_accessor(const DataArray<T> &acc, Kernel &&kernel)
{
    kernel(make_typed_accessor<T>(acc));
    return true;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//
// The following helpers exist to accelerate *existing use cases* found
// throughout the codebase. Additional overloads (e.g., larger group sizes, or
// mixed dtypes for groups of 3) could be added later if we find new use cases
// that aren't covered by the existing overloads. The reason to avoid
// supporting them now is to limit negative effects on compile time without
// first demonstrating that we have a need for them.
//

//-----------------------------------------------------------------------------
// dispatch works by trying to invoke the kernel functor with a raw,
// typed accessor to the underlying data. If the necessary conditions are met,
// the kernel is invoked with a typed accessor, which 1) avoids the per-access
// overhead of generic Accessors and 2) gives the compiler an opportunity to
// vectorize. If the conditions are not met, the kernel is invoked with the
// Accessor itself, which will function identically but incur per-access
// overhead. Because each type of accessor provides the same interface for
// reading and writing data, existing kernel code can automatically be upgraded
// to use the faster typed accessor when the conditions are met.
template <template <typename> class Accessor,
          typename T,
          typename Kernel>
void
dispatch(const Accessor<T> &acc, Kernel &&kernel)
{
    if (detail::has_upgradeable_layout(acc.dtype()) &&
        detail::try_typed_accessor(acc, kernel))
    {
        // The conditions were met to invoke the kernel with a typed accessor
        return;
    }
    // The conditions were not met to invoke the kernel with a typed accessor,
    // so we directly invoke it with the provided Accessor instead.
    kernel(acc);
}

//-----------------------------------------------------------------------------
// A special case of dispatch that takes two Accessors. Each side is dispatched
// independently, so the kernel can be upgraded to use typed accessors even if
// the accessor types (e.g., a DataAccessor and DataArray) and underlying
// dtypes differ.
template <template <typename> class Accessor0,
          template <typename> class Accessor1,
          typename T0,
          typename T1,
          typename Kernel>
void
dispatch(const Accessor0<T0> &acc0,
         const Accessor1<T1> &acc1,
         Kernel &&kernel)
{
    // For future reference, one could nest these an arbitrary number of times
    // to support executing kernels with more than two accessors of different
    // dtypes.
    dispatch(acc0, [&](auto vals0)
    {
        dispatch(acc1, [&](auto vals1)
        {
            kernel(vals0, vals1);
        });
    });
}

//-----------------------------------------------------------------------------
// A special case of dispatch that takes three Accessors of the same dtype. It
// is possible to template this for 3 different dtypes like in the previous
// helper, but that would require 11^3 instantiations of the kernel in the
// worst case (one for each combination of dtypes and fallbacks). That decision
// could be revisited if we later find that kernel dispatches with 3 accessors
// of mixed dtypes are more common than expected.
template <template <typename> class Accessor,
          typename T,
          typename Kernel>
void
dispatch(const Accessor<T> &acc0,
         const Accessor<T> &acc1,
         const Accessor<T> &acc2,
         Kernel &&kernel)
{
    if (detail::is_upgradeable_group(acc0, acc1, acc2) &&
        detail::try_typed_accessor(acc0, acc1, acc2, kernel))
    {
        // The conditions were met to invoke the kernel with typed accessors
        return;
    }
    // The conditions were not met to invoke the kernel with typed accessors,
    // so we directly invoke it with the provided Accessors instead.
    kernel(acc0, acc1, acc2);
}

// TODO: Investigate adding support for compact strided arrays, plus figure
// out a way to benchmark with compact strided data.

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif // CONDUIT_EXECUTION_DISPATCH_HPP
