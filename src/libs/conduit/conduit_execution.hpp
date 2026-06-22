// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_HPP
#define CONDUIT_EXECUTION_HPP

#include "conduit_config.hpp"
#include "conduit_exports.h"

// CONDUIT_DEVICE_ERROR_CHECK: error checking macro
#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

#include "conduit_execution_macros.hpp"
#include "conduit_execution_policy.hpp"
#include "conduit_annotations.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <typeinfo>
#include <utility>

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin conduit::execution --
//-----------------------------------------------------------------------------
namespace execution
{

//-----------------------------------------------------------------------------
/// Pass a Node to set execution options.
//-----------------------------------------------------------------------------
CONDUIT_API void execution_set_options(const Node &opts);

//-----------------------------------------------------------------------------
/// Get a Node that contains execution options.
//-----------------------------------------------------------------------------
CONDUIT_API void execution_options(Node &opts);

//-----------------------------------------------------------------------------
/// Reset execution options to their default values.
//-----------------------------------------------------------------------------
CONDUIT_API void reset_execution_options();

//-----------------------------------------------------------------------------
/// Get an execution policy based on the policy option.
//-----------------------------------------------------------------------------
CONDUIT_API ExecutionPolicy get_execution_policy(Node &src_node);

//-----------------------------------------------------------------------------
/// Get an execution policy based on the policy option.
//-----------------------------------------------------------------------------
CONDUIT_API ExecutionPolicy get_execution_policy();

//-----------------------------------------------------------------------------
/// Get the output allocator id based on the allocator option.
//-----------------------------------------------------------------------------
CONDUIT_API index_t get_output_allocator_id();

//-----------------------------------------------------------------------------
/// Get the output allocator id based on the allocator option.
//-----------------------------------------------------------------------------
CONDUIT_API index_t get_output_allocator_id(Node &src_node);

//-----------------------------------------------------------------------------
/// Get the sync strategy option.
//-----------------------------------------------------------------------------
CONDUIT_API const std::string& get_sync_strategy();

//-----------------------------------------------------------------------------
/// Get the device allocator id.
//-----------------------------------------------------------------------------
CONDUIT_API index_t get_device_allocator_id();

//-----------------------------------------------------------------------------
/// Get the host allocator id.
//-----------------------------------------------------------------------------
CONDUIT_API index_t get_host_allocator_id();

//---------------------------------------------------------------------------//
#if defined(CONDUIT_USE_RAJA)
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// RAJA_ON detail backend/reducers for when raja is on
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
// -- begin conduit::execution::detail --
//-----------------------------------------------------------------------------
namespace detail
{

//-----------------------------------------------------------------------------
// Reducers follow translation-unit capability rather than the runtime loop
// policy. In GPU-capable translation units, RAJA reduction objects can use the
// device reduction backend regardless of whether the surrounding forall runs
// with host or device execution policy.
#if defined(CONDUIT_TU_IS_CUDA)
using DefaultReducePolicy = RAJA::cuda_reduce;
#elif defined(CONDUIT_TU_IS_HIP)
using DefaultReducePolicy = RAJA::hip_reduce;
#elif defined(CONDUIT_USE_OPENMP)
using DefaultReducePolicy = RAJA::omp_reduce;
#else // Sequential execution
using DefaultReducePolicy = RAJA::seq_reduce;
#endif

//-----------------------------------------------------------------------------
// Atomics also follow translation-unit capability instead of the runtime loop
// policy. GPU-capable translation units use device atomics in device compile
// paths, with host fallback selected once here for all runtime policies.
#if defined(CONDUIT_TU_IS_CUDA)
  #if defined(CONDUIT_USE_OPENMP)
using DefaultAtomicPolicy = RAJA::cuda_atomic_explicit<RAJA::omp_atomic>;
  #else
using DefaultAtomicPolicy = RAJA::cuda_atomic;
  #endif
#elif defined(CONDUIT_TU_IS_HIP)
  #if defined(CONDUIT_USE_OPENMP)
using DefaultAtomicPolicy = RAJA::hip_atomic_explicit<RAJA::omp_atomic>;
  #else
using DefaultAtomicPolicy = RAJA::hip_atomic;
  #endif
#elif defined(CONDUIT_USE_OPENMP)
using DefaultAtomicPolicy = RAJA::omp_atomic;
#else
using DefaultAtomicPolicy = RAJA::seq_atomic;
#endif

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Kernel>
inline void
forall_exec(ExecPolicyTag,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    RAJA::forall<typename ExecPolicyTag::for_policy>(
        RAJA::RangeSegment(begin, end),
        std::forward<Kernel>(kernel));
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_ascending(SerialExec,
               Iterator begin,
               Iterator end) noexcept
{
    std::sort(begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_descending(SerialExec,
                Iterator begin,
                Iterator end) noexcept
{
    std::sort(begin, end, std::greater<>{});
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_ascending(ExecPolicyTag,
               Iterator begin,
               Iterator end) noexcept
{
    auto span = RAJA::make_span(begin, end - begin);
    // RAJA performs an ascending sort by default.
    RAJA::sort<typename ExecPolicyTag::sort_policy>(span);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_descending(ExecPolicyTag,
                Iterator begin,
                Iterator end) noexcept
{
    auto span = RAJA::make_span(begin, end - begin);
    RAJA::sort<typename ExecPolicyTag::sort_policy>(
        span,
        RAJA::operators::greater<typename std::iterator_traits<Iterator>::value_type>{});
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_add_exec(T *acc, T value)
{
    return RAJA::atomicAdd(DefaultAtomicPolicy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_min_exec(T *acc, T value)
{
    return RAJA::atomicMin(DefaultAtomicPolicy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_max_exec(T *acc, T value)
{
    return RAJA::atomicMax(DefaultAtomicPolicy{}, acc, value);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Reducers do not expose execution-tag selection at the API surface. Their
// implementation backend is chosen from translation-unit capability so users
// can use them directly inside forall(policy, ...) kernels.
template <typename T>
using ReduceSum = RAJA::ReduceSum<detail::DefaultReducePolicy, T>;

//-----------------------------------------------------------------------------
template <typename T>
using ReduceMin = RAJA::ReduceMin<detail::DefaultReducePolicy, T>;

//-----------------------------------------------------------------------------
template <typename T>
using ReduceMinLoc = RAJA::ReduceMinLoc<detail::DefaultReducePolicy, T>;

//-----------------------------------------------------------------------------
template <typename T>
using ReduceMax = RAJA::ReduceMax<detail::DefaultReducePolicy, T>;

//-----------------------------------------------------------------------------
template <typename T>
using ReduceMaxLoc = RAJA::ReduceMaxLoc<detail::DefaultReducePolicy, T>;

//---------------------------------------------------------------------------//
#else // !defined(CONDUIT_USE_RAJA)
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// RAJA_OFF detail backend/reducers for when raja is OFF
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
// -- begin conduit::execution::detail --
//-----------------------------------------------------------------------------
namespace detail
{
//-----------------------------------------------------------------------------
#if defined(CONDUIT_USE_OPENMP)
template <typename Kernel>
inline void
forall_exec(OpenMPExec,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    #pragma omp parallel for
    for (index_t i = begin; i < end; i ++)
    {
        kernel(i);
    }
}

//-----------------------------------------------------------------------------
// Parallel quicksort using OpenMP: tasks are spawned for
// at most ceil(log2(num_threads)) levels of recursion.
template <typename Iterator, typename Compare>
void
omp_quicksort(Iterator begin,
              Iterator end,
              Compare comp,
              int depth)
{
    if (depth == 0 || end - begin <= 1)
    {
        std::sort(begin, end, comp);
        return;
    }

    // Pivot selection.
    Iterator mid  = begin + (end - begin) / 2;
    Iterator last = end - 1;
    if (comp(*mid, *begin))
    {
        std::iter_swap(begin, mid);
    }
    if (comp(*last, *begin))
    {
        std::iter_swap(begin, last);
    }
    if (comp(*mid, *last))
    {
        std::iter_swap(mid, last);
    }

    // Copy pivot value so the lambda is safe after the iter_swap below.
    auto pivot = *last;
    Iterator split = std::partition(begin,
                                    last,
                                    [&](const auto &val){
                                        return comp(val, pivot);
                                    });
    std::iter_swap(split, last);

    // Recursively sort each partition.
    #pragma omp task
    omp_quicksort(begin, split, comp, depth - 1);
    #pragma omp task
    omp_quicksort(split + 1, end, comp, depth - 1);
    #pragma omp taskwait
}

//-----------------------------------------------------------------------------
// Computes recursion depth based on the number of threads.
inline int
get_thread_depth()
{
    return static_cast<int>(
        std::ceil(std::log2(static_cast<double>(omp_get_num_threads())))
    );
}

//-----------------------------------------------------------------------------
// Returns the minimum number of elements required to use the parallel
// quicksort. Matches the heuristic used by RAJA's OpenMP sort implementation.
inline int
get_sort_threshold()
{
    // 128 is what RAJA uses as the minimum number of elements per thread.
    const int min_elements_per_thread = 128;
    return min_elements_per_thread * omp_get_max_threads();
}

//-----------------------------------------------------------------------------
// TODO: Needs benchmarking and tuning.
template <typename Iterator>
inline void
sort_ascending(OpenMPExec,
               Iterator begin,
               Iterator end) noexcept
{
    if (end - begin < get_sort_threshold())
    {
        std::sort(begin, end);
        return;
    }
    #pragma omp parallel
    #pragma omp single nowait
    {
        const int depth = get_thread_depth();
        omp_quicksort(begin, end, std::less<>{}, depth);
    }
}

//-----------------------------------------------------------------------------
// TODO: Needs benchmarking and tuning.
template <typename Iterator>
inline void
sort_descending(OpenMPExec,
                Iterator begin,
                Iterator end) noexcept
{
    if (end - begin < get_sort_threshold())
    {
        std::sort(begin, end, std::greater<>{});
        return;
    }
    #pragma omp parallel
    #pragma omp single nowait
    {
        const int depth = get_thread_depth();
        omp_quicksort(begin, end, std::greater<>{}, depth);
    }
}
#endif // defined(CONDUIT_USE_OPENMP)

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Kernel>
inline void
forall_exec(ExecPolicyTag,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    for (int i = begin; i < end; i ++)
    {
        kernel(i);
    }
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_ascending(ExecPolicyTag,
               Iterator begin,
               Iterator end) noexcept
{
    std::sort(begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_descending(ExecPolicyTag,
                Iterator begin,
                Iterator end) noexcept
{
    std::sort(begin, end, std::greater<>{});
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_add_exec(T *acc, T value)
{
    T res;
#if defined(CONDUIT_USE_OPENMP)
    // atomic capture does both the read and write atomically
    #pragma omp atomic capture
    { 
#endif // defined(CONDUIT_USE_OPENMP)
        res = *acc;
        *acc += value;
#if defined(CONDUIT_USE_OPENMP)
    }
#endif // defined(CONDUIT_USE_OPENMP)
    return res;
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_min_exec(T *acc, T value)
{
    T res;
#if defined(CONDUIT_USE_OPENMP)
    // A conditional update can't use omp atomic, so we use a critical section instead
    #pragma omp critical(conduit_atomic_min)
    {
#endif // defined(CONDUIT_USE_OPENMP)
        res = *acc;
        if (value < *acc)
        {
            *acc = value;
        }
#if defined(CONDUIT_USE_OPENMP)
    }
#endif // defined(CONDUIT_USE_OPENMP)
    return res;
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_max_exec(T *acc, T value)
{
    T res;
#if defined(CONDUIT_USE_OPENMP)
    // A conditional update can't use omp atomic, so we use a critical section instead
    #pragma omp critical(conduit_atomic_max)
    {
#endif // defined(CONDUIT_USE_OPENMP)
        res = *acc;
        if (value > *acc)
        {
            *acc = value;
        }
#if defined(CONDUIT_USE_OPENMP)
    }
#endif // defined(CONDUIT_USE_OPENMP)
    return res;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Reducers are runtime-neutral; the RAJA-on and RAJA-off APIs match. The host
// fallback implementation does not need an execution tag.
template <typename T>
class ReduceSum
{
public:
    //-----------------------------------------------------------------------------
    ReduceSum()
    : m_value(0),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    explicit ReduceSum(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    ReduceSum(const ReduceSum &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void operator+=(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp atomic
#endif // defined(CONDUIT_USE_OPENMP)
        m_value_ptr[0] += value;
    }

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void sum(const T value) const
    {
        // Delegating to operator+= ensures thread safety for OpenMP
        operator+=(value);
    }

    //-----------------------------------------------------------------------------
    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMin
{
public:
    //-----------------------------------------------------------------------------
    ReduceMin()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    explicit ReduceMin(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    ReduceMin(const ReduceMin &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void min(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        // A conditional update can't use omp atomic, so we use a critical section instead
        // TODO: there may be a more efficient way to do this reduction
        #pragma omp critical(conduit_reduce_min)
#endif
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
    }

    //-----------------------------------------------------------------------------
    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMinLoc
{
public:
    //-----------------------------------------------------------------------------
    ReduceMinLoc()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    //-----------------------------------------------------------------------------
    ReduceMinLoc(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    //-----------------------------------------------------------------------------
    ReduceMinLoc(const ReduceMinLoc &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr),
      m_index(other.m_index),
      m_index_ptr(other.m_index_ptr)
    {}

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void minloc(const T value, index_t index) const
    {
#if defined(CONDUIT_USE_OPENMP)
        // A conditional update can't use omp atomic, so we use a critical section instead
        // TODO: there may be a more efficient way to do this reduction
        #pragma omp critical(conduit_reduce_minloc)
#endif
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = index;
        }
    }

    //-----------------------------------------------------------------------------
    T get()
    {
        return m_value_ptr[0];
    }

    //-----------------------------------------------------------------------------
    index_t getLoc()
    {
        return m_index_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
    index_t m_index;
    index_t *m_index_ptr;
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMax
{
public:
    //-----------------------------------------------------------------------------
    ReduceMax()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    explicit ReduceMax(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    //-----------------------------------------------------------------------------
    ReduceMax(const ReduceMax &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void max(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        // A conditional update can't use omp atomic, so we use a critical section instead
        // TODO: there may be a more efficient way to do this reduction
        #pragma omp critical(conduit_reduce_max)
#endif
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
    }

    //-----------------------------------------------------------------------------
    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMaxLoc
{
public:
    //-----------------------------------------------------------------------------
    ReduceMaxLoc()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    //-----------------------------------------------------------------------------
    ReduceMaxLoc(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    //-----------------------------------------------------------------------------
    ReduceMaxLoc(const ReduceMaxLoc &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr),
      m_index(other.m_index),
      m_index_ptr(other.m_index_ptr)
    {}

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void maxloc(const T value, index_t index) const
    {
#if defined(CONDUIT_USE_OPENMP)
        // A conditional update can't use omp atomic, so we use a critical section instead
        // TODO: there may be a more efficient way to do this reduction
        #pragma omp critical(conduit_reduce_maxloc)
#endif
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = index;
        }
    }

    //-----------------------------------------------------------------------------
    T get()
    {
        return m_value_ptr[0];
    }

    //-----------------------------------------------------------------------------
    index_t getLoc()
    {
        return m_index_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
    index_t m_index;
    index_t *m_index_ptr;
};

//---------------------------------------------------------------------------//
#endif // !defined(CONDUIT_USE_RAJA)
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
// end RAJA_ON/RAJA_OFF conditional
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Kernel>
inline void
forall(const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    detail::forall_exec(ExecPolicyTag{}, begin, end, std::forward<Kernel>(kernel));
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_ascending(Iterator begin,
               Iterator end) noexcept
{
    detail::sort_ascending(ExecPolicyTag{}, begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_descending(Iterator begin,
                Iterator end) noexcept
{
    detail::sort_descending(ExecPolicyTag{}, begin, end);
}

//-----------------------------------------------------------------------------
// Atomics are runtime-neutral at the API surface. Backend selection is chosen
// once from translation-unit capability so callers can use atomics directly
// inside forall(policy, ...) kernels.
template <typename T>
CONDUIT_EXEC T
atomic_add(T *acc, T value)
{
    return detail::atomic_add_exec(acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_min(T *acc, T value)
{
    return detail::atomic_min_exec(acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
CONDUIT_EXEC T
atomic_max(T *acc, T value)
{
    return detail::atomic_max_exec(acc, value);
}

//-----------------------------------------------------------------------------
template <typename Kernel>
inline void
forall(ExecutionPolicy &policy,
       const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;

    if (policy.is_serial())
    {
        forall<SerialExec>(begin, end, std::forward<Kernel>(kernel));
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_TU_IS_CUDA)
        forall<CudaExec>(begin, end, std::forward<Kernel>(kernel));
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_TU_IS_HIP)
        forall<HipExec>(begin, end, std::forward<Kernel>(kernel));
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        forall<OpenMPExec>(begin, end, std::forward<Kernel>(kernel));
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot call forall with an empty policy.");
    }
}

//-----------------------------------------------------------------------------
template <typename Iterator>
inline void
sort_ascending(ExecutionPolicy &policy,
               Iterator begin,
               Iterator end) noexcept
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    if (policy.is_serial())
    {
        sort_ascending<SerialExec>(begin, end);
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_TU_IS_CUDA)
        sort_ascending<CudaExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_TU_IS_HIP)
        sort_ascending<HipExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        sort_ascending<OpenMPExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot call sort_ascending with an empty policy.");
    }
}

//-----------------------------------------------------------------------------
template <typename Iterator>
inline void
sort_descending(ExecutionPolicy &policy,
                Iterator begin,
                Iterator end) noexcept
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    if (policy.is_serial())
    {
        sort_descending<SerialExec>(begin, end);
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_TU_IS_CUDA)
        sort_descending<CudaExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_TU_IS_HIP)
        sort_descending<HipExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        sort_descending<OpenMPExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot call sort_descending with an empty policy.");
    }
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
