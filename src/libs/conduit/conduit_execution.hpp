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


//-----------------------------------------------------------------------------
// conduit lib includes
//-----------------------------------------------------------------------------
#include "conduit_config.h"
#include "conduit_utils.hpp"
#include "conduit_memory_manager.hpp"

#if defined(CONDUIT_USE_OPENMP)
#include <omp.h>
#endif

#if defined(CONDUIT_USE_RAJA)
#include <RAJA/RAJA.hpp>
#endif

//-----------------------------------------------------------------------------
// cpp lib includes
//-----------------------------------------------------------------------------
#include <algorithm>
#include <utility>

//-----------------------------------------------------------------------------
//
/// The CONDUIT_DEVICE_ERROR_CHECK macro is TODO
//
//-----------------------------------------------------------------------------
#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_CUDA) && defined(RAJA_CUDA_ACTIVE)
#define CONDUIT_RAJA_CUDA_ACTIVE
#endif

#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_HIP) && defined(RAJA_HIP_ACTIVE)
#define CONDUIT_RAJA_HIP_ACTIVE
#endif

#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP) && defined(RAJA_ENABLE_OPENMP)
#define CONDUIT_RAJA_OPENMP_ACTIVE
#endif

#if defined(CONDUIT_RAJA_CUDA_ACTIVE) || defined(CONDUIT_RAJA_HIP_ACTIVE)
#define EXEC_LAMBDA __device__ __host__
#else
#define EXEC_LAMBDA 
#endif

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
// -- begin conduit::execution::ExecutionPolicy --
//-----------------------------------------------------------------------------
///
/// class: conduit::execution::ExecutionPolicy
///
/// description:
///  ExecutionPolicy is a runtime policy object.
///
//-----------------------------------------------------------------------------
class CONDUIT_API ExecutionPolicy
{
public:
//-----------------------------------------------------------------------------
/// Policy is an Enumeration used to describe the policy cases supported
///  by conduit:
//-----------------------------------------------------------------------------
    enum class PolicyID : conduit::index_t
    {
        EMPTY_ID,
        SERIAL_ID,
        CUDA_ID,
        HIP_ID,
        OPENMP_ID
    };

//-----------------------------------------------------------------------------
// -- begin conduit::execution::ExecutionPolicy Constructor Helpers --
//-----------------------------------------------------------------------------
    static ExecutionPolicy empty();
    static ExecutionPolicy host(); // prefer openmp to serial
    static ExecutionPolicy serial();
    static ExecutionPolicy device(); // prefer cuda to hip
    static ExecutionPolicy cuda();
    static ExecutionPolicy hip();
    static ExecutionPolicy openmp();

//-----------------------------------------------------------------------------
// -- end conduit::execution::ExecutionPolicy Constructor Helpers --
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// -- conduit::execution::ExecutionPolicy public methods --
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------
    /// standard constructor
    ExecutionPolicy();
    /// copy constructor
    ExecutionPolicy(const ExecutionPolicy& exec_policy);
    /// Assignment operator
    ExecutionPolicy& operator=(const ExecutionPolicy& exec_policy);
    /// Construct from given policy id
    ExecutionPolicy(PolicyID policy_id);
    /// Construct from policy name
    ExecutionPolicy(const std::string &policy_name);
    /// destructor
    ~ExecutionPolicy();

//-----------------------------------------------------------------------------
// Setters
//-----------------------------------------------------------------------------
    void set_policy(PolicyID policy_id)
        { m_policy_id = policy_id; }

    // TODO consider if we want to allow fallbacks. This could be the way to 
    // implement. (i.e. if you ask for openmp but there is no openmp it falls
    // back to serial. If this isn't set then you get an error in that case.)
    // void set_its_ok_to_lie(bool lies);

//-----------------------------------------------------------------------------
// Getters and info methods.
//-----------------------------------------------------------------------------
    PolicyID    policy_id()         const { return m_policy_id; }
    std::string policy_name()       const { return policy_id_to_name(m_policy_id); }

    // these methods answer questions about the chosen policy
    bool        is_empty()          const;
    bool        is_serial()         const;
    bool        is_cuda()           const;
    bool        is_hip()            const;
    bool        is_openmp()         const;

    // these methods answer questions about where the policy can execute
    bool        is_host_policy()    const;
    bool        is_device_policy()  const;

    // these methods answer questions about which policies are able to be instantiated
    static bool is_serial_enabled();
    static bool is_cuda_enabled();
    static bool is_hip_enabled();
    static bool is_openmp_enabled();

    // these methods answer questions about which places we can instantiate policies for
    static bool is_host_enabled();
    static bool is_device_enabled();

//-----------------------------------------------------------------------------
// Helpers to convert PolicyID Enum Values to human readable strings and 
// vice versa.
//-----------------------------------------------------------------------------
    static PolicyID    name_to_policy_id(const std::string &name);
    static std::string policy_id_to_name(const PolicyID policy_id);

private:
//-----------------------------------------------------------------------------
//
// -- conduit::execution::ExecutionPolicy private data members --
//
//-----------------------------------------------------------------------------
    PolicyID m_policy_id;
};
//-----------------------------------------------------------------------------
// -- end conduit::execution::ExecutionPolicy --
//-----------------------------------------------------------------------------

// registers the fancy conduit memory handlers for
// magic memset and memcpy
void init_device_memory_handlers();

// helper for the CONDUIT_DEVICE_ERROR_CHECK macro
void device_error_check(ExecutionPolicy policy, const char *file, const int line);



struct EmptyPolicy
{};

#if defined(CONDUIT_USE_RAJA)
//---------------------------------------------------------------------------//
// RAJA_ON policies for when raja is on
//---------------------------------------------------------------------------//
#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_CUDA)
#define CUDA_BLOCK_SIZE 128
#endif

#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_HIP)
#define HIP_BLOCK_SIZE 256
#endif

struct SerialExec
{
    using for_policy = RAJA::seq_exec;
#if defined(CONDUIT_RAJA_CUDA_ACTIVE)
    // the cuda/hip policy for reductions can be used
    // by other backends, and this should suppress
    // erroneous host device warnings
    using reduce_policy = RAJA::cuda_reduce;
#elif defined(CONDUIT_RAJA_HIP_ACTIVE)
    using reduce_policy = RAJA::hip_reduce;
#else
    using reduce_policy = RAJA::seq_reduce;
#endif
    using atomic_policy = RAJA::seq_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};

//---------------------------------------------------------------------------
#if defined(CONDUIT_RAJA_CUDA_ACTIVE)
struct CudaExec
{
    using for_policy    = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    using reduce_policy = RAJA::cuda_reduce;
    using atomic_policy = RAJA::cuda_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#if defined(CONDUIT_RAJA_HIP_ACTIVE)
//---------------------------------------------------------------------------
struct HipExec
{
    using for_policy    = RAJA::hip_exec<HIP_BLOCK_SIZE>;
    using reduce_policy = RAJA::hip_reduce;
    using atomic_policy = RAJA::hip_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#if defined(CONDUIT_RAJA_OPENMP_ACTIVE)
//---------------------------------------------------------------------------
struct OpenMPExec
{
    using for_policy = RAJA::omp_parallel_for_exec;
#if defined(CONDUIT_RAJA_CUDA_ACTIVE)
    // the cuda policy for reductions can be used
    // by other backends, and this should suppress
    // erroneous host device warnings
    using reduce_policy = RAJA::cuda_reduce;
#elif defined(CONDUIT_RAJA_HIP_ACTIVE)
    using reduce_policy = RAJA::hip_reduce;
#else
    using reduce_policy = RAJA::omp_reduce;
#endif
    using atomic_policy = RAJA::omp_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

//---------------------------------------------------------------------------//
template <typename ExecPolicy, typename Kernel>
inline void
forall(const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    RAJA::forall<ExecPolicy>(RAJA::RangeSegment(begin, end),
                             std::forward<Kernel>(kernel));
}

//---------------------------------------------------------------------------//
template <typename ExecutionPolicy, typename Iterator>
inline void
sort(Iterator begin,
     Iterator end) noexcept
{
    std::sort(begin, end);
}

//---------------------------------------------------------------------------//
template <typename ExecutionPolicy, typename Iterator, typename Predicate>
inline void
sort(Iterator begin,
     Iterator end,
     Predicate &&predicate) noexcept
{
    std::sort(begin, end, std::forward<Predicate>(predicate));
}


#else
//---------------------------------------------------------------------------//
// RAJA_OFF policies for when raja is OFF
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------
struct SerialExec
{
    using for_policy = EmptyPolicy;
    using reduce_policy = EmptyPolicy;
    using atomic_policy = EmptyPolicy;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};

// TODO do we want this to be an option when RAJA is off?
// TODO decide about if we want to provide this as an option when RAJA is on as well
// if raja is on we don't want to use openmp not through raja
// and we do want it to be an option when raja is off
#if defined(CONDUIT_USE_OPENMP)
//---------------------------------------------------------------------------
struct OpenMPExec
{
    using for_policy = EmptyPolicy;
    using reduce_policy = EmptyPolicy;
    using atomic_policy = EmptyPolicy;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

//---------------------------------------------------------------------------//
// mock up of a raja like forall implementation 
//---------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------//
template <typename Kernel>
inline void
forall_exec(OpenMPExec,
            const int& begin,
            const int& end,
            Kernel&& kernel) noexcept
{
    // #pragma message("omp::for_policy -> OMP")
    #pragma omp parallel for
    for (index_t i = begin; i < end; i ++)
    {
        kernel(i);
    }
}
#endif

//---------------------------------------------------------------------------//
// invoke forall with concrete template tag
//---------------------------------------------------------------------------//
template <typename ExecutionPolicy, typename Kernel>
inline void
forall(const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    forall_exec(ExecutionPolicy{}, begin, end, std::forward<Kernel>(kernel));
}

//---------------------------------------------------------------------------//
// mock up of a raja like sort implementation 
//---------------------------------------------------------------------------//
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

//---------------------------------------------------------------------------//
// mock up of a raja like sort implementation 
//---------------------------------------------------------------------------//
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
//---------------------------------------------------------------------------//
template <typename Iterator>
inline void
sort_exec(OpenMPExec,
          Iterator begin,
          Iterator end) noexcept
{
   // #pragma message("omp::sort_policy -> serial")
    // TODO: implement an OpenMP sort like in RAJA.
    std::sort(begin, end);
    // This is only allowed in C++14 or later.
    //this->operator()(begin, end, [](auto &lhs, auto &rhs) { return lhs < rhs; });
}

//---------------------------------------------------------------------------//
template <typename Iterator, typename Predicate>
inline void
sort_exec(OpenMPExec,
          Iterator begin,
          Iterator end,
          Predicate &&predicate) noexcept
{
   // #pragma message("omp::sort_policy -> serial")
    // TODO: implement an OpenMP sort like in RAJA.
    std::sort(begin, end);
    // This is only allowed in C++14 or later.
    //this->operator()(begin, end, [](auto &lhs, auto &rhs) { return lhs < rhs; });
}
#endif

//---------------------------------------------------------------------------//
// invoke sort with concrete template tag
//---------------------------------------------------------------------------//
template <typename ExecutionPolicy, typename Iterator>
inline void
sort(Iterator begin, 
     Iterator end) noexcept
{
    sort_exec(ExecutionPolicy{}, begin, end);
}

//---------------------------------------------------------------------------//
// invoke sort with concrete template tag
//---------------------------------------------------------------------------//
template <typename ExecutionPolicy, typename Iterator, typename Predicate>
inline void
sort(Iterator begin,
     Iterator end,
     Predicate &&predicate) noexcept
{
    sort_exec(ExecutionPolicy{}, begin, end, std::forward<Predicate>(predicate));
}

#endif
//---------------------------------------------------------------------------//
// end RAJA_OFF
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// invoke functor with concrete template tag
//---------------------------------------------------------------------------//
template <typename ExecPolicyTag, typename Function>
inline void invoke(ExecPolicyTag &exec, Function&& func) noexcept
{
    func(exec);
}

//---------------------------------------------------------------------------//
// runtime to concrete template tag dispatch of a functor
//---------------------------------------------------------------------------//
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
#if defined(CONDUIT_RAJA_CUDA_ACTIVE)
        CudaExec ce;
        invoke(ce, func);
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_CUDA)
        CONDUIT_ERROR("Conduit was built with CUDA, but this translation unit "
                      "was not compiled with CUDA support.");
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_RAJA_HIP_ACTIVE)
        HipExec he;
        invoke(he, func);
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_HIP)
        CONDUIT_ERROR("Conduit was built with HIP, but this translation unit "
                      "was not compiled with HIP support.");
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_RAJA_OPENMP_ACTIVE)
        OpenMPExec ompe;
        invoke(ompe, func);
#elif !defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        OpenMPExec ompe;
        invoke(ompe, func);
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        CONDUIT_ERROR("Conduit was built with OpenMP, but RAJA OpenMP support "
                      "is unavailable in this build.");
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else // policy.is_empty()
    {
        CONDUIT_ERROR("Cannot invoke with an empty policy.");
    }
}

//---------------------------------------------------------------------------//
// runtime to concrete template tag dispatch of a forall
//---------------------------------------------------------------------------//
template <typename Kernel>
inline void
forall(ExecutionPolicy &policy,
       const int& begin,
       const int& end,
       Kernel&& kernel) noexcept
{
    if (policy.is_serial())
    {
#if defined(CONDUIT_USE_RAJA)
        forall<typename SerialExec::for_policy>(begin, end,
                                                std::forward<Kernel>(kernel));
#else
        forall<SerialExec>(begin, end, std::forward<Kernel>(kernel));
#endif
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_RAJA_CUDA_ACTIVE)
        forall<typename CudaExec::for_policy>(begin, end,
                                              std::forward<Kernel>(kernel));
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_CUDA)
        CONDUIT_ERROR("Conduit was built with CUDA, but this translation unit "
                      "was not compiled with CUDA support.");
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }
    else if (policy.is_hip())
    {
#if defined(CONDUIT_RAJA_HIP_ACTIVE)
        forall<typename HipExec::for_policy>(begin, end,
                                             std::forward<Kernel>(kernel));
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_HIP)
        CONDUIT_ERROR("Conduit was built with HIP, but this translation unit "
                      "was not compiled with HIP support.");
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_openmp())
    {
#if defined(CONDUIT_RAJA_OPENMP_ACTIVE)
        forall<typename OpenMPExec::for_policy>(begin, end,
                                                std::forward<Kernel>(kernel));
#elif !defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        forall<OpenMPExec>(begin, end, std::forward<Kernel>(kernel));
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        CONDUIT_ERROR("Conduit was built with OpenMP, but RAJA OpenMP support "
                      "is unavailable in this build.");
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else // policy.is_empty()
    {
        CONDUIT_ERROR("Cannot call forall with an empty policy.");
    }
}

//---------------------------------------------------------------------------//
// runtime to concrete template tag dispatch of a sort
//---------------------------------------------------------------------------//
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
#if defined(CONDUIT_RAJA_OPENMP_ACTIVE) || (!defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP))
        sort<OpenMPExec>(begin, end);
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        CONDUIT_ERROR("Conduit was built with OpenMP, but RAJA OpenMP support "
                      "is unavailable in this build.");
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else // policy.is_empty()
    {
        CONDUIT_ERROR("Cannot call sort with an empty policy.");
    }
}

//---------------------------------------------------------------------------//
// runtime to concrete template tag dispatch of a sort
//---------------------------------------------------------------------------//
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
#if defined(CONDUIT_RAJA_OPENMP_ACTIVE) || (!defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP))
        sort<OpenMPExec>(begin, end, std::forward<Predicate>(predicate));
#elif defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_OPENMP)
        CONDUIT_ERROR("Conduit was built with OpenMP, but RAJA OpenMP support "
                      "is unavailable in this build.");
#else
        CONDUIT_ERROR("Conduit was not built with OpenMP.");
#endif
    }
    else // policy.is_empty()
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
