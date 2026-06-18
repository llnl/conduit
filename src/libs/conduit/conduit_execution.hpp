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

// CONDUIT_DEVICE_ERROR_CHECK: error checking macro
#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

#include "conduit_execution_macros.hpp"
#include "conduit_execution_policy.hpp"
#include "conduit_annotations.hpp"

#include <algorithm>
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
void execution_set_options(const Node &opts);

//-----------------------------------------------------------------------------
/// Get a Node that contains execution options.
//-----------------------------------------------------------------------------
void execution_options(Node &opts);

//-----------------------------------------------------------------------------
/// Reset execution options to their default values.
//-----------------------------------------------------------------------------
void reset_execution_options();

//-----------------------------------------------------------------------------
/// Get an execution policy based on the policy option.
//-----------------------------------------------------------------------------
ExecutionPolicy get_execution_policy(Node &src_node);

//-----------------------------------------------------------------------------
/// Get an execution policy based on the policy option.
//-----------------------------------------------------------------------------
ExecutionPolicy get_execution_policy();

//-----------------------------------------------------------------------------
/// Get the output allocator id based on the allocator option.
//-----------------------------------------------------------------------------
index_t get_output_allocator_id();

//-----------------------------------------------------------------------------
/// Get the output allocator id based on the allocator option.
//-----------------------------------------------------------------------------
index_t get_output_allocator_id(Node &src_node);

//-----------------------------------------------------------------------------
/// Get the sync strategy option.
//-----------------------------------------------------------------------------
const std::string& get_sync_strategy();

//-----------------------------------------------------------------------------
/// Get the device allocator id.
//-----------------------------------------------------------------------------
index_t get_device_allocator_id();

//-----------------------------------------------------------------------------
/// Get the host allocator id.
//-----------------------------------------------------------------------------
index_t get_host_allocator_id();


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
#else
using DefaultReducePolicy = RAJA::seq_reduce;
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
sort_exec(ExecPolicyTag,
          Iterator begin,
          Iterator end) noexcept
{
    std::sort(begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator, typename Predicate>
inline void
sort_exec(ExecPolicyTag,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::sort(begin, end, std::forward<Predicate>(predicate));
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_add_exec(T *acc, T value)
{
    return RAJA::atomicAdd(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_min_exec(T *acc, T value)
{
    return RAJA::atomicMin(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_max_exec(T *acc, T value)
{
    return RAJA::atomicMax(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Reducers no longer expose execution-tag selection at the API surface. Their
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
#else
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
#endif

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator>
inline void
sort_exec(ExecPolicyTag,
          Iterator begin,
          Iterator end) noexcept
{
    std::sort(begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator, typename Predicate>
inline void
sort_exec(ExecPolicyTag,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::sort(begin, end, predicate);
}

//-----------------------------------------------------------------------------
#if defined(CONDUIT_USE_OPENMP)
template <typename Iterator>
inline void
sort_exec(OpenMPExec,
          Iterator begin,
          Iterator end) noexcept
{
    std::sort(begin, end);
}

//-----------------------------------------------------------------------------
template <typename Iterator, typename Predicate>
inline void
sort_exec(OpenMPExec,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::sort(begin, end);
}
#endif

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_add_exec(T *acc, T value)
{
    T res = *acc;
    *acc += value;
    return res;
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_min_exec(T *acc, T value)
{
    T res = *acc;
    *acc = value < *acc ? value : *acc;
    return res;
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_max_exec(T *acc, T value)
{
    T res = *acc;
    *acc = value > *acc ? value : *acc;
    return res;
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Reducers stay runtime-neutral here too so the RAJA-on and RAJA-off APIs
// match. The host fallback implementation does not need an execution tag.
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
        m_value_ptr[0] += value;
    }

    //-----------------------------------------------------------------------------
    CONDUIT_EXEC void sum(const T value) const
    {
        m_value_ptr[0] += value;
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
#endif
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
sort(Iterator begin,
     Iterator end) noexcept
{
    detail::sort_exec(ExecPolicyTag{}, begin, end);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator, typename Predicate>
inline void
sort(Iterator begin,
     Iterator end,
     Predicate &&predicate) noexcept
{
    detail::sort_exec(ExecPolicyTag{}, begin, end, std::forward<Predicate>(predicate));
}

//-----------------------------------------------------------------------------
// Atomics are typed on the execution tag so callers must resolve the runtime
// policy before entering the hot loop. That keeps backend selection out of the
// atomic call site and lets RAJA instantiate the correct atomic policy once.
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_add(T *acc, T value)
{
    return detail::atomic_add_exec<ExecPolicyTag>(acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_min(T *acc, T value)
{
    return detail::atomic_min_exec<ExecPolicyTag>(acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
CONDUIT_EXEC T
atomic_max(T *acc, T value)
{
    return detail::atomic_max_exec<ExecPolicyTag>(acc, value);
}

//-----------------------------------------------------------------------------
// dispatch() converts a runtime policy object into a compile-time backend tag type
// We use std::forward so dispatch preserves whether the callable was passed as an
// lvalue or rvalue, rather than always treating it as an lvalue inside the function.
template <typename Function>
void
dispatch(ExecutionPolicy policy, Function&& func)
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    if (policy.is_serial())
    {
        SerialExec se;
        std::forward<Function>(func)(se);
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_TU_IS_CUDA)
        CudaExec ce;
        std::forward<Function>(func)(ce);
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_TU_IS_HIP)
        HipExec he;
        std::forward<Function>(func)(he);
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        OpenMPExec ompe;
        std::forward<Function>(func)(ompe);
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot invoke with an empty policy.");
    }
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
sort(ExecutionPolicy &policy,
     Iterator begin,
     Iterator end) noexcept
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    if (policy.is_serial())
    {
        sort<SerialExec>(begin, end);
    }
    else if (policy.is_cuda())
    {
        CONDUIT_ERROR("sort does not exist for CUDA.");
    }
    else if (policy.is_hip())
    {
        CONDUIT_ERROR("sort does not exist for HIP.");
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        sort<OpenMPExec>(begin, end);
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot call sort with an empty policy.");
    }
}

//-----------------------------------------------------------------------------
template <typename Iterator, typename Predicate>
inline void
sort(ExecutionPolicy &policy,
     Iterator begin,
     Iterator end,
     Predicate &&predicate) noexcept
{
    CONDUIT_ANNOTATE_MARK_FUNCTION;
    if (policy.is_serial())
    {
        sort<SerialExec>(begin, end, std::forward<Predicate>(predicate));
    }
    else if (policy.is_cuda())
    {
        CONDUIT_ERROR("sort does not exist for CUDA.");
    }
    else if (policy.is_hip())
    {
        CONDUIT_ERROR("sort does not exist for HIP.");
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        sort<OpenMPExec>(begin, end, std::forward<Predicate>(predicate));
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else
    {
        CONDUIT_ERROR("Cannot call sort with an empty policy.");
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
