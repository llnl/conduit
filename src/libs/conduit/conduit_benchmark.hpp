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
#include <iomanip>
#include <sstream>
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
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S");
    return oss.str();
}

//-----------------------------------------------------------------------------
struct ExecConfig
{
    std::string src_location;
    std::string exec_location;
    std::string output_location;
    std::string sync_strategy;
    index_t dim_size = 1;
};

//-----------------------------------------------------------------------------
inline
std::vector<ExecConfig>
get_exec_configs()
{
    /*
    The device execution model includes the concept of an execution and
    output location, which determines whether host or device memory gets
    used for computing and storing a result respectively. This implies
    that data must sometimes be copied to/from memory spaces so that it
    is in the correct location at the correct time.

    It does not include the concept of source location, which can be
    thought of as the memory space in which data originates before
    execution. We have that concept here to help us determine which
    memory space the initial data should live in before starting the
    benchmark.

    For example: a host->device->host configuration implies that the
    input data lives in host memory to start off, which is its source
    location. The execution location is device memory but the input
    data is on the host, so the input must be copied to device memory
    before we can execute there. The output location is host memory,
    requiring that we perform a final data transfer.

    Data transfer overhead is non-existent in the host->host->host and
    device->device->device configurations.

    The sync strategy determines how the result of that final data
    transfer gets moved from the accessor's working buffer into the
    destination Node: "sync" copies the data back, preserving the
    destination's original allocation/location, while "assume" instead
    hands the working buffer to the destination Node directly, avoiding
    a copy but potentially leaving the result in a different memory
    space than requested. The two strategies only behave differently
    when the execution location differs from the output location
    (otherwise there is no working buffer to move, and "assume" would be
    a redundant no-op identical to "sync"), so we only benchmark both
    strategies for the configurations where they can diverge.
    */
    std::vector<ExecConfig> configs{
        //source location, execution location, output location, sync strategy
        {"host",           "host",             "host",          "sync"},
#if defined(CONDUIT_USE_DEVICE)
        {"host",           "host",             "device",        "sync"},
        {"host",           "host",             "device",        "assume"},
        {"host",           "device",           "host",          "sync"},
        {"host",           "device",           "host",          "assume"},
        {"host",           "device",           "device",        "sync"},
        {"device",         "host",             "host",          "sync"},
        {"device",         "host",             "device",        "sync"},
        {"device",         "host",             "device",        "assume"},
        {"device",         "device",           "host",          "sync"},
        {"device",         "device",           "host",          "assume"},
        {"device",         "device",           "device",        "sync"},
#endif
    };
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
            exec_opts["sync_strategy"].set(config.sync_strategy);
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
                // This scope name is used to identify specific benchmarks in
                // the Caliper output and identify their attributes
                // (dim size, policy, etc.)
                const std::string scope_name = std::string(name)
                    + "_" + execution::get_execution_policy().policy_name()
                    + "_dim-"  + std::to_string(dim_size)
                    + "_src-"  + config.src_location
                    + "_exec-" + config.exec_location
                    + "_out-"  + config.output_location
                    + "_sync-" + config.sync_strategy
#if defined(CONDUIT_USE_OPENMP)
                    + "_threads-" + std::to_string(omp_get_max_threads())
#endif
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
