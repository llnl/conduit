// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution_policy.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_EXECUTION_POLICY_HPP
#define CONDUIT_EXECUTION_POLICY_HPP

#include "conduit_config.h"
#include "conduit_utils.hpp"
#include "conduit_memory_manager.hpp"

#if defined(CONDUIT_USE_RAJA)
#include <RAJA/RAJA.hpp>
#endif

// Build capability answers "was this Conduit build configured with a RAJA
// backend?", while TU capability answers "is this translation unit actually
// being compiled with that device compiler right now?"
#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_CUDA)
#define CONDUIT_EXEC_BUILD_HAS_CUDA
#endif

#if defined(CONDUIT_USE_RAJA) && defined(CONDUIT_USE_HIP)
#define CONDUIT_EXEC_BUILD_HAS_HIP
#endif

#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA) || defined(CONDUIT_EXEC_BUILD_HAS_HIP)
#define CONDUIT_EXEC_BUILD_HAS_DEVICE
#endif

#if defined(CONDUIT_EXEC_BUILD_HAS_CUDA) && defined(__CUDACC__)
#define CONDUIT_EXEC_TU_HAS_CUDA
#endif

#if defined(CONDUIT_EXEC_BUILD_HAS_HIP) && defined(__HIPCC__)
#define CONDUIT_EXEC_TU_HAS_HIP
#endif

#if defined(CONDUIT_EXEC_TU_HAS_CUDA) || defined(CONDUIT_EXEC_TU_HAS_HIP)
#define CONDUIT_EXEC_TU_HAS_DEVICE
#endif

#if defined(CONDUIT_USE_OPENMP)
#include <omp.h>
#endif

#include <string>

#define CONDUIT_DEVICE_ERROR_CHECK( policy ) conduit::execution::device_error_check(policy, __FILE__, __LINE__);

#if defined(CONDUIT_EXEC_TU_HAS_DEVICE)
#define EXEC_LAMBDA __device__ __host__
#else
#define EXEC_LAMBDA
#endif

#if defined(CONDUIT_USE_CUDA)
#define CUDA_BLOCK_SIZE 128
#endif

#if defined(CONDUIT_USE_HIP)
#define HIP_BLOCK_SIZE 256
#endif

namespace conduit
{
namespace execution
{

class CONDUIT_API ExecutionPolicy
{
public:
    enum class PolicyID : conduit::index_t
    {
        EMPTY_ID,
        SERIAL_ID,
        CUDA_ID,
        HIP_ID,
        OPENMP_ID
    };

    static ExecutionPolicy empty();
    static ExecutionPolicy host();
    static ExecutionPolicy serial();
    static ExecutionPolicy device();
    static ExecutionPolicy cuda();
    static ExecutionPolicy hip();
    static ExecutionPolicy openmp();

    EXEC_LAMBDA ExecutionPolicy()
    : m_policy_id(PolicyID::EMPTY_ID)
    {}

    EXEC_LAMBDA ExecutionPolicy(const ExecutionPolicy& exec_policy) = default;
    EXEC_LAMBDA ExecutionPolicy& operator=(const ExecutionPolicy& exec_policy) = default;

    EXEC_LAMBDA ExecutionPolicy(PolicyID policy_id)
    : m_policy_id(policy_id)
    {}

    ExecutionPolicy(const std::string &policy_name);
    EXEC_LAMBDA ~ExecutionPolicy() = default;

    void set_policy(PolicyID policy_id)
        { m_policy_id = policy_id; }

    EXEC_LAMBDA PolicyID policy_id() const { return m_policy_id; }
    std::string policy_name()       const { return policy_id_to_name(m_policy_id); }

    bool        is_empty()          const;
    bool        is_serial()         const;
    bool        is_cuda()           const;
    bool        is_hip()            const;
    bool        is_openmp()         const;

    bool        is_host_policy()    const;
    bool        is_device_policy()  const;

    static bool is_serial_enabled();
    static bool is_cuda_enabled();
    static bool is_hip_enabled();
    static bool is_openmp_enabled();

    static bool is_host_enabled();
    static bool is_device_enabled();

    static PolicyID    name_to_policy_id(const std::string &name);
    static std::string policy_id_to_name(const PolicyID policy_id);

private:
    PolicyID m_policy_id;
};

void init_device_memory_handlers();
void device_error_check(ExecutionPolicy policy, const char *file, const int line);

struct EmptyPolicy
{};

#if defined(CONDUIT_USE_RAJA)
struct SerialExec
{
    using for_policy = RAJA::seq_exec;
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    using reduce_policy = RAJA::cuda_reduce;
#elif defined(CONDUIT_EXEC_TU_HAS_HIP)
    using reduce_policy = RAJA::hip_reduce;
#else
    using reduce_policy = RAJA::seq_reduce;
#endif
    using atomic_policy = RAJA::seq_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};

#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
struct CudaExec
{
    using for_policy    = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    using reduce_policy = RAJA::cuda_reduce;
    using atomic_policy = RAJA::cuda_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#if defined(CONDUIT_EXEC_TU_HAS_HIP)
struct HipExec
{
    using for_policy    = RAJA::hip_exec<HIP_BLOCK_SIZE>;
    using reduce_policy = RAJA::hip_reduce;
    using atomic_policy = RAJA::hip_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#if defined(CONDUIT_USE_OPENMP)
struct OpenMPExec
{
    using for_policy = RAJA::omp_parallel_for_exec;
#if defined(CONDUIT_EXEC_TU_HAS_CUDA)
    using reduce_policy = RAJA::cuda_reduce;
#elif defined(CONDUIT_EXEC_TU_HAS_HIP)
    using reduce_policy = RAJA::hip_reduce;
#else
    using reduce_policy = RAJA::omp_reduce;
#endif
    using atomic_policy = RAJA::omp_atomic;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#else

struct SerialExec
{
    using for_policy = EmptyPolicy;
    using reduce_policy = EmptyPolicy;
    using atomic_policy = EmptyPolicy;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};

#if defined(CONDUIT_USE_OPENMP)
struct OpenMPExec
{
    using for_policy = EmptyPolicy;
    using reduce_policy = EmptyPolicy;
    using atomic_policy = EmptyPolicy;
    using sort_policy = EmptyPolicy;
    static std::string memory_space;
};
#endif

#endif

}
}

#endif
