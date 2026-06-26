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

    //-----------------------------------------------------------------------------
    // TODO: This might be nice as a public helper in the execution:: API
    template <typename PolicyFn>
    void
    add_if_enabled(std::vector<execution::ExecutionPolicy>& policies, bool enabled, PolicyFn&& make_policy)
    {
        // Don't execute policies that are compile-time disabled
        if (!enabled)
        {
            return;
        }
    
        execution::ExecutionPolicy policy = make_policy();
        const auto id = policy.policy_id();
    
        // Don't insert repeat policies into the vector (so that we only
        // do each one exactly once)
        auto it = std::find_if(
            policies.begin(),
            policies.end(),
            [id](const execution::ExecutionPolicy& p)
            {
                return p.policy_id() == id;
            });
    
        if (it == policies.end())
        {
            policies.push_back(policy);
        }
    }
    
    //-----------------------------------------------------------------------------
    std::vector<execution::ExecutionPolicy>
    get_enabled_policies()
    {
        std::vector<execution::ExecutionPolicy> policies;
        policies.reserve(7);
    
        add_if_enabled(policies, execution::ExecutionPolicy::is_serial_enabled(),   [] { return execution::ExecutionPolicy::serial(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_parallel_enabled(), [] { return execution::ExecutionPolicy::parallel(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_host_enabled(),     [] { return execution::ExecutionPolicy::host(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_device_enabled(),   [] { return execution::ExecutionPolicy::device(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_openmp_enabled(),   [] { return execution::ExecutionPolicy::openmp(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_cuda_enabled(),     [] { return execution::ExecutionPolicy::cuda(); });
        add_if_enabled(policies, execution::ExecutionPolicy::is_hip_enabled(),      [] { return execution::ExecutionPolicy::hip(); });
    
        return policies;
    }

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
