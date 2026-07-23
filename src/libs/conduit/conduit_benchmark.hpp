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
#include "conduit_node.hpp"

// This is not needed after device support is added back
#if defined(CONDUIT_USE_OPENMP)
#include <omp.h>
#endif

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
};

//-----------------------------------------------------------------------------
inline
std::vector<ExecConfig>
get_exec_configs()
{
    // In the pre-device execution model world, we don't have the concept of
    // host vs device execution
    std::vector<ExecConfig> configs{
        //source location, execution location, output location
        {"host",           "host",             "host"},
    };
    return configs;
}

//-----------------------------------------------------------------------------
template <typename SetupFn, typename RunFn>
void
exec(const std::string &name,
     SetupFn &&setup,
     RunFn &&run,
     const index_t warmup,
     const index_t iterations,
     const std::vector<index_t> &dim_sizes)
{
    const std::vector<ExecConfig> configs = get_exec_configs();


    // Benchmark each possible configuration
    for (const ExecConfig &config : configs)
    {
        // Benchmark each data size
        for (const auto &npts : dim_sizes)
        {
            // Build the input mesh once, outside the timed regions
            Node input;
            setup(config, npts, input);

            // The output node, reused across every iteration. It
            // gets reset() immediately before each call, so run()
            // always receives an empty node.
            Node dst;

            // Execute `run` `warmup` times
            {
                // Scope this separately to make it easier to disregard
                // warmup iterations in the timing output
                CONDUIT_ANNOTATE_MARK_SCOPE("warmup");
                for (index_t i = 0; i < warmup; i++)
                {
                    run(input, dst);
                    dst.reset();
                }
            }

            // Execute `run` `iterations` times
            {
                // This scope name is used to identify specific benchmarks in
                // the Caliper output and identify their attributes
                // (dim size, policy, etc.)
                const std::string scope_name = name
                    + "_dim-"  + std::to_string(npts)
                    + "_src-"  + config.src_location
                    + "_exec-" + config.exec_location
                    + "_out-"  + config.output_location
#if defined(CONDUIT_USE_OPENMP)
                    + "_threads-" + std::to_string(omp_get_max_threads())
#endif
                    + "_iter-" + std::to_string(iterations);
                CONDUIT_ANNOTATE_MARK_SCOPE(scope_name.c_str());
                for (index_t i = 0; i < iterations; i++)
                {
                    run(input, dst);
                    dst.reset();
                }
            }
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
