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

namespace conduit
{
namespace execution
{

#if defined(CONDUIT_USE_RAJA)
namespace detail
{

template <typename ExecutionPolicy, typename Kernel>
inline void
forall_exec(ExecutionPolicy,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    RAJA::forall<typename ExecutionPolicy::for_policy>(
        RAJA::RangeSegment(begin, end),
        std::forward<Kernel>(kernel));
}

template <typename ExecutionPolicy, typename Iterator>
inline void
sort_exec(ExecutionPolicy,
          Iterator begin,
          Iterator end) noexcept
{
    std::sort(begin, end);
}

template <typename ExecutionPolicy, typename Iterator, typename Predicate>
inline void
sort_exec(ExecutionPolicy,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::sort(begin, end, std::forward<Predicate>(predicate));
}

} // namespace detail

#else
namespace detail
{

template <typename ExecutionPolicy, typename Kernel>
inline void
forall_exec(ExecutionPolicy,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    std::cout << typeid(ExecutionPolicy).name() << "  START" << std::endl;
    for (int i = begin; i < end; i ++)
    {
        kernel(i);
    }
    std::cout << typeid(ExecutionPolicy).name() << "  END" << std::endl;
}

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

template <typename ExecutionPolicy, typename Iterator>
inline void
sort_exec(ExecutionPolicy,
          Iterator begin,
          Iterator end) noexcept
{
    std::cout << typeid(ExecutionPolicy).name() << "  START" << std::endl;
    std::sort(begin, end);
    std::cout << typeid(ExecutionPolicy).name() << "  END" << std::endl;
}

template <typename ExecutionPolicy, typename Iterator, typename Predicate>
inline void
sort_exec(ExecutionPolicy,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
    std::cout << typeid(ExecutionPolicy).name() << "  START" << std::endl;
    std::sort(begin, end, predicate);
    std::cout << typeid(ExecutionPolicy).name() << "  END" << std::endl;
}

#if defined(CONDUIT_USE_OPENMP)
template <typename Iterator>
inline void
sort_exec(OpenMPExec,
          Iterator begin,
          Iterator end) noexcept
{
    std::sort(begin, end);
}

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

} // namespace detail
#endif

template <typename ExecutionPolicy, typename Kernel>
inline void
forall(const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    detail::forall_exec(ExecutionPolicy{}, begin, end, std::forward<Kernel>(kernel));
}

#if defined(CONDUIT_USE_RAJA)

template <typename ExecPolicy, typename T>
using ReduceSum = RAJA::ReduceSum<ExecPolicy,T>;

template <typename ExecPolicy, typename T>
using ReduceMin = RAJA::ReduceMin<ExecPolicy,T>;

template <typename ExecPolicy, typename T>
using ReduceMinLoc = RAJA::ReduceMinLoc<ExecPolicy,T>;

template <typename ExecPolicy, typename T>
using ReduceMax = RAJA::ReduceMax<ExecPolicy,T>;

template <typename ExecPolicy, typename T>
using ReduceMaxLoc = RAJA::ReduceMaxLoc<ExecPolicy,T>;

template <typename ExecPolicy, typename T>
CONDUIT_EXEC T atomic_add(T *acc, T value)
{
    return RAJA::atomicAdd(ExecPolicy{}, acc, value);
}

template <typename ExecPolicy, typename T>
CONDUIT_EXEC T atomic_min(T *acc, T value)
{
    return RAJA::atomicMin(ExecPolicy{}, acc, value);
}

template <typename ExecPolicy, typename T>
CONDUIT_EXEC T atomic_max(T *acc, T value)
{
    return RAJA::atomicMax(ExecPolicy{}, acc, value);
}

#else

template <typename ExecPolicy, typename T>
class ReduceSum
{
public:
    ReduceSum()
    : m_value(0),
      m_value_ptr(&m_value)
    {}

    ReduceSum(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceSum(const ReduceSum &v)
    : m_value(v.m_value),
      m_value_ptr(v.m_value_ptr)
    {}

    void operator+=(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp critical(conduit_execution_reduce_sum)
        {
            m_value_ptr[0] += value;
        }
#else
        m_value_ptr[0] += value;
#endif
    }

    void sum(const T value) const
    {
        operator+=(value);
    }

    T get() const
    {
        return m_value_ptr[0];
    }

private:
    T  m_value;
    T *m_value_ptr;
};

template <typename ExecPolicy, typename T>
class ReduceMin
{
public:
    ReduceMin()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value)
    {}

    ReduceMin(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceMin(const ReduceMin &v)
    : m_value(v.m_value),
      m_value_ptr(v.m_value_ptr)
    {}

    void min(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp critical(conduit_execution_reduce_min)
        {
            if (value < m_value_ptr[0])
            {
                m_value_ptr[0] = value;
            }
        }
#else
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
#endif
    }

    T get() const
    {
        return m_value_ptr[0];
    }

private:
    T  m_value;
    T *m_value_ptr;
};

template <typename ExecPolicy, typename T>
class ReduceMinLoc
{
public:
    ReduceMinLoc()
    : m_value(std::numeric_limits<T>::max()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    ReduceMinLoc(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    ReduceMinLoc(const ReduceMinLoc &v)
    : m_value(v.m_value),
      m_value_ptr(v.m_value_ptr),
      m_index(v.m_index),
      m_index_ptr(v.m_index_ptr)
    {}

    void minloc(const T value, index_t i) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp critical(conduit_execution_reduce_minloc)
        {
            if (value < m_value_ptr[0])
            {
                m_value_ptr[0] = value;
                m_index_ptr[0] = i;
            }
        }
#else
        if (value < m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = i;
        }
#endif
    }

    T get() const
    {
        return m_value_ptr[0];
    }

    index_t getLoc() const
    {
        return m_index_ptr[0];
    }

private:
    T        m_value;
    T       *m_value_ptr;
    index_t  m_index;
    index_t *m_index_ptr;
};

template <typename ExecPolicy, typename T>
class ReduceMax
{
public:
    ReduceMax()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value)
    {}

    ReduceMax(T v_start)
    : m_value(v_start),
      m_value_ptr(&m_value)
    {}

    ReduceMax(const ReduceMax &v)
    : m_value(v.m_value),
      m_value_ptr(v.m_value_ptr)
    {}

    void max(const T value) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp critical(conduit_execution_reduce_max)
        {
            if (value > m_value_ptr[0])
            {
                m_value_ptr[0] = value;
            }
        }
#else
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
        }
#endif
    }

    T get() const
    {
        return m_value_ptr[0];
    }

private:
    T  m_value;
    T *m_value_ptr;
};

template <typename ExecPolicy, typename T>
class ReduceMaxLoc
{
public:
    ReduceMaxLoc()
    : m_value(std::numeric_limits<T>::lowest()),
      m_value_ptr(&m_value),
      m_index(-1),
      m_index_ptr(&m_index)
    {}

    ReduceMaxLoc(T v_start, index_t i_start)
    : m_value(v_start),
      m_value_ptr(&m_value),
      m_index(i_start),
      m_index_ptr(&m_index)
    {}

    ReduceMaxLoc(const ReduceMaxLoc &v)
    : m_value(v.m_value),
      m_value_ptr(v.m_value_ptr),
      m_index(v.m_index),
      m_index_ptr(v.m_index_ptr)
    {}

    void maxloc(const T value, index_t i) const
    {
#if defined(CONDUIT_USE_OPENMP)
        #pragma omp critical(conduit_execution_reduce_maxloc)
        {
            if (value > m_value_ptr[0])
            {
                m_value_ptr[0] = value;
                m_index_ptr[0] = i;
            }
        }
#else
        if (value > m_value_ptr[0])
        {
            m_value_ptr[0] = value;
            m_index_ptr[0] = i;
        }
#endif
    }

    T get() const
    {
        return m_value_ptr[0];
    }

    index_t getLoc() const
    {
        return m_index_ptr[0];
    }

private:
    T        m_value;
    T       *m_value_ptr;
    index_t  m_index;
    index_t *m_index_ptr;
};

template <typename ExecPolicy, typename T>
inline T atomic_add(T *acc, T value)
{
#if defined(CONDUIT_USE_OPENMP)
    T res;
    #pragma omp atomic capture
    {
        res = *acc;
        *acc += value;
    }
    return res;
#else
    T res = *acc;
    *acc += value;
    return res;
#endif
}

template <typename ExecPolicy, typename T>
inline T atomic_min(T *acc, T value)
{
#if defined(CONDUIT_USE_OPENMP)
    T res;
    #pragma omp critical(conduit_execution_atomic_min)
    {
        res = *acc;
        *acc = std::min(*acc, value);
    }
    return res;
#else
    T res = *acc;
    *acc = std::min(*acc, value);
    return res;
#endif
}

template <typename ExecPolicy, typename T>
inline T atomic_max(T *acc, T value)
{
#if defined(CONDUIT_USE_OPENMP)
    T res;
    #pragma omp critical(conduit_execution_atomic_max)
    {
        res = *acc;
        *acc = std::max(*acc, value);
    }
    return res;
#else
    T res = *acc;
    *acc = std::max(*acc, value);
    return res;
#endif
}

#endif

template <typename ExecutionPolicy, typename Iterator>
inline void
sort(Iterator begin,
     Iterator end) noexcept
{
    detail::sort_exec(ExecutionPolicy{}, begin, end);
}

template <typename ExecutionPolicy, typename Iterator, typename Predicate>
inline void
sort(Iterator begin,
     Iterator end,
     Predicate &&predicate) noexcept
{
    detail::sort_exec(ExecutionPolicy{}, begin, end, std::forward<Predicate>(predicate));
}

template <typename ExecPolicyTag, typename Function>
inline void invoke(ExecPolicyTag &exec, Function&& func) noexcept
{
    func(exec);
}

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
}

#endif
