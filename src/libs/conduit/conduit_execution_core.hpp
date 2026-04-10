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
