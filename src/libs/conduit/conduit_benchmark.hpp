// Copyright (c) Lawrence Livermore National Security, LLC and other Conduit
// Project developers. See top-level LICENSE AND COPYRIGHT files for dates and
// other details. No copyright assignment is required to contribute to Conduit.

//-----------------------------------------------------------------------------
///
/// file: conduit_benchmark.hpp
///
//-----------------------------------------------------------------------------

#ifndef CONDUIT_BENCHMARK_HPP
#define CONDUIT_BENCHMARK_HPP

#include <vector>

#include "conduit_annotations.hpp"
#include "conduit_data_type.hpp"
#include "conduit_execution_policy.hpp"

//-----------------------------------------------------------------------------
// -- begin conduit --
//-----------------------------------------------------------------------------
namespace conduit
{

//-----------------------------------------------------------------------------
// -- begin benchmark --
//-----------------------------------------------------------------------------
namespace benchmark
{

using EP = execution::ExecutionPolicy;

//-----------------------------------------------------------------------------
// TODO: This might be nice as a public helper in the execution:: API
inline
std::vector<EP>
get_enabled_policies()
{
    std::vector<EP> policies;
    policies.reserve(7);

    // Loop over the policy getters to make a list of enabled policies
    using PolicyFn = EP (*)();
    for (PolicyFn try_add : {EP::serial,
                             EP::host,
                             EP::openmp,
                             EP::parallel,
                             EP::device,
                             EP::cuda,
                             EP::hip})
    {
        try
        {
            EP policy = try_add();
            const auto id = policy.policy_id();
            auto it = std::find_if(
                policies.begin(),
                policies.end(),
                [id](const EP& p)
                {
                    return p.policy_id() == id;
                });
            if (it == policies.end())
            {
                // Only add the policy if it is not already present
                policies.push_back(policy);
            }
        }
        catch (...)
        {
            // Conduit was not compiled with support for this policy
        }
    }

    return policies;
}

//-----------------------------------------------------------------------------
template <typename BenchmarkFn>
void
exec(BenchmarkFn&& fn,
     const index_t warmup,
     const index_t iterations)
{
    // Setup
    execution::init_device_memory_handlers();
    
    // TODO: Loop over host vs device
    auto policies = get_enabled_policies();
    for (const auto& policy : policies)
    {
        // Execute fn `warmup` times
        for (index_t i = 0; i < warmup; i++)
        {
            fn();
        }

        // Outputs a file called region_profile.cali with profiling data for,
        // consumption by hatchet or thicket
        Node cali_opts;
        cali_opts["config"] = "hatchet-region-profile";
        annotations::initialize(cali_opts);

        // Execute fn `iterations` times
        for (index_t i = 0; i < iterations; i++)
        {
            fn();
        }

        annotations::finalize();
    }
}

}
//-----------------------------------------------------------------------------
// -- end benchmark:: --
//-----------------------------------------------------------------------------

}
//-----------------------------------------------------------------------------
// -- end conduit:: --
//-----------------------------------------------------------------------------

#endif // CONDUIT_BENCHMARK_HPP
