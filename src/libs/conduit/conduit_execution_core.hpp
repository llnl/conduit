// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_core.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_CORE_HPP
#define CONDUIT_EXECUTION_CORE_HPP

#include "conduit_execution_policy.hpp"

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

#if defined(CONDUIT_USE_RAJA)
//-----------------------------------------------------------------------------
// -- begin conduit::execution::detail --
//-----------------------------------------------------------------------------
namespace detail
{

template <typename ExecPolicyTag, typename T>
using ReduceSumImpl = RAJA::ReduceSum<typename ExecPolicyTag::reduce_policy, T>;

template <typename ExecPolicyTag, typename T>
using ReduceMinImpl = RAJA::ReduceMin<typename ExecPolicyTag::reduce_policy, T>;

template <typename ExecPolicyTag, typename T>
using ReduceMinLocImpl = RAJA::ReduceMinLoc<typename ExecPolicyTag::reduce_policy, T>;

template <typename ExecPolicyTag, typename T>
using ReduceMaxImpl = RAJA::ReduceMax<typename ExecPolicyTag::reduce_policy, T>;

template <typename ExecPolicyTag, typename T>
using ReduceMaxLocImpl = RAJA::ReduceMaxLoc<typename ExecPolicyTag::reduce_policy, T>;

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
EXEC_LAMBDA T
atomic_add_exec(T *acc, T value)
{
    return RAJA::atomicAdd(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
EXEC_LAMBDA T
atomic_min_exec(T *acc, T value)
{
    return RAJA::atomicMin(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
EXEC_LAMBDA T
atomic_max_exec(T *acc, T value)
{
    return RAJA::atomicMax(typename ExecPolicyTag::atomic_policy{}, acc, value);
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution::detail --
//-----------------------------------------------------------------------------

#else
//-----------------------------------------------------------------------------
// -- begin conduit::execution::detail --
//-----------------------------------------------------------------------------
namespace detail
{

template <typename ExecPolicyTag, typename T>
class ReduceSumImpl
{
public:
    ReduceSumImpl()
    : m_value(0),
      m_value_ptr(&m_value)
    {}

    explicit ReduceSumImpl(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceSumImpl(const ReduceSumImpl &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    EXEC_LAMBDA void operator+=(const T value) const
    {
        m_value_ptr[0] += value;
    }

    EXEC_LAMBDA void sum(const T value) const
    {
        m_value_ptr[0] += value;
    }

    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

template <typename ExecPolicyTag, typename T>
class ReduceMinImpl
{
public:
    ReduceMinImpl()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value)
    {}

    explicit ReduceMinImpl(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceMinImpl(const ReduceMinImpl &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    EXEC_LAMBDA void min(const T value) const
    {
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
    }

    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

template <typename ExecPolicyTag, typename T>
class ReduceMinLocImpl
{
public:
    ReduceMinLocImpl()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    ReduceMinLocImpl(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    ReduceMinLocImpl(const ReduceMinLocImpl &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr),
      m_index(other.m_index),
      m_index_ptr(other.m_index_ptr)
    {}

    EXEC_LAMBDA void minloc(const T value, index_t index) const
    {
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = index;
        }
    }

    T get()
    {
        return m_value_ptr[0];
    }

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

template <typename ExecPolicyTag, typename T>
class ReduceMaxImpl
{
public:
    ReduceMaxImpl()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value)
    {}

    explicit ReduceMaxImpl(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceMaxImpl(const ReduceMaxImpl &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr)
    {}

    EXEC_LAMBDA void max(const T value) const
    {
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
    }

    T get()
    {
        return m_value_ptr[0];
    }

private:
    T m_value;
    T *m_value_ptr;
};

template <typename ExecPolicyTag, typename T>
class ReduceMaxLocImpl
{
public:
    ReduceMaxLocImpl()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    ReduceMaxLocImpl(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    ReduceMaxLocImpl(const ReduceMaxLocImpl &other)
    : m_value(other.m_value),
      m_value_ptr(other.m_value_ptr),
      m_index(other.m_index),
      m_index_ptr(other.m_index_ptr)
    {}

    EXEC_LAMBDA void maxloc(const T value, index_t index) const
    {
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = index;
        }
    }

    T get()
    {
        return m_value_ptr[0];
    }

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
template <typename ExecPolicyTag, typename Kernel>
inline void
forall_exec(ExecPolicyTag,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    std::cout << typeid(ExecPolicyTag).name() << "  START" << std::endl;
    for (int i = begin; i < end; i ++)
    {
        kernel(i);
    }
    std::cout << typeid(ExecPolicyTag).name() << "  END" << std::endl;
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
    std::cout << typeid(ExecPolicyTag).name() << "  START" << std::endl;
    std::sort(begin, end);
    std::cout << typeid(ExecPolicyTag).name() << "  END" << std::endl;
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Iterator, typename Predicate>
inline void
sort_exec(ExecPolicyTag,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::cout << typeid(ExecPolicyTag).name() << "  START" << std::endl;
    std::sort(begin, end, predicate);
    std::cout << typeid(ExecPolicyTag).name() << "  END" << std::endl;
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
EXEC_LAMBDA T
atomic_add_exec(T *acc, T value)
{
    T res = *acc;
    *acc += value;
    return res;
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
EXEC_LAMBDA T
atomic_min_exec(T *acc, T value)
{
    T res = *acc;
    *acc = value < *acc ? value : *acc;
    return res;
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename T>
EXEC_LAMBDA T
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
#endif

//-----------------------------------------------------------------------------
inline void
validate_runtime_policy(const ExecutionPolicy &policy,
                        const char *context)
{
    if (policy.is_empty())
    {
        CONDUIT_ERROR(context << " does not support an empty policy.");
    }

    if (policy.is_openmp())
    {
#if !defined(CONDUIT_USE_OPENMP)
        CONDUIT_ERROR(context << " requires OpenMP support in this translation unit.");
#endif
    }
    else if (policy.is_cuda())
    {
#if !defined(CONDUIT_EXEC_TU_HAS_CUDA)
        CONDUIT_ERROR(context << " requires CUDA support in this translation unit.");
#endif
    }
    else if (policy.is_hip())
    {
#if !defined(CONDUIT_EXEC_TU_HAS_HIP)
        CONDUIT_ERROR(context << " requires HIP support in this translation unit.");
#endif
    }
}

//-----------------------------------------------------------------------------
template <typename T>
class ReduceSum
{
public:
    //-----------------------------------------------------------------------------
    explicit ReduceSum(ExecutionPolicy policy)
    : ReduceSum(policy, T(0))
    {}

    //-----------------------------------------------------------------------------
    ReduceSum(ExecutionPolicy policy, T v_start)
    : m_policy_id(policy.policy_id()),
      m_serial_reduce(v_start)
#if defined(CONDUIT_USE_OPENMP)
    , m_openmp_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    , m_cuda_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    , m_hip_reduce(v_start)
#endif
    {
        validate_runtime_policy(policy, "ReduceSum");
    }

    EXEC_LAMBDA void operator+=(const T value) const
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            m_openmp_reduce += value;
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            m_cuda_reduce += value;
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            m_hip_reduce += value;
            return;
#endif
        }

        m_serial_reduce += value;
    }

    EXEC_LAMBDA void sum(const T value) const
    {
        (*this) += value;
    }

    T get()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.get();
#endif
        }

        return m_serial_reduce.get();
    }

private:
    ExecutionPolicy::PolicyID m_policy_id;
    detail::ReduceSumImpl<SerialExec, T> m_serial_reduce;
#if defined(CONDUIT_USE_OPENMP)
    detail::ReduceSumImpl<OpenMPExec, T> m_openmp_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    detail::ReduceSumImpl<CudaExec, T> m_cuda_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    detail::ReduceSumImpl<HipExec, T> m_hip_reduce;
#endif
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMin
{
public:
    //-----------------------------------------------------------------------------
    explicit ReduceMin(ExecutionPolicy policy)
    : ReduceMin(policy, std::numeric_limits<T>::max())
    {}

    //-----------------------------------------------------------------------------
    ReduceMin(ExecutionPolicy policy, T v_start)
    : m_policy_id(policy.policy_id()),
      m_serial_reduce(v_start)
#if defined(CONDUIT_USE_OPENMP)
    , m_openmp_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    , m_cuda_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    , m_hip_reduce(v_start)
#endif
    {
        validate_runtime_policy(policy, "ReduceMin");
    }

    EXEC_LAMBDA void min(const T value) const
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            m_openmp_reduce.min(value);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            m_cuda_reduce.min(value);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            m_hip_reduce.min(value);
            return;
#endif
        }

        m_serial_reduce.min(value);
    }

    T get()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.get();
#endif
        }

        return m_serial_reduce.get();
    }

private:
    ExecutionPolicy::PolicyID m_policy_id;
    detail::ReduceMinImpl<SerialExec, T> m_serial_reduce;
#if defined(CONDUIT_USE_OPENMP)
    detail::ReduceMinImpl<OpenMPExec, T> m_openmp_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    detail::ReduceMinImpl<CudaExec, T> m_cuda_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    detail::ReduceMinImpl<HipExec, T> m_hip_reduce;
#endif
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMinLoc
{
public:
    //-----------------------------------------------------------------------------
    explicit ReduceMinLoc(ExecutionPolicy policy)
    : ReduceMinLoc(policy, std::numeric_limits<T>::max(), -1)
    {}

    //-----------------------------------------------------------------------------
    ReduceMinLoc(ExecutionPolicy policy, T v_start, index_t i_start)
    : m_policy_id(policy.policy_id()),
      m_serial_reduce(v_start, i_start)
#if defined(CONDUIT_USE_OPENMP)
    , m_openmp_reduce(v_start, i_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    , m_cuda_reduce(v_start, i_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    , m_hip_reduce(v_start, i_start)
#endif
    {
        validate_runtime_policy(policy, "ReduceMinLoc");
    }

    EXEC_LAMBDA void minloc(const T value, index_t index) const
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            m_openmp_reduce.minloc(value, index);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            m_cuda_reduce.minloc(value, index);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            m_hip_reduce.minloc(value, index);
            return;
#endif
        }

        m_serial_reduce.minloc(value, index);
    }

    T get()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.get();
#endif
        }

        return m_serial_reduce.get();
    }

    index_t getLoc()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.getLoc();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.getLoc();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.getLoc();
#endif
        }

        return m_serial_reduce.getLoc();
    }

private:
    ExecutionPolicy::PolicyID m_policy_id;
    detail::ReduceMinLocImpl<SerialExec, T> m_serial_reduce;
#if defined(CONDUIT_USE_OPENMP)
    detail::ReduceMinLocImpl<OpenMPExec, T> m_openmp_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    detail::ReduceMinLocImpl<CudaExec, T> m_cuda_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    detail::ReduceMinLocImpl<HipExec, T> m_hip_reduce;
#endif
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMax
{
public:
    //-----------------------------------------------------------------------------
    explicit ReduceMax(ExecutionPolicy policy)
    : ReduceMax(policy, std::numeric_limits<T>::lowest())
    {}

    //-----------------------------------------------------------------------------
    ReduceMax(ExecutionPolicy policy, T v_start)
    : m_policy_id(policy.policy_id()),
      m_serial_reduce(v_start)
#if defined(CONDUIT_USE_OPENMP)
    , m_openmp_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    , m_cuda_reduce(v_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    , m_hip_reduce(v_start)
#endif
    {
        validate_runtime_policy(policy, "ReduceMax");
    }

    EXEC_LAMBDA void max(const T value) const
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            m_openmp_reduce.max(value);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            m_cuda_reduce.max(value);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            m_hip_reduce.max(value);
            return;
#endif
        }

        m_serial_reduce.max(value);
    }

    T get()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.get();
#endif
        }

        return m_serial_reduce.get();
    }

private:
    ExecutionPolicy::PolicyID m_policy_id;
    detail::ReduceMaxImpl<SerialExec, T> m_serial_reduce;
#if defined(CONDUIT_USE_OPENMP)
    detail::ReduceMaxImpl<OpenMPExec, T> m_openmp_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    detail::ReduceMaxImpl<CudaExec, T> m_cuda_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    detail::ReduceMaxImpl<HipExec, T> m_hip_reduce;
#endif
};

//-----------------------------------------------------------------------------
template <typename T>
class ReduceMaxLoc
{
public:
    //-----------------------------------------------------------------------------
    explicit ReduceMaxLoc(ExecutionPolicy policy)
    : ReduceMaxLoc(policy, std::numeric_limits<T>::lowest(), -1)
    {}

    //-----------------------------------------------------------------------------
    ReduceMaxLoc(ExecutionPolicy policy, T v_start, index_t i_start)
    : m_policy_id(policy.policy_id()),
      m_serial_reduce(v_start, i_start)
#if defined(CONDUIT_USE_OPENMP)
    , m_openmp_reduce(v_start, i_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    , m_cuda_reduce(v_start, i_start)
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    , m_hip_reduce(v_start, i_start)
#endif
    {
        validate_runtime_policy(policy, "ReduceMaxLoc");
    }

    EXEC_LAMBDA void maxloc(const T value, index_t index) const
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            m_openmp_reduce.maxloc(value, index);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            m_cuda_reduce.maxloc(value, index);
            return;
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            m_hip_reduce.maxloc(value, index);
            return;
#endif
        }

        m_serial_reduce.maxloc(value, index);
    }

    T get()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.get();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.get();
#endif
        }

        return m_serial_reduce.get();
    }

    index_t getLoc()
    {
        if (m_policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
        {
#if defined(CONDUIT_USE_OPENMP)
            return m_openmp_reduce.getLoc();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
            return m_cuda_reduce.getLoc();
#endif
        }
        if (m_policy_id == ExecutionPolicy::PolicyID::HIP_ID)
        {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
            return m_hip_reduce.getLoc();
#endif
        }

        return m_serial_reduce.getLoc();
    }

private:
    ExecutionPolicy::PolicyID m_policy_id;
    detail::ReduceMaxLocImpl<SerialExec, T> m_serial_reduce;
#if defined(CONDUIT_USE_OPENMP)
    detail::ReduceMaxLocImpl<OpenMPExec, T> m_openmp_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    detail::ReduceMaxLocImpl<CudaExec, T> m_cuda_reduce;
#endif
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
    detail::ReduceMaxLocImpl<HipExec, T> m_hip_reduce;
#endif
};

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
template <typename T>
EXEC_LAMBDA T
atomic_add(ExecutionPolicy policy, T *acc, T value)
{
#if !defined(__CUDA_ARCH__) && !defined(__HIP_DEVICE_COMPILE__)
    validate_runtime_policy(policy, "atomic_add");
#endif
    const auto policy_id = policy.policy_id();
    if (policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
    {
#if defined(CONDUIT_USE_OPENMP)
        return detail::atomic_add_exec<OpenMPExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
        return detail::atomic_add_exec<CudaExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::HIP_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
        return detail::atomic_add_exec<HipExec>(acc, value);
#endif
    }

    return detail::atomic_add_exec<SerialExec>(acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
EXEC_LAMBDA T
atomic_min(ExecutionPolicy policy, T *acc, T value)
{
#if !defined(__CUDA_ARCH__) && !defined(__HIP_DEVICE_COMPILE__)
    validate_runtime_policy(policy, "atomic_min");
#endif
    const auto policy_id = policy.policy_id();
    if (policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
    {
#if defined(CONDUIT_USE_OPENMP)
        return detail::atomic_min_exec<OpenMPExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
        return detail::atomic_min_exec<CudaExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::HIP_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
        return detail::atomic_min_exec<HipExec>(acc, value);
#endif
    }

    return detail::atomic_min_exec<SerialExec>(acc, value);
}

//-----------------------------------------------------------------------------
template <typename T>
EXEC_LAMBDA T
atomic_max(ExecutionPolicy policy, T *acc, T value)
{
#if !defined(__CUDA_ARCH__) && !defined(__HIP_DEVICE_COMPILE__)
    validate_runtime_policy(policy, "atomic_max");
#endif
    const auto policy_id = policy.policy_id();
    if (policy_id == ExecutionPolicy::PolicyID::OPENMP_ID)
    {
#if defined(CONDUIT_USE_OPENMP)
        return detail::atomic_max_exec<OpenMPExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::CUDA_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
        return detail::atomic_max_exec<CudaExec>(acc, value);
#endif
    }
    if (policy_id == ExecutionPolicy::PolicyID::HIP_ID)
    {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
        return detail::atomic_max_exec<HipExec>(acc, value);
#endif
    }

    return detail::atomic_max_exec<SerialExec>(acc, value);
}

//-----------------------------------------------------------------------------
template <typename ExecPolicyTag, typename Function>
inline void invoke(ExecPolicyTag &exec_policy_tag, Function&& func) noexcept
{
    func(exec_policy_tag);
}

//-----------------------------------------------------------------------------
template <typename Function>
void
dispatch(ExecutionPolicy policy, Function&& func)
{
    if (policy.is_serial())
    {
        SerialExec se;
        invoke(se, func);
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
        CudaExec ce;
        invoke(ce, func);
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
        HipExec he;
        invoke(he, func);
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_USE_OPENMP)
        OpenMPExec ompe;
        invoke(ompe, func);
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
    if (policy.is_serial())
    {
        forall<SerialExec>(begin, end, std::forward<Kernel>(kernel));
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
        forall<CudaExec>(begin, end, std::forward<Kernel>(kernel));
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_EXEC_TU_HAS_HIP)
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
