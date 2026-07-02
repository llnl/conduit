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

#include <ctime>
#include <string>
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
inline
std::string
get_timestamp()
{
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&t));
    return std::string(buf);
}

//-----------------------------------------------------------------------------
struct ExecConfig
{
    std::string src_location;
    std::string exec_location;
    std::string output_location;
    index_t dim_size = 1;
};

//-----------------------------------------------------------------------------
inline
std::vector<std::string>
get_enabled_locations()
{
    std::vector<std::string> locations;
    locations.push_back("host");
    if (EP::is_device_enabled())
    {
        locations.push_back("device");
    }
    return locations;
}

//-----------------------------------------------------------------------------
inline
std::vector<ExecConfig>
get_exec_configs()
{
    std::vector<ExecConfig> configs;
    const auto locations = get_enabled_locations();

    // All of the source = host configs come first, as an optimization to
    // only have to move test data to device memory once
    for (const auto &src_loc : locations)
    {
        for (const auto &exec_loc : locations)
        {
            for (const auto &output_loc : locations)
            {
                configs.push_back({src_loc,
                                   exec_loc,
                                   output_loc});
            }
        }
    }

    return configs;
}

//-----------------------------------------------------------------------------
template <typename SetupFn, typename RunFn>
void
exec(const char *name,
     SetupFn &&setup,
     RunFn &&run,
     const index_t warmup,
     const index_t iterations,
     const std::vector<index_t> &dim_sizes)
{
    // Setup
    execution::init_device_memory_handlers();

    // Benchmark each data size
    for (const auto &dim_size : dim_sizes)
    {
        // Benchmark each possible configuration
        for (auto &config : get_exec_configs())
        {
            config.dim_size = dim_size;

            // Set all execution options for this configuration
            Node exec_opts;
            exec_opts["execution_location"].set(config.exec_location);
            exec_opts["output_location"].set(config.output_location);
            exec_opts["sync_strategy"].set("sync");
            execution::execution_set_options(exec_opts);

            // Build the input once, outside the timed regions
            Node input;
            setup(config, input);

            // Execute `run` `warmup` times
            {
                // Scope this separately to make it easier to disregard
                // warmup iterations in the timing output
                CONDUIT_ANNOTATE_MARK_SCOPE("warmup");
                for (index_t i = 0; i < warmup; i++)
                {
                    run(input);
                }
            }

            // Execute `run` `iterations` times
            {
                const std::string scope_name = std::string(name)
                    + "_" + execution::get_execution_policy().policy_name()
                    + "_dim-"  + std::to_string(dim_size)
                    + "_src-"  + config.src_location
                    + "_exec-" + config.exec_location
                    + "_out-"  + config.output_location
                    + "_iter-" + std::to_string(iterations);
                CONDUIT_ANNOTATE_MARK_SCOPE(scope_name.c_str());
                for (index_t i = 0; i < iterations; i++)
                {
                    run(input);
                }
            }

            execution::reset_execution_options();
        }
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
