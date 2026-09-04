// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_execution.cpp
///
//-----------------------------------------------------------------------------
#include "conduit_execution.hpp"

//-----------------------------------------------------------------------------
// conduit includes
//-----------------------------------------------------------------------------
#include "conduit_node.hpp"

#if defined(CONDUIT_USE_CUDA)
#include <cuda_runtime.h>
#endif

#if defined(CONDUIT_USE_HIP)
#include <hip/hip_runtime.h>
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
/// policy constructor helpers
//-----------------------------------------------------------------------------
ExecutionPolicy
ExecutionPolicy::empty()
{
    return ExecutionPolicy(PolicyID::EMPTY_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::host()
{
#if defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    return ExecutionPolicy(PolicyID::SERIAL_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::serial()
{
    return ExecutionPolicy(PolicyID::SERIAL_ID);
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::device()
{
#if defined(CONDUIT_USE_CUDA)
    return ExecutionPolicy(PolicyID::CUDA_ID);
#elif defined(CONDUIT_USE_HIP)
    return ExecutionPolicy(PolicyID::HIP_ID);
#else
    CONDUIT_ERROR("Conduit was built with neither CUDA nor HIP.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::cuda()
{
#if defined(CONDUIT_USE_CUDA)
    return ExecutionPolicy(PolicyID::CUDA_ID);
#else
    CONDUIT_ERROR("Conduit was built without CUDA.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::hip()
{
#if defined(CONDUIT_USE_HIP)
    return ExecutionPolicy(PolicyID::HIP_ID);
#else
    CONDUIT_ERROR("Conduit was built without HIP.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
ExecutionPolicy
ExecutionPolicy::openmp()
{
#if defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    CONDUIT_ERROR("Conduit was built without openMP.");
    return ExecutionPolicy(PolicyID::EMPTY_ID);
#endif
}

//---------------------------------------------------------------------------//
// parallel allows serial execution - it is more of a way of saying that code
// can safely run in parallel. Then we prefer parallel execution if possible.
ExecutionPolicy
ExecutionPolicy::parallel()
{
#if defined(CONDUIT_USE_CUDA)
    return ExecutionPolicy(PolicyID::CUDA_ID);
#elif defined(CONDUIT_USE_HIP)
    return ExecutionPolicy(PolicyID::HIP_ID);
#elif defined(CONDUIT_USE_OPENMP)
    return ExecutionPolicy(PolicyID::OPENMP_ID);
#else
    return ExecutionPolicy(PolicyID::SERIAL_ID);
#endif
}

//=============================================================================
//-----------------------------------------------------------------------------
//
//
// -- begin conduit::execution::ExecutionPolicy public methods --
//
//
//-----------------------------------------------------------------------------
//=============================================================================

//-----------------------------------------------------------------------------
// Construction and Destruction
//-----------------------------------------------------------------------------
//---------------------------------------------------------------------------//
ExecutionPolicy::ExecutionPolicy(const std::string &policy_name)
: m_policy_id(name_to_policy_id(policy_name))
{}

//-----------------------------------------------------------------------------
// Getters and info methods.
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_empty() const
{
    return m_policy_id == PolicyID::EMPTY_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_serial() const
{
    return m_policy_id == PolicyID::SERIAL_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_cuda() const
{
    return m_policy_id == PolicyID::CUDA_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_hip() const
{
    return m_policy_id == PolicyID::HIP_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_openmp() const
{
    return m_policy_id == PolicyID::OPENMP_ID;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_host_policy() const
{
    return is_serial() || is_openmp();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_device_policy() const
{
    return is_cuda() || is_hip();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_parallel_policy() const
{
    // TODO it is strange that you can instantiate a parallel policy that ends
    // up being serial, and then you can ask is_parallel_policy() and get false.
    // We should explore if we can make this more consistent.
    return is_cuda() || is_hip() || is_openmp();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_serial_enabled()
{
    return true;
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_cuda_enabled()
{
#if defined(CONDUIT_USE_CUDA) && defined(CONDUIT_USE_UMPIRE)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_hip_enabled()
{
#if defined(CONDUIT_USE_HIP) && defined(CONDUIT_USE_UMPIRE)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_openmp_enabled()
{
#if defined(CONDUIT_USE_OPENMP)
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_host_enabled()
{
    return is_serial_enabled() || is_openmp_enabled();
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_device_enabled()
{
#if defined(CONDUIT_USE_DEVICE)
    return is_cuda_enabled() || is_hip_enabled();
#else
    return false;
#endif
}

//---------------------------------------------------------------------------//
bool
ExecutionPolicy::is_parallel_enabled()
{
    return is_device_enabled() || is_openmp_enabled();
}

//-----------------------------------------------------------------------------
// PolicyID to string and string to PolicyID
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------//
ExecutionPolicy::PolicyID
ExecutionPolicy::name_to_policy_id(const std::string &policy_name)
{
    if      (policy_name == "empty")    return PolicyID::EMPTY_ID;
    else if (policy_name == "serial")   return PolicyID::SERIAL_ID;
    else if (policy_name == "cuda")     return PolicyID::CUDA_ID;
    else if (policy_name == "hip")      return PolicyID::HIP_ID;
    else if (policy_name == "openmp")   return PolicyID::OPENMP_ID;
    else if (policy_name == "host")     return host().policy_id();
    else if (policy_name == "device")   return device().policy_id();
    else if (policy_name == "parallel") return parallel().policy_id();
    return PolicyID::EMPTY_ID;
}

//---------------------------------------------------------------------------//
std::string 
ExecutionPolicy::policy_id_to_name(const PolicyID policy_id)
{
    if      (policy_id == PolicyID::EMPTY_ID)   return "empty";
    else if (policy_id == PolicyID::SERIAL_ID)  return "serial";
    else if (policy_id == PolicyID::CUDA_ID)    return "cuda";
    else if (policy_id == PolicyID::HIP_ID)     return "hip";
    else if (policy_id == PolicyID::OPENMP_ID)  return "openmp";
    return "empty";
}

//-----------------------------------------------------------------------------
/// Helper to validate execution option policy names.
//-----------------------------------------------------------------------------
static bool
is_valid_execution_option_policy_name(const std::string &policy_name)
{
    return policy_name == "host"     ||
           policy_name == "device"   ||
           policy_name == "serial"   ||
           policy_name == "cuda"     ||
           policy_name == "hip"      ||
           policy_name == "openmp"   ||
           policy_name == "parallel";
}

//-----------------------------------------------------------------------------
/// Resolve an execution option policy name to a concrete ExecutionPolicy.
//-----------------------------------------------------------------------------
static ExecutionPolicy
resolve_execution_option_policy(const std::string &policy_name)
{
    if      (policy_name == "host")     return ExecutionPolicy::host();
    else if (policy_name == "device")   return ExecutionPolicy::device();
    else if (policy_name == "serial")   return ExecutionPolicy::serial();
    else if (policy_name == "cuda")     return ExecutionPolicy::cuda();
    else if (policy_name == "hip")      return ExecutionPolicy::hip();
    else if (policy_name == "openmp")   return ExecutionPolicy::openmp();
    else if (policy_name == "parallel") return ExecutionPolicy::parallel();

    CONDUIT_ERROR("ExecutionOptions: invalid execution policy option '"
                  << policy_name << "'.");
    return ExecutionPolicy::empty();
}

//-----------------------------------------------------------------------------
// Private class used to hold options that control execution params.
//
// These values are read by about(), and are set by io::execution_set_options()
//
//
//-----------------------------------------------------------------------------

class ExecutionOptions
{
public:
    // chosen execution location
    static std::string execution_location;

    // chosen output location
    static std::string output_location;

    // "sync" or "assume"
    static SyncStrategy sync_strategy;

    // fallback location for when execution_location is "input" or 
    // output_location is "input" and there is no provided input Node that we
    // can query.
    static std::string fallback_location;
    
    // allocator ids that are available
    static index_t device_allocator;
    static index_t host_allocator;
    static index_t user_provided_allocator;

public:
    //------------------------------------------------------------------------
    // opts node:
    //   execution_location: "host"|"device"|"serial"|"cuda"|"hip"|"openmp"
    //                       |"parallel"|"input"
    //     # choose an explicit execution policy, or input (use `use_with` to
    //     # get a policy for the input data).
    //     # default is "input" (if no Node is provided then the 
    //     # fallback_location is used.)
    //   output_location: "host"|"device"|"input"|##
    //     # choose host alloc, device alloc, input alloc (get the allocator 
    //     # from the input node), or an index_t that is the allocator id to use
    //     # default is "input" (if no Node is provided then the 
    //     # fallback_location is used.)
    //   sync_strategy: "sync"|"assume"
    //     # choose to sync or assume (if we add a new option here we need to
    //     # update all the use sites).
    //     # default is "assume"
    //   fallback_location: "host"|"device"|"serial"|"cuda"|"hip"|"openmp"
    //                      |"parallel"
    //     # choose a fallback execution policy in the case that "input" is
    //     # chosen for either execution_location or output_location and there
    //     # is no input to operate on/reason about.
    //     # default is "host"
    static void set(const Node &opts)
    {
        if (opts.has_child("execution_location"))
        {
            if (opts["execution_location"].dtype().is_string())
            {
                const std::string policy = opts["execution_location"].as_string();
                if (policy == "input" ||
                    is_valid_execution_option_policy_name(policy))
                {
                    execution_location = policy;
                }
                else
                {
                    CONDUIT_ERROR("ExecutionOptions: invalid execution_location option.");
                }
            }
            else
            {
                CONDUIT_ERROR("ExecutionOptions: invalid execution_location option.");
            }
        }

        if (opts.has_child("output_location"))
        {
            if (opts["output_location"].dtype().is_string())
            {
                const std::string out_alloc = opts["output_location"].as_string();
                if (out_alloc == "host" ||
                    out_alloc == "device" ||
                    out_alloc == "input")
                {
                    output_location = out_alloc;
                }
                else
                {
                    CONDUIT_ERROR("ExecutionOptions: invalid output_location option.");
                }
            }
            else if (opts["output_location"].dtype().is_integer())
            {
                output_location = "user_provided";
                user_provided_allocator = opts["output_location"].to_index_t();
            }
            else
            {
                CONDUIT_ERROR("ExecutionOptions: invalid output_location option.");
            }
        }

        if (opts.has_child("sync_strategy"))
        {
            if (opts["sync_strategy"].dtype().is_string())
            {
                sync_strategy = sync_strategy_from_string(
                    opts["sync_strategy"].as_string());
            }
            else
            {
                CONDUIT_ERROR("ExecutionOptions: invalid sync_strategy option.");
            }
        }

        if (opts.has_child("fallback_location"))
        {
            if (opts["fallback_location"].dtype().is_string())
            {
                const std::string fallback = opts["fallback_location"].as_string();
                if (is_valid_execution_option_policy_name(fallback))
                {
                    fallback_location = fallback;
                }
                else
                {
                    CONDUIT_ERROR("ExecutionOptions: invalid fallback_location option.");
                }
            }
            else
            {
                CONDUIT_ERROR("ExecutionOptions: invalid fallback_location option.");
            }
        }
    }

    //------------------------------------------------------------------------
    static void about(Node &opts)
    {
        opts.reset();

        opts["execution_location"].set(execution_location);
        opts["output_location"].set(output_location);
        opts["sync_strategy"].set(sync_strategy_to_string(sync_strategy));
        opts["fallback_location"].set(fallback_location);
        opts["device_allocator"].set(device_allocator);
        opts["host_allocator"].set(host_allocator);
        opts["user_provided_allocator"].set(user_provided_allocator);
    }

    //------------------------------------------------------------------------
    static void reset()
    {
        execution_location = "input";
        output_location = "input";
        sync_strategy = SyncStrategy::Assume;
        fallback_location = "host";
        // no need to reset device_allocator
        // no need to reset host_allocator
        user_provided_allocator = -1;
    }

    //------------------------------------------------------------------------
    static ExecutionPolicy get_execution_policy_helper(const Node *src_node)
    {
        if ("input" == execution_location)
        {
            if (nullptr != src_node)
            {
                if (execution::DeviceMemory::is_device_ptr(src_node->data_ptr()))
                {
                    return execution::ExecutionPolicy::device();
                }
                else
                {
                    return execution::ExecutionPolicy::host();
                }
            }
            else
            {
                return resolve_execution_option_policy(fallback_location);
            }
        }

        return resolve_execution_option_policy(execution_location);
    }

    //------------------------------------------------------------------------
    static ExecutionPolicy get_execution_policy(const Node &src_node)
    {
        return get_execution_policy_helper(&src_node);
    }

    //------------------------------------------------------------------------
    static ExecutionPolicy get_execution_policy()
    {
        return get_execution_policy_helper(nullptr);
    }

    //------------------------------------------------------------------------
    static index_t get_output_allocator_id_helper(const Node *src_node)
    {
        if ("host" == output_location)
        {
            return host_allocator;
        }
        if ("device" == output_location)
        {
            return device_allocator;
        }
        if ("user_provided" == output_location)
        {
            return user_provided_allocator;
        }
        if ("input" == output_location)
        {
            // In unified memory, policies don't have to be concerned about
            // which output location they select. Benchmarks show that it is
            // fastest to use a device allocator if we are using executing on
            // device, and to use a host allocator if we are executing on host.
            if (DeviceMemory::unified())
            {
                return get_execution_policy_helper(src_node).is_device_policy() ?
                    device_allocator : host_allocator;
            }
            if (nullptr != src_node)
            {
                return src_node->allocator();
            }
            else
            {
                const ExecutionPolicy fallback_policy =
                    resolve_execution_option_policy(fallback_location);
                return fallback_policy.is_device_policy() ?
                    device_allocator : host_allocator;
            }
        }
        CONDUIT_ERROR("ExecutionOptions::get_output_allocator_id() cannot resolve "
                      "output_location " << output_location << ".");
        return -1;
    }

    //------------------------------------------------------------------------
    static index_t get_output_allocator_id(const Node &src_node)
    {
        return get_output_allocator_id_helper(&src_node);
    }

    //------------------------------------------------------------------------
    static index_t get_output_allocator_id()
    {
        return get_output_allocator_id_helper(nullptr);
    }

    //------------------------------------------------------------------------
    static SyncStrategy get_sync_strategy()
    {
        return sync_strategy;
    }

    //------------------------------------------------------------------------
    static index_t get_device_allocator_id()
    {
        return device_allocator;
    }

    //------------------------------------------------------------------------
    static index_t get_host_allocator_id()
    {
        return host_allocator;
    }
};

//
// default execution settings
//

// execution location
std::string ExecutionOptions::execution_location = "input";
// output allocation location
std::string ExecutionOptions::output_location = "input";
// "sync" or "assume"
SyncStrategy ExecutionOptions::sync_strategy = SyncStrategy::Assume;
// fallback if both exec and output location are "input" and there is no input
std::string ExecutionOptions::fallback_location = "host";

#if defined(CONDUIT_USE_DEVICE)
// allocator ids that are available - use lambdas to avoid overload ambiguity
index_t ExecutionOptions::device_allocator =
    conduit::utils::register_allocator(
        [](size_t num_items, size_t item_size) -> void *
        {
            return DeviceMemory::allocate(num_items, item_size);
        },
        DeviceMemory::deallocate);
index_t ExecutionOptions::host_allocator =
    conduit::utils::register_allocator(
        [](size_t num_items, size_t item_size) -> void *
        {
            return HostMemory::allocate(num_items, item_size);
        },
        HostMemory::deallocate);
#else
index_t ExecutionOptions::device_allocator = -1;
index_t ExecutionOptions::host_allocator = 0;
#endif
index_t ExecutionOptions::user_provided_allocator = -1;

//-----------------------------------------------------------------------------
void
execution_set_options(const Node &opts)
{
    ExecutionOptions::set(opts);
}

//-----------------------------------------------------------------------------
void
execution_options(Node &opts)
{
    ExecutionOptions::about(opts);
}

//-----------------------------------------------------------------------------
void
reset_execution_options()
{
    ExecutionOptions::reset();
}


//-----------------------------------------------------------------------------
ExecutionPolicy
get_execution_policy(const Node &src_node)
{
    return ExecutionOptions::get_execution_policy(src_node);
}

//-----------------------------------------------------------------------------
ExecutionPolicy
get_execution_policy()
{
    return ExecutionOptions::get_execution_policy();
}

//-----------------------------------------------------------------------------
index_t
get_output_allocator_id(const Node &src_node)
{
    return ExecutionOptions::get_output_allocator_id(src_node);
}

//-----------------------------------------------------------------------------
index_t
get_output_allocator_id()
{
    return ExecutionOptions::get_output_allocator_id();
}

//-----------------------------------------------------------------------------
SyncStrategy
get_sync_strategy()
{
    return ExecutionOptions::get_sync_strategy();
}

//-----------------------------------------------------------------------------
index_t
get_device_allocator_id()
{
    return ExecutionOptions::get_device_allocator_id();
}

//-----------------------------------------------------------------------------
index_t
get_host_allocator_id()
{
    return ExecutionOptions::get_host_allocator_id();
}

//-----------------------------------------------------------------------------
std::string
sync_strategy_to_string(const SyncStrategy strategy)
{
    if (SyncStrategy::Sync == strategy)
    {
        return "sync";
    }
    else if (SyncStrategy::Assume == strategy)
    {
        return "assume";
    }
    else
    {
        CONDUIT_ERROR("Unknown data movement strategy: "
                      << conduit::execution::sync_strategy_to_string(strategy)
                      << " (" << static_cast<int>(strategy) << ").");
        return "";
    }
}

//-----------------------------------------------------------------------------
SyncStrategy
sync_strategy_from_string(const std::string &strategy)
{
    if ("sync" == strategy)
    {
        return SyncStrategy::Sync;
    }
    else if ("assume" == strategy)
    {
        return SyncStrategy::Assume;
    }
    else
    {
        CONDUIT_ERROR("ExecutionOptions: invalid sync_strategy option.");
        return SyncStrategy::Assume;
    }
}

//---------------------------------------------------------------------------//
void
init_device_memory_handlers()
{
#if defined(CONDUIT_USE_CUDA) || defined(CONDUIT_USE_HIP)
    // we only need to override the mem handlers in the
    // presence of cuda or hip
    conduit::utils::set_memcpy_handler(MagicMemory::copy);
    conduit::utils::set_memset_handler(MagicMemory::set);
#endif
}

//---------------------------------------------------------------------------//
void
device_error_check(ExecutionPolicy policy, const char *file, const int line)
{
    if (policy.is_hip())
    {
#if defined(CONDUIT_USE_HIP)
        hipError_t err = hipGetLastError();
        if ( hipSuccess != err )
        {
            std::cerr<<"HIP error reported at: "<<file<<":"<<line;
            std::cerr<<" : "<<hipGetErrorName(err)<<"\n";
            //exit( -1 );
        }
#else
        CONDUIT_ERROR("Conduit was not built with HIP.");
#endif
    }
    else if (policy.is_cuda())
    {
#if defined(CONDUIT_USE_CUDA)
        cudaError err = cudaGetLastError();
        if ( cudaSuccess != err )
        {
            std::cerr<<"CUDA error reported at: "<<file<<":"<<line;
            std::cerr<<" : "<<cudaGetErrorString(err)<<"\n";
            //exit( -1 );
        }
#else
        CONDUIT_ERROR("Conduit was not built with CUDA.");
#endif
    }

    (void)file; // suppress unused variable warnings
    (void)line; // suppress unused variable warnings
}

}
//-----------------------------------------------------------------------------
// -- end conduit::execution --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------
