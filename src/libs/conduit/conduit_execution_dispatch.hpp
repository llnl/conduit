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
// DataAccessor and 2 instantiations per kernel for DataArray) for faster data
// accesses + allow the compiler to vectorize and coalesce certain operations.
//
// DataAccessor and DataArray (generically referred to as DataViews) are
// designed to support data allocated on host or device memory. However, safely
// accessing data of generic widths and strides requires these data structures
// to incur additional per-access overhead.
//
// The dispatch() helpers in this header exist to automatically avoid the
// per-access overhead of DataViews by evaluating their underlying dtype and
// data layout exactly once, after which the caller's kernel is invoked with
// either:
//
//  - A typed view (a wrapper around a raw pointer) when the data is
//    contiguous (not strided) and the dtype is numeric (not a string), or
//  - The original DataView itself, unchanged, for everything else (e.g.,
//    strided or non-compact data).
//
// This enables us to handle kernel dispatches with generic DataViews of any
// dtype, any stride, and any offset a user provides, while automatically
// upgrading them to a typed view (when possible). This optimization has been
// shown to significantly improve performance across all of our backends.
//
// The downside of this approach is that it requires us to compile additional
// kernel instantiations (i.e., we incur extra compile time for each separate
// instantiation). Each kernel is instantiated for each possible dtype that
// can be passed to it, so that users can enjoy improved DataView performance
// at runtime without having to worry about using specific dtypes.
//
// Each kernel is instantiated 11 times for DataAccessor (10 possible dtypes
// and 1 fallback) and only 2 times for DataArray (DataArrays don't do type
// conversion, so 1 typed view and 1 fallback). The two-view overload
// dispatches each side independently (any DataView and dtype mix, up to
// 11^2 = 121 instantiations in the worst case), while the three- and four-view
// overloads dispatch a group through one shared case (a compromise between
// 11 vs. 11^3 or 11^4 instantiations) and fall back to the original DataViews
// if any have a different dtype.
//
// Supported cases that can be upgraded:
//
//   DataViews passed to a kernel                  | result
//   ----------------------------------------------|----------------------
//   compact single,     any numeric dtype         | TypedDataView (fast)
//   compact pair,       *different* dtypes        | TypedDataView (fast)
//   compact triple,     *same* dtype              | TypedDataView (fast)
//   compact quadruple,  *same* dtype              | TypedDataView (fast)
//   compact group (>2), *different* dtypes        | DataView      (slow)
//   any combination with strided data             | DataView      (slow)
//
//-----------------------------------------------------------------------------

// TODO: Investigate adding support for compact strided arrays, plus figure
// out a way to benchmark with compact strided data.

// TODO: Investigate adding an API that dispatches by dtype width and not dtype
// itself, for cases where we don't intend to do any math within a kernel
// (e.g., a bulk set only copies data). For supported kernels, it would let us
// upgrade the views using only 5 instantiations (4 dtype widths + 1 fallback)
// instead of the 11 instantiations that are currently always required
// (10 dtypes + 1 fallback).

// TODO: It would be nice to provide a way to let users ask if a particular
// dispatch call resulted in upgraded DataViews or not. The tests demonstrate a
// way that users can get this information (with some effort). Given that the
// performance difference of upgrading can be significant, I think users/devs
// would appreciate a built-in API/utility to confirm that they're getting the
// benefits.

#ifndef CONDUIT_EXECUTION_DISPATCH_HPP
#define CONDUIT_EXECUTION_DISPATCH_HPP

//-----------------------------------------------------------------------------
// -- conduit includes --
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
// any of the 3 view types. T is the dtype that the kernel consumes while U
// is the dtype of the underlying data, since Conduit allows the dtype of the
// underlying data to differ from the dtype of the DataAccessor. Although in
// the DataArray case, T and U are always the same because DataArrays don't
// perform type conversion.
//
// NOTE: We verified via Godbolt (https://godbolt.org/) that gcc and clang both
// optimize the static_casts away when T and U are the same, so there is no
// performance to be gained by specializing TypedDataView for that case.
template <typename T, typename U>
struct TypedDataView
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
// Creates a TypedDataView from an input DataView after determining that it is
// safe to substitute it with a typed view.
template <typename U,
          template <typename> class DataView,
          typename T>
TypedDataView<T, U>
make_typed_view(const DataView<T> &view)
{
    // element_ptr(0) points at the first element with the dtype's byte offset
    // already applied (base + offset + stride * 0). If we've made it this
    // far, we have already verified that the data is upgradeable.
    //
    // The reason two casts are needed is because element_ptr()
    // const-qualifies its return value even though the underlying buffer is
    // not const. const_cast strips the const away and static_cast converts the
    // resulting void* into U*.
    //
    // In the case of DataArray<T>, U is always T because DataArrays don't
    // perform type conversion.
    return TypedDataView<T, U>{
        static_cast<U*>(const_cast<void*>(view.element_ptr(0)))
    };
}

//-----------------------------------------------------------------------------
// A helper for dispatching a kernel based on the underlying dtype of a
// DataAccessor, so that we can avoid having to reproduce this switch statement
// in multiple places.
template <typename Func>
bool
dispatch_dtype(index_t dtype_id, Func &&func)
{
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

//
// DataAccessor Specializations
//

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed view of the underlying dtype. Returns false
// when the dtype is not supported (e.g., non-numeric), in which case the
// caller falls back to invoking the kernel with the DataAccessor directly.
template <typename T, typename Kernel>
bool
try_typed_view(const DataAccessor<T> &view, Kernel &&kernel)
{
    const index_t dtype_id = view.dtype().id();
    return dispatch_dtype(dtype_id, [&](auto dtype)
    {
        // decltype deduces the underlying dtype for us
        using U = decltype(dtype);
        kernel(make_typed_view<U>(view));
    });
}

//-----------------------------------------------------------------------------
// Same as above but for a group of 3 DataAccessors with the same dtype.
template <typename T, typename Kernel>
bool
try_typed_view(const DataAccessor<T> &view0,
               const DataAccessor<T> &view1,
               const DataAccessor<T> &view2,
               Kernel &&kernel)
{
    const index_t dtype_id = view0.dtype().id();
    return dispatch_dtype(dtype_id, [&](auto dtype)
    {
        // decltype deduces the underlying dtype for us
        using U = decltype(dtype);
        kernel(make_typed_view<U>(view0),
               make_typed_view<U>(view1),
               make_typed_view<U>(view2));
    });
}

//-----------------------------------------------------------------------------
// Same as above, for a group of four DataAccessors.
template <typename T, typename Kernel>
bool
try_typed_view(const DataAccessor<T> &view0,
               const DataAccessor<T> &view1,
               const DataAccessor<T> &view2,
               const DataAccessor<T> &view3,
               Kernel &&kernel)
{
    const index_t dtype_id = view0.dtype().id();
    return dispatch_dtype(dtype_id, [&](auto dtype)
    {
        // decltype deduces the underlying dtype for us
        using U = decltype(dtype);
        kernel(make_typed_view<U>(view0),
               make_typed_view<U>(view1),
               make_typed_view<U>(view2),
               make_typed_view<U>(view3));
    });
}

//
// DataArray Specializations
//

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed view of the underlying dtype. Returns false
// when the dtype is not supported (e.g., non-numeric), in which case the
// caller falls back to invoking the kernel with the DataAccessor directly.
// Invokes the kernel with a typed view of the underlying data's dtype. Never
// returns false for this specialization because DataArray doesn't do type
// conversion. The only way that this could fail is if the input DataArray is
// strided, which gets checked before this function is called.
template <typename T, typename Kernel>
bool
try_typed_view(const DataArray<T> &view, Kernel &&kernel)
{
    kernel(make_typed_view<T>(view));
    return true;
}

//-----------------------------------------------------------------------------
// Same as above but for a group of 3 DataArrays with the same dtype.
template <typename T, typename Kernel>
bool
try_typed_view(const DataArray<T> &view0,
               const DataArray<T> &view1,
               const DataArray<T> &view2,
               Kernel &&kernel)
{
    kernel(make_typed_view<T>(view0),
           make_typed_view<T>(view1),
           make_typed_view<T>(view2));
    return true;
}

//-----------------------------------------------------------------------------
// Same as above, for a group of four DataArrays.
template <typename T, typename Kernel>
bool
try_typed_view(const DataArray<T> &view0,
               const DataArray<T> &view1,
               const DataArray<T> &view2,
               const DataArray<T> &view3,
               Kernel &&kernel)
{
    kernel(make_typed_view<T>(view0),
           make_typed_view<T>(view1),
           make_typed_view<T>(view2),
           make_typed_view<T>(view3));
    return true;
}

// A helper that returns true when a view's data is safe to access through a
// raw typed pointer, which implies a typed view can be used instead of the
// input DataView.
//
// "Upgradeable" means that the elements are spaced by the dtype's width
// (stride == sizeof(U) for the U that try_typed_view will select) and that the
// first element's address is aligned to that width. element_ptr accounts for
// the dtype's offset, but not for alignment: base + offset might not be a
// multiple of the element width. Those cases must fall back to the original
// DataView to avoid undefined behavior.
//
// Note that this is deliberately NOT DataType::is_compact(), which includes
// the offset in spanned_bytes() and would therefore cause us to fall back in
// cases that can otherwise be upgraded.
template <template <typename> class DataView,
          typename T>
bool
is_upgradeable(const DataView<T> &view)
{
    const DataType &dt = view.dtype();
    const index_t width = DataType::default_bytes(dt.id());

    // default_bytes() returns 0 for non-numeric dtypes
    if (width <= 0)
    {
        return false;
    }

    // Native stride means that the elements are spaced by the dtype's width
    if (dt.stride() != width)
    {
        return false;
    }

    // Aligned means that the memory address of the first element is a multiple
    // of the dtype's width.
    uintptr_t address = reinterpret_cast<uintptr_t>(view.element_ptr(0));
    return address % static_cast<uintptr_t>(width) == 0;
}

//-----------------------------------------------------------------------------
// Returns true when every view in the group is upgradeable and has the same
// dtype. When true, this implies that a single typed kernel instantiation can
// serve them all.
template <template <typename> class DataView,
          typename T>
bool
is_upgradeable_group(const DataView<T> &view0,
                     const DataView<T> &view1,
                     const DataView<T> &view2)
{
    const index_t view0_dtype_id = view0.dtype().id();
    return view0_dtype_id == view1.dtype().id() &&
           view0_dtype_id == view2.dtype().id() &&
           is_upgradeable(view0) &&
           is_upgradeable(view1) &&
           is_upgradeable(view2);
}

//-----------------------------------------------------------------------------
// Same as above but for a group of 4 DataViews
template <template <typename> class DataView,
          typename T>
bool
is_upgradeable_group(const DataView<T> &view0,
                     const DataView<T> &view1,
                     const DataView<T> &view2,
                     const DataView<T> &view3)
{
    const index_t view0_dtype_id = view0.dtype().id();
    return view0_dtype_id == view1.dtype().id() &&
           view0_dtype_id == view2.dtype().id() &&
           view0_dtype_id == view3.dtype().id() &&
           is_upgradeable(view0) &&
           is_upgradeable(view1) &&
           is_upgradeable(view2) &&
           is_upgradeable(view3);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//
// The following helpers exist to accelerate *existing use cases* found
// throughout the codebase. Additional overloads can be added later as we find
// new use cases that aren't covered by the existing overloads. The reason to
// avoid supporting them now is to limit negative effects on compile time
// without first demonstrating that we have a need for a new overload.
//

//-----------------------------------------------------------------------------
// dispatch works by trying to invoke the kernel functor with a typed view of
// the underlying data. If the necessary conditions are met, the kernel is
// invoked with a typed view, which 1) avoids the per-access overhead of
// DataAccessor and DataArray and 2) gives the compiler an opportunity to
// vectorize certain operations. If the conditions are not met, the kernel is
// invoked with the DataView itself, which will function identically but incur
// per-access overhead and prevent vectorization. Because DataAccessor and
// DataArray provide the same interface for reading and writing data, we can
// automatically upgrade existing kernels to a typed view that also implements
// the same access interface.
template <template <typename> class DataView,
          typename T,
          typename Kernel>
void
dispatch(const DataView<T> &view, Kernel &&kernel)
{
    if (detail::is_upgradeable(view) &&
        detail::try_typed_view(view, kernel))
    {
        // The conditions were met to invoke the kernel with a typed view
        return;
    }
    // The conditions were not met to invoke the kernel with a typed view,
    // so we directly invoke it with the provided DataView instead.
    kernel(view);
}

//-----------------------------------------------------------------------------
// A special case of dispatch that takes two DataViews of *different* dtypes.
template <template <typename> class DataView0,
          template <typename> class DataView1,
          typename T0,
          typename T1,
          typename Kernel>
void
dispatch(const DataView0<T0> &view0,
         const DataView1<T1> &view1,
         Kernel &&kernel)
{
    // For future reference, one could nest these an arbitrary number of times
    // to support executing kernels with more than two views of different
    // dtypes.
    dispatch(view0, [&](auto vals0)
    {
        dispatch(view1, [&](auto vals1)
        {
            kernel(vals0, vals1);
        });
    });
}

//-----------------------------------------------------------------------------
// A special case of dispatch that takes three DataViews of the *same* dtype.
template <template <typename> class DataView,
          typename T,
          typename Kernel>
void
dispatch(const DataView<T> &view0,
         const DataView<T> &view1,
         const DataView<T> &view2,
         Kernel &&kernel)
{
    if (detail::is_upgradeable_group(view0, view1, view2) &&
        detail::try_typed_view(view0, view1, view2, kernel))
    {
        // The conditions were met to invoke the kernel with typed views
        return;
    }
    // The conditions were not met to invoke the kernel with typed views,
    // so we directly invoke it with the provided DataViews instead.
    kernel(view0, view1, view2);
}

//-----------------------------------------------------------------------------
// A special case of dispatch that takes four DataViews of the *same* dtype.
template <template <typename> class DataView,
          typename T,
          typename Kernel>
void
dispatch(const DataView<T> &view0,
         const DataView<T> &view1,
         const DataView<T> &view2,
         const DataView<T> &view3,
         Kernel &&kernel)
{
    if (detail::is_upgradeable_group(view0, view1, view2, view3) &&
        detail::try_typed_view(view0, view1, view2, view3, kernel))
    {
        // The conditions were met to invoke the kernel with typed views
        return;
    }
    // The conditions were not met to invoke the kernel with typed views,
    // so we directly invoke it with the provided DataViews instead.
    kernel(view0, view1, view2, view3);
}

// TODO: Investigate adding support for compact strided arrays, plus figure
// out a way to benchmark with compact strided data.

// TODO: Investigate adding an API that dispatches by dtype width and not dtype
// itself, for cases where we don't intend to do any math within a kernel
// (e.g., a bulk set only copies data). For supported kernels, it would let us
// upgrade the views using only 5 instantiations (4 dtype widths + 1 fallback)
// instead of the 11 instantiations that are currently always required
// (10 dtypes + 1 fallback).

// TODO: It would be nice to provide a way to let users ask if a particular
// dispatch call resulted in upgraded DataViews or not. The tests demonstrate a
// way that users can get this information (with some effort). Given that the
// performance difference of upgrading can be significant, I think users/devs
// would appreciate a built-in API/utility to confirm that they're getting the
// benefits.

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif // CONDUIT_EXECUTION_DISPATCH_HPP
