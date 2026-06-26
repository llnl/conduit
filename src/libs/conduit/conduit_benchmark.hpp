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

#include "conduit_annotations.hpp"
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
    template <typename PolicyFn>
    void
    add_if_enabled(std::vector<EP>& policies,
                   bool enabled,
                   PolicyFn&& get_policy)
    {
        // Don't try to add policies that are compile-time disabled
        if (!enabled)
        {
            return;
        }
    
        EP policy = get_policy();
        const auto id = policy.policy_id();
    
        // Check if this policy is already in the vector
        auto it = std::find_if(
            policies.begin(),
            policies.end(),
            [id](const EP& p)
            {
                return p.policy_id() == id;
            });

        // If the policy is not already in the vector, add it
        if (it == policies.end())
        {
            policies.push_back(policy);
        }
    }
    
    //-----------------------------------------------------------------------------
    // TODO: This might be nice as a public helper in the execution:: API
    std::vector<EP>
    get_enabled_policies()
    {
        std::vector<EP> policies;
        policies.reserve(7);

        // Since asking for compile-time disabled policies will throw an error
        // (and subsequently cause our benchmark "tests" to fail), we have to
        // do extra work to avoid asking for disabled policies
        add_if_enabled(policies, EP::is_serial_enabled(),   [] { return EP::serial(); });
        add_if_enabled(policies, EP::is_parallel_enabled(), [] { return EP::parallel(); });
        add_if_enabled(policies, EP::is_host_enabled(),     [] { return EP::host(); });
        add_if_enabled(policies, EP::is_device_enabled(),   [] { return EP::device(); });
        add_if_enabled(policies, EP::is_openmp_enabled(),   [] { return EP::openmp(); });
        add_if_enabled(policies, EP::is_cuda_enabled(),     [] { return EP::cuda(); });
        add_if_enabled(policies, EP::is_hip_enabled(),      [] { return EP::hip(); });
    
        return policies;
    }

    // TODO: I think the current semantics are okay, but I want to investigate if
    // conduit::benchmark::(func) is possible
    template <typename BenchmarkFn>
    void
    exec(BenchmarkFn&& fn, const int warmup, const int iterations)
    {
        // Setup
        execution::init_device_memory_handlers();
        auto policies = conduit::benchmark::get_enabled_policies();
    
        for (const auto& policy : policies)
        {
            // Warm-up
            for (int i = 0; i < warmup; i++)
            {
                fn(policy);
            }
    
            std::cout << "\n" << policy.policy_name() << "," << warmup << "," << iterations << std::endl;
    
            // TODO: Do we care to have bespoke timing in the case that conduit was built
            // without caliper?
            Node cali_opts;
            cali_opts["config"] = "runtime-report";
            annotations::initialize(cali_opts);
    
            // Benchmark
            for (int i = 0; i < iterations; i++)
            {
                fn(policy);
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
