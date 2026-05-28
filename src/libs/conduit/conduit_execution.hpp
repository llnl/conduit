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

// 
// Macro disambiguation:
// 

// 1. CONDUIT_USE_CUDA/CONDUIT_USE_HIP: our build enabled these
// backends.

// 2. CONDUIT_USE_DEVICE: we are using CUDA or HIP and UMPIRE is
// enabled.
#if (defined(CONDUIT_USE_CUDA) || defined(CONDUIT_USE_HIP)) && defined(CONDUIT_USE_UMPIRE)
#define CONDUIT_USE_DEVICE
#endif

// 3. CONDUIT_TU_IS_CUDA/CONDUIT_TU_IS_HIP: our current translation
// unit is a CUDA/HIP target. We cannot get away with just using
// CONDUIT_USE_*** because execution is included broadly, and
// normal host-only TUs will not compile symbols like
// RAJA::cuda_exec/RAJA::hip_exec.
#if defined(CONDUIT_USE_CUDA) && defined(__CUDACC__)
#define CONDUIT_TU_IS_CUDA
#endif

#if defined(CONDUIT_USE_HIP) && defined(__HIPCC__)
#define CONDUIT_TU_IS_HIP
#endif

// 4. CONDUIT_DEVICE_COMPILE: means the compiler is compiling
// for the device right now (typically there is a host compilation
// pass and a device pass). This is useful for having different
// behavior for host and device (like for error-handling).
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define CONDUIT_DEVICE_COMPILE
#endif

// 5. CONDUIT_EXEC: host/device function decorator. For a
// CUDA/HIP TU, any function marked with this is compiled for both
// host and device, while for a normal C++ TU it means nothing.
#if defined(CONDUIT_TU_IS_CUDA) || defined(CONDUIT_TU_IS_HIP)
#define CONDUIT_EXEC __host__ __device__
#else
#define CONDUIT_EXEC
#endif

// 6. CONDUIT_DEVICE_ERROR_CHECK: error checking macro
#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

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
/// Get the execution policy option.
//-----------------------------------------------------------------------------
const std::string& get_execution_policy_option();

//-----------------------------------------------------------------------------
/// Get an execution policy based on the policy option.
//-----------------------------------------------------------------------------
ExecutionPolicy get_execution_policy();

//-----------------------------------------------------------------------------
/// Get the output allocator option.
//-----------------------------------------------------------------------------
const std::string& get_output_allocator_option();

//-----------------------------------------------------------------------------
/// Get the output allocator id based on the allocator option.
//-----------------------------------------------------------------------------
index_t get_output_allocator_id();

//-----------------------------------------------------------------------------
/// Get the sync strategy option.
//-----------------------------------------------------------------------------
const std::string& get_sync_strategy_option();

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
// Reducers are typed on the execution tag so the selected backend is concrete
// before the reducer is captured into a forall kernel. Each reducer extracts
// ExecPolicyTag::reduce_policy internally so call sites stay at the Exec level.
template <typename ExecPolicyTag, typename T>
using ReduceSum = RAJA::ReduceSum<typename ExecPolicyTag::reduce_policy, T>;

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
using ReduceMin = RAJA::ReduceMin<typename ExecPolicyTag::reduce_policy, T>;

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
using ReduceMinLoc = RAJA::ReduceMinLoc<typename ExecPolicyTag::reduce_policy, T>;

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
using ReduceMax = RAJA::ReduceMax<typename ExecPolicyTag::reduce_policy, T>;

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
using ReduceMaxLoc = RAJA::ReduceMaxLoc<typename ExecPolicyTag::reduce_policy, T>;

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
// Reducers are typed on the execution tag so the selected backend is concrete
// before the reducer is captured into a forall kernel. Each reducer extracts
// ExecPolicyTag::reduce_policy internally so call sites stay at the Exec level.

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
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
template <typename ExecPolicyTag, typename T>
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
template <typename ExecPolicyTag, typename T>
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
template <typename ExecPolicyTag, typename T>
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
template <typename ExecPolicyTag, typename T>
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
