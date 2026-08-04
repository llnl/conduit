// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_array_views.hpp
///
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// TLDR: Trade longer compile times for faster execution times.
//
// DataAccessor<T> accepts any leaf dtype and any stride, which is necessary
// for supporting the menu of choices that Conduit makes available to users.
// However, element(idx) computes offset + stride * idx and then switches on
// the dtype id to convert the value to T. In a kernel over millions of
// elements, that switch is evaluated per-element, even though the dtype will
// never change mid-kernel, and the switch prevents the compiler from
// vectorizing the loop (or coalescing on GPUs).
//
// The with_*_values() helpers in this header enable that decision to be
// moved out of the kernel so that we avoid the overhead of evaluating
// the dtype switch statement per-element. The idea is that each helper
// inspects the accessor's dtype once and then invokes the caller's kernel
// with either:
//
//  - A typed view (a wrapper around a raw pointer) when the data is
//    contiguous (not strided) and the dtype is numeric, or
//  - The accessor itself, unchanged, for everything else (strided data,
//    non-numeric leaves).
//
// This allows the kernel to be written once, templated over the array-like
// argument, and guarantees that it will behave identically on both paths,
// since the views apply the same conversion to T that DataAccessor would.
// This enables us to handle kernel dispatches with any dtype, any stride,
// and any offset a user provides, while automatically optimizing array
// accesses (when possible). On the mesh transform benchmarks this improves
// performance by 2-6x depending on the transform and backend.
//
// The downside of this approach is that it requires compilation of
// additional kernel instantiations (i.e., we incur extra compile time for
// each potential typed view of a given kernel). Every numeric dtype has a
// view compiled for it, so each dispatched array instantiates the kernel
// 11 times (10 views plus the accessor fallback), and nesting dispatches
// multiplies that. This trades a one-time compilation cost for faster
// execution regardless of which dtype the user provides at runtime. The
// grouped with_read_values() overloads help limit the number of kernel
// instantiations for common cases found throughout the codebase: 2 and 3
// same-typed arrays can share one kernel instantiation.
// 
// It would be possible to support strided views as well, but 1) since the
// stride is a runtime value, strided views could not be vectorized the way
// contiguous views are, so most of the benefit would not materialize, and 2)
// it would add another view per dtype (21 kernel instantiations per array
// instead of 11).
// 
// TODO: It may be interesting to support compile-time strided views as a
// cmake option. This would improve performance at least somewhat in
// exchange for significantly worse compile times (which still may be worth
// it if conduit isn't recompiled often, e.g., in an HPC setting). It would
// certainly be worse for development, though.
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_ARRAY_VIEWS_HPP
#define CONDUIT_EXECUTION_ARRAY_VIEWS_HPP

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
// Typed raw-pointer views over an array, mirroring the parts of the
// DataAccessor interface kernels use (operator[] and set(idx,val)). T is the
// type the kernel consumes; U is the type stored in the buffer. Each access
// converts with a single cast, matching DataAccessor<T>. Kernels templated
// over "some array-like thing" accept either a DataAccessor<T> or one of
// these interchangeably.
//
// conduit::execution::with_read_values() and friends (below) select the
// view from the buffer's dtype.
//-----------------------------------------------------------------------------
template <typename T, typename U>
struct ConvertingArrayReader
{
    const U *ptr;
    CONDUIT_EXEC T operator[](index_t idx) const
                 { return static_cast<T>(ptr[idx]); }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct ConvertingArrayWriter
{
    U *ptr;
    CONDUIT_EXEC void set(index_t idx, T value) const
                 { ptr[idx] = static_cast<U>(value); }
};

//-----------------------------------------------------------------------------
template <typename T, typename U>
struct ConvertingArrayReadWriter
{
    U *ptr;
    CONDUIT_EXEC T operator[](index_t idx) const
                 { return static_cast<T>(ptr[idx]); }
    CONDUIT_EXEC void set(index_t idx, T value) const
                 { ptr[idx] = static_cast<U>(value); }
};

//-----------------------------------------------------------------------------
// True when the dtype's elements are contiguous, so a typed pointer can walk
// them directly. DataType::is_compact() is not usable here: it also requires
// a zero offset.
inline bool
is_compact_layout(const DataType &dt)
{
    return dt.stride() == dt.element_bytes();
}

//-----------------------------------------------------------------------------
// DataAccessor::set() writes through a const accessor, so the mutable views
// below get their pointer the same way.
template <typename U, typename T>
U *
mutable_array_ptr(const DataAccessor<T> &acc)
{
    return static_cast<U*>(const_cast<void*>(acc.element_ptr(0)));
}

//-----------------------------------------------------------------------------
template <typename U, typename T>
ConvertingArrayReader<T, U>
make_reader(const DataAccessor<T> &acc)
{
    return ConvertingArrayReader<T, U>{
        static_cast<const U*>(acc.element_ptr(0))
    };
}

//-----------------------------------------------------------------------------
// Invokes the kernel with a typed read view matching the buffer's dtype.
// Returns false when the dtype has no view, in which case the caller falls
// back to the accessor.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
bool
try_read_view(const DataAccessor<T> &acc, Kernel &&kernel)
{
    switch(acc.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(make_reader<int8>(acc));
            return true;
        case DataType::INT16_ID:
            kernel(make_reader<int16>(acc));
            return true;
        case DataType::INT32_ID:
            kernel(make_reader<int32>(acc));
            return true;
        case DataType::INT64_ID:
            kernel(make_reader<int64>(acc));
            return true;
        case DataType::UINT8_ID:
            kernel(make_reader<uint8>(acc));
            return true;
        case DataType::UINT16_ID:
            kernel(make_reader<uint16>(acc));
            return true;
        case DataType::UINT32_ID:
            kernel(make_reader<uint32>(acc));
            return true;
        case DataType::UINT64_ID:
            kernel(make_reader<uint64>(acc));
            return true;
        case DataType::FLOAT32_ID:
            kernel(make_reader<float32>(acc));
            return true;
        case DataType::FLOAT64_ID:
            kernel(make_reader<float64>(acc));
            return true;
        default:
            return false;
    }
}

//-----------------------------------------------------------------------------
// True when every accessor in the group is compact and they all share one
// dtype, so a single typed view can serve them all.
//-----------------------------------------------------------------------------
template <typename T>
bool
group_is_uniform(const DataAccessor<T> &acc0,
                 const DataAccessor<T> &acc1)
{
    return is_compact_layout(acc0.dtype()) &&
           is_compact_layout(acc1.dtype()) &&
           acc0.dtype().id() == acc1.dtype().id();
}

//-----------------------------------------------------------------------------
// Similar to group_is_uniform(), but for three accessors.
//-----------------------------------------------------------------------------
template <typename T>
bool
group_is_uniform(const DataAccessor<T> &acc0,
                 const DataAccessor<T> &acc1,
                 const DataAccessor<T> &acc2)
{
    return group_is_uniform(acc0, acc1) &&
           is_compact_layout(acc2.dtype()) &&
           acc0.dtype().id() == acc2.dtype().id();
}

//-----------------------------------------------------------------------------
// Invokes the kernel with typed read views for a group of accessors that
// group_is_uniform() accepted. One dtype case serves the whole group, so
// the number of kernel instantiations does not depend on the group size.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
bool
try_group_read_view(const DataAccessor<T> &acc0,
                    const DataAccessor<T> &acc1,
                    Kernel &&kernel)
{
    switch(acc0.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(make_reader<int8>(acc0),
                   make_reader<int8>(acc1));
            return true;
        case DataType::INT16_ID:
            kernel(make_reader<int16>(acc0),
                   make_reader<int16>(acc1));
            return true;
        case DataType::INT32_ID:
            kernel(make_reader<int32>(acc0),
                   make_reader<int32>(acc1));
            return true;
        case DataType::INT64_ID:
            kernel(make_reader<int64>(acc0),
                   make_reader<int64>(acc1));
            return true;
        case DataType::UINT8_ID:
            kernel(make_reader<uint8>(acc0),
                   make_reader<uint8>(acc1));
            return true;
        case DataType::UINT16_ID:
            kernel(make_reader<uint16>(acc0),
                   make_reader<uint16>(acc1));
            return true;
        case DataType::UINT32_ID:
            kernel(make_reader<uint32>(acc0),
                   make_reader<uint32>(acc1));
            return true;
        case DataType::UINT64_ID:
            kernel(make_reader<uint64>(acc0),
                   make_reader<uint64>(acc1));
            return true;
        case DataType::FLOAT32_ID:
            kernel(make_reader<float32>(acc0),
                   make_reader<float32>(acc1));
            return true;
        case DataType::FLOAT64_ID:
            kernel(make_reader<float64>(acc0),
                   make_reader<float64>(acc1));
            return true;
        default:
            return false;
    }
}

//-----------------------------------------------------------------------------
// Same as try_group_read_view(), but for three accessors.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
bool
try_group_read_view(const DataAccessor<T> &acc0,
                    const DataAccessor<T> &acc1,
                    const DataAccessor<T> &acc2,
                    Kernel &&kernel)
{
    switch(acc0.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(make_reader<int8>(acc0),
                   make_reader<int8>(acc1),
                   make_reader<int8>(acc2));
            return true;
        case DataType::INT16_ID:
            kernel(make_reader<int16>(acc0),
                   make_reader<int16>(acc1),
                   make_reader<int16>(acc2));
            return true;
        case DataType::INT32_ID:
            kernel(make_reader<int32>(acc0),
                   make_reader<int32>(acc1),
                   make_reader<int32>(acc2));
            return true;
        case DataType::INT64_ID:
            kernel(make_reader<int64>(acc0),
                   make_reader<int64>(acc1),
                   make_reader<int64>(acc2));
            return true;
        case DataType::UINT8_ID:
            kernel(make_reader<uint8>(acc0),
                   make_reader<uint8>(acc1),
                   make_reader<uint8>(acc2));
            return true;
        case DataType::UINT16_ID:
            kernel(make_reader<uint16>(acc0),
                   make_reader<uint16>(acc1),
                   make_reader<uint16>(acc2));
            return true;
        case DataType::UINT32_ID:
            kernel(make_reader<uint32>(acc0),
                   make_reader<uint32>(acc1),
                   make_reader<uint32>(acc2));
            return true;
        case DataType::UINT64_ID:
            kernel(make_reader<uint64>(acc0),
                   make_reader<uint64>(acc1),
                   make_reader<uint64>(acc2));
            return true;
        case DataType::FLOAT32_ID:
            kernel(make_reader<float32>(acc0),
                   make_reader<float32>(acc1),
                   make_reader<float32>(acc2));
            return true;
        case DataType::FLOAT64_ID:
            kernel(make_reader<float64>(acc0),
                   make_reader<float64>(acc1),
                   make_reader<float64>(acc2));
            return true;
        default:
            return false;
    }
}

//-----------------------------------------------------------------------------
// Shared by the write-only and in-place helpers, templated on the view
// (ConvertingArrayWriter or ConvertingArrayReadWriter).
//-----------------------------------------------------------------------------
template <template <typename, typename> class View,
          typename T,
          typename Kernel>
bool
try_mutable_view(const DataAccessor<T> &acc, Kernel &&kernel)
{
    switch(acc.dtype().id())
    {
        case DataType::INT8_ID:
            kernel(View<T, int8>{mutable_array_ptr<int8>(acc)});
            return true;
        case DataType::INT16_ID:
            kernel(View<T, int16>{mutable_array_ptr<int16>(acc)});
            return true;
        case DataType::INT32_ID:
            kernel(View<T, int32>{mutable_array_ptr<int32>(acc)});
            return true;
        case DataType::INT64_ID:
            kernel(View<T, int64>{mutable_array_ptr<int64>(acc)});
            return true;
        case DataType::UINT8_ID:
            kernel(View<T, uint8>{mutable_array_ptr<uint8>(acc)});
            return true;
        case DataType::UINT16_ID:
            kernel(View<T, uint16>{mutable_array_ptr<uint16>(acc)});
            return true;
        case DataType::UINT32_ID:
            kernel(View<T, uint32>{mutable_array_ptr<uint32>(acc)});
            return true;
        case DataType::UINT64_ID:
            kernel(View<T, uint64>{mutable_array_ptr<uint64>(acc)});
            return true;
        case DataType::FLOAT32_ID:
            kernel(View<T, float32>{mutable_array_ptr<float32>(acc)});
            return true;
        case DataType::FLOAT64_ID:
            kernel(View<T, float64>{mutable_array_ptr<float64>(acc)});
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
// throughout the codebase. Additional overloads (e.g. more group sizes)
// can be added as call sites need them.
//

//-----------------------------------------------------------------------------
// with_read_values selects a typed raw-pointer view of the buffer's actual
// dtype (detail::ConvertingArrayReader<T,U>, above) and invokes the kernel
// functor with it, falling back to the accessor itself when the data is
// strided. The view performs the same T conversion the accessor would, so
// results are identical.
//
// Every numeric dtype has a view, so a kernel is instantiated 11 times per
// dispatched array (10 views plus the accessor fallback), and nested
// dispatches multiply the number of kernel instantiations. In other words,
// we're trading compile-time overhead for runtime performance.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
void
with_read_values(const DataAccessor<T> &acc, Kernel &&kernel)
{
    if (detail::is_compact_layout(acc.dtype()) &&
        detail::try_read_view(acc, kernel))
    {
        // The conditions were met to run the kernel with a typed view
        return;
    }
    // The conditions were not met to run the kernel with a typed view,
    // so run the kernel with the provided accessor.
    kernel(acc);
}

//-----------------------------------------------------------------------------
// Grouped read dispatch over arrays that share the kernel type T, invoking
// kernel(vals0, vals1, ...). All members must be compact and share one
// dtype, otherwise the whole group falls back to the accessors. One typed
// view covers the group, so the kernel is still instantiated only 11 times,
// not once per combination.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
void
with_read_values(const DataAccessor<T> &acc0,
                 const DataAccessor<T> &acc1,
                 Kernel &&kernel)
{
    if (detail::group_is_uniform(acc0, acc1) &&
        detail::try_group_read_view(acc0, acc1, kernel))
    {
        // The conditions were met to run the kernel with a typed view
        return;
    }
    // The conditions were not met to run the kernel with a typed view,
    // so run the kernel with the provided accessor.
    kernel(acc0, acc1);
}

//-----------------------------------------------------------------------------
// Same as with_read_values(), but for 3 accessors.
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
void
with_read_values(const DataAccessor<T> &acc0,
                 const DataAccessor<T> &acc1,
                 const DataAccessor<T> &acc2,
                 Kernel &&kernel)
{
    if (detail::group_is_uniform(acc0, acc1, acc2) &&
        detail::try_group_read_view(acc0, acc1, acc2, kernel))
    {
        // The conditions were met to run the kernel with a typed view
        return;
    }
    // The conditions were not met to run the kernel with a typed view,
    // so run the kernel with the provided accessor.
    kernel(acc0, acc1, acc2);
}

//-----------------------------------------------------------------------------
// Write-side counterpart of with_read_values().
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
void
with_write_values(const DataAccessor<T> &acc, Kernel &&kernel)
{
    if (detail::is_compact_layout(acc.dtype()) &&
        detail::try_mutable_view<detail::ConvertingArrayWriter>(acc, kernel))
    {
        // The conditions were met to run the kernel with a typed view
        return;
    }
    // The conditions were not met to run the kernel with a typed view,
    // so run the kernel with the provided accessor.
    kernel(acc);
}

//-----------------------------------------------------------------------------
// In-place counterpart of with_read_values(), for kernels that both read and
// write to the same array (e.g. vals.set(i, f(vals[i]))).
//-----------------------------------------------------------------------------
template <typename T, typename Kernel>
void
with_read_write_values(const DataAccessor<T> &acc, Kernel &&kernel)
{
    if (detail::is_compact_layout(acc.dtype()) &&
        detail::try_mutable_view<detail::ConvertingArrayReadWriter>(acc,
                                                                    kernel))
    {
        // The conditions were met to run the kernel with a typed view
        return;
    }
    // The conditions were not met to run the kernel with a typed view,
    // so run the kernel with the provided accessor.
    kernel(acc);
}

//-----------------------------------------------------------------------------
// Dispatch over two different arrays, invoking kernel(src_vals, dst_vals).
// For an in-place kernel over a single array, use with_read_write_values().
//-----------------------------------------------------------------------------
template <typename SrcT, typename DstT, typename Kernel>
void
with_values(const DataAccessor<SrcT> &src_acc,
            const DataAccessor<DstT> &dst_acc,
            Kernel &&kernel)
{
    with_write_values(dst_acc, [&](auto dst_vals)
    {
        with_read_values(src_acc, [&](auto src_vals)
        {
            kernel(src_vals, dst_vals);
        });
    });
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
